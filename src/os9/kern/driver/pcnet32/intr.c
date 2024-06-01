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

#define ANY_INTR (CSR0_TINT_W1C | CSR0_RINT_W1C | CSR0_ERR_RO)

KernIntrDispType
PCNET32_Isr(
    void *              apKey,
    KernIntrActionType  aAction
)
{
    PCNET32_DEVICE *    pDev;
    UINT32              csr0;
    BOOL                disp;

    pDev = K2_GET_CONTAINER(PCNET32_DEVICE, apKey, mIrqHookKey);

    disp = K2OSKERN_SeqLock(&pDev->SeqLock);

    csr0 = PCNET32_ReadCSR(pDev, 0);

    K2_ASSERT(0 != (csr0 & CSR0_INTR_RO));

    // clear IENA, which turns off PCI interrupt
    PCNET32_WriteCSR(pDev, 0, 0);

    K2OSKERN_SeqUnlock(&pDev->SeqLock, disp);

    // go to IST to handle everything
    return KernIntrDisp_Fire;
}

UINT32
PCNET32_ServiceThread(
    PCNET32_DEVICE *apDevice
)
{
    K2OS_WaitResult             waitResult;
    UINT32                      csr0;
    UINT32                      w1c;
    BOOL                        disp;
    PCNET32_RX_HDR volatile *   pRx;
    PCNET32_TX_HDR volatile *   pTx;

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, apDevice->mTokIntr, K2OS_TIMEOUT_INFINITE))
        {
            K2OSKERN_Debug("PCNET32(%08X): IST wait for interrupt failed!\n", apDevice);
            break;
        }

        do {
            // get and clear current interrupt status
            w1c = 0;

            disp = K2OSKERN_SeqLock(&apDevice->SeqLock);

            csr0 = PCNET32_ReadCSR(apDevice, 0);

            w1c = (csr0 & (CSR0_TINT_W1C | CSR0_RINT_W1C));
            if (0 == w1c)
                break;

            PCNET32_WriteCSR(apDevice, 0, w1c);

            K2OSKERN_SeqUnlock(&apDevice->SeqLock, disp);

            if (0 != (w1c & CSR0_TINT_W1C))
            {
//                K2OSKERN_Debug("TX INTR\n");

                // transmit interrupted so there must be an outstanding frame
                K2_ASSERT(TX_COUNT != apDevice->mTx_Avail);

                do {
                    pTx = (PCNET32_TX_HDR volatile *)(apDevice->mRingsVirt + MAP_OFFSET_TX_RING + (apDevice->mTx_NextRingBufDone * sizeof(PCNET32_TX_HDR)));
//                    K2OSKERN_Debug("TX_NEXTDONE = %08X; phys = %08X; status = %04X\n", pTx, pTx->mBufPhys, pTx->mStatus);
                    if (0 != (pTx->mStatus & PCNET32_DESC_ADAPTER_OWNS))
                    {
                        // next tx done is owned by adapter
                        break;
                    }

                    // nextringbufdone frame went out
                    if (++apDevice->mTx_NextRingBufDone == TX_COUNT)
                        apDevice->mTx_NextRingBufDone = 0;
                    K2ATOMIC_Inc((INT32 volatile *)&apDevice->mTx_Avail);

                    // next tx done is not owned by adapter (i.e. it's done)
                    if ((NULL != apDevice->mpXmitDoneKey) &&
                        (NULL != *(apDevice->mpXmitDoneKey)))
                    {
                        (*apDevice->mpXmitDoneKey)(apDevice->mpXmitDoneKey, apDevice->mDevCtx, apDevice, pTx->mUserSpace);
                    }

                } while (TX_COUNT != apDevice->mTx_Avail);
            }

            if (0 != (w1c & CSR0_RINT_W1C))
            {
//                K2OSKERN_Debug("RX INTR\n");

                if (RX_COUNT == apDevice->mRx_RingBufsFull)
                {
                    K2OSKERN_Debug("RX INTR signalled buf no free buffers-------------------------------------------\n");
                }
                else
                {
                    // receive interrupted.  send frame(s) up
                    do {
                        pRx = (PCNET32_RX_HDR volatile *)(apDevice->mRingsVirt + MAP_OFFSET_RX_RING + (apDevice->mRx_RingNext * sizeof(PCNET32_RX_HDR)));
//                        K2OSKERN_Debug("RX_NEXTBUF pRx = %08X; phys = %08X; status = %04X\n", pRx, pRx->mBufPhys, pRx->mStatus);
                        if (0 != (pRx->mStatus & PCNET32_DESC_ADAPTER_OWNS))
                        {
                            // next rx is owned by adapter
                            break;
                        }

                        // if we are full we should not have been able to receive
                        K2_ASSERT(RX_COUNT != apDevice->mRx_RingBufsFull);

                        // frame came in
                        if (++apDevice->mRx_RingNext == RX_COUNT)
                            apDevice->mRx_RingNext = 0;
                        K2ATOMIC_Inc((INT32 volatile *)&apDevice->mRx_RingBufsFull);

                        if ((NULL != apDevice->mpRecvKey) &&
                            (NULL != *(apDevice->mpRecvKey)))
                        {
                            (*apDevice->mpRecvKey)(apDevice->mpRecvKey, apDevice->mDevCtx, apDevice, pRx->mBufPhys, pRx->mMsgCount);
                        }

                        // stop if we bump up against next rx input (frame returned by user)
                    } while (RX_COUNT != apDevice->mRx_RingBufsFull);
                }
            }

        } while (1);

        // re-enable interrupts and exit back to intr wait
        PCNET32_WriteCSR(apDevice, 0, CSR0_IENA);

        K2OSKERN_IntrDone(apDevice->mTokIntr);

        K2OSKERN_SeqUnlock(&apDevice->SeqLock, disp);

    } while (1);

    return 0;
}

