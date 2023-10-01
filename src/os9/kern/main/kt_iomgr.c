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

typedef struct _PROCIO PROCIO;
struct _PROCIO
{
    K2OSKERN_OBJREF RefProc;
    K2LIST_ANCHOR   ThreadsInIoList;
    K2LIST_LINK     ListLink;
};

void
KernIoMgr_UserThread_Return(
    K2OSKERN_OBJ_THREAD *   apThread,
    UINT32                  aResult,
    BOOL                    aSetStat,
    K2STAT                  aStat
)
{
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    // release the blockio that is referred to by the iop
    K2_ASSERT(NULL != apThread->ThreadIo.Iop.RefObj.AsAny);
    KernObj_ReleaseRef(&apThread->ThreadIo.Iop.RefObj);

    // switch on syscall id if necessary
    apThread->User.mSysCall_Result = aResult;
    if (aSetStat)
    {
        apThread->mpKernRwViewOfThreadPage->mLastStatus = aStat;
    }

    // go into the scheduler to take the appropriate next action for the thread (resume, probably)
    apThread->mState = KernThreadState_InScheduler;
    pSchedItem = &apThread->SchedItem;
    apThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
    KernSched_QueueItem(pSchedItem);
}

void
KernIoMgr_UserThread_IoComplete(
    K2OSKERN_OBJ_THREAD *   apThread,
    UINT32                  aResult,
    BOOL                    aSetStat,
    K2STAT                  aStat
)
{
    K2LIST_LINK *           pListLink;
    PROCIO *                pProcIo;
    K2OSKERN_OBJ_THREAD *   pScan;

    K2OS_CritSec_Enter(&gData.IoMgr.Sec);

    pListLink = gData.IoMgr.ProcIoList.mpHead;
    K2_ASSERT(NULL != pListLink);
    do {
        pProcIo = K2_GET_CONTAINER(PROCIO, pListLink, ListLink);
        if (apThread->User.ProcRef.AsProc == pProcIo->RefProc.AsProc)
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
    K2_ASSERT(NULL != pProcIo);

#if K2_DEBUG
    pListLink = pProcIo->ThreadsInIoList.mpHead;
    K2_ASSERT(NULL != pListLink);
    do {
        pScan = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListLink, ThreadIo.ProcIoListLink);
        if (pScan == apThread)
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
    K2_ASSERT(NULL != pListLink);
#endif

    K2LIST_Remove(&pProcIo->ThreadsInIoList, &apThread->ThreadIo.ProcIoListLink);

    if (0 == pProcIo->ThreadsInIoList.mNodeCount)
    {
        K2LIST_Remove(&gData.IoMgr.ProcIoList, &pProcIo->ListLink);
    }
    else
    {
        pProcIo = NULL;
    }

    K2OS_CritSec_Leave(&gData.IoMgr.Sec);

    if (NULL != pProcIo)
    {
        KernObj_ReleaseRef(&pProcIo->RefProc);
        K2OS_Heap_Free(pProcIo);
    }

    KernIoMgr_UserThread_Return(apThread, aResult, aSetStat, aStat);
}

void
KernIoMgr_UserThread_GetMedia(
    K2OSKERN_OBJ_THREAD *   apThread,
    K2OSKERN_OBJ_BLOCKIO *  apBlockIo,
    KERN_IOP *              apIop
)
{
    K2OSEXEC_IOTHREAD * pIoThread;

    apIop->ExecIop.mType = K2OSExecIop_BlockIo;

    apIop->ExecIop.mOwner = (UINT32)apThread->User.ProcRef.AsProc->mId;

    apIop->ExecIop.Op.BlockIo.mOp = K2OSExecIop_BlockIoOp_GetMedia;

    // address of getmedia is always kernel virtual addr in thread page
    apIop->ExecIop.Op.BlockIo.mMemAddr = (UINT32)&apThread->mpKernRwViewOfThreadPage->mMiscBuffer;
    
    pIoThread = apBlockIo->mpIoThread;

    pIoThread->Submit(pIoThread, &apIop->ExecIop);
}

void
KernIoMgr_UserThread_RangeOp(
    K2OSKERN_OBJ_THREAD *   apThread,
    K2OSKERN_OBJ_BLOCKIO *  apBlockIo,
    KERN_IOP *              apIop
)
{
    K2OSEXEC_IOTHREAD * pIoThread;
    K2OS_THREAD_PAGE *  pThreadPage;

    pThreadPage = apThread->mpKernRwViewOfThreadPage;

    apIop->ExecIop.mType = K2OSExecIop_BlockIo;
    if (K2OS_SYSCALL_ID_BLOCKIO_RANGE_CREATE == apThread->User.mSysCall_Id)
    {
        apIop->ExecIop.Op.BlockIo.mOp = K2OSExecIop_BlockIoOp_RangeCreate;
        apIop->ExecIop.Op.BlockIo.mOffset = pThreadPage->mSysCall_Arg1;
        apIop->ExecIop.Op.BlockIo.mCount = pThreadPage->mSysCall_Arg2;
        apIop->ExecIop.Op.BlockIo.mRange = pThreadPage->mSysCall_Arg3;    // makePrivate
    }
    else
    {
        apIop->ExecIop.Op.BlockIo.mOp = K2OSExecIop_BlockIoOp_RangeDelete;
        apIop->ExecIop.Op.BlockIo.mRange = pThreadPage->mSysCall_Arg1;
    }

    apIop->ExecIop.mOwner = (UINT32)apThread->User.ProcRef.AsProc->mId;

    pIoThread = apBlockIo->mpIoThread;
    pIoThread->Submit(pIoThread, &apIop->ExecIop);
}

K2STAT
KernIoMgr_UserThread_Transfer(
    K2OSKERN_OBJ_THREAD *   apThread,
    K2OSKERN_OBJ_BLOCKIO *  apBlockIo,
    KERN_IOP *              apIop
)
{
    K2OS_THREAD_PAGE *          pThreadPage;
    UINT32                      firstIoSize;
    UINT32                      firstIoPages;
    UINT32                      userVirt;
    K2OSKERN_OBJREF             mapRef;
    UINT32                      pageIx;
    K2STAT                      stat;
    UINT32                      mapPagesLeft;
    UINT32                      pageArrayPagesLeft;
    K2OSKERN_OBJ_PAGEARRAY *    pPageArray;
    UINT32                      firstPageOffset;
    K2OSEXEC_IOTHREAD *         pIoThread;

    pThreadPage = apThread->mpKernRwViewOfThreadPage;

    userVirt = pThreadPage->mSysCall_Arg5_Result2;
    firstPageOffset = (userVirt & K2_VA_MEMPAGE_OFFSET_MASK);

    //
    // validate entire IO range one time
    //
    pIoThread = apBlockIo->mpIoThread;
    if (!pIoThread->ValidateRange(pIoThread,
        (K2OS_BLOCKIO_RANGE)pThreadPage->mSysCall_Arg1,     // range
        pThreadPage->mSysCall_Arg2,     // bytes offset
        pThreadPage->mSysCall_Arg3,     // byte count
        firstPageOffset
    ))
    {
        return K2STAT_ERROR_OUT_OF_BOUNDS;
    }

    //
    // if we get here the requested data can be accessed without 
    // going out of bounds of the specified range
    //

    mapRef.AsAny = NULL;
    stat = KernProc_FindMapAndCreateRef(apThread->User.ProcRef.AsProc, userVirt, &mapRef, &pageIx);
    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    mapPagesLeft = mapRef.AsVirtMap->mPageCount - pageIx;
    pPageArray = mapRef.AsVirtMap->PageArrayRef.AsPageArray;
    pageArrayPagesLeft = pPageArray->mPageCount - (mapRef.AsVirtMap->mPageArrayStartPageIx + pageIx);
    K2_ASSERT(mapPagesLeft <= pageArrayPagesLeft);

    firstIoSize = K2_VA_MEMPAGE_BYTES - firstPageOffset;

    switch (pPageArray->mType)
    {
    case KernPageArray_Sparse:
    case KernPageArray_PreMap:
        // must go page by page for i/o in this kind of a page array
        break;

    case KernPageArray_Track:
    case KernPageArray_Spec: 
        // can go from current page to end of the map as the physical memory is contiguous
        firstIoSize += ((mapPagesLeft - 1) * K2_VA_MEMPAGE_BYTES);
        break;

    default:
        K2_ASSERT(0);
        return K2STAT_ERROR_NOT_IMPL;
    }

    // clip max to what was asked for
    if (firstIoSize > pThreadPage->mSysCall_Arg3)
    {
        firstIoSize = pThreadPage->mSysCall_Arg3;
    }
    firstIoPages = (firstIoSize + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;

    if (pIoThread->mUsePhysAddr)
    {
        // physical address for transfer (fast)
        apIop->ExecIop.Op.BlockIo.mMemAddr = 
            KernPageArray_PagePhys(pPageArray, mapRef.AsVirtMap->mPageArrayStartPageIx + pageIx) +
            firstPageOffset;
        if (0 == apIop->ExecIop.Op.BlockIo.mMemAddr)
        {
            stat = K2STAT_ERROR_UNKNOWN;
        }
        else
        {
            stat = K2STAT_NO_ERROR;
        }
    }
    else
    {
        // must map to kernel virtual for transfer (slow)
        apThread->ThreadIo.mKernMapVirtAddr = K2OS_Virt_Reserve(firstIoPages);
        if (0 == apThread->ThreadIo.mKernMapVirtAddr)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            stat = KernVirtMap_Create(pPageArray,
                mapRef.AsVirtMap->mPageArrayStartPageIx + pageIx,
                firstIoPages,
                apThread->ThreadIo.mKernMapVirtAddr,
                K2OS_MapType_Data_ReadWrite,
                &apThread->ThreadIo.RefCurKernMap
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                KernObj_CreateRef(&apThread->ThreadIo.RefCurUserMap, mapRef.AsAny);
                apIop->ExecIop.Op.BlockIo.mMemAddr = apThread->ThreadIo.mKernMapVirtAddr + firstPageOffset;
            }
            else
            {
                K2OS_Virt_Release(apThread->ThreadIo.mKernMapVirtAddr);
                apThread->ThreadIo.mKernMapVirtAddr = 0;
            }
        }
    }

    KernObj_ReleaseRef(&mapRef);

    if (K2STAT_IS_ERROR(stat))
        return stat;

    apIop->ExecIop.mType = K2OSExecIop_BlockIo;
    if (0 != pThreadPage->mSysCall_Arg4_Result3)
    {
        apIop->ExecIop.Op.BlockIo.mOp = K2OSExecIop_BlockIoOp_Write;
    }
    else
    {
        apIop->ExecIop.Op.BlockIo.mOp = K2OSExecIop_BlockIoOp_Read;
    }

    apIop->ExecIop.Op.BlockIo.mRange = pThreadPage->mSysCall_Arg1;
    apIop->ExecIop.Op.BlockIo.mOffset = pThreadPage->mSysCall_Arg2;
    apIop->ExecIop.Op.BlockIo.mCount = firstIoSize;

    apIop->ExecIop.mOwner = (UINT32)apThread->User.ProcRef.AsProc->mId;

    pIoThread->Submit(pIoThread, &apIop->ExecIop);

    return K2STAT_NO_ERROR;
}

void
KernIoMgr_UserThread_Erase(
    K2OSKERN_OBJ_THREAD *   apThread,
    K2OSKERN_OBJ_BLOCKIO *  apBlockIo,
    KERN_IOP *              apIop
)
{
    K2OSEXEC_IOTHREAD * pIoThread;
    K2OS_THREAD_PAGE *  pThreadPage;

    pThreadPage = apThread->mpKernRwViewOfThreadPage;

    apIop->ExecIop.mType = K2OSExecIop_BlockIo;
    apIop->ExecIop.Op.BlockIo.mOp = K2OSExecIop_BlockIoOp_Erase;

    apIop->ExecIop.Op.BlockIo.mRange = pThreadPage->mSysCall_Arg1;
    apIop->ExecIop.Op.BlockIo.mOffset = pThreadPage->mSysCall_Arg2;
    apIop->ExecIop.Op.BlockIo.mCount = pThreadPage->mSysCall_Arg3;

    apIop->ExecIop.mOwner = (UINT32)apThread->User.ProcRef.AsProc->mId;

    pIoThread = apBlockIo->mpIoThread;
    pIoThread->Submit(pIoThread, &apIop->ExecIop);
}

void
KernIoMgr_Ingest_UserThread_BlockIo(
    KERN_IOP *  apIop
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_BLOCKIO *  pBlockIo;
    K2LIST_LINK *           pListLink;
    PROCIO *                pProcIo;
    UINT32                  detached;
    K2STAT                  stat;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apIop, ThreadIo.Iop);
    K2_ASSERT(pThread->mState == KernThreadState_InIo);
    K2_ASSERT(NULL != apIop->RefObj.AsAny);
    K2_ASSERT(KernObj_BlockIo == apIop->RefObj.AsAny->mObjType);
    pBlockIo = apIop->RefObj.AsBlockIo;
    K2_ASSERT(NULL != pBlockIo->mpIoThread);

    detached = pBlockIo->mDetached;
    K2_CpuReadBarrier();
    if (detached)
    {
        KernIoMgr_UserThread_Return(pThread, 0, TRUE, K2STAT_ERROR_NO_INTERFACE);
        return;
    }

    K2OS_CritSec_Enter(&gData.IoMgr.Sec);

    pListLink = gData.IoMgr.ProcIoList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProcIo = K2_GET_CONTAINER(PROCIO, pListLink, ListLink);
            if (pProcIo->RefProc.AsProc == pThread->User.ProcRef.AsProc)
                break;
        } while (NULL != pListLink);
    }

    if (NULL == pListLink)
    {
        K2OS_CritSec_Leave(&gData.IoMgr.Sec);

        pProcIo = (PROCIO *)K2OS_Heap_Alloc(sizeof(PROCIO));
        if (NULL == pProcIo)
        {
            KernIoMgr_UserThread_Return(pThread, 0, TRUE, K2STAT_ERROR_OUT_OF_MEMORY);
            return;
        }

        K2MEM_Zero(pProcIo, sizeof(PROCIO));
        K2LIST_Init(&pProcIo->ThreadsInIoList);
        KernObj_CreateRef(&pProcIo->RefProc, pThread->User.ProcRef.AsAny);

        K2OS_CritSec_Enter(&gData.IoMgr.Sec);

        K2LIST_AddAtTail(&gData.IoMgr.ProcIoList, &pProcIo->ListLink);
    }
    else
    {
        K2_ASSERT(NULL != pProcIo);
    }

    K2LIST_AddAtTail(&pProcIo->ThreadsInIoList, &pThread->ThreadIo.ProcIoListLink);

    K2OS_CritSec_Leave(&gData.IoMgr.Sec);

    switch (pThread->User.mSysCall_Id)
    {
    case K2OS_SYSCALL_ID_BLOCKIO_GETMEDIA:
        stat = K2STAT_NO_ERROR;
        KernIoMgr_UserThread_GetMedia(pThread, pBlockIo, apIop);
        break;

    case K2OS_SYSCALL_ID_BLOCKIO_RANGE_CREATE:
    case K2OS_SYSCALL_ID_BLOCKIO_RANGE_DELETE:
        stat = K2STAT_NO_ERROR;
        KernIoMgr_UserThread_RangeOp(pThread, pBlockIo, apIop);
        break;

    case K2OS_SYSCALL_ID_BLOCKIO_TRANSFER:
        stat = KernIoMgr_UserThread_Transfer(pThread, pBlockIo, apIop);
        break;

    case K2OS_SYSCALL_ID_BLOCKIO_ERASE:
        stat = K2STAT_NO_ERROR;
        KernIoMgr_UserThread_Erase(pThread, pBlockIo, apIop);
        break;

    default:
        stat = K2STAT_ERROR_UNKNOWN;
        break;
    };

    if (K2STAT_IS_ERROR(stat))
    {
        //
        // command marshal or submit failed, not the op itself
        //
        KernIoMgr_UserThread_IoComplete(pThread, 0, TRUE, stat);
    }
}

