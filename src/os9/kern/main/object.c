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

static UINT_PTR const sgObjSizes[] =
{
    0,                                       //KernObjType_Invalid = 0,
    sizeof(K2OSKERN_OBJ_PROCESS),            //KernObj_Process,        // 1
    sizeof(K2OSKERN_OBJ_THREAD),             //KernObj_Thread,         // 2
    sizeof(K2OSKERN_OBJ_PAGEARRAY),          //KernObj_PageArray,      // 3
    sizeof(K2OSKERN_OBJ_VIRTMAP),            //KernObj_VirtMap,        // 4
    sizeof(K2OSKERN_OBJ_NOTIFY),             //KernObj_Notify,         // 5
    sizeof(K2OSKERN_OBJ_GATE),               //KernObj_Gate,           // 6
    sizeof(K2OSKERN_OBJ_ALARM),              //KernObj_Alarm,          // 7
    sizeof(K2OSKERN_OBJ_SEM),                //KernObj_Sem,            // 8
    sizeof(K2OSKERN_OBJ_SEMUSER),            //KernObj_SemUser,        // 9 
    sizeof(K2OSKERN_OBJ_INTERRUPT),          //KernObj_Interrupt,      // 10
    sizeof(K2OSKERN_OBJ_MAILBOX),            //KernObj_Mailbox,        // 11
    sizeof(K2OSKERN_OBJ_MAILBOXOWNER),       //KernObj_MailboxOwner,   // 12
    sizeof(K2OSKERN_OBJ_MAILSLOT),           //KernObj_Mailslot,       // 13
    sizeof(K2OSKERN_OBJ_IFINST),             //KernObj_IfInst,         // 14
    sizeof(K2OSKERN_OBJ_IFENUM),             //KernObj_IfEnum,         // 15
    sizeof(K2OSKERN_OBJ_IFSUBS),             //KernObj_IfSubs,         // 16
    sizeof(K2OSKERN_OBJ_IPCEND),             //KernObj_IpcEnd,         // 17
    sizeof(K2OSKERN_OBJ_NOTIFYPROXY)         //KernObj_NotifyProxy,    // 18
};
K2_STATIC_ASSERT(sizeof(sgObjSizes) == (sizeof(UINT_PTR) * KernObjType_Count));

void    
KernObj_Init(
    void
)
{
    UINT_PTR ix;

    K2OSKERN_SeqInit(&gData.Obj.SeqLock);

    for (ix = 0; ix < KernObjType_Count; ix++)
    {
        K2LIST_Init(&gData.Obj.Cache[ix]);
    }
}

K2OSKERN_OBJ_HEADER *
KernObj_Alloc(
    KernObjType aObjType
)
{
    K2LIST_ANCHOR *         pList;
    K2LIST_LINK *           pListLink;
    BOOL                    disp;
    K2OSKERN_OBJ_HEADER *   pObjHdr;
    UINT32                  virtAddr;
    UINT32                  physAddr;
    K2OSKERN_PHYSRES        res;
    K2LIST_ANCHOR           physTrack;
    K2STAT                  stat;
    UINT32                  objSize;

    K2_ASSERT((aObjType > KernObjType_Invalid) && (aObjType < KernObjType_Count));

    pObjHdr = NULL;
    pList = &gData.Obj.Cache[aObjType];
    objSize = (sgObjSizes[aObjType] + 4) & ~3;

    disp = K2OSKERN_SeqLock(&gData.Obj.SeqLock);

    pListLink = pList->mpHead;

    if (NULL == pListLink)
    {
        if (KernPhys_Reserve_Init(&res, 1))
        {
            virtAddr = KernVirt_Reserve(1);
            if (0 != virtAddr)
            {
                stat = KernPhys_AllocSparsePages(&res, 1, &physTrack);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                physAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)physTrack.mpHead);
                KernPte_MakePageMap(NULL, virtAddr, physAddr, K2OS_MAPTYPE_KERN_DATA);

                physAddr = K2_VA_MEMPAGE_BYTES / objSize;
                do
                {
                    K2LIST_AddAtTail(pList, (K2LIST_LINK *)virtAddr);
                    virtAddr += objSize;
                } while (--physAddr);

                // take the first one
                pListLink = pList->mpHead;
                K2_ASSERT(NULL != pListLink);
            }
            else
            {
                KernPhys_Reserve_Release(&res);
            }
        }
    }

    if (NULL != pListLink)
    {
        K2LIST_Remove(pList, pListLink);
    }

    K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);

    if (NULL == pListLink)
        return NULL;

    pObjHdr = (K2OSKERN_OBJ_HEADER *)pListLink;

    K2MEM_Zero(pObjHdr, sgObjSizes[aObjType]);
    pObjHdr->mObjType = aObjType;
    K2LIST_Init(&pObjHdr->RefObjList);

    return pObjHdr;
}

