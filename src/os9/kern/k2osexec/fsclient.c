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

static const K2_GUID128 sgFsFileRpcClassId = K2OS_RPCCLASS_FSFILE;

K2TREE_ANCHOR gFsClientTree;

static
void
iDirName(
    K2OSKERN_FSNODE *   apFsNode,
    char **             appOut,
    UINT32 *            apLeft
)
{
    UINT32 out;

    if (0 == (*apLeft))
        return;
    if (NULL != apFsNode->Static.mpParentDir)
    {
        // not root dir
        iDirName(apFsNode->Static.mpParentDir, appOut, apLeft);
        out = K2ASC_PrintfLen(*appOut, *apLeft, "%s", apFsNode->Static.mName);
        (*appOut) += out;
        (*apLeft) -= out;
        if (apFsNode->Static.mIsDir)
        {
            if (1 < *apLeft)
            {
                **appOut = '/';
                (*appOut)++;
                (*apLeft)--;
                **appOut = 0;
            }
        }
    }
    else
    {
        // root dir
        if (1 < *apLeft)
        {
            **appOut = '/';
            (*appOut)++;
            (*apLeft)--;
            **appOut = 0;
            (*apLeft)--;
        }
    }
}

K2STAT 
K2OSEXEC_FsClient_GetBaseDir(
    FSCLIENT *                          apClient,
    K2OS_FSCLIENT_GETBASEDIR_IN const * apIn
)
{
    K2OSKERN_MAPUSER    mapUser;
    char *              pDstBuf;
    K2STAT              stat;
    BOOL                disp;
    UINT32              bufLeft;
    K2OSKERN_FILE *     pCurDir;

    if (0 != (apIn->TargetBufDesc.mAttrib & K2OS_BUFDESC_ATTRIB_READONLY))
        return K2STAT_ERROR_ADDR_NOT_WRITEABLE;

    if (0 != apClient->mProcId)
    {
        if ((apIn->TargetBufDesc.mAddress >= K2OS_KVA_KERN_BASE) ||
            ((K2OS_KVA_KERN_BASE - apIn->TargetBufDesc.mAddress) < apIn->TargetBufDesc.mBytesLength))
            return K2STAT_ERROR_BAD_ARGUMENT;

        mapUser = gKernDdk.MapUserBuffer(apClient->mProcId, &apIn->TargetBufDesc, (UINT32 *)&pDstBuf);
        if (NULL == mapUser)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            return stat;
        }
    }
    else
    {
        pDstBuf = (char *)apIn->TargetBufDesc.mAddress;
    }

    stat = K2STAT_NO_ERROR;

    disp = K2OSKERN_SeqLock(&apClient->SeqLockForCurDir);
    pCurDir = apClient->mpCurDir;
    K2OSEXEC_KernFile_AddRef(pCurDir);
    K2OSKERN_SeqUnlock(&apClient->SeqLockForCurDir, disp);

    K2_ASSERT(0 != pCurDir->MapTreeNode.mUserVal);
    bufLeft = apIn->TargetBufDesc.mBytesLength;
    iDirName((K2OSKERN_FSNODE *)pCurDir->MapTreeNode.mUserVal, &pDstBuf, &bufLeft);

    K2OSEXEC_KernFile_Release(pCurDir);

    if (0 != apClient->mProcId)
    {
        gKernDdk.UnmapUserBuffer(mapUser);
    }

    return stat;
}

