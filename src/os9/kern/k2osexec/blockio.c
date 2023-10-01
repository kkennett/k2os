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
BlockIo_Locked_RangeCreate(
    BLOCKIO *   apBlockIo,
    UINT32      aStartBlock,
    UINT32      aBlockCount,
    UINT32      aOwner,
    BOOL        aMakePrivate,
    UINT32 *    apRetRange
)
{
    BLOCKIO_RANGE * pNewRange;
    K2STAT          stat;
    K2LIST_LINK *   pListLink;
    BLOCKIO_RANGE * pScan;

    pNewRange = K2OS_Heap_Alloc(sizeof(BLOCKIO_RANGE));
    if (NULL == pNewRange)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pNewRange, sizeof(BLOCKIO_RANGE));
    pNewRange->mOwner = aOwner;
    pNewRange->mId = ++apBlockIo->mLastRangeId;
    pNewRange->mStartBlock = aStartBlock;
    pNewRange->mBlockCount = aBlockCount;
    pNewRange->mPrivate = aMakePrivate;

    stat = K2STAT_NO_ERROR;

    do {
        pListLink = apBlockIo->RangeList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pScan = K2_GET_CONTAINER(BLOCKIO_RANGE, pListLink, ListLink);
                if (aStartBlock < pScan->mStartBlock)
                {
                    if ((pScan->mStartBlock - aStartBlock) < aBlockCount)
                    {
                        // range collision
                        if (aMakePrivate || pScan->mPrivate)
                        {
                            stat = K2STAT_ERROR_ALREADY_RESERVED;
                            break;
                        }
                    }
                }
                else
                {
                    if ((aStartBlock - pScan->mStartBlock) < pScan->mBlockCount)
                    {
                        // range collision
                        if (aMakePrivate || pScan->mPrivate)
                        {
                            stat = K2STAT_ERROR_ALREADY_RESERVED;
                        }
                    }
                }

                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }

        if (K2STAT_IS_ERROR(stat))
            break;

        K2LIST_AddAtTail(&apBlockIo->RangeList, &pNewRange->ListLink);
        *apRetRange = pNewRange->mId;
        pNewRange = NULL;

    } while (0);

    if (NULL != pNewRange)
    {
        K2OS_Heap_Free(pNewRange);
    }

    return stat;
}

