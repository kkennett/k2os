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

typedef struct _VOLMGR VOLMGR;
struct _VOLMGR
{
    K2OS_RPC_OBJ        mRpcObj;

    K2OS_THREAD_TOKEN   mTokThread;
    UINT32              mThreadId;

    K2OS_RPC_CLASS      mVolClass;

    K2OS_MAILBOX_TOKEN  mTokMailbox;
    K2OS_RPC_IFINST     mRpcIfInst;
    K2OS_IFINST_ID      mIfInstId;

    K2OS_CRITSEC        VolListSec;
    K2LIST_ANCHOR       VolList;

    K2OS_CRITSEC        PartListSec;
    K2LIST_ANCHOR       PartList;
};
static VOLMGR sgVolMgr;

typedef struct _VOLPART VOLPART;
struct _VOLPART
{
    K2LIST_LINK             PartListLink;

    K2OS_RPC_OBJ_HANDLE     mRpcHandle;
    K2OS_IFINST_ID          mIfInstId;
    
    K2OS_STORAGE_PARTITION  StorPart;
    UINT32                  mOffset;

    K2OS_IFINST_ID          mDevIfInstId;
    K2OSSTOR_BLOCKIO        mDevBlockIo;
    K2OSSTOR_BLOCKIO_RANGE  mDevRange;
    BLOCKIO *               mpBlockIo;
};

typedef struct _VOL VOL;
struct _VOL
{
    INT32 volatile          mRefCount;
    K2RUNDOWN               Rundown;

    K2OS_RPC_OBJ            mRpcObj;
    K2OS_RPC_OBJ_HANDLE     mRpcObjHandle;
    K2OS_RPC_IFINST         mIfInst;
    K2OS_IFINST_ID          mIfInstId;

    K2OS_STORAGE_VOLUME     StorVol;
    VOLPART **              mppParts;

    K2LIST_LINK             VolListLink;

    K2OS_CRITSEC            Sec;

    BOOL                    mIsMade;

    UINT32                  mShare;

    K2LIST_ANCHOR           UserList;
};

typedef struct _VOLUSER VOLUSER;
struct _VOLUSER
{
    UINT32          mProcId;
    BOOL            mIsMaster;
    BOOL            mConfigSet;
    UINT32          mAccess;
    UINT32          mShare;
    K2LIST_LINK     UserListLink;
};

K2STAT
VolRpc_Config(
    VOL *                           apVol,
    VOLUSER *                       apUser,
    K2OS_STORVOL_CONFIG_IN const *  apConfigIn
)
{
    K2STAT stat;

    K2OS_CritSec_Enter(&apVol->Sec);

    if (apUser->mConfigSet)
    {
        stat = K2STAT_ERROR_CONFIGURED;
    }
    else
    {
        if (apUser->mIsMaster)
        {
            apUser->mConfigSet = TRUE;
            apUser->mAccess = apConfigIn->mAccess;
            stat = K2STAT_NO_ERROR;
        }
        else
        {
            //
            // one or more non-master users
            //
            do {
                if (apVol->mShare != (UINT32)-1)
                {
                    // sharing settings have been set 
                    if (apConfigIn->mAccess & K2OS_ACCESS_R)
                    {
                        if (0 == (apVol->mShare & K2OS_ACCESS_R))
                        {
                            stat = K2STAT_ERROR_NOT_SHARED;
                            break;
                        }
                    }

                    if (apConfigIn->mAccess & K2OS_ACCESS_W)
                    {
                        if (0 == (apVol->mShare & K2OS_ACCESS_W))
                        {
                            stat = K2STAT_ERROR_NOT_SHARED;
                            break;
                        }
                    }
                }
                else
                {
                    apVol->mShare = apUser->mShare = apConfigIn->mShare;
                    stat = K2STAT_NO_ERROR;
                }

                apUser->mAccess = apConfigIn->mAccess;

                apUser->mConfigSet = TRUE;
                
            } while (0);
        }
    }

    K2OS_CritSec_Leave(&apVol->Sec);

    return stat;
}

