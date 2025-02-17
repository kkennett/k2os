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

#define FORCE_IO_REGS   0

K2STAT
CreateInstance(
    K2OS_DEVCTX aDevCtx,
    void **     appRetDriverContext
)
{
    PCNET32_DEVICE *    pDevice;
    K2STAT              stat;
    UINT32              totalBufs;
    UINT32              align;
    UINT32              bufsPerPage;
    UINT32              needPages;
    UINT32              u;
    UINT32              p;
    UINT32              frameIx;

    pDevice = (PCNET32_DEVICE *)K2OS_Heap_Alloc(sizeof(PCNET32_DEVICE));
    if (NULL == pDevice)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pDevice, sizeof(PCNET32_DEVICE));

    K2OSKERN_SeqInit(&pDevice->SeqLock);

    if (!K2OS_CritSec_Init(&pDevice->TxSec))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    do {
        pDevice->mTokRingsPageArray = K2OSDDK_PageArray_CreateIo(0, 1, &pDevice->mRingsPhys);
        if (NULL == pDevice->mTokRingsPageArray)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            K2OSKERN_Debug("PCNET32(%08X): failed to alloc phys page for init+desc block (%08X)\n", pDevice, stat);
            break;
        }

        do {
            pDevice->mRingsVirt = K2OS_Virt_Reserve(1);
            if (0 == pDevice->mRingsVirt)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OSKERN_Debug("PCNET32(%08X): failed to alloc virt page for init+desc block (%08X)\n", pDevice, stat);
                break;
            }

            do {
                pDevice->mTokRingsVirtMap = K2OS_VirtMap_Create(
                    pDevice->mTokRingsPageArray,
                    0, 1,
                    pDevice->mRingsVirt,
                    K2OS_MapType_MemMappedIo_ReadWrite
                );
                if (NULL == pDevice->mTokRingsVirtMap)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    K2OSKERN_Debug("PCNET32(%08X): failed to map init+desc block (%08X)\n", pDevice, stat);
                    break;
                }

                do {
                    totalBufs = RX_COUNT + TX_COUNT;
                    align = ((ETHER_FRAME_FULL_LENGTH + (K2OS_CACHELINE_BYTES - 1)) / K2OS_CACHELINE_BYTES) * K2OS_CACHELINE_BYTES;
                    bufsPerPage = K2_VA_MEMPAGE_BYTES / align;
                    K2_ASSERT(0 != bufsPerPage);
                    needPages = (totalBufs + (bufsPerPage - 1)) / bufsPerPage;
                    pDevice->mFramesPagesCount = needPages;

                    // 
                    // round up needPages to power of 2 for contiguous physical alloc
                    //
                    if (0 != (needPages & (needPages - 1)))
                    {
                        // not a power of 2 pages
                        do {
                            // isolate lowest bit set
                            u = needPages - 1;
                            u = u ^ (needPages | u);
                            needPages &= ~u;
                        } while (0 != (needPages & (needPages - 1)));
                        // needPages is now a power of two
                        needPages <<= 1;
                    }

                    pDevice->mpFramePhysAddrs = (UINT32 *)K2OS_Heap_Alloc(sizeof(UINT32) * totalBufs);
                    if (NULL == pDevice->mpFramePhysAddrs)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(stat));
                        K2OSKERN_Debug("PCNET32(%08X): failed to allocate memory for phys buf addresses (%08X)\n", pDevice, stat);
                        break;
                    }

                    pDevice->mTokFramesPageArray = K2OSDDK_PageArray_CreateIo(0, needPages, &pDevice->mFramesPhys);
                    if (NULL == pDevice->mTokFramesPageArray)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(stat));
                        K2OSKERN_Debug("PCNET32(%08X): failed to alloc pagearray for frames (%08X)\n", pDevice, stat);
                    }
                    else
                    {
                        K2_ASSERT(0 != pDevice->mFramesPhys);

                        //
                        // put cacheline-aligned non-page-boundary-crossing physical buffer addresses into the array
                        //
                        frameIx = 0;
                        u = pDevice->mFramesPhys;
                        do {
                            p = bufsPerPage;
                            do {
                                pDevice->mpFramePhysAddrs[frameIx] = u;
                                u += align;
                                if (++frameIx == totalBufs)
                                    break;
                            } while (--p > 0);
                            // page align for next set
                            u = ((u + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
                        } while (frameIx < totalBufs);

                        stat = K2STAT_NO_ERROR;
                    }

                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OS_Heap_Free(pDevice->mpFramePhysAddrs);
                    }

                } while (0);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_Token_Destroy(pDevice->mTokRingsVirtMap);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Virt_Release(pDevice->mRingsVirt);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Token_Destroy(pDevice->mTokRingsPageArray);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_CritSec_Done(&pDevice->TxSec);
        K2OS_Heap_Free(pDevice);
    }
    else
    {
        pDevice->mDevCtx = aDevCtx;
        *appRetDriverContext = pDevice;
    }

    return stat;
}

