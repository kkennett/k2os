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

static KernUser_pf_SysCall sgSysCall[K2OS_SYSCALL_COUNT];

void 
KernThread_UnimplementedSysCall(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_Debug("**Unimplemented system call %d\n", apCurThread->mSysCall_Id);
    apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_NOT_IMPL;
    apCurThread->mSysCall_Result = 0;
}

void KernThread_SysCall_OutputDebug(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void KernThread_SysCall_RaiseException(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);

void
KernThread_Init(
    void
)
{
    UINT32 ix;

    for (ix = 0; ix < K2OS_SYSCALL_COUNT; ix++)
    {
        sgSysCall[ix] = KernThread_UnimplementedSysCall;
    }

    sgSysCall[K2OS_SYSCALL_ID_PROCESS_EXIT              ] = KernProc_SysCall_Exit;
    sgSysCall[K2OS_SYSCALL_ID_PROCESS_START             ] = KernProc_SysCall_AtStart;
    sgSysCall[K2OS_SYSCALL_ID_OUTPUT_DEBUG              ] = KernThread_SysCall_OutputDebug;
    sgSysCall[K2OS_SYSCALL_ID_RAISE_EXCEPTION           ] = KernThread_SysCall_RaiseException;
    sgSysCall[K2OS_SYSCALL_ID_TOKEN_DESTROY             ] = KernProc_SysCall_TokenDestroy;
    sgSysCall[K2OS_SYSCALL_ID_NOTIFY_CREATE             ] = KernNotify_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_NOTIFY_SIGNAL             ] = KernNotify_SysCall_Signal;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_WAIT               ] = KernWait_SysCall;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_EXIT               ] = KernThread_SysCall_Exit;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_CREATE             ] = KernThread_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_SETAFF             ] = KernThread_SysCall_SetAffinity;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_GETEXITCODE        ] = KernThread_SysCall_GetExitCode;
    sgSysCall[K2OS_SYSCALL_ID_PAGEARRAY_CREATE          ] = KernPageArray_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_PAGEARRAY_GETLEN          ] = KernPageArray_SysCall_GetLen;
    sgSysCall[K2OS_SYSCALL_ID_MAP_CREATE                ] = KernMap_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_MAP_ACQUIRE               ] = KernMap_SysCall_Acquire;
    sgSysCall[K2OS_SYSCALL_ID_MAP_ACQ_PAGEARRAY         ] = KernMap_SysCall_AcqPageArray;
    sgSysCall[K2OS_SYSCALL_ID_MAP_GET_INFO              ] = KernMap_SysCall_GetInfo;
    sgSysCall[K2OS_SYSCALL_ID_VIRT_RESERVE              ] = KernProc_SysCall_VirtReserve;
    sgSysCall[K2OS_SYSCALL_ID_VIRT_GET                  ] = KernProc_SysCall_VirtGet;
    sgSysCall[K2OS_SYSCALL_ID_VIRT_RELEASE              ] = KernProc_SysCall_VirtRelease;
    sgSysCall[K2OS_SYSCALL_ID_MAILBOX_CREATE            ] = KernMailbox_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_MAILBOX_CLOSE             ] = KernMailbox_SysCall_Close;
    sgSysCall[K2OS_SYSCALL_ID_MAILBOX_RECV_RES          ] = KernMailbox_SysCall_RecvRes;
    sgSysCall[K2OS_SYSCALL_ID_MAILSLOT_SEND             ] = KernMailslot_SysCall_Send;
    sgSysCall[K2OS_SYSCALL_ID_GET_LAUNCH_INFO           ] = KernProc_SysCall_GetLaunchInfo;
    sgSysCall[K2OS_SYSCALL_ID_TRAP_MOUNT                ] = KernProc_SysCall_TrapMount;
    sgSysCall[K2OS_SYSCALL_ID_TRAP_DISMOUNT             ] = KernProc_SysCall_TrapDismount;
    sgSysCall[K2OS_SYSCALL_ID_GET_SYSINFO               ] = KernInfo_SysCall_Get;
    sgSysCall[K2OS_SYSCALL_ID_GATE_CREATE               ] = KernGate_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_GATE_CHANGE               ] = KernGate_SysCall_Change;
    sgSysCall[K2OS_SYSCALL_ID_ALARM_CREATE              ] = KernAlarm_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_SEM_CREATE                ] = KernSemUser_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_SEM_INC                   ] = KernSemUser_SysCall_Inc;
    sgSysCall[K2OS_SYSCALL_ID_IFENUM_CREATE             ] = KernIfEnum_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_IFENUM_RESET              ] = KernIfEnum_SysCall_Reset;
    sgSysCall[K2OS_SYSCALL_ID_IFENUM_NEXT               ] = KernIfEnum_SysCall_Next;
	sgSysCall[K2OS_SYSCALL_ID_IFSUBS_CREATE             ] = KernIfSubs_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_IFINST_CREATE             ] = KernIfInst_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_IFINST_PUBLISH            ] = KernIfInst_SysCall_Publish;
    sgSysCall[K2OS_SYSCALL_ID_IFINST_REMOVE             ] = KernIfInst_SysCall_Remove;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_CREATE             ] = KernIpcEnd_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_ACCEPT             ] = KernIpcEnd_SysCall_Accept;
    sgSysCall[K2OS_SYSCALL_ID_IPCREQ_REJECT             ] = KernIpcEnd_SysCall_RejectRequest;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_SEND               ] = KernIpcEnd_SysCall_Send;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_MANUAL_DISCONNECT  ] = KernIpcEnd_SysCall_ManualDisconnect;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_CLOSE              ] = KernIpcEnd_SysCall_Close;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_REQUEST            ] = KernIpcEnd_SysCall_Request;
    sgSysCall[K2OS_SYSCALL_ID_INTR_SETENABLE            ] = KernIntr_SysCall_SetEnable;
    sgSysCall[K2OS_SYSCALL_ID_INTR_END                  ] = KernIntr_SysCall_End;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_PROCESS_LAUNCH       ] = KernProc_SysCall_Root_Launch;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_IOPERMIT_ADD         ] = KernProc_SysCall_Root_IoPermit_Add;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_PAGEARRAY_CREATEAT   ] = KernPageArray_SysCall_Root_CreateAt;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_TOKEN_EXPORT         ] = KernProc_SysCall_Root_TokenExport;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_PLATNODE_CREATE      ] = KernPlat_SysCall_Root_CreateNode;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_PLATNODE_ADDRES      ] = KernPlat_SysCall_Root_AddRes;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_PLATNODE_HOOKINTR    ] = KernPlat_SysCall_Root_HookIntr;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_GET_PROCEXIT         ] = KernProc_SysCall_GetExitCode;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_PROCESS_STOP         ] = KernProc_SysCall_Root_ProcessStop;
    sgSysCall[K2OS_SYSCALL_ID_ROOT_TOKEN_IMPORT         ] = KernProc_SysCall_Root_TokenImport;
    sgSysCall[K2OS_SYSCALL_ID_DEBUG_BREAK               ] = KernThread_SysCall_DebugBreak;
}

