//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
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
#include "ik2osrpc.h"

BOOL
K2OSRPC_Object_Create(
    UINT_PTR            aLocalSystemInterfaceInstanceIdOfRemoteRpcServer,
    K2_GUID128 const *  apClassId,
    UINT_PTR            aCreatorContext,
    UINT_PTR *          apRetNewServerObjectId,
    UINT_PTR *          apRetNewUsageId
)
{
    UINT_PTR fakeObjectId;
    UINT_PTR fakeUsageId;

    if (apRetNewServerObjectId == NULL)
    {
        apRetNewServerObjectId = &fakeObjectId;
    }
    *apRetNewServerObjectId = 0;

    if (apRetNewUsageId == NULL)
    {
        apRetNewUsageId = &fakeUsageId;
    }
    *apRetNewUsageId = 0;

    if ((NULL == apClassId) ||
        ((0 == *(((UINT32 *)apClassId) + 0)) &&
         (0 == *(((UINT32 *)apClassId) + 1)) &&
         (0 == *(((UINT32 *)apClassId) + 2)) &&
         (0 == *(((UINT32 *)apClassId) + 3))))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (0 == aLocalSystemInterfaceInstanceIdOfRemoteRpcServer)
    {
        return K2OSRPC_ServerObject_Create(apClassId, aCreatorContext, apRetNewServerObjectId, apRetNewUsageId);
    }

    return K2OSRPC_ClientUsage_Create(aLocalSystemInterfaceInstanceIdOfRemoteRpcServer, apClassId, aCreatorContext, apRetNewServerObjectId, apRetNewUsageId);
}

BOOL
K2OSRPC_Object_Acquire(
    UINT_PTR    aLocalSystemInterfaceInstanceIdOfRemoteRpcServer,
    UINT_PTR    aExistingServerObjectId,
    UINT_PTR *  apRetNewUsageId
)
{
    UINT_PTR fakeRef;

    if (apRetNewUsageId != NULL)
    {
        apRetNewUsageId = &fakeRef;
    }
    *apRetNewUsageId = 0;

    if (0 == aExistingServerObjectId)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (0 == aLocalSystemInterfaceInstanceIdOfRemoteRpcServer)
    {
        return K2OSRPC_ServerObject_Acquire(aExistingServerObjectId, apRetNewUsageId);
    }

    return K2OSRPC_ClientUsage_Acquire(aLocalSystemInterfaceInstanceIdOfRemoteRpcServer, aExistingServerObjectId, apRetNewUsageId);
}

K2OSRPC_OBJECT_USAGE_HDR *
K2OSRPC_ObjectUsage_FindAddRef(
    UINT_PTR    aUsageId
)
{
    K2OSRPC_OBJECT_USAGE_HDR *  pHdr;
    K2TREE_NODE *               pTreeNode;

    K2OS_CritSec_Enter(&gK2OSRPC_GlobalUsageTreeSec);

    pTreeNode = K2TREE_Find(&gK2OSRPC_GlobalUsageTree, aUsageId);

    if (NULL != pTreeNode)
    {
        pHdr = K2_GET_CONTAINER(K2OSRPC_OBJECT_USAGE_HDR, pTreeNode, GlobalUsageTreeNode);
        K2ATOMIC_Inc(&pHdr->mRefCount);
    }
    else
    {
        pHdr = NULL;
    }

    K2OS_CritSec_Leave(&gK2OSRPC_GlobalUsageTreeSec);

    return pHdr;
}

K2STAT
K2OSRPC_Object_Call(
    UINT_PTR                    aUsageId,
    K2OSRPC_CALLARGS const *    apArgs,
    UINT_PTR *                  apRetActualOut
)
{
    K2STAT                      stat;
    K2OSRPC_OBJECT_USAGE_HDR *  pHdr;
    UINT_PTR                    dummy;

    if (NULL == apArgs)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (NULL == apArgs->mpInBuf)
    {
        if (0 != apArgs->mInBufByteCount)
        {
            return K2STAT_ERROR_BAD_ARGUMENT;
        }
    }

    if (NULL == apArgs->mpOutBuf)
    {
        if (0 != apArgs->mOutBufByteCount)
        {
            return K2STAT_ERROR_BAD_ARGUMENT;
        }
    }

    pHdr = K2OSRPC_ObjectUsage_FindAddRef(aUsageId);
    if (NULL == pHdr)
        return K2STAT_ERROR_NOT_FOUND;

    if (NULL == apRetActualOut)
        apRetActualOut = &dummy;
    *apRetActualOut = 0;

    if (pHdr->mIsServer)
    {
        stat = K2OSRPC_ServerUsage_Call((K2OSRPC_SERVER_OBJECT_USAGE *)pHdr, apArgs, apRetActualOut);
        K2OSRPC_ServerUsage_Release((K2OSRPC_SERVER_OBJECT_USAGE *)pHdr);
    }
    else
    {
        stat = K2OSRPC_ClientUsage_Call((K2OSRPC_CLIENT_OBJECT_USAGE *)pHdr, apArgs, apRetActualOut);
        K2OSRPC_ClientUsage_Release((K2OSRPC_CLIENT_OBJECT_USAGE *)pHdr);
    }

    return stat;
}

BOOL
K2OSRPC_Object_Release(
    UINT_PTR aUsageId
)
{
    K2OSRPC_OBJECT_USAGE_HDR * pHdr;

    pHdr = K2OSRPC_ObjectUsage_FindAddRef(aUsageId);
    if (NULL == pHdr)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    if (pHdr->mIsServer)
    {
        K2OSRPC_ServerUsage_Release((K2OSRPC_SERVER_OBJECT_USAGE *)pHdr);
        K2OSRPC_ServerUsage_Release((K2OSRPC_SERVER_OBJECT_USAGE *)pHdr);
    }
    else
    {
        K2OSRPC_ClientUsage_Release((K2OSRPC_CLIENT_OBJECT_USAGE *)pHdr);
        K2OSRPC_ClientUsage_Release((K2OSRPC_CLIENT_OBJECT_USAGE *)pHdr);
    }

    return TRUE;
}
