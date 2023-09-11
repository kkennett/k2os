//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2023, Kurt Kennett
//   All rights reserved.
//   
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//   
//   1. Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//   
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//   
//   3. Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//   
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS"AS IS"
//   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include"kern.h"

static char const * const sgEvtName[] =
{
"<NONE>",
"CORE_ENTER_IDLE            ",  
"CORE_LEAVE_IDLE            ",  
"CORE_RESUME_THREAD         ",  
"CORE_STOP_PROC             ",  
"CORE_ABORT_THREAD          ",  
"CORE_SWITCH_NOPROC         ",  
"CORE_RECV_ICI              ",  
"CORE_SUSPEND_THREAD        ",  
"CORE_PANIC_SPIN            ",  
"CORE_EXEC_DPC              ",  
"CORE_ENTER_SCHEDULER       ",  
"CORE_LEAVE_SCHEDULER       ",  
"THREAD_STOPPED             ",  
"THREAD_QUANTUM_EXPIRED     ",  
"THREAD_RUN                 ",  
"THREAD_EXCEPTION           ",  
"THREAD_SYSCALL             ",  
"THREAD_RAISE_EX            ",  
"THREAD_MIGRATE             ",  
"THREAD_EXIT                ",  
"THREAD_START               ",  
"THREAD_COW_SENDICI_DPC     ",  
"THREAD_COW_CHECK_DPC       ",  
"THREAD_COW_COPY_DPC        ",  
"THREAD_KERNPAGE_CHECK_DPC  ",  
"THREAD_KERNPAGE_SENDICI_DPC",  
"THREAD_USERPAGE_CHECK_DPC  ",  
"THREAD_USERPAGE_SENDICI_DPC",  
"PROC_START                 ",  
"PROC_BEGIN_STOP            ",  
"PROC_STOPPED               ",  
"PROC_TLBSHOOT_ICI          ",  
"PROC_STOP_DPC              ",  
"PROC_EXITED_DPC            ",  
"PROC_STOP_CHECK_DPC        ",  
"PROC_STOP_SENDICI_DPC      ",  
"PROC_TRANS_CHECK_DPC       ",  
"PROC_TRANS_SENDICI_DPC     ",  
"PROC_HIVIRT_CHECK_DPC      ",  
"PROC_HIVIRT_SENDICI_DPC    ",  
"PROC_LOVIRT_CHECK_DPC      ",  
"PROC_LOVIRT_SENDICI_DPC    ",  
"PROC_TOKEN_CHECK_DPC       ",  
"PROC_TOKEN_SENDICI_DPC     ",  
"TIMER_FIRED                ",  
"SCHED_EXEC_ITEM            ",  
"SCHED_EXEC_SYSCALL         ",  
"ALARM_POST_CLEANUP_DPC     ",  
"MAP_CLEAN_CHECK_DPC        ",  
"MAP_CLEAN_SENDICI_DPC      ",  
"OBJ_CLEANUP_DPC            ",  
"SEM_POST_CLEANUP_DPC       ",  
"KERN_TLBSHOOT_ICI          ",  
"PROC_TLBSHOOT_ICI_IGNORED  ",  
"THREAD_COW_COMPLETE        ",  
"MAP_CLEAN_DONE             ",  
"CORE_ENTERED_DEBUG         ",  
"DEBUG_ENTER_CHECK_DPC      ",  
"DEBUG_ENTER_SENDICI_DPC    ",  
"DEBUG_ENTERED              ",  
"THREAD_KERNMEM_TCHECK_DPC  ",  
"THREAD_KERNMEM_SEG_SHOOT_START_DPC",
"THREAD_KERNMEM_SEG_SHOOT_SEND_DPC",
"THREAD_KERNMEM_SEG_SHOOT_CHECK_DPC",
"THREAD_KERNMEM_SEG_SHOOT_DONE",
"THREAD_KERNMEM_TSEND_DPC   " 
};

#define POS_MASK        0x0000FFFF

K2_STATIC_ASSERT(POS_MASK >= 0x00000FFF);
#define TRACE_PAGES     ((POS_MASK+1) / K2_VA_MEMPAGE_BYTES)

static UINT32           sgBufferBase;
static UINT32           sgBufferEnd;
static UINT32 volatile  sgNext;
static UINT32 volatile  sgEnable;

