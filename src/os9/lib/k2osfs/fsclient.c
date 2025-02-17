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

#include "k2osfs.h"

K2_GUID128 const    gFsClientRpcClassId = K2OS_RPCCLASS_FSCLIENT;

K2OSFS_CLIENT       gK2OSFS_DefaultClient;
K2OS_CRITSEC        gK2OSFS_ClientTreeSec;
K2TREE_ANCHOR       gK2OSFS_ClientTree;

static K2_GUID128 const sgFsFileRpcClassId = K2OS_RPCCLASS_FSFILE;

K2OS_FSCLIENT   
K2OS_FsClient_Create(
    void
)
{
    K2OSFS_CLIENT * pFsClient;

    if (NULL == gK2OSFS_DefaultClient.mhRpcObj)
    {
        if (!FsMgr_Connected())
        {
            K2_ASSERT(K2STAT_IS_ERROR(K2OS_Thread_GetLastStatus()));
            return NULL;
        }
    }

    pFsClient = (K2OSFS_CLIENT *)K2OS_Heap_Alloc(sizeof(K2OSFS_CLIENT));
    if (NULL == pFsClient)
        return NULL;

    K2MEM_Zero(pFsClient, sizeof(K2OSFS_CLIENT));
    pFsClient->mhRpcObj = K2OS_Rpc_CreateObj(gK2OSFS_FsMgrRpcServerIfInstId, &gFsClientRpcClassId, 0);
    if (NULL != pFsClient->mhRpcObj)
    {
        if (!K2OS_Rpc_GetObjId(pFsClient->mhRpcObj, &pFsClient->mObjId))
        {
            pFsClient->mObjId = 0;
        }
        else
        {
            pFsClient->TreeNode.mUserVal = (UINT_PTR)pFsClient;
            K2OS_CritSec_Enter(&gK2OSFS_ClientTreeSec);
            K2TREE_Insert(&gK2OSFS_ClientTree, (UINT_PTR)pFsClient, &pFsClient->TreeNode);
            K2OS_CritSec_Leave(&gK2OSFS_ClientTreeSec);
        }
    }

    if (0 == pFsClient->mObjId)
    {
        if (NULL != pFsClient->mhRpcObj)
        {
            K2OS_Rpc_Release(pFsClient->mhRpcObj);
            pFsClient->mhRpcObj = NULL;
        }
        K2OS_Heap_Free(pFsClient->mhRpcObj);
        pFsClient = NULL;
    }

    return (K2OS_FSCLIENT)pFsClient;
}

K2OS_RPC_OBJ_HANDLE
K2OSFS_GetClientRpc(
    K2OS_FSCLIENT   aClient,
    UINT32 *        apRetObjId
)
{
    K2TREE_NODE *       pTreeNode;
    K2OSFS_CLIENT *     pClient;
    K2OS_RPC_OBJ_HANDLE hResult;

    if (NULL == aClient)
    {
        if (NULL == gK2OSFS_DefaultClient.mhRpcObj)
        {
            if (!FsMgr_Connected())
            {
                K2_ASSERT(K2STAT_IS_ERROR(K2OS_Thread_GetLastStatus()));
                return NULL;
            }
        }

        if (NULL != apRetObjId)
        {
            *apRetObjId = gK2OSFS_DefaultClient.mObjId;
        }

        hResult = gK2OSFS_DefaultClient.mhRpcObj;
    }
    else
    {
        K2OS_CritSec_Enter(&gK2OSFS_ClientTreeSec);

        pTreeNode = K2TREE_Find(&gK2OSFS_ClientTree, (UINT_PTR)aClient);
        if (NULL != pTreeNode)
        {
            pClient = K2_GET_CONTAINER(K2OSFS_CLIENT, pTreeNode, TreeNode);
            hResult = pClient->mhRpcObj;
            if (NULL != apRetObjId)
            {
                *apRetObjId = pClient->mObjId;
            }
        }

        K2OS_CritSec_Leave(&gK2OSFS_ClientTreeSec);

        if (NULL == pTreeNode)
        {
            hResult = NULL;
        }
    }

    return hResult;
}

