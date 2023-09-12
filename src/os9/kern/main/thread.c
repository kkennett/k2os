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
#include "..\lib\k2osrpc\k2osrpc.h"

static K2OSKERN_DPC_SIMPLE sgDpc_OneTimeInitInMonitor;

static KernUser_pf_SysCall sgSysCall[K2OS_SYSCALL_COUNT];

void 
KernThread_UnimplementedSysCall(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_Debug("**Unimplemented system call %d\n", apCurThread->User.mSysCall_Id);
    apCurThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_NOT_IMPL;
    apCurThread->User.mSysCall_Result = 0;
}

void KernThread_SysCall_OutputDebug(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void KernThread_SysCall_RaiseException(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);

UINT32
K2_CALLCONV_REGS
KernThread_FirstThreadEntryPoint(
    UINT32  aFirstArg,
    UINT32  aSecondArg
)
{
    K2OSACPI_INIT acpiInit;

//    K2OSKERN_Debug("\nFirst Kernel Thread Starts...\n");

    KernCritSec_Threaded_InitDeferred();

    KernXdl_Threaded_Init();

    KernIpcEnd_Threaded_Init();

    KernExec_Threaded_Init();

    acpiInit.FwInfo.mFwBasePhys = gData.mpShared->LoadInfo.mFwTabPagesPhys;
    acpiInit.FwInfo.mFwSizeBytes = gData.mpShared->LoadInfo.mFwTabPageCount * K2_VA_MEMPAGE_BYTES;
    acpiInit.FwInfo.mFacsPhys = gData.mpShared->LoadInfo.mFwFacsPhys;
    acpiInit.FwInfo.mXFacsPhys = gData.mpShared->LoadInfo.mFwXFacsPhys;
    acpiInit.mFwBaseVirt = gData.mpShared->LoadInfo.mFwTabPagesVirt;
    acpiInit.mFacsVirt = K2OS_KVA_FACS_BASE;
    acpiInit.mXFacsVirt = K2OS_KVA_XFACS_BASE;

    K2OSRPC_Init();

    KernThread_Exit(((K2OS_pf_THREAD_ENTRY)gData.Exec.mfMainThreadEntryPoint)(&acpiInit));

    K2OSKERN_Panic("*** - KernThread_FirstThreadEntryPoint return\n");
    while (1);
}

UINT32
K2_CALLCONV_REGS
KernThread_EntryPoint(
    UINT32  aFirstArg,
    UINT32  aSecondArg
)
{
    KernThread_Exit(((K2OS_pf_THREAD_ENTRY)aFirstArg)((void *)aSecondArg));
    K2OSKERN_Panic("*** - KernThread_EntryPoint return\n");
    while (1);
}

void
KernThread_OneTimeInitInMonitor(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2STAT                      stat;
    K2OSKERN_PHYSRES            res;
    BOOL                        ok;
    K2OSKERN_PHYSTRACK *        pTrack;
    UINT32                      physPageAddr;
    K2OSKERN_OBJ_THREAD *       pThread;
    K2OS_THREAD_CONFIG          config;
    K2OSKERN_PHYSSCAN           scan;
    UINT32                      virtWork;

    K2_ASSERT(apKey == (void *)&sgDpc_OneTimeInitInMonitor.Func);

    //
    // install pagetable for tls area
    //
    ok = KernPhys_Reserve_Init(&res, 1);
    K2_ASSERT(ok);
    stat = KernPhys_AllocPow2Bytes(&res, K2_VA_MEMPAGE_BYTES, &pTrack);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);

    KernPhys_ZeroPage(physPageAddr);

    KernArch_InstallPageTable(NULL, K2OS_KVA_TLSAREA_BASE, physPageAddr);

    //
    // create executive initial thread
    //
    config.mStackPages = K2OS_THREAD_DEFAULT_STACK_PAGES;
    config.mPriority = 0;
    config.mAffinityMask = (1 << gData.mCpuCoreCount) - 1;
    config.mQuantum = 30;

    stat = KernThread_Create(NULL, &config, &pThread);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    ok = KernPhys_Reserve_Init(&res, config.mStackPages);
    K2_ASSERT(ok);

    stat = KernPhys_AllocSparsePages(&res, config.mStackPages, &pThread->Kern.StackPhysTrackList);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    KernPhys_CutTrackListIntoPages(&pThread->Kern.StackPhysTrackList, FALSE, NULL, TRUE, KernPhysPageList_KernThread_Stack);
    KernPhys_ScanInit(&scan, &pThread->Kern.StackPhysTrackList, 0);

    pThread->Kern.mStackBaseVirt = KernVirt_Reserve(config.mStackPages + 2);
    K2_ASSERT(0 != pThread->Kern.mStackBaseVirt);

    virtWork = pThread->Kern.mStackBaseVirt + K2_VA_MEMPAGE_BYTES;
    physPageAddr = config.mStackPages;
    do {
        KernPte_MakePageMap(NULL, virtWork, KernPhys_ScanIter(&scan), K2OS_MAPTYPE_KERN_DATA);
        virtWork += K2_VA_MEMPAGE_BYTES;
    } while (--physPageAddr > 0);

    // top and bottom pages are guards
    pThread->Kern.mStackPtr = pThread->Kern.mStackBaseVirt + ((1 + config.mStackPages) * K2_VA_MEMPAGE_BYTES);
    pThread->Kern.mStackPtr -= sizeof(UINT32);
    *((UINT32 *)pThread->Kern.mStackPtr) = 0;
    pThread->Kern.mStackPtr -= sizeof(UINT32);
    *((UINT32 *)pThread->Kern.mStackPtr) = 0;

    KernArch_KernThreadPrep(pThread,
        (UINT32)KernThread_FirstThreadEntryPoint,
        &pThread->Kern.mStackPtr,
        (UINT32)pThread, 0);

    //
    // manually set initial thread as running and add it to the current core
    //
    K2_ASSERT(0 == apThisCore->mCoreIx);
    K2LIST_Remove(&gData.Thread.CreateList, &pThread->OwnerThreadListLink);
    K2LIST_AddAtTail(&gData.Thread.ActiveList, &pThread->OwnerThreadListLink);
    KernObj_CreateRef(&pThread->Running_SelfRef, &pThread->Hdr);

    KTRACE(apThisCore, 3, KTRACE_THREAD_START, 0, pThread->mGlobalIx);
#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Starting Kernel Thread %d\n", pThread->mGlobalIx);
#endif

    pThread->mState = KernThreadState_OnCpuLists;
    K2LIST_AddAtTail((K2LIST_ANCHOR *)&apThisCore->RunList, &pThread->CpuCoreThreadListLink);
    K2ATOMIC_Inc(&gData.Sched.mCoreThreadCount[apThisCore->mCoreIx]);
}

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
    sgSysCall[K2OS_SYSCALL_ID_DEBUG_BREAK               ] = KernThread_SysCall_DebugBreak;
    sgSysCall[K2OS_SYSCALL_ID_RAISE_EXCEPTION           ] = KernThread_SysCall_RaiseException;
    sgSysCall[K2OS_SYSCALL_ID_TOKEN_CLONE               ] = KernProc_SysCall_TokenClone;
    sgSysCall[K2OS_SYSCALL_ID_TOKEN_DESTROY             ] = KernProc_SysCall_TokenDestroy;
    sgSysCall[K2OS_SYSCALL_ID_NOTIFY_CREATE             ] = KernNotify_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_NOTIFY_SIGNAL             ] = KernNotify_SysCall_Signal;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_WAIT               ] = KernWait_SysCall;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_EXIT               ] = KernThread_SysCall_Exit;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_CREATE             ] = KernThread_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_SETAFF             ] = KernThread_SysCall_SetAffinity;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_GETEXITCODE        ] = KernThread_SysCall_GetExitCode;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_SET_NAME           ] = KernThread_SysCall_SetName;
    sgSysCall[K2OS_SYSCALL_ID_THREAD_GET_NAME           ] = KernThread_SysCall_GetName;
    sgSysCall[K2OS_SYSCALL_ID_PAGEARRAY_CREATE          ] = KernPageArray_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_PAGEARRAY_GETLEN          ] = KernPageArray_SysCall_GetLen;
    sgSysCall[K2OS_SYSCALL_ID_MAP_CREATE                ] = KernVirtMap_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_MAP_ACQUIRE               ] = KernVirtMap_SysCall_Acquire;
    sgSysCall[K2OS_SYSCALL_ID_MAP_ACQ_PAGEARRAY         ] = KernVirtMap_SysCall_AcqPageArray;
    sgSysCall[K2OS_SYSCALL_ID_MAP_GET_INFO              ] = KernVirtMap_SysCall_GetInfo;
    sgSysCall[K2OS_SYSCALL_ID_CACHE_OP                  ] = KernProc_SysCall_CacheOp;
    sgSysCall[K2OS_SYSCALL_ID_VIRT_RESERVE              ] = KernProc_SysCall_VirtReserve;
    sgSysCall[K2OS_SYSCALL_ID_VIRT_GET                  ] = KernProc_SysCall_VirtGet;
    sgSysCall[K2OS_SYSCALL_ID_VIRT_RELEASE              ] = KernProc_SysCall_VirtRelease;
    sgSysCall[K2OS_SYSCALL_ID_GET_LAUNCH_INFO           ] = KernProc_SysCall_GetLaunchInfo;
    sgSysCall[K2OS_SYSCALL_ID_TRAP_MOUNT                ] = KernProc_SysCall_TrapMount;
    sgSysCall[K2OS_SYSCALL_ID_TRAP_DISMOUNT             ] = KernProc_SysCall_TrapDismount;
    sgSysCall[K2OS_SYSCALL_ID_GATE_CREATE               ] = KernGate_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_GATE_CHANGE               ] = KernGate_SysCall_Change;
    sgSysCall[K2OS_SYSCALL_ID_ALARM_CREATE              ] = KernAlarm_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_SEM_CREATE                ] = KernSemUser_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_SEM_INC                   ] = KernSemUser_SysCall_Inc;
    sgSysCall[K2OS_SYSCALL_ID_MAILBOX_CREATE            ] = KernMailbox_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_MAILBOXOWNER_RECVRES      ] = KernMailboxOwner_SysCall_RecvRes;
    sgSysCall[K2OS_SYSCALL_ID_MAILBOXOWNER_RECV         ] = KernMailboxOwner_SysCall_Recv;
    sgSysCall[K2OS_SYSCALL_ID_MAILBOXOWNER_SHARE        ] = KernMailboxOwner_SysCall_Share;
    sgSysCall[K2OS_SYSCALL_ID_MAILSLOT_GET              ] = KernMailslot_SysCall_Get;
    sgSysCall[K2OS_SYSCALL_ID_MAILBOX_RECVFIRST         ] = KernMailbox_SysCall_RecvFirst;
    sgSysCall[K2OS_SYSCALL_ID_IFINST_CREATE             ] = KernIfInst_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_IFINST_SETMAILBOX         ] = KernIfInst_SysCall_SetMailbox;
    sgSysCall[K2OS_SYSCALL_ID_IFINST_PUBLISH            ] = KernIfInst_SysCall_Publish;
    sgSysCall[K2OS_SYSCALL_ID_IFINSTID_GETDETAIL        ] = KernIfInstId_SysCall_GetDetail;
    sgSysCall[K2OS_SYSCALL_ID_IFENUM_CREATE             ] = KernIfEnum_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_IFENUM_RESET              ] = KernIfEnum_SysCall_Reset;
    sgSysCall[K2OS_SYSCALL_ID_IFENUM_NEXT               ] = KernIfEnum_SysCall_Next;
    sgSysCall[K2OS_SYSCALL_ID_IFSUBS_CREATE             ] = KernIfSubs_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_CREATE             ] = KernIpcEnd_SysCall_Create;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_SEND               ] = KernIpcEnd_SysCall_Send;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_ACCEPT             ] = KernIpcEnd_SysCall_Accept;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_MANUAL_DISCONNECT  ] = KernIpcEnd_SysCall_ManualDisconnect;
    sgSysCall[K2OS_SYSCALL_ID_IPCEND_REQUEST            ] = KernIpcEnd_SysCall_Request;
    sgSysCall[K2OS_SYSCALL_ID_IPCREQ_REJECT             ] = KernIpcEnd_SysCall_RejectRequest;
    sgSysCall[K2OS_SYSCALL_ID_SYSPROC_REGISTER          ] = KernSysProc_SysCall_Register;

    sgDpc_OneTimeInitInMonitor.Func = KernThread_OneTimeInitInMonitor;
    KernCpu_QueueDpc(&sgDpc_OneTimeInitInMonitor.Dpc, &sgDpc_OneTimeInitInMonitor.Func, KernDpcPrio_Med);
}

