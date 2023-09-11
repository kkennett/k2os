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

#include "ide.h"

K2STAT 
IDE_InitAndDiscover(
    void
)
{
    UINT32          ixIo;
    UINT32          ixPhys;
    UINT32          barToIx[5];
    BOOL            barIsPhys[5];
    UINT32          ixDev;
    BOOL            isATA;
    BOOL            isATAPI;
    UINT8           status;
    UINT8           cl;
    UINT8           ch;
    UINT32          ixData;
    UINT16 *        pData;
    IDE_DEVICE *    pDevice;
    UINT8 *         pCheck;

    K2MEM_Zero(&gChannel[0], sizeof(IDE_CHANNEL) * 2);

    gChannel[ATA_CHANNEL_PRIMARY].mIndex = ATA_CHANNEL_PRIMARY;
    gChannel[ATA_CHANNEL_PRIMARY].Device[ATA_DEVICE_MASTER].mIndex = ATA_DEVICE_MASTER;
    gChannel[ATA_CHANNEL_PRIMARY].Device[ATA_DEVICE_MASTER].mpChannel = &gChannel[ATA_CHANNEL_PRIMARY];
    gChannel[ATA_CHANNEL_PRIMARY].Device[ATA_DEVICE_SLAVE].mIndex = ATA_DEVICE_SLAVE;
    gChannel[ATA_CHANNEL_PRIMARY].Device[ATA_DEVICE_SLAVE].mpChannel = &gChannel[ATA_CHANNEL_PRIMARY];

    gChannel[ATA_CHANNEL_SECONDARY].mIndex = ATA_CHANNEL_SECONDARY;
    gChannel[ATA_CHANNEL_SECONDARY].Device[ATA_DEVICE_MASTER].mIndex = ATA_DEVICE_MASTER;
    gChannel[ATA_CHANNEL_SECONDARY].Device[ATA_DEVICE_MASTER].mpChannel = &gChannel[ATA_CHANNEL_SECONDARY];
    gChannel[ATA_CHANNEL_SECONDARY].Device[ATA_DEVICE_SLAVE].mIndex = ATA_DEVICE_SLAVE;
    gChannel[ATA_CHANNEL_SECONDARY].Device[ATA_DEVICE_SLAVE].mpChannel = &gChannel[ATA_CHANNEL_SECONDARY];

    gBusMasterAddr = 0;
    ixIo = 0;
    ixPhys = 0;

    K2MEM_Zero(barToIx, sizeof(barToIx));
    K2MEM_Zero(barIsPhys, sizeof(barIsPhys));

    // 
    // find bars
    //
    for (ixIo = 0; ixIo < gInst.mCountIo; ixIo++)
    {
        K2_ASSERT(gResIo[ixIo].mId < 5);
        barToIx[gResIo[ixIo].mId] = ixIo;
    }
    for (ixPhys = 0; ixPhys < gInst.mCountPhys; ixPhys++)
    {
        K2_ASSERT(gResPhys[ixPhys].mId < 5);
        barToIx[gResPhys[ixPhys].mId] = ixPhys;
        barIsPhys[gResPhys[ixPhys].mId] = TRUE;
    }

#if 0
    K2OSDrv_DebugPrintf("BAR[0] is %s index %d\n", barIsPhys[0] ? "PHYS" : " IO ", barToIx[0]);
    K2OSDrv_DebugPrintf("BAR[1] is %s index %d\n", barIsPhys[1] ? "PHYS" : " IO ", barToIx[1]);
    K2OSDrv_DebugPrintf("BAR[2] is %s index %d\n", barIsPhys[2] ? "PHYS" : " IO ", barToIx[2]);
    K2OSDrv_DebugPrintf("BAR[3] is %s index %d\n", barIsPhys[3] ? "PHYS" : " IO ", barToIx[3]);
    K2OSDrv_DebugPrintf("BAR[4] is %s index %d\n", barIsPhys[4] ? "PHYS" : " IO ", barToIx[4]);
#endif

    //
    // primary channel
    //
    if (barIsPhys[barToIx[0]])
    {
        if (!barIsPhys[barToIx[1]])
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): primary channel interface split between io and phys\n", K2OS_Process_GetId());
            return K2STAT_ERROR_UNSUPPORTED;
        }

        if (gResPhys[barToIx[0]].Range.mSizeBytes != 8)
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): primary channel base phys range is wrong size (%d)\n", K2OS_Process_GetId(), gResPhys[barToIx[0]].Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        gChannel[ATA_CHANNEL_PRIMARY].Regs.mBase = gResPhys[barToIx[0]].Range.mBaseAddr;

        if (gResPhys[barToIx[1]].Range.mSizeBytes != 4)
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): primary channel control phys range is wrong size (%d)\n", K2OS_Process_GetId(), gResPhys[barToIx[1]].Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        gChannel[ATA_CHANNEL_PRIMARY].Regs.mControl = gResPhys[barToIx[1]].Range.mBaseAddr;

        gChannel[ATA_CHANNEL_PRIMARY].Regs.mIsPhys = TRUE;
    }
    else
    {
        if (barIsPhys[barToIx[1]])
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): primary channel interface split between phys and io\n", K2OS_Process_GetId());
            return K2STAT_ERROR_UNSUPPORTED;
        }

        if (gResIo[barToIx[0]].Range.mSizeBytes != 8)
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): primary channel base io range is wrong size (%d)\n", K2OS_Process_GetId(), gResIo[barToIx[0]].Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        gChannel[ATA_CHANNEL_PRIMARY].Regs.mBase = gResIo[barToIx[0]].Range.mBasePort;

        if (gResIo[barToIx[1]].Range.mSizeBytes != 4)
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): primary channel control io range is wrong size (%d)\n", K2OS_Process_GetId(), gResIo[barToIx[1]].Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        gChannel[ATA_CHANNEL_PRIMARY].Regs.mControl = gResIo[barToIx[1]].Range.mBasePort;

        gChannel[ATA_CHANNEL_PRIMARY].Regs.mIsPhys = FALSE;
    }
    if (gInst.mCountIrq > 0)
    {
        gChannel[ATA_CHANNEL_PRIMARY].mpIrq = &gResIrq[0];
    }

    //
    // secondary channel
    //
    if (barIsPhys[barToIx[2]])
    {
        if (!barIsPhys[barToIx[3]])
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): secondary channel interface split between io and phys\n", K2OS_Process_GetId());
            return K2STAT_ERROR_UNSUPPORTED;
        }

        if (gResPhys[barToIx[2]].Range.mSizeBytes != 8)
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): secondary channel base phys range is wrong size (%d)\n", K2OS_Process_GetId(), gResPhys[barToIx[2]].Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        gChannel[ATA_CHANNEL_SECONDARY].Regs.mBase = gResPhys[barToIx[2]].Range.mBaseAddr;

        if (gResPhys[barToIx[3]].Range.mSizeBytes != 4)
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): secondary channel control phys range is wrong size (%d)\n", K2OS_Process_GetId(), gResPhys[barToIx[3]].Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        gChannel[ATA_CHANNEL_SECONDARY].Regs.mControl = gResPhys[barToIx[3]].Range.mBaseAddr;

        gChannel[ATA_CHANNEL_SECONDARY].Regs.mIsPhys = TRUE;
    }
    else
    {
        if (barIsPhys[barToIx[3]])
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): secondary channel interface split between phys and io\n", K2OS_Process_GetId());
            return K2STAT_ERROR_UNSUPPORTED;
        }

        if (gResIo[barToIx[2]].Range.mSizeBytes != 8)
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): secondary channel base io range is wrong size (%d)\n", K2OS_Process_GetId(), gResIo[barToIx[2]].Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        gChannel[ATA_CHANNEL_SECONDARY].Regs.mBase = gResIo[barToIx[2]].Range.mBasePort;

        if (gResIo[barToIx[3]].Range.mSizeBytes != 4)
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): secondary channel control io range is wrong size (%d)\n", K2OS_Process_GetId(), gResIo[barToIx[3]].Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        gChannel[ATA_CHANNEL_SECONDARY].Regs.mControl = gResIo[barToIx[3]].Range.mBasePort;

        gChannel[ATA_CHANNEL_SECONDARY].Regs.mIsPhys = FALSE;
    }
    if (gInst.mCountIrq > 1)
    {
        gChannel[ATA_CHANNEL_SECONDARY].mpIrq = &gResIrq[1];
    }
    else if (gInst.mCountIrq > 0)
    {
        gChannel[ATA_CHANNEL_SECONDARY].mpIrq = &gResIrq[0];
    }

    if (5 == (gInst.mCountIo + gInst.mCountPhys))
    {
        if (barIsPhys[barToIx[4]])
        {
            if (gResPhys[barToIx[4]].Range.mSizeBytes != 16)
            {
                K2OSDrv_DebugPrintf("*** Ide(%d): bus master phys range is the wrong size (%d)\n", K2OS_Process_GetId(), gResPhys[barToIx[4]].Range.mSizeBytes);
                return K2STAT_ERROR_UNSUPPORTED;
            }
            gBusMasterAddr = gResPhys[barToIx[4]].Range.mBaseAddr;
            gBusMasterIsPhys = TRUE;
        }
        else
        {
            if (gResIo[barToIx[4]].Range.mSizeBytes != 16)
            {
                K2OSDrv_DebugPrintf("*** Ide(%d): bus master io range is the wrong size (%d)\n", K2OS_Process_GetId(), gResIo[barToIx[4]].Range.mSizeBytes);
                return K2STAT_ERROR_UNSUPPORTED;
            }
            gBusMasterAddr = gResIo[barToIx[4]].Range.mBasePort;
            gBusMasterIsPhys = FALSE;
        }
    }
    else
    {
        //
        // no bus master interface
        //
        gBusMasterAddr = 0;
        gBusMasterIsPhys = FALSE;
    }

