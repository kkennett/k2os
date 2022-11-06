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

#define REF_SENTINEL_0  0xFEEDF00D
#define REF_SENTINEL_1  0xDEADBEEF
#define REF_SENTINEL_2  0xCAFEF00D
#define REF_SENTINEL_3  0xD00FDEAD

#if DEBUG_REF

void
ValidateList(
    K2LIST_ANCHOR *apAnchor
)
{
    K2LIST_LINK * pLink;
    K2LIST_LINK * pDump;

    K2_ASSERT(((apAnchor->mpHead == NULL) && (apAnchor->mpTail == NULL)) || ((apAnchor->mpHead != NULL) && (apAnchor->mpTail != NULL)));
    K2_ASSERT((apAnchor->mNodeCount == 0) || ((apAnchor->mpHead != NULL) && (apAnchor->mpTail != NULL)));
    if (apAnchor->mNodeCount == 0)
        return;
    pLink = apAnchor->mpHead;
    K2_ASSERT(pLink->mpPrev == NULL);
    do {
        if (pLink->mpNext == NULL)
            break;
        if (pLink->mpNext->mpPrev != pLink)
        {
            pDump = apAnchor->mpHead;
            do {
                K2OSKERN_Debug(" %08X p%08X n%08X\n", pDump, pDump->mpPrev, pDump->mpNext);
                pDump = pDump->mpNext;
            } while (NULL != pDump);
        }
        K2_ASSERT(pLink->mpNext->mpPrev == pLink);
        pLink = pLink->mpNext;
    } while (1);
    K2_ASSERT(pLink == apAnchor->mpTail);
}

#else
#define ValidateList(x)
#endif

void    
KernObj_Init(
    void
)
{
    K2OSKERN_SeqInit(&gData.Obj.SeqLock);
}

#if DEBUG_REF
void 
KernObj_DebugCreateRef(
    K2OSKERN_OBJREF *       apRef,
    K2OSKERN_OBJ_HEADER *   apTarget,
    char const *            apFile,
    int                     aLine
)
#else
void
KernObj_CreateRef(
    K2OSKERN_OBJREF *       apRef,
    K2OSKERN_OBJ_HEADER *   apTarget
)
#endif
{
    BOOL disp;

    disp = K2OSKERN_SeqLock(&gData.Obj.SeqLock);

    if (0 != (apTarget->mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO))
    {
        K2OSKERN_Panic("\n\n****Ref taken on object after dec to zero\n\n");
    }

    K2_ASSERT(apRef->Ptr.AsHdr == NULL);
    apRef->Ptr.AsHdr = apTarget;
#if DEBUG_REF
    apRef->mpFile = apFile;
    apRef->mLine = aLine;
#endif

    ValidateList(&apTarget->RefObjList);
    K2LIST_AddAtTail(&apTarget->RefObjList, &apRef->RefObjListLink);
    ValidateList(&apTarget->RefObjList);

#if SENTINEL_REF
    apRef->mSentinel0 = REF_SENTINEL_0;
    apRef->mSentinel1 = REF_SENTINEL_1;
    apRef->mSentinel2 = REF_SENTINEL_2;
    apRef->mSentinel3 = REF_SENTINEL_3;
    apRef->mpRefList = &apTarget->RefObjList;
#endif

    K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);
}

void
KernObj_CleanupDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
);

UINT32 
KernObj_ReleaseRef(
    K2OSKERN_OBJREF * apRef
)
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    BOOL                        disp;
    UINT32                      result;
    K2OSKERN_OBJ_HEADER *       pHdr;

    disp = K2OSKERN_SeqLock(&gData.Obj.SeqLock);

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

    pHdr = apRef->Ptr.AsHdr;
    if (NULL == pHdr)
    {
        K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);
        K2OSKERN_Panic("***releasing null reference\n");
    }

#if SENTINEL_REF
    if (REF_SENTINEL_0 != apRef->mSentinel0)
    {
        K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);
        K2OSKERN_Panic("***Ref Sentinel 0 missing, corrupt or incorrect (0x%08X)\n", apRef->mSentinel0);
    }
    if (REF_SENTINEL_1 != apRef->mSentinel1)
    {
        K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);
        K2OSKERN_Panic("***Ref Sentinel 1 missing, corrupt or incorrect (0x%08X)\n", apRef->mSentinel1);
    }
    if (REF_SENTINEL_2 != apRef->mSentinel2)
    {
        K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);
        K2OSKERN_Panic("***Ref Sentinel 2 missing, corrupt or incorrect (0x%08X)\n", apRef->mSentinel2);
    }
    if (REF_SENTINEL_3 != apRef->mSentinel3)
    {
        K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);
        K2OSKERN_Panic("***Ref Sentinel 3 missing, corrupt or incorrect (0x%08X)\n", apRef->mSentinel3);
    }
    if (apRef->mpRefList != &pHdr->RefObjList)
    {
        K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);
        K2OSKERN_Panic("***Ref list ptr is wrong (0x%08X)\n", apRef->mpRefList);
    }
#endif

    K2_ASSERT(pHdr->RefObjList.mNodeCount != 0);
    ValidateList(&pHdr->RefObjList);
    K2LIST_Remove(&pHdr->RefObjList, &apRef->RefObjListLink);
    ValidateList(&pHdr->RefObjList);
    apRef->Ptr.AsHdr = NULL;

