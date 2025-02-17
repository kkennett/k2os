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

#include "k2osexec.h"
#include <lib/k2rofshelp.h>

typedef struct _ROFSEXEC ROFSEXEC;
typedef struct _ROFSNODE ROFSNODE;

struct _ROFSNODE
{
    K2OSKERN_FSNODE     KernFsNode;
    union
    {
        K2ROFS_DIR const *  mpDir;
        K2ROFS_FILE const * mpFile;
    };
};

struct _ROFSEXEC
{
    K2OS_RPC_OBJ        mRpcObj;

    K2OS_RPC_IFINST     mRpcIfInst;
    K2OS_IFINST_ID      mIfInstId;

    K2OS_CRITSEC        Sec;

    K2ROFS const *      mpRofs;
    UINT32              mPageCount;

    K2OSKERN_FSPROV     KernFsProv;
    K2OSKERN_FILESYS *  mpFileSys;
    K2OSKERN_FSNODE *   mpRootFsNode;
};
static ROFSEXEC     sgExecRofs;
K2OSKERN_FSPROV *   gpRofsProv = NULL;

K2STAT
RofsNode_GetSizeBytes(
    K2OSKERN_FSNODE *   apFsNode,
    UINT64 *            apRetSizeBytes
)
{
    ROFSNODE * pRofsNode;

    pRofsNode = K2_GET_CONTAINER(ROFSNODE, apFsNode, KernFsNode);

    if (!pRofsNode->KernFsNode.Static.mIsDir)
    {
        *apRetSizeBytes = pRofsNode->mpFile->mSizeBytes;
    }
    else
    {
        *apRetSizeBytes = 0;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
RofsNode_GetTime(
    K2OSKERN_FSNODE *   apFsNode,
    UINT64 *            apRetTime
)
{
    ROFSNODE * pRofsNode;

    pRofsNode = K2_GET_CONTAINER(ROFSNODE, apFsNode, KernFsNode);

    if (!pRofsNode->KernFsNode.Static.mIsDir)
    {
        *apRetTime = pRofsNode->mpFile->mTime;
    }
    else
    {
        *apRetTime = 0;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
RofsNode_Read(
    K2OSKERN_FSNODE *   apFsNode,
    void *              apBuffer,
    UINT64 const *      apOffset,
    UINT32              aBytes,
    UINT32 *            apRetBytesRed
)
{
    ROFSNODE *          pRofsNode;
    UINT32              offset;
    K2ROFS_FILE const * pFile;

    pRofsNode = K2_GET_CONTAINER(ROFSNODE, apFsNode, KernFsNode);

#if 0
    K2OSKERN_Debug("ROFS - read file %08X %08X%08X %d\n",
        apBuffer,
        (UINT32)((*apOffset) >> 32), (UINT32)((*apOffset) & 0xFFFFFFFFull),
        aBytes);
#endif

    if (pRofsNode->KernFsNode.Static.mIsDir)
    {
        return K2STAT_ERROR_NOT_SUPPORTED;
    }

    if (0 == aBytes)
    {
        if (NULL != apRetBytesRed)
            *apRetBytesRed = 0;
        return K2STAT_NO_ERROR;
    }

    pFile = pRofsNode->mpFile;

    offset = (UINT32)(*apOffset);

    if ((offset >= pFile->mSizeBytes) ||
        ((pFile->mSizeBytes - offset) < aBytes))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2MEM_Copy(apBuffer, K2ROFS_FILEDATA(sgExecRofs.mpRofs, pFile) + offset, aBytes);

    if (NULL != apRetBytesRed)
    {
        *apRetBytesRed = aBytes;
    }

    return K2STAT_NO_ERROR;
}

typedef struct _ROFSLOCK ROFSLOCK;
struct _ROFSLOCK
{
    K2OSKERN_FSFILE_LOCK FsLock;
};

K2STAT
RofsNode_LockData(
    K2OSKERN_FSNODE *       apFsNode,
    UINT64 const *          apOffset,
    UINT32                  aByteCount,
    BOOL                    aWriteable,
    K2OSKERN_FSFILE_LOCK ** appRetFileLock
)
{
    ROFSNODE *          pRofsNode;
    UINT32              offset;
    UINT32              dataLeft;
    K2ROFS_FILE const * pFile;
    ROFSLOCK *          pLock;
    K2STAT              stat;

    if (aWriteable)
    {
        return K2STAT_ERROR_READ_ONLY;
    }

    K2_ASSERT(NULL != appRetFileLock);
    K2_ASSERT(NULL != apOffset);

    pRofsNode = K2_GET_CONTAINER(ROFSNODE, apFsNode, KernFsNode);

    if (pRofsNode->KernFsNode.Static.mIsDir)
    {
        return K2STAT_ERROR_NOT_SUPPORTED;
    }

    pFile = pRofsNode->mpFile;

    if (*apOffset >= ((UINT64)pFile->mSizeBytes))
    {
        return K2STAT_ERROR_END_OF_FILE;
    }

    pLock = (ROFSLOCK *)K2OS_Heap_Alloc(sizeof(ROFSLOCK));
    if (NULL == pLock)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pLock, sizeof(ROFSLOCK));

    pLock->FsLock.mpFsNode = apFsNode;

    if (0 == aByteCount)
    {
        pLock->FsLock.mpData = NULL;
        pLock->FsLock.mLockedByteCount = 0;
    }
    else
    {
        offset = (UINT32)(*apOffset & 0xFFFFFFFFull);

        dataLeft = pFile->mSizeBytes - offset;
        if (aByteCount > dataLeft)
            aByteCount = dataLeft;

        pLock->FsLock.mpData = (UINT8 *)(K2ROFS_FILEDATA(sgExecRofs.mpRofs, pFile) + offset);
        pLock->FsLock.mLockedByteCount = aByteCount;
    }

    *appRetFileLock = &pLock->FsLock;

    return K2STAT_NO_ERROR;
}

void    
RofsNode_UnlockData(
    K2OSKERN_FSFILE_LOCK *apLock
)
{
    K2OS_Heap_Free(apLock);
}

K2STAT
Rofs_AcquireChild(
    K2OSKERN_FILESYS *  apFileSys,
    K2OSKERN_FSNODE *   apFsNode,
    char const *        apChildName,
    K2OS_FileOpenType   aOpenType,
    UINT32              aAccess,
    UINT32              aNewFileAttrib,
    K2OSKERN_FSNODE **  appRetFsNode
)
{
    ROFSNODE *          pRofsNode;
    K2ROFS_DIR const *  pBaseDir;
    K2ROFS_DIR const *  pDir;
    K2ROFS_FILE const * pFile;
    K2STAT              stat;
    BOOL                disp;
    K2OSKERN_FSNODE *   pResult;
    K2TREE_NODE *       pTreeNode;

    stat = K2STAT_ERROR_UNKNOWN;

//    K2OSKERN_Debug("Rofs_AcquireChild(%s,%s)\n", apFsNode->Static.mName, apChildName);

    K2_ASSERT(apFileSys == sgExecRofs.mpFileSys);

    if (0 != (aAccess & K2OS_ACCESS_W))
    {
        return K2STAT_ERROR_READ_ONLY;
    }

    K2OS_CritSec_Enter(&sgExecRofs.Sec);

    do
    {
        if (apFsNode == sgExecRofs.mpRootFsNode)
        {
            pBaseDir = K2ROFS_ROOTDIR(sgExecRofs.mpRofs);
        }
        else
        {
            pRofsNode = K2_GET_CONTAINER(ROFSNODE, apFsNode, KernFsNode);
            if (!pRofsNode->KernFsNode.Static.mIsDir)
            {
                stat = K2STAT_ERROR_NO_INTERFACE;
                break;
            }
            pBaseDir = pRofsNode->mpDir;
        }

        pDir = K2ROFSHELP_SubDir(sgExecRofs.mpRofs, pBaseDir, apChildName);
        if (NULL == pDir)
        {
            pFile = K2ROFSHELP_SubFile(sgExecRofs.mpRofs, pBaseDir, apChildName);
            if (NULL == pFile)
            {
                stat = K2STAT_ERROR_NOT_FOUND;
                break;
            }
        }
        else
        {
            pFile = NULL;
        }

        pRofsNode = (ROFSNODE *)K2OS_Heap_Alloc(sizeof(ROFSNODE));
        if (NULL == pRofsNode)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }
        
        K2MEM_Zero(pRofsNode, sizeof(ROFSNODE));
        apFileSys->Ops.Kern.FsNodeInit(apFileSys, &pRofsNode->KernFsNode);
        K2ASC_CopyLen(pRofsNode->KernFsNode.Static.mName, apChildName, K2OS_FSITEM_MAX_COMPONENT_NAME_LENGTH - 1);
        pRofsNode->KernFsNode.Static.mName[K2OS_FSITEM_MAX_COMPONENT_NAME_LENGTH] = 0;
        if (NULL != pDir)
        {
            pRofsNode->mpDir = pDir;
            pRofsNode->KernFsNode.Static.mIsDir = TRUE;
            pRofsNode->KernFsNode.Locked.mFsAttrib = K2_FSATTRIB_DIR;
        }
        else
        {
            // files must start on a page align boundary
            K2_ASSERT(0 == ((pFile->mStartSectorOffset * K2ROFS_SECTOR_BYTES) & K2_VA_MEMPAGE_OFFSET_MASK));
            pRofsNode->mpFile = pFile;
            pRofsNode->KernFsNode.Static.Ops.Fs.GetSizeBytes = RofsNode_GetSizeBytes;
            pRofsNode->KernFsNode.Static.Ops.Fs.GetTime = RofsNode_GetTime;
            pRofsNode->KernFsNode.Static.Ops.Fs.LockData = RofsNode_LockData;
            pRofsNode->KernFsNode.Static.Ops.Fs.UnlockData = RofsNode_UnlockData;
        }

        disp = K2OSKERN_SeqLock(&apFsNode->ChangeSeqLock);

        pTreeNode = K2TREE_Find(&apFsNode->Locked.ChildTree, (UINT_PTR)apChildName);
        if (NULL == pTreeNode)
        {
            pRofsNode->KernFsNode.Static.mpParentDir = apFsNode;
            apFsNode->Static.Ops.Kern.AddRef(apFsNode);
            K2TREE_Insert(&apFsNode->Locked.ChildTree, (UINT_PTR)apChildName, &pRofsNode->KernFsNode.ParentLocked.ParentsChildTreeNode);
        }
        else
        {
            pResult = K2_GET_CONTAINER(K2OSKERN_FSNODE, pTreeNode, ParentLocked.ParentsChildTreeNode);
            K2_ASSERT(pResult->Static.mpParentDir == apFsNode);
            K2ATOMIC_Inc(&pResult->mRefCount);
        }

        K2OSKERN_SeqUnlock(&apFsNode->ChangeSeqLock, disp);

        if (NULL != pTreeNode)
        {
            // was already found when we went to add it
            K2OS_Heap_Free(pRofsNode);
            *appRetFsNode = pResult;
        }
        else
        {
            *appRetFsNode = &pRofsNode->KernFsNode;
        }

        stat = K2STAT_NO_ERROR;

    } while (0);

    K2OS_CritSec_Leave(&sgExecRofs.Sec);

    return stat;
}

void    
Rofs_FreeClosedNode(
    K2OSKERN_FILESYS *  apFileSys,
    K2OSKERN_FSNODE *   apFsNode
)
{
    ROFSNODE * pRofsNode;

    K2_ASSERT(apFileSys == sgExecRofs.mpFileSys);

    // 
    // node has already been removed from parent
    //

    pRofsNode = K2_GET_CONTAINER(ROFSNODE, apFsNode, KernFsNode);
    K2OS_Heap_Free(pRofsNode);
}

K2STAT
Rofs_RpcObj_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    if ((apCreate->mCreatorProcessId != 0) ||
        (apCreate->mCreatorContext != (UINT32)&sgExecRofs))
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    *apRetContext = 0;

    sgExecRofs.mRpcObj = aObject;

    return K2STAT_NO_ERROR;
}

K2STAT
Rofs_RpcObj_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    if (0 != aProcessId)
        return K2STAT_ERROR_NOT_ALLOWED;
    return K2STAT_NO_ERROR;
}