K2STAT
VolRpc_AddPart(
    VOL *                           apVol,
    VOLUSER *                       apUser,
    K2OSSTOR_VOLUME_PART const *    apVolPart
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
VolRpc_RemPart(
    VOL *                           apVol,
    VOLUSER *                       apUser,
    K2OS_STORVOL_REMPART_IN const * apRemPartIn
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
VolRpc_GetPart(
    VOL *                           apVol,
    VOLUSER *                       apUser,
    K2OS_STORVOL_GETPART_IN const * apGetPartIn,
    K2OSSTOR_VOLUME_PART *          apOutPart
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
Vol_Locked_Make(
    VOL *   apVol
)
{
    static K2_GUID128 sVolIfaceClassId = K2OS_IFACE_STORAGE_VOLUME;

    K2STAT      stat;
    UINT32      ixPart;
    VOLPART *   pVolPart;
    UINT32      accessFlags;

    if (0 == apVol->StorVol.mPartitionCount)
    {
        return K2STAT_ERROR_EMPTY;
    }

    stat = K2STAT_NO_ERROR;

    for (ixPart = 0; ixPart < apVol->StorVol.mPartitionCount; ixPart++)
    {
        pVolPart = apVol->mppParts[ixPart];
        K2_ASSERT(NULL == pVolPart->mDevBlockIo);

        accessFlags = K2OS_ACCESS_R;
        if (!pVolPart->StorPart.mFlagReadOnly)
        {
            accessFlags |= K2OS_ACCESS_W;
        }

        pVolPart->mDevBlockIo = K2OS_BlockIo_Attach(
            pVolPart->mDevIfInstId, 
            accessFlags, 
            K2OS_ACCESS_RW, 
            sgVolMgr.mTokMailbox
        );
        if (NULL == pVolPart->mDevBlockIo)
        {
            K2OSKERN_Debug("VOLMGR: Could not attach to partition parent blockio device\n");
            stat = K2OS_Thread_GetLastStatus();
            break;
        }

        pVolPart->mDevRange = K2OS_BlockIo_RangeCreate(
            pVolPart->mDevBlockIo,
            &pVolPart->StorPart.mStartBlock,
            &pVolPart->StorPart.mBlockCount,
            TRUE
        );
        if (NULL == pVolPart->mDevRange)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2OSKERN_Debug("VOLMGR: Could not define private range for partition (%08X)\n", stat);
            K2OS_BlockIo_Detach(pVolPart->mDevBlockIo);
            pVolPart->mDevBlockIo = NULL;
            break;
        }
        else
        {
            pVolPart->mpBlockIo = BlockIo_AcquireByIfInstId(pVolPart->mDevIfInstId);
            K2_ASSERT(NULL != pVolPart->mpBlockIo);
        }
    }

    if (ixPart != apVol->StorVol.mPartitionCount)
    {
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        // undo in reverse order
        if (ixPart > 0)
        {
            do {
                --ixPart;
                pVolPart = apVol->mppParts[ixPart];
                K2OS_BlockIo_RangeDelete(pVolPart->mDevBlockIo, pVolPart->mDevRange);
                pVolPart->mDevRange = NULL;
                K2OS_BlockIo_Detach(pVolPart->mDevBlockIo);
                pVolPart->mDevBlockIo = NULL;
                BlockIo_Release(pVolPart->mpBlockIo);
                pVolPart->mpBlockIo = NULL;
            } while (ixPart != 0);
        }

        return stat;
    }

    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    do {
        K2OS_CritSec_Enter(&sgVolMgr.VolListSec);
        K2LIST_AddAtTail(&sgVolMgr.VolList, &apVol->VolListLink);
        K2OS_CritSec_Leave(&sgVolMgr.VolListSec);

        apVol->mIsMade = TRUE;

        apVol->mIfInst = K2OS_RpcObj_AddIfInst(
            apVol->mRpcObj,
            K2OS_IFACE_CLASSCODE_STORAGE_VOLUME,
            &sVolIfaceClassId,
            &apVol->mIfInstId,
            TRUE
        );

        if (NULL == apVol->mIfInst)
        {
            apVol->mIsMade = FALSE;

            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));

            K2OS_CritSec_Enter(&sgVolMgr.VolListSec);
            K2LIST_Remove(&sgVolMgr.VolList, &apVol->VolListLink);
            K2OS_CritSec_Leave(&sgVolMgr.VolListSec);

            K2OSKERN_Debug("VOLMGR: failed to publish ifinst for volume (%08X)\n", stat);
            break;
        }

//        K2OSKERN_Debug("Volume %d published\n", apVol->mIfInstId);
        stat = K2STAT_NO_ERROR;

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        // undo partition attach in reverse order
        ixPart = apVol->StorVol.mPartitionCount;
        do {
            ixPart--;
            pVolPart = apVol->mppParts[ixPart];
            K2OS_BlockIo_RangeDelete(pVolPart->mDevBlockIo, pVolPart->mDevRange);
            pVolPart->mDevRange = NULL;
            K2OS_BlockIo_Detach(pVolPart->mDevBlockIo);
            pVolPart->mDevBlockIo = NULL;
            BlockIo_Release(pVolPart->mpBlockIo);
            pVolPart->mpBlockIo = NULL;
        } while (ixPart > 0);
    }

    return stat;
}

