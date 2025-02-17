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

#include "storerun.h"

static K2_GUID128 sgQemuQuadPeiBootCardMediaGuid = { 0xde4c4e86, 0x5e7f, 0x4a68, { 0x99, 0x6f, 0xd4, 0x79, 0xa4, 0xe6, 0xba, 0xc5 } };

K2STAT
STORERUN_GetMedia(
    STORERUN_DEVICE *        apDevice,
    K2OS_STORAGE_MEDIA *    apRetMedia
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
STORERUN_Transfer(
    STORERUN_DEVICE *                apDevice,
    K2OS_BLOCKIO_TRANSFER const *   apTransfer
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

static K2OSDDK_BLOCKIO_REGISTER sgBlockIoFuncTab =
{
    { FALSE },  // does not use hardware addresses
    (K2OSDDK_pf_BlockIo_GetMedia)STORERUN_GetMedia,
    (K2OSDDK_pf_BlockIo_Transfer)STORERUN_Transfer
};

K2STAT
CreateInstance(
    K2OS_DEVCTX aDevCtx,
    void **     appRetDriverContext
)
{
    K2STAT              stat;
    UINT32              ioBytes;
    STORERUN_DEVICE *   pDevice;

    pDevice = (STORERUN_DEVICE *)K2OS_Heap_Alloc(sizeof(STORERUN_DEVICE));
    if (NULL == pDevice)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pDevice, sizeof(STORERUN_DEVICE));

    ioBytes = sizeof(pDevice->mpProto);
    if (!K2OSKERN_GetFirmwareTable(&sgQemuQuadPeiBootCardMediaGuid, &ioBytes, &pDevice->mpProto))
    {
        K2OSKERN_Debug("StoreRun firmware table not found %08X\n", K2OS_Thread_GetLastStatus());
        K2OS_Heap_Free(pDevice);
        return K2STAT_ERROR_NOT_FOUND;
    }

    pDevice->mDevCtx = aDevCtx;
    *appRetDriverContext = pDevice;

    return K2STAT_NO_ERROR;
}

K2STAT
StartDriver(
    STORERUN_DEVICE *    apDevice
)
{
    K2STAT  stat;

    //
    // allocate resources
    //

    stat = K2OSDDK_DriverStarted(apDevice->mDevCtx);
    if (!K2STAT_IS_ERROR(stat))
    {
        stat = K2OSDDK_BlockIoRegister(apDevice->mDevCtx, apDevice, &sgBlockIoFuncTab, &apDevice->mpNotifyKey);

        if (!K2STAT_IS_ERROR(stat))
        {
            stat = K2OSDDK_SetEnable(apDevice->mDevCtx, TRUE);
        
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDDK_BlockIoDeregister(apDevice->mDevCtx, apDevice);
            }
        }

        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDDK_DriverStopped(apDevice->mDevCtx, stat);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        //
        // tear down any resources allocated above
        //

    }

    return stat;
}

K2STAT
StopDriver(
    STORERUN_DEVICE *    apDevice
)
{
    K2OSDDK_BlockIoDeregister(apDevice->mDevCtx, apDevice);

    K2OSDDK_DriverStopped(apDevice->mDevCtx, K2STAT_NO_ERROR);

    return K2STAT_NO_ERROR;
}

K2STAT
DeleteInstance(
    STORERUN_DEVICE *    apDevice
)
{
    K2OS_Heap_Free(apDevice);

    return K2STAT_NO_ERROR;
}