K2STAT
Init(
    PCNET32_DEVICE * apDevice
)
{
    UINT32                  u32;
    UINT32                  countPow;
    UINT8                   checkMac[8];
    PCNET32_INIT volatile * pInit;
    PCNET32_RX_HDR *        pRxDesc;
    PCNET32_TX_HDR *        pTxDesc;
    K2OSDDK_NETIO_REGISTER  netReg;
    K2STAT                  stat;
    K2OS_PAGEARRAY_TOKEN    cloneTokFramesPageArray;

    //
    // enable bus master if not set
    //
    u32 = PCNET32_ReadPciCfg(apDevice, PCI_CONFIG_TYPEX_OFFSET_COMMAND, 16);
    if (0 == (u32 & PCI_CMDREG_BUSMASTER_ENABLE))
    {
        u32 |= PCI_CMDREG_BUSMASTER_ENABLE;
        PCNET32_WritePciCfg(apDevice, PCI_CONFIG_TYPEX_OFFSET_COMMAND, 16, u32);
    }

    //
    // set PCI latency timer
    //
    PCNET32_WritePciCfg(apDevice, PCI_CONFIG_TYPEX_OFFSET_LATENCYTIMER, 8, 0x20);

    //
    // reset the unit now
    //
    PCNET32_ReadRESET(apDevice);        // will issue S_RESET
    K2OS_Thread_Sleep(5);               // needs 1ms, give it 5
    PCNET32_WriteRESET(apDevice, 0);    // has no effect on some cards
    K2OS_Thread_Sleep(5);               // *may* need 1ms

    //
    // set to 32-bit mode
    //
    PCNET32_WriteRDP(apDevice, 0);
    // we are in 32-bit IO mode now.  Can never go back until another reset

    if (0 != PCNET32_ReadRAP(apDevice))
    {
        K2OSKERN_Debug("PCNET32(%08X): RAP read did not return 0 after reset\n", apDevice);
        return K2STAT_ERROR_HARDWARE;
    }

    u32 = PCNET32_ReadCSR(apDevice, 88) | (PCNET32_ReadCSR(apDevice, 89) << 16);
    if (0x02625000 != (u32 & 0xFFFFF000))
    {
        K2OSKERN_Debug("PCNET32(%08X): Chip version 0x%08X\n", apDevice, u32);
        K2OSKERN_Debug("PCNET32(%08X): Unsupported (not \"PCnet/FAST III 79C973\")\n", apDevice);
        return K2STAT_ERROR_NOT_IMPL;
    }

    u32 = PCNET32_ReadCSR(apDevice, 0);
    if (0x0004 != (u32 & 0x0007))
    {
        K2OSKERN_Debug("PCNET32(%08X): Adapter did not reset to stopped state\n", apDevice);
        return K2STAT_ERROR_HARDWARE;
    }

    if (apDevice->mMappedIo)
    {
        u32 = K2MMIO_Read32(apDevice->mRegsVirtAddr);
        K2MEM_Copy(&apDevice->mCacheAPROM[0], &u32, sizeof(UINT32));
        u32 = K2MMIO_Read32(apDevice->mRegsVirtAddr + 4);
        K2MEM_Copy(&apDevice->mCacheAPROM[4], &u32, sizeof(UINT32));
        u32 = K2MMIO_Read32(apDevice->mRegsVirtAddr + 8);
        K2MEM_Copy(&apDevice->mCacheAPROM[8], &u32, sizeof(UINT32));
        u32 = K2MMIO_Read32(apDevice->mRegsVirtAddr + 12);
        K2MEM_Copy(&apDevice->mCacheAPROM[12], &u32, sizeof(UINT32));
    }
#if K2_TARGET_ARCH_IS_INTEL
    else
    {
        for (u32 = 0; u32 < PCNET32_APROM_LENGTH; u32++)
        {
            apDevice->mCacheAPROM[u32] = X32_IoRead8(apDevice->Res.Def.Io.Range.mBasePort + u32);
        }
    }
#endif

    // configure SRAM size to split between TX/RX
    PCNET32_WriteBCR(apDevice, 25, 0x17);
    PCNET32_WriteBCR(apDevice, 26, 0x0C);

    //
    // CSR 12-14 has currently active MAC address
    //
    u32 = PCNET32_ReadCSR(apDevice, 12);
    K2MEM_Copy(&checkMac[0], &u32, sizeof(UINT16));
    u32 = PCNET32_ReadCSR(apDevice, 13);
    K2MEM_Copy(&checkMac[2], &u32, sizeof(UINT16));
    u32 = PCNET32_ReadCSR(apDevice, 14);
    K2MEM_Copy(&checkMac[4], &u32, sizeof(UINT16));
    if (0 != K2MEM_Compare(checkMac, apDevice->mCacheAPROM, ETHER_FRAME_MAC_LENGTH))
    {
        K2OSKERN_Debug("PCNET32(%08X): MAC in registers does not match MAC in prom.  Using one in PROM.");
        u32 = (UINT32)(*((UINT16 *)&apDevice->mCacheAPROM[0]));
        PCNET32_WriteCSR(apDevice, 12, (UINT16)u32);
        u32 = (UINT32)(*((UINT16 *)&apDevice->mCacheAPROM[2]));
        PCNET32_WriteCSR(apDevice, 13, (UINT16)u32);
        u32 = (UINT32)(*((UINT16 *)&apDevice->mCacheAPROM[4]));
        PCNET32_WriteCSR(apDevice, 14, (UINT16)u32);
    }

    //
    // BCR20 needs to have SWSTYLE set to 3.
    //
    u32 = PCNET32_ReadBCR(apDevice, 20);
    if ((u32 & 0xFF) != 3)
    {
        u32 = (u32 & ~0xFF) | 3;
        PCNET32_WriteBCR(apDevice, 20, u32);
        u32 = PCNET32_ReadBCR(apDevice, 20);
        if (0 == (u32 & (1 << 8)))
        {
            K2OSKERN_Debug("PCNET32(%08X): could not set SWSTYLE to 3, readback %08X\n", apDevice, u32);
            return K2STAT_ERROR_HARDWARE;
        }
    }

    //
    // turn off DANAS and turn off forcing PHY to full duplex
    // turn on PHY autonegotiation
    //
    u32 = PCNET32_ReadBCR(apDevice, 32);
    u32 &= ~0x98;
    u32 |= 0x20;
    PCNET32_WriteBCR(apDevice, 32, (UINT16)u32);

    //
    // enable NOUFLO - no underflow on transmit (table 27 - transmit only on full packet)
    // enable Burst mode
    //
    u32 = PCNET32_ReadBCR(apDevice, 18);
    u32 |= (1 << 11);
    u32 |= (1 << 6) | (1 << 5); // Burst enable (sw style 3)
    PCNET32_WriteBCR(apDevice, 18, u32);
    PCNET32_WriteCSR(apDevice, 80, PCNET32_ReadCSR(apDevice, 80) | (3 << 10));

    //
    // set up init and ring descriptors
    //

    pInit = (PCNET32_INIT volatile *)(apDevice->mRingsVirt + MAP_OFFSET_INIT);
    K2MEM_Zero((void *)pInit, sizeof(PCNET32_INIT));
    pInit->MODE = CSR15_PORTSEL_MASK | CSR15_DTX | CSR15_DRX;
    pInit->RLEN = RX_COUNT_POW2;
    pInit->TLEN = TX_COUNT_POW2;
    K2MEM_Copy((UINT8 *)&pInit->PADR, &apDevice->mCacheAPROM, ETHER_FRAME_MAC_LENGTH);
    pInit->LADRF = 0;
    pInit->RDRA = apDevice->mRingsPhys + MAP_OFFSET_RX_RING;
    pInit->TDRA = apDevice->mRingsPhys + MAP_OFFSET_TX_RING;

    //
    // initial rx/tx setup - all buffers owned by driver (own bit clear)
    //
    pRxDesc = (PCNET32_RX_HDR *)(apDevice->mRingsVirt + MAP_OFFSET_RX_RING);
    K2MEM_Zero(pRxDesc, sizeof(PCNET32_RX_HDR) * RX_COUNT);
    pTxDesc = (PCNET32_TX_HDR *)(apDevice->mRingsVirt + MAP_OFFSET_TX_RING);
    K2MEM_Zero(pTxDesc, sizeof(PCNET32_TX_HDR) * TX_COUNT);

    K2_CpuWriteBarrier();

    // 
    // set init block address
    //
    u32 = apDevice->mRingsPhys + MAP_OFFSET_INIT;
    PCNET32_WriteCSR(apDevice, 1, (UINT16)(u32 & 0xFFFF));
    PCNET32_WriteCSR(apDevice, 2, (UINT16)((u32 >> 16) & 0xFFFF));

    //
    // mask interrupt on init done (doing it via polling)
    // also mask missed frame interrupt because we don't care
    //
    u32 = PCNET32_ReadCSR(apDevice, 3);
    u32 |= CSR3_IDONM | CSR3_MISSM;
    PCNET32_WriteCSR(apDevice, 3, (UINT16)u32);

    //
    // configure for enable
    //
    PCNET32_WriteCSR(apDevice, 4, 0x0915);  // APAD_XMIT, MFCOM, RCVCCOM, TXSTRTM

    //
    // seng INIT command
    //
    PCNET32_WriteCSR(apDevice, 0, CSR0_INIT_CMD);

    //
    // wait for INIT done (IDON) with timeout
    //
    countPow = 150;
    do {
        K2OS_Thread_Sleep(1);
        u32 = PCNET32_ReadCSR(apDevice, 0);
    } while ((0 == (CSR0_IDON_W1C & u32)) && (0 != --countPow));
    if (0 == countPow)
    {
        K2OSKERN_Debug("PCNET32(%08X): Failed to respond to INIT command\n", apDevice);
        PCNET32_WriteCSR(apDevice, 0, 4);
        return K2STAT_ERROR_HARDWARE;
    }

    //
    // clear init done and leave interrupts enabled
    //
    PCNET32_WriteCSR(apDevice, 0, CSR0_IDON_W1C);

    if (!K2OS_Token_Clone(apDevice->mTokFramesPageArray, &cloneTokFramesPageArray))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        PCNET32_WriteCSR(apDevice, 0, 4);
        return stat;
    }

    K2MEM_Zero(&netReg, sizeof(netReg));
    K2ASC_CopyLen(netReg.Desc.mName, "PCNet32", K2_NET_ADAPTER_NAME_MAX_LEN);
    netReg.Desc.mNetAdapterType = K2_NetAdapter_Ethernet;
    netReg.Desc.mPhysicalMTU = ETHER_FRAME_HDR_LENGTH + ETHER_FRAME_MTU;    // crc auto
    netReg.Desc.Addr.mLen = ETHER_FRAME_MAC_LENGTH;
    K2MEM_Copy(&netReg.Desc.Addr.mValue, apDevice->mCacheAPROM, ETHER_FRAME_MAC_LENGTH);
    netReg.BufCounts.mRecv = RX_COUNT;
    netReg.BufCounts.mXmit = TX_COUNT;
    netReg.mBufsPhysBaseAddr = apDevice->mFramesPhys;
    netReg.mpBufsPhysAddrs = apDevice->mpFramePhysAddrs;
    netReg.SetEnable = (K2OSDDK_pf_NetIo_SetEnable)PCNET32_SetEnable;
    netReg.GetState = (K2OSDDK_pf_NetIo_GetState)PCNET32_GetState;
    netReg.DoneRecv = (K2OSDDK_pf_NetIo_DoneRecv)PCNET32_DoneRecv;
    netReg.Xmit = (K2OSDDK_pf_NetIo_Xmit)PCNET32_Xmit;
    stat = K2OSDDK_NetIoRegister(
        apDevice->mDevCtx,
        apDevice,
        &netReg,
        cloneTokFramesPageArray,
        &apDevice->mpRecvKey,
        &apDevice->mpXmitDoneKey,
        &apDevice->mpNotifyKey
    );
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("PCNET32(%08X): Failed to register network device\n", apDevice);
        K2OS_Token_Destroy(cloneTokFramesPageArray);
        PCNET32_WriteCSR(apDevice, 0, 4);
        return stat;
    }

    //
    // exec owns cloned frames pagearray token now
    //

    apDevice->mTokIntrThread = K2OS_Thread_Create(
        "PCNET32_IST",
        (K2OS_pf_THREAD_ENTRY)PCNET32_ServiceThread,
        apDevice,
        NULL,
        &apDevice->mIntrThreadId
    );
    if (NULL == apDevice->mTokIntrThread)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OSKERN_Debug("PCNET32(%08X): Failed to start interrupt thread\n", apDevice);
        K2OSDDK_NetIoDeregister(apDevice->mDevCtx, apDevice);
        PCNET32_WriteCSR(apDevice, 0, 4);
    }