K2STAT
VolRpc_Make(
    VOL *       apVol,
    VOLUSER *   apUser
)
{
    K2STAT stat;

    if (0 == (apUser->mAccess & K2OS_ACCESS_W))
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    K2OS_CritSec_Enter(&apVol->Sec);

    if (apVol->mIsMade)
    {
        stat = K2STAT_ERROR_ALREADY_EXISTS;
    }
    else
    {
        stat = Vol_Locked_Make(apVol);
    }

    K2OS_CritSec_Leave(&apVol->Sec);

    return stat;
}

K2STAT
VolRpc_Break(
    VOL *                           apVol,
    VOLUSER *                       apUser
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
VolRpc_Transfer(
    VOL *                           apVol,
    VOLUSER *                       apUser,
    K2OS_STORVOL_TRANSFER_IN const *apTransfer
)
{
    K2OS_BLOCKIO_TRANSFER_IN    transIn;
    UINT32                      startBlock;
    UINT32                      blockCount;
    UINT32                      blockSizeBytes;
    UINT32                      volBlockCount;
    K2STAT                      stat;
    VOLPART *                   pPart;
    UINT32                      blockIx;
    UINT32                      transferBlockCount;
    BOOL                        inSec;

    stat = K2STAT_NO_ERROR;

    K2OS_CritSec_Enter(&apVol->Sec);
    inSec = TRUE;

    do {
        if (!apVol->mIsMade)
        {
            stat = K2STAT_ERROR_NOT_READY;
            break;
        }

        blockSizeBytes = apVol->StorVol.mBlockSizeBytes;
        volBlockCount = apVol->StorVol.mBlockCount;
        K2_ASSERT(0 != blockSizeBytes);
        K2_ASSERT(0 != volBlockCount);
        startBlock = apTransfer->mBytesOffset;
        blockCount = apTransfer->mByteCount;

        if ((0 != (startBlock % blockSizeBytes)) ||
            (0 != (blockCount % blockSizeBytes)) ||
            (0 != apTransfer->mMemAddr % blockSizeBytes))
        {
            stat = K2STAT_ERROR_BAD_ALIGNMENT;
            break;
        }

        startBlock /= blockSizeBytes;
        blockCount /= blockSizeBytes;

        if ((0 == blockCount) ||
            (startBlock >= volBlockCount) ||
            ((volBlockCount - startBlock) < blockCount))
        {
            stat = K2STAT_ERROR_OUT_OF_BOUNDS;
            break;
        }

        inSec = FALSE;
        K2OS_CritSec_Leave(&apVol->Sec);

        transIn.mIsWrite = apTransfer->mIsWrite;
        transIn.mMemAddr = apTransfer->mMemAddr;

        do {
            blockIx = startBlock;
            pPart = apVol->mppParts[0];
            while (blockIx >= pPart->StorPart.mBlockCount)
            {
                blockIx -= pPart->StorPart.mBlockCount;
                pPart++;
            }

            transIn.mRange = pPart->mDevRange;
            transIn.mBytesOffset = blockIx * blockSizeBytes;

            transferBlockCount = (pPart->StorPart.mBlockCount - blockIx);
            if (blockCount < transferBlockCount)
            {
                transferBlockCount = blockCount;
            }

            transIn.mByteCount = transferBlockCount * blockSizeBytes;

            stat = BlockIo_Transfer(pPart->mpBlockIo, apUser->mProcId, &transIn);

            if (K2STAT_IS_ERROR(stat))
                break;

            blockCount -= transferBlockCount;
            if (0 == blockCount)
                break;

            transIn.mMemAddr += transIn.mByteCount;
            startBlock += transferBlockCount;

        } while (1);

    } while (0);

    if (inSec)
    {
        K2OS_CritSec_Leave(&apVol->Sec);
    }

    return stat;
}

K2STAT
K2OSEXEC_VolRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    VOL *       pVol;
    VOLUSER *   pUser;
    K2STAT      stat;

    pVol = (VOL *)apCall->mObjContext;
    pUser = (VOLUSER *)apCall->mUseContext;

    stat = K2STAT_ERROR_NOT_IMPL;

    switch (apCall->Args.mMethodId)
    {
    case K2OS_STORVOL_METHOD_CONFIG:
        if ((apCall->Args.mInBufByteCount < sizeof(K2OS_STORVOL_CONFIG_IN)) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else 
        {
            stat = VolRpc_Config(pVol, pUser, (K2OS_STORVOL_CONFIG_IN const *)apCall->Args.mpInBuf);
        }
        break;

    case K2OS_STORVOL_METHOD_GETINFO:
        if ((apCall->Args.mInBufByteCount != 0) ||
            (apCall->Args.mOutBufByteCount < sizeof(K2OS_STORAGE_VOLUME)))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else
        {
            K2MEM_Copy(apCall->Args.mpOutBuf, &pVol->StorVol, sizeof(K2OS_STORAGE_VOLUME));
            *apRetUsedOutBytes = sizeof(K2OS_STORAGE_VOLUME);
            stat = K2STAT_NO_ERROR;
        }
        break;

    case K2OS_STORVOL_METHOD_ADDPART:
        if ((apCall->Args.mInBufByteCount < sizeof(K2OSSTOR_VOLUME_PART)) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = VolRpc_AddPart(
                pVol, 
                pUser, 
                (K2OSSTOR_VOLUME_PART const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_STORVOL_METHOD_REMPART:
        if ((apCall->Args.mInBufByteCount < sizeof(K2OS_STORVOL_REMPART_IN)) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = VolRpc_RemPart(
                pVol, 
                pUser, 
                (K2OS_STORVOL_REMPART_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_STORVOL_METHOD_GETPART:
        if ((apCall->Args.mInBufByteCount < sizeof(K2OS_STORVOL_GETPART_IN)) ||
            (apCall->Args.mOutBufByteCount < sizeof(K2OSSTOR_VOLUME_PART)))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = VolRpc_GetPart(
                pVol, 
                pUser, 
                (K2OS_STORVOL_GETPART_IN const *)apCall->Args.mpInBuf, 
                (K2OSSTOR_VOLUME_PART *)apCall->Args.mpOutBuf 
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OSSTOR_VOLUME_PART);
            }
        }
        break;

    case K2OS_STORVOL_METHOD_MAKE:
        if ((apCall->Args.mInBufByteCount != 0) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = VolRpc_Make(pVol, pUser);
        }
        break;

    case K2OS_STORVOL_METHOD_GETSTATE:
        if ((apCall->Args.mInBufByteCount != 0) ||
            (apCall->Args.mOutBufByteCount < sizeof(K2OS_STORVOL_GETSTATE_OUT)))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            ((K2OS_STORVOL_GETSTATE_OUT *)apCall->Args.mpOutBuf)->mIsMade = pVol->mIsMade;
            *apRetUsedOutBytes = sizeof(K2OS_STORVOL_GETSTATE_OUT);
            stat = K2STAT_NO_ERROR;
        }
        break;

    case K2OS_STORVOL_METHOD_BREAK:
        if ((apCall->Args.mInBufByteCount != 0) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = VolRpc_Break(pVol, pUser);
        }
        break;

    case K2OS_STORVOL_METHOD_TRANSFER:
        if ((apCall->Args.mInBufByteCount < sizeof(K2OS_STORVOL_TRANSFER_IN)) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = VolRpc_Transfer(
                pVol, 
                pUser, 
                (K2OS_STORVOL_TRANSFER_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    default:
        break;
    }

    return stat;
}

K2STAT 
K2OSEXEC_VolRpc_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    VOL * pVol;

    if (0 != apCreate->mCreatorProcessId)
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pVol = (VOL *)apCreate->mCreatorContext;
    pVol->mRpcObj = aObject;
    *apRetContext = (UINT32)pVol;

    return K2STAT_NO_ERROR;
}

K2STAT 
K2OSEXEC_VolRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    VOL *       pVol;
    VOLUSER *   pUser;
    K2STAT      stat;

    pVol = (VOL *)aObjContext;

    pUser = (VOLUSER *)K2OS_Heap_Alloc(sizeof(VOLUSER));
    if (NULL == pUser)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pUser, sizeof(VOLUSER));
    pUser->mProcId = aProcessId;

    //
    // users are on list in order they attach
    // this is important for sharing rights
    //
    K2OS_CritSec_Enter(&pVol->Sec);

    if (0 == pVol->UserList.mNodeCount)
    {
        // first user is creator/master
        pUser->mIsMaster = TRUE;
        K2_ASSERT(pVol->mShare == ((UINT32)-1));
    }

    K2LIST_AddAtTail(&pVol->UserList, &pUser->UserListLink);

    K2OS_CritSec_Leave(&pVol->Sec);

    *apRetUseContext = (UINT32)pUser;

    return K2STAT_NO_ERROR;
}

K2STAT 
K2OSEXEC_VolRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    VOL *       pVol;
    VOLUSER *   pUser;
    VOLUSER *   pOtherUser;

    pVol = (VOL *)aObjContext;
    pUser = (VOLUSER *)aUseContext;

    K2OS_CritSec_Enter(&pVol->Sec);

    K2LIST_Remove(&pVol->UserList, &pUser->UserListLink);

    if (pVol->UserList.mNodeCount == 1)
    {
        pOtherUser = K2_GET_CONTAINER(VOLUSER, pVol->UserList.mpHead, UserListLink);
        if (pOtherUser->mIsMaster)
        {
            //
            // last remaining user is master. reset sharing config
            //
            pVol->mShare = (UINT32)-1;
        }
    }
    else if (pUser->mIsMaster)
    {
        //
        // master user detach.  volume should be dead now
        //
        K2_ASSERT(0);
    }

    K2OS_CritSec_Leave(&pVol->Sec);

    K2OS_Heap_Free(pUser);

    return K2STAT_NO_ERROR;
}

K2STAT 
K2OSEXEC_VolRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    VOL * pVol;

    pVol = (VOL *)aObjContext;

    K2_ASSERT(0 == pVol->UserList.mNodeCount);
    K2_ASSERT(NULL == pVol->mIfInst);

    pVol->mRpcObj = NULL;
    pVol->mRpcObjHandle = NULL;

    return K2STAT_NO_ERROR;
}

// {266DEF40-5E27-4980-A9D7-3230CB69AD10}
static K2OS_RPC_OBJ_CLASSDEF sgVolClassDef =
{
    { 0x266def40, 0x5e27, 0x4980, { 0xa9, 0xd7, 0x32, 0x30, 0xcb, 0x69, 0xad, 0x10 } },
    K2OSEXEC_VolRpc_Create,
    K2OSEXEC_VolRpc_OnAttach,
    K2OSEXEC_VolRpc_OnDetach,
    K2OSEXEC_VolRpc_Call,
    K2OSEXEC_VolRpc_Delete
};

//
// -------------------------------------------------------------------------------------------
//

void
VolMgr_PartLocked_Change(
    VOLPART *   apPart,
    BOOL        aIsAdd
)
{
    VOL *   pVol;
    K2STAT  stat;

    stat = K2STAT_NO_ERROR;

    if (aIsAdd)
    {
        K2LIST_AddAtTail(&sgVolMgr.PartList, &apPart->PartListLink);

        do {
            pVol = (VOL *)K2OS_Heap_Alloc(sizeof(VOL));
            if (NULL == pVol)
            {
                K2OSKERN_Debug("VOLMGR: Mem alloc failure creating volume\n");
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }

            do {
                K2MEM_Zero(pVol, sizeof(VOL));

                if (!K2OS_CritSec_Init(&pVol->Sec))
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    K2OSKERN_Debug("VOLMGR: failed to create cs for volume (%08X)\n", stat);
                    break;
                }

                do {
                    pVol->mppParts = (VOLPART **)K2OS_Heap_Alloc(sizeof(VOLPART *));
                    if (NULL == pVol->mppParts)
                    {
                        K2OSKERN_Debug("VOLMGR: Mem alloc failure creating volume partition track\n");
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    do {
                        pVol->mppParts[0] = apPart;
                        pVol->StorVol.mPartitionCount = 1;
                        pVol->StorVol.mBlockSizeBytes = apPart->StorPart.mBlockSizeBytes;
                        pVol->StorVol.mBlockCount = apPart->StorPart.mBlockCount;
                        pVol->StorVol.mTotalBytes = ((UINT64)pVol->StorVol.mBlockCount) * ((UINT64)pVol->StorVol.mBlockSizeBytes);
                        if (apPart->StorPart.mFlagActive)
                        {
                            pVol->StorVol.mAttributes |= K2OS_STORAGE_VOLUME_ATTRIB_BOOT;
                        }
                        if (apPart->StorPart.mFlagReadOnly)
                        {
                            pVol->StorVol.mAttributes |= K2OS_STORAGE_VOLUME_ATTRIB_READ_ONLY;
                        }

                        pVol->mShare = (UINT32)-1;

                        pVol->mRpcObjHandle = K2OS_Rpc_CreateObj(0, &sgVolClassDef.ClassId, (UINT32)pVol);
                        if (NULL == pVol->mRpcObjHandle)
                        {
                            stat = K2OS_Thread_GetLastStatus();
                            K2_ASSERT(K2STAT_IS_ERROR(stat));
                            K2OSKERN_Debug("VOLMGR: failed to create rpc object for volume (%08X)\n", stat);
                            break;
                        }

                        K2_ASSERT(NULL != pVol->mRpcObj);

                        K2OS_CritSec_Enter(&pVol->Sec);
                        stat = Vol_Locked_Make(pVol);
                        K2OS_CritSec_Leave(&pVol->Sec);

                        if (K2STAT_IS_ERROR(stat))
                        {
                            K2OS_Rpc_Release(pVol->mRpcObjHandle);
                        }

                    } while (0);

                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OS_Heap_Free(pVol->mppParts);
                    }

                } while (0);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_CritSec_Done(&pVol->Sec);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Heap_Free(pVol);
            }

        } while (0);
    }

    if (!aIsAdd)
    {
        K2_ASSERT(0);
        K2LIST_Remove(&sgVolMgr.PartList, &apPart->PartListLink);
    }
}

void
VolMgr_Part_Arrived(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OS_RPC_OBJ_HANDLE                 hRpcObj;
    K2OS_RPC_CALLARGS                   args;
    K2OS_STORAGE_PARTITION_GET_INFO_OUT info;
    K2STAT                              stat;
    UINT32                              actualOut;
    VOLPART *                           pPart;

    hRpcObj = K2OS_Rpc_AttachByIfInstId(aIfInstId, NULL);
    if (NULL == hRpcObj)
    {
        K2OSKERN_Debug("VOLMGR: Could not attach to partition ifinstid %d that just arrived (%08X)\n", aIfInstId, K2OS_Thread_GetLastStatus());
        return;
    }

    K2MEM_Zero(&args, sizeof(args));
    args.mMethodId = K2OS_STORAGE_PARTITION_METHOD_GET_INFO;
    args.mpOutBuf = (UINT8 *)&info;
    args.mOutBufByteCount = sizeof(info);

    actualOut = 0;
    stat = K2OS_Rpc_Call(hRpcObj, &args, &actualOut);
    if ((K2STAT_IS_ERROR(stat)) || (actualOut != sizeof(info)))
    {
        K2OS_Rpc_Release(hRpcObj);
        K2OSKERN_Debug("VOLMGR: call to get partition info failed (%08X)\n", stat);
        return;
    }

    pPart = (VOLPART *)K2OS_Heap_Alloc(sizeof(VOLPART));
    if (NULL != pPart)
    {
        K2MEM_Zero(pPart, sizeof(VOLPART));

        pPart->mDevIfInstId = info.mDevIfInstId;
        pPart->mIfInstId = aIfInstId;
        pPart->mRpcHandle = hRpcObj;
        hRpcObj = NULL;
        K2MEM_Copy(&pPart->StorPart, &info.Info, sizeof(K2OS_STORAGE_PARTITION));

        K2OS_CritSec_Enter(&sgVolMgr.PartListSec);
        VolMgr_PartLocked_Change(pPart, TRUE);
        K2OS_CritSec_Leave(&sgVolMgr.PartListSec);
    }
    else
    {
        K2OSKERN_Debug("VOLMGR: mem alloc failed for VOLPART\n");
    }

    if (NULL != hRpcObj)
    {
        K2OS_Rpc_Release(hRpcObj);
    }
}

void
VolMgr_Part_Departed(
    K2OS_IFINST_ID aIfInstId
)
{
    K2LIST_LINK *   pListLink;
    VOLPART *       pPart;

    K2OSKERN_Debug("VOLMGR: Partition ifinst %d departed\n", aIfInstId);

    pPart = NULL;

    K2OS_CritSec_Enter(&sgVolMgr.PartListSec);

    pListLink = sgVolMgr.PartList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pPart = K2_GET_CONTAINER(VOLPART, pListLink, PartListLink);
            if (pPart->mIfInstId == aIfInstId)
            {
                VolMgr_PartLocked_Change(pPart, FALSE);
                break;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&sgVolMgr.PartListSec);

    if (NULL == pListLink)
        return;

    K2_ASSERT(NULL != pPart);

    K2OS_Rpc_Release(pPart->mRpcHandle);
    
    K2OS_Heap_Free(pPart);
}

void
VolMgr_Recv_Notify(
    UINT32  aPayload0,
    UINT32  aPayload1,
    UINT32  aPayload2
)
{
    K2OSKERN_Debug("VOLMGR: Recv Notify(%08X,%08X,%08X)\n", aPayload0, aPayload1, aPayload2);
    K2_ASSERT(0);
}

UINT32
VolMgr_Thread(
    void *apArg
)
{
    static K2_GUID128 const sPartIfaceId = K2OS_IFACE_STORAGE_PARTITION;
    K2OS_IFSUBS_TOKEN   tokSubs;
    K2OS_MSG            msg;
    K2OS_WaitResult     waitResult;

    sgVolMgr.mTokMailbox = K2OS_Mailbox_Create();
    if (NULL == sgVolMgr.mTokMailbox)
    {
        K2OSKERN_Panic("***VolMgr thread coult not create a mailbox\n");
    }

    tokSubs = K2OS_IfSubs_Create(sgVolMgr.mTokMailbox, K2OS_IFACE_CLASSCODE_STORAGE_PARTITION, &sPartIfaceId, 12, FALSE, NULL);
    if (NULL == tokSubs)
    {
        K2OSKERN_Panic("***VolMgr thread coult not subscribe to partition interface pop changes\n");
    }

    //
    // notify we have started and main thread can continue
    //
    K2OS_Gate_Open((K2OS_SIGNAL_TOKEN)apArg);

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, sgVolMgr.mTokMailbox, K2OS_TIMEOUT_INFINITE))
            break;
        K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);
        if (K2OS_Mailbox_Recv(sgVolMgr.mTokMailbox, &msg, 0))
        {
            if (msg.mType == K2OS_SYSTEM_MSGTYPE_IFINST)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    // payload[0] = subscription context
                    // payload[1] = interface instance id
                    // payload[2] = process that published the interface
                    VolMgr_Part_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                }
                else if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    VolMgr_Part_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                }
                else
                {
                    K2OSKERN_Debug("*** VolMgr received unexpected IFINST message (%04X)\n", msg.mShort);
                }
            }
            else if (msg.mType == K2OS_SYSTEM_MSGTYPE_RPC)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_RPC_SHORT_NOTIFY)
                {
                    VolMgr_Recv_Notify(msg.mPayload[0], msg.mPayload[1], msg.mPayload[2]);
                }
                else
                {
                    K2OSKERN_Debug("*** VolMgr received unexpected RPC message (%04X)\n", msg.mShort);
                }
            }
            else
            {
                K2OSKERN_Debug("*** SysProc VolMgr received unexpected message type (%04X)\n", msg.mType);
            }
        }

    } while (1);

    return 0;
}