void
KernObj_Free(
    K2OSKERN_OBJ_HEADER *apObjHdr
)
{
    KernObjType objType;
    BOOL        disp;

    objType = apObjHdr->mObjType;

    K2_ASSERT((objType > KernObjType_Invalid) && (objType < KernObjType_Count));
    K2_ASSERT(0 == apObjHdr->RefObjList.mNodeCount);
    K2_ASSERT(0 == apObjHdr->mTokenCount);

    if (KernObj_PageArray == apObjHdr->mObjType)
    {
        disp = (KernPageArray_Sparse == ((K2OSKERN_OBJ_PAGEARRAY *)apObjHdr)->mPageArrayType) ? TRUE : FALSE;
    }
    else
    {
        disp = FALSE;
    }

    K2MEM_Zero(apObjHdr, sgObjSizes[objType]);

    if (disp)
    {
        disp = KernHeap_Free(apObjHdr);
        K2_ASSERT(disp);
    }
    else
    {
        disp = K2OSKERN_SeqLock(&gData.Obj.SeqLock);

        K2LIST_AddAtTail(&gData.Obj.Cache[objType], (K2LIST_LINK *)apObjHdr);

        K2OSKERN_SeqUnlock(&gData.Obj.SeqLock, disp);
    }
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

    K2_ASSERT(NULL != apRef);
    K2_ASSERT(NULL != apTarget);

    disp = K2OSKERN_SeqLock(&gData.Obj.SeqLock);

    if (0 != (apTarget->mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO))
    {
        K2OSKERN_Panic("\n\n****Ref taken on object after dec to zero\n\n");
    }

    K2_ASSERT(apRef->AsAny == NULL);
    apRef->AsAny = apTarget;
#if DEBUG_REF
    apRef->mpFile = apFile;
    apRef->mLine = aLine;
    K2OSKERN_Debug("o %08X %s %08X %d->%d %s:%d\n", apTarget, KernObj_Name(apTarget->mObjType), apRef, apTarget->RefObjList.mNodeCount, apTarget->RefObjList.mNodeCount + 1, apFile, aLine);
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

#if DEBUG_REF
UINT32 
KernObj_DebugReleaseRef(
    K2OSKERN_OBJREF *   apRef, 
    char const *        apFile, 
    int                 aLine
)
#else
UINT32 
KernObj_ReleaseRef(
    K2OSKERN_OBJREF * apRef
)
#endif
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    BOOL                        disp;
    UINT32                      result;
    K2OSKERN_OBJ_HEADER *       pHdr;
    BOOL                        doEnterMonitor;
    K2OSKERN_OBJ_THREAD *       pThisThread;

    K2_ASSERT(NULL != apRef);

    disp = K2OSKERN_SeqLock(&gData.Obj.SeqLock);

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

    pHdr = apRef->AsAny;
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

#if DEBUG_REF
    K2OSKERN_Debug("o %08X %s %08X %d->%d %s:%d\n", pHdr, KernObj_Name(pHdr->mObjType), apRef, pHdr->RefObjList.mNodeCount, pHdr->RefObjList.mNodeCount - 1, apFile, aLine);
#endif

    K2_ASSERT(pHdr->RefObjList.mNodeCount != 0);
    ValidateList(&pHdr->RefObjList);
    K2LIST_Remove(&pHdr->RefObjList, &apRef->RefObjListLink);
    ValidateList(&pHdr->RefObjList);
    apRef->AsAny = NULL;

#if SENTINEL_REF
    apRef->mSentinel0 = 0xABABABAB;
    apRef->mSentinel1 = 0xCDCDCDCD;
    apRef->mSentinel2 = 0xEFEFEFEF;
    apRef->mSentinel3 = 0x0ABCDEF1;
    apRef->mpRefList = NULL;
#endif

    doEnterMonitor = FALSE;
    if (0 == (pHdr->mObjFlags & K2OSKERN_OBJ_FLAG_PERMANENT))
    {
        result = pHdr->RefObjList.mNodeCount;
        if (0 == result)
        {
            //
            // dpcs do not operate outside the monitor
            //
            if (!pThisCore->mIsInMonitor)
            {
                //
                // release is being done from a thread. if interrupts were off we are hosed
                //
                K2_ASSERT(disp);
                //
                // interrupts stay off when we exit the seqlock below
                // otherwise the core may change as this is a thread
                //
                disp = FALSE;
                doEnterMonitor = TRUE;
            }

            pHdr->mObjFlags |= K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO;
            K2_CpuWriteBarrier();
        }
#if 0
        else
        {
            if (KernObj_Process == pHdr->mObjType)
            {
                K2LIST_LINK *pListLink;
                K2OSKERN_OBJREF *pRef;

                K2OSKERN_Debug("    PROCESS\n");

                pListLink = pHdr->RefObjList.mpHead;
                do
                {
                    pRef = K2_GET_CONTAINER(K2OSKERN_OBJREF, pListLink, RefObjListLink);
                    pListLink = pListLink->mpNext;
                    K2OSKERN_Debug("    r %08X %s:%d\n", pRef, pRef->mpFile, pRef->mLine);
                } while (NULL != pListLink);


            }
        }
#endif
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
        if (doEnterMonitor)
        {
            //
            // interrupts will still be off
            //
            K2_ASSERT(!K2OSKERN_GetIntr());

            pThisThread = pThisCore->mpActiveThread;
            K2_ASSERT(pThisThread->mIsKernelThread);

            KernArch_IntsOff_SaveKernelThreadStateAndEnterMonitor(pThisCore, pThisThread);

            //
            // interrupts will be on when we return
            //
            K2_ASSERT(K2OSKERN_GetIntr());
        }
    }

    return result;
}

