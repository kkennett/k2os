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
KernBlockIo_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_BLOCKIO *      apBlockIo
)
{
    K2_ASSERT(0 != (apBlockIo->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    K2_ASSERT(NULL == apBlockIo->mpIoThread);

    K2_ASSERT(0 == apBlockIo->RangeList.mNodeCount);

    K2MEM_Zero(apBlockIo, sizeof(K2OSKERN_OBJ_BLOCKIO));

    KernHeap_Free(apBlockIo);
}

void
KernBlockIo_SysCall_Attach(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    BOOL                    disp;
    K2OSKERN_OBJREF         refBlockIo;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_OBJ_IFINST *   pIfInst;
    K2STAT                  stat;
    K2OS_TOKEN              tokResult;

    stat = K2STAT_ERROR_NOT_FOUND;

    refBlockIo.AsAny = NULL;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
    pTreeNode = K2TREE_Find(&gData.Iface.IdTree, apCurThread->User.mSysCall_Arg0);
    if (NULL != pTreeNode)
    {
        pIfInst = K2_GET_CONTAINER(K2OSKERN_OBJ_IFINST, pTreeNode, IdTreeNode);
        if (!pIfInst->mIsDeparting)
        {
            if ((NULL != pIfInst->RefChild.AsAny) &&
                (pIfInst->RefChild.AsAny->mObjType == KernObj_BlockIo))
            {
                KernObj_CreateRef(&refBlockIo, pIfInst->RefChild.AsAny);
                stat = K2STAT_NO_ERROR;
            }
        }
    }

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (!K2STAT_IS_ERROR(stat))
    {
        tokResult = NULL;

        stat = KernProc_TokenCreate(apCurThread->User.ProcRef.AsProc, refBlockIo.AsAny, &tokResult);

        if (!K2STAT_IS_ERROR(stat))
        {
            K2_ASSERT(NULL != tokResult);
            apCurThread->User.mSysCall_Result = (UINT32)tokResult;
        }

        KernObj_ReleaseRef(&refBlockIo);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}

void
KernBlockIo_SysCall_To_IoMgr(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF     refBlockIo;
    K2STAT              stat;
    UINT32              detached;
    K2OS_THREAD_PAGE *  pThreadPage;

    refBlockIo.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refBlockIo);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (refBlockIo.AsAny->mObjType != KernObj_BlockIo)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            detached = refBlockIo.AsBlockIo->mDetached;
            K2_CpuReadBarrier();
            if (detached)
            {
                stat = K2STAT_ERROR_NO_INTERFACE;
            }
            else
            {
                pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

                switch (apCurThread->User.mSysCall_Id)
                {
                case K2OS_SYSCALL_ID_BLOCKIO_RANGE_CREATE:
                    if (pThreadPage->mSysCall_Arg2 == 0)    // block count
                    {
                        stat = K2STAT_ERROR_BAD_ARGUMENT;   
                    }
                    break;

                case K2OS_SYSCALL_ID_BLOCKIO_RANGE_DELETE:
                    if (pThreadPage->mSysCall_Arg1 == 0)    // range
                    {
                        stat = K2STAT_ERROR_BAD_ARGUMENT;   
                    }
                    break;

                case K2OS_SYSCALL_ID_BLOCKIO_TRANSFER:
                    if ((pThreadPage->mSysCall_Arg1 == 0) ||    // range
                        // bytes offset
                        (pThreadPage->mSysCall_Arg3 == 0) ||    // bytes count
                        // read or write
                        ((pThreadPage->mSysCall_Arg5_Result2 < K2OS_UVA_LOW_BASE) ||    // buffer user virt addr
                         (pThreadPage->mSysCall_Arg5_Result2 >= K2OS_KVA_KERN_BASE)))
                    {
                        stat = K2STAT_ERROR_BAD_ARGUMENT;
                    }
                    break;

                case K2OS_SYSCALL_ID_BLOCKIO_ERASE:
                    if ((pThreadPage->mSysCall_Arg1 == 0) ||    // range
                        // bytes offset
                        (pThreadPage->mSysCall_Arg3 == 0))      // bytes count
                    {
                        stat = K2STAT_ERROR_BAD_ARGUMENT;
                    }
                    break;

                default:
                    stat = K2STAT_NO_ERROR;
                    break;
                }

                K2_ASSERT(NULL == apCurThread->ThreadIo.Iop.RefObj.AsAny);
                KernObj_CreateRef(&apCurThread->ThreadIo.Iop.RefObj, refBlockIo.AsAny);
                apCurThread->ThreadIo.Iop.mType = KernIop_UserThread_BlockIo;

                KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InIo);

                KernIoMgr_Queue(&apCurThread->ThreadIo.Iop);

                stat = K2STAT_NO_ERROR;
            }
        }

        KernObj_ReleaseRef(&refBlockIo);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}

