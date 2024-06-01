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
#include <lib/k2fat.h>

typedef struct _FSNODE      FSNODE;
typedef struct _FS          FS;
typedef struct _FSMGR_USER  FSMGR_USER;
typedef struct _FSMGR       FSMGR;

struct _FSNODE
{
    FSNODE *        mpParent;
    K2TREE_ANCHOR   ChildTree;
    K2TREE_NODE     TreeNode;
    UINT32          mNameLen;
    FS *            mpFS;
    K2OS_FILE_INFO  Info;  // must be last thing in structure
};

struct _FS
{
    K2OS_IFINST_ID      mVolIfInstId;

    K2OS_XDL            mXdl;
    K2OS_pf_Ifs_Start   mfStart;
    K2OS_pf_Ifs_Stop    mfStop;

    K2LIST_LINK         ListLink;
};

struct _FSMGR_USER
{
    UINT32          mProcId;
    K2LIST_LINK     UserListLink;
};

struct _FSMGR
{
    K2OS_RPC_OBJ        mRpcObj;

    K2OS_THREAD_TOKEN   mTokThread;
    UINT32              mThreadId;

    K2OS_MAILBOX_TOKEN  mTokMailbox;
    K2OS_RPC_IFINST     mRpcIfInst;
    K2OS_IFINST_ID      mIfInstId;

    K2OS_CRITSEC        Sec;

    FSNODE              Root;

    K2LIST_ANCHOR       FsList;
    K2LIST_ANCHOR       UserList;
};
static FSMGR sgFsMgr;

