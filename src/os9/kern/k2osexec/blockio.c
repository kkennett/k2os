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

#define MAX_BLOCKIO_CHUNK_BYTES 8192

static K2OS_CRITSEC     sgBlockIoListSec;
static K2LIST_ANCHOR    sgBlockIoList;

K2STAT 
K2OSEXEC_BlockIoRpc_Create(
    K2OS_RPC_OBJ                aObject, 
    K2OS_RPC_OBJ_CREATE const * apCreate, 
    UINT32 *                    apRetContext
)
{
    BLOCKIO * pBlockIo;

    if (0 != apCreate->mCreatorProcessId)
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pBlockIo = (BLOCKIO *)apCreate->mCreatorContext;
    pBlockIo->mRpcObj = aObject;
    *apRetContext = (UINT32)pBlockIo;

    return K2STAT_NO_ERROR;
}

K2STAT 
K2OSEXEC_BlockIoRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    BLOCKIO *       pBlockIo;
    BLOCKIO_USER *  pUser;
    BLOCKIO_PROC *  pProc;
    K2LIST_LINK *   pListLink;
    BLOCKIO_PROC *  pOtherProc;

    pBlockIo = (BLOCKIO *)aObjContext;
    K2_ASSERT(pBlockIo->mRpcObj == aObject);

    pUser = (BLOCKIO_USER *)K2OS_Heap_Alloc(sizeof(BLOCKIO_USER));
    if (NULL == pUser)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pProc = (BLOCKIO_PROC *)K2OS_Heap_Alloc(sizeof(BLOCKIO_PROC));
    if (NULL == pProc)
    {
        K2OS_Heap_Free(pUser);
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pProc, sizeof(BLOCKIO_PROC));
    pProc->mProcessId = aProcessId;
    K2LIST_Init(&pProc->UserList);

    K2MEM_Zero(pUser, sizeof(BLOCKIO_USER));
    *apRetUseContext = (UINT32)pUser;

    pOtherProc = NULL;

    K2OS_CritSec_Enter(&pBlockIo->Sec);

    pListLink = pBlockIo->ProcList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOtherProc = K2_GET_CONTAINER(BLOCKIO_PROC, pListLink, InstanceProcListLink);
            if (pOtherProc->mProcessId == aProcessId)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    if (NULL == pListLink)
    {
        //
        // proc not found
        //
        pUser->mpProc = pProc;
        K2LIST_AddAtTail(&pBlockIo->ProcList, &pProc->InstanceProcListLink);
        pProc = NULL;
    }
    else
    {
        pUser->mpProc = pOtherProc;
    }

    K2LIST_AddAtTail(&pUser->mpProc->UserList, &pUser->ProcUserListLink);

    K2OS_CritSec_Leave(&pBlockIo->Sec);

    if (NULL != pProc)
    {
        K2OS_Heap_Free(pProc);
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
K2OSEXEC_BlockIoRpc_OnDetach(
    K2OS_RPC_OBJ    aObject, 
    UINT32          aObjContext, 
    UINT32          aUseContext
)
{
    BLOCKIO *           pBlockIo;
    BLOCKIO_USER *      pUser;
    BLOCKIO_PROC *      pProc;
    BLOCKIO_RANGE *     pRange;
    K2LIST_LINK *       pListLink;
    BLOCKIO_SESSION *   pSession;
    K2LIST_LINK *       pSessLink;

    pBlockIo = (BLOCKIO *)aObjContext;
    K2_ASSERT(pBlockIo->mRpcObj == aObject);

    pUser = (BLOCKIO_USER *)aUseContext;
    pProc = pUser->mpProc;

    K2OS_CritSec_Enter(&pBlockIo->Sec);

    K2LIST_Remove(&pProc->UserList, &pUser->ProcUserListLink);
    if (0 == pProc->UserList.mNodeCount)
    {
        //
        // last user in this process is detaching
        //
        K2LIST_Remove(&pBlockIo->ProcList, &pProc->InstanceProcListLink);

        //
        // any sessions ever made that have ranges from this process
        // must have those ranges removed from that session
        // 
        pSessLink = pBlockIo->SessionList.mpHead;
        K2_ASSERT(NULL != pSessLink);
        do {
            pSession = K2_GET_CONTAINER(BLOCKIO_SESSION, pSessLink, InstanceSessionListLink);
            pSessLink = pSessLink->mpNext;

            pListLink = pSession->RangeList.mpHead;
            if (NULL != pListLink)
            {
                do {
                    pRange = K2_GET_CONTAINER(BLOCKIO_RANGE, pListLink, SessionRangeListLink);
                    pListLink = pListLink->mpNext;

                    if (pRange->mProcessId == pProc->mProcessId)
                    {
                        K2LIST_Remove(&pSession->RangeList, &pRange->SessionRangeListLink);
                        K2OS_Heap_Free(pRange);
                    }
                } while (NULL != pListLink);
            }

            if (0 == pSession->RangeList.mNodeCount)
            {
                //
                // session has no ranges left. if it not the current session
                // then prune it
                //
                if (pSession != pBlockIo->mpCurSession)
                {
                    K2LIST_Remove(&pBlockIo->SessionList, &pSession->InstanceSessionListLink);
                    K2OS_Heap_Free(pSession);
                }
            }

        } while (NULL != pSessLink);
    }
    else
    {
        pProc = NULL;
    }

    K2OS_CritSec_Leave(&pBlockIo->Sec);

    if (NULL != pProc)
    {
        K2OS_Heap_Free(pProc);
    }

    K2OS_Heap_Free(pUser);

    return K2STAT_NO_ERROR;
}

K2STAT
BlockIo_Config(
    BLOCKIO *                       apBlockIo,
    BLOCKIO_USER *                  apUser,
    K2OS_BLOCKIO_CONFIG_IN const *  apConfigIn
)
{
    K2STAT stat;

    K2OS_CritSec_Enter(&apBlockIo->Sec);

    if (apUser->mConfigSet)
    {
        stat = K2STAT_ERROR_NOT_CONFIGURED;
    }
    else
    {
        stat = K2STAT_NO_ERROR;

        //
        // actually validate access and share here
        //

        K2MEM_Copy(&apUser->Config, apConfigIn, sizeof(K2OS_BLOCKIO_CONFIG_IN));
        apUser->mConfigSet = TRUE;
    }

    K2OS_CritSec_Leave(&apBlockIo->Sec);

    return stat;
}

K2STAT
BlockIo_GetMedia(
    BLOCKIO *               apBlockIo,
    BLOCKIO_USER *          apUser,
    K2OS_STORAGE_MEDIA *    apRetMedia
)
{
    K2STAT              stat;
    BLOCKIO_SESSION *   pSession;

    K2OS_CritSec_Enter(&apBlockIo->Sec);

    if (!apUser->mConfigSet)
    {
        stat = K2STAT_ERROR_NOT_CONFIGURED;
    }
    else
    {
        pSession = apBlockIo->mpCurSession;
        if (NULL == pSession)
        {
            stat = K2STAT_ERROR_DEVICE_REMOVED;
        }
        else
        {
            if (!K2RUNDOWN_Get(&apBlockIo->Rundown))
            {
                stat = K2STAT_ERROR_DEVICE_REMOVED;
            }
            else
            {
                K2_ASSERT(NULL != apBlockIo->Register.GetMedia);
                stat = apBlockIo->Register.GetMedia(apBlockIo->mpDriverContext, apRetMedia);
                if (!K2STAT_IS_ERROR(stat))
                {
                    pSession->mBlockCount = apRetMedia->mBlockCount;
                    pSession->mBlockSizeBytes = apRetMedia->mBlockSizeBytes;
                    pSession->mIsReadOnly = (0 != (apRetMedia->mAttrib & K2OS_STORAGE_MEDIA_ATTRIB_READ_ONLY)) ? TRUE : FALSE;
                }
                K2RUNDOWN_Put(&apBlockIo->Rundown);
            }
        }
    }

    K2OS_CritSec_Leave(&apBlockIo->Sec);

    return stat;
}

K2STAT
BlockIo_RangeCreate(
    BLOCKIO *                           apBlockIo,
    BLOCKIO_USER *                      apUser,
    K2OS_BLOCKIO_RANGE_CREATE_IN const *apCreateIn,
    K2OS_BLOCKIO_RANGE_CREATE_OUT *     apCreateOut
)
{
    BLOCKIO_RANGE *     pNewRange;
    K2STAT              stat;
    K2LIST_LINK *       pListLink;
    BLOCKIO_RANGE *     pScan;
    BLOCKIO_SESSION *   pSession;

    pNewRange = K2OS_Heap_Alloc(sizeof(BLOCKIO_RANGE));
    if (NULL == pNewRange)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pNewRange, sizeof(BLOCKIO_RANGE));
    pNewRange->mStartBlock = apCreateIn->mRangeBaseBlock;
    pNewRange->mBlockCount = apCreateIn->mRangeBlockCount;
    pNewRange->mPrivate = apCreateIn->mMakePrivate;
    pNewRange->mProcessId = apUser->mpProc->mProcessId;

    K2OS_CritSec_Enter(&apBlockIo->Sec);

    do {
        pSession = apBlockIo->mpCurSession;
        if (NULL == pSession)
        {
            stat = K2STAT_ERROR_DEVICE_REMOVED;
            break;
        }
        if (0 == pSession->mBlockCount)
        {
            stat = K2STAT_ERROR_MEDIA_CHANGED;
            break;
        }

        pNewRange->mId = ++apBlockIo->mLastRangeId;

        stat = K2STAT_NO_ERROR;

        pListLink = pSession->RangeList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pScan = K2_GET_CONTAINER(BLOCKIO_RANGE, pListLink, SessionRangeListLink);
                if (pScan->mProcessId != apUser->mpProc->mProcessId)
                {
                    if (pNewRange->mStartBlock < pScan->mStartBlock)
                    {
                        if ((pScan->mStartBlock - pNewRange->mStartBlock) < pNewRange->mBlockCount)
                        {
                            // range collision
                            if (pNewRange->mPrivate || pScan->mPrivate)
                            {
                                stat = K2STAT_ERROR_ALREADY_RESERVED;
                                break;
                            }
                        }
                    }
                    else
                    {
                        if ((pNewRange->mStartBlock - pScan->mStartBlock) < pScan->mBlockCount)
                        {
                            // range collision
                            if (pNewRange->mPrivate || pScan->mPrivate)
                            {
                                stat = K2STAT_ERROR_ALREADY_RESERVED;
                                break;
                            }
                        }
                    }
                }

                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }

        if (!K2STAT_IS_ERROR(stat))
        {
            K2LIST_AddAtTail(&pSession->RangeList, &pNewRange->SessionRangeListLink);
            apCreateOut->mRange = (K2OSSTOR_BLOCKIO_RANGE)pNewRange->mId;
            pNewRange = NULL;
        }

    } while (0);

    K2OS_CritSec_Leave(&apBlockIo->Sec);

    if (NULL != pNewRange)
    {
        K2OS_Heap_Free(pNewRange);
    }

    return stat;
}