K2STAT  
KernThread_Create(
    K2OSKERN_OBJ_PROCESS *          apProc,
    K2OS_USER_THREAD_CONFIG const * apConfig,
    K2OSKERN_OBJ_THREAD **          appRetThread
)
{
    K2STAT                  stat;
    K2OSKERN_PHYSRES        res;
    K2OSKERN_OBJ_THREAD *   pNewThread;
    K2OSKERN_PHYSTRACK *    pTrack;
    UINT32 *                pThreadPtrs;
    BOOL                    disp;
    UINT32                  newThreadIx;
    UINT32                  physPageAddr;

    pNewThread = (K2OSKERN_OBJ_THREAD *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_THREAD));
    if (NULL == pNewThread)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    do
    {
        //
        // need thread page for this new thread
        //
        if (!KernPhys_Reserve_Init(&res, 1))
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }
        stat = KernPhys_AllocPow2Bytes(&res, K2_VA_MEMPAGE_BYTES, &pTrack);
        if (K2STAT_IS_ERROR(stat))
        {
            KernPhys_Reserve_Release(&res);
            break;
        }

        //
        // we should be good from this point on unless we run out of thread indexes
        //
        pThreadPtrs = ((UINT32 *)K2OS_KVA_THREADPTRS_BASE);

        K2MEM_Zero(pNewThread, sizeof(K2OSKERN_OBJ_THREAD));
        pNewThread->Hdr.mObjType = KernObj_Thread;
        K2LIST_Init(&pNewThread->Hdr.RefObjList);
        pNewThread->mState = KernThreadState_Init;

        KernObj_CreateRef(&pNewThread->ProcRef, &apProc->Hdr);
        do
        {
            newThreadIx = gData.Proc.mNextThreadSlotIx;
        } while (newThreadIx != K2ATOMIC_CompareExchange(&gData.Proc.mNextThreadSlotIx, pThreadPtrs[newThreadIx], newThreadIx));
        K2_ASSERT(newThreadIx != 0);
        K2_ASSERT(newThreadIx < K2OS_MAX_THREAD_COUNT);
        K2_ASSERT(0 != gData.Proc.mNextThreadSlotIx);

        pNewThread->mGlobalIx = newThreadIx;
        pThreadPtrs[newThreadIx] = (UINT32)pNewThread;

        pNewThread->mpKernRwViewOfUserThreadPage = (K2OS_USER_THREAD_PAGE *)
            (K2OS_KVA_TLSAREA_BASE + (newThreadIx * K2_VA_MEMPAGE_BYTES));

        disp = K2OSKERN_SeqLock(&apProc->PageList.SeqLock);
        K2_ASSERT(pTrack->Flags.Field.PageListIx == KernPhysPageList_None);
        pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_ThreadTls;
        K2LIST_AddAtTail(&apProc->PageList.Locked.ThreadTls, &pTrack->ListLink);
        K2OSKERN_SeqUnlock(&apProc->PageList.SeqLock, disp);

        pNewThread->mTlsPagePhys = physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
        KernPhys_ZeroPage(physPageAddr);

        KernPte_MakePageMap(apProc, K2OS_UVA_TLSAREA_BASE + (newThreadIx * K2_VA_MEMPAGE_BYTES), physPageAddr, K2OS_MAPTYPE_USER_DATA);
        KernPte_MakePageMap(NULL, (UINT32)pNewThread->mpKernRwViewOfUserThreadPage, physPageAddr, K2OS_MAPTYPE_KERN_DATA);

        K2MEM_Copy(&pNewThread->UserConfig, apConfig, sizeof(K2OS_USER_THREAD_CONFIG));
        K2MEM_Zero(pNewThread->mpKernRwViewOfUserThreadPage, sizeof(K2OS_USER_THREAD_PAGE));

        pNewThread->mState = KernThreadState_InitNoPrep;

        disp = K2OSKERN_SeqLock(&apProc->Thread.SeqLock);
        K2LIST_AddAtTail(&apProc->Thread.Locked.CreateList, &pNewThread->ProcThreadListLink);
        K2OSKERN_SeqUnlock(&apProc->Thread.SeqLock, disp);