#if SENTINEL_REF
    apRef->mSentinel0 = 0xABABABAB;
    apRef->mSentinel1 = 0xCDCDCDCD;
    apRef->mSentinel2 = 0xEFEFEFEF;
    apRef->mSentinel3 = 0x0ABCDEF1;
    apRef->mpRefList = NULL;
#endif

    if (0 == (pHdr->mObjFlags & K2OSKERN_OBJ_FLAG_PERMANENT))
    {
        result = pHdr->RefObjList.mNodeCount;
        if (0 == result)
        {
            //
            // unless also aquired outside monitor, 
            // object references can only be released by monitor
            // since dpcs do not operate outside of it
            // 
            // if this fires then you are releasing a reference
            // outside the monitor that you got inside the monitor
            //
            K2_ASSERT(pThisCore->mIsInMonitor);

            pHdr->mObjFlags |= K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO;
            K2_CpuWriteBarrier();
        }
    }
    else
    {
        result = 0x7FFFFFFF;
    }

    K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);

    if (0 == result)
    {
//        K2OSKERN_Debug("Refs on non-permanent Object %08X type %s hit zero\n", pHdr, KernObj_Name(pHdr->mObjType));
        pHdr->ObjDpc.Func = KernObj_CleanupDpc;
        KernCpu_QueueDpc(&pHdr->ObjDpc.Dpc, &pHdr->ObjDpc.Func, KernDpcPrio_Hi);
    }

    return result;
}

typedef void (*K2OSKERN_pf_Cleanup)(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_HEADER *apHdr);

static const K2OSKERN_pf_Cleanup sgCleanupFunc[] =// KernObj_Count
{
    NULL,                   // Error
    (K2OSKERN_pf_Cleanup)KernProc_Cleanup,          // KernObj_Process,        // 1
    (K2OSKERN_pf_Cleanup)KernThread_Cleanup,        // KernObj_Thread,         // 2
    (K2OSKERN_pf_Cleanup)KernPageArray_Cleanup,     // KernObj_PageArray,      // 3
    (K2OSKERN_pf_Cleanup)KernMap_Cleanup,           // KernObj_Map,            // 4
    (K2OSKERN_pf_Cleanup)KernNotify_Cleanup,        // KernObj_Notify,         // 5
    (K2OSKERN_pf_Cleanup)KernGate_Cleanup,          // KernObj_Gate,           // 6
    (K2OSKERN_pf_Cleanup)KernAlarm_Cleanup,         // KernObj_Alarm,          // 7
    (K2OSKERN_pf_Cleanup)KernSem_Cleanup,           // KernObj_Sem,            // 8
    (K2OSKERN_pf_Cleanup)KernSemUser_Cleanup,       // KernObj_SemUser         // 9 
    (K2OSKERN_pf_Cleanup)KernMailbox_Cleanup,       // KernObj_Mailbox,        // 10
    (K2OSKERN_pf_Cleanup)KernMailslot_Cleanup,      // KernObj_Mailslot,       // 11
    (K2OSKERN_pf_Cleanup)KernIfEnum_Cleanup,        // KernObj_IfEnum,         // 12
    (K2OSKERN_pf_Cleanup)KernIfSubs_Cleanup,        // KernObj_IfSubs,         // 13
    (K2OSKERN_pf_Cleanup)KernIpcEnd_Cleanup,        // KernObj_IpcEnd,         // 14
    (K2OSKERN_pf_Cleanup)KernSignalProxy_Cleanup,   // KernObj_SignalProxy,    // 15
    (K2OSKERN_pf_Cleanup)KernIntr_Cleanup,          // KernObj_Intrerrupt,     // 16
};
K2_STATIC_ASSERT(sizeof(sgCleanupFunc) == (KernObj_Count * sizeof(K2OSKERN_pf_Cleanup)));

void
KernObj_CleanupDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_HEADER *   pObjHdr;
    K2OSKERN_pf_Cleanup     fCleanup;

    pObjHdr = K2_GET_CONTAINER(K2OSKERN_OBJ_HEADER, apArg, ObjDpc.Func);
//    K2OSKERN_Debug("CleanupDpc %08X type %s\n", pObjHdr, KernObj_Name(pObjHdr->mObjType));
    KTRACE(apThisCore, 2, KTRACE_OBJ_CLEANUP_DPC, pObjHdr->mObjType);
    fCleanup = sgCleanupFunc[pObjHdr->mObjType];
    K2_ASSERT(NULL != fCleanup);
    fCleanup(apThisCore, pObjHdr);
}

char const * const sgpNoName = "<ERROR>";

char const * const sgNames[] =
{
    NULL,
    "Process",
    "Thread",
    "PageArray",
    "Map",
    "Notify",
    "Gate",
    "Alarm",
    "Sem",
    "SemUser",
    "Mailbox",
    "Mailslot",
	"IfEnum",
	"IfSubs",
    "IpcEnd",
    "SignalProxy",
    "Interrupt"
};

char const * const  
KernObj_Name(
    KernObjType aType
)
{
    if ((aType == KernObj_Error) ||
        (aType >= KernObj_Count))
        return sgpNoName;
    return sgNames[aType];
}