//    else
//    {
//        K2OSKERN_Debug("PCNET32(%08X) initialized\n");
//    }

    return stat;
}

K2STAT
StartDriver(
    PCNET32_DEVICE * apDevice
)
{
    K2STAT  stat;
    UINT32  u32;

    stat = K2OSDDK_GetInstanceInfo(apDevice->mDevCtx, &apDevice->InstInfo);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PCNET32(%08X): Enum resources failed (0x%08X)\n", apDevice, stat);
        return stat;
    }

    do {
        if (apDevice->InstInfo.mBusType != K2OS_BUSTYPE_PCI)
        {
            K2OSKERN_Debug("*** PCNET32(%08X): bustype %d unexpected, should be %d\n", apDevice, apDevice->InstInfo.mBusType, K2OS_BUSTYPE_PCI);
            return K2STAT_ERROR_HARDWARE;
        }

        apDevice->mhBusRpc = K2OS_Rpc_AttachByIfInstId(apDevice->InstInfo.mBusIfInstId, NULL);
        if (NULL == apDevice->mhBusRpc)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            K2OSKERN_Debug("*** PCNET32(%08X): Count not attach to parent bus RPC instanceid %d\n", apDevice, apDevice->InstInfo.mBusIfInstId);
            break;
        }

        do {
            u32 = PCNET32_ReadPciCfg(apDevice, PCI_CONFIG_TYPEX_OFFSET_VENDORID, 16) << 16;
            u32 |= PCNET32_ReadPciCfg(apDevice, PCI_CONFIG_TYPEX_OFFSET_DEVICEID, 16);
            if (u32 != 0x10222000)
            {
                K2OSKERN_Debug("PCNET32(%08X): VEND|DEVI(%08X) is not 0x10222000.\n", apDevice, u32);
                stat = K2STAT_ERROR_NOT_IMPL;
                break;
            }

            do {
                u32 = PCNET32_ReadPciCfg(apDevice, PCI_CONFIG_TYPEX_OFFSET_COMMAND, 16);
                if ((0 != apDevice->InstInfo.mCountPhys) && (0 != (u32 & PCI_CMDREG_MEM_ENABLE)))
                {
                    apDevice->mMappedIo = TRUE;
                    stat = K2OSDDK_GetRes(apDevice->mDevCtx, K2OS_RESTYPE_PHYS, 0, &apDevice->Res);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OSKERN_Debug("*** PCNET32(%08X): Count not get physical base address of adapter\n", apDevice);
                        break;
                    }
                }
                else if ((0 == apDevice->InstInfo.mCountIo) || (0 == (u32 & PCI_CMDREG_IO_ENABLE)))
                {
                    K2OSKERN_Debug("*** PCNET32(%08X): Unable to find phys or I/O for adapter or access disabled in PCI cmd reg\n", apDevice);
                    stat = K2STAT_ERROR_NOT_CONFIGURED;
                    break;
                }
                else
                {
                    apDevice->mMappedIo = FALSE;
                    stat = K2OSDDK_GetRes(apDevice->mDevCtx, K2OS_RESTYPE_IO, 0, &apDevice->Res);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OSKERN_Debug("*** PCNET32(%08X): Count not get io base address of adapter\n", apDevice);
                        break;
                    }
                }

                if (apDevice->mMappedIo)
                {
                    do {
                        apDevice->mRegsPageCount = K2OS_PageArray_GetLength(apDevice->Res.Phys.mTokPageArray);
                        if (0 == apDevice->mRegsPageCount)
                        {
                            stat = K2OS_Thread_GetLastStatus();
                            K2_ASSERT(K2STAT_IS_ERROR(stat));
                            K2OSKERN_Debug("*** PCNET32(%08X): Count not get size of mapped io register bank\n", apDevice);
                            break;
                        }

                        apDevice->mRegsVirtAddr = K2OS_Virt_Reserve(apDevice->mRegsPageCount);
                        if (0 == apDevice->mRegsVirtAddr)
                        {
                            stat = K2OS_Thread_GetLastStatus();
                            K2_ASSERT(K2STAT_IS_ERROR(stat));
                            K2OSKERN_Debug("*** PCNET32(%08X): Count not allocate virtual mapping for memory mapped io registers\n", apDevice);
                            apDevice->mRegsPageCount = 0;
                            break;
                        }

                        apDevice->mTokRegsVirtMap = K2OS_VirtMap_Create(
                            apDevice->Res.Phys.mTokPageArray,
                            0,
                            apDevice->mRegsPageCount,
                            apDevice->mRegsVirtAddr,
                            K2OS_MapType_MemMappedIo_ReadWrite
                        );
                        if (NULL == apDevice->mTokRegsVirtMap)
                        {
                            stat = K2OS_Thread_GetLastStatus();
                            K2_ASSERT(K2STAT_IS_ERROR(stat));
                            K2OSKERN_Debug("*** PCNET32(%08X): Count not map device registers\n", apDevice);

                            K2OS_Virt_Release(apDevice->mRegsVirtAddr);
                            apDevice->mRegsVirtAddr = 0;
                            apDevice->mRegsPageCount = 0;
                            break;
                        }

                    } while (0);
                }

                if (!K2STAT_IS_ERROR(stat))
                {
                    do {
                        if (0 != apDevice->InstInfo.mCountIrq)
                        {
                            stat = K2OSDDK_GetRes(apDevice->mDevCtx, K2OS_RESTYPE_IRQ, 0, &apDevice->ResIrq);
                            if (K2STAT_IS_ERROR(stat))
                            {
                                K2OSKERN_Debug("*** PCNET32(%08X): Count not get irq of adapter\n", apDevice);
                                break;
                            }

                            if (!K2OSKERN_IrqDefine(&apDevice->ResIrq.Def.Irq.Config))
                            {
                                stat = K2OS_Thread_GetLastStatus();
                                K2_ASSERT(K2STAT_IS_ERROR(stat));
                                K2MEM_Zero(&apDevice->ResIrq, sizeof(K2OSDDK_RES));
                                break;
                            }

                            apDevice->mIrqHookKey = PCNET32_Isr;
                            apDevice->mTokIntr = K2OSKERN_IrqHook(apDevice->ResIrq.Def.Irq.Config.mSourceIrq, &apDevice->mIrqHookKey);
                            if (NULL == apDevice->mTokIntr)
                            {
                                stat = K2OS_Thread_GetLastStatus();
                                K2_ASSERT(K2STAT_IS_ERROR(stat));
                                K2OSKERN_Debug("*** PCNET32(%08X): Failed to hook irq\n", apDevice);
                                K2MEM_Zero(&apDevice->ResIrq, sizeof(K2OSDDK_RES));
                                break;
                            }
                        }

                        stat = Init(apDevice);

                        if (K2STAT_IS_ERROR(stat))
                        {
                            if (0 != apDevice->InstInfo.mCountIrq)
                            {
                                K2OS_Token_Destroy(apDevice->mTokIntr);
                                apDevice->mTokIntr = NULL;
                                apDevice->mIrqHookKey = NULL;
                            }
                        }

                    } while (0);

                    if ((K2STAT_IS_ERROR(stat)) && (apDevice->mMappedIo))
                    {
                        K2OS_Token_Destroy(apDevice->mTokRegsVirtMap);
                        apDevice->mTokRegsVirtMap = NULL;
                        K2OS_Virt_Release(apDevice->mRegsVirtAddr);
                        apDevice->mRegsVirtAddr = 0;
                        apDevice->mRegsPageCount = 0;
                    }
                }

                if (K2STAT_IS_ERROR(stat))
                {
                    if (apDevice->mMappedIo)
                    {
                        K2OS_Token_Destroy(apDevice->Res.Phys.mTokPageArray);
                    }
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2MEM_Zero(&apDevice->Res, sizeof(K2OSDDK_RES));
                break;
            }

        } while (0);

        K2OS_Rpc_Release(apDevice->mhBusRpc);
        apDevice->mhBusRpc = NULL;

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2MEM_Zero(&apDevice->InstInfo, sizeof(K2OSDDK_INSTINFO));
    }

    return stat;
}