void
FsMgr_Vol_Arrived(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OSSTOR_VOLUME     storVol;
    UINT8 *             pSecBuffer;
    UINT8 *             pSector;
    K2OS_STORAGE_VOLUME volInfo;
    K2STAT              stat;
    K2FAT_PART          fatPart;
    UINT64              volOffset;
    BOOL                startFat;
    K2OS_XDL            fatXdl;
    K2OS_pf_Ifs_Start   fsStart;
    K2OS_pf_Ifs_Stop    fsStop;
    FS *                pNewFs;

    storVol = K2OS_Vol_Attach(aIfInstId, K2OS_ACCESS_R, K2OS_ACCESS_RW, NULL);
    if (NULL == storVol)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OSKERN_Debug("FSMGR: Could not attach for readonly to volume %d\n", aIfInstId);
        return;
    }

    do {
        if (!K2OS_Vol_GetInfo(storVol, &volInfo))
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            K2OSKERN_Debug("FSMGR: Could not get info for volume %d\n", aIfInstId);
            break;
        }

        K2_ASSERT(volInfo.mBlockSizeBytes != 0);
        K2_ASSERT(volInfo.mBlockCount != 0);

        pSecBuffer = (UINT8 *)K2OS_Heap_Alloc(volInfo.mBlockSizeBytes * 2);
        if (NULL == pSecBuffer)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            K2OSKERN_Debug("FSMGR: Out of memory allocating sector buffer for volume %d\n", aIfInstId);
            break;
        }

        do {
            pSector = (UINT8 *)(((((UINT32)pSecBuffer) + (volInfo.mBlockSizeBytes - 1)) / volInfo.mBlockSizeBytes) * volInfo.mBlockSizeBytes);
            volOffset = 0;
            if (!K2OS_Vol_Read(storVol, &volOffset, pSector, volInfo.mBlockSizeBytes))
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OSKERN_Debug("FSMGR: Failed to read sector 0 of voluem %d, error %08X\n", aIfInstId, stat);
                break;
            }

            //
            // determine if this is a supported filesystem from sector 0 contents
            //
            if ((pSector[510] == 0x55) &&
                (pSector[511] == 0xAA) &&
                (0 == K2ASC_CompInsLen((char const *)(pSector + 3), "MSDOS", 5)))
            {
                stat = K2FAT_DetermineFromBootSector((FAT_GENERIC_BOOTSECTOR const *)pSector, &fatPart);
                if (!K2STAT_IS_ERROR(stat))
                {
                    if (fatPart.mBytesPerSector != volInfo.mBlockSizeBytes)
                    {
                        K2OSKERN_Debug(
                            "FSMGR: FAT on volume %d bytesPerSector(%d) does not match volume blockSizeBytes(%d)\n",
                            aIfInstId, 
                            fatPart.mBytesPerSector,
                            volInfo.mBlockSizeBytes
                        );
                    }
                    else
                    {
                        startFat = TRUE;
                        if (fatPart.mFATType == K2FAT_Type12)
                        {
                            K2OSKERN_Debug("FSMGR: Vol %d is FAT12\n", aIfInstId);
                        }
                        else if (fatPart.mFATType == K2FAT_Type16)
                        {
                            K2OSKERN_Debug("FSMGR: Vol %d is FAT16\n", aIfInstId);
                        }
                        else if (fatPart.mFATType == K2FAT_Type32)
                        {
                            K2OSKERN_Debug("FSMGR: Vol %d is FAT32\n", aIfInstId);
                        }
                        else
                        {
                            K2OSKERN_Debug("FSMGR: Fat type on volume %d is unknown\n", aIfInstId);
                            startFat = FALSE;
                        }

                        if (startFat)
                        {
                            K2OSKERN_Debug("FSMGR: starting FAT on volume %d\n", aIfInstId);
                            // ...
                            fatXdl = K2OS_Xdl_Acquire(":fatfs.xdl");
                            if (NULL == fatXdl)
                            {
                                K2OSKERN_Debug("FSMGR: Failed to start FAT on volume %d\n", aIfInstId);
                            }
                            else
                            {
                                fsStart = (K2OS_pf_Ifs_Start)K2OS_Xdl_FindExport(fatXdl, TRUE, "StartFs");
                                if (NULL != fsStart)
                                {
                                    fsStop = (K2OS_pf_Ifs_Stop)K2OS_Xdl_FindExport(fatXdl, TRUE, "StopFs");
                                }
                                if ((NULL != fsStart) && (NULL != fsStop))
                                {
                                    pNewFs = (FS *)K2OS_Heap_Alloc(sizeof(FS));
                                    if (NULL == pNewFs)
                                    {
                                        stat = K2OS_Thread_GetLastStatus();
                                        K2_ASSERT(K2STAT_IS_ERROR(stat));
                                    }
                                    else
                                    {
                                        K2MEM_Zero(pNewFs, sizeof(FS));
                                        pNewFs->mXdl = fatXdl;
                                        pNewFs->mfStart = fsStart;
                                        pNewFs->mfStop = fsStop;
                                        pNewFs->mVolIfInstId = aIfInstId;
                                        K2OS_CritSec_Enter(&sgFsMgr.Sec);
                                        K2LIST_AddAtTail(&sgFsMgr.FsList, &pNewFs->ListLink);
                                        K2OS_CritSec_Leave(&sgFsMgr.Sec);

                                        // detach so fs can have exclusive access
                                        K2OS_Vol_Detach(storVol);
                                        storVol = NULL;

                                        stat = fsStart(aIfInstId, &volInfo, pSector);
                                        if (K2STAT_IS_ERROR(stat))
                                        {
                                            K2OSKERN_Debug("FSMGR: Error 0x%08X starting filesystem\n", stat);
                                            K2OS_CritSec_Enter(&sgFsMgr.Sec);
                                            K2LIST_Remove(&sgFsMgr.FsList, &pNewFs->ListLink);
                                            K2OS_CritSec_Leave(&sgFsMgr.Sec);
                                            K2OS_Heap_Free(pNewFs);
                                        }
                                    }
                                }
                                else
                                {
                                    K2OSKERN_Debug("FSMGR: FAT filesystem XDL is malformed\n");
                                    stat = K2STAT_ERROR_BAD_FORMAT;
                                }
                                if (K2STAT_IS_ERROR(stat))
                                {
                                    K2OS_Xdl_Release(fatXdl);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                K2OSKERN_Debug("FSMGR: Volume %d has unknown filesystem\n", aIfInstId);
            }

        } while (0);

        K2OS_Heap_Free(pSecBuffer);

    } while (0);

    if (NULL != storVol)
    {
        K2OS_Vol_Detach(storVol);
    }
}

void
FsMgr_Vol_Departed(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OSKERN_Debug("FSMGR: Volume with ifinstid %d departed\n", aIfInstId);
}

void
FsMgr_FileSys_Arrived(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OSKERN_Debug("FSMGR: FileSys with ifinstid %d arrived\n", aIfInstId);
}

void
FsMgr_FileSys_Departed(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OSKERN_Debug("FSMGR: FileSys with ifinstid %d departed\n", aIfInstId);
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

UINT32
FsMgr_Thread(
    void *apArg
)
{
    static K2_GUID128 const sVolIfaceId = K2OS_IFACE_STORAGE_VOLUME;
    static K2_GUID128 const sFileSysIfaceId = K2OS_IFACE_FILESYS;
    K2OS_IFSUBS_TOKEN   tokVolSubs;
    K2OS_IFSUBS_TOKEN   tokFsSubs;
    K2OS_MSG            msg;
    K2OS_WaitResult     waitResult;

    sgFsMgr.mTokMailbox = K2OS_Mailbox_Create();
    if (NULL == sgFsMgr.mTokMailbox)
    {
        K2OSKERN_Panic("***FsMgr thread coult not create a mailbox\n");
    }

    tokVolSubs = K2OS_IfSubs_Create(sgFsMgr.mTokMailbox, K2OS_IFACE_CLASSCODE_STORAGE_VOLUME, &sVolIfaceId, 12, FALSE, (void *)1);
    if (NULL == tokVolSubs)
    {
        K2OSKERN_Panic("***FsMgr thread coult not subscribe to volume interface pop changes\n");
    }

    tokFsSubs = K2OS_IfSubs_Create(sgFsMgr.mTokMailbox, K2OS_IFACE_CLASSCODE_FILESYS, &sFileSysIfaceId, 12, FALSE, (void *)2);
    if (NULL == tokFsSubs)
    {
        K2OSKERN_Panic("***FsMgr thread coult not subscribe to filesys interface pop changes\n");
    }

    //
    // notify we have started and main thread can continue
    //
    K2OS_Gate_Open((K2OS_SIGNAL_TOKEN)apArg);

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, sgFsMgr.mTokMailbox, K2OS_TIMEOUT_INFINITE))
            break;
        K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);
        if (K2OS_Mailbox_Recv(sgFsMgr.mTokMailbox, &msg, 0))
        {
            if (msg.mType == K2OS_SYSTEM_MSGTYPE_IFINST)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    // payload[0] = subscription context
                    // payload[1] = interface instance id
                    // payload[2] = process that published the interface
                    if (msg.mPayload[0] == 1)
                    {
                        FsMgr_Vol_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                    }
                    else if (msg.mPayload[0] == 2)
                    {
                        FsMgr_FileSys_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                    }
                }
                else if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    if (msg.mPayload[0] == 1)
                    {
                        FsMgr_Vol_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                    }
                    else if (msg.mPayload[0] == 2)
                    {
                        FsMgr_FileSys_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                    }
                }
                else
                {
                    K2OSKERN_Debug("*** FsMgr received unexpected IFINST message (%04X)\n", msg.mShort);
                }
            }
            else if (msg.mType == K2OS_SYSTEM_MSGTYPE_RPC)
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
                K2OSKERN_Debug("*** SysProc FsMgr received unexpected message type (%04X)\n", msg.mType);
            }
        }

    } while (1);

    return 0;
}

