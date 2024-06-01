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

typedef struct _ROFSHANDLE      ROFSHANDLE;
typedef struct _ROFSUSER_PROC   ROFSPROC;
typedef struct _ROFSUSER        ROFSUSER;
typedef struct _ROFSEXEC        ROFSEXEC;

struct _ROFSHANDLE
{
    ROFSPROC * mpProc;
    K2TREE_NODE     HandleTreeNode;
    K2LIST_LINK     ProcHandleListLink;
};

struct _ROFSUSER_PROC
{
    UINT32          mProcessId;
    K2LIST_LINK     ProcListLink;

    K2LIST_ANCHOR   UserList;
    K2LIST_ANCHOR   HandleList;
};

struct _ROFSUSER
{
    ROFSPROC * mpProc;
    K2LIST_LINK     ProcUserListLink;
};

struct _ROFSEXEC
{
    K2OS_RPC_OBJ        mRpcObj;

    K2OS_RPC_IFINST     mRpcIfInst;
    K2OS_IFINST_ID      mIfInstId;

    K2OS_CRITSEC        Sec;

    K2LIST_ANCHOR       ProcList;
    K2TREE_ANCHOR       HandleTree;
    UINT32 volatile     mNextHandleId;
};
static ROFSEXEC sgExecRofs;