K2STAT 
K2OSEXEC_FsClient_SetBaseDir(
    FSCLIENT *                          apClient,
    K2OS_FSCLIENT_SETBASEDIR_IN const * apIn
)
{
    K2OSKERN_MAPUSER    mapUser;
    char const *        pSrcBuf;
    K2STAT              stat;
    BOOL                disp;
    K2OSKERN_FILE *     pCurDir;
    K2OSKERN_FILE *     pTarget;

    if (0 != apClient->mProcId)
    {
        if ((apIn->SourceBufDesc.mAddress >= K2OS_KVA_KERN_BASE) ||
            ((K2OS_KVA_KERN_BASE - apIn->SourceBufDesc.mAddress) < apIn->SourceBufDesc.mBytesLength))
            return K2STAT_ERROR_BAD_ARGUMENT;

        mapUser = gKernDdk.MapUserBuffer(apClient->mProcId, &apIn->SourceBufDesc, (UINT32 *)&pSrcBuf);
        if (NULL == mapUser)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            return stat;
        }
    }
    else
    {
        pSrcBuf = (char const *)apIn->SourceBufDesc.mAddress;
    }

    disp = K2OSKERN_SeqLock(&apClient->SeqLockForCurDir);
    pCurDir = apClient->mpCurDir;
    K2OSEXEC_KernFile_AddRef(pCurDir);
    K2OSKERN_SeqUnlock(&apClient->SeqLockForCurDir, disp);

    stat = K2OSEXEC_KernFile_Acquire(
        pCurDir, 
        pSrcBuf, 
        K2OS_ACCESS_R, 
        K2OS_ACCESS_RW, 
        K2OS_FileOpen_Existing, 
        0,
        0,
        &pTarget);

    K2OSEXEC_KernFile_Release(pCurDir);

    if (!K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL != pTarget);

        //
        // swap the dir and release the one we held
        //
        disp = K2OSKERN_SeqLock(&apClient->SeqLockForCurDir);
        pCurDir = apClient->mpCurDir;
        apClient->mpCurDir = pTarget;
        K2OSKERN_SeqUnlock(&apClient->SeqLockForCurDir, disp);

        K2OSEXEC_KernFile_Release(pCurDir);
    }

    if (0 != apClient->mProcId)
    {
        gKernDdk.UnmapUserBuffer(mapUser);
    }

    return stat;
}

