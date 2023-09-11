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
    UINT32                  bEnable;

    tokWait[0] = apChannel->mTokNotify;
    numWait = 1;

    if (NULL != apChannel->mpIrqRes)
    {
        tokWait[1] = apChannel->mpIrqRes->Irq.mTokInterrupt;
        numWait++;
        ok = K2OSKERN_IntrVoteIrqEnable(tokWait[1], TRUE);
        K2_ASSERT(ok);
    }

    bEnable = apChannel->mpController->mPopMask;
    if (apChannel->mChannelIndex == IDE_CHANNEL_PRIMARY)
    {
        bEnable &= 0x03;
    }
    else
    {
        bEnable &= 0x0C;
    }

    if (apChannel->mpController->mPopMask == K2ATOMIC_Or(&apChannel->mpController->mEnableMask, bEnable))
    {
        //
        // we are the last channel to get here, so enable the controller now
        //
        K2OSDDK_SetEnable(apChannel->mpController->mDevCtx, TRUE);
    }

    //
    // service channel
    //
    do {
        ok = K2OS_Thread_WaitMany(&waitResult, numWait, tokWait, FALSE, K2OS_TIMEOUT_INFINITE);
        if (!ok)
        {
            stat = K2OS_Thread_GetLastStatus();
            if (K2STAT_ERROR_TIMEOUT != stat)
            {
                K2OSKERN_Debug("IDE Channel %d wait failure (%08X)\n", apChannel->mChannelIndex, stat);
                break;
            }
        }
        else
        {
            if (waitResult == K2OS_Wait_Signalled_0)
            {
                //
                // notify fired
                //
                K2OSKERN_Debug("IDE Channel %d notify\n", apChannel->mChannelIndex);
            }
            else
            {
                //
                // interrupt fired
                //
                K2OSKERN_Debug("IDE Channel %d interrupt\n", apChannel->mChannelIndex);
                K2OSKERN_IntrDone(tokWait[1]);
            }
        }
    } while (1);

    return 0;
}


#if 0


UINT32
IDE_DeviceService_EvalIdent(
    IDE_DEVICE *apDevice
)
{
    static K2_GUID128 sgBlockIoIfaceId = K2OS_IFACE_BLOCKIO_DEVICE_CLASSID;

    UINT8 * pCheck;
    UINT32  ixData;
    UINT8   cl;
    char    ch;

    // return 0 to wait for further service

    if (ATA_IDENTIFY_SIG_CORRECT != (apDevice->AtaIdent.SigCheck & ATA_IDENTIFY_SIGCHECK_SIG_MASK))
    {
        K2OSKERN_Debug(
            "IDE device %d/%d IDENTIFY signature incorrect (0x%02X)\n",
            apDevice->mpChannel->mIndex, apDevice->mIndex,
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
            "IDE device %d/%d IDENTIFY checksum incorrect (calc 0x%02X but see 0x%02X)\n",
            apDevice->mpChannel->mIndex, apDevice->mIndex,
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

    if (ATA_MODE_CHS == apDevice->mAtaMode)
    {
        apDevice->Media.mBlockCount = apDevice->AtaIdent.NumCylinders * apDevice->AtaIdent.NumHeads * apDevice->AtaIdent.NumSectorsPerTrack;
    }
    else
    {
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
    }
    apDevice->Media.mBlockSizeBytes = ATA_SECTOR_BYTES;

    apDevice->Media.mTotalBytes = ((UINT64)apDevice->Media.mBlockCount) * ((UINT64)apDevice->Media.mBlockSizeBytes);
    K2MEM_Copy(apDevice->Media.mFriendly, apDevice->AtaIdent.ModelNumber, K2OS_STORAGE_MEDIA_FRIENDLY_BUFFER_CHARS - 1);
    apDevice->Media.mFriendly[K2OS_STORAGE_MEDIA_FRIENDLY_BUFFER_CHARS - 1] = 0;

    if (apDevice->mIsRemovable)
    {
        ixData = K2CRC_Calc32(0, &apDevice->AtaIdent.CurrentMediaSerialNumber, 15);
        apDevice->Media.mUniqueId = (((UINT64)ixData) << 32) | (UINT64)K2CRC_Calc32(0, &apDevice->AtaIdent.CurrentMediaSerialNumber[15], 15);
    }
    else
    {
        ixData = K2CRC_Calc32(0, &apDevice->AtaIdent.SerialNumber, 10);
        apDevice->Media.mUniqueId = (((UINT64)ixData) << 32) | (UINT64)K2CRC_Calc32(0, &apDevice->AtaIdent.SerialNumber[10], 10);
    }

    if (NULL == apDevice->mRpcIfInst)
    {
        apDevice->mRpcIfInst = K2OS_RpcObj_PublishIfInst(apDevice->mRpcObj, K2OS_IFACE_CLASSCODE_STORAGE_DEVICE, &sgBlockIoIfaceId, &apDevice->mRpcIfInstId);
        if (NULL == apDevice->mRpcIfInst)
        {
            K2OSKERN_Debug(
                "IDE device blockio interface for %d/%d could not be published (0x%08X)\n",
                apDevice->mpChannel->mIndex, apDevice->mIndex,
                K2OS_Thread_GetLastStatus
            );

            // switch to check for media change after 5 seconds
            apDevice->mWaitMs = 5000;
            apDevice->mState = IdeState_WaitMediaChange;

            return 0;
        }

        K2OSKERN_Debug("IDE: Published %d/%d BlockIo Interface as id %d\n", apDevice->mpChannel->mIndex, apDevice->mIndex, apDevice->mRpcIfInstId);
    }

    apDevice->mState = IdeState_Idle;
    apDevice->mWaitMs = K2OS_TIMEOUT_INFINITE;


#endif