K2STAT
Rofs_RpcObj_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    return K2STAT_NO_ERROR;
}

K2STAT
Rofs_RpcObj_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    K2STAT  stat;
    UINT32  u32;

    stat = K2STAT_ERROR_UNKNOWN;

    switch (apCall->Args.mMethodId)
    {
    case K2OS_FsProv_Method_GetCount:
        if ((apCall->Args.mInBufByteCount > 0) ||
            (apCall->Args.mOutBufByteCount < sizeof(UINT32)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            u32 = 1;
            K2MEM_Copy(apCall->Args.mpOutBuf, &u32, sizeof(u32));
            *apRetUsedOutBytes = sizeof(UINT32);
            stat = K2STAT_NO_ERROR;
        }
        break;

    case K2OS_FsProv_Method_GetEntry:
        if ((apCall->Args.mInBufByteCount < sizeof(UINT32)) ||
            (apCall->Args.mOutBufByteCount < sizeof(void *)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            K2MEM_Copy(&u32, apCall->Args.mpInBuf, sizeof(UINT32));
            if (u32 > 0)
            {
                stat = K2STAT_ERROR_OUT_OF_BOUNDS;
            }
            else
            {
                u32 = (UINT32)&sgExecRofs.KernFsProv;
                K2MEM_Copy(apCall->Args.mpOutBuf, &u32, sizeof(void *));
                *apRetUsedOutBytes = sizeof(void *);
                stat = K2STAT_NO_ERROR;
            }
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    };

    return stat;
}

K2STAT
Rofs_RpcObj_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    K2OSKERN_Panic("Rofs obj delete - should be impossible.\n");
    return K2STAT_ERROR_UNKNOWN;
}

K2STAT
Rofs_Probe(
    K2OSKERN_FSPROV *   apProv,
    void *              apContext,
    BOOL *              apRetMatched
)
{
    if ((apProv != &sgExecRofs.KernFsProv) ||
        (apContext != &sgExecRofs.KernFsProv))
        return K2STAT_ERROR_NOT_FOUND;

    *apRetMatched = TRUE;

    return K2STAT_NO_ERROR;
}

K2STAT
Rofs_Attach(
    K2OSKERN_FSPROV *   apProv,
    K2OSKERN_FILESYS *  apFileSys,
    K2OSKERN_FSNODE *   apRootFsNode
)
{
    if ((apProv != &sgExecRofs.KernFsProv) ||
        (apFileSys->Kern.mpAttachContext != &sgExecRofs.KernFsProv))
        return K2STAT_ERROR_BAD_ARGUMENT;

    sgExecRofs.mpFileSys = apFileSys;
    apFileSys->Ops.Fs.AcquireChild = Rofs_AcquireChild;
    apFileSys->Ops.Fs.FreeClosedNode = Rofs_FreeClosedNode;
    apFileSys->Fs.mReadOnly = TRUE;
    apFileSys->Fs.mCaseSensitive = FALSE;
    apFileSys->Fs.mProvInstanceContext = (UINT32)&sgExecRofs;
    apFileSys->Fs.mDoNotUseForPaging = TRUE;

    sgExecRofs.mpRootFsNode = apRootFsNode;

    return K2STAT_NO_ERROR;
}

void
Rofs_Init(
    K2ROFS const *apRofs
)
{
    // {411A183D-7D95-4126-867C-8B0F1A918927}
    static const K2OS_RPC_OBJ_CLASSDEF sExecRofsClassDef =
    {
        { 0x411a183d, 0x7d95, 0x4126, { 0x86, 0x7c, 0x8b, 0xf, 0x1a, 0x91, 0x89, 0x27 } },
        Rofs_RpcObj_Create,
        Rofs_RpcObj_OnAttach,
        Rofs_RpcObj_OnDetach,
        Rofs_RpcObj_Call,
        Rofs_RpcObj_Delete
    };
    static const K2_GUID128 sFsProvIfaceId = K2OS_IFACE_FSPROV;

    K2OS_RPC_CLASS      rofsClass;
    K2OS_RPC_OBJ_HANDLE hRpcObj;

    K2MEM_Zero(&sgExecRofs, sizeof(sgExecRofs));
    if (!K2OS_CritSec_Init(&sgExecRofs.Sec))
    {
        K2OSKERN_Panic("ROFS: Critsec init failed\n");
    }
    K2ASC_Copy(sgExecRofs.KernFsProv.mName, "builtin");
    sgExecRofs.KernFsProv.mFlags = 0;
    sgExecRofs.KernFsProv.Ops.Probe = Rofs_Probe;
    sgExecRofs.KernFsProv.Ops.Attach = Rofs_Attach;
    sgExecRofs.mpRofs = apRofs;
    sgExecRofs.mPageCount = ((sgExecRofs.mpRofs->mSectorCount * K2ROFS_SECTOR_BYTES) + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;
    gpRofsProv = &sgExecRofs.KernFsProv;

    rofsClass = K2OS_RpcServer_Register(&sExecRofsClassDef, 0);
    if (NULL == rofsClass)
    {
        K2OSKERN_Panic("ROFS: Could not register rofs object class\n");
    }

    hRpcObj = K2OS_Rpc_CreateObj(0, &sExecRofsClassDef.ClassId, (UINT32)&sgExecRofs);
    if (NULL == hRpcObj)
    {
        K2OSKERN_Panic("ROFS: failed to create rofs object(%08X)\n", K2OS_Thread_GetLastStatus());
    }

    sgExecRofs.mRpcIfInst = K2OS_RpcObj_AddIfInst(
        sgExecRofs.mRpcObj,
        K2OS_IFACE_CLASSCODE_FSPROV,
        &sFsProvIfaceId,
        &sgExecRofs.mIfInstId,
        TRUE
    );
    if (NULL == sgExecRofs.mRpcIfInst)
    {
        K2OSKERN_Panic("ROFS: Could not publish rofs ifinst\n");
    }

    //    Rofs_LoadAllKernelBuiltIn();
}
