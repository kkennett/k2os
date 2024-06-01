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

#include "ramdisk.h"

#define RAMDISK_PAGES       (256 * 4)  // 4MB
#define RAMDISK_BLOCKSIZE   512

K2STAT
RAMDISK_GetMedia(
    RAMDISK_DEVICE *        apDevice,
    K2OS_STORAGE_MEDIA *    apRetMedia
)
{
    apRetMedia->mBlockCount = (apDevice->mPageCount * K2_VA_MEMPAGE_BYTES) / RAMDISK_BLOCKSIZE;
    apRetMedia->mBlockSizeBytes = RAMDISK_BLOCKSIZE;
    K2ASC_Copy(apRetMedia->mFriendly, "RAMDISK");
    apRetMedia->mTotalBytes = ((UINT64)apRetMedia->mBlockCount) * ((UINT64)RAMDISK_BLOCKSIZE);
    apRetMedia->mUniqueId = 0xFEEDF00D + (UINT32)apDevice;
    return K2STAT_NO_ERROR;
}

K2STAT
RAMDISK_Transfer(
    RAMDISK_DEVICE *                apDevice,
    K2OS_BLOCKIO_TRANSFER const *   apTransfer
)
{
    UINT8 * pBlock;

    pBlock = (UINT8 *)(apDevice->mVirtBase + (((UINT32)apTransfer->mStartBlock) * RAMDISK_BLOCKSIZE));

    if (!apTransfer->mIsWrite)
    {
        K2MEM_Copy((void *)apTransfer->mAddress, pBlock, apTransfer->mBlockCount * RAMDISK_BLOCKSIZE);
    }
    else
    {
        K2MEM_Copy(pBlock, (void *)apTransfer->mAddress, apTransfer->mBlockCount * RAMDISK_BLOCKSIZE);
    }

    K2_CpuFullBarrier();

    return K2STAT_NO_ERROR;
}

static K2OSDDK_BLOCKIO_REGISTER sgBlockIoFuncTab =
{
    { FALSE },  // does not use hardware addresses
    (K2OSDDK_pf_BlockIo_GetMedia)RAMDISK_GetMedia,
    (K2OSDDK_pf_BlockIo_Transfer)RAMDISK_Transfer
};

K2STAT
CreateInstance(
    K2OS_DEVCTX aDevCtx,
    void **     appRetDriverContext
)
{
    RAMDISK_DEVICE *    pDevice;
    K2STAT              stat;

    pDevice = (RAMDISK_DEVICE *)K2OS_Heap_Alloc(sizeof(RAMDISK_DEVICE));
    if (NULL == pDevice)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pDevice, sizeof(RAMDISK_DEVICE));

    pDevice->mDevCtx = aDevCtx;
    *appRetDriverContext = pDevice;

    return K2STAT_NO_ERROR;
}

K2STAT
StartDriver(
    RAMDISK_DEVICE *    apDevice
)
{
    K2STAT                  stat;
    K2OS_PAGEARRAY_TOKEN    tokPageArray;

    apDevice->mPageCount = RAMDISK_PAGES;

    tokPageArray = K2OS_PageArray_Create(apDevice->mPageCount);
    if (NULL == tokPageArray)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    apDevice->mVirtBase = K2OS_Virt_Reserve(apDevice->mPageCount);
    if (0 == apDevice->mVirtBase)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Token_Destroy(tokPageArray);
        return stat;
    }

    apDevice->mTokVirtMap = K2OS_VirtMap_Create(
        tokPageArray, 
        0, 
        apDevice->mPageCount, 
        apDevice->mVirtBase, 
        K2OS_MapType_Data_ReadWrite
    );

    K2OS_Token_Destroy(tokPageArray);

    if (NULL == apDevice->mTokVirtMap)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Virt_Release(apDevice->mVirtBase);
        apDevice->mVirtBase = 0;
    }

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
        K2OS_Token_Destroy(apDevice->mTokVirtMap);
        apDevice->mTokVirtMap = NULL;

        K2OS_Virt_Release(apDevice->mVirtBase);
        apDevice->mVirtBase = 0;
    }

    return stat;
}

K2STAT
StopDriver(
    RAMDISK_DEVICE *    apDevice
)
{
    K2OSDDK_BlockIoDeregister(apDevice->mDevCtx, apDevice);

    K2OSDDK_DriverStopped(apDevice->mDevCtx, K2STAT_NO_ERROR);

    return K2STAT_NO_ERROR;
}

K2STAT
DeleteInstance(
    RAMDISK_DEVICE *    apDevice
)
{
    K2OS_Token_Destroy(apDevice->mTokVirtMap);
    apDevice->mTokVirtMap = NULL;

    K2OS_Virt_Release(apDevice->mVirtBase);
    apDevice->mVirtBase = 0;

    K2OS_Heap_Free(apDevice);

    return K2STAT_NO_ERROR;
}
