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

static const K2OS_RPC_OBJ_CLASSDEF sFsClientClassDef =
{
    K2OS_RPCCLASS_FSCLIENT,
    K2OSEXEC_FsClientRpc_Create,
    K2OSEXEC_FsClientRpc_OnAttach,
    K2OSEXEC_FsClientRpc_OnDetach,
    K2OSEXEC_FsClientRpc_Call,
    K2OSEXEC_FsClientRpc_Delete
};

static const K2OS_RPC_OBJ_CLASSDEF sFsFileUseClassDef =
{
    K2OS_RPCCLASS_FSFILE,
    K2OSEXEC_FsFileUseRpc_Create,
    K2OSEXEC_FsFileUseRpc_OnAttach,
    K2OSEXEC_FsFileUseRpc_OnDetach,
    K2OSEXEC_FsFileUseRpc_Call,
    K2OSEXEC_FsFileUseRpc_Delete
};

// {8D8CC5EF-2C55-4702-964E-A4CCE3B24B7E}
static const K2OS_RPC_OBJ_CLASSDEF sFileSysClassDef =
{
    { 0x8d8cc5ef, 0x2c55, 0x4702, { 0x96, 0x4e, 0xa4, 0xcc, 0xe3, 0xb2, 0x4b, 0x7e } },
    K2OSEXEC_FileSysRpc_Create,
    K2OSEXEC_FileSysRpc_OnAttach,
    K2OSEXEC_FileSysRpc_OnDetach,
    K2OSEXEC_FileSysRpc_Call,
    K2OSEXEC_FileSysRpc_Delete
};

FSMGR gFsMgr;

K2STAT 
K2OSEXEC_FileSysRpc_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    FSMGR_FILESYS *pFileSys;
    K2_ASSERT(0 == apCreate->mCreatorProcessId);
    pFileSys = (FSMGR_FILESYS *)apCreate->mCreatorContext;
    pFileSys->mRpcObj = aObject;
    *apRetContext = (UINT32)pFileSys;
    return K2STAT_NO_ERROR;
}

K2STAT 
K2OSEXEC_FileSysRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    *apRetUseContext = aProcessId;
    return K2STAT_NO_ERROR;
}

K2STAT 
K2OSEXEC_FileSysRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    return K2STAT_NO_ERROR;
}

K2STAT 
K2OSEXEC_FileSysRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    K2STAT                          stat;
    FSMGR_FILESYS *                 pFileSys;
    K2OS_FILESYS_GETDETAIL_OUT *    pDetail;

    pFileSys = (FSMGR_FILESYS *)apCall->mObjContext;
    K2_ASSERT(apCall->mObj == pFileSys->mRpcObj);

    switch (apCall->Args.mMethodId)
    {
    case K2OS_FileSys_Method_GetDetail:
        if ((0 != apCall->Args.mInBufByteCount) ||
            (sizeof(K2OS_FILESYS_GETDETAIL_OUT) != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            pDetail = (K2OS_FILESYS_GETDETAIL_OUT *)apCall->Args.mpOutBuf;
            pDetail->mAttachContext = (UINT32)pFileSys->KernFileSys.Kern.mpAttachContext;
            pDetail->mProviderContext = pFileSys->KernFileSys.Fs.mProvInstanceContext;
            pDetail->mFsProv_Flags = pFileSys->KernFileSys.Kern.mpKernFsProv->mFlags;
            pDetail->mFsNumber = pFileSys->mFsNumber;
            pDetail->mAttachedReadOnly = pFileSys->KernFileSys.Fs.mReadOnly;
            pDetail->mIsCaseSensitive = pFileSys->KernFileSys.Fs.mCaseSensitive;
            pDetail->mCanDoPaging = !pFileSys->KernFileSys.Fs.mDoNotUseForPaging;
            *apRetUsedOutBytes = sizeof(K2OS_FILESYS_GETDETAIL_OUT);
            stat = K2STAT_NO_ERROR;
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    };

    return stat;
}

K2STAT 
K2OSEXEC_FileSysRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    FSMGR_FILESYS *pFileSys;

    pFileSys = (FSMGR_FILESYS *)aObjContext;
    K2_ASSERT(aObject == pFileSys->mRpcObj);

    if (NULL != pFileSys->mRpcIfInst)
    {
        K2OS_RpcObj_RemoveIfInst(aObject, pFileSys->mRpcIfInst);
    }

    // must have been shut down first. check that here
    K2_ASSERT(0);

    return K2STAT_ERROR_NOT_IMPL;
}