K2STAT
K2OSEXEC_VolMgrRpc_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    K2OS_SIGNAL_TOKEN   tokStartGate;
    K2OS_WaitResult     waitResult;
    K2OS_THREAD_TOKEN   tokThread;

    if ((apCreate->mCreatorProcessId != 0) ||
        (apCreate->mCreatorContext != (UINT32)&sgVolMgr))
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    *apRetContext = 0;

    sgVolMgr.mRpcObj = aObject;
    
    if (!K2OS_CritSec_Init(&sgVolMgr.VolListSec))
    {
        K2OSKERN_Panic("VOLMGR: Could not create cs for volume list\n");
    }
    K2LIST_Init(&sgVolMgr.VolList);

    if (!K2OS_CritSec_Init(&sgVolMgr.PartListSec))
    {
        K2OSKERN_Panic("VOLMGR: Could not create cs for partition list\n");
    }
    K2LIST_Init(&sgVolMgr.PartList);

    sgVolMgr.mVolClass = K2OS_RpcServer_Register(&sgVolClassDef, 0);
    if (NULL == sgVolMgr.mVolClass)
    {
        K2OSKERN_Panic("VOLMGR: Could not register volume object class\n");
    }

    tokStartGate = K2OS_Gate_Create(FALSE);
    if (NULL == tokStartGate)
    {
        K2OSKERN_Panic("VOLMGR: could not create volmgr start gate\n");
    }

    tokThread = K2OS_Thread_Create("Volume Manager", VolMgr_Thread, tokStartGate, NULL, &sgVolMgr.mThreadId);
    if (NULL == tokThread)
    {
        K2OSKERN_Panic("VOLMGR: Could not start volume manager thread\n");
    }

    if (!K2OS_Thread_WaitOne(&waitResult, tokStartGate, K2OS_TIMEOUT_INFINITE))
    {
        K2OSKERN_Panic("VOLMGR: Failed to wait for startup gate\n");
    }

    K2OS_Token_Destroy(tokThread);

    K2OS_Token_Destroy(tokStartGate);

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_VolMgrRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_VolMgrRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_VolMgrRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
K2OSEXEC_VolMgrRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    K2OSKERN_Panic("VolMgr obj delete - should be impossible.\n");
    return K2STAT_ERROR_UNKNOWN;
}

