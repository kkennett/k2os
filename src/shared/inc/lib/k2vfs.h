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
#ifndef __K2VFS_H
#define __K2VFS_H

#include <k2systype.h>
#include <lib/k2mem.h>
#include <lib/k2tree.h>
#include <lib/k2list.h>
#include <lib/k2asc.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

#define K2VFS_MAX_COMP_LEN      255

typedef struct _K2VFS_NODE      K2VFS_NODE;
typedef struct _K2VFS_BRANCH    K2VFS_BRANCH;
typedef struct _K2VFS_USAGE     K2VFS_USAGE;
typedef struct _K2VFS_TREE      K2VFS_TREE;

struct _K2VFS_USAGE
{
    K2VFS_NODE *    mpNode;
    K2LIST_LINK     NodeUsageListLink;
};

struct _K2VFS_NODE
{
    BOOL            mIsBranch;
    K2VFS_BRANCH *  mpParentBranch;
    K2TREE_NODE     ParentBranchChildTreeNode;

    char *          mpNameZ;
    UINT32          mNameLen;

    K2LIST_ANCHOR   UsageList;
};

struct _K2VFS_BRANCH
{
    K2VFS_NODE      Node;

    K2TREE_ANCHOR   ChildBranchTree;
    K2TREE_ANCHOR   ChildLeafTree;
};

typedef enum _K2VFS_NodeNotifyType K2VFS_NodeNotifyType;
enum _K2VFS_NodeNotifyType
{
    K2VFS_NodeNotify_Invalid = 0,

    K2VFS_NodeNotify_AfterCreateSub,
    K2VFS_NodeNotify_AfterCreateLeaf,
    K2VFS_NodeNotify_AtDelete,

    K2VFS_NodeNotifyType_Count
};

typedef K2VFS_NODE * (*K2VFS_pf_NodeAlloc)(K2VFS_TREE *apTree, BOOL aIsBranch, UINT32 aNameLen, char **appNameStorage);
typedef void         (*K2VFS_pf_NodeFree)(K2VFS_NODE *apNode);
typedef void         (*K2VFS_pf_NodeNotify)(K2VFS_NODE *apNode, K2VFS_NodeNotifyType aType, UINT32 aReserved);

struct _K2VFS_TREE
{
    K2VFS_BRANCH        Root;
    K2VFS_pf_NodeAlloc  mfNodeAlloc;
    K2VFS_pf_NodeFree   mfNodeFree;
    K2VFS_pf_NodeNotify mfNodeNotify;
};

void            K2VFS_Create(K2VFS_TREE *apTree, K2VFS_pf_NodeAlloc afNodeAlloc, K2VFS_pf_NodeFree afNodeFree, K2VFS_pf_NodeNotify afNodeNotify);

K2VFS_TREE *    K2VFS_GetTree(K2VFS_NODE *apNode);

void            K2VFS_AddUsage(K2VFS_NODE *apNode, K2VFS_USAGE *apUsage);
void            K2VFS_DelUsage(K2VFS_USAGE *apUsage);

K2STAT          K2VFS_Use(K2VFS_NODE *apNode, char const *apSub, BOOL aSubIsBranch, K2VFS_USAGE *apUsage, BOOL aCreateIfNotExist);

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif // __K2NET_H
