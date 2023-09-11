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

K2OSDRV_GET_INSTANCE_OUT    gInst;
K2OSDRV_RES_IO              gResIo[5];
K2OSDRV_RES_PHYS            gResPhys[5];
K2OSDRV_RES_IRQ             gResIrq[2];
BOOL                        gBusMasterIsPhys;
UINT32                      gBusMasterAddr;
UINT32                      gPopMask;
IDE_CHANNEL                 gChannel[2];

static volatile BOOL        sgSecondaryReady;
static UINT32               sgDriverThreadId;

K2STAT
IdeBlockIoDevice_Create(
    K2OS_RPC_OBJ                    aObj,
    K2OS_RPC_OBJECT_CREATE const *  apCreate,
    UINT32 *                        apRetContext,
    BOOL *                          apRetSingleUsage
)
{
    IDE_DEVICE * pDev;

    if (apCreate->mCreatorProcessId != K2OS_Process_GetId())
    {
        K2OSDrv_DebugPrintf("!!!IDE - Attempt to create nonlocal handle of device\n");
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pDev = (IDE_DEVICE *)apCreate->mCreatorContext;
    pDev->mRpcObj = aObj;
    *apRetContext = apCreate->mCreatorContext;
    *apRetSingleUsage = FALSE;

    return K2STAT_NO_ERROR;
}

K2STAT
IdeBlockIoDevice_Call(
    K2OS_RPC_OBJ                    aObj,
    UINT32                          aObjContext,
    K2OS_RPC_OBJECT_CALL const *    apCall,
    UINT32 *                        apRetUsedOutBytes
)
{
    IDE_DEVICE *    pDevice;
    K2STAT          stat;

    pDevice = (IDE_DEVICE *)aObjContext;
    K2_ASSERT(aObj == pDevice->mRpcObj);

    switch (apCall->Args.mMethodId)
    {
    case K2OS_BLOCKIO_METHOD_GET_MEDIA:
        if ((apCall->Args.mOutBufByteCount < sizeof(K2OS_STORAGE_MEDIA)) ||
            (apCall->Args.mInBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            K2OS_CritSec_Enter(&pDevice->Sec);
            if ((!pDevice->mIsRemovable) || (pDevice->mMediaPresent))
            {
                K2MEM_Copy(apCall->Args.mpOutBuf, &pDevice->Media, sizeof(K2OS_STORAGE_MEDIA));
                *apRetUsedOutBytes = sizeof(K2OS_STORAGE_MEDIA);
                stat = K2STAT_NO_ERROR;
            }
            else
            {
                stat = K2STAT_ERROR_NO_MEDIA;
            }
            K2OS_CritSec_Leave(&pDevice->Sec);
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    }

    return stat;
}

K2STAT
IdeBlockIoDevice_Delete(
    K2OS_RPC_OBJ    aObj,
    UINT32          aObjContext
)
{
    // this should never happen
    K2OS_RaiseException(K2STAT_EX_LOGIC);
    return K2STAT_NO_ERROR;
}

K2OS_RPC_OBJECT_CLASSDEF const gIdeBlockIoDevice_ObjectClassDef =
{
    // {191336DB-3E2C-4C4F-AC1D-BCE8158078E0}
    { 0x191336db, 0x3e2c, 0x4c4f, { 0xac, 0x1d, 0xbc, 0xe8, 0x15, 0x80, 0x78, 0xe0 } },
    IdeBlockIoDevice_Create,
    IdeBlockIoDevice_Call,
    IdeBlockIoDevice_Delete
};

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
        K2OSDrv_DebugPrintf(
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
        K2OSDrv_DebugPrintf(
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
            K2OSDrv_DebugPrintf(
                "IDE device blockio interface for %d/%d could not be published (0x%08X)\n",
                apDevice->mpChannel->mIndex, apDevice->mIndex,
                K2OS_Thread_GetLastStatus
            );

            // switch to check for media change after 5 seconds
            apDevice->mWaitMs = 5000;
            apDevice->mState = IdeState_WaitMediaChange;

            return 0;
        }

        K2OSDrv_DebugPrintf("IDE: Published %d/%d BlockIo Interface as id %d\n", apDevice->mpChannel->mIndex, apDevice->mIndex, apDevice->mRpcIfInstId);
    }

    apDevice->mState = IdeState_Idle;
    apDevice->mWaitMs = K2OS_TIMEOUT_INFINITE;

    return 0;
}

UINT32
IDE_DeviceService_Idle(
    IDE_DEVICE *    apDevice
)
{
    return 0;
}

UINT32
IDE_DeviceService(
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
        return IDE_DeviceService_EvalIdent(apDevice);

    case IdeState_Idle:
        return IDE_DeviceService_Idle(apDevice);

    default:
        K2_ASSERT(0);
    }

    return 0;
}

UINT32 
IDE_ChannelService(
    IDE_CHANNEL *   apChannel,
    BOOL            aDueToInterrupt
)
{
    UINT32 more_service_needed;
    UINT32 result;

    result = K2OS_TIMEOUT_INFINITE;

    do {
        more_service_needed = 0;

        if (0 != apChannel->Device[ATA_DEVICE_MASTER].mState)
        {
            more_service_needed |= IDE_DeviceService(&apChannel->Device[ATA_DEVICE_MASTER], aDueToInterrupt);
            if (apChannel->Device[ATA_DEVICE_MASTER].mWaitMs < result)
                result = apChannel->Device[ATA_DEVICE_MASTER].mWaitMs;
        }

        if (0 != apChannel->Device[ATA_DEVICE_SLAVE].mState)
        {
            more_service_needed |= IDE_DeviceService(&apChannel->Device[ATA_DEVICE_SLAVE], aDueToInterrupt);
            if (apChannel->Device[ATA_DEVICE_SLAVE].mWaitMs < result)
                result = apChannel->Device[ATA_DEVICE_SLAVE].mWaitMs;
        }

        aDueToInterrupt = FALSE;

    } while (0 != more_service_needed);

    return result;
}

UINT32
IDE_ChannelThread(
    UINT32 ixChannel
)
{
    IDE_CHANNEL *           pChannel;
    K2OS_WAITABLE_TOKEN     tokWait[2];
    UINT32                  numWait;
    UINT32                  waitResult;
    UINT32                  timeOut;

    K2_ASSERT(ixChannel < 2);
    pChannel = &gChannel[ixChannel];
    K2_ASSERT(ixChannel == pChannel->mIndex);

    pChannel->mNotifyToken = K2OS_Notify_Create(TRUE);
    K2_ASSERT(NULL != pChannel->mNotifyToken);
    tokWait[0] = pChannel->mNotifyToken;
    numWait = 1;

    if (NULL != pChannel->mpIrq)
    {
        tokWait[1] = pChannel->mpIrq->mInterruptToken;
        numWait++;
    }

    //
    // enable the driver here
    //
    if (sgDriverThreadId == K2OS_Thread_GetId())
    {
        if (ixChannel == ATA_CHANNEL_PRIMARY)
        {
            if (gPopMask & 0x0C)
            {
                //
                // wait for secondary channel thread to be ready
                //
                while (!sgSecondaryReady)
                {
                    K2OS_Thread_Sleep(100);
                    K2_CpuReadBarrier();
                }
            }
        }
        else
        {
            //
            // only reason we are here is because secondary channel is running on driver thread
            // so there are no usable devices on the primary channel
            //
        }
        K2OSDrv_SetEnable(TRUE);
    }
    else
    {
        sgSecondaryReady = TRUE;
        K2_CpuWriteBarrier();
    }

    //
    // service the channel
    //
    if (NULL != pChannel->mpIrq)
    {
        K2OS_Intr_SetEnable(pChannel->mpIrq->mInterruptToken, TRUE);
    }

    timeOut = K2OS_TIMEOUT_INFINITE;

    do {
        K2OSDrv_DebugPrintf("IDE Channel %d wait\n", ixChannel);
        waitResult = K2OS_Wait_Many(numWait, tokWait, FALSE, timeOut);

        if (waitResult != (K2OS_THREAD_WAIT_SIGNALLED_0 + 1))
        {
            K2OSDrv_DebugPrintf("IDE(%d) notify or timeout\n", ixChannel);
            timeOut = IDE_ChannelService(pChannel, FALSE);
        }
        else
        {
            K2OSDrv_DebugPrintf("IDE(%d) interrupt\n", ixChannel);
            timeOut = IDE_ChannelService(pChannel, TRUE);
            K2OS_Intr_End(pChannel->mpIrq->mInterruptToken);
        }

    } while (1);

    K2OS_Token_Destroy(pChannel->mNotifyToken);

    return 0;
}

UINT32 
DriverThread(
    UINT32 aBusDriverProcessId,
    UINT32 aBusDriverChildObjectId
)
{
    K2STAT              stat;
    UINT32              resIx;
    K2OS_THREAD_TOKEN   tokThread;

    sgDriverThreadId = K2OS_Thread_GetId();

    K2OSDrv_DebugPrintf("Ide(proc %d), bus driver proc %d, child object %d\n", K2OS_Process_GetId(), aBusDriverProcessId, aBusDriverChildObjectId);

    K2MEM_Zero(&gInst, sizeof(gInst));
    stat = K2OSDrv_GetInstance(&gInst);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** Ide(%d): GetInstance failed (0x%08X)\n", K2OS_Process_GetId(), stat);
        return stat;
    }

    if ((gInst.mCountIo + gInst.mCountPhys) < 4)
    {
        K2OSDrv_DebugPrintf("*** Ide(%d): insufficient ios (io+phys) for IDE bus (%d specified)\n", K2OS_Process_GetId(), gInst.mCountIo + gInst.mCountPhys);
        return K2STAT_ERROR_NOT_EXIST;
    }

    if (gInst.mCountIo > 5)
        gInst.mCountIo = 5;
    for (resIx = 0; resIx < gInst.mCountIo; resIx++)
    {
        K2MEM_Zero(&gResIo[resIx], sizeof(gResIo[resIx]));
        stat = K2OSDrv_GetResIo(resIx, &gResIo[resIx]);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): Could not get io resource %d (0x%08X)\n", K2OS_Process_GetId(), resIx, stat);
            return stat;
        }
        //        K2OSDrv_DebugPrintf("Bar %d = io %04X\n", gResIo[resIx].mId, gResIo[resIx].Range.mBasePort);
    }

    if (gInst.mCountPhys > 5)
        gInst.mCountPhys = 5;
    for (resIx = 0; resIx < gInst.mCountPhys; resIx++)
    {
        K2MEM_Zero(&gResPhys[resIx], sizeof(gResPhys[resIx]));

        stat = K2OSDrv_GetResPhys(resIx, &gResPhys[resIx]);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): Could not get phys resource %d (0x%08X)\n", K2OS_Process_GetId(), resIx, stat);
            return stat;
        }
        //        K2OSDrv_DebugPrintf("Bar %d = phys %08X\n", gResPhys[resIx].mId, gResPhys[resIx].Range.mBaseAddr);
    }

    if (gInst.mCountIrq > 2)
        gInst.mCountIrq = 2;
    for (resIx = 0; resIx < gInst.mCountIrq; resIx++)
    {
        K2MEM_Zero(&gResIrq[resIx], sizeof(gResIrq[resIx]));

        stat = K2OSDrv_GetResIrq(resIx, &gResIrq[resIx]);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("*** Ide(%d): Could not get irq resource %d (0x%08X)\n", K2OS_Process_GetId(), resIx, stat);
            return stat;
        }
        //        K2OSDrv_DebugPrintf("Irq id %d = line %08X\n", gResIrq[resIx].mId, gResIrq[resIx].Config.mSourceIrq);
    }

    gPopMask = 0;

    if (NULL == K2OS_RpcServer_Register(&gIdeBlockIoDevice_ObjectClassDef, 0))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2OSDrv_DebugPrintf("*** Ide(%d): Could not register block io device rpc object class (0x%08X)\n", K2OS_Process_GetId(), stat);
        return stat;
    }

    stat = IDE_InitAndDiscover();
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (0 == gPopMask)
    {
        K2OSDrv_DebugPrintf("*** Ide(%d): No devices found\n", K2OS_Process_GetId());
        return 0;
    }

    if (gPopMask & 0x0C)
    {
        //
        // something is on secondary channel
        // need to run a thread for secondary channel unless there is nothing on primary channel
        //
        if (0 != (gPopMask & 0x03))
        {
            //
            // there is stuff on both primary and secondary channels
            //
            sgSecondaryReady = FALSE;
            tokThread = K2OS_Thread_Create("IDE Secondary Channel", (K2OS_pf_THREAD_ENTRY)IDE_ChannelThread, (void *)ATA_CHANNEL_SECONDARY, NULL, NULL);
            if (NULL == tokThread)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2OSDrv_DebugPrintf("*** Ide: Coult not create channel thread for secondary channel (%08X)\n", stat);
                return stat;
            }
        }
        else
        {
            //
            // secondary channel is only thing that has stuff on it
            //
            K2OS_Thread_SetName("IDE Secondary Channel");
            IDE_ChannelThread(ATA_CHANNEL_SECONDARY);
            K2OS_Thread_Exit(0);
        }
    }

    K2OS_Thread_SetName("IDE Primary Channel");
    IDE_ChannelThread(ATA_CHANNEL_PRIMARY);

    return 0;
}