void
KernIoMgr_Ingest(
    KERN_IOP *  apIop
)
{
    switch (apIop->mType)
    {
    case KernIop_UserThread_BlockIo:
        KernIoMgr_Ingest_UserThread_BlockIo(apIop);
        break;

    case KernIop_KernThread_BlockIo:
        K2_ASSERT(0);
        break;

    case KernIop_ProcExit:
        K2_ASSERT(0);
        break;

    default:
        K2OSKERN_Panic("!!! Unhandled IOP type\n");
        break;
    }
}

UINT32
KernIoMgr_Thread(
    void *apArg
)
{
    K2OS_WaitResult waitResult;
    UINT32          v;
    KERN_IOP *      pInList;
    KERN_IOP *      pWork;
    KERN_IOP *      pPrev;

    do {
        v = gData.IoMgr.mAwake;
        K2_CpuReadBarrier();

        do {
            pInList = (KERN_IOP *)K2ATOMIC_Exchange((UINT32 volatile *)&gData.IoMgr.mpIopIn, 0);
            if (NULL == pInList)
                break;

            // new iops came in. reverse the list and ingest in order
            pPrev = NULL;
            do {
                pWork = pInList->mpChainNext;
                pInList->mpChainNext = pPrev;
                pPrev = pInList;
                pInList = pWork;
            } while (NULL != pWork);

            pInList = pPrev;
            do {
                pWork = pInList;
                pInList = pInList->mpChainNext;
                KernIoMgr_Ingest(pWork);
            } while (NULL != pInList);

        } while (1);

        if (v == K2ATOMIC_CompareExchange(&gData.IoMgr.mAwake, 0, v))
        {
            if (!K2OS_Thread_WaitOne(&waitResult, gData.IoMgr.mTokNotify, K2OS_TIMEOUT_INFINITE))
            {
                break;
            }
        }

    } while (1);

    K2OSKERN_Panic("!!!KernIoMgr Thread exits!\n");
    return 0;
}