K2STAT
BlockIo_Locked_RangeDelete(
    BLOCKIO *   apBlockIo,
    UINT32      aRange,
    UINT32      aOwner
)
{
    K2LIST_LINK *   pListLink;
    BLOCKIO_RANGE * pScan;

    pListLink = apBlockIo->RangeList.mpHead;
    if (NULL == pListLink)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    do {
        pScan = K2_GET_CONTAINER(BLOCKIO_RANGE, pListLink, ListLink);
        if ((aOwner == pScan->mOwner) && (aRange == pScan->mId))
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    if (NULL != pListLink)
    {
        K2LIST_Remove(&apBlockIo->RangeList, &pScan->ListLink);
        K2OS_Heap_Free(pScan);
        return K2STAT_NO_ERROR;
    }

    return K2STAT_ERROR_NOT_FOUND;
}

BOOL
BlockIo_ValidateRange(
    K2OSEXEC_IOTHREAD * apIoThread,
    K2OS_BLOCKIO_RANGE  aRange,
    UINT32              aBytesOffset,
    UINT32              aByteCount,
    UINT32              aMemPageOffset
)
{
    BLOCKIO *           pBlockIo;
    K2LIST_LINK *       pListLink;
    BLOCKIO_RANGE *     pRange;
    BOOL                ok;
    UINT32              rangeBytes;

    pBlockIo = K2_GET_CONTAINER(BLOCKIO, apIoThread, IoThread);

    ok = FALSE;

    K2OS_CritSec_Enter(&pBlockIo->Sec);

    if (0 != pBlockIo->mMediaBlockCount)
    {
        // there is media
        K2_ASSERT(0 != pBlockIo->mMediaBlockSizeBytes);
        if ((aByteCount > 0) && 
            (0 == (aMemPageOffset % pBlockIo->mMediaBlockSizeBytes)) &&
            (0 == (aBytesOffset % pBlockIo->mMediaBlockSizeBytes)) &&
            (0 == (aByteCount % pBlockIo->mMediaBlockSizeBytes)))
        {
            // byte count exists and it and memory page offset are aligned to block size
            pListLink = pBlockIo->RangeList.mpHead;
            if (NULL != pListLink)
            {
                // some range exists
                do {
                    pRange = K2_GET_CONTAINER(BLOCKIO_RANGE, pListLink, ListLink);
                    if (pRange->mId == (UINT32)aRange)
                        break;
                    pListLink = pListLink->mpNext;
                } while (NULL != pListLink);
                if (NULL != pListLink)
                {
                    // the desired range exists
                    rangeBytes = pRange->mBlockCount * pBlockIo->mMediaBlockSizeBytes;
                    if ((aBytesOffset < rangeBytes) &&
                        ((rangeBytes - aBytesOffset) >= aByteCount))
                    {
                        // the span requested fits inside the range
                        ok = TRUE;
                    }
                }
            }
        }
    }

    K2OS_CritSec_Leave(&pBlockIo->Sec);

    return ok;
}

void
BlockIo_Submit(
    K2OSEXEC_IOTHREAD * apIoThread,
    K2OSEXEC_IOP *      apIop
)
{
    BLOCKIO *           pBlockIo;
    BOOL                setNotify;
    BOOL                completeNow;
    UINT32              result;
    K2STAT              stat;
    K2_EXCEPTION_TRAP   trap;
    K2OS_STORAGE_MEDIA  storMedia;
    K2LIST_LINK *       pListLink;
    BLOCKIO_RANGE *     pRange;
    UINT32              rangeBytes;

    //
    // we are on the kernel's iomgr thread here
    // so we move the iop to the device thread
    //

    pBlockIo = K2_GET_CONTAINER(BLOCKIO, apIoThread, IoThread);
    setNotify = FALSE;
    result = 0;
    completeNow = TRUE;

    K2_ASSERT(K2OSExecIop_BlockIo == apIop->mType);

    K2OS_CritSec_Enter(&pBlockIo->Sec);

    if (apIop->Op.BlockIo.mOp == K2OSExecIop_BlockIoOp_GetMedia)
    {
        stat = K2_EXTRAP(&trap, pBlockIo->Register.GetMedia(pBlockIo->mpDriverContext, &storMedia));
        if (K2STAT_IS_ERROR(stat))
        {
            pBlockIo->mMediaBlockCount = 0;
            pBlockIo->mMediaBlockSizeBytes = 0;
        }
        else
        {
            pBlockIo->mMediaBlockCount = storMedia.mBlockCount;
            pBlockIo->mMediaBlockSizeBytes = storMedia.mBlockSizeBytes;
            K2MEM_Copy((void *)apIop->Op.BlockIo.mMemAddr, &storMedia, sizeof(K2OS_STORAGE_MEDIA));
            result = TRUE;
        }
    }
    else if (0 == pBlockIo->mMediaBlockCount)
    {
        stat = K2STAT_ERROR_UNKNOWN_MEDIA_SIZE;
    }
    else
    {
        if ((apIop->Op.BlockIo.mOp == K2OSExecIop_BlockIoOp_Read) ||
            (apIop->Op.BlockIo.mOp == K2OSExecIop_BlockIoOp_Write) ||
            (apIop->Op.BlockIo.mOp == K2OSExecIop_BlockIoOp_Erase))
        {
            // these ops are done in bytes
            if ((0 != (apIop->Op.BlockIo.mOffset % pBlockIo->mMediaBlockSizeBytes)) ||
                (0 != (apIop->Op.BlockIo.mCount % pBlockIo->mMediaBlockSizeBytes)))
            {
                stat = K2STAT_ERROR_BAD_ALIGNMENT;
            }
            else
            {
                //
                // find range
                //
                pListLink = pBlockIo->RangeList.mpHead;
                if (NULL == pListLink)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                }
                else
                {
                    do {
                        pRange = K2_GET_CONTAINER(BLOCKIO_RANGE, pListLink, ListLink);
                        if (pRange->mId == apIop->Op.BlockIo.mRange)
                            break;
                        pListLink = pListLink->mpNext;
                    } while (NULL != pListLink);
                    if (NULL == pListLink)
                    {
                        stat = K2STAT_ERROR_NOT_FOUND;
                    }
                    else
                    {
                        rangeBytes = pRange->mBlockCount * pBlockIo->mMediaBlockSizeBytes;
                        if ((apIop->Op.BlockIo.mOffset >= rangeBytes) ||
                            ((rangeBytes - apIop->Op.BlockIo.mOffset) < apIop->Op.BlockIo.mCount))
                        {
                            K2OSKERN_Debug("%s:%d\n", __FUNCTION__, __LINE__);
                            stat = K2STAT_ERROR_OUT_OF_BOUNDS;
                        }
                        else
                        {
                            completeNow = FALSE;
                            if (apIop->Op.BlockIo.mOp != K2OSExecIop_BlockIoOp_Erase)
                            {
                                // pBlockIo->mMediaBlockCount checked for zero already above
                                if (0 != (apIop->Op.BlockIo.mMemAddr % pBlockIo->mMediaBlockSizeBytes))
                                {
                                    // memory address is not blocksize aligned
                                    stat = K2STAT_ERROR_BAD_ALIGNMENT;
                                    completeNow = TRUE;
                                }
                            }
                            if (!completeNow)
                            {
                                // adjust start block for range and put onto device thread work queue
                                apIop->Op.BlockIo.mOffset += (pRange->mStartBlock * pBlockIo->mMediaBlockSizeBytes);
                                completeNow = FALSE;
                                K2LIST_AddAtTail(&pBlockIo->IopList, &apIop->ListLink);
                                setNotify = (pBlockIo->IopList.mNodeCount == 1) ? TRUE : FALSE;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // range op
            if (apIop->Op.BlockIo.mOp == K2OSExecIop_BlockIoOp_RangeCreate)
            {
                if ((apIop->Op.BlockIo.mOffset >= pBlockIo->mMediaBlockCount) ||
                    ((pBlockIo->mMediaBlockCount - apIop->Op.BlockIo.mOffset) < apIop->Op.BlockIo.mCount))
                {
                    K2OSKERN_Debug("%s:%d\n", __FUNCTION__, __LINE__);
                    stat = K2STAT_ERROR_OUT_OF_BOUNDS;
                }
                else
                {
                    stat = BlockIo_Locked_RangeCreate(
                        pBlockIo,
                        apIop->Op.BlockIo.mOffset,
                        apIop->Op.BlockIo.mCount,
                        apIop->mOwner,
                        (BOOL)apIop->Op.BlockIo.mRange,
                        &result
                    );
                }
            }
            else
            {
                K2_ASSERT(apIop->Op.BlockIo.mOp == K2OSExecIop_BlockIoOp_RangeDelete);
                stat = BlockIo_Locked_RangeDelete(
                    pBlockIo,
                    apIop->Op.BlockIo.mRange,
                    apIop->mOwner
                );
                if (!K2STAT_IS_ERROR(stat))
                {
                    result = TRUE;
                }
            }
        }
    }

    K2OS_CritSec_Leave(&pBlockIo->Sec);

    if (completeNow)
    {
        gKernDdk.Io_Complete(&pBlockIo->IoThread, apIop, result, K2STAT_IS_ERROR(stat), stat);
    }
    else if (setNotify)
    {
        K2OS_Notify_Signal(pBlockIo->mTokNotify);
    }
}

K2STAT
BlockIo_Cancel(
    K2OSEXEC_IOTHREAD * apIoThread,
    K2OSEXEC_IOP *      apIop
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

void
BlockIo_DeviceThread_ExecIop(
    BLOCKIO *       apBlockIo,
    K2OSEXEC_IOP *  apIop
)
{
    K2STAT                  stat;
    UINT32                  result;
    K2_EXCEPTION_TRAP       trap;
    K2OS_BLOCKIO_TRANSFER   transfer;

    K2MEM_Zero(&transfer, sizeof(transfer));

    K2OS_CritSec_Enter(&apBlockIo->Sec);

    if (0 == apBlockIo->mMediaBlockCount)
    {
        stat = K2STAT_ERROR_MEDIA_CHANGED;

        K2OS_CritSec_Leave(&apBlockIo->Sec);
    }
    else
    {
        transfer.mStartBlock = apIop->Op.BlockIo.mOffset / apBlockIo->mMediaBlockSizeBytes;
        transfer.mBlockCount = apIop->Op.BlockIo.mCount / apBlockIo->mMediaBlockSizeBytes;
        if (apIop->Op.BlockIo.mOp == K2OSExecIop_BlockIoOp_Read)
        {
            transfer.mTargetAddr = apIop->Op.BlockIo.mMemAddr;
            transfer.mType = K2OS_BlockIoTransfer_Read;
        }
        else if (apIop->Op.BlockIo.mOp == K2OS_BlockIoTransfer_Write)
        {
            transfer.mTargetAddr = apIop->Op.BlockIo.mMemAddr;
            transfer.mType = K2OS_BlockIoTransfer_Write;
        }
        else
        {
            transfer.mTargetAddr = 0;
            transfer.mType = K2OS_BlockIoTransfer_Erase;
        }

        K2OS_CritSec_Leave(&apBlockIo->Sec);

        stat = K2_EXTRAP(&trap, apBlockIo->Register.Transfer(apBlockIo->mpDriverContext, &transfer));
    }

    result = K2STAT_IS_ERROR(stat) ? FALSE : TRUE;

    gKernDdk.Io_Complete(&apBlockIo->IoThread, apIop, result, !result, stat);
}

UINT32
BlockIo_DeviceThread(
    void *apArg
)
{
    static K2_GUID128 sBlockIoClassId = K2OS_IFACE_BLOCKIO_DEVICE_CLASSID;

    BLOCKIO *               pThis;
    BOOL                    ok;
    K2OS_WaitResult         waitResult;
    K2STAT                  stat;
    K2OSEXEC_IOP *          pIop;
    K2LIST_LINK *           pHead;

    pThis = (BLOCKIO *)apArg;

    ok = K2OS_CritSec_Init(&pThis->Sec);
    K2_ASSERT(ok);

    pThis->mTokNotify = K2OS_Notify_Create(FALSE);
    K2_ASSERT(NULL != pThis->mTokNotify);

    pThis->IoThread.mUsePhysAddr = pThis->Register.Config.mUseHwDma;
    pThis->IoThread.ValidateRange = BlockIo_ValidateRange;
    pThis->IoThread.Submit = BlockIo_Submit;
    pThis->IoThread.Cancel = BlockIo_Cancel;

    stat = gKernDdk.Io_SetThread(pThis->mIfInstId, &pThis->IoThread);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Token_Destroy(pThis->mTokNotify);
        pThis->mTokNotify = NULL;
        K2OS_CritSec_Done(&pThis->Sec);
        return stat;
    }

    //
    // once we publish we can immediately start getting submissions
    //
    ok = K2OS_IfInst_Publish(pThis->mTokIfInst, K2OS_IFACE_CLASSCODE_STORAGE_DEVICE, &sBlockIoClassId);
    K2_ASSERT(ok);

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, pThis->mTokNotify, K2OS_TIMEOUT_INFINITE))
            break;

        do {
            K2OS_CritSec_Enter(&pThis->Sec);

            pHead = pThis->IopList.mpHead;
            if (NULL != pHead)
            {
                K2LIST_Remove(&pThis->IopList, pHead);
            }

            K2OS_CritSec_Leave(&pThis->Sec);

            if (NULL == pHead)
                break;

            pIop = K2_GET_CONTAINER(K2OSEXEC_IOP, pHead, ListLink);

            BlockIo_DeviceThread_ExecIop(pThis, pIop);

        } while (1);

    } while (1);

    return 0;
}

