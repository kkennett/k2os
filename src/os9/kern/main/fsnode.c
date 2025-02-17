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

int
KernFsNode_CaseNameCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
)
{
    K2OSKERN_FSNODE *   pFsNode;
    char const *        pComp;
    char const *        pName;
    char                chComp;
    int                 i;

    pFsNode = K2_GET_CONTAINER(K2OSKERN_FSNODE, apNode, ParentLocked.ParentsChildTreeNode);
    pComp = ((char const *)aKey) - 1;
    pName = pFsNode->Static.mName;
    do
    {
        pComp++;
        chComp = *pComp;
        if ((chComp == '/') || (chComp == '\\'))
            chComp = 0;
        i = ((int)(*pName)) - ((int)chComp);
        if (i != 0)
            return i;
        pName++;
    } while (0 != chComp);

    return 0;
}

int
KernFsNode_CaseInsNameCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
)
{
    K2OSKERN_FSNODE *   pFsNode;
    char const *        pComp;
    char const *        pName;
    char                chName;
    char                chComp;
    int                 i;

    pFsNode = K2_GET_CONTAINER(K2OSKERN_FSNODE, apNode, ParentLocked.ParentsChildTreeNode);
    pComp = ((char const *)aKey) - 1;
    pName = pFsNode->Static.mName;
    do
    {
        pComp++;
        chComp = *pComp;
        if ((chComp == '/') || (chComp == '\\'))
            chComp = 0;
        chName = *pName;
        if ((chName >= 'a') && (chName <= 'z'))
            chName -= ('a' - 'A');
        if ((chComp >= 'a') && (chComp <= 'z'))
            chComp -= ('a' - 'A');
        i = ((int)chName) - ((int)chComp);
        if (i != 0)
            return i;
        pName++;
    } while (0 != chComp);

    return 0;
}

void
KernFsNode_Init(
    K2OSKERN_FILESYS *  apFileSys,
    K2OSKERN_FSNODE *   apFsNode
)
{
    K2MEM_Zero(apFsNode, sizeof(K2OSKERN_FSNODE));
    apFsNode->mRefCount = 1;
    K2OSKERN_SeqInit(&apFsNode->ChangeSeqLock);
    if ((NULL != apFileSys) && (apFileSys->Fs.mCaseSensitive))
    {
        K2TREE_Init(&apFsNode->Locked.ChildTree, KernFsNode_CaseNameCompare);
    }
    else
    {
        K2TREE_Init(&apFsNode->Locked.ChildTree, KernFsNode_CaseInsNameCompare);
    }
    apFsNode->Static.mpFileSys = apFileSys;
    apFsNode->Locked.mCurrentShare = K2OS_ACCESS_RW;
    apFsNode->Static.Ops.Kern.AddRef = KernFsNode_AddRef;
    apFsNode->Static.Ops.Kern.Release = KernFsNode_Release;
}

INT_PTR 
KernFsNode_AddRef(
    K2OSKERN_FSNODE *apFsNode
)
{
    INT_PTR result;

    result = K2ATOMIC_Inc(&apFsNode->mRefCount);
//    KernFsNode_DebugPrint(apFsNode);
//    K2OSKERN_Debug(" AddRef %d\n", result);
    return result;
}