K2STAT
K2OSEXEC_RofsRpc_Create(
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

    K2OSKERN_Debug("%s() ok\n", __FUNCTION__);

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_RofsRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    ROFSPROC * pNewProc;
    ROFSPROC * pProc;
    ROFSUSER *      pUser;
    K2LIST_LINK *   pListLink;

    K2OSKERN_Debug("%s()\n", __FUNCTION__);

    pUser = (ROFSUSER *)K2OS_Heap_Alloc(sizeof(ROFSUSER));
    if (NULL == pUser)
    {
        return K2OS_Thread_GetLastStatus();
    }
    K2MEM_Zero(pUser, sizeof(ROFSUSER));
    
    K2OS_CritSec_Enter(&sgExecRofs.Sec);

    pListLink = sgExecRofs.ProcList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProc = K2_GET_CONTAINER(ROFSPROC, pListLink, ProcListLink);
            if (pProc->mProcessId == aProcessId)
            {
                pUser->mpProc = pProc;
                K2LIST_AddAtTail(&pProc->UserList, &pUser->ProcUserListLink);
                break;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&sgExecRofs.Sec);

    if (NULL != pListLink)
    {
        *apRetUseContext = (UINT32)pUser;
        return K2STAT_NO_ERROR;
    }

    pNewProc = (ROFSPROC *)K2OS_Heap_Alloc(sizeof(ROFSPROC));
    if (NULL == pNewProc)
    {
        K2OS_Heap_Free(pUser);
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pNewProc, sizeof(ROFSPROC));
    K2LIST_Init(&pNewProc->HandleList);
    K2LIST_Init(&pNewProc->UserList);
    pNewProc->mProcessId = aProcessId;
    
    K2OS_CritSec_Enter(&sgExecRofs.Sec);

    pListLink = sgExecRofs.ProcList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProc = K2_GET_CONTAINER(ROFSPROC, pListLink, ProcListLink);
            if (pProc->mProcessId == aProcessId)
            {
                pUser->mpProc = pProc;
                K2LIST_AddAtTail(&pProc->UserList, &pUser->ProcUserListLink);
                break;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    if (NULL == pListLink)
    {
        pUser->mpProc = pNewProc;
        K2LIST_AddAtTail(&pNewProc->UserList, &pUser->ProcUserListLink);
        K2LIST_AddAtTail(&sgExecRofs.ProcList, &pNewProc->ProcListLink);
        pNewProc = NULL;
    }

    K2OS_CritSec_Leave(&sgExecRofs.Sec);

    if (NULL != pNewProc)
    {
        K2OS_Heap_Free(pNewProc);
    }

    *apRetUseContext = (UINT32)pUser;

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_RofsRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    ROFSUSER *      pUser;
    ROFSPROC * pProc;
    K2LIST_LINK *   pListLink;
    ROFSHANDLE *    pHandle;

    K2OSKERN_Debug("%s()\n", __FUNCTION__);

    pUser = (ROFSUSER *)aUseContext;
    pProc = pUser->mpProc;

    K2OS_CritSec_Enter(&sgExecRofs.Sec);

    K2LIST_Remove(&pProc->UserList, &pUser->ProcUserListLink);
    if (0 == pProc->UserList.mNodeCount)
    {
        //
        // last user in process is detaching
        //
        K2LIST_Remove(&sgExecRofs.ProcList, &pProc->ProcListLink);
        pListLink = pProc->HandleList.mpHead;
        if (NULL != pListLink)
        {
            //
            // process has open handles when last user from process is detaching
            //
            do {
                pHandle = K2_GET_CONTAINER(ROFSHANDLE, pListLink, ProcHandleListLink);
                pListLink = pListLink->mpNext;
                K2TREE_Remove(&sgExecRofs.HandleTree, &pHandle->HandleTreeNode);
                K2LIST_Remove(&pProc->HandleList, &pHandle->ProcHandleListLink);
                // dec refcount on target of handle
                K2_ASSERT(0);
                K2OS_Heap_Free(pHandle);
            } while (NULL != pListLink);
        }
    }
    else
    {
        pProc = NULL;
    }

    K2OS_CritSec_Leave(&sgExecRofs.Sec);

    K2OS_Heap_Free(pUser);

    if (NULL != pProc)
    {
        K2_ASSERT(0 == pProc->HandleList.mNodeCount);
        K2_ASSERT(0 == pProc->UserList.mNodeCount);
        K2OS_Heap_Free(pProc);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_RofsRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    K2OS_FILESYS_OPENROOT_OUT   openOut;
    K2OSKERN_Debug("%s(%d)\n", __FUNCTION__, apCall->Args.mMethodId);

    if (K2OS_FILESYS_METHOD_OPEN_ROOT != apCall->Args.mMethodId)
    {
        return K2STAT_ERROR_NOT_IMPL;
    }

    if ((apCall->Args.mInBufByteCount > 0) ||
        (apCall->Args.mOutBufByteCount < sizeof(K2OS_FILESYS_OPENROOT_OUT)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2MEM_Zero(&openOut, sizeof(openOut));


    K2MEM_Copy(apCall->Args.mpOutBuf, &openOut, sizeof(openOut));
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
K2OSEXEC_RofsRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    K2OSKERN_Panic("Rofs obj delete - should be impossible.\n");
    return K2STAT_ERROR_UNKNOWN;
}

void
Rofs_Init(
    void
)
{
    // {411A183D-7D95-4126-867C-8B0F1A918927}
    static const K2OS_RPC_OBJ_CLASSDEF sExecRofsClassDef =
    {
        { 0x411a183d, 0x7d95, 0x4126, { 0x86, 0x7c, 0x8b, 0xf, 0x1a, 0x91, 0x89, 0x27 } },
        K2OSEXEC_RofsRpc_Create,
        K2OSEXEC_RofsRpc_OnAttach,
        K2OSEXEC_RofsRpc_OnDetach,
        K2OSEXEC_RofsRpc_Call,
        K2OSEXEC_RofsRpc_Delete
    };
    static const K2_GUID128 sFileSysIfaceId = K2OS_IFACE_FILESYS;

    K2OS_RPC_CLASS      rofsClass;
    K2OS_RPC_OBJ_HANDLE hRpcObj;

    K2MEM_Zero(&sgExecRofs, sizeof(sgExecRofs));
    K2LIST_Init(&sgExecRofs.ProcList);
    K2TREE_Init(&sgExecRofs.HandleTree, NULL);
    sgExecRofs.mNextHandleId = 1;
    if (!K2OS_CritSec_Init(&sgExecRofs.Sec))
    {
        K2OSKERN_Panic("ROFS: Critsec init failed\n");
    }

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
        K2OS_IFACE_CLASSCODE_FILESYS,
        &sFileSysIfaceId,
        &sgExecRofs.mIfInstId,
        TRUE
    );
    if (NULL == sgExecRofs.mRpcIfInst)
    {
        K2OSKERN_Panic("ROFS: Could not publish rofs ifinst\n");
    }
}

