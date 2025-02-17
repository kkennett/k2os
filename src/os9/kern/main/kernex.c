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
KernEx_Assert(
    char const *    apFile,
    int             aLineNum,
    char const *    apCondition
    )
{
    K2OSKERN_Panic("***KERNEL ASSERT:\nLocation: %s(%d)\nFALSE ==> %s\n", apFile, aLineNum, apCondition);
    while (1);
}

void 
K2_CALLCONV_REGS    
KernEx_RaiseException(
    K2STAT aExceptionCode
)
{
    K2OSKERN_Panic("***Kernel RaiseException: %08X\n", aExceptionCode);
}

K2STAT 
K2_CALLCONV_REGS
KernEx_TrapDismount(
    K2_EXCEPTION_TRAP *apTrap
)
{
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThisThread;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (KernThread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    K2_ASSERT(((UINT32)apTrap) == pThreadPage->mTrapStackTop);
    pThreadPage->mTrapStackTop = (UINT32)apTrap->mpNextTrap;

    return apTrap->mTrapResult;
}

void
KernEx_DumpThreadList(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2LIST_ANCHOR *             apList,
    BOOL                        aActiveThreads
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2LIST_LINK *           pListLink;

    pListLink = apList->mpHead;
    if (NULL == pListLink)
    {
        K2OSKERN_Debug("    NONE\n");
        return;
    }
    do
    {
        pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListLink, OwnerThreadListLink);
        pListLink = pListLink->mpNext;
        K2OSKERN_Debug("    %4d STATE %d LAST_SYSCALL %d\n", pThread->mGlobalIx, pThread->mState, pThread->User.mSysCall_Id);
        if (aActiveThreads)
        {
            KernArch_DumpThreadContext(apThisCore, pThread);
        }
    } while (NULL != pListLink);
}

