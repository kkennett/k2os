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

#include "pcnet32.h"

BOOL    
PCNET32_GetState(
    PCNET32_DEVICE *apDevice,
    BOOL *          apRetPhysConn,
    BOOL *          apRetIsUp
)
{
    if (NULL != apRetPhysConn)
    {
        // is cable connected?
        *apRetPhysConn = TRUE;
    }
    if (NULL != apRetIsUp)
    {
        // link state
        *apRetIsUp = TRUE;
    }
    return TRUE;
}

BOOL    
PCNET32_DoneRecv(
    PCNET32_DEVICE *apDevice,
    UINT32          aBufIx
)
{
    BOOL                        disp;
    PCNET32_RX_HDR volatile *   pRx;

    K2_ASSERT(aBufIx < RX_COUNT);

    disp = K2OSKERN_SeqLock(&apDevice->SeqLock);

    // aBufIx may be out of sequence from ring next

    pRx = (PCNET32_RX_HDR volatile *)
        (apDevice->mRingsVirt + 
         MAP_OFFSET_RX_RING + 
         (aBufIx * sizeof(PCNET32_RX_HDR))
        );
    pRx->mBufPhys = apDevice->mpFramePhysAddrs[aBufIx];
    pRx->mUserSpace = aBufIx;
    pRx->mMsgCount = 0;
    pRx->mBufLen = (UINT16)(-ETHER_FRAME_ALIGN_BYTES);
    K2_CpuWriteBarrier();

    K2ATOMIC_Dec((INT32 volatile *)&apDevice->mRx_RingBufsFull);

    // as soon as this is set, adapter can receive into buffer
    // on another core in the interrupt THREAD
    // which is not restricted by the seqlock
    pRx->mStatus = (UINT16)PCNET32_DESC_ADAPTER_OWNS;
    K2_CpuWriteBarrier();

    K2OSKERN_SeqUnlock(&apDevice->SeqLock, disp);

    return TRUE;
}

BOOL    
PCNET32_Xmit(
    PCNET32_DEVICE *apDevice,
    UINT32          aBufIx,
    UINT32          aDataLen
)
{
    PCNET32_TX_HDR volatile * pTx;

    if (!apDevice->mUserEnable)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ENABLED);
        return FALSE;
    }

    if ((aDataLen == 0) || (aDataLen > ETHER_FRAME_BYTES))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2_ASSERT(aBufIx >= RX_COUNT);
    K2_ASSERT(aBufIx < RX_COUNT + TX_COUNT);

    K2OS_CritSec_Enter(&apDevice->TxSec);

    if (0 == apDevice->mTx_Avail)
    {
        pTx = NULL;
    }
    else
    {
        pTx = (PCNET32_TX_HDR volatile *)(apDevice->mRingsVirt + MAP_OFFSET_TX_RING + (apDevice->mTx_NextRingBufToUse * sizeof(PCNET32_TX_HDR)));

        K2_ASSERT(0 == (pTx->mStatus & PCNET32_DESC_ADAPTER_OWNS));

        K2ATOMIC_Dec((INT32 volatile *)&apDevice->mTx_Avail);
        if (++apDevice->mTx_NextRingBufToUse == TX_COUNT)
            apDevice->mTx_NextRingBufToUse = 0;

        pTx->mBufPhys = apDevice->mpFramePhysAddrs[aBufIx];
        pTx->mBufLen = (UINT16)-aDataLen;
        pTx->mUserSpace = aBufIx;
        K2_CpuWriteBarrier();

        // as soon as we set this tx interrupt can occur
        pTx->mStatus = PCNET32_DESC_ADAPTER_OWNS | PCNET32_DESC_STP | PCNET32_DESC_ENP;
        K2_CpuWriteBarrier();
    }

    K2OS_CritSec_Leave(&apDevice->TxSec);

    if (NULL == pTx)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_FULL);
        return FALSE;
    }

    return TRUE;
}