//        K2OSKERN_Debug("Create Thread %08X\n", pNewThread);

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        KernHeap_Free(pNewThread);
    }
    else
    {
        *appRetThread = pNewThread;
    }

    return stat;
}

K2STAT
KernThread_OnException_UserMode_Trapped(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OSKERN_THREAD_EX *        apEx
)
{
    K2STAT                  stat;
    K2_EXCEPTION_TRAP *     pTrap;
    UINT32                  mapCount;
    K2OSKERN_OBJREF         mapRef[2];
    UINT32                  map0PageIx;

    K2_ASSERT(NULL != apCurThread->mpTrapStack);

    K2OSKERN_Debug("***Trapped usermode exception (%08X) on thread %d\n", apEx->mExCode, apCurThread->mGlobalIx);

    pTrap = apCurThread->mpTrapStack;

    mapRef[0].Ptr.AsHdr = mapRef[1].Ptr.AsHdr = NULL;

    stat = KernProc_AcqMaps(
        apCurThread->ProcRef.Ptr.AsProc,
        (UINT32)pTrap,
        sizeof(K2_EXCEPTION_TRAP),
        TRUE,
        &mapCount,
        &mapRef[0],
        &map0PageIx);
    if (K2STAT_IS_ERROR(stat))
        return apEx->mExCode;

    //
    // exceptions are being trapped here - pop the trap
    //
    apCurThread->mpTrapStack = pTrap->mpNextTrap;

    //
    // set the exception code in the trap and copy back the context that was saved
    //
    pTrap->mTrapResult = apEx->mExCode;
    KernArch_TrapToContext(&pTrap->SavedContext, &apCurThread->ArchExecContext, TRUE);

    //
    // release the maps
    //
    if (1 < mapCount)
    {
        KernObj_ReleaseRef(&mapRef[1]);
    }
    KernObj_ReleaseRef(&mapRef[0]);

    return K2STAT_NO_ERROR;
}

void
KernThread_OnException_Unhandled(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OSKERN_THREAD_EX *        apEx
)
{
    K2STAT                  stat;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    if (NULL != apCurThread->mpTrapStack)
    {
        stat = KernThread_OnException_UserMode_Trapped(apThisCore, apCurThread, apEx);
        if (!K2STAT_IS_ERROR(stat))
            return;
    }

    K2OSKERN_Debug(
        "\n*** Thread %d caused unhandled exception, will crash process %d\n\n", 
        apCurThread->mGlobalIx, 
        apCurThread->ProcRef.Ptr.AsProc->mId);

    if (apCurThread->ProcRef.Ptr.AsProc->mId == 1)
    {
//        K2OSKERN_Panic("Root Crash\n");
    }

    //
    // just crash the process
    //
    KernArch_DumpThreadContext(apThisCore, apCurThread);
    if (KernDbg_Attached())
    {
        KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_Debug_Crashed);
        KernDbg_BeginModeEntry(apThisCore);
    }
    else
    {
        pSchedItem = &apCurThread->SchedItem;
        pSchedItem->mType = KernSchedItem_Thread_Crash_Process;
        KernTimer_GetHfTick(&pSchedItem->mHfTick);
        KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
        KernSched_QueueItem(pSchedItem);
    }
}

void
KernThread_CoW_Check_Complete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
);

void
KernThread_CoW_Dpc_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    UINT32                  sentMask;
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_THREAD_EX *    pEx;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, CoW.DpcSimple.Func);
    pEx = &pThread->LastEx;

    KTRACE(apThisCore, 3, KTRACE_THREAD_COW_SENDICI_DPC, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);

    K2_ASSERT(0 != pEx->mIciSendMask);

    sentMask = KernArch_SendIci(
        apThisCore,
        pEx->mIciSendMask,
        KernIci_TlbInv, 
        &pThread->TlbShoot
    );
    
    pEx->mIciSendMask &= ~sentMask;

    if (0 == pEx->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for this thread
        //
        pThread->CoW.DpcSimple.Func = KernThread_CoW_Check_Complete;
    }
    else
    {
        pThread->CoW.DpcSimple.Func = KernThread_CoW_Dpc_SendIci;
    }

    KernCpu_QueueDpc(&pThread->CoW.DpcSimple.Dpc, &pThread->CoW.DpcSimple.Func, KernDpcPrio_Med);
}