void
FsMgr_ShutdownFs(
    K2OSKERN_FILESYS *  apFileSys
)
{
    K2_ASSERT(0);
}

K2STAT
FsMgr_DoFsProv_Attach(
    FSPROV *    apProv,
    void *      apAttachContext
)
{
    K2STAT              stat;
    BOOL                disp;
    K2OSKERN_FSNODE *   pFsNode;
    FSMGR_FILESYS *     pMgrFileSys;
    static K2_GUID128   sFileSysIface = K2OS_IFACE_FILESYS;

    pFsNode = (K2OSKERN_FSNODE *)K2OS_Heap_Alloc(sizeof(K2OSKERN_FSNODE));
    if (NULL == pFsNode)
    {
        K2OSKERN_Debug("***FSMGR: Out of memory trying to allocate fsnode\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pMgrFileSys = (FSMGR_FILESYS *)K2OS_Heap_Alloc(sizeof(FSMGR_FILESYS));
    if (NULL == pMgrFileSys)
    {
        K2OS_Heap_Free(pFsNode);
        K2OSKERN_Debug("***FSMGR: Out of memory trying to allocate internal file system\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pMgrFileSys, sizeof(FSMGR_FILESYS));

    pMgrFileSys->mhRpcObj = K2OS_Rpc_CreateObj(0, &sFileSysClassDef.ClassId, (UINT32)pMgrFileSys);
    if (NULL == pMgrFileSys->mhRpcObj)
    {
        K2OS_Heap_Free(pMgrFileSys);
        K2OS_Heap_Free(pFsNode);
        K2OSKERN_Debug("***FSMGR: Out of memory trying to allocate internal file system rpc\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pMgrFileSys->KernFileSys.Kern.mpKernFsProv = apProv->mpKernFsProv;
    pMgrFileSys->KernFileSys.Ops.Kern.FsNodeInit = gFsMgr.mfFsNodeInit;
    pMgrFileSys->KernFileSys.Ops.Kern.FsShutdown = FsMgr_ShutdownFs;
    pMgrFileSys->KernFileSys.Kern.mpAttachContext = apAttachContext;
    gFsMgr.mfFsNodeInit(&pMgrFileSys->KernFileSys, pFsNode);
    pFsNode->Static.mpParentDir = gFsMgr.mpFsRootFsNode;
    pFsNode->Static.mIsDir = TRUE;
    pFsNode->Locked.mFsAttrib = K2_FSATTRIB_DIR;

    stat = K2STAT_NO_ERROR;

    do {
        stat = apProv->mpKernFsProv->Ops.Attach(apProv->mpKernFsProv, &pMgrFileSys->KernFileSys, pFsNode);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("FsProv \"%s\" failed to attach with context %08X\n", apProv->mpKernFsProv->mName, apAttachContext);
            break;
        }

        // put this back incase the driver killed it
        gFsMgr.mfFsNodeInit(&pMgrFileSys->KernFileSys, pFsNode);
        pFsNode->Static.mpParentDir = gFsMgr.mpFsRootFsNode;
        pFsNode->Static.mIsDir = TRUE;
        pFsNode->Locked.mFsAttrib = K2_FSATTRIB_DIR;
        gFsMgr.mpFsRootFsNode->Static.Ops.Kern.AddRef(gFsMgr.mpFsRootFsNode);

        // add the fs to the system
        K2OS_CritSec_Enter(&gFsMgr.Sec);

        K2LIST_AddAtTail(&gFsMgr.FsList, &pMgrFileSys->FsListLink);

        pMgrFileSys->mFsNumber = gFsMgr.mNextFsNumber++;
        K2ASC_Printf(pFsNode->Static.mName, "%d", pMgrFileSys->mFsNumber);

        disp = K2OSKERN_SeqLock(&gFsMgr.mpFsRootFsNode->ChangeSeqLock);

        K2TREE_Insert(&gFsMgr.mpFsRootFsNode->Locked.ChildTree, (UINT_PTR)pFsNode->Static.mName, &pFsNode->ParentLocked.ParentsChildTreeNode);

        K2OSKERN_SeqUnlock(&gFsMgr.mpFsRootFsNode->ChangeSeqLock, disp);

        K2OS_RpcObj_SendNotify(gFsMgr.mRpcObj, 0, K2OS_FsMgr_Notify_FsArrived, pMgrFileSys->mFsNumber);

        K2OSKERN_Debug("FileSystem #%d attached\n", pMgrFileSys->mFsNumber);

        K2OS_CritSec_Leave(&gFsMgr.Sec);

        pMgrFileSys->mRpcIfInst = K2OS_RpcObj_AddIfInst(
            pMgrFileSys->mRpcObj, 
            K2OS_IFACE_CLASSCODE_FILESYS,
            &sFileSysIface, 
            &pMgrFileSys->mIfInstId, 
            TRUE);

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Rpc_Release(pMgrFileSys->mhRpcObj);
        K2OS_Heap_Free(pFsNode);
    }

    return stat;
}

void
FsMgr_Vol_Arrived(
    K2OS_IFINST_ID aIfInstId
)
{
    K2LIST_LINK *           pListLink;
    K2STAT                  stat;
    BOOL                    ok;
    FSPROV *                pProv;
    FSMGR_KNOWN_VOLUME *    pKnownVol;

//    K2OSKERN_Debug("FSMGR: Volume with ifinstid %d arrived\n", aIfInstId);

    pKnownVol = (FSMGR_KNOWN_VOLUME *)K2OS_Heap_Alloc(sizeof(FSMGR_KNOWN_VOLUME));
    if (NULL == pKnownVol)
    {
        K2OSKERN_Debug("FSMGR: Out of memory\n");
        return;
    }

    K2MEM_Zero(pKnownVol, sizeof(FSMGR_KNOWN_VOLUME));

    pKnownVol->mIfInstId = aIfInstId;

    pProv = NULL;

    K2OS_CritSec_Enter(&gFsMgr.Sec);

    K2LIST_AddAtTail(&gFsMgr.KnownVolList, &pKnownVol->ListLink);

    pListLink = gFsMgr.FsProvList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProv = K2_GET_CONTAINER(FSPROV, pListLink, ListLink);
            if (0 != (pProv->mpKernFsProv->mFlags & K2OSKERN_FSPROV_FLAG_USE_VOLUME_IFINSTID))
            {
                ok = FALSE;
                stat = pProv->mpKernFsProv->Ops.Probe(pProv->mpKernFsProv, (void *)aIfInstId, &ok);
                if ((!K2STAT_IS_ERROR(stat)) && (ok))
                {
                    // provisional - revoked below on failure
                    pKnownVol->mpFsProv = pProv;
                    break;
                }
            }
            pProv = NULL;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&gFsMgr.Sec);

    if (NULL == pProv)
        return;

    stat = FsMgr_DoFsProv_Attach(pProv, (void *)aIfInstId);
    if (K2STAT_IS_ERROR(stat))
    {
        //
        // attach failed
        //
        pKnownVol->mpFsProv = NULL;
    }
}

void
FsMgr_Vol_Departed(
    K2OS_IFINST_ID aIfInstId
)
{
    FSMGR_KNOWN_VOLUME *    pKnownVol;
    K2LIST_LINK *           pListLink;
    FSPROV *                pFsProv;

//    K2OSKERN_Debug("FSMGR: Volume with ifinstid %d departed\n", aIfInstId);

    pKnownVol = NULL;

    K2OS_CritSec_Enter(&gFsMgr.Sec);

    pListLink = gFsMgr.KnownVolList.mpHead;
    if (NULL != pListLink)
    {
        do
        {
            pKnownVol = K2_GET_CONTAINER(FSMGR_KNOWN_VOLUME, pListLink, ListLink);
            if (pKnownVol->mIfInstId == aIfInstId)
                break;
            pKnownVol = NULL;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);

        if (NULL != pListLink)
        {
            K2LIST_Remove(&gFsMgr.KnownVolList, &pKnownVol->ListLink);
        }
    }

    K2OS_CritSec_Leave(&gFsMgr.Sec);

    if (NULL == pKnownVol)
        return;

    pFsProv = pKnownVol->mpFsProv;

    if (NULL != pFsProv)
    {
        //
        // shut down file system
        //
        K2OSKERN_Debug("FSMGR: Shutdown FS [%s:%d] on volume ifinstid %d\n", pFsProv->mpKernFsProv->mName, pFsProv->mIndex, pKnownVol->mIfInstId);
        K2_ASSERT(0);
    }
}

void
FsMgr_FsProv_Arrived(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OS_RPC_OBJ_HANDLE     rpcHandle;
    UINT32                  fsCount;
    FSPROV *                pFsProv;
    K2OS_RPC_CALLARGS       args;
    UINT32                  actualOut;
    K2STAT                  stat;
    UINT32                  gotFs;
    UINT32                  ix;
    FSPROV *                pRofsProv;
    K2LIST_LINK *           pListLink;
    FSMGR_KNOWN_VOLUME *    pKnownVol;
    UINT32                  iter;
    BOOL                    ok;
    BOOL                    triedToAttach;

//    K2OSKERN_Debug("FSMGR: FsProv with ifinstid %d arrived\n", aIfInstId);

    rpcHandle = K2OS_Rpc_AttachByIfInstId(aIfInstId, NULL);
    if (NULL == rpcHandle)
    {
        K2OSKERN_Debug("FSMGR: Could not open fsProv with ifInstId %d\n", aIfInstId);
        return;
    }

    gotFs = 0;
    pRofsProv = NULL;

    K2MEM_Zero(&args, sizeof(args));
    args.mMethodId = K2OS_FsProv_Method_GetCount;
    args.mpOutBuf = (void *)&fsCount;
    args.mOutBufByteCount = sizeof(fsCount);
    actualOut = 0;
    stat = K2OS_Rpc_Call(rpcHandle, &args, &actualOut);

    if (!K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(sizeof(fsCount) == actualOut);
        //        K2OSKERN_Debug("FSMGR: fsprov has %d filesystem type(s)\n", fsCount);
        for (ix = 0; ix < fsCount; ix++)
        {
            pFsProv = (FSPROV *)K2OS_Heap_Alloc(sizeof(FSPROV));
            if (NULL != pFsProv)
            {
                K2MEM_Zero(pFsProv, sizeof(FSPROV));
                pFsProv->mFsProvIfInstId = aIfInstId;
                pFsProv->mRpcObjHandle = rpcHandle;
                pFsProv->mIndex = ix;

                args.mMethodId = K2OS_FsProv_Method_GetEntry;
                args.mpInBuf = (void const *)&ix;
                args.mInBufByteCount = sizeof(UINT32);
                args.mpOutBuf = (void *)&pFsProv->mpKernFsProv;
                args.mOutBufByteCount = sizeof(void *);
                actualOut = 0;
                stat = K2OS_Rpc_Call(rpcHandle, &args, &actualOut);
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSKERN_Debug("FSMGR: Erorr 0x%08X retrieving filesystem type index %d from provider with ifinstid %d\n", stat, ix, aIfInstId);
                    K2OS_Heap_Free(pFsProv);
                }
                else
                {
                    gotFs++;
                    //                    K2OSKERN_Debug("FSPROV[provider interface instance %d: index %d: type name \"%s\" arrived]\n", aIfInstId, ix, pFsProv->mpKernFsProv->mName);
                    K2OS_CritSec_Enter(&gFsMgr.Sec);
                    K2LIST_AddAtTail(&gFsMgr.FsProvList, &pFsProv->ListLink);
                    K2OS_CritSec_Leave(&gFsMgr.Sec);
                    if (pFsProv->mpKernFsProv == gpRofsProv)
                    {
                        pRofsProv = pFsProv;
                    }
                }
            }
        }
    }

    if (0 == gotFs)
    {
        K2OS_Rpc_Release(rpcHandle);
    }
    else
    {
        K2_ASSERT(NULL != pFsProv);

        //
        // provider registered at least one filesystem type
        //

        // special case code to attach to rofs provider at startup
        if (NULL != pRofsProv)
        {
            FsMgr_DoFsProv_Attach(pRofsProv, pRofsProv->mpKernFsProv);
        }

        if (0 != (pFsProv->mpKernFsProv->mFlags & K2OSKERN_FSPROV_FLAG_USE_VOLUME_IFINSTID))
        {
            K2OS_CritSec_Enter(&gFsMgr.Sec);

            pListLink = gFsMgr.KnownVolList.mpHead;
            if (NULL != pListLink)
            {
                //
                // set iteration
                //
                iter = gFsMgr.mKnownIter;
                do
                {
                    pKnownVol = K2_GET_CONTAINER(FSMGR_KNOWN_VOLUME, pListLink, ListLink);
                    pListLink = pListLink->mpNext;
                    if (NULL == pKnownVol->mpFsProv)
                    {
                        pKnownVol->mIter = iter;
                    }
                } while (NULL != pListLink);
                iter = ++gFsMgr.mKnownIter;

                //
                // now for any known volume without a file system 
                // that is not in this new iteration,
                // see if we can probe to attach this filesystem
                //
                do
                {
                    triedToAttach = FALSE;

                    pListLink = gFsMgr.KnownVolList.mpHead;
                    do
                    {
                        pKnownVol = K2_GET_CONTAINER(FSMGR_KNOWN_VOLUME, pListLink, ListLink);
                        pListLink = pListLink->mpNext;

                        if (NULL == pKnownVol->mpFsProv)
                        {
                            if (pKnownVol->mIter != iter)
                            {
                                pKnownVol->mIter = iter;

                                ok = FALSE;
                                stat = pFsProv->mpKernFsProv->Ops.Probe(pFsProv->mpKernFsProv, (void *)pKnownVol->mIfInstId, &ok);
                                if ((!K2STAT_IS_ERROR(stat)) && (ok))
                                {
                                    // provisional - revoked below on failure
                                    pKnownVol->mpFsProv = pFsProv;
                                    break;
                                }
                            }
                        }
                        pKnownVol = NULL;


                    } while (NULL != pListLink);

                    if (NULL != pKnownVol)
                    {
                        triedToAttach = TRUE;

                        K2OS_CritSec_Leave(&gFsMgr.Sec);

                        stat = FsMgr_DoFsProv_Attach(pFsProv, (void *)pKnownVol->mIfInstId);
                        if (K2STAT_IS_ERROR(stat))
                        {
                            pKnownVol->mpFsProv = NULL;
                        }

                        K2OS_CritSec_Enter(&gFsMgr.Sec);
                    }

                } while (triedToAttach);
            } 

            K2OS_CritSec_Leave(&gFsMgr.Sec);
        }
    }
}

void 
FsMgr_FsProv_Departed(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OSKERN_Debug("FSMGR: FsProv with ifinstid %d departed\n", aIfInstId);
    K2_ASSERT(0);
}

void
FsMgr_Recv_Notify(
    UINT32  aPayload0,
    UINT32  aPayload1,
    UINT32  aPayload2
)
{
    K2OSKERN_Debug("FSMGR: Recv Notify(%08X,%08X,%08X)\n", aPayload0, aPayload1, aPayload2);
    K2_ASSERT(0);
}

#define FSMGR_VOLPOP_SUBSCRIPTION       1
#define FSMGR_FSPROVPOP_SUBSCRIPTION    2

UINT32
FsMgr_Thread(
    void *apArg
)
{
    static K2_GUID128 const sVolIfaceId = K2OS_IFACE_STORAGE_VOLUME;
    static K2_GUID128 const sFsProvIfaceId = K2OS_IFACE_FSPROV;
    K2OS_IFSUBS_TOKEN   tokVolSubs;
    K2OS_IFSUBS_TOKEN   tokFsProvSubs;
    K2OS_MSG            msg;
    K2OS_WaitResult     waitResult;

    gFsMgr.mTokMailbox = K2OS_Mailbox_Create();
    if (NULL == gFsMgr.mTokMailbox)
    {
        K2OSKERN_Panic("***FsMgr thread coult not create a mailbox\n");
    }

    tokVolSubs = K2OS_IfSubs_Create(gFsMgr.mTokMailbox, K2OS_IFACE_CLASSCODE_STORAGE_VOLUME, &sVolIfaceId, 12, FALSE, (void *)FSMGR_VOLPOP_SUBSCRIPTION);
    if (NULL == tokVolSubs)
    {
        K2OSKERN_Panic("***FsMgr thread coult not subscribe to volume interface pop changes\n");
    }

    tokFsProvSubs = K2OS_IfSubs_Create(gFsMgr.mTokMailbox, K2OS_IFACE_CLASSCODE_FSPROV, &sFsProvIfaceId, 12, FALSE, (void *)FSMGR_FSPROVPOP_SUBSCRIPTION);
    if (NULL == tokFsProvSubs)
    {
        K2OSKERN_Panic("***FsMgr thread coult not subscribe to volume interface pop changes\n");
    }

    //
    // notify we have started and main thread can continue
    //
    K2OS_Gate_Open((K2OS_SIGNAL_TOKEN)apArg);

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, gFsMgr.mTokMailbox, K2OS_TIMEOUT_INFINITE))
            break;
        K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);
        if (K2OS_Mailbox_Recv(gFsMgr.mTokMailbox, &msg))
        {
            if (msg.mMsgType == K2OS_SYSTEM_MSGTYPE_IFINST)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    // payload[0] = subscription context
                    // payload[1] = interface instance id
                    // payload[2] = process that published the interface
                    if (msg.mPayload[0] == FSMGR_VOLPOP_SUBSCRIPTION)
                    {
                        FsMgr_Vol_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                    }
                    else
                    {
                        K2_ASSERT(msg.mPayload[0] == FSMGR_FSPROVPOP_SUBSCRIPTION);
                        // file system providers must be in the kernel
                        if (0 == msg.mPayload[2])
                        {
                            FsMgr_FsProv_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                        }
                    }
                }
                else if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_DEPART)
                {
                    if (msg.mPayload[0] == FSMGR_VOLPOP_SUBSCRIPTION)
                    {
                        FsMgr_Vol_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                    }
                    else
                    {
                        K2_ASSERT(msg.mPayload[0] == FSMGR_FSPROVPOP_SUBSCRIPTION);
                        // file system providers must be in the kernel
                        if (0 == msg.mPayload[2])
                        {
                            FsMgr_FsProv_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                        }
                    }
                }
                else
                {
                    K2OSKERN_Debug("*** FsMgr received unexpected IFINST message (%04X)\n", msg.mShort);
                }
            }
            else if (msg.mMsgType == K2OS_SYSTEM_MSGTYPE_RPC)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_RPC_SHORT_NOTIFY)
                {
                    FsMgr_Recv_Notify(msg.mPayload[0], msg.mPayload[1], msg.mPayload[2]);
                }
                else
                {
                    K2OSKERN_Debug("*** FsMgr received unexpected RPC message (%04X)\n", msg.mShort);
                }
            }
            else
            {
                K2OSKERN_Debug("*** SysProc FsMgr received unexpected message type (%04X)\n", msg.mMsgType);
            }
        }

    } while (1);

    return 0;
}