K2STAT
BlockIo_RangeDelete(
    BLOCKIO *                           apBlockIo,
    BLOCKIO_USER *                      apUser,
    K2OS_BLOCKIO_RANGE_DELETE_IN const *apDeleteIn
)
{
    K2LIST_LINK *       pSessionListLink;
    BLOCKIO_SESSION *   pSession;
    K2LIST_LINK *       pRangeListLink;
    BLOCKIO_RANGE *     pScan;

    pScan = NULL;

    K2OS_CritSec_Enter(&apBlockIo->Sec);

    pSessionListLink = apBlockIo->SessionList.mpHead;
    K2_ASSERT(NULL != pSessionListLink);
    do {
        pSession = K2_GET_CONTAINER(BLOCKIO_SESSION, pSessionListLink, InstanceSessionListLink);
        pRangeListLink = pSession->RangeList.mpHead;
        if (NULL != pRangeListLink)
        {
            do {
                pScan = K2_GET_CONTAINER(BLOCKIO_RANGE, pRangeListLink, SessionRangeListLink);
                if (pScan->mId == ((UINT32)apDeleteIn->mRange))
                {
                    K2LIST_Remove(&pSession->RangeList, &pScan->SessionRangeListLink);
                    break;
                }
                pRangeListLink = pRangeListLink->mpNext;
            } while (NULL != pRangeListLink);

            if (NULL != pRangeListLink)
                break;
        }
        pSessionListLink = pSessionListLink->mpNext;
    } while (NULL != pSessionListLink);

    K2OS_CritSec_Leave(&apBlockIo->Sec);

    if (NULL != pScan)
    {
        K2OS_Heap_Free(pScan);
        return K2STAT_NO_ERROR;
    }

    return K2STAT_ERROR_NOT_FOUND;
}