void
KernThread_CoW_Dpc_Complete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_THREAD_EX *    pEx;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, CoW.DpcSimple.Func);
    pEx = &pThread->LastEx;

    KernObj_ReleaseRef(&pEx->MapRef);

    KTRACE(apThisCore, 3, KTRACE_THREAD_COW_COMPLETE, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);

    //
    // this reference was acquired when the ici distribution sequence 
    // was started (or not in the case of single core
    //
    KernObj_ReleaseRef(&pThread->SchedItem.ObjRef);
}

void
KernThread_CoW_Check_Complete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, CoW.DpcSimple.Func);

    KTRACE(apThisCore, 3, KTRACE_THREAD_COW_CHECK_DPC, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);

    if (0 == pThread->TlbShoot.mCoresRemaining)
    {
        KernThread_CoW_Dpc_Complete(apThisCore, apKey);
        return;
    }

    pThread->CoW.DpcSimple.Func = KernThread_CoW_Check_Complete;
    KernCpu_QueueDpc(&pThread->CoW.DpcSimple.Dpc, &pThread->CoW.DpcSimple.Func, KernDpcPrio_Med);
}

void
KernThread_CoW_Dpc_Copy(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_THREAD_EX *    pEx;
    K2OSKERN_OBJ_MAP *      pMap;

    UINT32                  virtAddrSrc;
    UINT32                  virtAddrDst;
    UINT32                  physAddr;
    BOOL                    disp;
    K2LIST_LINK *           pListLink;
    K2OSKERN_PHYSTRACK *    pTrack;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, CoW.DpcSimple.Func);
    pProc = pThread->ProcRef.Ptr.AsProc;
    pEx = &pThread->LastEx;
    pMap = pEx->MapRef.Ptr.AsMap;
    K2_ASSERT(NULL != pMap);

    KTRACE(apThisCore, 3, KTRACE_THREAD_COW_COPY_DPC, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);

    disp = K2OSKERN_SeqLock(&pProc->PageList.SeqLock);
        pListLink = pProc->PageList.Locked.Reserved_Dirty.mpHead;
        K2_ASSERT(NULL != pListLink);
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pListLink, ListLink);
        K2LIST_Remove(&pProc->PageList.Locked.Reserved_Dirty, &pTrack->ListLink);
        pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_Working;
        K2LIST_AddAtTail(&pProc->PageList.Locked.Working, &pTrack->ListLink);
    K2OSKERN_SeqUnlock(&pProc->PageList.SeqLock, disp);

    virtAddrSrc = K2OS_KVA_PERCOREWORKPAGES_BASE + ((K2OSKERN_GetCpuIndex() * K2OS_PERCOREWORKPAGES_PERCORE) *  K2_VA_MEMPAGE_BYTES);
    virtAddrDst = virtAddrSrc + K2_VA_MEMPAGE_BYTES;

    physAddr = KernPageArray_PagePhys(pMap->PageArrayRef.Ptr.AsPageArray, pMap->mPageArrayStartPageIx + pEx->mMapPageIx);
    KernPte_MakePageMap(NULL, virtAddrSrc, physAddr, K2OS_MAPTYPE_KERN_READ);

    physAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
    KernPte_MakePageMap(NULL, virtAddrDst, physAddr, K2OS_MAPTYPE_KERN_DATA);

    K2MEM_Copy((void *)virtAddrDst, (void *)virtAddrSrc, K2_VA_MEMPAGE_BYTES);
    K2OS_CacheOperation(K2OS_CACHEOP_FlushData, virtAddrDst, K2_VA_MEMPAGE_BYTES);

    KernPte_BreakPageMap(NULL, virtAddrSrc, 0);
    KernArch_InvalidateTlbPageOnCurrentCore(virtAddrSrc);

    KernPte_BreakPageMap(NULL, virtAddrDst, 0);
    KernArch_InvalidateTlbPageOnCurrentCore(virtAddrDst);

    //
    // update PTE and do TLB shoot down across cores
    //
    virtAddrDst = pEx->mFaultAddr & K2_VA_PAGEFRAME_MASK;
    disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);
    KernPte_MakePageMap(pProc, virtAddrDst, physAddr, K2OS_MAPTYPE_USER_DATA);
    K2OSKERN_SeqUnlock(&pProc->Virt.SeqLock, disp);

    //
    // pte in map updated.  begin shoot down TLB.  process that needs shootdown
    // may no longer be mapped on this core.  if it isn't then we don't bother
    // with the invalidate.  We still send it to the other cores though.
    //
    if (apThisCore->MappedProcRef.Ptr.AsProc == pProc)
        KernArch_InvalidateTlbPageOnCurrentCore(virtAddrDst);

    //
    // completion releases this self-reference to the thread
    //
    KernObj_CreateRef(&pThread->SchedItem.ObjRef, &pThread->Hdr);

    if (gData.mCpuCoreCount > 1)
    {
        pThread->TlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        pThread->TlbShoot.mpProc = pProc;
        pThread->TlbShoot.mVirtBase = virtAddrDst;
        pThread->TlbShoot.mPageCount = 1;
        pEx->mIciSendMask = pThread->TlbShoot.mCoresRemaining;
        KernThread_CoW_Dpc_SendIci(apThisCore, &pThread->CoW.DpcSimple.Func);
    }
    else
    {
        KernThread_CoW_Dpc_Complete(apThisCore, &pThread->CoW.DpcSimple.Func);
    }
}

