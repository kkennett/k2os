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

K2OS_THREAD_TOKEN
K2OS_Thread_Create(
    char const *                apName,
    K2OS_pf_THREAD_ENTRY        aEntryPoint,
    void *                      apArg,
    K2OS_THREAD_CONFIG const *  apConfig,
    UINT32 *                    apRetThreadId
)
{
    K2STAT                      stat;
    K2OSKERN_OBJ_THREAD *       pNewThread;
    K2OS_THREAD_TOKEN           tokThread;
    K2OS_THREAD_CONFIG          config;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    K2OS_XDL                    parentXdl;
    BOOL                        disp;
    K2TREE_NODE *               pTreeNode;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_OBJ_VIRTMAP *      pVirtMap;
    K2OSKERN_OBJREF             ioNotifyRef;

    //
    // sanitize config
    //
    if (NULL == apConfig)
    {
        *((UINT32 *)&config) = 0;
        config.mStackPages = K2OS_THREAD_DEFAULT_STACK_PAGES;
    }
    else
    {
        *((UINT32 *)&config) = *((UINT32 *)apConfig);
    }
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
    // entry point must be in kernel space in a loaded xdl
    //
    if (((UINT32)aEntryPoint) < K2OS_KVA_KERN_BASE)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ALLOWED);
        return NULL;
    }
    parentXdl = K2OS_Xdl_AddRefContaining((UINT32)aEntryPoint);
    if (NULL == parentXdl)
    {
        K2OSKERN_Debug("Thread entrypoint %08X not found in a known XDL\n", aEntryPoint);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return NULL;
    }

    tokThread = NULL;

    do
    {
        // stack must be nonzero in pages
        if (0 == config.mStackPages)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        //
        // user entry must be within some kernel text segment
        //
        pVirtMap = NULL;

        disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);

        pTreeNode = K2TREE_FindOrAfter(&gData.VirtMap.Tree, (UINT32)aEntryPoint);
        if (pTreeNode == NULL)
        {
            pTreeNode = K2TREE_LastNode(&gData.VirtMap.Tree);
            if (NULL != pTreeNode)
            {
                pTreeNode = K2TREE_PrevNode(&gData.VirtMap.Tree, pTreeNode);
                if (pTreeNode != NULL)
                {
                    pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
                }
            }
        }
        else
        {
            if (pTreeNode->mUserVal != (UINT32)aEntryPoint)
            {
                pTreeNode = K2TREE_PrevNode(&gData.VirtMap.Tree, pTreeNode);
                if (pTreeNode != NULL)
                {
                    pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
                }
            }
            else
            {
                pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
            }
        }

        if (pVirtMap != NULL)
        {
            K2_ASSERT(NULL == pVirtMap->ProcRef.AsAny);
            K2_ASSERT(pVirtMap->OwnerMapTreeNode.mUserVal <= (UINT32)aEntryPoint);
            if ((KernSeg_Xdl_Part != pVirtMap->Kern.mSegType) ||
                (pVirtMap->Kern.XdlPart.mXdlSegmentIx != XDLSegmentIx_Text))
            {
                stat = K2STAT_ERROR_ADDR_NOT_TEXT;
                pVirtMap = NULL;
            }
        }
        else
        {
            stat = K2STAT_ERROR_NOT_FOUND;
        }

        K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);

        if (NULL == pVirtMap)
        {
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }

        ioNotifyRef.AsAny = NULL;
        stat = KernNotify_Create(FALSE, &ioNotifyRef);
        if (K2STAT_IS_ERROR(stat))
            break;

        stat = KernThread_Create(NULL, &config, &pNewThread);
        if (K2STAT_IS_ERROR(stat))
        {
            KernObj_ReleaseRef(&ioNotifyRef);
            break;
        }

        K2_ASSERT(NULL != pNewThread);

        pNewThread->mIsKernelThread = TRUE;

        KernObj_CreateRef(&pNewThread->Kern.Io.NotifyRef, ioNotifyRef.AsAny);
        KernObj_ReleaseRef(&ioNotifyRef);

        if (NULL != apName)
        {
            K2ASC_CopyLen(pNewThread->mName, apName, K2OS_THREAD_NAME_BUFFER_CHARS - 1);
            pNewThread->mName[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;
        }

        stat = KernVirtMap_CreateThreadStack(pNewThread);
        if (!K2STAT_IS_ERROR(stat))
        {
            KernArch_KernThreadPrep(pNewThread,
                (UINT32)KernThread_EntryPoint,
                &pNewThread->Kern.mStackPtr,
                (UINT32)aEntryPoint, (UINT32)apArg);

            pNewThread->Kern.mXdlRef = parentXdl;
            parentXdl = NULL;
        }

        if (K2STAT_IS_ERROR(stat))
        {
            //
            // undo KernThread_Create
            //
            disp = K2OSKERN_SetIntr(FALSE);
            K2_ASSERT(disp);

            K2OSKERN_SeqLock(&gData.Thread.ListSeqLock);
            K2LIST_Remove(&gData.Thread.CreateList, &pNewThread->OwnerThreadListLink);
            K2LIST_AddAtTail(&gData.Thread.DeadList, &pNewThread->OwnerThreadListLink);
            K2OSKERN_SeqUnlock(&gData.Thread.ListSeqLock, FALSE);

            pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
            pThisThread = pThisCore->mpActiveThread;
            K2_ASSERT(pThisThread->mIsKernelThread);

            pThisThread->mpKernRwViewOfThreadPage->mSysCall_Arg1 = (UINT32)pNewThread;
            pThisThread->Hdr.ObjDpc.Func = KernThread_Cleanup_Dpc;
            KernCpu_QueueDpc(&pThisThread->Hdr.ObjDpc.Dpc, &pThisThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);

            KernArch_IntsOff_SaveKernelThreadStateAndEnterMonitor(pThisCore, pThisThread);
            //
            // this is return point from entering the monitor to do the thread create
            // interrupts will be on
            K2_ASSERT(K2OSKERN_GetIntr());
        }

    } while (0);

    if (NULL != parentXdl)
    {
        K2OS_Xdl_Release(parentXdl);
    }

    if (!K2STAT_IS_ERROR(stat))
    {
        //
        // thread is created at this point and ready to be pushed onto the
        // run list of some core.  we need to create a token here because if
        // it fails we have to tear down the whole thing.
        //
        stat = KernToken_Create(&pNewThread->Hdr, &tokThread);

        disp = K2OSKERN_SetIntr(FALSE);
        K2_ASSERT(disp);

        pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
        pThisThread = pThisCore->mpActiveThread;
        K2_ASSERT(pThisThread->mIsKernelThread);

        if (K2STAT_IS_ERROR(stat))
        {
            //
            // undo KernThread_Create
            //
            K2OSKERN_SeqLock(&gData.Thread.ListSeqLock);
            K2LIST_Remove(&gData.Thread.CreateList, &pNewThread->OwnerThreadListLink);
            K2LIST_AddAtTail(&gData.Thread.DeadList, &pNewThread->OwnerThreadListLink);
            K2OSKERN_SeqUnlock(&gData.Thread.ListSeqLock, FALSE);

            pThisThread->mpKernRwViewOfThreadPage->mSysCall_Arg1 = (UINT32)pNewThread;
            pThisThread->Hdr.ObjDpc.Func = KernThread_Cleanup_Dpc;
            KernCpu_QueueDpc(&pThisThread->Hdr.ObjDpc.Dpc, &pThisThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);

            KernArch_IntsOff_SaveKernelThreadStateAndEnterMonitor(pThisCore, pThisThread);
            //
            // this is return point from entering the monitor to do the new thread cleanup
            // interrupts will be on
            K2_ASSERT(K2OSKERN_GetIntr());
        }
        else
        {
            // current thread is starting the new thread
            if (NULL != apRetThreadId)
            {
                *apRetThreadId = pNewThread->mGlobalIx;
            }

            pSchedItem = &pThisThread->SchedItem;
            pSchedItem->mSchedItemType = KernSchedItem_KernThread_StartThread;
            KernObj_CreateRef(&pSchedItem->ObjRef, &pNewThread->Hdr);
            pSchedItem->Args.Thread_Create.mThreadToken = (UINT32)tokThread;

            KernThread_CallScheduler(pThisCore);

            // interrupts will be back on again here
            KernObj_ReleaseRef(&pSchedItem->ObjRef);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        K2_ASSERT(NULL == tokThread);
    }

    return tokThread;
}

