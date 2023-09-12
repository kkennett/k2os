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
StartDriver(
    IDE_CONTROLLER *apController
)
{
    K2STAT              stat;
    UINT32              resIx;
    K2OS_THREAD_TOKEN   tokThread;
    IDE_CHANNEL *       pChannel;

    stat = K2OSDDK_GetInstanceInfo(apController->mDevCtx, &apController->Ident, &apController->mCountIo, &apController->mCountPhys, &apController->mCountIrq);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** IDE(%08X): Enum resources failed (0x%08X)\n", apController, stat);
        return stat;
    }

    if ((apController->mCountIo + apController->mCountPhys) < 4)
    {
        K2OSKERN_Debug("*** IDE(%08X): insufficient ios (io+phys) for IDE bus (%d specified)\n", K2OS_Process_GetId(), apController, apController->mCountIo + apController->mCountPhys);
        return K2STAT_ERROR_NOT_EXIST;
    }

    if (apController->mCountIo > 5)
        apController->mCountIo = 5;
    for (resIx = 0; resIx < apController->mCountIo; resIx++)
    {
        stat = K2OSDDK_GetRes(apController->mDevCtx, K2OS_RESTYPE_IO, resIx, &apController->ResIo[resIx]);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** IDE(%08X): Could not get io resource %d (0x%08X)\n", apController, resIx, stat);
            return stat;
        }
    }

    if (apController->mCountPhys > 5)
        apController->mCountPhys = 5;
    for (resIx = 0; resIx < apController->mCountPhys; resIx++)
    {
        stat = K2OSDDK_GetRes(apController->mDevCtx, K2OS_RESTYPE_PHYS, resIx, &apController->ResPhys[resIx]);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** IDE(%08X): Could not get phys resource %d (0x%08X)\n", apController, resIx, stat);
            return stat;
        }
    }

    if (apController->mCountIrq > 2)
        apController->mCountIrq = 2;
    for (resIx = 0; resIx < apController->mCountIrq; resIx++)
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
                tokThread = K2OS_Thread_Create("IDEPrimary", (K2OS_pf_THREAD_ENTRY)IDE_Channel_Thread, (void *)pChannel, NULL, &pChannel->mThreadId);
                if (NULL == tokThread)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    apController->mPopMask &= ~3;
                    K2OSKERN_Debug("*** IDE(%08X): Primary channel thread failed to start (0x%08X)\n", apController, stat);
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
                tokThread = K2OS_Thread_Create("IDESecondary", (K2OS_pf_THREAD_ENTRY)IDE_Channel_Thread, (void *)pChannel, NULL, &pChannel->mThreadId);
                if (NULL == tokThread)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    apController->mPopMask &= 0x0C;
                    K2OSKERN_Debug("*** IDE(%08X): Secondary channel thread failed to start (0x%08X)\n", apController, stat);
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
        return K2STAT_ERROR_NO_MORE_ITEMS;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
OnDdkMessage(
    IDE_CONTROLLER *    apController,
    K2OS_MSG const *    apMsg
)
{
    K2STAT stat;

    if (apMsg->mShort == K2OS_SYSTEM_MSG_DDK_SHORT_START)
    {
        stat = StartDriver(apController);
    }
    else
    {
        K2OSKERN_Debug("IDE(%08X): caught unsupported DDK message %d\n", apController, apMsg->mShort);
        stat = K2STAT_NO_ERROR;
    }

    return stat;
}

UINT32 
IDE_Instance_Thread(
    IDE_CONTROLLER *apController
)
{
    K2OS_MSG    msg;
    K2STAT      stat;

    stat = K2OSDDK_SetMailslot(apController->mDevCtx, (UINT32)apController->mTokMailbox);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    do {
        if (!K2OS_Mailbox_Recv(apController->mTokMailbox, &msg, K2OS_TIMEOUT_INFINITE))
        {
            stat = K2OS_Thread_GetLastStatus();
            K2OSKERN_Debug("IDE(%08X): Mailbox Recv returned failure - %08X\n", apController, stat);
            break;
        }

        if (msg.mType == K2OS_SYSTEM_MSGTYPE_DDK)
        {
            stat = OnDdkMessage(apController, &msg);
            if (K2STAT_IS_ERROR(stat))
                break;
        }
        else
        {
            K2OSKERN_Debug("IDE(%08X): caught unsupported type message 0x%04X\n", apController, msg.mType);
        }
    } while (1);

    return K2OSDDK_DriverStopped(apController->mDevCtx, stat);
}

