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

void 
KernSysProc_SysCall_Register(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJ_PROCESS *      pProc;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_OBJREF             refPageArray1;
    K2OSKERN_OBJREF             refPageArray4;
    K2STAT                      stat;
    UINT32                      virtAddr;
    K2OSKERN_PROCHEAP_NODE *    pUserHeapNode;
    K2OS_SYSPROC_PAGE *         pSysProcPage;
    K2OSKERN_OBJREF             refGate;
    K2OSKERN_SCHED_ITEM *       pSchedItem;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    pProc = apCurThread->User.ProcRef.AsProc;

    if (pProc->mId != K2OS_SYSPROC_ID)
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_NOT_ALLOWED;
        return;
    }

    refGate.AsAny = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refGate);
    if ((K2STAT_IS_ERROR(stat)) ||
        (refGate.AsAny->mObjType != KernObj_Gate))
    {
        K2OSKERN_Panic("*** Sysproc did not register with a gate token\n");
    }
    stat = KernToken_Create(refGate.AsAny, &gData.SysProc.mTokReady2);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Panic("*** Coult not create kernel token for sysproc ready\n");
    }
    KernObj_ReleaseRef(&refGate);

    refPageArray1.AsAny = NULL;
    stat = KernPageArray_CreateSparse(1, K2OS_MAPTYPE_USER_DATA, &refPageArray1);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    virtAddr = KernVirt_Reserve(1);
    K2_ASSERT(0 != virtAddr);

    stat = KernVirtMap_Create(
        refPageArray1.AsPageArray,
        0,
        1,
        virtAddr,
        K2OS_MapType_Data_ReadWrite,
        &gData.SysProc.RefKernVirtMap1
    );
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    K2MEM_Zero((void *)virtAddr, K2_VA_MEMPAGE_BYTES);

    pSysProcPage = (K2OS_SYSPROC_PAGE *)gData.SysProc.RefKernVirtMap1.AsVirtMap->OwnerMapTreeNode.mUserVal;
    pSysProcPage->IxConsumer.mVal = K2OS_SYSPROC_NOTIFY_EMPTY_BIT;
    K2_CpuWriteBarrier();

    refPageArray4.AsAny = NULL;
    stat = KernPageArray_CreateSparse(4, K2OS_MAPTYPE_USER_READ, &refPageArray4);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    virtAddr = KernVirt_Reserve(4);
    K2_ASSERT(0 != virtAddr);

    stat = KernVirtMap_Create(
        refPageArray4.AsPageArray,
        0,
        4,
        virtAddr,
        K2OS_MapType_Data_ReadWrite,
        &gData.SysProc.RefKernVirtMap4
    );
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = KernProc_UserVirtHeapAlloc(apCurThread->User.ProcRef.AsProc, 1, &pUserHeapNode);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    virtAddr = K2HEAP_NodeAddr(&pUserHeapNode->HeapNode);
    pThreadPage->mSysCall_Arg7_Result0 = virtAddr;

    stat = KernVirtMap_CreateUser(
        pProc,
        refPageArray1.AsPageArray,
        0,
        virtAddr,
        1,
        K2OS_MapType_Data_ReadWrite,
        &gData.SysProc.RefUserVirtMap1
    );
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = KernProc_UserVirtHeapAlloc(apCurThread->User.ProcRef.AsProc, 4, &pUserHeapNode);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    virtAddr = K2HEAP_NodeAddr(&pUserHeapNode->HeapNode);
    pThreadPage->mSysCall_Arg6_Result1 = virtAddr;

    stat = KernVirtMap_CreateUser(
        pProc,
        refPageArray4.AsPageArray,
        0,
        virtAddr,
        4,
        K2OS_MapType_Data_ReadOnly,
        &gData.SysProc.RefUserVirtMap4
    );
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    KernObj_ReleaseRef(&refPageArray1);
    KernObj_ReleaseRef(&refPageArray4);

    stat = KernNotify_Create(FALSE, &gData.SysProc.RefNotify);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = KernProc_TokenCreate(pProc, gData.SysProc.RefNotify.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    pThreadPage->mSysCall_Arg5_Result2 = (UINT32)stat;

    pSchedItem = &apCurThread->SchedItem;
    pSchedItem->mType = KernSchedItem_Thread_SysCall;
    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
    KernSched_QueueItem(pSchedItem);
}