K2STAT
BlockIo_Transfer(
    BLOCKIO *                       apBlockIo,
    UINT32                          aProcessId,
    K2OS_BLOCKIO_TRANSFER_IN const *apTransferIn
)
{
    K2STAT                      stat;
    BLOCKIO_SESSION *           pSession;
    K2LIST_LINK *               pRangeListLink;
    BLOCKIO_RANGE *             pScan;
    UINT32                      rangeBlockOffset;
    UINT32                      rangeBlockCount;
    UINT32                      mediaBlockBytes;
    UINT32                      mediaBlockCount;
    BOOL                        mediaReadOnly;
    K2OS_BLOCKIO_TRANSFER       trans;
    K2OS_TOKEN                  tokHold;
    UINT32                      holdAddr;
    UINT32                      ioSize;
    UINT32                      blocksLeft;

    if ((NULL == apTransferIn->mRange) ||
        (0 == apTransferIn->mByteCount))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    mediaBlockBytes = mediaBlockCount = 0;
    stat = K2STAT_ERROR_NOT_FOUND;

    K2OS_CritSec_Enter(&apBlockIo->Sec);

    pSession = apBlockIo->mpCurSession;
    if (NULL != pSession)
    {
        if (0 == pSession->mBlockCount)
        {
            stat = K2STAT_ERROR_MEDIA_CHANGED;
        }
        else
        {
            pRangeListLink = pSession->RangeList.mpHead;
            if (NULL != pRangeListLink)
            {
                do {
                    pScan = K2_GET_CONTAINER(BLOCKIO_RANGE, pRangeListLink, SessionRangeListLink);
                    if (pScan->mId == ((UINT32)apTransferIn->mRange))
                    {
                        stat = K2STAT_NO_ERROR;
                        mediaBlockBytes = pSession->mBlockSizeBytes;
                        mediaBlockCount = pSession->mBlockCount;
                        mediaReadOnly = pSession->mIsReadOnly;
                        rangeBlockOffset = pScan->mStartBlock;
                        rangeBlockCount = pScan->mBlockCount;
                        break;
                    }
                    pRangeListLink = pRangeListLink->mpNext;
                } while (NULL != pRangeListLink);
            }
        }
    }
    else
    {
        stat = K2STAT_ERROR_DEVICE_REMOVED;
    }

    K2OS_CritSec_Leave(&apBlockIo->Sec);

    do {
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }

        K2_ASSERT(rangeBlockOffset < mediaBlockCount);
        K2_ASSERT((mediaBlockCount - rangeBlockOffset) >= rangeBlockCount);
        K2_ASSERT(apTransferIn->mByteCount != 0);  // checked before we get here

        if ((0 != (apTransferIn->mMemAddr % mediaBlockBytes)) ||
            (0 != (apTransferIn->mBytesOffset % mediaBlockBytes)) ||
            (0 != (apTransferIn->mByteCount % mediaBlockBytes)))
        {
            stat = K2STAT_ERROR_BAD_ALIGNMENT;
            break;
        }
        else if ((apTransferIn->mIsWrite) && (mediaReadOnly))
        {
            stat = K2STAT_ERROR_READ_ONLY;
            break;
        }

        trans.mStartBlock = apTransferIn->mBytesOffset / mediaBlockBytes;
        trans.mBlockCount = apTransferIn->mByteCount / mediaBlockBytes;

        if ((trans.mStartBlock >= rangeBlockCount) ||
            ((rangeBlockCount - trans.mStartBlock) < trans.mBlockCount))
        {
            stat = K2STAT_ERROR_OUT_OF_BOUNDS;
            break;
        }

        trans.mStartBlock += rangeBlockOffset;
        trans.mIsWrite = apTransferIn->mIsWrite;
        blocksLeft = trans.mBlockCount;
        trans.mAddress = apTransferIn->mMemAddr;

        do {
            holdAddr = trans.mAddress;
            ioSize = trans.mBlockCount * mediaBlockBytes;
            if (ioSize > MAX_BLOCKIO_CHUNK_BYTES)
            {
                ioSize = (MAX_BLOCKIO_CHUNK_BYTES / mediaBlockBytes) * mediaBlockBytes;
            }
            stat = K2OSKERN_PrepIo(
                apBlockIo->Register.Config.mUseHwDma,
                apTransferIn->mIsWrite,
                aProcessId,
                &trans.mAddress,
                &ioSize,
                &tokHold
            );
            if (K2STAT_IS_ERROR(stat))
                break;

            K2_ASSERT(NULL != tokHold);

            K2OS_CritSec_Enter(&apBlockIo->Sec);

            if (apBlockIo->mpCurSession != pSession)
            {
                stat = K2STAT_ERROR_MEDIA_CHANGED;
            }

            K2OS_CritSec_Leave(&apBlockIo->Sec);

            if (!K2STAT_IS_ERROR(stat))
            {
                if (K2RUNDOWN_Get(&apBlockIo->Rundown))
                {
                    trans.mBlockCount = ioSize / mediaBlockBytes;

                    if ((apBlockIo->Register.Config.mUseHwDma) &&
                        (apTransferIn->mIsWrite))
                    {
                        // drain write buffer and flush data range
                        K2_ASSERT(0);
                    }

                    stat = apBlockIo->Register.Transfer(apBlockIo->mpDriverContext, &trans);

                    if ((apBlockIo->Register.Config.mUseHwDma) &&
                        (!K2STAT_IS_ERROR(stat)) &&
                        (!apTransferIn->mIsWrite))
                    {
                        // invalidate data range    
                        K2_ASSERT(0);
                    }

                    K2RUNDOWN_Put(&apBlockIo->Rundown);
                }
                else
                {
                    stat = K2STAT_ERROR_DEVICE_REMOVED;
                }
            }

            K2_CpuWriteBarrier();
            K2OS_Token_Destroy(tokHold);

            if (K2STAT_IS_ERROR(stat))
            {
                break;
            }

            blocksLeft -= trans.mBlockCount;
            if (blocksLeft == 0)
            {
                break;
            }

            trans.mAddress = holdAddr + (trans.mBlockCount * mediaBlockBytes);
            trans.mStartBlock += trans.mBlockCount;

            trans.mBlockCount = blocksLeft;

        } while (1);

    } while (0);

    return stat;
}