K2STAT
K2OSEXEC_FsMgrRpc_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    K2OS_SIGNAL_TOKEN   tokStartGate;
    K2OS_WaitResult     waitResult;
    K2OS_THREAD_TOKEN   tokThread;

    //
    // only the kernel can create the single fsmgr object
    //
    if ((apCreate->mCreatorProcessId != 0) ||
        (apCreate->mCreatorContext != (UINT32)&gFsMgr))
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    *apRetContext = 0;

    gFsMgr.mRpcObj = aObject;

    if (!K2OS_CritSec_Init(&gFsMgr.Sec))
    {
        K2OSKERN_Panic("FSMGR: Could not create cs for fs list\n");
    }
    K2LIST_Init(&gFsMgr.FsProvList);
    K2LIST_Init(&gFsMgr.UserList);
    K2LIST_Init(&gFsMgr.FsList);
    K2LIST_Init(&gFsMgr.KnownVolList);

    tokStartGate = K2OS_Gate_Create(FALSE);
    if (NULL == tokStartGate)
    {
        K2OSKERN_Panic("FSMGR: could not create volmgr start gate\n");
    }

    tokThread = K2OS_Thread_Create("FileSys Manager", FsMgr_Thread, tokStartGate, NULL, &gFsMgr.mThreadId);
    if (NULL == tokThread)
    {
        K2OSKERN_Panic("FSMGR: Could not start fs manager thread\n");
    }

    if (!K2OS_Thread_WaitOne(&waitResult, tokStartGate, K2OS_TIMEOUT_INFINITE))
    {
        K2OSKERN_Panic("FSMGR: Failed to wait for startup gate\n");
    }

    K2OS_Token_Destroy(tokThread);

    K2OS_Token_Destroy(tokStartGate);

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_FsMgrRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    FSMGR_USER *    pUser;
    K2STAT          stat;

    K2_ASSERT(aObject == gFsMgr.mRpcObj);

    pUser = (FSMGR_USER *)K2OS_Heap_Alloc(sizeof(FSMGR_USER));
    if (NULL == pUser)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pUser, sizeof(FSMGR_USER));

    pUser->mProcId = aProcessId;

    K2OS_CritSec_Enter(&gFsMgr.Sec);

    K2LIST_AddAtTail(&gFsMgr.UserList, &pUser->FsMgrUserListLink);

    K2OS_CritSec_Leave(&gFsMgr.Sec);

    *apRetUseContext = (UINT32)pUser;

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_FsMgrRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    FSMGR_USER *    pUser;

    pUser = (FSMGR_USER *)aUseContext;

    K2OS_CritSec_Enter(&gFsMgr.Sec);

    K2LIST_Remove(&gFsMgr.UserList, &pUser->FsMgrUserListLink);

    K2OS_CritSec_Leave(&gFsMgr.Sec);

    K2OS_Heap_Free(pUser);

    return K2STAT_NO_ERROR;
}