#if 0
    K2OSDrv_DebugPrintf("IDE primary channel base @         %04X\n", gChannel[ATA_CHANNEL_PRIMARY].Regs.mBase);
    K2OSDrv_DebugPrintf("IDE primary channel ctrl @         %04X\n", gChannel[ATA_CHANNEL_PRIMARY].Regs.mControl);
    K2OSDrv_DebugPrintf("IDE primary channel is Phys        %s\n", gChannel[ATA_CHANNEL_PRIMARY].Regs.mIsPhys ? "YES" : "NO");
    K2OSDrv_DebugPrintf("IDE primary channel IRQ            %d\n", (gChannel[ATA_CHANNEL_PRIMARY].mpIrq == NULL) ? 255 : gChannel[ATA_CHANNEL_PRIMARY].mpIrq->Config.mSourceIrq);

    K2OSDrv_DebugPrintf("\nIDE secondary channel base @       %04X\n", gChannel[ATA_CHANNEL_SECONDARY].Regs.mBase);
    K2OSDrv_DebugPrintf("IDE secondary channel ctrl @       %04X\n", gChannel[ATA_CHANNEL_SECONDARY].Regs.mControl);
    K2OSDrv_DebugPrintf("IDE secondary channel is Phys      %s\n", gChannel[ATA_CHANNEL_SECONDARY].Regs.mIsPhys ? "YES" : "NO");
    K2OSDrv_DebugPrintf("IDE secondary channel IRQ          %d\n", (gChannel[ATA_CHANNEL_SECONDARY].mpIrq == NULL) ? 255 : gChannel[ATA_CHANNEL_SECONDARY].mpIrq->Config.mSourceIrq);

    K2OSDrv_DebugPrintf("\nIDE bus master is phys             %s\n", gBusMasterIsPhys ? "YES" : "NO");
    K2OSDrv_DebugPrintf("IDE bus master @                   %04X\n", gBusMasterAddr);