INT_PTR 
KernFsNode_Release(
    K2OSKERN_FSNODE *apFsNode
)
{
    BOOL                disp;
    INT_PTR             result;
    K2OSKERN_FSNODE *   pParentDir;
    K2OSKERN_FILESYS *  pFileSys;

    pParentDir = apFsNode->Static.mpParentDir;

    if (NULL == pParentDir)
    {
        result = K2ATOMIC_Dec(&apFsNode->mRefCount);
        K2_ASSERT(0 != result);
//        KernFsNode_DebugPrint(apFsNode);
//        K2OSKERN_Debug(" Release %d\n", result);
    }
    else
    {
        disp = K2OSKERN_SeqLock(&pParentDir->ChangeSeqLock);

        result = K2ATOMIC_Dec(&apFsNode->mRefCount);

//        KernFsNode_DebugPrint(apFsNode);
//        K2OSKERN_Debug(" Release %d\n", result);

        if (0 == result)
        {
            K2_ASSERT(0 == apFsNode->Locked.ChildTree.mNodeCount);

            K2OSKERN_Debug("FreeClosedNode[");
            KernFsNode_DebugPrint(apFsNode);
            K2OSKERN_Debug("]\n");

            K2TREE_Remove(&pParentDir->Locked.ChildTree, &apFsNode->ParentLocked.ParentsChildTreeNode);
        }

        K2OSKERN_SeqUnlock(&pParentDir->ChangeSeqLock, disp);
    }

    if (0 != result)
        return result;

    pFileSys = apFsNode->Static.mpFileSys;

    if ((NULL == pFileSys) || 
        (apFsNode->Static.mpParentDir == &gData.FileSys.FsRootFsNode))
    {
        KernHeap_Free(apFsNode);

        if (apFsNode->Static.mpParentDir == &gData.FileSys.FsRootFsNode)
        {
            K2_ASSERT(NULL != pFileSys);
            if (NULL != pFileSys->Ops.Kern.FsShutdown)
            {
                pFileSys->Ops.Kern.FsShutdown(pFileSys);
            }
        }
    }
    else
    {
        if (NULL != pFileSys->Ops.Fs.FreeClosedNode)
        {
            pFileSys->Ops.Fs.FreeClosedNode(pFileSys, apFsNode);
        }
    }

    KernFsNode_Release(pParentDir);

    return 0;
}

void    
KernFsNode_DebugPrint(
    K2OSKERN_FSNODE *apFsNode
)
{
    if (NULL != apFsNode->Static.mpParentDir)
    {
        KernFsNode_DebugPrint(apFsNode->Static.mpParentDir);
        K2OSKERN_Debug("%s", apFsNode->Static.mName);
        if (apFsNode->Static.mIsDir)
        {
            K2OSKERN_Debug("/");
        }
        else
        {
            K2OSKERN_Debug("<file>");
        }
    }
    else
    {
        K2OSKERN_Debug("/");
    }
}

void 
iKernFsNodeLocked_Dump(
    K2OSKERN_FSNODE *   apFsNode,
    UINT32              aLevel
)
{
    UINT32              ix;
    K2TREE_NODE *       pTreeNode;
    K2OSKERN_FSNODE *   pChild;

    for (ix = 0; ix < aLevel; ix++)
    {
        K2OSKERN_Debug(" ");
    }
    K2OSKERN_Debug("%2d ", apFsNode->mRefCount);
    KernFsNode_DebugPrint(apFsNode);
    K2OSKERN_Debug("\n");
    if (apFsNode->Static.mIsDir)
    {
        pTreeNode = K2TREE_FirstNode(&apFsNode->Locked.ChildTree);
        if (NULL != pTreeNode)
        {
            do
            {
                pChild = K2_GET_CONTAINER(K2OSKERN_FSNODE, pTreeNode, ParentLocked.ParentsChildTreeNode);
                K2OSKERN_SeqLock(&pChild->ChangeSeqLock);
                iKernFsNodeLocked_Dump(pChild, aLevel + 2);
                K2OSKERN_SeqUnlock(&pChild->ChangeSeqLock, FALSE);
                pTreeNode = K2TREE_NextNode(&apFsNode->Locked.ChildTree, pTreeNode);
            } while (NULL != pTreeNode);
        }
    }
}

void    
KernFsNode_Dump(
    void
)
{
    BOOL disp;
    disp = K2OSKERN_SeqLock(&gData.FileSys.RootFsNode.ChangeSeqLock);
    K2OSKERN_Debug("\nKernFsNode_Dump()\n----------------------\n");
    iKernFsNodeLocked_Dump(&gData.FileSys.RootFsNode, 0);
    K2OSKERN_Debug("----------------------\n");
    K2OSKERN_SeqUnlock(&gData.FileSys.RootFsNode.ChangeSeqLock, disp);
}