K2STAT 
K2OSEXEC_BlockIoRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall, 
    UINT32 *                    apRetUsedOutBytes
)
{
    BLOCKIO *       pBlockIo;
    K2STAT          stat;
    BLOCKIO_USER *  pUser;

    pBlockIo = (BLOCKIO *)apCall->mObjContext;
    K2_ASSERT(pBlockIo->mRpcObj == apCall->mObj);

    pUser = (BLOCKIO_USER *)apCall->mUseContext;

    switch (apCall->Args.mMethodId)
    {
    case K2OS_BLOCKIO_METHOD_CONFIG:
        if ((apCall->Args.mOutBufByteCount != 0) ||
            (apCall->Args.mInBufByteCount < sizeof(K2OS_BLOCKIO_CONFIG_IN)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else if (pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = BlockIo_Config(pBlockIo, pUser, (K2OS_BLOCKIO_CONFIG_IN const *)apCall->Args.mpInBuf);
        }
        break;

    case K2OS_BLOCKIO_METHOD_GET_MEDIA:
        if ((apCall->Args.mInBufByteCount != 0) ||
            (apCall->Args.mOutBufByteCount < sizeof(K2OS_STORAGE_MEDIA)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = BlockIo_GetMedia(pBlockIo, pUser, (K2OS_STORAGE_MEDIA *)apCall->Args.mpOutBuf);
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_STORAGE_MEDIA);
            }
        }
        break;

    case K2OS_BLOCKIO_METHOD_RANGE_CREATE:
        if ((apCall->Args.mInBufByteCount < sizeof(K2OS_BLOCKIO_RANGE_CREATE_IN)) ||
            (apCall->Args.mOutBufByteCount < sizeof(K2OS_BLOCKIO_RANGE_CREATE_OUT)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = BlockIo_RangeCreate(
                pBlockIo,
                pUser,
                (K2OS_BLOCKIO_RANGE_CREATE_IN const *)apCall->Args.mpInBuf,
                (K2OS_BLOCKIO_RANGE_CREATE_OUT *)apCall->Args.mpOutBuf
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_BLOCKIO_RANGE_CREATE_OUT);
            }
        }
        break;

    case K2OS_BLOCKIO_METHOD_RANGE_DELETE:
        if ((apCall->Args.mInBufByteCount < sizeof(K2OS_BLOCKIO_RANGE_DELETE_IN)) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = BlockIo_RangeDelete(
                pBlockIo,
                pUser,
                (K2OS_BLOCKIO_RANGE_DELETE_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    case K2OS_BLOCKIO_METHOD_TRANSFER:
        if ((apCall->Args.mInBufByteCount < sizeof(K2OS_BLOCKIO_TRANSFER_IN)) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else if (!pUser->mConfigSet)
        {
            stat = K2STAT_ERROR_NOT_CONFIGURED;
        }
        else
        {
            stat = BlockIo_Transfer(
                pBlockIo,
                pUser->mpProc->mProcessId,
                (K2OS_BLOCKIO_TRANSFER_IN const *)apCall->Args.mpInBuf
            );
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
    }

    return stat;
}

K2STAT 
K2OSEXEC_BlockIoRpc_Delete(
    K2OS_RPC_OBJ    aObject, 
    UINT32          aObjContext
)
{
    BLOCKIO *   pBlockIo;

    pBlockIo = (BLOCKIO *)aObjContext;

    K2_ASSERT(aObjContext == (UINT32)pBlockIo->mRpcObj);

    // handle should have been nullified by deregister
    K2_ASSERT(0 != (pBlockIo->Rundown.mState & K2RUNDOWN_TRIGGER));
    K2_ASSERT(NULL == pBlockIo->mRpcObjHandle);
    K2_ASSERT(NULL == pBlockIo->mRpcIfInst);
    K2_ASSERT(0 == pBlockIo->mRpcIfInstId);
    K2_ASSERT(NULL == pBlockIo->mpCurSession);
    K2_ASSERT(0 == pBlockIo->ProcList.mNodeCount);

    pBlockIo->mRpcObj = NULL;

    return K2STAT_NO_ERROR;
}

//
// ----------------------------------------------------------------------------------
//

// {3EE88F1D-6779-43F7-B8FE-F56BDBA391CA}
static K2OS_RPC_OBJ_CLASSDEF sgBlockIoRpcClassDef =
{
    { 0x3ee88f1d, 0x6779, 0x43f7, { 0xb8, 0xfe, 0xf5, 0x6b, 0xdb, 0xa3, 0x91, 0xca } },
    K2OSEXEC_BlockIoRpc_Create,
    K2OSEXEC_BlockIoRpc_OnAttach,
    K2OSEXEC_BlockIoRpc_OnDetach,
    K2OSEXEC_BlockIoRpc_Call,
    K2OSEXEC_BlockIoRpc_Delete
};

void 
BlockIo_Notify(
    void *      apKey,
    K2OS_DEVCTX aDevCtx,
    void *      apDevice,
    UINT32      aNotifyCode
)
{
    BLOCKIO *   pBlockIo;

    pBlockIo = K2_GET_CONTAINER(BLOCKIO, apKey, mfNotify);

    K2_ASSERT(aDevCtx == (K2OS_DEVCTX)pBlockIo->mpDevNode);
    K2_ASSERT(apDevice == pBlockIo->mpDriverContext);

    K2OS_CritSec_Enter(&pBlockIo->Sec);
    K2OS_RpcObj_SendNotify(pBlockIo->mRpcObj, 0, aNotifyCode, pBlockIo->mRpcIfInstId);
    K2OS_CritSec_Leave(&pBlockIo->Sec);
}

UINT32      
BlockIo_AddRef(
    BLOCKIO *apBlockIo
)
{
    return (UINT32)K2ATOMIC_Inc(&apBlockIo->mRefCount);
}

UINT32
BlockIo_Release(
    BLOCKIO *   apBlockIo
)
{
    UINT32              result;
    K2LIST_LINK *       pSessionListLink;
    K2LIST_LINK *       pRangeListLink;
    BLOCKIO_SESSION *   pSession;
    BLOCKIO_RANGE *     pRange;

    result = K2ATOMIC_Dec(&apBlockIo->mRefCount);
    if (0 != result)
    {
        return result;
    }

    K2_ASSERT(0 != (K2RUNDOWN_TRIGGER & apBlockIo->Rundown.mState));
    K2_ASSERT(NULL == apBlockIo->mpDevNode);
    K2_ASSERT(NULL == apBlockIo->mpDriverContext);
    K2_ASSERT(NULL == apBlockIo->mRpcIfInst);
    K2_ASSERT(0 == apBlockIo->mRpcIfInstId);
    K2_ASSERT(NULL == apBlockIo->mRpcObjHandle);
    K2_ASSERT(NULL == apBlockIo->mpCurSession);
    K2_ASSERT(0 == apBlockIo->ProcList.mNodeCount);

    K2OS_CritSec_Enter(&sgBlockIoListSec);
    K2LIST_Remove(&sgBlockIoList, &apBlockIo->GlobalListLink);
    K2OS_CritSec_Leave(&sgBlockIoListSec);

    K2RUNDOWN_Done(&apBlockIo->Rundown);

    K2OS_CritSec_Done(&apBlockIo->Sec);

    K2OS_Xdl_Release(apBlockIo->mHoldXdl[0]);
    K2OS_Xdl_Release(apBlockIo->mHoldXdl[1]);

    pSessionListLink = apBlockIo->SessionList.mpHead;
    if (NULL != pSessionListLink)
    {
        do {
            pSession = K2_GET_CONTAINER(BLOCKIO_SESSION, pSessionListLink, InstanceSessionListLink);
            pSessionListLink = pSessionListLink->mpNext;

            pRangeListLink = pSession->RangeList.mpHead;
            if (NULL != pRangeListLink)
            {
                do {
                    pRange = K2_GET_CONTAINER(BLOCKIO_RANGE, pRangeListLink, SessionRangeListLink);
                    pRangeListLink = pRangeListLink->mpNext;

                    K2OS_Heap_Free(pRange);

                } while (NULL != pRangeListLink);
            }

            K2OS_Heap_Free(pSession);

        } while (NULL != pSessionListLink);
    }

    K2MEM_Zero(apBlockIo, sizeof(BLOCKIO));

    K2OS_Heap_Free(apBlockIo);

    return 0;
}

K2STAT
K2OSDDK_BlockIoRegister(
    K2OS_DEVCTX                     aDevCtx,
    void *                          apDevice,
    K2OSDDK_BLOCKIO_REGISTER const *apRegister,
    K2OSDDK_pf_BlockIo_NotifyKey ** apRetNotifyKey
)
{
    static K2_GUID128 const sBlockIoIfaceId = K2OS_IFACE_BLOCKIO_DEVICE;

    BLOCKIO *                   pNewBlockIo;
    K2STAT                      stat;
    DEVNODE *                   pDevNode;
    K2LIST_LINK *               pListLink;
    BLOCKIO *                   pOtherBlockIo;
    BLOCKIO_SESSION *           pNewSession;
    K2OSDDK_BLOCKIO_REGISTER    regis;
    K2OS_XDL                    holdXdl[2];

    if (NULL == apRegister)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2MEM_Copy(&regis, apRegister, sizeof(K2OSDDK_BLOCKIO_REGISTER));

    if ((NULL == regis.GetMedia) ||
        (NULL == regis.Transfer))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    holdXdl[0] = K2OS_Xdl_AddRefContaining((UINT32)regis.GetMedia);
    if (NULL == holdXdl[0])
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    stat = K2STAT_NO_ERROR;

    do {
        holdXdl[1] = K2OS_Xdl_AddRefContaining((UINT32)regis.Transfer);
        if (NULL == holdXdl[1])
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }

        do {
            pNewBlockIo = (BLOCKIO *)K2OS_Heap_Alloc(sizeof(BLOCKIO));

            if (NULL == pNewBlockIo)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
            }

            do {
                pNewSession = (BLOCKIO_SESSION *)K2OS_Heap_Alloc(sizeof(BLOCKIO_SESSION));
                if (NULL == pNewSession)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    break;
                }

                do {
                    K2MEM_Zero(pNewBlockIo, sizeof(BLOCKIO));

                    if (!K2OS_CritSec_Init(&pNewBlockIo->Sec))
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(stat));
                        break;
                    }

                    do {
                        if (!K2RUNDOWN_Init(&pNewBlockIo->Rundown))
                        {
                            stat = K2OS_Thread_GetLastStatus();
                            K2_ASSERT(K2STAT_IS_ERROR(stat));
                            break;
                        }

                        pNewBlockIo->mRpcObjHandle = K2OS_Rpc_CreateObj(0, &sgBlockIoRpcClassDef.ClassId, (UINT32)pNewBlockIo);
                        if (NULL == pNewBlockIo->mRpcObjHandle)
                        {
                            stat = K2OS_Thread_GetLastStatus();
                            K2_ASSERT(K2STAT_IS_ERROR(stat));
                        }
                        else
                        {
                            K2_ASSERT(NULL != pNewBlockIo->mRpcObj);

                            pNewBlockIo->mRefCount = 1;
                            K2MEM_Copy(&pNewBlockIo->Register, &regis, sizeof(K2OSDDK_BLOCKIO_REGISTER));
                            pNewBlockIo->mpDevNode = (DEVNODE *)aDevCtx;
                            pNewBlockIo->mpDriverContext = apDevice;
                            K2LIST_Init(&pNewBlockIo->ProcList);
                            K2LIST_Init(&pNewBlockIo->SessionList);

                            K2MEM_Zero(pNewSession, sizeof(BLOCKIO_SESSION));
                            K2LIST_Init(&pNewSession->RangeList);
                            K2LIST_AddAtTail(&pNewBlockIo->SessionList, &pNewSession->InstanceSessionListLink);
                            pNewBlockIo->mpCurSession = pNewSession;
                            pNewBlockIo->mfNotify = BlockIo_Notify;
                            *apRetNotifyKey = &pNewBlockIo->mfNotify;

                            pNewBlockIo->mHoldXdl[0] = holdXdl[0];
                            K2OS_Xdl_AddRef(holdXdl[0]);
                            pNewBlockIo->mHoldXdl[1] = holdXdl[1];
                            K2OS_Xdl_AddRef(holdXdl[1]);
                        }

                        if (K2STAT_IS_ERROR(stat))
                        {
                            K2RUNDOWN_Done(&pNewBlockIo->Rundown);
                        }

                    } while (0);

                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OS_CritSec_Done(&pNewBlockIo->Sec);
                    }

                } while (0);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_Heap_Free(pNewSession);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Heap_Free(pNewBlockIo);
            }

        } while (0);

        K2OS_Xdl_Release(holdXdl[1]);

    } while (0);

    K2OS_Xdl_Release(holdXdl[0]);

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    //
    // try to add to devnode now
    //
    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    pListLink = pDevNode->InSec.BlockIo.List.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOtherBlockIo = K2_GET_CONTAINER(BLOCKIO, pListLink, DevNodeBlockIoListLink);
            if (apDevice == pOtherBlockIo->mpDriverContext)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    if (NULL != pListLink)
    {
        stat = K2STAT_ERROR_ALREADY_EXISTS;
    }
    else
    {
        K2LIST_AddAtTail(&pDevNode->InSec.BlockIo.List, &pNewBlockIo->DevNodeBlockIoListLink);
        stat = K2STAT_NO_ERROR;
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    if (!K2STAT_IS_ERROR(stat))
    {
        //
        // new blockio track
        //
        K2OS_CritSec_Enter(&sgBlockIoListSec);
        K2LIST_AddAtTail(&sgBlockIoList, &pNewBlockIo->GlobalListLink);
        K2OS_CritSec_Leave(&sgBlockIoListSec);

        //
        // calls can come in before we exit this function
        //
        pNewBlockIo->mRpcIfInst = K2OS_RpcObj_AddIfInst(
            pNewBlockIo->mRpcObj,
            K2OS_IFACE_CLASSCODE_STORAGE_DEVICE,
            &sBlockIoIfaceId,
            &pNewBlockIo->mRpcIfInstId,
            TRUE
        );

        if (NULL == pNewBlockIo->mRpcIfInst)
        {
            stat = K2OS_Thread_GetLastStatus();

            K2_ASSERT(K2STAT_IS_ERROR(stat));

            K2OS_CritSec_Enter(&pDevNode->Sec);
            K2LIST_Remove(&pDevNode->InSec.BlockIo.List, &pNewBlockIo->DevNodeBlockIoListLink);
            K2OS_CritSec_Leave(&pDevNode->Sec);

            K2OS_CritSec_Enter(&sgBlockIoListSec);
            K2LIST_Remove(&sgBlockIoList, &pNewBlockIo->GlobalListLink);
            K2OS_CritSec_Leave(&sgBlockIoListSec);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Rpc_Release(pNewBlockIo->mRpcObjHandle);
        K2_ASSERT(NULL == pNewBlockIo->mRpcObj);
        BlockIo_Release(pNewBlockIo);
    }

    return stat;
}

K2STAT
K2OSDDK_BlockIoDeregister(
    K2OS_DEVCTX aDevCtx,
    void *      apDevice
)
{
    DEVNODE *           pDevNode;
    BLOCKIO *           pBlockIo;
    K2LIST_LINK *       pListLink;
    K2STAT              stat;
    BOOL                ok;
    K2OS_RPC_OBJ_HANDLE hRpcObj;

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    pListLink = pDevNode->InSec.BlockIo.List.mpHead;
    if (NULL != pListLink)
    {
        do {
            pBlockIo = K2_GET_CONTAINER(BLOCKIO, pListLink, DevNodeBlockIoListLink);
            if (pBlockIo->mpDriverContext == apDevice)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
    if (NULL == pListLink)
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        //
        // waiting inside devnode sec!
        //
        K2RUNDOWN_Wait(&pBlockIo->Rundown);

        K2LIST_Remove(&pDevNode->InSec.BlockIo.List, &pBlockIo->DevNodeBlockIoListLink);

        //
        // rundown will prevent use of any of this
        //
        pBlockIo->mpDevNode = NULL;
        pBlockIo->mpDriverContext = NULL;

        stat = K2STAT_NO_ERROR;
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    if (K2STAT_IS_ERROR(stat))
        return stat;

    K2OS_CritSec_Enter(&pBlockIo->Sec);

    ok = K2OS_RpcObj_RemoveIfInst(pBlockIo->mRpcObj, pBlockIo->mRpcIfInst);
    pBlockIo->mRpcIfInst = NULL;
    pBlockIo->mRpcIfInstId = 0;
    K2_ASSERT(ok);
    hRpcObj = pBlockIo->mRpcObjHandle;
    pBlockIo->mRpcObjHandle = NULL;
    K2MEM_Zero(&pBlockIo->Register, sizeof(K2OSDDK_BLOCKIO_REGISTER));
    pBlockIo->mpCurSession = NULL;

    K2OS_CritSec_Leave(&pBlockIo->Sec);

    K2OS_Rpc_Release(hRpcObj);

    BlockIo_Release(pBlockIo);

    return K2STAT_NO_ERROR;
}

BLOCKIO *   
BlockIo_AcquireByIfInstId(
    K2OS_IFINST_ID aIfInstId
)
{
    K2LIST_LINK *   pListLink;
    BLOCKIO *       pBlockIo;

    pBlockIo = NULL;

    K2OS_CritSec_Enter(&sgBlockIoListSec);

    pListLink = sgBlockIoList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pBlockIo = K2_GET_CONTAINER(BLOCKIO, pListLink, GlobalListLink);
            if (pBlockIo->mRpcIfInstId == aIfInstId)
            {
                BlockIo_AddRef(pBlockIo);
                break;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&sgBlockIoListSec);

    if (NULL == pListLink)
        return NULL;

    return pBlockIo;
}

void
BlockIo_Init(
    void
)
{
    if (!K2OS_CritSec_Init(&sgBlockIoListSec))
    {
        K2OSKERN_Panic("Could not init blockio list cs\n");
    }
    K2LIST_Init(&sgBlockIoList);

    if (NULL == K2OS_RpcServer_Register(&sgBlockIoRpcClassDef, 0))
    {
        K2OSKERN_Panic("*** could not register blockio class\n");
    }
}