int
FsMgrTree_Compare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
)
{
    FSNODE *    pNode;
    pNode = K2_GET_CONTAINER(FSNODE, apNode, TreeNode);

    return K2ASC_CompIns((char const *)aKey, pNode->Info.mName);
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

    if ((apCreate->mCreatorProcessId != 0) ||
        (apCreate->mCreatorContext != (UINT32)&sgFsMgr))
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    *apRetContext = 0;

    sgFsMgr.mRpcObj = aObject;
    
    if (!K2OS_CritSec_Init(&sgFsMgr.Sec))
    {
        K2OSKERN_Panic("FSMGR: Could not create cs for fs list\n");
    }
    K2LIST_Init(&sgFsMgr.FsList);
    K2TREE_Init(&sgFsMgr.Root.ChildTree, FsMgrTree_Compare);
    sgFsMgr.Root.Info.mName[0] = '/';
    sgFsMgr.Root.Info.mName[1] = 0;
    sgFsMgr.Root.Info.mAttrib = K2OS_FATTRIB_FOLDER | K2OS_FATTRIB_SYSTEM;
    sgFsMgr.Root.mNameLen = 1;

    tokStartGate = K2OS_Gate_Create(FALSE);
    if (NULL == tokStartGate)
    {
        K2OSKERN_Panic("FSMGR: could not create volmgr start gate\n");
    }

    tokThread = K2OS_Thread_Create("FileSys Manager", FsMgr_Thread, tokStartGate, NULL, &sgFsMgr.mThreadId);
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

    K2_ASSERT(aObject == sgFsMgr.mRpcObj);

    pUser = (FSMGR_USER *)K2OS_Heap_Alloc(sizeof(FSMGR_USER));
    if (NULL == pUser)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pUser, sizeof(FSMGR_USER));
    pUser->mProcId = aProcessId;

    K2OS_CritSec_Enter(&sgFsMgr.Sec);

    K2LIST_AddAtTail(&sgFsMgr.UserList, &pUser->UserListLink);

    K2OS_CritSec_Leave(&sgFsMgr.Sec);

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
    FSMGR_USER * pUser;

    pUser = (FSMGR_USER *)aUseContext;

    K2OS_CritSec_Enter(&sgFsMgr.Sec);

    K2LIST_Remove(&sgFsMgr.UserList, &pUser->UserListLink);

    K2OS_CritSec_Leave(&sgFsMgr.Sec);

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
    case K2OS_FSMGR_METHOD_FORMAT_VOLUME:
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

    case K2OS_FSMGR_METHOD_CLEAN_VOLUME:
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

    case K2OS_FSMGR_METHOD_MOUNT:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            ((sizeof(K2OS_FSMGR_MOUNT_IN)-2) > apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = FsMgr_Mount((K2OS_FSMGR_MOUNT_IN const *)apCall->Args.mpInBuf, apCall->Args.mInBufByteCount);
        }
        break;

    case K2OS_FSMGR_METHOD_DISMOUNT:
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
    void
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
    K2OS_RPC_OBJ_HANDLE hRpcObj;

    K2MEM_Zero(&sgFsMgr, sizeof(sgFsMgr));

    fsMgrClass = K2OS_RpcServer_Register(&sFsMgrClassDef, 0);
    if (NULL == fsMgrClass)
    {
        K2OSKERN_Panic("FSMGR: Could not register filesys manager object class\n");
    }

    hRpcObj = K2OS_Rpc_CreateObj(0, &sFsMgrClassDef.ClassId, (UINT32)&sgFsMgr);
    if (NULL == hRpcObj)
    {
        K2OSKERN_Panic("FSMGR: failed to create filesys manager object(%08X)\n", K2OS_Thread_GetLastStatus());
    }

    sgFsMgr.mRpcIfInst = K2OS_RpcObj_AddIfInst(
        sgFsMgr.mRpcObj,
        K2OS_IFACE_CLASSCODE_FSMGR,
        &sFsMgrIfaceId,
        &sgFsMgr.mIfInstId,
        TRUE
    );
    if (NULL == sgFsMgr.mRpcIfInst)
    {
        K2OSKERN_Panic("FSMGR: Could not publish filesys manager ifinst\n");
    }
}