K2STAT
FsMgr_Format_Volume(
    K2OS_FSMGR_FORMAT_VOLUME_IN const * apIn
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
FsMgr_Clean_Volume(
    K2OS_FSMGR_CLEAN_VOLUME_IN const * apIn
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
FsMgr_Mount(
    UINT32                      aUseContext,
    K2OS_FSMGR_MOUNT_IN const * apIn,
    UINT32                      aInByteCount
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
FsMgr_Dismount(
    K2OS_FSMGR_DISMOUNT_IN const *  apIn,
    UINT32                          aInByteCount
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
K2OSEXEC_FsMgrRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    K2STAT stat;

    stat = K2STAT_ERROR_NOT_IMPL;

    switch (apCall->Args.mMethodId)
    {
    case K2OS_FsMgr_Method_FormatVolume:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            (sizeof(K2OS_FSMGR_FORMAT_VOLUME_IN) > apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = FsMgr_Format_Volume((K2OS_FSMGR_FORMAT_VOLUME_IN const *)apCall->Args.mpInBuf);
        }
        break;

    case K2OS_FsMgr_Method_CleanVolume:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            (sizeof(K2OS_FSMGR_CLEAN_VOLUME_IN) > apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = FsMgr_Clean_Volume((K2OS_FSMGR_CLEAN_VOLUME_IN const *)apCall->Args.mpInBuf);
        }
        break;

    case K2OS_FsMgr_Method_Mount:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            ((sizeof(K2OS_FSMGR_MOUNT_IN) - 2) > apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = FsMgr_Mount(apCall->mUseContext, (K2OS_FSMGR_MOUNT_IN const *)apCall->Args.mpInBuf, apCall->Args.mInBufByteCount);
        }
        break;

    case K2OS_FsMgr_Method_Dismount:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            ((sizeof(K2OS_FSMGR_DISMOUNT_IN) - 2) > apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = FsMgr_Dismount((K2OS_FSMGR_DISMOUNT_IN const *)apCall->Args.mpInBuf, apCall->Args.mInBufByteCount);
        }
        break;

    default:
        break;
    }

    return stat;
}

K2STAT
K2OSEXEC_FsMgrRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    K2OSKERN_Panic("FsMgr obj delete - should be impossible.\n");
    return K2STAT_ERROR_UNKNOWN;
}

void
FsMgr_Init(
    K2OSKERN_FSNODE *       apRootFsNode,
    K2OSKERN_FSNODE *       apFsRootFsNode,
    K2OSKERN_pf_FsNodeInit  afFsNodeInit
)
{
    // {F082B3AC-E770-4D69-942A-AE46B55F6946}
    static const K2OS_RPC_OBJ_CLASSDEF sFsMgrClassDef =
    {
        { 0xf082b3ac, 0xe770, 0x4d69, { 0x94, 0x2a, 0xae, 0x46, 0xb5, 0x5f, 0x69, 0x46 } },
        K2OSEXEC_FsMgrRpc_Create,
        K2OSEXEC_FsMgrRpc_OnAttach,
        K2OSEXEC_FsMgrRpc_OnDetach,
        K2OSEXEC_FsMgrRpc_Call,
        K2OSEXEC_FsMgrRpc_Delete
    };
    static const K2_GUID128 sFsMgrIfaceId = K2OS_IFACE_FSMGR;

    K2OS_RPC_CLASS      fsMgrClass;
    K2OS_RPC_CLASS      fsClientClass;
    K2OS_RPC_CLASS      fsFileUseClass;
    K2OS_RPC_CLASS      fsFileSysClass;
    K2OS_RPC_OBJ_HANDLE hRpcObj;
    BOOL                ok;

    K2MEM_Zero(&gFsMgr, sizeof(gFsMgr));
    gFsMgr.mpRootFsNode = apRootFsNode;
    gFsMgr.mpFsRootFsNode = apFsRootFsNode;
    gFsMgr.mfFsNodeInit = afFsNodeInit;

    fsMgrClass = K2OS_RpcServer_Register(&sFsMgrClassDef, 0);
    if (NULL == fsMgrClass)
    {
        K2OSKERN_Panic("FSMGR: Could not register filesys manager object class\n");
    }

    hRpcObj = K2OS_Rpc_CreateObj(0, &sFsMgrClassDef.ClassId, (UINT32)&gFsMgr);
    if (NULL == hRpcObj)
    {
        K2OSKERN_Panic("FSMGR: failed to create filesys manager object(%08X)\n", K2OS_Thread_GetLastStatus());
    }

    gFsMgr.mRpcIfInst = K2OS_RpcObj_AddIfInst(
        gFsMgr.mRpcObj,
        K2OS_IFACE_CLASSCODE_FSMGR,
        &sFsMgrIfaceId,
        &gFsMgr.mIfInstId,
        TRUE
    );
    if (NULL == gFsMgr.mRpcIfInst)
    {
        K2OSKERN_Panic("FSMGR: Could not publish filesys manager ifinst\n");
    }

    K2TREE_Init(&gFsClientTree, NULL);
    K2TREE_Init(&gFsNodeMapTree, NULL);
    K2OSKERN_SeqInit(&gFsNodeMapTreeSeqLock);

    fsClientClass = K2OS_RpcServer_Register(&sFsClientClassDef, 1);
    if (NULL == fsClientClass)
    {
        K2OSKERN_Panic("FSMGR: Could not register filesys client object class\n");
    }

    fsFileUseClass = K2OS_RpcServer_Register(&sFsFileUseClassDef, 2);
    if (NULL == fsFileUseClass)
    {
        K2OSKERN_Panic("FSMGR: Could not register filesys file object class\n");
    }

    gFsMgr.mpKernFileForRoot = (K2OSKERN_FILE *)K2OS_Heap_Alloc(sizeof(K2OSKERN_FILE));
    K2_ASSERT(NULL != gFsMgr.mpKernFileForRoot);
    gFsMgr.mpKernFileForRoot->mRefCount = 1;
    ok = K2OS_CritSec_Init(&gFsMgr.mpKernFileForRoot->Sec);
    K2_ASSERT(ok);
    K2LIST_Init(&gFsMgr.mpKernFileForRoot->Locked.UseList);
    gFsMgr.mpKernFileForRoot->Static.mAccess = K2OS_ACCESS_RW;
    gFsMgr.mpKernFileForRoot->Static.mShare = K2OS_ACCESS_RW;
    gFsMgr.mpKernFileForRoot->MapTreeNode.mUserVal = (UINT_PTR)gFsMgr.mpRootFsNode;
    K2TREE_Insert(&gFsNodeMapTree, (UINT_PTR)gFsMgr.mpRootFsNode, &gFsMgr.mpKernFileForRoot->MapTreeNode);

    fsFileSysClass = K2OS_RpcServer_Register(&sFileSysClassDef, 0);
    if (NULL == fsFileSysClass)
    {
        K2OSKERN_Panic("FSMGR: Could not register filesys object class\n");
    }

}