BOOL
K2OS_Thread_Sleep(
    UINT32 aTimeoutMs
)
{
    K2OS_WaitResult waitResult;

    if (K2OS_Thread_WaitOne(&waitResult, NULL, aTimeoutMs))
    {
        return FALSE;
    }

    if (K2STAT_ERROR_TIMEOUT != K2OS_Thread_GetLastStatus())
    {
        return FALSE;
    }

    return TRUE;
}

void
K2OS_Thread_StallMicroseconds(
    UINT32 aMicroseconds
)
{
    UINT32 work;
    UINT32 last;
    UINT32 v;

    while (aMicroseconds > 1000000)
    {
        K2OS_Thread_StallMicroseconds(1000000);
        aMicroseconds -= 1000000;
    }

    if (0 == aMicroseconds)
        return;

    work = (UINT32)((((UINT64)gData.Timer.mFreq) * ((UINT64)aMicroseconds)) / 1000000ull);

    if (0 == work)
        return;

    last = K2OS_System_GetMsTick32();
    do {
        v = K2OS_System_GetMsTick32();
        last = v - last;
        if (work <= last)
            break;
        work -= last;
        last = v;
    } while (1);
}

UINT32
K2OS_Thread_GetId(
    void
)
{
    return KernThread_GetId();
}

