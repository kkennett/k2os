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

K2STAT
K2OSEXEC_FsFileUse_Bind(
    FSFILEUSE *                 apUse,
    K2OS_FSFILE_BIND_IN const * apIn
)
{
    FSCLIENT *          pClient;
    K2OSKERN_FILE *     pStartDir;
    BOOL                disp;
    K2OSKERN_MAPUSER    mapUser;
    K2STAT              stat;
    char const *        pPath;
    K2OSKERN_FILE *     pKernFile;

    pClient = apUse->mpClient;

    if (0 != pClient->mProcId)
    {
        mapUser = gKernDdk.MapUserBuffer(pClient->mProcId, &apIn->SourceBufDesc, (UINT32 *)&pPath);
        if (NULL == mapUser)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            return stat;
        }
    }
    else
    {
        pPath = (char const *)apIn->SourceBufDesc.mAddress;
        K2_ASSERT(NULL != pPath);
    }

    K2OS_CritSec_Enter(&apUse->Sec);

    if (NULL != apUse->mpKernFile)
    {
        stat = K2STAT_ERROR_ALREADY_OPEN;
    }
    else
    {
        disp = K2OSKERN_SeqLock(&pClient->SeqLockForCurDir);
        pStartDir = pClient->mpCurDir;
        K2OSEXEC_KernFile_AddRef(pStartDir);
        K2OSKERN_SeqUnlock(&pClient->SeqLockForCurDir, disp);

        pKernFile = NULL;
        stat = K2OSEXEC_KernFile_Acquire(
            pStartDir,
            pPath,
            apIn->mAccess,
            apIn->mShare,
            apIn->mOpenType,
            apIn->mOpenFlags,
            apIn->mNewFileAttrib,
            &pKernFile
        );

        K2OSEXEC_KernFile_Release(pStartDir);

        if (!K2STAT_IS_ERROR(stat))
        {
            apUse->mpKernFile = pKernFile;
        }
    }

    K2OS_CritSec_Leave(&apUse->Sec);

    if (0 != pClient->mProcId)
    {
        gKernDdk.UnmapUserBuffer(mapUser);
    }

    return stat;
}

K2STAT
K2OSEXEC_FsFileUse_Read(
    FSFILEUSE *                 apUse,
    K2OS_FSFILE_READ_IN const * apIn,
    K2OS_FSFILE_READ_OUT *      apOut
)
{
    K2STAT          stat;
    UINT32          red;
    K2OSKERN_FILE * pKernFile;

    K2OS_CritSec_Enter(&apUse->Sec);

    pKernFile = apUse->mpKernFile;
    if (NULL == pKernFile)
    {
        stat = K2STAT_ERROR_NOT_OPEN;
    }
    else
    {
        red = 0;
        stat = K2OSEXEC_KernFile_Read(pKernFile, apUse->mpClient->mProcId, &apIn->TargetBufDesc, &apUse->mPointer, apIn->mBytesToRead, &red);
        if (0 != red)
        {
            apUse->mPointer += red;
        }
        apOut->mBytesRed = red;
    }

    K2OS_CritSec_Leave(&apUse->Sec);

    return stat;
}

K2STAT
K2OSEXEC_FsFileUseRpc_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    static K2_GUID128 const sRpcClientClassId = K2OS_RPCCLASS_FSCLIENT;

    FSFILEUSE *         pNewFileUse;
    K2OS_RPC_OBJ_HANDLE hClient;
    FSCLIENT *          pClient;
    K2STAT              stat;
    K2TREE_NODE *       pTreeNode;
    K2_GUID128          clientClassCheck;

    hClient = K2OS_Rpc_AttachByObjId(0, apCreate->mCreatorContext);
    if (NULL == hClient)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    do
    {
        if (!K2OS_Rpc_GetObjClass(hClient, &clientClassCheck))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }
        if (0 != K2MEM_Compare(&clientClassCheck, &sRpcClientClassId, sizeof(K2_GUID128)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        K2OS_CritSec_Enter(&gFsMgr.Sec);
        pTreeNode = K2TREE_Find(&gFsClientTree, apCreate->mCreatorContext);
        if (NULL != pTreeNode)
        {
            pClient = K2_GET_CONTAINER(FSCLIENT, pTreeNode, TreeNode);
            if (pClient->mProcId != apCreate->mCreatorProcessId)
            {
                stat = K2STAT_ERROR_NOT_ALLOWED;
                pTreeNode = NULL;
            }
        }
        else
        {
            stat = K2STAT_ERROR_NOT_FOUND;
        }
        K2OS_CritSec_Leave(&gFsMgr.Sec);

        if (NULL == pTreeNode)
            break;

        pNewFileUse = (FSFILEUSE *)K2OS_Heap_Alloc(sizeof(FSFILEUSE));
        if (NULL == pNewFileUse)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }

        do
        {
            K2MEM_Zero(pNewFileUse, sizeof(FSFILEUSE));

            pNewFileUse->mhClient = hClient;
            pNewFileUse->mpClient = pClient;

            pNewFileUse->mRpcObj = aObject;

            if (!K2OS_CritSec_Init(&pNewFileUse->Sec))
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
            }

            stat = K2STAT_NO_ERROR;

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Heap_Free(pNewFileUse);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Rpc_Release(hClient);
    }
    else
    {
        *apRetContext = (UINT32)pNewFileUse;
    }

    return stat;
}

K2STAT
K2OSEXEC_FsFileUseRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    FSFILEUSE * pFsFileUse;

    pFsFileUse = (FSFILEUSE *)aObjContext;
    K2_ASSERT(aObject == pFsFileUse->mRpcObj);

    K2_ASSERT(NULL != pFsFileUse->mpClient);
    K2_ASSERT(NULL != pFsFileUse->mhClient);

    // only one attach is allowed
    if (0 != K2ATOMIC_CompareExchange(&pFsFileUse->mInUse, 1, 0))
    {
        return K2STAT_ERROR_IN_USE;
    }

    *apRetUseContext = 0;

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_FsFileUseRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_FsFileUseRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    FSFILEUSE * pFileUse;
    K2STAT      stat;

    pFileUse = (FSFILEUSE *)apCall->mObjContext;
    K2_ASSERT(pFileUse->mRpcObj == apCall->mObj);

    switch (apCall->Args.mMethodId)
    {
    case K2OS_FsFile_Method_Bind:
        if ((sizeof(K2OS_FSFILE_BIND_IN) > apCall->Args.mInBufByteCount) ||
            (0 != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsFileUse_Bind(
                pFileUse, 
                (K2OS_FSFILE_BIND_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_FsFile_Method_Read:
        if ((sizeof(K2OS_FSFILE_READ_IN) > apCall->Args.mInBufByteCount) ||
            (sizeof(K2OS_FSFILE_READ_OUT) > apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2OSEXEC_FsFileUse_Read(
                pFileUse,
                (K2OS_FSFILE_READ_IN const *)apCall->Args.mpInBuf,
                (K2OS_FSFILE_READ_OUT *)apCall->Args.mpOutBuf
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_FSFILE_READ_OUT);
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
K2OSEXEC_FsFileUseRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    FSFILEUSE * pFsFileUse;

    pFsFileUse = (FSFILEUSE *)aObjContext;

    K2OSKERN_Debug("%s:%d\n", __FUNCTION__, __LINE__);

    K2_ASSERT(aObject == pFsFileUse->mRpcObj);

    K2OS_Rpc_Release(pFsFileUse->mhClient);

    K2OS_CritSec_Done(&pFsFileUse->Sec);

    K2OS_Heap_Free(pFsFileUse);

    return K2STAT_NO_ERROR;
}

