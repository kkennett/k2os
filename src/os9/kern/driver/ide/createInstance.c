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
CreateInstance(
    K2OS_DEVCTX aDevCtx,
    void **     appRetDriverContext
)
{
    IDE_CONTROLLER *    pIdeController;
    IDE_CHANNEL *       pChannel;
    K2STAT              stat;

    pIdeController = (IDE_CONTROLLER *)K2OS_Heap_Alloc(sizeof(IDE_CONTROLLER));
    if (NULL == pIdeController)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pIdeController, sizeof(IDE_CONTROLLER));

    pIdeController->mDevCtx = aDevCtx;
    *appRetDriverContext = pIdeController;

    pChannel = &pIdeController->Channel[IDE_CHANNEL_PRIMARY];
    pChannel->mpController = pIdeController;
    pChannel->mChannelIndex = IDE_CHANNEL_PRIMARY;
    pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mpChannel = pChannel;
    pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mDeviceIndex = IDE_CHANNEL_DEVICE_MASTER;
    pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mpChannel = pChannel;
    pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mDeviceIndex = IDE_CHANNEL_DEVICE_SLAVE;

    pChannel = &pIdeController->Channel[IDE_CHANNEL_SECONDARY];
    pChannel->mpController = pIdeController;
    pChannel->mChannelIndex = IDE_CHANNEL_SECONDARY;
    pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mpChannel = pChannel;
    pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mDeviceIndex = IDE_CHANNEL_DEVICE_MASTER;
    pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mpChannel = pChannel;
    pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mDeviceIndex = IDE_CHANNEL_DEVICE_SLAVE;

    pIdeController->mTokMailbox = K2OS_Mailbox_Create();
    if (NULL == pIdeController->mTokMailbox)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
    }
    else
    {
        pIdeController->mTokThread = K2OS_Thread_Create("IdeController", (K2OS_pf_THREAD_ENTRY)IDE_Instance_Thread, pIdeController, NULL, &pIdeController->mThreadId);
        if (NULL == pIdeController->mTokThread)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Token_Destroy(pIdeController->mTokMailbox);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pIdeController);
    }

    return stat;
}

