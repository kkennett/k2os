//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
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
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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

#include "kern.h"

void
KernDbg_Emitter(
    void *  apContext,
    char    aCh
)
{
    gData.mpShared->FuncTab.DebugOut(aCh);
}

UINT32 
KernDbg_OutputWithArgs(
    char const *apFormat, 
    VALIST      aList
)
{
    UINT32  result;
    BOOL    disp;

    if (gData.mpShared->FuncTab.DebugOut == NULL)
        return 0;

    disp = K2OSKERN_SeqLock(&gData.Debug.SeqLock);
    
    result = K2ASC_Emitf(KernDbg_Emitter, NULL, (UINT32)-1, apFormat, aList);
    
    K2OSKERN_SeqUnlock(&gData.Debug.SeqLock, disp);

    return result;
}

UINT32
K2OSKERN_Debug(
    char const *apFormat,
    ...
)
{
    VALIST  vList;
    UINT32  result;

    K2_VASTART(vList, apFormat);

    result = KernDbg_OutputWithArgs(apFormat, vList);

    K2_VAEND(vList);

    return result;
}

UINT_PTR
K2_CALLCONV_REGS
K2OS_Debug_OutputString(
    char const *apString
)
{
    UINT_PTR    result;
    char        ch;

    if (gData.mpShared->FuncTab.DebugOut == NULL)
        return 0;

    result = 0;

    do {
        ch = *apString;
        if (0 == ch)
            break;
        KernDbg_Emitter(NULL, ch);
        result++;
        apString++;
    } while (1);

    return result;
}

UINT32
KernDbg_IntsOff_Enter(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    UINT32 mask;

    if (!apThisCore->mInDebugMode)
    {
        KTRACE(apThisCore, 1, KTRACE_CORE_ENTERED_DEBUG);
        apThisCore->mInDebugMode = TRUE;
        mask = K2ATOMIC_Or((UINT32 volatile *)&gData.Debug.mCoresInDebugMode, 1 << apThisCore->mCoreIx);
//        K2OSKERN_Debug("Core %d entered debug mode\n", apThisCore->mCoreIx);
    }
    else
    {
        mask = (UINT32)-1;
    }

    return mask;
}

void
KernDbg_Enter(
    K2OSKERN_CPUCORE volatile * apThisCore
    )
{
    BOOL disp;

    disp = K2OSKERN_SetIntr(FALSE);
    KernDbg_IntsOff_Enter(apThisCore);
    K2OSKERN_SetIntr(disp);
}

void   
KernDbg_RecvSlaveCommand(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_DBG_COMMAND *      apRecvCmd
)
{
    KernDbg_Enter(apThisCore);

    K2_ASSERT(apThisCore->mCoreIx != gData.Debug.mLeadCoreIx);

    //
    // process slave command here
    //

    //
    // let initiator know we have processed this command
    //
    K2ATOMIC_Or(&apRecvCmd->mProcessedMask, (1 << apThisCore->mCoreIx));
}

void
KernDbg_Entered(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    KTRACE(apThisCore, 1, KTRACE_DEBUG_ENTERED);
    K2OSKERN_Debug("\n\nSYSTEM STOPPED FOR DEBUG\n\n");
}

void
KernDbg_EntryDpc_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    KTRACE(apThisCore, 1, KTRACE_DEBUG_ENTER_CHECK_DPC);

    if (gData.Debug.mCoresInDebugMode == ((1 << gData.mCpuCoreCount) - 1))
    {
        KernDbg_Entered(apThisCore);
        return;
    }

    gData.Debug.EnterDpc.Func = KernDbg_EntryDpc_CheckComplete;
    KernCpu_QueueDpc(&gData.Debug.EnterDpc.Dpc, &gData.Debug.EnterDpc.Func, KernDpcPrio_Hi);
}

void
KernDbg_EntryDpc_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    UINT32  sentMask;

    KTRACE(apThisCore, 1, KTRACE_DEBUG_ENTER_SENDICI_DPC);

    sentMask = KernArch_SendIci(
        apThisCore,
        gData.Debug.mIciSendMask,
        KernIci_DebugCmd,
        &gData.Debug.Command
    );

    gData.Debug.mIciSendMask &= ~sentMask;

    if (0 == gData.Debug.mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        gData.Debug.EnterDpc.Func = KernDbg_EntryDpc_CheckComplete;
    }
    else
    {
        gData.Debug.EnterDpc.Func = KernDbg_EntryDpc_SendIci;
    }
    KernCpu_QueueDpc(&gData.Debug.EnterDpc.Dpc, &gData.Debug.EnterDpc.Func, KernDpcPrio_Hi);
}

void    
KernDbg_BeginModeEntry(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    BOOL    disp;
    UINT32  coreBit;

    disp = K2OSKERN_SetIntr(FALSE);
    if (!apThisCore->mInDebugMode)
    {
        coreBit = 1 << apThisCore->mCoreIx;
        if (coreBit == KernDbg_IntsOff_Enter(apThisCore))
        {
            //
            // we are lead core for debug entry
            //
            gData.Debug.mLeadCoreIx = apThisCore->mCoreIx;
            if (gData.mCpuCoreCount > 1)
            {
                gData.Debug.mIciSendMask = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
                KernDbg_EntryDpc_SendIci(apThisCore, &gData.Debug.EnterDpc.Func);
            }
            else
            {
                KernDbg_Entered(apThisCore);
            }
        }
    }
    K2OSKERN_SetIntr(disp);
}

BOOL    
KernDbg_Attached(
    void
)
{
    return TRUE;
}

void
KernDbg_InByte(
    UINT8 aByte
)
{
    if (aByte < 32)
    {
        K2OSKERN_Debug("DBG[%d]\n", aByte);
    }
    else
    {
        K2OSKERN_Debug("DBG['%c']\n", aByte);
    }
}

void    
KernDbg_IoCheck(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    UINT32  chk;
    UINT8   recvByte;

    chk = gData.Debug.mCoresInDebugMode;
    if (0 == chk)
    {
        if (0 == apThisCore->mCoreIx)
        {
            //
            // poll for debug break here
            //
            if (gData.mpShared->FuncTab.DebugIn(&recvByte))
            {
                KernDbg_BeginModeEntry(apThisCore);
                KernDbg_InByte(recvByte);
            }
        }
    }
    else if (chk == ((1 << gData.mCpuCoreCount) - 1))
    {
        //
        // all cores in debug mode
        //
        if (apThisCore->mCoreIx == gData.Debug.mLeadCoreIx)
        {
            //
            // lead core should check for debug io with host
            //
            if (gData.mpShared->FuncTab.DebugIn(&recvByte))
            {
                KernDbg_InByte(recvByte);
            }
        }
    }
}

