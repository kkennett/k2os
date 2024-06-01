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
    IDE_CONTROLLER *apController
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
    IDE_CHANNEL *   pChannel;

    K2MEM_Zero(barToIx, sizeof(barToIx));
    K2MEM_Zero(barIsPhys, sizeof(barIsPhys));

    // 
    // find bars
    //
    for (ixIo = 0; ixIo < apController->InstInfo.mCountIo; ixIo++)
    {
        K2_ASSERT(apController->ResIo[ixIo].Def.mId < 5);
        barToIx[apController->ResIo[ixIo].Def.mId] = ixIo;
    }
    for (ixPhys = 0; ixPhys < apController->InstInfo.mCountPhys; ixPhys++)
    {
        K2_ASSERT(apController->ResPhys[ixPhys].Def.mId < 5);
        barToIx[apController->ResPhys[ixPhys].Def.mId] = ixPhys;
        barIsPhys[apController->ResPhys[ixPhys].Def.mId] = TRUE;
    }

#if 0
    K2OSKERN_Debug("BAR[0] is %s index %d\n", barIsPhys[0] ? "PHYS" : " IO ", barToIx[0]);
    K2OSKERN_Debug("BAR[1] is %s index %d\n", barIsPhys[1] ? "PHYS" : " IO ", barToIx[1]);
    K2OSKERN_Debug("BAR[2] is %s index %d\n", barIsPhys[2] ? "PHYS" : " IO ", barToIx[2]);
    K2OSKERN_Debug("BAR[3] is %s index %d\n", barIsPhys[3] ? "PHYS" : " IO ", barToIx[3]);
    K2OSKERN_Debug("BAR[4] is %s index %d\n", barIsPhys[4] ? "PHYS" : " IO ", barToIx[4]);
