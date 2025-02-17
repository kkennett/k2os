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

#include <lib/k2vfs.h>

static 
int
K2VFS_TreeCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
)
{
    char            c, v;
    char const *    pChk;
    char const *    pStr;
    K2VFS_NODE *    pVfsNode;

    pChk = (char const *)aKey;
    pVfsNode = K2_GET_CONTAINER(K2VFS_NODE, apNode, ParentBranchChildTreeNode);
    pStr = pVfsNode->mpNameZ;

    do {
        c = *pStr;
        v = *pChk;

        if (0 == c)
        {
            if ((v == 0) ||
                (v == '/') ||
                (v == '\\'))
                break;

            return 1;
        }

        if (c < v)
        {
            return 1;
        }

        if (v < c)
        {
            return -1;
        }

        pStr++;
        pChk++;

    } while (1);

    return 0;
}

void
K2VFS_NodeInit(
    K2VFS_NODE *    apNode,
    BOOL            aIsBranch
)
{
    K2MEM_Zero(apNode, sizeof(K2VFS_NODE));
    K2LIST_Init(&apNode->UsageList);
    if (aIsBranch)
    {
        apNode->mIsBranch = TRUE;
        K2TREE_Init(&((K2VFS_BRANCH *)apNode)->ChildBranchTree, K2VFS_TreeCompare);
        K2TREE_Init(&((K2VFS_BRANCH *)apNode)->ChildLeafTree, K2VFS_TreeCompare);
    }
}

void
K2VFS_Create(
    K2VFS_TREE *        apTree,
    K2VFS_pf_NodeAlloc  afNodeAlloc,
    K2VFS_pf_NodeFree   afNodeFree,
    K2VFS_pf_NodeNotify afNodeNotify
)
{
    K2_ASSERT(NULL != apTree);
    K2_ASSERT(NULL != afNodeAlloc);
    K2_ASSERT(NULL != afNodeFree);
    K2_ASSERT(NULL != afNodeNotify);

    K2MEM_Zero(apTree, sizeof(K2VFS_TREE));
    apTree->mfNodeAlloc = afNodeAlloc;
    apTree->mfNodeFree = afNodeFree;
    apTree->mfNodeNotify = afNodeNotify;

    K2VFS_NodeInit(&apTree->Root.Node, TRUE);
    apTree->Root.Node.mpNameZ = "/";
    apTree->Root.Node.mNameLen = 1;
}

void
K2VFS_AddUsage(
    K2VFS_NODE *    apNode,
    K2VFS_USAGE *   apUsage
)
{
    K2_ASSERT(NULL != apNode);
    K2_ASSERT(NULL != apUsage);
    apUsage->mpNode = apNode;
    K2LIST_AddAtTail(&apNode->UsageList, &apUsage->NodeUsageListLink);
}

K2VFS_TREE *
K2VFS_GetTree(
    K2VFS_NODE *apNode
)
{
    K2VFS_BRANCH *  pParent;
    K2VFS_BRANCH *  pFind;
    K2VFS_BRANCH *  pChk;

    K2_ASSERT(NULL != apNode);

    pParent = apNode->mpParentBranch;
    if (NULL == pParent)
    {
        return K2_GET_CONTAINER(K2VFS_TREE, apNode, Root);
    }

    pFind = pParent;
    do {
        pChk = pFind->Node.mpParentBranch;
        if (NULL == pChk)
            break;
        pFind = pChk;
    } while (1);

    return K2_GET_CONTAINER(K2VFS_TREE, pFind, Root);;
}

void
K2VFS_DelNode(
    K2VFS_TREE * apTree,
    K2VFS_NODE * apNode
)
{
    K2VFS_BRANCH *  pParent;

    apTree->mfNodeNotify(apNode, K2VFS_NodeNotify_AtDelete, 0);

    pParent = apNode->mpParentBranch;
    K2_ASSERT(NULL != pParent);
    K2_ASSERT(0 == apNode->UsageList.mNodeCount);
    if (apNode->mIsBranch)
    {
        K2_ASSERT(0 == ((K2VFS_BRANCH *)apNode)->ChildBranchTree.mNodeCount);
        K2_ASSERT(0 == ((K2VFS_BRANCH *)apNode)->ChildLeafTree.mNodeCount);
        K2TREE_Remove(&pParent->ChildBranchTree, &apNode->ParentBranchChildTreeNode);
    }
    else
    {
        K2TREE_Remove(&pParent->ChildLeafTree, &apNode->ParentBranchChildTreeNode);
    }

    apTree->mfNodeFree(apNode);

    if ((0 != pParent->ChildBranchTree.mNodeCount) ||
        (0 != pParent->ChildLeafTree.mNodeCount) ||
        (0 != pParent->Node.UsageList.mNodeCount) ||
        (pParent == &apTree->Root))
    {
        return;
    }

    K2VFS_DelNode(apTree, &pParent->Node);
}