K2STAT 
K2OSEXEC_FsClient_CreateDir(
    FSCLIENT *                          apClient,
    K2OS_FSCLIENT_CREATEDIR_IN const *  apIn
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
K2OSEXEC_FsClient_RemoveDir(
    FSCLIENT *                          apClient,
    K2OS_FSCLIENT_REMOVEDIR_IN const *  apIn
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
K2OSEXEC_FsClient_GetAttrib(
    FSCLIENT *                          apClient,
    K2OS_FSCLIENT_GETATTRIB_IN const *  apIn,
    K2OS_FSCLIENT_GETATTRIB_OUT *       apOut
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
K2OSEXEC_FsClient_SetAttrib(
    FSCLIENT *                          apClient,
    K2OS_FSCLIENT_SETATTRIB_IN const *  apIn,
    K2OS_FSCLIENT_SETATTRIB_OUT *       apOut
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
K2OSEXEC_FsClient_DeleteFile(
    FSCLIENT *                          apClient,
    K2OS_FSCLIENT_DELETEFILE_IN const * apIn
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
K2OSEXEC_FsClient_GetIoBlockAlign(
    FSCLIENT *                              apClient,
    K2OS_FSCLIENT_GETIOBLOCKALIGN_IN const *apIn,
    K2OS_FSCLIENT_GETIOBLOCKALIGN_OUT *     apOut
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
K2OSEXEC_FsClientRpc_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    FSCLIENT *  pFsClient;

    pFsClient = (FSCLIENT *)K2OS_Heap_Alloc(sizeof(FSCLIENT));
    if (NULL == pFsClient)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pFsClient, sizeof(FSCLIENT));

    K2OSKERN_SeqInit(&pFsClient->SeqLockForCurDir);

    pFsClient->mRpcObj = aObject;
    pFsClient->mProcId = apCreate->mCreatorProcessId;
    pFsClient->mCreatorContext = apCreate->mCreatorContext;

    //
    // all clients start with the current dir set to root
    //
    pFsClient->mpCurDir = gFsMgr.mpKernFileForRoot;
    K2OSEXEC_KernFile_AddRef(pFsClient->mpCurDir);

    *apRetContext = (UINT32)pFsClient;

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_FsClientRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    FSCLIENT *  pFsClient;
    K2STAT      stat;

    pFsClient = (FSCLIENT *)aObjContext;

    K2_ASSERT(aObject == pFsClient->mRpcObj);

    if (aProcessId != pFsClient->mProcId)
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    stat = K2STAT_NO_ERROR;

    K2OS_CritSec_Enter(&gFsMgr.Sec);

    if (0 == pFsClient->mObjId)
    {
        //
        // first attach
        //
        if (!K2OS_RpcObj_GetDetail(pFsClient->mRpcObj, NULL, &pFsClient->mObjId))
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
        else
        {
            pFsClient->TreeNode.mUserVal = pFsClient->mObjId;
            K2TREE_Insert(&gFsClientTree, pFsClient->mObjId, &pFsClient->TreeNode);
        }
    }

    K2OS_CritSec_Leave(&gFsMgr.Sec);

    *apRetUseContext = 0;

    return stat;
}

K2STAT
K2OSEXEC_FsClientRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_FsClientRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    FSCLIENT *  pFsClient;
    K2STAT      stat;

    pFsClient = (FSCLIENT *)apCall->mObjContext;

    K2_ASSERT(apCall->mObj == pFsClient->mRpcObj);

    switch (apCall->Args.mMethodId)
    {
    case K2OS_FsClient_Method_GetBaseDir:
        if ((sizeof(K2OS_FSCLIENT_GETBASEDIR_IN) > apCall->Args.mInBufByteCount) ||
            (0 != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsClient_GetBaseDir(
                pFsClient,
                (K2OS_FSCLIENT_GETBASEDIR_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_FsClient_Method_SetBaseDir:
        if ((sizeof(K2OS_FSCLIENT_SETBASEDIR_IN) > apCall->Args.mInBufByteCount) ||
            (0 != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsClient_SetBaseDir(
                pFsClient,
                (K2OS_FSCLIENT_SETBASEDIR_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_FsClient_Method_CreateDir:
        if ((sizeof(K2OS_FSCLIENT_CREATEDIR_IN) > apCall->Args.mInBufByteCount) ||
            (0 != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsClient_CreateDir(
                pFsClient,
                (K2OS_FSCLIENT_CREATEDIR_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_FsClient_Method_RemoveDir:
        if ((sizeof(K2OS_FSCLIENT_REMOVEDIR_IN) > apCall->Args.mInBufByteCount) ||
            (0 != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsClient_RemoveDir(
                pFsClient,
                (K2OS_FSCLIENT_REMOVEDIR_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_FsClient_Method_GetAttrib:
        if ((sizeof(K2OS_FSCLIENT_GETATTRIB_IN) > apCall->Args.mInBufByteCount) ||
            (sizeof(K2OS_FSCLIENT_GETATTRIB_OUT) > apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsClient_GetAttrib(
                pFsClient,
                (K2OS_FSCLIENT_GETATTRIB_IN const *)apCall->Args.mpInBuf,
                (K2OS_FSCLIENT_GETATTRIB_OUT *)apCall->Args.mpOutBuf
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_FSCLIENT_GETATTRIB_OUT);
            }
        }
        break;

    case K2OS_FsClient_Method_SetAttrib:
        if ((sizeof(K2OS_FSCLIENT_SETATTRIB_IN) > apCall->Args.mInBufByteCount) ||
            (sizeof(K2OS_FSCLIENT_SETATTRIB_OUT) > apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsClient_SetAttrib(
                pFsClient,
                (K2OS_FSCLIENT_SETATTRIB_IN const *)apCall->Args.mpInBuf,
                (K2OS_FSCLIENT_SETATTRIB_OUT *)apCall->Args.mpOutBuf
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_FSCLIENT_SETATTRIB_OUT);
            }
        }
        break;

    case K2OS_FsClient_Method_DeleteFile:
        if ((sizeof(K2OS_FSCLIENT_DELETEFILE_IN) > apCall->Args.mInBufByteCount) ||
            (0 != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsClient_DeleteFile(
                pFsClient,
                (K2OS_FSCLIENT_DELETEFILE_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_FsClient_Method_GetIoBlockAlign:
        if ((sizeof(K2OS_FSCLIENT_GETIOBLOCKALIGN_IN) > apCall->Args.mInBufByteCount) ||
            (sizeof(K2OS_FSCLIENT_GETIOBLOCKALIGN_OUT) > apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsClient_GetIoBlockAlign(
                pFsClient,
                (K2OS_FSCLIENT_GETIOBLOCKALIGN_IN const *)apCall->Args.mpInBuf,
                (K2OS_FSCLIENT_GETIOBLOCKALIGN_OUT *)apCall->Args.mpOutBuf
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_FSCLIENT_GETIOBLOCKALIGN_OUT);
            }
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    }

    return stat;
}

K2STAT
K2OSEXEC_FsClientRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    FSCLIENT *  pFsClient;

    pFsClient = (FSCLIENT *)aObjContext;

    K2_ASSERT(aObject == pFsClient->mRpcObj);

    K2OS_CritSec_Enter(&gFsMgr.Sec);
    if (0 != pFsClient->mObjId)
    {
        K2TREE_Remove(&gFsClientTree, &pFsClient->TreeNode);
    }
    K2OS_CritSec_Leave(&gFsMgr.Sec);

    if (NULL != pFsClient->mpCurDir)
    {
        K2OSEXEC_KernFile_Release(pFsClient->mpCurDir);
    }

    K2OS_Heap_Free(pFsClient);

    return K2STAT_NO_ERROR;
}