#endif

    //
    // primary channel
    //
    if (barIsPhys[barToIx[0]])
    {
        if (!barIsPhys[barToIx[1]])
        {
            K2OSKERN_Debug("*** IDE(%08X): primary channel interface split between io and phys\n", apController);
            return K2STAT_ERROR_UNSUPPORTED;
        }

        if (apController->ResPhys[barToIx[0]].Def.Phys.Range.mSizeBytes != 8)
        {
            K2OSKERN_Debug("*** IDE(%08X): primary channel base phys range is wrong size (%d)\n", apController, apController->ResPhys[barToIx[0]].Def.Phys.Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mBase = apController->ResPhys[barToIx[0]].Def.Phys.Range.mBaseAddr;

        if (apController->ResPhys[barToIx[1]].Def.Phys.Range.mSizeBytes != 4)
        {
            K2OSKERN_Debug("*** IDE(%08X): primary channel control phys range is wrong size (%d)\n", apController, apController->ResPhys[barToIx[1]].Def.Phys.Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mControl = apController->ResPhys[barToIx[1]].Def.Phys.Range.mBaseAddr;
        apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mIsPhys = TRUE;
    }
    else
    {
        if (barIsPhys[barToIx[1]])
        {
            K2OSKERN_Debug("*** IDE(%d): primary channel interface split between phys and io\n", K2OS_Process_GetId());
            return K2STAT_ERROR_UNSUPPORTED;
        }

        if (apController->ResIo[barToIx[0]].Def.Io.Range.mSizeBytes != 8)
        {
            K2OSKERN_Debug("*** IDE(%d): primary channel base io range is wrong size (%d)\n", K2OS_Process_GetId(), apController->ResIo[barToIx[0]].Def.Io.Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mBase = apController->ResIo[barToIx[0]].Def.Io.Range.mBasePort;

        if (apController->ResIo[barToIx[1]].Def.Io.Range.mSizeBytes != 4)
        {
            K2OSKERN_Debug("*** IDE(%d): primary channel control io range is wrong size (%d)\n", K2OS_Process_GetId(), apController->ResIo[barToIx[1]].Def.Io.Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mControl = apController->ResIo[barToIx[1]].Def.Io.Range.mBasePort;

        apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mIsPhys = FALSE;
    }
    if (apController->InstInfo.mCountIrq > 0)
    {
        apController->Channel[IDE_CHANNEL_PRIMARY].mpIrqRes = &apController->ResIrq[0];
    }

    //
    // secondary channel
    //
    if (barIsPhys[barToIx[2]])
    {
        if (!barIsPhys[barToIx[3]])
        {
            K2OSKERN_Debug("*** IDE(%d): secondary channel interface split between io and phys\n", K2OS_Process_GetId());
            return K2STAT_ERROR_UNSUPPORTED;
        }

        if (apController->ResPhys[barToIx[2]].Def.Phys.Range.mSizeBytes != 8)
        {
            K2OSKERN_Debug("*** IDE(%d): secondary channel base phys range is wrong size (%d)\n", K2OS_Process_GetId(), apController->ResPhys[barToIx[2]].Def.Phys.Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mBase = apController->ResPhys[barToIx[2]].Def.Phys.Range.mBaseAddr;

        if (apController->ResPhys[barToIx[3]].Def.Phys.Range.mSizeBytes != 4)
        {
            K2OSKERN_Debug("*** IDE(%d): secondary channel control phys range is wrong size (%d)\n", K2OS_Process_GetId(), apController->ResPhys[barToIx[3]].Def.Phys.Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mControl = apController->ResPhys[barToIx[3]].Def.Phys.Range.mBaseAddr;

        apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mIsPhys = TRUE;
    }
    else
    {
        if (barIsPhys[barToIx[3]])
        {
            K2OSKERN_Debug("*** IDE(%d): secondary channel interface split between phys and io\n", K2OS_Process_GetId());
            return K2STAT_ERROR_UNSUPPORTED;
        }

        if (apController->ResIo[barToIx[2]].Def.Io.Range.mSizeBytes != 8)
        {
            K2OSKERN_Debug("*** IDE(%d): secondary channel base io range is wrong size (%d)\n", K2OS_Process_GetId(), apController->ResIo[barToIx[2]].Def.Io.Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mBase = apController->ResIo[barToIx[2]].Def.Io.Range.mBasePort;

        if (apController->ResIo[barToIx[3]].Def.Io.Range.mSizeBytes != 4)
        {
            K2OSKERN_Debug("*** IDE(%d): secondary channel control io range is wrong size (%d)\n", K2OS_Process_GetId(), apController->ResIo[barToIx[3]].Def.Io.Range.mSizeBytes);
            return K2STAT_ERROR_UNSUPPORTED;
        }
        apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mControl = apController->ResIo[barToIx[3]].Def.Io.Range.mBasePort;

        apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mIsPhys = FALSE;
    }
    if (apController->InstInfo.mCountIrq > 1)
    {
        apController->Channel[IDE_CHANNEL_SECONDARY].mpIrqRes = &apController->ResIrq[1];
    }
    else if (apController->InstInfo.mCountIrq > 0)
    {
        apController->Channel[IDE_CHANNEL_SECONDARY].mpIrqRes = &apController->ResIrq[0];
    }

    if (5 == (apController->InstInfo.mCountIo + apController->InstInfo.mCountPhys))
    {
        if (barIsPhys[barToIx[4]])
        {
            if (apController->ResPhys[barToIx[4]].Def.Phys.Range.mSizeBytes != 16)
            {
                K2OSKERN_Debug("*** IDE(%d): bus master phys range is the wrong size (%d)\n", K2OS_Process_GetId(), apController->ResPhys[barToIx[4]].Def.Phys.Range.mSizeBytes);
                return K2STAT_ERROR_UNSUPPORTED;
            }
            apController->mBusMasterAddr = apController->ResPhys[barToIx[4]].Def.Phys.Range.mBaseAddr;
            apController->mBusMasterIsPhys = TRUE;
        }
        else
        {
            if (apController->ResIo[barToIx[4]].Def.Io.Range.mSizeBytes != 16)
            {
                K2OSKERN_Debug("*** IDE(%d): bus master io range is the wrong size (%d)\n", K2OS_Process_GetId(), apController->ResIo[barToIx[4]].Def.Io.Range.mSizeBytes);
                return K2STAT_ERROR_UNSUPPORTED;
            }
            apController->mBusMasterAddr = apController->ResIo[barToIx[4]].Def.Io.Range.mBasePort;
            apController->mBusMasterIsPhys = FALSE;
        }
    }
    else
    {
        //
        // no bus master interface
        //
        apController->mBusMasterAddr = 0;
        apController->mBusMasterIsPhys = FALSE;
    }

#if 0
    K2OSKERN_Debug("IDE primary channel base @         %04X\n", apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mBase);
    K2OSKERN_Debug("IDE primary channel ctrl @         %04X\n", apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mControl);
    K2OSKERN_Debug("IDE primary channel is Phys        %s\n", apController->Channel[IDE_CHANNEL_PRIMARY].ChanRegs.mIsPhys ? "YES" : "NO");
    K2OSKERN_Debug("IDE primary channel IRQ            %d\n", (apController->Channel[IDE_CHANNEL_PRIMARY].mpIrqRes == NULL) ? 255 : apController->Channel[IDE_CHANNEL_PRIMARY].mpIrqRes->Def.Irq.Config.mSourceIrq);

    K2OSKERN_Debug("\nIDE secondary channel base @       %04X\n", apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mBase);
    K2OSKERN_Debug("IDE secondary channel ctrl @       %04X\n", apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mControl);
    K2OSKERN_Debug("IDE secondary channel is Phys      %s\n", apController->Channel[IDE_CHANNEL_SECONDARY].ChanRegs.mIsPhys ? "YES" : "NO");
    K2OSKERN_Debug("IDE secondary channel IRQ          %d\n", (apController->Channel[IDE_CHANNEL_SECONDARY].mpIrqRes == NULL) ? 255 : apController->Channel[IDE_CHANNEL_SECONDARY].mpIrqRes->Def.Irq.Config.mSourceIrq);

    K2OSKERN_Debug("\nIDE bus master is phys             %s\n", apController->mBusMasterIsPhys ? "YES" : "NO");
    K2OSKERN_Debug("IDE bus master @                   %04X\n", apController->mBusMasterAddr);
#endif

    for (ixIo = 0; ixIo < 2; ixIo++)
    {
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_RW_DATA] = apController->Channel[ixIo].ChanRegs.mBase;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_R_ERROR] = apController->Channel[ixIo].ChanRegs.mBase + 1;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_W_FEATURES] = apController->Channel[ixIo].ChanRegs.mBase + 1;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_RW_SECCOUNT0] = apController->Channel[ixIo].ChanRegs.mBase + 2;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_RW_LBA_LOW] = apController->Channel[ixIo].ChanRegs.mBase + 3;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_RW_LBA_MID] = apController->Channel[ixIo].ChanRegs.mBase + 4;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_RW_LBA_HI] = apController->Channel[ixIo].ChanRegs.mBase + 5;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_RW_HDDEVSEL] = apController->Channel[ixIo].ChanRegs.mBase + 6;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_W_COMMAND] = apController->Channel[ixIo].ChanRegs.mBase + 7;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_R_STATUS] = apController->Channel[ixIo].ChanRegs.mBase + 7;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_R_ASR] = apController->Channel[ixIo].ChanRegs.mControl + 2;
        apController->Channel[ixIo].ChanRegs.mRegAddr[IdeChanReg_W_DCR] = apController->Channel[ixIo].ChanRegs.mControl + 2;
    }

    //
    // regio will work now. 
    //

    //
    // disable irqs and issue soft reset
    //
    IDE_Write8(&apController->Channel[IDE_CHANNEL_PRIMARY], IdeChanReg_W_DCR, ATA_DCR_INT_MASK | ATA_DCR_SW_RESET);
    IDE_Write8(&apController->Channel[IDE_CHANNEL_SECONDARY], IdeChanReg_W_DCR, ATA_DCR_INT_MASK | ATA_DCR_SW_RESET);
    K2OS_Thread_Sleep(5);
    IDE_Write8(&apController->Channel[IDE_CHANNEL_PRIMARY], IdeChanReg_W_DCR, ATA_DCR_INT_MASK);
    IDE_Write8(&apController->Channel[IDE_CHANNEL_SECONDARY], IdeChanReg_W_DCR, ATA_DCR_INT_MASK);
    K2OS_Thread_Sleep(10);
    do {
        status = IDE_Read8(&apController->Channel[IDE_CHANNEL_PRIMARY], IdeChanReg_R_STATUS);
    } while (status & ATA_SR_BSY);
    do {
        status = IDE_Read8(&apController->Channel[IDE_CHANNEL_SECONDARY], IdeChanReg_R_STATUS);
    } while (status & ATA_SR_BSY);
    apController->Channel[IDE_CHANNEL_PRIMARY].mIrqMaskFlag = ATA_DCR_INT_MASK;
    apController->Channel[IDE_CHANNEL_SECONDARY].mIrqMaskFlag = ATA_DCR_INT_MASK;

    //
    // reset completed
    //
    for (ixIo = 0; ixIo < 2; ixIo++)
    {
        pChannel = &apController->Channel[ixIo];
        for (ixDev = 0; ixDev < 2; ixDev++)
        {
            pDevice = &pChannel->Device[ixDev];
            pDevice->mLocation = (ixIo << 1) | ixDev;

            // select device
            IDE_Write8(pChannel, IdeChanReg_RW_HDDEVSEL, ATA_HDDSEL_STATIC | ((ixDev != 0) ? ATA_HDDSEL_DEVICE_SEL : 0));

            // pause 1ms (400ns needed but we do 1ms because time)
            K2OS_Thread_Sleep(1);

            status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
            do {
                if (0 == (status & ATA_SR_BSY))
                    break;
                status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
            } while (1);

            // send identify command
            IDE_Write8(pChannel, IdeChanReg_W_COMMAND, ATA_CMD_IDENTIFY);

            // check if no device
            status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
            if (0 == status)
            {
                //                K2OSKERN_Debug("%d/%d - NO DEVICE(1)\n", ixIo, ixDev);
                continue;
            }
            if (status & ATA_SR_DF)
            {
                // DWF on identify - probably nothing there
//                K2OSKERN_Debug("%d/%d - NO DEVICE(2)\n", ixIo, ixDev);
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
                status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
            } while (1);

            isATAPI = FALSE;
            if (!isATA)
            {
                cl = IDE_Read8(pChannel, IdeChanReg_RW_LBA_MID);
                ch = IDE_Read8(pChannel, IdeChanReg_RW_LBA_HI);
                if (!(((cl == 0x14) && (ch == 0xEB)) ||
                    ((cl == 0x69) && (ch == 0x96))))
                {
                    // dont know what this is
                    continue;
                }

                isATAPI = TRUE;
                IDE_Write8(pChannel, IdeChanReg_W_COMMAND, ATA_CMD_IDENTIFY_PACKET);

                // pause 1ms
                K2OS_Thread_Sleep(1);
            }

            // wait not busy
            status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
            do {
                if (0 == (status & ATA_SR_BSY))
                    break;
                status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
            } while (1);

            // read back initial identification data here
            pData = (UINT16 *)&pDevice->AtaIdent;
            for (ixData = 0; ixData < 256; ixData++)
            {
                *pData = IDE_Read16(pChannel, IdeChanReg_RW_DATA);
                pData++;
            }

            // initial ident must have correct signature
            if (ATA_IDENTIFY_SIG_CORRECT != (pDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_SIG_MASK))
            {
                K2OSKERN_Debug(
                    "*** IDE(%08X) device %d/%d initial IDENTIFY signature incorrect (0x%02X)\n", 
                    apController,
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
                K2OSKERN_Debug(
                    "*** IDE(%08X) device %d/%d IDENTIFY checksum incorrect (0x%02X)\n",
                    apController,
                    ixIo, ixDev,
                    (pDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_CHKSUM_MASK) >> ATA_IDENTIFY_SIGCHECK_CHKSUM_SHL);
                continue;
            }

            // major revision is mandatory fixed; must be ata6+ compatible
            if (0 == (pDevice->AtaIdent.MajorRevision & ATA_IDENTIFY_VERSION_SUPP_ATA6))
            {
                K2OSKERN_Debug("*** IDE(%08X) device %d/%d does not support ATA6, device ignored.\n", apController, ixIo, ixDev);
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
                    IDE_Write8(pChannel, IdeChanReg_RW_HDDEVSEL, ATA_HDDSEL_STATIC | ((ixDev != 0) ? ATA_HDDSEL_DEVICE_SEL : 0));
                    // pause 1ms (400ns needed but we do 1ms because time)
                    K2OS_Thread_Sleep(1);
                    status = IDE_Read8(pChannel, IdeChanReg_R_STATUS); 
                    do {
                        if (0 == (status & ATA_SR_BSY))
                            break;
                        status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
                    } while (1);
                    IDE_Write8(pChannel, IdeChanReg_W_FEATURES, 0x95);
                    IDE_Write8(pChannel, IdeChanReg_W_COMMAND, ATA_CMD_SET_FEATURES);
                    // wait not busy
                    status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
                    do {
                        if (0 == (status & ATA_SR_BSY))
                            break;
                        status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
                    } while (1);
                    if (0 != (0x40 & IDE_Read8(pChannel, IdeChanReg_R_ERROR)))
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

                pDevice->mState = IdeState_EvalIdent;
                pDevice->mIsATAPI = isATAPI;
                if (K2OS_CritSec_Init(&pDevice->Sec))
                {
                    apController->mPopMask |= (1 << pDevice->mLocation);
                }
            }
            else
            {
                // CHS - ignore, not supported
            }
        }
    }

    // enable interrupts on channels

    if (apController->mPopMask & 0x3)
    {
        // primary channel has something on it
        pChannel = &apController->Channel[IDE_CHANNEL_PRIMARY];
        IDE_Write8(pChannel, IdeChanReg_W_DCR, 0);
        pChannel->mIrqMaskFlag = 0;

        K2OS_Thread_Sleep(10);
        do {
            status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
        } while (status & ATA_SR_BSY);
    }

    if (apController->mPopMask & 0xC)
    {
        // secondary channel has something on it
        pChannel = &apController->Channel[IDE_CHANNEL_SECONDARY];
        IDE_Write8(pChannel, IdeChanReg_W_DCR, 0);
        pChannel->mIrqMaskFlag = 0;
        do {
            status = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
        } while (status & ATA_SR_BSY);
    }

//    K2OSKERN_Debug("IDE(%08X): Discovered\n", apController);

    return K2STAT_NO_ERROR;
}