void
KernIoMgr_Threaded_Init(
    void
)
{
    K2OS_THREAD_TOKEN   tokThread;
    K2STAT              stat;
    BOOL                ok;

    ok = K2OS_CritSec_Init(&gData.IoMgr.Sec);
    K2_ASSERT(ok);

    K2LIST_Init(&gData.IoMgr.ProcIoList);
    K2LIST_Init(&gData.IoMgr.InServiceList);

    stat = KernNotify_Create(FALSE, &gData.IoMgr.RefNotify);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = KernToken_Create(gData.IoMgr.RefNotify.AsAny, &gData.IoMgr.mTokNotify);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    tokThread = K2OS_Thread_Create("IoMgr", KernIoMgr_Thread, NULL, NULL, NULL);
    K2_ASSERT(NULL != tokThread);
    K2OS_Token_Destroy(tokThread);
}

void
KernIoMgr_Queue(
    KERN_IOP *  apIop
)
{
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    UINT32                  v;

    do {
        v = (UINT32)gData.IoMgr.mpIopIn;
        K2_CpuReadBarrier();
        apIop->mpChainNext = (KERN_IOP *)v;
        K2_CpuWriteBarrier();
    } while (v != K2ATOMIC_CompareExchange((UINT32 volatile *)&gData.IoMgr.mpIopIn, (UINT32)apIop, v));

    if (0 != K2ATOMIC_AddExchange((INT32 volatile *)&gData.IoMgr.mAwake, 1))
    {
        // iomgr already awake
        return;
    }

    //
    // iomgr is sleeping. wake it up.
    // only one of these type of sched events can be in flight at a time
    //
    pSchedItem = &gData.IoMgr.SchedItem;
    K2_ASSERT(pSchedItem->mType == KernSchedItem_Invalid);
    pSchedItem->mType = KernSchedItem_IoMgr_Wake;
    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
    KernSched_QueueItem(pSchedItem);
}