K2STAT  
KernThread_Create(
    K2OSKERN_OBJ_PROCESS *      apProc,
    K2OS_THREAD_CONFIG const *  apConfig,
    K2OSKERN_OBJ_THREAD **      appRetThread
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

        if (NULL != apProc)
        {
            KernObj_CreateRef(&pNewThread->User.ProcRef, &apProc->Hdr);
        }

        do
        {
            newThreadIx = gData.Proc.mNextThreadSlotIx;
        } while (newThreadIx != K2ATOMIC_CompareExchange(&gData.Proc.mNextThreadSlotIx, pThreadPtrs[newThreadIx], newThreadIx));
        K2_ASSERT(newThreadIx != 0);
        K2_ASSERT(newThreadIx < K2OS_MAX_THREAD_COUNT);
        K2_ASSERT(0 != gData.Proc.mNextThreadSlotIx);

        pNewThread->mGlobalIx = newThreadIx;
        pThreadPtrs[newThreadIx] = (UINT32)pNewThread;

        pNewThread->mpKernRwViewOfThreadPage = (K2OS_THREAD_PAGE *)
            (K2OS_KVA_TLSAREA_BASE + (newThreadIx * K2_VA_MEMPAGE_BYTES));

        if (NULL != apProc)
        {
            disp = K2OSKERN_SeqLock(&apProc->PageList.SeqLock);
            K2_ASSERT(pTrack->Flags.Field.PageListIx == KernPhysPageList_None);
            pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_ThreadTls;
            K2LIST_AddAtTail(&apProc->PageList.Locked.ThreadTls, &pTrack->ListLink);
            K2OSKERN_SeqUnlock(&apProc->PageList.SeqLock, disp);
        }
        else
        {
            disp = K2OSKERN_SeqLock(&gData.Phys.SeqLock);
            K2_ASSERT(pTrack->Flags.Field.PageListIx == KernPhysPageList_None);
            pTrack->Flags.Field.PageListIx = KernPhysPageList_Kern_ThreadTls;
            K2LIST_AddAtTail(&gData.Phys.PageList[KernPhysPageList_Kern_ThreadTls], &pTrack->ListLink);
            K2OSKERN_SeqUnlock(&gData.Phys.SeqLock, disp);
        }

        pNewThread->mTlsPagePhys = physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
        KernPhys_ZeroPage(physPageAddr);

        if (NULL != apProc)
        {
            KernPte_MakePageMap(apProc, K2OS_UVA_TLSAREA_BASE + (newThreadIx * K2_VA_MEMPAGE_BYTES), physPageAddr, K2OS_MAPTYPE_USER_DATA);
        }

        KernPte_MakePageMap(NULL, (UINT32)pNewThread->mpKernRwViewOfThreadPage, physPageAddr, K2OS_MAPTYPE_KERN_DATA);

        K2MEM_Copy(&pNewThread->Config, apConfig, sizeof(K2OS_THREAD_CONFIG));
        K2MEM_Zero(pNewThread->mpKernRwViewOfThreadPage, sizeof(K2OS_THREAD_PAGE));

        pNewThread->mpKernRwViewOfThreadPage->mContext = (UINT32)pNewThread;

        pNewThread->mState = KernThreadState_InitNoPrep;

        if (NULL != apProc)
        {
            disp = K2OSKERN_SeqLock(&apProc->Thread.SeqLock);
            K2LIST_AddAtTail(&apProc->Thread.Locked.CreateList, &pNewThread->OwnerThreadListLink);
            K2OSKERN_SeqUnlock(&apProc->Thread.SeqLock, disp);
        }
        else
        {
            disp = K2OSKERN_SeqLock(&gData.Thread.ListSeqLock);
            K2LIST_AddAtTail(&gData.Thread.CreateList, &pNewThread->OwnerThreadListLink);
            K2OSKERN_SeqUnlock(&gData.Thread.ListSeqLock, disp);
        }

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

    mapRef[0].AsAny = mapRef[1].AsAny = NULL;

    stat = KernProc_AcqMaps(
        apCurThread->User.ProcRef.AsProc,
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
    KernArch_TrapToContext(&pTrap->SavedContext, &apCurThread->User.ArchExecContext, TRUE);

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
        "\n*** Thread %d caused unhandled exception 0x%08X, will crash process %d\n\n", 
        apCurThread->mGlobalIx, 
        apEx->mExCode,
        apCurThread->User.ProcRef.AsProc->mId);

    if (apCurThread->User.ProcRef.AsProc->mId == 1)
    {
//        K2OSKERN_Panic("SysProc Crash\n");
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
        KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
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

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, User.CoW.DpcSimple.Func);
    pEx = &pThread->User.LastEx;

    KTRACE(apThisCore, 3, KTRACE_THREAD_COW_SENDICI_DPC, pThread->User.ProcRef.AsProc->mId, pThread->mGlobalIx);

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
        pThread->User.CoW.DpcSimple.Func = KernThread_CoW_Check_Complete;
    }
    else
    {
        pThread->User.CoW.DpcSimple.Func = KernThread_CoW_Dpc_SendIci;
    }

    KernCpu_QueueDpc(&pThread->User.CoW.DpcSimple.Dpc, &pThread->User.CoW.DpcSimple.Func, KernDpcPrio_Med);
}