void
VolMgr_Init(
    void
)
{
    // {6909660A-C139-4B45-BC95-D1F1EB885498}
    static const K2OS_RPC_OBJ_CLASSDEF sVolMgrClassDef =
    {
        { 0x6909660a, 0xc139, 0x4b45, { 0xbc, 0x95, 0xd1, 0xf1, 0xeb, 0x88, 0x54, 0x98 } },
        K2OSEXEC_VolMgrRpc_Create,
        K2OSEXEC_VolMgrRpc_OnAttach,
        K2OSEXEC_VolMgrRpc_OnDetach,
        K2OSEXEC_VolMgrRpc_Call,
        K2OSEXEC_VolMgrRpc_Delete
    };
    static const K2_GUID128 sVolMgrIfaceId = K2OS_IFACE_STORAGE_VOLMGR;

    K2OS_RPC_CLASS      volMgrClass;
    K2OS_RPC_OBJ_HANDLE hRpcObj;

    K2MEM_Zero(&sgVolMgr, sizeof(sgVolMgr));

    volMgrClass = K2OS_RpcServer_Register(&sVolMgrClassDef, 0);
    if (NULL == volMgrClass)
    {
        K2OSKERN_Panic("VOLMGR: Could not register volume manager object class\n");
    }

    hRpcObj = K2OS_Rpc_CreateObj(0, &sVolMgrClassDef.ClassId, (UINT32)&sgVolMgr);
    if (NULL == hRpcObj)
    {
        K2OSKERN_Panic("VOLMGR: failed to create volume manager object(%08X)\n", K2OS_Thread_GetLastStatus());
    }

    sgVolMgr.mRpcIfInst = K2OS_RpcObj_AddIfInst(
        sgVolMgr.mRpcObj,
        K2OS_IFACE_CLASSCODE_STORAGE_VOLMGR,
        &sVolMgrIfaceId,
        &sgVolMgr.mIfInstId,
        TRUE
    );
    if (NULL == sgVolMgr.mRpcIfInst)
    {
        K2OSKERN_Panic("VOLMGR: Could not publish volume manager ifinst\n");
    }
}