#endif

    for (ixIo = 0; ixIo < 2; ixIo++)
    {
        gChannel[ixIo].RegAddr.RW_DATA = gChannel[ixIo].Regs.mBase;
        gChannel[ixIo].RegAddr.R_ERROR = gChannel[ixIo].Regs.mBase + 1;
        gChannel[ixIo].RegAddr.W_FEATURES = gChannel[ixIo].Regs.mBase + 1;
        gChannel[ixIo].RegAddr.RW_SECCOUNT0 = gChannel[ixIo].Regs.mBase + 2;
        gChannel[ixIo].RegAddr.RW_LBA_LOW = gChannel[ixIo].Regs.mBase + 3;
        gChannel[ixIo].RegAddr.RW_LBA_MID = gChannel[ixIo].Regs.mBase + 4;
        gChannel[ixIo].RegAddr.RW_LBA_HI = gChannel[ixIo].Regs.mBase + 5;
        gChannel[ixIo].RegAddr.RW_HDDEVSEL = gChannel[ixIo].Regs.mBase + 6;
        gChannel[ixIo].RegAddr.W_COMMAND = gChannel[ixIo].Regs.mBase + 7;
        gChannel[ixIo].RegAddr.R_STATUS = gChannel[ixIo].Regs.mBase + 7;
        gChannel[ixIo].RegAddr.R_ASR = gChannel[ixIo].Regs.mControl + 2;
        gChannel[ixIo].RegAddr.W_DCR = gChannel[ixIo].Regs.mControl + 2;
    }

    //
    // regio will work now. 
    //

    //
    // disable irqs and issue soft reset
    //
    IDE_Write8_DCR(ATA_CHANNEL_PRIMARY, ATA_DCR_INT_MASK | ATA_DCR_SW_RESET);
    IDE_Write8_DCR(ATA_CHANNEL_SECONDARY, ATA_DCR_INT_MASK | ATA_DCR_SW_RESET);
    K2OS_Thread_Sleep(5);
    IDE_Write8_DCR(ATA_CHANNEL_PRIMARY, ATA_DCR_INT_MASK);
    IDE_Write8_DCR(ATA_CHANNEL_SECONDARY, ATA_DCR_INT_MASK);
    K2OS_Thread_Sleep(10);
    do {
        status = IDE_Read8_STATUS(ATA_CHANNEL_PRIMARY);
    } while (status & ATA_SR_BSY);
    do {
        status = IDE_Read8_STATUS(ATA_CHANNEL_SECONDARY);
    } while (status & ATA_SR_BSY);
    gChannel[ATA_CHANNEL_PRIMARY].mIrqMaskFlag = ATA_DCR_INT_MASK;
    gChannel[ATA_CHANNEL_SECONDARY].mIrqMaskFlag = ATA_DCR_INT_MASK;

    //
    // reset completed
    //
    for (ixIo = 0; ixIo < 2; ixIo++)
    {
        for (ixDev = 0; ixDev < 2; ixDev++)
        {
            pDevice = &gChannel[ixIo].Device[ixDev];

            pDevice->mLocation = (ixIo << 1) | ixDev;

            // select device
            IDE_Write8_HDDEVSEL(ixIo, ATA_HDDSEL_STATIC | ((ixDev != 0) ? ATA_HDDSEL_DEVICE_SEL : 0));

            // pause 1ms (400ns needed but we do 1ms because time)
            K2OS_Thread_Sleep(1);

            status = IDE_Read8_STATUS(ixIo);
            do {
                if (0 == (status & ATA_SR_BSY))
                    break;
                status = IDE_Read8_STATUS(ixIo);
            } while (1);

            // send identify command
            IDE_Write8_COMMAND(ixIo, ATA_CMD_IDENTIFY);

            // check if no device
            status = IDE_Read8_STATUS(ixIo);
            if (0 == status)
            {
                //                K2OSDrv_DebugPrintf("%d/%d - NO DEVICE(1)\n", ixIo, ixDev);
                continue;
            }
            if (status & ATA_SR_DF)
            {
                // DWF on identify - probably nothing there
//                K2OSDrv_DebugPrintf("%d/%d - NO DEVICE(2)\n", ixIo, ixDev);
                continue;
            }

            isATA = TRUE;
            do {
                if (status & ATA_SR_ERR)
                {
                    // device is not ATA
                    isATA = FALSE;
                    break;
                }
                if ((0 == (status & ATA_SR_BSY)) &&
                    (0 != (status & ATA_SR_DRQ)))
                {
                    // not busy and data ready
                    break;
                }
                status = IDE_Read8_STATUS(ixIo);
            } while (1);

            isATAPI = FALSE;
            if (!isATA)
            {
                cl = IDE_Read8_LBA_MID(ixIo);
                ch = IDE_Read8_LBA_HI(ixIo);
                if (!(((cl == 0x14) && (ch == 0xEB)) ||
                    ((cl == 0x69) && (ch == 0x96))))
                {
                    // dont know what this is
                    continue;
                }

                isATAPI = TRUE;
                IDE_Write8_COMMAND(ixIo, ATA_CMD_IDENTIFY_PACKET);

                // pause 1ms
                K2OS_Thread_Sleep(1);
            }

            // wait not busy
            status = IDE_Read8_STATUS(ixIo);
            do {
                if (0 == (status & ATA_SR_BSY))
                    break;
                status = IDE_Read8_STATUS(ixIo);
            } while (1);

            // read back initial identification data here
            pData = (UINT16 *)&pDevice->AtaIdent;
            for (ixData = 0; ixData < 256; ixData++)
            {
                *pData = IDE_Read16_DATA(ixIo);
                pData++;
            }

            // initial ident must have correct signature
            if (ATA_IDENTIFY_SIG_CORRECT != (pDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_SIG_MASK))
            {
                K2OSDrv_DebugPrintf(
                    "IDE device %d/%d initial IDENTIFY signature incorrect (0x%02X)\n",
                    ixIo, ixDev,
                    pDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_SIG_MASK);
                continue;
            }

            // initial identify must have correct checksum
            pCheck = (UINT8 *)&pDevice->AtaIdent;
            cl = 0xA5;
            for (ixData = 0; ixData < 510; ixData++)
            {
                cl += *pCheck;
                pCheck++;
            }
            cl = ~cl + 1;
            if (cl != ((pDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_CHKSUM_MASK) >> ATA_IDENTIFY_SIGCHECK_CHKSUM_SHL))
            {
                K2OSDrv_DebugPrintf(
                    "IDE device %d/%d IDENTIFY checksum incorrect (0x%02X)\n",
                    ixIo, ixDev,
                    (pDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_CHKSUM_MASK) >> ATA_IDENTIFY_SIGCHECK_CHKSUM_SHL);
                continue;
            }

            // major revision is mandatory fixed; must be ata6+ compatible
            if (0 == (pDevice->AtaIdent.MajorRevision & ATA_IDENTIFY_VERSION_SUPP_ATA6))
            {
                K2OSDrv_DebugPrintf("IDE device %d/%d does not support ATA6, device ignored.\n", ixIo, ixDev);
                continue;
            }

            // removable device flag is mandatory fixed
            if (pDevice->AtaIdent.GenConfig & ATA_IDENTIFY_GENCONFIG_MEDIA_IS_REMOVABLE)
            {
                pDevice->mIsRemovable = TRUE;
                if (isATAPI)
                {
                    // removable device implements PACKET feature set
                    // so turn on the "removable media status notification" feature
                    if (pDevice->AtaIdent.CommandSetSupport2 & ATA_IDENTIFY_CMDSET2_MSN)
                    {
                        pDevice->mHasMSN = TRUE;
                    }
                    else
                    {
                        pDevice->mHasMSN = FALSE;
                    }
                }
                else
                {
                    // removable device does NOT implement the PACKET feature set
                    // so turn on the "removable media" feature
                    pDevice->mHasMSN = FALSE;
                }

                if (pDevice->mHasMSN)
                {
                    // send SET_FEATURES command, subcommand enable MSN
                    IDE_Write8_HDDEVSEL(ixIo, ATA_HDDSEL_STATIC | ((ixDev != 0) ? ATA_HDDSEL_DEVICE_SEL : 0));
                    // pause 1ms (400ns needed but we do 1ms because time)
                    K2OS_Thread_Sleep(1);
                    status = IDE_Read8_STATUS(ixIo);
                    do {
                        if (0 == (status & ATA_SR_BSY))
                            break;
                        status = IDE_Read8_STATUS(ixIo);
                    } while (1);
                    IDE_Write8_FEATURES(ixIo, 0x95);
                    IDE_Write8_COMMAND(ixIo, ATA_CMD_SET_FEATURES);
                    // wait not busy
                    status = IDE_Read8_STATUS(ixIo);
                    do {
                        if (0 == (status & ATA_SR_BSY))
                            break;
                        status = IDE_Read8_STATUS(ixIo);
                    } while (1);
                    if (0 != (0x40 & IDE_Read8_ERROR(ixIo)))
                    {
                        // attempt to enable MSN failed
                        pDevice->mHasMSN = FALSE;
                    }
                }
            }
            else
            {
                pDevice->mIsRemovable = FALSE;
            }

            // caps/lba support is mandatroy fixed
            if (0 != (pDevice->AtaIdent.Caps & ATA_IDENTIFY_CAPS_LBA_SUPPORTED))
            {
                // this will be re-evaluated every time media changes but we do it
                // here to set the atamode to something
                if (pDevice->AtaIdent.UserAddressableSectors >= 0x10000000)
                {
                    pDevice->mAtaMode = ATA_MODE_LBA48;
                }
                else
                {
                    pDevice->mAtaMode = ATA_MODE_LBA28;
                }
            }
            else
            {
                // chs determines total addressible sectors
                pDevice->mAtaMode = ATA_MODE_CHS;
            }

            pDevice->mState = IdeState_EvalIdent;
            pDevice->mIsATAPI = isATAPI;
            pDevice->mRpcObjHandle = K2OS_Rpc_CreateObj(0, &gIdeBlockIoDevice_ObjectClassDef.ClassId, (UINT32)pDevice);
            if (NULL != pDevice->mRpcObj)
            {
                K2_ASSERT(NULL != pDevice->mRpcObj);
                if (K2OS_CritSec_Init(&pDevice->Sec))
                {
                    gPopMask |= (1 << pDevice->mLocation);
                }
            }
        }
    }

    // enable interrupts on channels

    if (gPopMask & 0x3)
    {
        // primary channel has something on it
        IDE_Write8_DCR(ATA_CHANNEL_PRIMARY, 0);
        gChannel[ATA_CHANNEL_PRIMARY].mIrqMaskFlag = 0;

        K2OS_Thread_Sleep(10);
        do {
            status = IDE_Read8_STATUS(ATA_CHANNEL_PRIMARY);
        } while (status & ATA_SR_BSY);
    }

    if (gPopMask & 0xC)
    {
        // secondary channel has something on it
        IDE_Write8_DCR(ATA_CHANNEL_SECONDARY, 0);
        gChannel[ATA_CHANNEL_SECONDARY].mIrqMaskFlag = 0;
        do {
            status = IDE_Read8_STATUS(ATA_CHANNEL_SECONDARY);
        } while (status & ATA_SR_BSY);
    }

    return K2STAT_NO_ERROR;
}