void
KernTrace_Init(
    void
)
{
    K2OSKERN_PHYSRES    res;
    BOOL                ok;
    K2STAT              stat;
    UINT32              physAddr;
    UINT32              virtAddr;
    UINT32              left;
    K2LIST_ANCHOR       trackList;
    K2OSKERN_PHYSSCAN   scan;

    ok = KernPhys_Reserve_Init(&res, TRACE_PAGES);
    K2_ASSERT(ok);

    stat = KernPhys_AllocSparsePages(&res, TRACE_PAGES, &trackList);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    KernPhys_ScanInit(&scan, &trackList, 0);

    sgBufferBase = virtAddr = KernVirt_Reserve(TRACE_PAGES);
    K2_ASSERT(0 != virtAddr);

    left = TRACE_PAGES;
    do {
        physAddr = KernPhys_ScanIter(&scan);
        KernPte_MakePageMap(NULL, virtAddr, physAddr, K2OS_MAPTYPE_KERN_DATA);
        virtAddr += K2_VA_MEMPAGE_BYTES;
    } while (--left);

    K2MEM_Zero((void *)sgBufferBase, TRACE_PAGES * K2_VA_MEMPAGE_BYTES);
    sgBufferEnd = sgBufferBase + TRACE_PAGES * K2_VA_MEMPAGE_BYTES;

    sgNext = 0;

    sgEnable = 1;
}

BOOL
KernTrace_SetEnable(BOOL aEnable)
{
    UINT32 old;

    old = K2ATOMIC_Exchange(&sgEnable, aEnable ? 1 : 0);
    
    return old ? TRUE : FALSE;
}

void    
KernTrace(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT32                    aCount,
    ...
)
{
    VALIST      vList;
    UINT32      pos;
    UINT32 *    pOut;
    BOOL        disp;
    UINT64      hfTick;

    if (!sgEnable)
        return;

    K2_VASTART(vList, aCount);
    
    K2_ASSERT(aCount > 0);
    K2_ASSERT(aCount < 0x10000);

    KernArch_GetHfTimerTick(&hfTick);

    disp = K2OSKERN_SetIntr(FALSE);

    pos = POS_MASK & K2ATOMIC_AddExchange((INT32 volatile *)&sgNext, (aCount + 2) * sizeof(UINT32));

    pOut = (UINT32 *)(sgBufferBase + pos);

    *pOut = 0x5A000000 | (apThisCore->mCoreIx << 16) | aCount;
    pOut++;
    if (((UINT32)pOut) == sgBufferEnd)
    {
        pOut = (UINT32 *)sgBufferBase;
    }
    *pOut = *((UINT32 *)&hfTick);

    if (aCount > 0)
    {
        do {
            pOut++;
            if (((UINT32)pOut) == sgBufferEnd)
            {
                pOut = (UINT32 *)sgBufferBase;
            }
            *pOut = K2_VAARG(vList, UINT32);
        } while (--aCount);
    }

    K2OSKERN_SetIntr(disp);
}

void 
KernTrace_EventStr(
    UINT32 aEvent,
    UINT32 aCountArgs,
    UINT32 const *apArgs
)
{
    UINT32 ix;

    K2OSKERN_Debug("%s", sgEvtName[aEvent]);
    if (0 == aCountArgs)
        return;

    K2OSKERN_Debug(" ");
    ix = 0;
    do {
        K2OSKERN_Debug("%d", apArgs[ix]);
        if (++ix == aCountArgs)
            break;
        K2OSKERN_Debug(" ");
    } while (1);
}

void    
KernTrace_Dump(
    void
)
{
    BOOL        disp;
    UINT32      u;
    UINT64      t;
    UINT64      mst;
    UINT32 *    pOut;
    UINT32      coreChk;
    UINT32      a[12];
    UINT32      count;
    UINT32      ix;

    disp = KernTrace_SetEnable(FALSE);

    K2OSKERN_Debug("\n\n--------------------------\nTrace Buffer Dump\n");

    pOut = (UINT32 *)sgBufferBase;

    while (((*pOut) & 0xFF000000) != 0x5A000000)
    {
        pOut++;
    }

    do {
        u = *pOut;
        if ((u & 0xFF000000) != 0x5A000000)
            break;

        pOut++;
        if (sgBufferEnd == (UINT32)pOut)
            break;
        t = *pOut;
        KernTimer_MsTickFromHfTick(&mst, &t);

        count = u & 0xFFFF;
        for (ix = 0; ix < count; ix++)
        {
            pOut++;
            if (sgBufferEnd == (UINT32)pOut)
                break;
            a[ix] = *pOut;
        }
        if (ix < count)
            break;

        u &= 0x00FF0000;

        K2OSKERN_Debug("%d.%d,", mst, t);
        coreChk = 0;
        while (u != coreChk)
        {
            K2OSKERN_Debug(",");
            coreChk += 0x00010000;
        }
        KernTrace_EventStr(a[0], count - 1, &a[1]);
        while (coreChk != 0x00030000)
        {
            K2OSKERN_Debug(",");
            coreChk += 0x00010000;
        }
        K2OSKERN_Debug("\n");

        pOut++;
    } while (sgBufferEnd != (UINT32)pOut);

    K2OSKERN_Debug("--------------------------\n");

    KernTrace_SetEnable(disp);
}