BOOL            
K2OS_FsClient_GetBaseDir(
    K2OS_FSCLIENT   aFsClient,
    char *          apRetPathBuf,
    UINT_PTR        aBufferBytes
)
{
    K2OS_RPC_OBJ_HANDLE         hClient;
    K2OS_RPC_CALLARGS           Args;
    K2OS_FSCLIENT_GETBASEDIR_IN InParams;
    UINT32                      actualOut;
    K2STAT                      stat;

    K2MEM_Touch(apRetPathBuf, aBufferBytes);

    hClient = K2OSFS_GetClientRpc(aFsClient, NULL);
    if (NULL == hClient)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    InParams.TargetBufDesc.mAddress = (UINT32)apRetPathBuf;
    InParams.TargetBufDesc.mBytesLength = aBufferBytes;
    InParams.TargetBufDesc.mAttrib = 0;

    Args.mMethodId = K2OS_FsClient_Method_GetBaseDir;
    Args.mpInBuf = (UINT8 const *)&InParams;
    Args.mInBufByteCount = sizeof(InParams);
    Args.mpOutBuf = NULL;
    Args.mOutBufByteCount = 0;

    actualOut = 0;
    stat = K2OS_Rpc_Call(hClient, &Args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL            
K2OS_FsClient_SetBaseDir(
    K2OS_FSCLIENT   aFsClient,
    char const *    apPath
)
{
    K2OS_RPC_OBJ_HANDLE         hClient;
    K2OS_RPC_CALLARGS           Args;
    K2OS_FSCLIENT_SETBASEDIR_IN InParams;
    UINT32                      actualOut;
    K2STAT                      stat;

    hClient = K2OSFS_GetClientRpc(aFsClient, NULL);
    if (NULL == hClient)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }
    
    InParams.SourceBufDesc.mAddress = (UINT32)apPath;
    while (0 != *apPath)
        apPath++;
    InParams.SourceBufDesc.mBytesLength = ((UINT32)apPath) - InParams.SourceBufDesc.mAddress + 1;
    InParams.SourceBufDesc.mAttrib = K2OS_BUFDESC_ATTRIB_READONLY;

    Args.mMethodId = K2OS_FsClient_Method_SetBaseDir;
    Args.mpInBuf = (UINT8 const *)&InParams;
    Args.mInBufByteCount = sizeof(InParams);
    Args.mpOutBuf = NULL;
    Args.mOutBufByteCount = 0;

    actualOut = 0;
    stat = K2OS_Rpc_Call(hClient, &Args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL            
K2OS_FsClient_CreateDir(
    K2OS_FSCLIENT   aFsClient,
    char const *    apPath
)
{
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

BOOL            
K2OS_FsClient_RemoveDir(
    K2OS_FSCLIENT   aFsClient,
    char const *    apPath
)
{
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

K2OS_FILE
K2OS_FsClient_OpenFile(
    K2OS_FSCLIENT       aFsClient,
    char const *        apPathToItem,
    UINT32              aAccess,
    UINT32              aShare,
    K2OS_FileOpenType   aOpenType,
    UINT32              aOpenFlags,
    UINT32              aNewFileAttrib
)
{
    K2OS_RPC_OBJ_HANDLE     hClient;
    K2OS_RPC_CALLARGS       Args;
    K2OS_FSFILE_BIND_IN     InParams;
    UINT32                  actualOut;
    K2STAT                  stat;
    K2OS_RPC_OBJ_HANDLE     hFile;
    UINT32                  fsClientObjId;

    hClient = K2OSFS_GetClientRpc(aFsClient, &fsClientObjId);
    if (NULL == hClient)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return NULL;
    }

    stat = K2STAT_NO_ERROR;

    do
    {
        hFile = K2OS_Rpc_CreateObj(gK2OSFS_FsMgrRpcServerIfInstId, &sgFsFileRpcClassId, fsClientObjId);
        if (NULL == hFile)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }

        InParams.mFsClientObjId = fsClientObjId;

        InParams.SourceBufDesc.mAddress = (UINT32)apPathToItem;
        while (0 != *apPathToItem)
            apPathToItem++;
        InParams.SourceBufDesc.mBytesLength = ((UINT32)apPathToItem) - InParams.SourceBufDesc.mAddress + 1;
        InParams.SourceBufDesc.mAttrib = K2OS_BUFDESC_ATTRIB_READONLY;

        InParams.mAccess = aAccess;
        InParams.mShare = aShare;
        InParams.mOpenType = aOpenType;
        InParams.mOpenFlags = aOpenFlags;
        InParams.mNewFileAttrib = aNewFileAttrib;

        Args.mMethodId = K2OS_FsFile_Method_Bind;
        Args.mpInBuf = (UINT8 const *)&InParams;
        Args.mInBufByteCount = sizeof(InParams);
        Args.mpOutBuf = NULL;
        Args.mOutBufByteCount = 0;

        actualOut = 0;
        stat = K2OS_Rpc_Call(hFile, &Args, &actualOut);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Rpc_Release(hFile);
            hFile = NULL;
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    return (K2OS_FILE)hFile;
}

UINT32          
K2OS_FsClient_GetAttrib(
    K2OS_FSCLIENT   aFsClient,
    char const *    apPathToItem
)
{
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return K2_FSATTRIB_INVALID;
}

UINT32          
K2OS_FsClient_SetAttrib(
    K2OS_FSCLIENT   aFsClient,
    char const *    apPathToItem,
    UINT32          aNewAttrib
)
{
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return K2_FSATTRIB_INVALID;
}

BOOL            
K2OS_FsClient_DeleteFile(
    K2OS_FSCLIENT   aFsClient,
    char const *    apPathToItem
)
{
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

UINT32          
K2OS_FsClient_GetIoBlockAlign(
    K2OS_FSCLIENT   aFsClient,
    char const *    apPathToItem
)
{
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return 0;
}

BOOL            
K2OS_FsClient_Destroy(
    K2OS_FSCLIENT aClient
)
{
    return K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)aClient);
}

K2OS_FSENUM     
K2OS_FsClient_CreateEnum(
    K2OS_FSCLIENT       aFsClient,
    char const *        apSpec,
    K2OS_FSITEM_INFO *  apRetInfo
)
{
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return NULL;
}