K2STAT
StopDriver(
    PCNET32_DEVICE * apDevice
)
{
    K2_ASSERT(!apDevice->mIsRunning);

    K2OSDDK_NetIoDeregister(apDevice->mDevCtx, apDevice);

    K2OS_Token_Destroy(apDevice->mTokIntr);
    apDevice->mTokIntr = FALSE;
    apDevice->mIrqHookKey = NULL;

    if (apDevice->mMappedIo)
    {
        if (NULL != apDevice->mTokRingsVirtMap)
        {
            K2OS_Token_Destroy(apDevice->mTokRingsVirtMap);
            apDevice->mTokRingsVirtMap = NULL;
        }

        if (0 != apDevice->mRegsVirtAddr)
        {
            K2OS_Virt_Release(apDevice->mRegsVirtAddr);
            apDevice->mRegsVirtAddr = 0;
        }

        K2OS_Token_Destroy(apDevice->Res.Phys.mTokPageArray);
        apDevice->Res.Phys.mTokPageArray = NULL;
    }

    if (NULL != apDevice->mhBusRpc)
    {
        K2OS_Rpc_Release(apDevice->mhBusRpc);
        apDevice->mhBusRpc = NULL;
    }

    K2OSDDK_DriverStopped(apDevice->mDevCtx, K2STAT_NO_ERROR);

    return K2STAT_NO_ERROR;
}

K2STAT
DeleteInstance(
    PCNET32_DEVICE * apDevice
)
{
    K2OS_Token_Destroy(apDevice->mTokFramesPageArray);
    K2OS_Heap_Free(apDevice->mpFramePhysAddrs);
    K2OS_Token_Destroy(apDevice->mTokRingsVirtMap);
    K2OS_Virt_Release(apDevice->mRingsVirt);
    K2OS_Token_Destroy(apDevice->mTokRingsPageArray);
    K2OS_CritSec_Done(&apDevice->TxSec);
    K2OS_Heap_Free(apDevice);
    return K2STAT_NO_ERROR;
}