void
KernThread_OnException_Access(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OSKERN_THREAD_EX *        apEx
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJ_MAP *      pMap;
    BOOL                    isCow;
    BOOL                    disp;
    UINT32                  pte;

    isCow = FALSE;

    pMap = apEx->MapRef.Ptr.AsMap;
    if (NULL != pMap)
    {
        pProc = apCurThread->ProcRef.Ptr.AsProc;
        disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);
        pte = pMap->mpPte[apEx->mMapPageIx];
        if ((apEx->mPageWasPresent) &&
            (pte & K2OSKERN_PTE_PRESENT_BIT) &&
            (apEx->mWasWrite) &&
            (!KernArch_PteMapsWriteable(pte)) &&
            (pMap->mUserMapType == K2OS_MapType_Data_CopyOnWrite))
        {
            //
            // mark PTE as NP and start copy-on-write processing for the thread
            //
            isCow = TRUE;
            KernPte_BreakPageMap(apCurThread->ProcRef.Ptr.AsProc, apEx->mFaultAddr & K2_VA_PAGEFRAME_MASK, K2OSKERN_PTE_ACTIVE_COW);
            apCurThread->CoW.DpcSimple.Func = KernThread_CoW_Dpc_Copy;
            KernCpu_QueueDpc(&apCurThread->CoW.DpcSimple.Dpc, &apCurThread->CoW.DpcSimple.Func, KernDpcPrio_Med);
        }
        K2OSKERN_SeqUnlock(&pProc->Virt.SeqLock, disp);
    }

    if (!isCow)
    {
        KernThread_OnException_Unhandled(apThisCore, apCurThread, apEx);
    }
}

void    
KernThread_CpuEvent_Exception(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    K2STAT                  stat;
    K2OSKERN_THREAD_EX *    pEx;

    pEx = &apThread->LastEx;

    KTRACE(apThisCore, 4, KTRACE_THREAD_EXCEPTION, apThread->ProcRef.Ptr.AsProc->mId, apThread->mGlobalIx, pEx->mExCode);

    stat = KernProc_FindCreateMapRef(apThread->ProcRef.Ptr.AsProc, pEx->mFaultAddr, &pEx->MapRef, &pEx->mMapPageIx);
    if (K2STAT_IS_ERROR(stat))
    {
        pEx->MapRef.Ptr.AsHdr = NULL;
    }

    if (pEx->mExCode == K2STAT_EX_ACCESS)
    {
        KernThread_OnException_Access(apThisCore, apThread, pEx);
    }
    else
    {
        KernThread_OnException_Unhandled(apThisCore, apThread, pEx);
    }
}

void    
KernThread_CpuEvent_SysCall(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    K2_ASSERT(NULL != apThread);
    KTRACE(apThisCore, 4, KTRACE_THREAD_SYSCALL, apThread->ProcRef.Ptr.AsProc->mId, apThread->mGlobalIx, apThread->mSysCall_Id);
    if ((0 == apThread->mSysCall_Id) ||
        (apThread->mSysCall_Id >= K2OS_SYSCALL_COUNT))
    {
        KernThread_UnimplementedSysCall(apThisCore, apThread);
    }
    else
    {
//        K2OSKERN_Debug("---SysCall(%d,%d)\n", apThread->mGlobalIx, apThread->mSysCall_Id);
        sgSysCall[apThread->mSysCall_Id](apThisCore, apThread);
    }
}

void 
KernThread_SysCall_OutputDebug(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    apCurThread->mSysCall_Result = K2OSKERN_Debug("%s", (char const *)apCurThread->mSysCall_Arg0);
}

void 
KernThread_SysCall_RaiseException(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_THREAD_EX  * pEx;

    pEx = &apCurThread->LastEx;
    pEx->mFaultAddr = 0;
    pEx->mPageWasPresent = FALSE;
    pEx->mWasWrite = FALSE;
    pEx->mExCode = apCurThread->mSysCall_Arg0 | K2STAT_FACILITY_EXCEPTION;
    apCurThread->mIsInSysCall = FALSE;
    KTRACE(apThisCore, 4, KTRACE_THREAD_RAISE_EX, apCurThread->ProcRef.Ptr.AsProc->mId, apCurThread->mGlobalIx, pEx->mExCode);
    KernThread_OnException_Unhandled(apThisCore, apCurThread, pEx);
}