void                     
KernEx_PanicDump(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
#if 0
    K2OSKERN_CPUCORE volatile * pCore;
    K2OSKERN_OBJ_PROCESS *      pProc;
    K2OSKERN_OBJ_THREAD *       pThread;
    K2LIST_LINK *               pOuter;
    UINT32                      ix;
    K2OSKERN_OBJREF             holdRef;
    K2TREE_NODE *               pTreeNode;
    K2HEAP_NODE *               pHeapNode;
    char const *                pNodeType;
    K2RAMHEAP_CHUNK *           pRamHeapChunk;
    K2RAMHEAP_NODE *            pRamHeapNode;

    K2OSKERN_Debug("Panic Dump:\n");

    for (ix = 0; ix < gData.mCpuCoreCount; ix++)
    {
        pCore = K2OSKERN_COREIX_TO_CPUCORE(ix);
        pProc = pCore->MappedProcRef.AsProc;
        pThread = pCore->mpActiveThread;
        K2OSKERN_Debug("%cCore %d: Proc %3d, Thread %4d\n",
            (ix == apThisCore->mCoreIx) ? '*' : ' ',
            ix,
            (pProc == NULL) ? 0 : pProc->mId,
            (pThread == NULL) ? 0 : pThread->mGlobalIx);
    }
    
    holdRef.AsAny = NULL;
    if (NULL != apThisCore->MappedProcRef.AsAny)
    {
        KernObj_CreateRef(&holdRef, apThisCore->MappedProcRef.AsAny);
    }

    pOuter = gData.Proc.List.mpHead;
    if (pOuter == NULL)
    {
        K2OSKERN_Debug("NO PROCESSES EXIST\n");
    }
    else
    {
        K2OSKERN_Debug("%d PROCESSES\n", gData.Proc.List.mNodeCount);
        do
        {
            pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, pOuter, GlobalProcListLink);
            pOuter = pOuter->mpNext;

            if (pProc != apThisCore->MappedProcRef.AsProc)
            {
                KernArch_SetCoreToProcess(apThisCore, pProc);
            }

            K2OSKERN_Debug("PROCESS %d:\n", pProc->mId);

            //
            // dump proc details
            //
            K2OSKERN_Debug("  STATE %d\n", pProc->mState);
            // moar

            K2OSKERN_Debug("  %d THREADS\n",
                pProc->Thread.Locked.CreateList.mNodeCount +
                pProc->Thread.SchedLocked.ActiveList.mNodeCount +
                pProc->Thread.Locked.DeadList.mNodeCount
            );
            K2OSKERN_Debug("  CREATE (%d):\n", pProc->Thread.Locked.CreateList.mNodeCount);
            if (0 != pProc->Thread.Locked.CreateList.mNodeCount)
            {
                KernEx_DumpThreadList(apThisCore, &pProc->Thread.Locked.CreateList, FALSE);
            }
            K2OSKERN_Debug("  ACTIVE (%d):\n", pProc->Thread.SchedLocked.ActiveList.mNodeCount);
            if (0 != pProc->Thread.SchedLocked.ActiveList.mNodeCount)
            {
                KernEx_DumpThreadList(apThisCore, &pProc->Thread.SchedLocked.ActiveList, TRUE);
            }
            K2OSKERN_Debug("  DEAD (%d):\n", pProc->Thread.Locked.DeadList.mNodeCount);
            if (0 != pProc->Thread.Locked.DeadList.mNodeCount)
            {
                KernEx_DumpThreadList(apThisCore, &pProc->Thread.Locked.DeadList, FALSE);
            }
        } while (NULL != pOuter);
    } 

    //
    // dump kernel virtual space
    //
    K2OSKERN_Debug("KERNEL VIRTUAL SPACE:\n");
    K2OSKERN_Debug("  BotPT = %08X\n", gData.Virt.mBotPt);
    K2OSKERN_Debug("  TopPT = %08X\n", gData.Virt.mTopPt);
    pTreeNode = K2TREE_FirstNode(&gData.Virt.Heap.AddrTree);
    if (NULL == pTreeNode)
    {
        K2OSKERN_Debug("  VIRTUAL HEAP EMPTY\n");
    }
    else
    {
        ix = K2OS_KVA_KERN_BASE;
        do
        {
            pHeapNode = K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, AddrTreeNode);
            if (pHeapNode->AddrTreeNode.mUserVal != ix)
            {
                K2OSKERN_Debug("  %08X-%08X %8d ----\n",
                    ix,
                    pHeapNode->AddrTreeNode.mUserVal - 1,
                    (pHeapNode->AddrTreeNode.mUserVal - ix) / K2_VA_MEMPAGE_BYTES
                );
            }
            ix = pHeapNode->AddrTreeNode.mUserVal + pHeapNode->SizeTreeNode.mUserVal;
            if (K2HEAP_NodeIsFree(pHeapNode))
                pNodeType = "FREE";
            else
                pNodeType = "USED";
            K2OSKERN_Debug("  %08X-%08X %8d %s\n",
                pHeapNode->AddrTreeNode.mUserVal,
                ix - 1,
                pHeapNode->SizeTreeNode.mUserVal / K2_VA_MEMPAGE_BYTES,
                pNodeType);
            pTreeNode = K2TREE_NextNode(&gData.Virt.Heap.AddrTree, pTreeNode);
        } while (NULL != pTreeNode);
        if (ix != 0)
        {
            K2OSKERN_Debug("  %08X-FFFFFFFF %8d ----\n",
                ix,
                ((~ix)+1) / K2_VA_MEMPAGE_BYTES
            );
        }
    }

#if 0
    //
    // dump kernel heap
    //
    pOuter = gData.Virt.RamHeap.AddrList.mpHead;
    if (NULL == pOuter)
    {
        K2OSKERN_Debug("RAM HEAP EMPTY\n");
    }
    else
    {
        K2OSKERN_Debug("RAM HEAP:\n");
        K2OSKERN_Debug("  CHUNKS:\n");
        pOuter = gData.Virt.RamHeap.ChunkList.mpHead;
        do
        {
            pRamHeapChunk = K2_GET_CONTAINER(K2RAMHEAP_CHUNK, pOuter, ChunkListLink);
            pOuter = pOuter->mpNext;
            K2OSKERN_Debug("    %08X-%08X, BREAK %08X\n",
                pRamHeapChunk->mStart, 
                pRamHeapChunk->mTop - 1, 
                pRamHeapChunk->mBreak);
        } while (NULL != pOuter);

        pOuter = gData.Virt.RamHeap.AddrList.mpHead;
        K2OSKERN_Debug("  NODES:\n");
        do
        {
            pRamHeapNode = K2_GET_CONTAINER(K2RAMHEAP_NODE, pOuter, AddrListLink);
            pOuter = pOuter->mpNext;
            K2OSKERN_Debug("    %08X %08X(%8d) %s\n",
                ((UINT32)pRamHeapNode) + sizeof(K2RAMHEAP_NODE),
                pRamHeapNode->TreeNode.mUserVal, pRamHeapNode->TreeNode.mUserVal,
                (pRamHeapNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_FREE) ? "FREE" : "USED");
        } while (NULL != pOuter);
    }
#endif
#endif
    KTRACE_DUMP;
}