K2STAT
KernSysProc_Notify(
    BOOL            aFromKernelThread,
    K2OS_MSG const *apMsg,
    BOOL *          apRetDoSignal
)
{
    K2OS_SYSPROC_PAGE * pSysProcPage;
    UINT32              ixProd;
    UINT32              ixCons;
    UINT32              ixWord;
    UINT32              ixBit;
    UINT32              bitsVal;
    K2OS_MSG *          pMsgBuf;

    //
    // lockless, can call this from anywhere at any time on any core/thread
    //
    K2_ASSERT(NULL != apRetDoSignal);
    *apRetDoSignal = FALSE;

    if (NULL == gData.SysProc.RefNotify.AsAny)
    {
        // not set up yet
        if (!aFromKernelThread)
        {
//            K2OSKERN_Debug("!!! SysProc notify before it is ready (%d.%d ignored)\n", apMsg->mType, apMsg->mShort);
            return K2STAT_ERROR_API_ORDER;
        }
        do {
            K2OS_Thread_Sleep(10);
            K2_CpuReadBarrier();
        } while (NULL == (void * volatile)gData.SysProc.RefNotify.AsAny);
    }

    pSysProcPage = (K2OS_SYSPROC_PAGE *)gData.SysProc.RefKernVirtMap1.AsVirtMap->OwnerMapTreeNode.mUserVal;

    do {
        ixProd = pSysProcPage->IxProducer.mVal;
        K2_CpuReadBarrier();
        ixWord = ixProd >> 5;
        ixBit = ixProd & 0x1F;
        bitsVal = pSysProcPage->OwnerMask[ixWord].mVal;
        K2_CpuReadBarrier();
        if (bitsVal & (1 << ixBit))
        {
            // full
            return K2STAT_ERROR_FULL;
        }
    } while (ixProd != K2ATOMIC_CompareExchange(&pSysProcPage->IxProducer.mVal, (ixProd + 1) & K2OS_SYSPROC_NOTIFY_MSG_IX_MASK, ixProd));

    pMsgBuf = (K2OS_MSG *)gData.SysProc.RefKernVirtMap4.AsVirtMap->OwnerMapTreeNode.mUserVal;

    K2MEM_Copy(&pMsgBuf[ixProd], apMsg, sizeof(K2OS_MSG));
    K2_CpuWriteBarrier();

    K2ATOMIC_Or(&pSysProcPage->OwnerMask[ixWord].mVal, (1 << ixBit));

    // if sleeping bit is set try to clear it
    do {
        ixCons = pSysProcPage->IxConsumer.mVal;
        K2_CpuReadBarrier();
        if (0 == (ixCons & K2OS_SYSPROC_NOTIFY_EMPTY_BIT))
        {
            //
            // it was already clear or somebody else cleared it
            // so do not set the notify
            //
            return K2STAT_NO_ERROR;
        }
    } while (ixCons != K2ATOMIC_CompareExchange(&pSysProcPage->IxConsumer.mVal, ixCons & ~K2OS_SYSPROC_NOTIFY_EMPTY_BIT, ixCons));

    //
    // we cleared the sleeping bit, so send the notify
    //
    *apRetDoSignal = TRUE;
    K2_CpuWriteBarrier();

    return K2STAT_NO_ERROR;
}

void 
K2OSKERN_WaitSysProcReady(
    void
)
{
    K2OS_WaitResult waitResult;

    if ((!K2OS_Thread_WaitOne(&waitResult, gData.SysProc.mTokReady1, K2OS_TIMEOUT_INFINITE)) ||
        (waitResult != K2OS_Wait_Signalled_0))
    {
        K2OSKERN_Panic("*** Wait for sysproc ready1 failed\n");
    }

    if ((!K2OS_Thread_WaitOne(&waitResult, gData.SysProc.mTokReady2, K2OS_TIMEOUT_INFINITE)) ||
        (waitResult != K2OS_Wait_Signalled_0))
    {
        K2OSKERN_Panic("*** Wait for sysproc ready2 failed\n");
    }
}
