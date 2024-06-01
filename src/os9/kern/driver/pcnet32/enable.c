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
PCNET32_SetEnable(
    PCNET32_DEVICE *apDevice,
    BOOL            aSetEnable
)
{
    UINT32              u32;
    BOOL                result;
    BOOL                disp;
    BOOL                ok;
    PCNET32_RX_HDR *    pRx;
    PCNET32_TX_HDR *    pTx;

    result = FALSE;

    disp = K2OSKERN_SeqLock(&apDevice->SeqLock);

    if (aSetEnable != apDevice->mUserEnable)
    {
        if (aSetEnable)
        {
            pRx = (PCNET32_RX_HDR *)(apDevice->mRingsVirt + MAP_OFFSET_RX_RING);
            for (u32 = 0; u32 < RX_COUNT; u32++)
            {
                pRx->mBufPhys = apDevice->mpFramePhysAddrs[u32];
                pRx->mUserSpace = u32;
                pRx->mMsgCount = 0;
                pRx->mBufLen = (UINT16)(-ETHER_FRAME_ALIGN_BYTES);
                pRx->mStatus = (UINT16)PCNET32_DESC_ADAPTER_OWNS;
                pRx++;
            }
            apDevice->mRx_RingBufsFull = 0;
            apDevice->mRx_RingNext = 0;
            K2_CpuWriteBarrier();

            //
            // initial tx setup - buffers owned by driver
            //
            pTx = (PCNET32_TX_HDR *)(apDevice->mRingsVirt + MAP_OFFSET_TX_RING);
            K2MEM_Zero(pTx, sizeof(PCNET32_TX_HDR) * TX_COUNT);
            apDevice->mTx_Avail = TX_COUNT;
            apDevice->mTx_NextRingBufDone = 0;
            apDevice->mTx_NextRingBufToUse = 0;
            K2_CpuWriteBarrier();

            //
            // turn on TX/RX by clearing DRX and DTX. must do before STRT
            // also make sure auto-gen of FCS is not disabled
            //
            u32 = PCNET32_ReadCSR(apDevice, 15);
            u32 &= ~(CSR15_DTX | CSR15_DRX | CSR15_DXMTFCS);
            PCNET32_WriteCSR(apDevice, 15, (UINT16)u32);

            //
            // start and enable interrupts
            //
            PCNET32_WriteCSR(apDevice, 0, CSR0_STRT_CMD | CSR0_IENA);
            ok = K2OSKERN_IntrVoteIrqEnable(apDevice->mTokIntr, TRUE);
            K2_ASSERT(ok);

            apDevice->mUserEnable = TRUE;
            result = TRUE;
        }
        else
        {
            // disable
            ok = K2OSKERN_IntrVoteIrqEnable(apDevice->mTokIntr, FALSE);
            K2_ASSERT(ok);
            PCNET32_WriteCSR(apDevice, 0, CSR0_STOP_CMD);

            apDevice->mUserEnable = FALSE;
            result = TRUE;
        }
    }

    K2OSKERN_SeqUnlock(&apDevice->SeqLock, disp);

    return result;
}