void
K2OS_Thread_Exit(
    UINT32 aExitCode
)
{
    KernThread_Exit(aExitCode);
}

K2STAT
K2OS_Thread_GetLastStatus(
    void
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));

    return pThreadPage->mLastStatus;
}

K2STAT
K2OS_Thread_SetLastStatus(
    K2STAT aStatus
)
{
    K2OS_THREAD_PAGE *  pThreadPage;
    UINT32              result;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));

    result = pThreadPage->mLastStatus;
    pThreadPage->mLastStatus = aStatus;

    return result;
}

UINT32
K2OS_Thread_GetCpuCoreAffinityMask(
    void
)
{
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThisThread;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;

    return pThisThread->Config.mAffinityMask;
}

UINT32
K2OS_Thread_SetCpuCoreAffinityMask(
    UINT32 aNewAffinity
)
{
    UINT32                      oldAffinity;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_SCHED_ITEM *       pSchedItem;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    aNewAffinity &= ((1 << gData.mCpuCoreCount) - 1);
    if (0 == aNewAffinity)
    {
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        return 0;
    }

    oldAffinity = pThisThread->Config.mAffinityMask;

    K2OSKERN_SetIntr(FALSE);

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

    K2_ASSERT(pThisThread == pThisCore->mpActiveThread);

    pSchedItem = &pThisThread->SchedItem;
    pSchedItem->Args.Thread_SetAff.mNewMask = aNewAffinity;
    pSchedItem->mSchedItemType = KernSchedItem_KernThread_SetAffinity;

    KernThread_CallScheduler(pThisCore);

    if (0 == pThisThread->Kern.mSchedCall_Result)
    {
        return 0;
    }

    return oldAffinity;
}