void
K2VFS_DelUsage(
    K2VFS_USAGE *   apUsage
)
{
    K2VFS_NODE * pNode;

    K2_ASSERT(NULL != apUsage);

    pNode = apUsage->mpNode;
    K2_ASSERT(NULL != pNode);

    K2LIST_Remove(&pNode->UsageList, &apUsage->NodeUsageListLink);
    apUsage->mpNode = NULL;

    if (0 != pNode->UsageList.mNodeCount)
    {
        return;
    }

    //
    // usage list node count hit zero
    //

    if (pNode->mIsBranch)
    {
        if (0 != ((K2VFS_BRANCH *)pNode)->ChildBranchTree.mNodeCount)
        {
            return;
        }

        if (0 != ((K2VFS_BRANCH *)pNode)->ChildLeafTree.mNodeCount)
        {
            return;
        }
    }

    //
    // if we get here we are going to delete this node
    // unless it is the root node
    //
    if (NULL == pNode->mpParentBranch)
    {
        return;
    }

    K2VFS_DelNode(K2VFS_GetTree(pNode), pNode);
}

K2VFS_NODE *
K2VFS_CreateSub(
    K2VFS_TREE *    apTree,
    K2VFS_BRANCH *  apBranch,
    char const *    apNameZ,
    UINT32          aNameLen,
    BOOL            aIsBranch
)
{
    K2VFS_NODE *    pSubNode;
    char *          pNameStor;

    pNameStor = NULL;
    pSubNode = apTree->mfNodeAlloc(apTree, aIsBranch, aNameLen, &pNameStor);
    if (NULL != pSubNode)
    {
        K2_ASSERT(NULL != pNameStor);

        K2VFS_NodeInit(pSubNode, aIsBranch);
        pSubNode->mpNameZ = pNameStor;
        K2MEM_Copy(pNameStor, apNameZ, aNameLen);
        pNameStor[aNameLen] = 0;
        pSubNode->mNameLen = aNameLen;

        pSubNode->mpParentBranch = (K2VFS_BRANCH *)apBranch;
        if (aIsBranch)
        {
            K2TREE_Insert(&apBranch->ChildBranchTree, (UINT_PTR)pSubNode->mpNameZ, &pSubNode->ParentBranchChildTreeNode);
        }
        else
        {
            K2TREE_Insert(&apBranch->ChildLeafTree, (UINT_PTR)pSubNode->mpNameZ, &pSubNode->ParentBranchChildTreeNode);
        }
    }

    return pSubNode;
}