typedef void (*K2OSKERN_pf_Cleanup)(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_HEADER *apHdr);

static const K2OSKERN_pf_Cleanup sgCleanupFunc[] =
{
    NULL,                   // Error
    (K2OSKERN_pf_Cleanup)KernProc_Cleanup,          // KernObj_Process,        // 1
    (K2OSKERN_pf_Cleanup)KernThread_Cleanup,        // KernObj_Thread,         // 2
    (K2OSKERN_pf_Cleanup)KernPageArray_Cleanup,     // KernObj_PageArray,      // 3
    (K2OSKERN_pf_Cleanup)KernVirtMap_Cleanup,       // KernObj_VirtMap,        // 4
    (K2OSKERN_pf_Cleanup)KernNotify_Cleanup,        // KernObj_Notify,         // 5
    (K2OSKERN_pf_Cleanup)KernGate_Cleanup,          // KernObj_Gate,           // 6
    (K2OSKERN_pf_Cleanup)KernAlarm_Cleanup,         // KernObj_Alarm,          // 7
    (K2OSKERN_pf_Cleanup)KernSem_Cleanup,           // KernObj_Sem,            // 8
    (K2OSKERN_pf_Cleanup)KernSemUser_Cleanup,       // KernObj_SemUser         // 9 
    (K2OSKERN_pf_Cleanup)KernIntr_Cleanup,          // KernObj_Intrerrupt,     // 10
    (K2OSKERN_pf_Cleanup)KernMailbox_Cleanup,       // KernObj_Mailbox,        // 11
    (K2OSKERN_pf_Cleanup)KernMailboxOwner_Cleanup,  // KernObj_MailboxOwner,   // 12
    (K2OSKERN_pf_Cleanup)KernMailslot_Cleanup,      // KernObj_Mailslot,       // 13
    (K2OSKERN_pf_Cleanup)KernIfInst_Cleanup,        // KernObj_IfInst,         // 14
    (K2OSKERN_pf_Cleanup)KernIfEnum_Cleanup,        // KernObj_IfEnum,         // 15
    (K2OSKERN_pf_Cleanup)KernIfSubs_Cleanup,        // KernObj_IfSubs,         // 16
    (K2OSKERN_pf_Cleanup)KernIpcEnd_Cleanup,        // KernObj_IpcEnd,         // 17
    (K2OSKERN_pf_Cleanup)KernNotifyProxy_Cleanup,   // KernObj_NotifyProxy     // 18
};
K2_STATIC_ASSERT(sizeof(sgCleanupFunc) == (KernObjType_Count * sizeof(K2OSKERN_pf_Cleanup)));

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
    "VirtMap",
    "Notify",
    "Gate",
    "Alarm",
    "Sem",
    "SemUser",
    "Interrupt",
    "Mailbox",
    "MailboxOwner",
    "Mailslot",
    "IfInst",
    "IfEnum",
    "IfSubs",
    "IpcEnd",
    "NotifyProxy",
};