void    
KernThread_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         mapRef;
    UINT32                  pageIx;
    UINT32                  crtEntry;
    UINT32                  userEntry;
    UINT32                  stackTop;
    BOOL                    ok;
    K2OSKERN_OBJ_THREAD *   pNewThread;
    K2OS_THREAD_TOKEN       tokThread;
    K2OS_USER_THREAD_CONFIG config;
    UINT32                  stackPtr;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    //
    // sanitize config
    //
    *((UINT32 *)&config) = pThreadPage->mSysCall_Arg4_Result3;
    if (0 == config.mPriority)
        config.mPriority = 1;
    else if (config.mPriority > 3)
        config.mPriority = 3;

    config.mAffinityMask &= (UINT8)((1 << gData.mCpuCoreCount) - 1);
    if (0 == config.mAffinityMask)
        config.mAffinityMask = (UINT8)(1 << gData.mCpuCoreCount) - 1;

    if (0 == config.mQuantum)
        config.mQuantum = 10;

    if (pThreadPage->mSysCall_Arg2 >= K2OS_KVA_KERN_BASE)
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_NOT_ALLOWED;
        return;
    }

    pProc = apCurThread->ProcRef.Ptr.AsProc;

    do
    {
        //
        // crt entry must be within crt text map
        //
        crtEntry = apCurThread->mSysCall_Arg0;
        mapRef.Ptr.AsHdr = NULL;
        stat = KernProc_FindCreateMapRef(pProc, crtEntry, &mapRef, &pageIx);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }
        if (mapRef.Ptr.AsMap != pProc->CrtMapRef[KernUserCrtSeg_Text].Ptr.AsMap)
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        KernObj_ReleaseRef(&mapRef);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }

        //
        // user entry must be within some user text map
        //
        userEntry = pThreadPage->mSysCall_Arg1;
        stat = KernProc_FindCreateMapRef(pProc, userEntry, &mapRef, &pageIx);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }
        ok = (mapRef.Ptr.AsMap->mUserMapType == K2OS_MapType_Text) ? TRUE : FALSE;
        KernObj_ReleaseRef(&mapRef);
        if (!ok)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        //
        // stack map must point to a valid stack map
        //
        stackTop = pThreadPage->mSysCall_Arg3;
        stat = KernProc_FindCreateMapRef(pProc, stackTop, &mapRef, &pageIx);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }
        do
        {
            if ((mapRef.Ptr.AsMap->mUserMapType != K2OS_MapType_Thread_Stack) ||
                (pageIx != 0) ||
                (mapRef.Ptr.AsMap->mpProcHeapNode->HeapNode.AddrTreeNode.mUserVal != (stackTop - K2_VA_MEMPAGE_BYTES)))
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                break;
            }

            //
            // actual creation of thread here
            //
            stat = KernThread_Create(pProc, &config, &pNewThread);
            if (K2STAT_IS_ERROR(stat))
                break;

            KernObj_CreateRef(&pNewThread->StackMapRef, mapRef.Ptr.AsHdr);
            mapRef.Ptr.AsMap->mpProcHeapNode->mUserOwned = 0;

            stackPtr = mapRef.Ptr.AsMap->mpProcHeapNode->HeapNode.AddrTreeNode.mUserVal;
            stackPtr += (mapRef.Ptr.AsMap->mPageCount * K2_VA_MEMPAGE_BYTES) - 4;
            KernArch_UserThreadPrep(
                pNewThread, 
                crtEntry, 
                stackPtr, 
                userEntry, 
                pThreadPage->mSysCall_Arg2);

        } while (0);

        KernObj_ReleaseRef(&mapRef);

    } while (0);

    if (!K2STAT_IS_ERROR(stat))
    {
        //
        // thread is created at this point and ready to be pushed onto the
        // run list of some core.  we need to create a token here because if
        // it fails we have to tear down the whole thing.
        //
        stat = KernProc_TokenCreate(pProc, &pNewThread->Hdr, &tokThread);
        if (K2STAT_IS_ERROR(stat))
        {
            KernThread_Cleanup(apThisCore, pNewThread);
        }
        else
        {
            pSchedItem = &apCurThread->SchedItem;
            pSchedItem->mType = KernSchedItem_Thread_SysCall;
            KernTimer_GetHfTick(&pSchedItem->mHfTick);
            KernObj_CreateRef(&pSchedItem->ObjRef, &pNewThread->Hdr);
            pSchedItem->Args.Thread_Create.mThreadToken = (UINT32)tokThread;
            KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
            KernSched_QueueItem(pSchedItem);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void
KernThread_Cleanup_Done(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
    )
{
    K2OSKERN_PHYSTRACK *    pTrack;
    UINT32                  threadSlotIx;
    UINT32                  lastSlotIx;
    UINT32 *                pThreadPtrs;
    BOOL                    disp;
    UINT_PTR                chk;

//    K2OSKERN_Debug("Cleanup thread %d completed\n", apThread->mGlobalIx);

    K2_ASSERT(NULL == apThread->StackMapRef.Ptr.AsHdr);

    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(apThread->mTlsPagePhys);
    KernPhys_FreeTrack(pTrack);

    KernObj_ReleaseRef(&apThread->ProcRef);
    K2_ASSERT(NULL == apThread->LastEx.MapRef.Ptr.AsHdr);
    K2_ASSERT(NULL == apThread->IoPermitProcRef.Ptr.AsHdr);
    for (chk = 0; chk < K2OS_THREAD_WAIT_MAX_ITEMS; chk++)
    {
        K2_ASSERT(NULL == apThread->MacroWait.WaitEntry[chk].ObjRef.Ptr.AsHdr);
    }

    threadSlotIx = apThread->mGlobalIx;

    K2MEM_Zero(apThread, sizeof(K2OSKERN_OBJ_THREAD));

    KernHeap_Free(apThread);

    //
    // put thread id onto the end of the thread list
    //
    pThreadPtrs = ((UINT32 *)K2OS_KVA_THREADPTRS_BASE);
    pThreadPtrs[threadSlotIx] = 0;

    disp = K2OSKERN_SeqLock(&gData.Proc.SeqLock);
    lastSlotIx = gData.Proc.mLastThreadSlotIx;
    pThreadPtrs[lastSlotIx] = threadSlotIx;
    gData.Proc.mLastThreadSlotIx = threadSlotIx;
    K2OSKERN_SeqUnlock(&gData.Proc.SeqLock, disp);
}

void
KernThread_Cleanup_Kern_ThreadPage_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD * pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 3, KTRACE_THREAD_KERNPAGE_CHECK_DPC, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);

    if (0 == pThread->TlbShoot.mCoresRemaining)
    {
        KernThread_Cleanup_Done(apThisCore, pThread);
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_Kern_ThreadPage_CheckComplete;
        KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernThread_Cleanup_SendIci_Kern_ThreadPage(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    UINT32                  sentMask;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pThread->mIciSendMask);
    
    KTRACE(apThisCore, 3, KTRACE_THREAD_KERNPAGE_SENDICI_DPC, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);

    sentMask = KernArch_SendIci(
        apThisCore,
        pThread->mIciSendMask,
        KernIci_TlbInv,
        &pThread->TlbShoot
    );

    pThread->mIciSendMask &= ~sentMask;

    if (0 == pThread->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_Kern_ThreadPage_CheckComplete;
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_SendIci_Kern_ThreadPage;
    }
    KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernThread_Cleanup_User_ThreadPage_Done(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    if (gData.mCpuCoreCount > 1)
    {
        apThread->TlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        apThread->TlbShoot.mpProc = NULL;
        apThread->TlbShoot.mVirtBase = (UINT32)apThread->mpKernRwViewOfUserThreadPage;
        apThread->TlbShoot.mPageCount = 1;
        apThread->mIciSendMask = apThread->TlbShoot.mCoresRemaining;
        KernThread_Cleanup_SendIci_Kern_ThreadPage(apThisCore, &apThread->Hdr.ObjDpc.Func);
    }

    KernArch_InvalidateTlbPageOnCurrentCore((UINT32)apThread->mpKernRwViewOfUserThreadPage);

    if (gData.mCpuCoreCount == 1)
    {
        KernThread_Cleanup_Done(apThisCore, apThread);
    }
}

void
KernThread_Cleanup_User_ThreadPage_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD * pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 3, KTRACE_THREAD_USERPAGE_CHECK_DPC, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);

    if (0 == pThread->TlbShoot.mCoresRemaining)
    {
        KernThread_Cleanup_User_ThreadPage_Done(apThisCore, pThread);
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_User_ThreadPage_CheckComplete;
        KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernThread_Cleanup_SendIci_User_ThreadPage(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    UINT32                  sentMask;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pThread->mIciSendMask);
    
    KTRACE(apThisCore, 3, KTRACE_THREAD_USERPAGE_CHECK_DPC, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);

    sentMask = KernArch_SendIci(
        apThisCore,
        pThread->mIciSendMask,
        KernIci_TlbInv,
        &pThread->TlbShoot
    );

    pThread->mIciSendMask &= ~sentMask;

    if (0 == pThread->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_User_ThreadPage_CheckComplete;
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_SendIci_User_ThreadPage;
    }
    KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernThread_Cleanup_StartShootDown_User_ThreadPage(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread,
    UINT32                      aUserPageVirt
)
{
    if (gData.mCpuCoreCount > 1)
    {
        apThread->TlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        apThread->TlbShoot.mpProc = apThread->ProcRef.Ptr.AsProc;
        apThread->TlbShoot.mVirtBase = aUserPageVirt;
        apThread->TlbShoot.mPageCount = 1;
        apThread->mIciSendMask = apThread->TlbShoot.mCoresRemaining;
        KernThread_Cleanup_SendIci_User_ThreadPage(apThisCore, &apThread->Hdr.ObjDpc.Func);
    }

    KernArch_InvalidateTlbPageOnCurrentCore(aUserPageVirt);

    if (gData.mCpuCoreCount == 1)
    {
        KernThread_Cleanup_User_ThreadPage_Done(apThisCore, apThread);
    }
}

void    
KernThread_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    BOOL                    disp;
    UINT32                  pagePhys;
    UINT32                  userPageVirt;
    K2OSKERN_PHYSTRACK *    pTrack;

//    K2OSKERN_Debug("Cleanup thread %d\n", apThread->mGlobalIx);

    K2_ASSERT(apThread->mState == KernThreadState_Exited);
    K2_ASSERT(NULL == apThread->SchedItem.ObjRef.Ptr.AsHdr);

    pProc = apThread->ProcRef.Ptr.AsProc;

    disp = K2OSKERN_SeqLock(&pProc->Thread.SeqLock);
    K2LIST_Remove(&pProc->Thread.Locked.DeadList, &apThread->ProcThreadListLink);
    K2OSKERN_SeqUnlock(&pProc->Thread.SeqLock, disp);

    userPageVirt = K2OS_UVA_TLSAREA_BASE + (apThread->mGlobalIx * K2_VA_MEMPAGE_BYTES);
    if (NULL != apThread->StackMapRef.Ptr.AsHdr)
    {
        KernObj_ReleaseRef(&apThread->StackMapRef);
    }

    if (pProc->mState != KernProcState_Stopped)
    {
        pagePhys = KernPte_BreakPageMap(pProc, userPageVirt, 0);
        K2_ASSERT(pagePhys == apThread->mTlsPagePhys);
    }

    //
    // break the kernel mapping of the thread page
    //
    pagePhys = KernPte_BreakPageMap(NULL, (UINT32)apThread->mpKernRwViewOfUserThreadPage, 0);
    K2_ASSERT(pagePhys == apThread->mTlsPagePhys);

    //
    // free the thread page physical page
    //
    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(pagePhys);
    disp = K2OSKERN_SeqLock(&pProc->PageList.SeqLock);
    K2_ASSERT(pTrack->Flags.Field.PageListIx == KernPhysPageList_Proc_ThreadTls);
    K2LIST_Remove(&pProc->PageList.Locked.ThreadTls, &pTrack->ListLink);
    pTrack->Flags.Field.PageListIx = KernPhysPageList_None;
    K2OSKERN_SeqUnlock(&pProc->PageList.SeqLock, disp);

    if (pProc->mState != KernProcState_Stopped)
    {
        //        K2OSKERN_Debug("Thread %d cleanup when process NOT exited\n", apThread->mGlobalIx);
        KernThread_Cleanup_StartShootDown_User_ThreadPage(apThisCore, apThread, userPageVirt);
    }
    else
    {
        //        K2OSKERN_Debug("Thread %d cleanup when process exited\n", apThread->mGlobalIx);
        KernThread_Cleanup_User_ThreadPage_Done(apThisCore, apThread);
    }
}

void    
KernThread_SysCall_Exit(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_SCHED_ITEM *   pSchedItem;

//    K2OSKERN_Debug("Thread %d exiting\n", apCurThread->mGlobalIx);

    pSchedItem = &apCurThread->SchedItem;
    pSchedItem->mType = KernSchedItem_Thread_SysCall;
    KernTimer_GetHfTick(&pSchedItem->mHfTick);
    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
    KernSched_QueueItem(pSchedItem);
}

void    
KernThread_SysCall_SetAffinity(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    UINT32 desired;
    UINT32 oldAffinity;

    oldAffinity = apCurThread->UserConfig.mAffinityMask;

    desired = apCurThread->mSysCall_Arg0;

    if (desired == (UINT32)-1)
    {
        apCurThread->mSysCall_Result = oldAffinity;
        return;
    }

    desired &= ((1 << gData.mCpuCoreCount) - 1);
    if (0 == desired)
    {
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        apCurThread->mSysCall_Result = 0;
        return;
    }

    apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
    apCurThread->SchedItem.Args.Thread_SetAff.mNewMask = desired;
    KernTimer_GetHfTick(&apCurThread->SchedItem.mHfTick);
    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
    KernSched_QueueItem(&apCurThread->SchedItem);
}

void    
KernThread_SysCall_GetExitCode(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT          stat;
    K2OSKERN_OBJREF threadRef;

    threadRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(
        apCurThread->ProcRef.Ptr.AsProc,
        (K2OS_TOKEN)apCurThread->mSysCall_Arg0,
        &threadRef
    );
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Thread == threadRef.Ptr.AsHdr->mObjType)
        {
            if (threadRef.Ptr.AsThread->mState == KernThreadState_Exited)
            {
                apCurThread->mSysCall_Result = threadRef.Ptr.AsThread->mExitCode;
            }
            else
            {
                stat = K2STAT_ERROR_RUNNING;
            }
        }
        else
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        KernObj_ReleaseRef(&threadRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = (UINT_PTR)-1;
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = stat;
    }
}

void    
KernThread_SysCall_DebugBreak(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    KernDbg_BeginModeEntry(apThisCore);
}
