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

static K2OSDDK_BLOCKIO_REGISTER sgBlockIoFuncTab =
{
    { FALSE },
    (K2OSDDK_pf_BlockIo_GetMedia)IDE_Device_GetMedia,
    (K2OSDDK_pf_BlockIo_Transfer)IDE_Device_Transfer
};

KernIntrDispType
IDE_Channel_Isr(
    void *              apKey,
    KernIntrActionType  aAction
)
{
    IDE_CHANNEL *       pChannel;
    BOOL                disp;
    KernIntrDispType    result;

    pChannel = K2_GET_CONTAINER(IDE_CHANNEL, apKey, mIrqHook);

    result = KernIntrDisp_None;

    disp = K2OSKERN_SeqLock(&pChannel->IrqSeqLock);
    K2_ASSERT(!disp);

    K2OSKERN_Debug("IDE channel %d IRQ\n", pChannel->mChannelIndex);

    K2OSKERN_SeqUnlock(&pChannel->IrqSeqLock, disp);

    return result;
}

K2STAT
IDE_Device_ReadBlock_LBA28(
    IDE_DEVICE *    apDevice,
    UINT64 const *  apBlockNum,
    UINT8 *         apData
)
{
    IDE_CHANNEL *   pChannel;
    UINT8           ideStatus;
    UINT32          ixData;
    UINT16 *        pData;
    BOOL            disp;
    UINT32          blockNum32;

    pChannel = apDevice->mpChannel;

    blockNum32 = *apBlockNum;

    //
    // disable interrupts across setup and command issue
    //
    disp = K2OSKERN_SeqLock(&pChannel->IrqSeqLock);

    IDE_Write8(pChannel, IdeChanReg_RW_SECCOUNT0, 1);
    IDE_Write8(pChannel, IdeChanReg_RW_LBA_LOW, (UINT8)(blockNum32 & 0xFF));
    IDE_Write8(pChannel, IdeChanReg_RW_LBA_MID, (UINT8)((blockNum32 >> 8) & 0xFF));
    IDE_Write8(pChannel, IdeChanReg_RW_LBA_HI, (UINT8)((blockNum32 >> 16) & 0xFF));
    ideStatus = (UINT8)((blockNum32 >> 24) & 0x0F) | ATA_HDDSEL_STATIC | ATA_HDDSEL_LBA_MODE;
    if (apDevice->mDeviceIndex != 0)
    {
        ideStatus |= ATA_HDDSEL_DEVICE_SEL;
    }
    IDE_Write8(pChannel, IdeChanReg_RW_HDDEVSEL, ideStatus);

    // send read command
    IDE_Write8(pChannel, IdeChanReg_W_COMMAND, ATA_CMD_READ_PIO);

    // read asr 4 times for 400ns delay
    for (ixData = 0; ixData < 4; ixData++)
    {
        IDE_Read8(pChannel, IdeChanReg_R_ASR);
    }

    K2OSKERN_SeqUnlock(&pChannel->IrqSeqLock, disp);

    // wait for BSY clear
    do {
        ideStatus = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
        if (0 == (ideStatus & ATA_SR_BSY))
            break;
        // check for timeout here
    } while (1);

    // wait for DRQ
    do {
        ideStatus = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
        if (0 != (ideStatus & ATA_SR_DRQ))
            break;
        if (0 != (ideStatus & ATA_SR_ERR))
        {
            ideStatus = IDE_Read8(pChannel, IdeChanReg_R_ERROR);
            K2OSKERN_Debug("error = 0x%02X\n", ideStatus);
            return K2STAT_ERROR_HARDWARE;
        }
        // check for timeout here
    } while (1);

    // slurp
    pData = (UINT16 *)apData;
    for (ixData = 0; ixData < 256; ixData++)
    {
        *pData = IDE_Read16(pChannel, IdeChanReg_RW_DATA);
        pData++;
    }

    for (ixData = 0; ixData < 4; ixData++)
    {
        IDE_Read8(pChannel, IdeChanReg_R_ASR);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
IDE_Device_ReadBlock_LBA48(
    IDE_DEVICE *    apDevice,
    UINT64 const *  apBlockNum,
    UINT8 *         apData
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
IDE_Device_WriteBlock_LBA28(
    IDE_DEVICE *    apDevice,
    UINT64 const *  apBlockNum,
    UINT8 *         apData
)
{
    IDE_CHANNEL *   pChannel;
    UINT8           ideStatus;
    UINT32          ixData;
    UINT16 *        pData;
    BOOL            disp;
    UINT32          blockNum32;

    pChannel = apDevice->mpChannel;

    blockNum32 = *apBlockNum;

    //
    // disable interrupts across setup and command issue
    //
    disp = K2OSKERN_SetIntr(FALSE);

    IDE_Write8(pChannel, IdeChanReg_RW_SECCOUNT0, 1);
    IDE_Write8(pChannel, IdeChanReg_RW_LBA_LOW, (UINT8)(blockNum32 & 0xFF));
    IDE_Write8(pChannel, IdeChanReg_RW_LBA_MID, (UINT8)((blockNum32 >> 8) & 0xFF));
    IDE_Write8(pChannel, IdeChanReg_RW_LBA_HI, (UINT8)((blockNum32 >> 16) & 0xFF));
    ideStatus = (UINT8)((blockNum32 >> 24) & 0x0F) | ATA_HDDSEL_STATIC | ATA_HDDSEL_LBA_MODE;
    if (apDevice->mDeviceIndex != 0)
    {
        ideStatus |= ATA_HDDSEL_DEVICE_SEL;
    }
    IDE_Write8(pChannel, IdeChanReg_RW_HDDEVSEL, ideStatus);

    // send write command
    IDE_Write8(pChannel, IdeChanReg_W_COMMAND, ATA_CMD_WRITE_PIO);

    K2OSKERN_SetIntr(disp);

    // read asr 4 times for 400ns delay
    for (ixData = 0; ixData < 4; ixData++)
    {
        IDE_Read8(pChannel, IdeChanReg_R_ASR);
    }

    // wait for BSY clear
    do {
        ideStatus = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
        if (0 == (ideStatus & ATA_SR_BSY))
            break;
        // check for timeout here
    } while (1);

    // wait for DRQ
    do {
        ideStatus = IDE_Read8(pChannel, IdeChanReg_R_STATUS);
        if (0 != (ideStatus & ATA_SR_DRQ))
            break;
        if (0 != (ideStatus & ATA_SR_ERR))
        {
            ideStatus = IDE_Read8(pChannel, IdeChanReg_R_ERROR);
            K2OSKERN_Debug("error = 0x%02X\n", ideStatus);
            return K2STAT_ERROR_HARDWARE;
        }
        // check for timeout here
    } while (1);

    // bleah
    pData = (UINT16 *)apData;
    for (ixData = 0; ixData < 256; ixData++)
    {
        IDE_Write16(pChannel, IdeChanReg_RW_DATA, *pData);
        pData++;
    }

    for (ixData = 0; ixData < 4; ixData++)
    {
        IDE_Read8(pChannel, IdeChanReg_R_ASR);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
IDE_Device_WriteBlock_LBA48(
    IDE_DEVICE *    apDevice,
    UINT64 const *  apBlockNum,
    UINT8 *         apData
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
IDE_Device_ReadBlock(
    IDE_DEVICE *    apDevice,
    UINT64 const *  apBlockNum,
    UINT8 *         apData
)
{
    if (apDevice->mAtaMode == ATA_MODE_LBA28)
    {
        return IDE_Device_ReadBlock_LBA28(apDevice, apBlockNum, apData);
    }

    return IDE_Device_ReadBlock_LBA48(apDevice, apBlockNum, apData);
}

K2STAT
IDE_Device_WriteBlock(
    IDE_DEVICE *    apDevice,
    UINT64 const *  apBlockNum,
    UINT8 const *   apData
)
{
    if (apDevice->mAtaMode == ATA_MODE_LBA28)
    {
        return IDE_Device_WriteBlock_LBA28(apDevice, apBlockNum, (UINT8 *)apData);
    }

    return IDE_Device_WriteBlock_LBA48(apDevice, apBlockNum, (UINT8 *)apData);
}

UINT32
IDE_Device_Service(
    IDE_DEVICE *apDevice,
    BOOL        aDueToInterrupt
)
{
    K2OS_BLOCKIO_TRANSFER const *   pTransfer;
    UINT64                          workBlock;
    UINT8 *                         mpWorkAddr;
    UINT32                          blocksLeft;
    K2STAT                          stat;

//    K2OSKERN_Debug("IDE_Device_Service(%d/%d)\n", apDevice->mpChannel->mChannelIndex, apDevice->mDeviceIndex);

    K2OS_CritSec_Enter(&apDevice->Sec);

    pTransfer = apDevice->mpTransfer;
    K2_ASSERT(NULL != pTransfer);

    //
    // for now do the whole transfer here using PIO
    //
    workBlock = pTransfer->mStartBlock;
    K2_ASSERT(workBlock < apDevice->Media.mBlockCount);
    blocksLeft = pTransfer->mBlockCount;
    K2_ASSERT(0 != blocksLeft);

    mpWorkAddr = (UINT8 *)pTransfer->mAddress;
    K2_ASSERT(NULL != mpWorkAddr);

    do {
        if (!pTransfer->mIsWrite)
        {
            stat = IDE_Device_ReadBlock(apDevice, &workBlock, mpWorkAddr);
        }
        else
        {
            stat = IDE_Device_WriteBlock(apDevice, &workBlock, mpWorkAddr);
        }
        mpWorkAddr += apDevice->Media.mBlockSizeBytes;

        if (K2STAT_IS_ERROR(stat))
            break;

        workBlock++;
    } while (--blocksLeft);

    apDevice->mTransferStatus = stat; 
    apDevice->mpTransfer = NULL;
    K2_CpuWriteBarrier();
    K2OS_Notify_Signal(apDevice->mTokTransferWaitNotify);

    //
    // go back to waiting
    //

    K2OS_CritSec_Leave(&apDevice->Sec);

    apDevice->mWaitMs = K2OS_TIMEOUT_INFINITE;
    return 0;
}

UINT32
IDE_Device_EvalIdent(
    IDE_DEVICE *apDevice
)
{
    UINT8 * pCheck;
    UINT32  ixData;
    UINT8   cl;
    char    ch;
    K2STAT  stat;

    // return 0 to wait for further service
//    K2OSKERN_Debug("IDE(%08X) Device %d/%d eval ident\n", apDevice->mpChannel->mpController, apDevice->mpChannel->mChannelIndex, apDevice->mDeviceIndex);

    if (ATA_IDENTIFY_SIG_CORRECT != (apDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_SIG_MASK))
    {
        K2OSKERN_Debug(
            "*** IDE(%08X) device %d/%d IDENTIFY signature incorrect (0x%02X)\n",
            apDevice->mpChannel->mpController, apDevice->mpChannel->mChannelIndex, apDevice->mDeviceIndex,
            apDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_SIG_MASK);

        // switch to check for media change after 5 seconds
        apDevice->mWaitMs = 5000;
        apDevice->mState = IdeState_WaitMediaChange;

        return 0;
    }

    // identify must have correct checksum
    pCheck = (UINT8 *)&apDevice->AtaIdent;
    cl = ATA_IDENTIFY_SIG_CORRECT;
    for (ixData = 0; ixData < 510; ixData++)
    {
        cl += *pCheck;
        pCheck++;
    }
    cl = ~cl + 1;
    if (cl != ((apDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_CHKSUM_MASK) >> ATA_IDENTIFY_SIGCHECK_CHKSUM_SHL))
    {
        K2OSKERN_Debug(
            "*** IDE(%08X) device %d/%d IDENTIFY checksum incorrect (calc 0x%02X but see 0x%02X)\n",
            apDevice->mpChannel->mpController, apDevice->mpChannel->mChannelIndex, apDevice->mDeviceIndex,
            cl,
            (apDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_CHKSUM_MASK) >> ATA_IDENTIFY_SIGCHECK_CHKSUM_SHL);

        // switch to check for media change after 5 seconds
        apDevice->mWaitMs = 5000;
        apDevice->mState = IdeState_WaitMediaChange;

        return 0;
    }

    //
    // identify data should be treated as correct;  render media
    //
    for (ixData = 0; ixData < 39; ixData += 2)
    {
        ch = apDevice->AtaIdent.ModelNumber[ixData];
        apDevice->AtaIdent.ModelNumber[ixData] = apDevice->AtaIdent.ModelNumber[ixData + 1];
        apDevice->AtaIdent.ModelNumber[ixData + 1] = ch;
    }
    ixData = 39;
    do {
        apDevice->AtaIdent.ModelNumber[ixData] = 0;
        --ixData;
    } while ((ixData > 0) && (apDevice->AtaIdent.ModelNumber[ixData] == ' '));

    for (ixData = 0; ixData < 19; ixData += 2)
    {
        ch = apDevice->AtaIdent.SerialNumber[ixData];
        apDevice->AtaIdent.SerialNumber[ixData] = apDevice->AtaIdent.SerialNumber[ixData + 1];
        apDevice->AtaIdent.SerialNumber[ixData + 1] = ch;
    }
    ixData = 19;
    do {
        apDevice->AtaIdent.SerialNumber[ixData] = 0;
        --ixData;
    } while ((ixData > 0) && (apDevice->AtaIdent.SerialNumber[ixData] == ' '));

    for (ixData = 0; ixData < 7; ixData += 2)
    {
        ch = apDevice->AtaIdent.FirmwareRevision[ixData];
        apDevice->AtaIdent.FirmwareRevision[ixData] = apDevice->AtaIdent.FirmwareRevision[ixData + 1];
        apDevice->AtaIdent.FirmwareRevision[ixData + 1] = ch;
    }
    ixData = 7;
    do {
        apDevice->AtaIdent.FirmwareRevision[ixData] = 0;
        --ixData;
    } while ((ixData > 0) && (apDevice->AtaIdent.FirmwareRevision[ixData] == ' '));

    K2_ASSERT(0 != (apDevice->AtaIdent.Caps & ATA_IDENTIFY_CAPS_LBA_SUPPORTED));
    if (apDevice->AtaIdent.UserAddressableSectors >= 0x10000000)
    {
        apDevice->mAtaMode = ATA_MODE_LBA48;
        apDevice->Media.mBlockCount = apDevice->AtaIdent.ExtendedNumberOfUserAddressableSectors[0];

    }
    else
    {
        apDevice->mAtaMode = ATA_MODE_LBA28;
        apDevice->Media.mBlockCount = apDevice->AtaIdent.UserAddressableSectors;
    }

    apDevice->Media.mBlockSizeBytes = ATA_SECTOR_BYTES;

    apDevice->Media.mTotalBytes = ((UINT64)apDevice->Media.mBlockCount) * ((UINT64)apDevice->Media.mBlockSizeBytes);
    K2MEM_Copy(apDevice->Media.mFriendly, apDevice->AtaIdent.ModelNumber, K2OS_STORAGE_MEDIA_FRIENDLY_BUFFER_CHARS - 1);
    apDevice->Media.mFriendly[K2OS_STORAGE_MEDIA_FRIENDLY_BUFFER_CHARS - 1] = 0;

    if (apDevice->mIsRemovable)
    {
        // ATAPI - do TEST UNIT READY command to determine if media present ("MEDIUM NOT PRESENT" error)
//        K2OSKERN_Debug("device removable: %04X %04X %04X...\n",
//            apDevice->AtaIdent.CurrentMediaSerialNumber[0],
//            apDevice->AtaIdent.CurrentMediaSerialNumber[1],
//            apDevice->AtaIdent.CurrentMediaSerialNumber[2]);
        ixData = K2CRC_Calc32(0, &apDevice->AtaIdent.CurrentMediaSerialNumber, 15);
        apDevice->Media.mUniqueId = (((UINT64)ixData) << 32) | (UINT64)K2CRC_Calc32(0, &apDevice->AtaIdent.CurrentMediaSerialNumber[15], 15);
    }
    else
    {
//        K2OSKERN_Debug("device not removable: %04X %04X %04X...\n",
//            apDevice->AtaIdent.SerialNumber[0],
//            apDevice->AtaIdent.SerialNumber[1],
//            apDevice->AtaIdent.SerialNumber[2]);
        ixData = K2CRC_Calc32(0, &apDevice->AtaIdent.SerialNumber, 10);
        apDevice->Media.mUniqueId = (((UINT64)ixData) << 32) | (UINT64)K2CRC_Calc32(0, &apDevice->AtaIdent.SerialNumber[10], 10);
    }

    //
    // register this blockio device with the kernel now
    // this will translate to the publish of interface instance by the io manager
    // when it is ready to handle requests to this device
    //
    stat = K2OSDDK_BlockIoRegister(
        apDevice->mpChannel->mpController->mDevCtx,
        apDevice,
        &sgBlockIoFuncTab,
        &apDevice->mpChannel->mpNotifyKey);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug(
            "*** IDE(%08X) device for %d/%d could not be registered (0x%08X)\n",
            apDevice->mpChannel->mpController, apDevice->mpChannel->mChannelIndex, apDevice->mDeviceIndex,
            K2OS_Thread_GetLastStatus()
        );

        // switch to check for media change after 5 seconds
        apDevice->mWaitMs = 5000;
        apDevice->mState = IdeState_WaitMediaChange;

        return 0;
    }

    apDevice->mState = IdeState_InService;
    apDevice->mWaitMs = K2OS_TIMEOUT_INFINITE;

    return 0;
}

UINT32
IDE_Device_CheckMediaChange(
    IDE_DEVICE *    apDevice
)
{
    K2OSKERN_Debug("IDE(%d/%d)-Poll media change\n", apDevice->mpChannel->mChannelIndex, apDevice->mDeviceIndex);
    apDevice->mState = IdeState_WaitMediaChange;
    apDevice->mWaitMs = 5000;
    return 0;
}

UINT32
IDE_Device_Eval(
    IDE_DEVICE *    apDevice,
    BOOL            aDueToInterrupt
)
{
    switch (apDevice->mState)
    {
    case IdeState_WaitIdent:
        K2_ASSERT(0);
        break;

    case IdeState_EvalIdent:
        return IDE_Device_EvalIdent(apDevice);

    case IdeState_WaitMediaChange:
        return IDE_Device_CheckMediaChange(apDevice);

    case IdeState_InService:
        return IDE_Device_Service(apDevice, aDueToInterrupt);

    default:
        K2_ASSERT(0);
    }

    return 0;
}

UINT32
IDE_Channel_Eval(
    IDE_CHANNEL *   apChannel,
    BOOL            aDueToInterrupt
)
{
    UINT32 more_service_needed;
    UINT32 result;

    result = K2OS_TIMEOUT_INFINITE;

    do {
        more_service_needed = 0;

        if (apChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mState != IdeState_DeviceNotPresent)
        {
            more_service_needed |= IDE_Device_Eval(&apChannel->Device[IDE_CHANNEL_DEVICE_MASTER], aDueToInterrupt);
            if (apChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mWaitMs < result)
                result = apChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mWaitMs;
        }

        if (apChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mState != IdeState_DeviceNotPresent)
        {
            more_service_needed |= IDE_Device_Eval(&apChannel->Device[IDE_CHANNEL_DEVICE_SLAVE], aDueToInterrupt);
            if (apChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mWaitMs < result)
                result = apChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mWaitMs;
        }

        aDueToInterrupt = FALSE;

    } while (0 != more_service_needed);

    return result;
}

UINT32
IDE_Channel_Thread(
    IDE_CHANNEL *apChannel
)
{
    K2OS_WAITABLE_TOKEN     tokWait[2];
    UINT32                  numWait;
    K2OS_WaitResult         waitResult;
    BOOL                    ok;
    K2STAT                  stat;
    UINT32                  waitMs;

    tokWait[0] = apChannel->mTokNotify;
    numWait = 1;

    if (NULL != apChannel->mpIrqRes)
    {
        if (K2OSKERN_IrqDefine(&apChannel->mpIrqRes->Def.Irq.Config))
        {
            apChannel->mIrqHook = IDE_Channel_Isr;
            tokWait[1] = K2OSKERN_IrqHook(apChannel->mpIrqRes->Def.Irq.Config.mSourceIrq, &apChannel->mIrqHook);
            if (NULL == tokWait[1])
            {
                K2OSKERN_Debug("***Failed to hook irq for IDE channel %d\n", apChannel->mChannelIndex);
            }
            else
            {
                numWait++;
                ok = K2OSKERN_IntrVoteIrqEnable(tokWait[1], TRUE);
                K2_ASSERT(ok);
            }
        }
    }

    //
    // service channel
    //
    waitMs = K2OS_TIMEOUT_INFINITE;

    do {
//        K2OSKERN_Debug("IDE Channel %d sleep\n", apChannel->mChannelIndex);
        ok = K2OS_Thread_WaitMany(&waitResult, numWait, tokWait, FALSE, waitMs);
        if (!ok)
        {
            stat = K2OS_Thread_GetLastStatus();
            if (K2STAT_ERROR_TIMEOUT != stat)
            {
                K2OSKERN_Debug("IDE Channel %d wait failure (%08X)\n", apChannel->mChannelIndex, stat);
                break;
            }
        }

        waitMs = IDE_Channel_Eval(apChannel, (waitResult == (K2OS_Wait_Signalled_0 + 1)));

    } while (1);

    return 0;
}