K2STAT 
K2VFS_Get(
    K2VFS_TREE *    apTree,
    K2VFS_BRANCH *  apBranch,
    char const *    apSub,
    BOOL            aSubIsBranch,
    K2VFS_USAGE *   apUsage,
    BOOL            aCreateIfNotExist
)
{
    char const *    pEnd;
    char            c;
    UINT32          compLen;
    K2VFS_NODE *    pSubNode;
    K2VFS_USAGE     buildUse;
    K2STAT          stat;
    K2TREE_NODE *   pTreeNode;
    BOOL            doCreateCallback;

    pEnd = apSub;
    do {
        c = *pEnd;
        if ((0 == c) || ('/' == c) || ('\\' == c))
            break;
        if ((c == '*') ||
            (c == '?') ||
            (c <= ' ') ||
            (c == '\'') ||
            (c == '\"') ||
            (c == '\t'))
        {
            return K2STAT_ERROR_BAD_ARGUMENT;
        }
        pEnd++;
    } while (1);

    compLen = (UINT32)(pEnd - apSub);
    if ((0 == compLen) ||
        (K2VFS_MAX_COMP_LEN < compLen))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (c != 0)
    {
        // stopped on a slash or backslash
        // so this component must be a branch
        pEnd++;

        pTreeNode = K2TREE_Find(&apBranch->ChildBranchTree, (UINT_PTR)apSub);
        if (NULL != pTreeNode)
        {
            pSubNode = K2_GET_CONTAINER(K2VFS_NODE, pTreeNode, ParentBranchChildTreeNode);

            doCreateCallback = FALSE;
        }
        else
        {
            if (!aCreateIfNotExist)
            {
                return K2STAT_ERROR_NOT_EXIST;
            }

            pSubNode = K2VFS_CreateSub(apTree, apBranch, apSub, compLen, TRUE);
            if (NULL == pSubNode)
            {
                return K2STAT_ERROR_OUT_OF_MEMORY;
            }

            doCreateCallback = TRUE;
        }

        K2VFS_AddUsage(pSubNode, &buildUse);

        if (doCreateCallback)
        {
            apTree->mfNodeNotify(pSubNode, K2VFS_NodeNotify_AfterCreateSub, 0);
        }

        stat = K2VFS_Get(apTree, (K2VFS_BRANCH *)pSubNode, pEnd, aSubIsBranch, apUsage, aCreateIfNotExist);

        K2VFS_DelUsage(&buildUse);

        return stat;
    }

    //
    // stopped on a null char so this is the last component in the path
    //
    if (aSubIsBranch)
    {
        pTreeNode = K2TREE_Find(&apBranch->ChildBranchTree, (UINT_PTR)apSub);
    }
    else
    {
        pTreeNode = K2TREE_Find(&apBranch->ChildLeafTree, (UINT_PTR)apSub);
    }

    if (NULL != pTreeNode)
    {
        pSubNode = K2_GET_CONTAINER(K2VFS_NODE, pTreeNode, ParentBranchChildTreeNode);
    }
    else
    {
        pSubNode = NULL;
    }

    if (NULL != pTreeNode)
    {
        K2_ASSERT(NULL != pSubNode);

        doCreateCallback = FALSE;
    }
    else
    {
        if (!aCreateIfNotExist)
        {
            return K2STAT_ERROR_NOT_EXIST;
        }

        //
        // create it
        //
        pSubNode = K2VFS_CreateSub(apTree, apBranch, apSub, compLen, aSubIsBranch);
        if (NULL == pSubNode)
        {
            return K2STAT_ERROR_OUT_OF_MEMORY;
        }

        doCreateCallback = TRUE;
    }

    K2VFS_AddUsage(pSubNode, apUsage);

    if (doCreateCallback)
    {
        apTree->mfNodeNotify(pSubNode, K2VFS_NodeNotify_AfterCreateLeaf, 0);
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
K2VFS_Use(
    K2VFS_NODE *    apNode,
    char const *    apSub,
    BOOL            aSubIsBranch,
    K2VFS_USAGE *   apUsage,
    BOOL            aCreateIfNotExist
)
{
    K2VFS_TREE * pTree;

    K2_ASSERT(NULL != apNode);
    K2_ASSERT(NULL != apUsage);

    if ((NULL == apSub) ||
        (0 == *apSub))
    {
        // null or empty string = duplicate usage
        if ((aSubIsBranch && (!apNode->mIsBranch)) ||
            ((!aSubIsBranch) && apNode->mIsBranch))
            return K2STAT_ERROR_BAD_ARGUMENT;
        K2VFS_AddUsage(apNode, apUsage);
        return K2STAT_NO_ERROR;
    }

    if (!apNode->mIsBranch)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pTree = K2VFS_GetTree(apNode);

    if (((*apSub) == '/') ||
        ((*apSub) == '\\'))
    {
        // sub starts with slash means 'from root'
        apNode = &pTree->Root.Node;
        apSub++;
        if (0 == *apSub)
        {
            if (!aSubIsBranch)
            {
                return K2STAT_ERROR_BAD_ARGUMENT;
            }
            K2VFS_AddUsage(apNode, apUsage);
            return K2STAT_NO_ERROR;
        }
    }

    //
    // now we are searching/building from pNode for path at apSub
    //
    return K2VFS_Get(pTree, (K2VFS_BRANCH *)apNode, apSub, aSubIsBranch, apUsage, aCreateIfNotExist);
}