char const * const  
KernObj_Name(
    KernObjType aType
)
{
    if ((aType == KernObjType_Invalid) ||
        (aType >= KernObjType_Count))
        return sgpNoName;
    return sgNames[aType];
}

K2STAT              
KernObj_Share(
    K2OSKERN_OBJ_PROCESS *  apSrcProc,
    K2OSKERN_OBJ_HEADER *   apObjHdr,
    K2OSKERN_OBJ_PROCESS *  apTargetProc,
    UINT32 *                apRetTokenValue
)
{
    K2STAT stat;

    K2_ASSERT(apSrcProc != apTargetProc);

    stat = K2STAT_ERROR_NOT_SHAREABLE;
    *apRetTokenValue = 0;

    switch (apObjHdr->mObjType)
    {
    case KernObj_Process:
    case KernObj_Thread:
    case KernObj_PageArray:
    case KernObj_Notify:
    case KernObj_Gate:
    case KernObj_Alarm:
        if (NULL == apTargetProc)
        {
            stat = KernToken_Create(apObjHdr, (K2OS_TOKEN *)apRetTokenValue);
        }
        else
        {
            stat = KernProc_TokenCreate(apTargetProc, apObjHdr, (K2OS_TOKEN *)apRetTokenValue);
        }
        break;

    case KernObj_SemUser:
        stat = KernSem_Share(((K2OSKERN_OBJ_SEMUSER *)apObjHdr)->SemRef.AsSem, apTargetProc, apRetTokenValue);
        break;

    case KernObj_MailboxOwner:
        stat = KernMailbox_Share(((K2OSKERN_OBJ_MAILBOXOWNER *)apObjHdr)->RefMailbox.AsMailbox, apTargetProc, apRetTokenValue);
        break;

    case KernObj_Mailslot:
        stat = KernMailbox_Share(((K2OSKERN_OBJ_MAILSLOT *)apObjHdr)->RefMailbox.AsMailbox, apTargetProc, apRetTokenValue);
        break;

    default:
        K2OSKERN_Debug("Proc %d attempt to share token of type %d\n", (NULL == apSrcProc) ? 0 : apSrcProc->mId, apObjHdr->mObjType);
        break;
    }

    return stat;
}