K2STAT  
K2OSKERN_IoMgr_SetThread(
    K2OS_IFINST_ID      aIfInstId,
    K2OSEXEC_IOTHREAD * apThread
)
{
    BOOL                    disp;
    K2OSKERN_OBJ_BLOCKIO *  pNewBlockIo;
    K2STAT                  stat;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_OBJ_IFINST *   pIfInst;

    if (NULL != apThread)
    {
        pNewBlockIo = (K2OSKERN_OBJ_BLOCKIO *)K2OS_Heap_Alloc(sizeof(K2OSKERN_OBJ_BLOCKIO));
        if (NULL == pNewBlockIo)
        {
            return K2STAT_ERROR_OUT_OF_MEMORY;
        }

        K2MEM_Zero(pNewBlockIo, sizeof(K2OSKERN_OBJ_BLOCKIO));
        pNewBlockIo->Hdr.mObjType = KernObj_BlockIo;
        K2LIST_Init(&pNewBlockIo->Hdr.RefObjList);
        pNewBlockIo->mIfInstId = aIfInstId;
        pNewBlockIo->mpIoThread = apThread;
        K2OSKERN_SeqInit(&pNewBlockIo->SeqLock);
        K2LIST_Init(&pNewBlockIo->RangeList);
    }
    else
    {
        pNewBlockIo = NULL;
    }

    stat = K2STAT_NO_ERROR;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

    pTreeNode = K2TREE_Find(&gData.Iface.IdTree, aIfInstId);
    if (NULL == pTreeNode)
    {
        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
        if (NULL != pNewBlockIo)
        {
            K2OS_Heap_Free(pNewBlockIo);
        }
        return K2STAT_ERROR_NOT_FOUND;
    }

    pIfInst = K2_GET_CONTAINER(K2OSKERN_OBJ_IFINST, pTreeNode, IdTreeNode);
    if (NULL != apThread)
    {
        //
        // attach
        //
        if (pIfInst->RefChild.AsAny != NULL)
        {
            stat = K2STAT_ERROR_IN_USE;
        }
        else
        {
            KernObj_CreateRef(&pIfInst->RefChild, &pNewBlockIo->Hdr);
        }
    }
    else
    {
        //
        // detach
        //
        if (pIfInst->RefChild.AsAny == NULL)
        {
            stat = K2STAT_ERROR_NOT_IN_USE;
        }
        else
        {
            K2ATOMIC_Exchange(&pIfInst->RefChild.AsBlockIo->mDetached, 1);
            KernObj_ReleaseRef(&pIfInst->RefChild);
        }
    }

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    return stat;
}

void    
K2OSKERN_IoMgr_Complete(
    K2OSEXEC_IOTHREAD * apThread,
    K2OSEXEC_IOP *      apExecIop,
    UINT32              aResult,
    BOOL                aSetStat,
    K2STAT              aStat
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    KERN_IOP *              pKernIop;

    pKernIop = K2_GET_CONTAINER(KERN_IOP, apExecIop, ExecIop);
    switch (pKernIop->mType)
    {
    case KernIop_UserThread_BlockIo:
        pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pKernIop, ThreadIo.Iop);
        KernIoMgr_UserThread_IoComplete(pThread, aResult, aSetStat, aStat);
        break;

    case KernIop_KernThread_BlockIo:
        K2_ASSERT(0);
        break;

    default:
        K2OSKERN_Panic("Unknown/Unsupported Iop completed\n");
        break;
    }
}