UINT32              
K2OS_Thread_GetExitCode(
    K2OS_THREAD_TOKEN aThreadToken
)
{
    K2OSKERN_OBJREF threadRef;
    K2STAT          stat;
    UINT32          result;

    if (NULL == aThreadToken)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return (UINT32)-1;
    }

    threadRef.AsAny = NULL;
    stat = KernToken_Translate(aThreadToken, &threadRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return (UINT32)-1;
    }

    result = (UINT32)-1;

    if (KernObj_Thread == threadRef.AsAny->mObjType)
    {
        if (threadRef.AsThread->mState == KernThreadState_Exited)
        {
            result = threadRef.AsThread->mExitCode;
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

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
    }

    return result;
}

BOOL
K2OS_Thread_SetName(
    char const *apName
)
{
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThisThread;
    BOOL                    disp;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    disp = K2OSKERN_SetIntr(FALSE);
    if (NULL != apName)
    {
        K2ASC_CopyLen(pThisThread->mName, apName, K2OS_THREAD_NAME_BUFFER_CHARS);
        pThisThread->mName[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;
    }
    else
    {
        pThisThread->mName[0] = 0;
    }
    K2OSKERN_SetIntr(disp);

    return TRUE;
}

void
K2OS_Thread_GetName(
    char *apRetNameBuffer
)
{
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThisThread;

    if (NULL == apRetNameBuffer)
        return;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    K2ASC_CopyLen(apRetNameBuffer, pThisThread->mName, K2OS_THREAD_NAME_BUFFER_CHARS);
}

BOOL
K2OS_Tls_AllocSlot(
    UINT32 *apRetNewIndex
)
{
    UINT64  oldVal;
    UINT64  nextBit;
    UINT64  alt;

    if (NULL == apRetNewIndex)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    do
    {
        oldVal = gData.Thread.mTlsUse;
        K2_CpuReadBarrier();
        if (0xFFFFFFFFFFFFFFFFull == oldVal)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_RESOURCES);
            return FALSE;
        }

        //
        // find first set bit in inverse
        //
        nextBit = ~oldVal;
        alt = nextBit - 1;
        alt = ((nextBit | alt) ^ alt);

    } while (oldVal != K2ATOMIC_CompareExchange64(&gData.Thread.mTlsUse, oldVal | alt, oldVal));

    K2_ASSERT(alt < K2OS_NUM_THREAD_TLS_SLOTS);

    *apRetNewIndex = alt;

    return TRUE;
}

BOOL
K2OS_Tls_FreeSlot(
    UINT32 aSlotIndex
)
{
    UINT64 oldVal;
    UINT64 newVal;

    if (aSlotIndex >= K2OS_NUM_THREAD_TLS_SLOTS)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_BOUNDS);
        return FALSE;
    }

    if (0 == ((((UINT64)1) << aSlotIndex) & gData.Thread.mTlsUse))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IN_USE);
        return FALSE;
    }

    do
    {
        oldVal = gData.Thread.mTlsUse;
        K2_CpuReadBarrier();
        if (0 == (oldVal & (1ull << aSlotIndex)))
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IN_USE);
            return FALSE;
        }
        newVal = oldVal & ~(1ull << aSlotIndex);
    } while (oldVal != K2ATOMIC_CompareExchange64(&gData.Thread.mTlsUse, newVal, oldVal));

    return TRUE;
}

BOOL
K2OS_Tls_SetValue(
    UINT32 aSlotIndex,
    UINT32 aValue
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    if (K2OS_NUM_THREAD_TLS_SLOTS <= aSlotIndex)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_BOUNDS);
        return FALSE;
    }

    if (0 == (gData.Thread.mTlsUse & (1ull << aSlotIndex)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IN_USE);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));

    pThreadPage->mTlsValue[aSlotIndex] = aValue;

    return TRUE;
}

BOOL
K2OS_Tls_GetValue(
    UINT32 aSlotIndex,
    UINT32 *apRetValue
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    if (K2OS_NUM_THREAD_TLS_SLOTS <= aSlotIndex)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_BOUNDS);
        return FALSE;
    }

    if (0 == (gData.Thread.mTlsUse & (1ull << aSlotIndex)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IN_USE);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));

    *apRetValue = pThreadPage->mTlsValue[aSlotIndex];

    return TRUE;
}
