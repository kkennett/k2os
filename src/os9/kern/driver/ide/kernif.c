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
    K2OSKERN_SeqInit(&pChannel->IrqSeqLock);

    pChannel = &pIdeController->Channel[IDE_CHANNEL_SECONDARY];
    pChannel->mpController = pIdeController;
    pChannel->mChannelIndex = IDE_CHANNEL_SECONDARY;
    pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mpChannel = pChannel;
    pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mDeviceIndex = IDE_CHANNEL_DEVICE_MASTER;
    pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mpChannel = pChannel;
    pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mDeviceIndex = IDE_CHANNEL_DEVICE_SLAVE;
    K2OSKERN_SeqInit(&pChannel->IrqSeqLock);

    return K2STAT_NO_ERROR;
}

K2STAT
StartDriver(
    IDE_CONTROLLER *apController
)
{
    K2STAT              stat;
    UINT32              resIx;
    K2OS_THREAD_TOKEN   tokThread;
    IDE_CHANNEL *       pChannel;

    stat = K2OSDDK_GetInstanceInfo(apController->mDevCtx, &apController->InstInfo);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** IDE(%08X): Enum resources failed (0x%08X)\n", apController, stat);
        return stat;
    }

    if ((apController->InstInfo.mCountIo + apController->InstInfo.mCountPhys) < 4)
    {
        K2OSKERN_Debug("*** IDE(%08X): insufficient ios (io+phys) for IDE bus (%d specified)\n", K2OS_Process_GetId(), apController, apController->InstInfo.mCountIo + apController->InstInfo.mCountPhys);
        return K2STAT_ERROR_NOT_EXIST;
    }

    if (apController->InstInfo.mCountIo > 5)
        apController->InstInfo.mCountIo = 5;
    for (resIx = 0; resIx < apController->InstInfo.mCountIo; resIx++)
    {
        stat = K2OSDDK_GetRes(apController->mDevCtx, K2OS_RESTYPE_IO, resIx, &apController->ResIo[resIx]);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** IDE(%08X): Could not get io resource %d (0x%08X)\n", apController, resIx, stat);
            return stat;
        }
    }

    if (apController->InstInfo.mCountPhys > 5)
        apController->InstInfo.mCountPhys = 5;
    for (resIx = 0; resIx < apController->InstInfo.mCountPhys; resIx++)
    {
        stat = K2OSDDK_GetRes(apController->mDevCtx, K2OS_RESTYPE_PHYS, resIx, &apController->ResPhys[resIx]);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** IDE(%08X): Could not get phys resource %d (0x%08X)\n", apController, resIx, stat);
            return stat;
        }
    }

    if (apController->InstInfo.mCountIrq > 2)
        apController->InstInfo.mCountIrq = 2;
    for (resIx = 0; resIx < apController->InstInfo.mCountIrq; resIx++)
    {
        stat = K2OSDDK_GetRes(apController->mDevCtx, K2OS_RESTYPE_IRQ, resIx, &apController->ResIrq[resIx]);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** IDE(%08X): Could not get irq resource %d (0x%08X)\n", apController, resIx, stat);
            return stat;
        }
    }

    apController->mPopMask = 0;

    stat = IDE_InitAndDiscover(apController);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (0 == apController->mPopMask)
    {
        K2OSKERN_Debug("*** IDE(%08X): No devices found\n", apController);
        return K2STAT_ERROR_NO_MORE_ITEMS;
    }

    K2OSDDK_DriverStarted(apController->mDevCtx);

    //
    // start the channels
    //
    if (apController->mPopMask & 0x3)
    {
        pChannel = &apController->Channel[IDE_CHANNEL_PRIMARY];
        if (!K2OS_CritSec_Init(&pChannel->Sec))
        {
            stat = K2OS_Thread_GetLastStatus();
            K2OSKERN_Debug("*** IDE(%08X): Failed to init primary channel critsec (0x%08X)\n", apController, stat);
            apController->mPopMask &= ~3;
        }
        else
        {
            pChannel->mTokNotify = K2OS_Notify_Create(TRUE);    // starts in evalident
            if (NULL == pChannel->mTokNotify)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2OSKERN_Debug("*** IDE(%08X): Failed to init primary channel notify (0x%08X)\n", apController, stat);
                apController->mPopMask &= ~3;
            }
            else
            {
                if (0 != (apController->mPopMask & 1))
                {
                    pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mTokTransferWaitNotify = K2OS_Notify_Create(FALSE);
                    if (NULL == pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mTokTransferWaitNotify)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2OSKERN_Debug("*** IDE(%08X): Failed to init primary master device notify (0x%08X)\n", apController, stat);
                        apController->mPopMask &= ~1;
                    }
                }
                if (0 != (apController->mPopMask & 2))
                {
                    pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mTokTransferWaitNotify = K2OS_Notify_Create(FALSE);
                    if (NULL == pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mTokTransferWaitNotify)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2OSKERN_Debug("*** IDE(%08X): Failed to init primary slave device notify (0x%08X)\n", apController, stat);
                        apController->mPopMask &= ~2;
                    }
                }
                if (0 != (apController->mPopMask & 3))
                {
                    tokThread = K2OS_Thread_Create("IDEPrimary", (K2OS_pf_THREAD_ENTRY)IDE_Channel_Thread, (void *)pChannel, NULL, &pChannel->mThreadId);
                    if (NULL == tokThread)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                    }
                }
                else
                {
                    tokThread = NULL;
                }
                if (NULL == tokThread)
                {
                    if (0 != (apController->mPopMask & 1))
                    {
                        K2OS_Token_Destroy(pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mTokTransferWaitNotify);
                        pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mTokTransferWaitNotify = NULL;
                    }
                    if (0 != (apController->mPopMask & 2))
                    {
                        K2OS_Token_Destroy(pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mTokTransferWaitNotify);
                        pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mTokTransferWaitNotify = NULL;
                    }
                    apController->mPopMask &= ~3;
                    K2OSKERN_Debug("*** IDE(%08X): Primary channel failed to start (0x%08X)\n", apController, stat);
                    K2OS_Token_Destroy(pChannel->mTokNotify);
                }
                else
                {
                    K2OS_Token_Destroy(tokThread);
                }
            }

            if (0 == (apController->mPopMask & 0x3))
            {
                K2OS_CritSec_Done(&pChannel->Sec);
            }
        }
    }

    if (apController->mPopMask & 0xC)
    {
        pChannel = &apController->Channel[IDE_CHANNEL_SECONDARY];
        if (!K2OS_CritSec_Init(&pChannel->Sec))
        {
            stat = K2OS_Thread_GetLastStatus();
            K2OSKERN_Debug("*** IDE(%08X): Failed to init Secondary channel critsec (0x%08X)\n", apController, stat);
            apController->mPopMask &= ~0x0C;
        }
        else
        {
            pChannel->mTokNotify = K2OS_Notify_Create(TRUE);    // starts in evalident
            if (NULL == pChannel->mTokNotify)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2OSKERN_Debug("*** IDE(%08X): Failed to init Secondary channel notify (0x%08X)\n", apController, stat);
                apController->mPopMask &= ~0x0C;
            }
            else
            {
                if (0 != (apController->mPopMask & 4))
                {
                    pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mTokTransferWaitNotify = K2OS_Notify_Create(FALSE);
                    if (NULL == pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mTokTransferWaitNotify)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2OSKERN_Debug("*** IDE(%08X): Failed to init secondary master device notify (0x%08X)\n", apController, stat);
                        apController->mPopMask &= ~4;
                    }
                }
                if (0 != (apController->mPopMask & 8))
                {
                    pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mTokTransferWaitNotify = K2OS_Notify_Create(FALSE);
                    if (NULL == pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mTokTransferWaitNotify)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2OSKERN_Debug("*** IDE(%08X): Failed to init secondary slave device notify (0x%08X)\n", apController, stat);
                        apController->mPopMask &= ~8;
                    }
                }
                if (0 != (apController->mPopMask & 3))
                {
                    tokThread = K2OS_Thread_Create("IDESecondary", (K2OS_pf_THREAD_ENTRY)IDE_Channel_Thread, (void *)pChannel, NULL, &pChannel->mThreadId);
                    if (NULL == tokThread)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                    }
                }
                else
                {
                    tokThread = NULL;
                }
                if (NULL == tokThread)
                {
                    if (0 != (apController->mPopMask & 4))
                    {
                        K2OS_Token_Destroy(pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mTokTransferWaitNotify);
                        pChannel->Device[IDE_CHANNEL_DEVICE_MASTER].mTokTransferWaitNotify = NULL;
                    }
                    if (0 != (apController->mPopMask & 8))
                    {
                        K2OS_Token_Destroy(pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mTokTransferWaitNotify);
                        pChannel->Device[IDE_CHANNEL_DEVICE_SLAVE].mTokTransferWaitNotify = NULL;
                    }
                    apController->mPopMask &= ~0xC;
                    K2OSKERN_Debug("*** IDE(%08X): Secondary channel failed to start (0x%08X)\n", apController, stat);
                    K2OS_Token_Destroy(pChannel->mTokNotify);
                }
                else
                {
                    K2OS_Token_Destroy(tokThread);
                }
            }

            if (0 == (apController->mPopMask & 0x0C))
            {
                K2OS_CritSec_Done(&pChannel->Sec);
            }
        }
    }

    if (0 == apController->mPopMask)
    {
        K2OSKERN_Debug("*** IDE(%08X): All available channels failed to have threads start\n", apController);
        K2OSDDK_DriverStopped(apController->mDevCtx, K2STAT_ERROR_NO_MORE_ITEMS);
        return K2STAT_ERROR_NO_MORE_ITEMS;
    }

    K2OSDDK_SetEnable(apController->mDevCtx, TRUE);

    return K2STAT_NO_ERROR;
}

K2STAT
StopDriver(
    IDE_CONTROLLER *apController
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
DeleteInstance(
    IDE_CONTROLLER *apController
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}
