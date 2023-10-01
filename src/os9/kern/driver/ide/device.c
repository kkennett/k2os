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
IDE_Device_GetMedia(
    IDE_DEVICE *        apDevice,
    K2OS_STORAGE_MEDIA *apRetMedia
)
{
    K2STAT stat;

    K2OS_CritSec_Enter(&apDevice->Sec);

    if ((!apDevice->mIsRemovable) || (apDevice->mMediaPresent))
    {
        K2MEM_Copy(apRetMedia, &apDevice->Media, sizeof(K2OS_STORAGE_MEDIA));
        stat = K2STAT_NO_ERROR;
    }
    else
    {
        stat = K2STAT_ERROR_NO_MEDIA;
    }

    K2OS_CritSec_Leave(&apDevice->Sec);

    return stat;
}

K2STAT 
IDE_Device_Transfer(
    IDE_DEVICE *                    apDevice,
    K2OS_BLOCKIO_TRANSFER const *   apTransfer
)
{
    switch (apTransfer->mType)
    {
    case K2OS_BlockIoTransfer_Read:
        K2MEM_Set((void *)apTransfer->mTargetAddr, 0x5A, apTransfer->mBlockCount * apDevice->Media.mBlockSizeBytes);
        break;

    case K2OS_BlockIoTransfer_Write:
    case K2OS_BlockIoTransfer_Erase:
        break;

    default:
        return K2STAT_ERROR_NOT_SUPPORTED;
    }

    K2_CpuFullBarrier();

    return K2STAT_NO_ERROR;
}