void
KernThread_CoW_Dpc_Complete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_THREAD_EX *    pEx;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, User.CoW.DpcSimple.Func);
    pEx = &pThread->User.LastEx;

    KernObj_ReleaseRef(&pEx->MapRef);

    KTRACE(apThisCore, 3, KTRACE_THREAD_COW_COMPLETE, pThread->User.ProcRef.AsProc->mId, pThread->mGlobalIx);

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

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, User.CoW.DpcSimple.Func);

    KTRACE(apThisCore, 3, KTRACE_THREAD_COW_CHECK_DPC, pThread->User.ProcRef.AsProc->mId, pThread->mGlobalIx);

    if (0 == pThread->TlbShoot.mCoresRemaining)
    {
        KernThread_CoW_Dpc_Complete(apThisCore, apKey);
        return;
    }

    pThread->User.CoW.DpcSimple.Func = KernThread_CoW_Check_Complete;
    KernCpu_QueueDpc(&pThread->User.CoW.DpcSimple.Dpc, &pThread->User.CoW.DpcSimple.Func, KernDpcPrio_Med);
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
    K2OSKERN_OBJ_VIRTMAP *  pMap;

    UINT32                  virtAddrSrc;
    UINT32                  virtAddrDst;
    UINT32                  physAddr;
    BOOL                    disp;
    K2LIST_LINK *           pListLink;
    K2OSKERN_PHYSTRACK *    pTrack;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, User.CoW.DpcSimple.Func);
    pProc = pThread->User.ProcRef.AsProc;
    pEx = &pThread->User.LastEx;
    pMap = pEx->MapRef.AsVirtMap;
    K2_ASSERT(NULL != pMap);

    KTRACE(apThisCore, 3, KTRACE_THREAD_COW_COPY_DPC, pThread->User.ProcRef.AsProc->mId, pThread->mGlobalIx);

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

    physAddr = KernPageArray_PagePhys(pMap->PageArrayRef.AsPageArray, pMap->mPageArrayStartPageIx + pEx->mMapPageIx);
    KernPte_MakePageMap(NULL, virtAddrSrc, physAddr, K2OS_MAPTYPE_KERN_READ);

    physAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
    KernPte_MakePageMap(NULL, virtAddrDst, physAddr, K2OS_MAPTYPE_KERN_DATA);

    K2MEM_Copy((void *)virtAddrDst, (void *)virtAddrSrc, K2_VA_MEMPAGE_BYTES);
    K2OS_CacheOperation(K2OS_CACHEOP_FlushDataRange, virtAddrDst, K2_VA_MEMPAGE_BYTES);

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
    if (apThisCore->MappedProcRef.AsProc == pProc)
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
        KernThread_CoW_Dpc_SendIci(apThisCore, &pThread->User.CoW.DpcSimple.Func);
    }
    else
    {
        KernThread_CoW_Dpc_Complete(apThisCore, &pThread->User.CoW.DpcSimple.Func);
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
    K2OSKERN_OBJ_VIRTMAP *  pMap;
    BOOL                    isCow;
    BOOL                    disp;
    UINT32                  pte;

    isCow = FALSE;

    pMap = apEx->MapRef.AsVirtMap;
    if (NULL != pMap)
    {
        pProc = apCurThread->User.ProcRef.AsProc;
        disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);
        pte = pMap->mpPte[apEx->mMapPageIx];
        if ((apEx->mPageWasPresent) &&
            (pte & K2OSKERN_PTE_PRESENT_BIT) &&
            (apEx->mWasWrite) &&
            (!KernArch_PteMapsUserWriteable(pte)) &&
            (pMap->mMapType == K2OS_MapType_Data_CopyOnWrite))
        {
            //
            // mark PTE as NP and start copy-on-write processing for the thread
            //
            isCow = TRUE;
            KernPte_BreakPageMap(apCurThread->User.ProcRef.AsProc, apEx->mFaultAddr & K2_VA_PAGEFRAME_MASK, K2OSKERN_PTE_ACTIVE_COW);
            apCurThread->User.CoW.DpcSimple.Func = KernThread_CoW_Dpc_Copy;
            KernCpu_QueueDpc(&apCurThread->User.CoW.DpcSimple.Dpc, &apCurThread->User.CoW.DpcSimple.Func, KernDpcPrio_Med);
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

    pEx = &apThread->User.LastEx;

    KTRACE(apThisCore, 4, KTRACE_THREAD_EXCEPTION, apThread->User.ProcRef.AsProc->mId, apThread->mGlobalIx, pEx->mExCode);

    stat = KernProc_FindMapAndCreateRef(apThread->User.ProcRef.AsProc, pEx->mFaultAddr, &pEx->MapRef, &pEx->mMapPageIx);
    if (K2STAT_IS_ERROR(stat))
    {
        pEx->MapRef.AsAny = NULL;
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
    KTRACE(apThisCore, 4, KTRACE_THREAD_SYSCALL, apThread->User.ProcRef.AsProc->mId, apThread->mGlobalIx, apThread->User.mSysCall_Id);
    if ((0 == apThread->User.mSysCall_Id) ||
        (apThread->User.mSysCall_Id >= K2OS_SYSCALL_COUNT))
    {
        KernThread_UnimplementedSysCall(apThisCore, apThread);
    }
    else
    {
//        K2OSKERN_Debug("---SysCall(%d,%d)\n", apThread->mGlobalIx, apThread->User.mSysCall_Id);
        sgSysCall[apThread->User.mSysCall_Id](apThisCore, apThread);
    }
}

void 
KernThread_SysCall_OutputDebug(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    apCurThread->User.mSysCall_Result = K2OSKERN_Debug("%s", (char const *)apCurThread->User.mSysCall_Arg0);
}

void 
KernThread_SysCall_RaiseException(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_THREAD_EX  * pEx;

    pEx = &apCurThread->User.LastEx;
    pEx->mFaultAddr = 0;
    pEx->mPageWasPresent = FALSE;
    pEx->mWasWrite = FALSE;
    pEx->mExCode = apCurThread->User.mSysCall_Arg0 | K2STAT_FACILITY_EXCEPTION;
    apCurThread->User.mIsInSysCall = FALSE;
    KTRACE(apThisCore, 4, KTRACE_THREAD_RAISE_EX, apCurThread->User.ProcRef.AsProc->mId, apCurThread->mGlobalIx, pEx->mExCode);
    KernThread_OnException_Unhandled(apThisCore, apCurThread, pEx);
}

void
KernThread_Cleanup_Dpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD * pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(pThread->mIsKernelThread);

    KernThread_Cleanup(apThisCore, (K2OSKERN_OBJ_THREAD *)pThread->Kern.mpArg);
}

void    
KernThread_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJREF         mapRef;
    UINT32                  pageIx;
    UINT32                  crtEntry;
    UINT32                  userEntry;
    UINT32                  stackTop;
    BOOL                    ok;
    K2OSKERN_OBJ_THREAD *   pNewThread;
    K2OS_THREAD_TOKEN       tokThread;
    K2OS_THREAD_CONFIG      config;
    UINT32                  stackPtr;
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    BOOL                    disp;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

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

    //
    // entry point must not be in kernel space
    //
    if (apCurThread->User.mSysCall_Arg0 >= K2OS_KVA_KERN_BASE)
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_NOT_ALLOWED;
        return;
    }

    pProc = apCurThread->User.ProcRef.AsProc;

    do
    {
        // stack must be nonzero in pages
        if (0 == config.mStackPages)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        //
        // crt entry must be within crt text map
        //
        crtEntry = apCurThread->User.mSysCall_Arg0;
        mapRef.AsAny = NULL;
        stat = KernProc_FindMapAndCreateRef(pProc, crtEntry, &mapRef, &pageIx);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }
        if (mapRef.AsVirtMap != pProc->CrtMapRef[KernUserCrtSeg_Text].AsVirtMap)
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
        stat = KernProc_FindMapAndCreateRef(pProc, userEntry, &mapRef, &pageIx);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }
        ok = (mapRef.AsVirtMap->mMapType == K2OS_MapType_Text) ? TRUE : FALSE;
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
        stat = KernProc_FindMapAndCreateRef(pProc, stackTop, &mapRef, &pageIx);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }
        do
        {
            if ((mapRef.AsVirtMap->mMapType != K2OS_MapType_Thread_Stack) ||
                (pageIx != 0) ||
                (mapRef.AsVirtMap->mPageCount != config.mStackPages) ||
                (mapRef.AsVirtMap->User.mpProcHeapNode->HeapNode.AddrTreeNode.mUserVal != (stackTop - K2_VA_MEMPAGE_BYTES)))
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

            K2ASC_CopyLen(pNewThread->mName, (char *)pThreadPage->mMiscBuffer, K2OS_THREAD_NAME_BUFFER_CHARS - 1);
            pNewThread->mName[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;

            KernObj_CreateRef(&pNewThread->User.StackMapRef, mapRef.AsAny);
            mapRef.AsVirtMap->User.mpProcHeapNode->mUserOwned = 0;

            stackPtr = stackTop;
            stackPtr += (mapRef.AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES) - 4;
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
            disp = K2OSKERN_SeqLock(&pProc->Thread.SeqLock);
            K2LIST_Remove(&pProc->Thread.Locked.CreateList, &pNewThread->OwnerThreadListLink);
            K2LIST_AddAtTail(&pProc->Thread.Locked.DeadList, &pNewThread->OwnerThreadListLink);
            K2OSKERN_SeqUnlock(&pProc->Thread.SeqLock, disp);

            pThreadPage->mSysCall_Arg1 = (UINT32)pNewThread;
            apCurThread->Hdr.ObjDpc.Func = KernThread_Cleanup_Dpc;
            KernCpu_QueueDpc(&apCurThread->Hdr.ObjDpc.Dpc, &apCurThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
        }
        else
        {
            pSchedItem = &apCurThread->SchedItem;
            pSchedItem->mType = KernSchedItem_Thread_SysCall;
            KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
            KernObj_CreateRef(&pSchedItem->ObjRef, &pNewThread->Hdr);
            pSchedItem->Args.Thread_Create.mThreadToken = (UINT32)tokThread;
            KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
            KernSched_QueueItem(pSchedItem);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
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
    UINT32                  chk;

//    K2OSKERN_Debug("Cleanup thread %d completed\n", apThread->mGlobalIx);

    K2_ASSERT((apThread->mIsKernelThread) || (NULL == apThread->User.StackMapRef.AsAny));

    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(apThread->mTlsPagePhys);
    KernPhys_FreeTrack(pTrack);

    if (!apThread->mIsKernelThread)
    {
        KernObj_ReleaseRef(&apThread->User.ProcRef);
        K2_ASSERT(NULL == apThread->User.LastEx.MapRef.AsAny);
    }
    else
    {
        KernVirt_Release(apThread->Kern.mStackBaseVirt);
        KernPhys_FreeTrackList(&apThread->Kern.StackPhysTrackList);
    }

    for (chk = 0; chk < K2OS_THREAD_WAIT_MAX_ITEMS; chk++)
    {
        K2_ASSERT(NULL == apThread->MacroWait.WaitEntry[chk].ObjRef.AsAny);
    }

    threadSlotIx = apThread->mGlobalIx;

    K2MEM_Zero(apThread, sizeof(K2OSKERN_OBJ_THREAD));

    KernHeap_Free(apThread);

#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Thread Cleanup completed\n", threadSlotIx);
#endif

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

    KTRACE(apThisCore, 3, KTRACE_THREAD_KERNPAGE_CHECK_DPC, pThread->User.ProcRef.AsProc->mId, pThread->mGlobalIx);

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
    
    KTRACE(apThisCore, 3, KTRACE_THREAD_KERNPAGE_SENDICI_DPC, pThread->User.ProcRef.AsProc->mId, pThread->mGlobalIx);

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
KernThread_Cleanup_ThreadPage_Done(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    if (gData.mCpuCoreCount > 1)
    {
        apThread->TlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        apThread->TlbShoot.mpProc = NULL;
        apThread->TlbShoot.mVirtBase = (UINT32)apThread->mpKernRwViewOfThreadPage;
        apThread->TlbShoot.mPageCount = 1;
        apThread->mIciSendMask = apThread->TlbShoot.mCoresRemaining;
        KernThread_Cleanup_SendIci_Kern_ThreadPage(apThisCore, &apThread->Hdr.ObjDpc.Func);
    }

    KernArch_InvalidateTlbPageOnCurrentCore((UINT32)apThread->mpKernRwViewOfThreadPage);

    if (gData.mCpuCoreCount == 1)
    {
        KernThread_Cleanup_Done(apThisCore, apThread);
    }
}

void
KernThread_Cleanup_ThreadPage_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD * pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 3, KTRACE_THREAD_USERPAGE_CHECK_DPC, pThread->User.ProcRef.AsProc->mId, pThread->mGlobalIx);

    if (0 == pThread->TlbShoot.mCoresRemaining)
    {
        KernThread_Cleanup_ThreadPage_Done(apThisCore, pThread);
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_ThreadPage_CheckComplete;
        KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernThread_Cleanup_SendIci_ThreadPage(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    UINT32                  sentMask;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pThread->mIciSendMask);
    
    KTRACE(apThisCore, 3, KTRACE_THREAD_USERPAGE_CHECK_DPC, pThread->User.ProcRef.AsProc->mId, pThread->mGlobalIx);

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
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_ThreadPage_CheckComplete;
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_SendIci_ThreadPage;
    }
    KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernThread_Cleanup_StartShootDown_ThreadPage(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread,
    UINT32                      aUserPageVirt
)
{
    if (gData.mCpuCoreCount > 1)
    {
        apThread->TlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        apThread->TlbShoot.mpProc = apThread->User.ProcRef.AsProc;
        apThread->TlbShoot.mVirtBase = aUserPageVirt;
        apThread->TlbShoot.mPageCount = 1;
        apThread->mIciSendMask = apThread->TlbShoot.mCoresRemaining;
        KernThread_Cleanup_SendIci_ThreadPage(apThisCore, &apThread->Hdr.ObjDpc.Func);
    }

    KernArch_InvalidateTlbPageOnCurrentCore(aUserPageVirt);

    if (gData.mCpuCoreCount == 1)
    {
        KernThread_Cleanup_ThreadPage_Done(apThisCore, apThread);
    }
}

void
KernThread_Cleanup_KernelThreadMemory_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD * pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 2, KTRACE_THREAD_KERNMEM_TCHECK_DPC, pThread->mGlobalIx);

    if (0 == pThread->TlbShoot.mCoresRemaining)
    {
        KernThread_Cleanup_Done(apThisCore, pThread);
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_KernelThreadMemory_CheckComplete;
        KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernThread_RecvThreadTlbInv(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_TLBSHOOT *         apShoot
)
{
    UINT32                  virtWork;
    UINT32                  pagesLeft;
    K2OSKERN_OBJ_THREAD *   pThread;

    KTRACE(apThisCore, 1, KTRACE_KERN_TLBSHOOT_ICI);

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apShoot, TlbShoot);

    KernArch_InvalidateTlbPageOnCurrentCore((UINT32)pThread->mpKernRwViewOfThreadPage);

    pagesLeft = pThread->Config.mStackPages;
    virtWork = pThread->Kern.mStackBaseVirt + K2_VA_MEMPAGE_BYTES;
    do
    {
        KernArch_InvalidateTlbPageOnCurrentCore(virtWork);
        virtWork += K2_VA_MEMPAGE_BYTES;
    } while (--pagesLeft);

    K2ATOMIC_And(&apShoot->mCoresRemaining, ~(1 << apThisCore->mCoreIx));
}

void
KernThread_Cleanup_SendIci_KernelThreadMemory(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    UINT32                  sentMask;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pThread->mIciSendMask);

    KTRACE(apThisCore, 2, KTRACE_THREAD_KERNMEM_TSEND_DPC, pThread->mGlobalIx);

    sentMask = KernArch_SendIci(
        apThisCore,
        pThread->mIciSendMask,
        KernIci_KernThreadTlbInv,
        &pThread->TlbShoot
    );

    pThread->mIciSendMask &= ~sentMask;

    if (0 == pThread->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_KernelThreadMemory_CheckComplete;
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernThread_Cleanup_SendIci_KernelThreadMemory;
    }
    KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernThread_Cleanup_StartShootDown_KernelThreadMemory(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread,
    UINT32                      aThreadPageVirt
)
{
    UINT32 virtAddr;
    UINT32 ixPage;

    if (gData.mCpuCoreCount > 1)
    {
        apThread->TlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        apThread->TlbShoot.mpProc = NULL;
        apThread->TlbShoot.mVirtBase = aThreadPageVirt;
        apThread->TlbShoot.mPageCount = 1;
        apThread->mIciSendMask = apThread->TlbShoot.mCoresRemaining;
        KernThread_Cleanup_SendIci_KernelThreadMemory(apThisCore, &apThread->Hdr.ObjDpc.Func);
    }

    KernArch_InvalidateTlbPageOnCurrentCore(aThreadPageVirt);

    virtAddr = apThread->Kern.mStackBaseVirt + K2_VA_MEMPAGE_BYTES;
    for (ixPage = 0; ixPage < apThread->Config.mStackPages; ixPage++)
    {
        KernArch_InvalidateTlbPageOnCurrentCore(virtAddr);
        virtAddr += K2_VA_MEMPAGE_BYTES;
    }

    if (gData.mCpuCoreCount == 1)
    {
        KernThread_Cleanup_Done(apThisCore, apThread);
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
    UINT32                  threadPageVirt;
    K2OSKERN_PHYSTRACK *    pTrack;
    UINT32                  ixPage;
    UINT32                  virtAddr;

//    K2OSKERN_Debug("Cleanup thread %d\n", apThread->mGlobalIx);

    K2_ASSERT(apThread->mState == KernThreadState_Exited);
    K2_ASSERT(NULL == apThread->SchedItem.ObjRef.AsAny);

    if (apThread->mIsKernelThread)
    {
        pProc = NULL;
        disp = K2OSKERN_SeqLock(&gData.Thread.ListSeqLock);
        K2LIST_Remove(&gData.Thread.DeadList, &apThread->OwnerThreadListLink);
        K2OSKERN_SeqUnlock(&gData.Thread.ListSeqLock, disp);

        //
        // break the kernel mapping of the thread stack
        //
        virtAddr = apThread->Kern.mStackBaseVirt;
        virtAddr += K2_VA_MEMPAGE_BYTES;
        for (ixPage = 0; ixPage < apThread->Config.mStackPages; ixPage++)
        {
            KernPte_BreakPageMap(NULL, virtAddr, 0);
            virtAddr += K2_VA_MEMPAGE_BYTES;
        }
    }
    else
    {
        pProc = apThread->User.ProcRef.AsProc;
        disp = K2OSKERN_SeqLock(&pProc->Thread.SeqLock);
        K2LIST_Remove(&pProc->Thread.Locked.DeadList, &apThread->OwnerThreadListLink);
        K2OSKERN_SeqUnlock(&pProc->Thread.SeqLock, disp);

        threadPageVirt = K2OS_UVA_TLSAREA_BASE + (apThread->mGlobalIx * K2_VA_MEMPAGE_BYTES);
        if (NULL != apThread->User.StackMapRef.AsAny)
        {
            KernObj_ReleaseRef(&apThread->User.StackMapRef);
        }
        if (pProc->mState != KernProcState_Stopped)
        {
            pagePhys = KernPte_BreakPageMap(pProc, threadPageVirt, 0);
            K2_ASSERT(pagePhys == apThread->mTlsPagePhys);
        }
    }

    //
    // break the kernel mapping of the thread page
    //
    pagePhys = KernPte_BreakPageMap(NULL, (UINT32)apThread->mpKernRwViewOfThreadPage, 0);
    K2_ASSERT(pagePhys == apThread->mTlsPagePhys);

    //
    // free the thread page physical page off the tracked list
    //
    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(pagePhys);

    if (apThread->mIsKernelThread)
    {
        disp = K2OSKERN_SeqLock(&gData.Phys.SeqLock);
        K2_ASSERT(pTrack->Flags.Field.PageListIx == KernPhysPageList_Kern_ThreadTls);
        K2LIST_Remove(&gData.Phys.PageList[KernPhysPageList_Kern_ThreadTls], &pTrack->ListLink);
        pTrack->Flags.Field.PageListIx = KernPhysPageList_None;
        K2OSKERN_SeqUnlock(&gData.Phys.SeqLock, disp);

        KernThread_Cleanup_StartShootDown_KernelThreadMemory(apThisCore, apThread, threadPageVirt);
    }
    else
    {
        disp = K2OSKERN_SeqLock(&pProc->PageList.SeqLock);
        K2_ASSERT(pTrack->Flags.Field.PageListIx == KernPhysPageList_Proc_ThreadTls);
        K2LIST_Remove(&pProc->PageList.Locked.ThreadTls, &pTrack->ListLink);
        pTrack->Flags.Field.PageListIx = KernPhysPageList_None;
        K2OSKERN_SeqUnlock(&pProc->PageList.SeqLock, disp);

        if (pProc->mState != KernProcState_Stopped)
        {
            //        K2OSKERN_Debug("Thread %d cleanup when process NOT exited\n", apThread->mGlobalIx);
            KernThread_Cleanup_StartShootDown_ThreadPage(apThisCore, apThread, threadPageVirt);
        }
        else
        {
            //        K2OSKERN_Debug("Thread %d cleanup when process exited\n", apThread->mGlobalIx);
            KernThread_Cleanup_ThreadPage_Done(apThisCore, apThread);
        }
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
    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
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

    oldAffinity = apCurThread->Config.mAffinityMask;

    desired = apCurThread->User.mSysCall_Arg0;

    if (desired == (UINT32)-1)
    {
        apCurThread->User.mSysCall_Result = oldAffinity;
        return;
    }

    desired &= ((1 << gData.mCpuCoreCount) - 1);
    if (0 == desired)
    {
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        apCurThread->User.mSysCall_Result = 0;
        return;
    }

    apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
    apCurThread->SchedItem.Args.Thread_SetAff.mNewMask = desired;
    KernArch_GetHfTimerTick(&apCurThread->SchedItem.mHfTick);
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

    threadRef.AsAny = NULL;
    stat = KernProc_TokenTranslate(
        apCurThread->User.ProcRef.AsProc,
        (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0,
        &threadRef
    );
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Thread == threadRef.AsAny->mObjType)
        {
            if (threadRef.AsThread->mState == KernThreadState_Exited)
            {
                apCurThread->User.mSysCall_Result = threadRef.AsThread->mExitCode;
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
        apCurThread->User.mSysCall_Result = (UINT32)-1;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
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

void
KernThread_SysCall_SetName(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    K2ASC_CopyLen(apCurThread->mName, (char *)pThreadPage->mMiscBuffer, K2OS_THREAD_NAME_BUFFER_CHARS - 1);
    apCurThread->mName[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;

    apCurThread->User.mSysCall_Result = (UINT32)TRUE;
}

void
KernThread_SysCall_GetName(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    K2ASC_CopyLen((char *)pThreadPage->mMiscBuffer, apCurThread->mName, K2OS_THREAD_NAME_BUFFER_CHARS - 1);
    pThreadPage->mMiscBuffer[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;

    apCurThread->User.mSysCall_Result = (UINT32)TRUE;
}

UINT32
KernThread_GetId(
    void
)
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    BOOL                        disp;

    disp = K2OSKERN_SetIntr(FALSE);
    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    pThisThread = pThisCore->mpActiveThread;
    K2_ASSERT(!pThisCore->mIsInMonitor);        // being called from inside monitor
    K2_ASSERT(NULL != pThisThread);             // being called from monitor with no thread
    K2_ASSERT(pThisThread->mIsKernelThread);    // being called on non-kernel thread
    K2OSKERN_SetIntr(disp);

    return pThisThread->mGlobalIx;
}

void
KernThread_Exit(
    UINT32 aExitCode
)
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_SCHED_ITEM *       pSchedItem;

    // if interrupts are off core may change
    K2OSKERN_SetIntr(FALSE);

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    pThisThread = pThisCore->mpActiveThread;

    K2_ASSERT(pThisThread->mIsKernelThread);

    pThisThread->mExitCode = aExitCode;

    pSchedItem = &pThisThread->SchedItem;
    pSchedItem->mType = KernSchedItem_KernThread_Exit;

    KernThread_CallScheduler(pThisCore);

    K2OSKERN_Panic("KernThread_Exit call to enter scheduler returned!\n");
}

void
KernThread_CallScheduler(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    K2OSKERN_OBJ_THREAD * pThisThread;

    if (K2OSKERN_GetIntr())
    {
        K2OSKERN_Panic("KernThread_EnterScheduler() with interrupts on!\n");
        return;
    }

    pThisThread = apThisCore->mpActiveThread;

    K2_ASSERT(pThisThread->mIsKernelThread);

    KernCpu_SetTickMode(apThisCore, KernTickMode_Kern);

    KernArch_GetHfTimerTick(&pThisThread->SchedItem.mHfTick);

    KernCpu_TakeCurThreadOffThisCore(apThisCore, pThisThread, KernThreadState_InScheduler);

    KernSched_QueueItem(&pThisThread->SchedItem);

    apThisCore->mIsInMonitor = TRUE;

    KernArch_IntsOff_EnterMonitorFromKernelThread(apThisCore, &pThisThread->Kern.mStackPtr);

    //
    // this is return point from a call to the scheduler
    // interrupts will be on
    K2_ASSERT(K2OSKERN_GetIntr());
}

