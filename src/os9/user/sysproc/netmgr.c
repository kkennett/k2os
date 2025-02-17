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

#include "sysproc.h"
#include "netmgr.h"

static K2OS_THREAD_TOKEN    sgNetMgrTokThread;
static UINT32               sgNetMgrThreadId;
static K2OS_CRITSEC         sgNetDevListSec;
static K2LIST_ANCHOR        sgNetDevList;
static K2OS_MAILBOX_TOKEN   sgNetMgrTokMailbox;

UINT32
NetMgr_Adapter_Thread(
    NETDEV *    apNetDev
)
{
    K2OS_WaitResult             waitResult;
    K2OS_MSG                    msg;
    K2OS_TOKEN                  tok[2];
    UINT32                      msTicks;
    UINT64                      before;
    UINT64                      after;
    K2LIST_LINK *               pListLink;
    NETDEV *                    pNetDev;
    K2OS_IPV4_ADAPTER_ASSIGN    DefaultAssign;

    apNetDev->mTokMailbox = K2OS_Mailbox_Create();
    if (NULL != apNetDev->mTokMailbox)
    {
        apNetDev->mNetIo = K2OS_NetIo_Attach(apNetDev->mIfInstId, apNetDev, apNetDev->mTokMailbox);
        if (NULL != apNetDev->mNetIo)
        {
            if (K2OS_NetIo_GetDesc(apNetDev->mNetIo, &apNetDev->Desc))
            {
                //
                // Acquire the IPV4_ADAPTER_ASSIGN for this adapter
                //
                K2MEM_Zero(&DefaultAssign, sizeof(DefaultAssign));
                K2ASC_Copy(DefaultAssign.Config.mHostName, "K2Host");
                DefaultAssign.Config.mUseDhcp = TRUE;
                //
                // later this will have been found in persistent storage
                //

                if (NetDev_Init(apNetDev, &DefaultAssign.Config))
                {
                    if (K2OS_NetIo_SetEnable(apNetDev->mNetIo, TRUE))
                    {
                        Debug_Printf("NETMGR: adapter \"%s\" enabled\n", apNetDev->Desc.mName);

                        tok[0] = apNetDev->mTokMailbox;
                        tok[1] = apNetDev->mTokDoneGate;

                        NetDev_Start(apNetDev);

                        do {
                            if (NULL != apNetDev->mpTimers)
                            {
                                msTicks = apNetDev->mpTimers->mLeft;
                            }
                            else
                            {
                                msTicks = K2OS_TIMEOUT_INFINITE;
                            }

                            K2OS_System_GetMsTick(&before);

                            waitResult = K2OS_Wait_TooManyTokens;
                            K2OS_Thread_WaitMany(&waitResult, 2, tok, FALSE, msTicks);

                            K2OS_System_GetMsTick(&after);
                            msTicks = (UINT32)(after - before);
                            if (0 != msTicks)
                            {
                                NetDev_OnTimeExpired(apNetDev, msTicks);
                            }

                            if (K2OS_Wait_TimedOut != waitResult)
                            {
                                if (K2OS_Wait_Signalled_0 != waitResult)
                                {
                                    // something went wrong
                                    Debug_Printf("***NETMGR: Unexpected wait result %d\n", waitResult);
                                    break;
                                }

                                if (K2OS_Mailbox_Recv(apNetDev->mTokMailbox, &msg))
                                {
                                    do {
                                        before = after;

                                        if (msg.mMsgType == K2OS_NETIO_MSGTYPE)
                                        {
                                            NetDev_Recv_NetIo(apNetDev, (K2OS_NETIO_MSG *)&msg);
                                        }
                                        else if (msg.mMsgType == K2OS_SYSTEM_MSGTYPE_RPC)
                                        {
                                            if (msg.mShort == K2OS_SYSTEM_MSG_RPC_SHORT_NOTIFY)
                                            {
                                                pNetDev = NULL;
                                                K2OS_CritSec_Enter(&sgNetDevListSec);
                                                pListLink = sgNetDevList.mpHead;
                                                if (NULL != pListLink)
                                                {
                                                    do {
                                                        pNetDev = K2_GET_CONTAINER(NETDEV, pListLink, ListLink);
                                                        if (msg.mPayload[0] == (UINT32)pNetDev->mNetIo)
                                                            break;
                                                        pListLink = pListLink->mpNext;
                                                    } while (NULL != pListLink);
                                                }
                                                K2OS_CritSec_Leave(&sgNetDevListSec);
                                                if (pListLink != NULL)
                                                {
                                                    NetDev_Recv_RpcNotify(pNetDev, &msg);
                                                }
                                            }
                                            else
                                            {
                                                Debug_Printf("***NETMGR: received unexpected RPC message (%04X)\n", msg.mShort);
                                            }
                                        }
                                        else
                                        {
                                            Debug_Printf("***NETMGR: received unexpected message type (%04X)\n", msg.mMsgType);
                                        }

                                        K2OS_System_GetMsTick(&after);
                                        msTicks = (UINT32)(after - before);

                                        if (0 != msTicks)
                                        {
                                            NetDev_OnTimeExpired(apNetDev, msTicks);
                                        }

                                    } while (K2OS_Mailbox_Recv(apNetDev->mTokMailbox, &msg));
                                }
                            }

                        } while (!apNetDev->mStop);

                        Debug_Printf("NETMGR: adapter \"%s\" disabled\n", apNetDev->Desc.mName);
                        K2OS_NetIo_SetEnable(apNetDev->mNetIo, FALSE);

                        NetDev_Stopped(apNetDev);
                    }
                    else
                    {
                        Debug_Printf("***NETMGR: adapter \"%s\" could not be enabled (%08X)\n", apNetDev->Desc.mName, K2OS_Thread_GetLastStatus());
                    }

                    NetDev_Deinit(apNetDev);
                }
            }
            else
            {
                Debug_Printf("***NETMGR: descriptor read for ifinstId %d failed (%08X)\n", apNetDev->mIfInstId, K2OS_Thread_GetLastStatus());
            }

            K2OS_NetIo_Detach(apNetDev->mNetIo);
        }
        else
        {
            Debug_Printf("***NETMGR: Could not attach to netio device by interface instance id (%08X)\n", K2OS_Thread_GetLastStatus());
        }

        K2OS_Token_Destroy(apNetDev->mTokMailbox);
    }

    K2OS_Thread_WaitOne(&waitResult, apNetDev->mTokDoneGate, K2OS_TIMEOUT_INFINITE);

    K2OS_Heap_Free(apNetDev);

    return 0;
}

void
NetMgr_Thread_NetIo_Arrived(
    K2OS_IFINST_ID aIfInstId
)
{
    NETDEV *            pNetDev;
    K2STAT              stat;
    K2OS_THREAD_TOKEN   tokThread;

    pNetDev = (NETDEV *)K2OS_Heap_Alloc(sizeof(NETDEV));
    if (NULL == pNetDev)
    {
        Debug_Printf("NETMGR: Failed to allocate memory for network device with netio ifinstid %d\n", aIfInstId);
        return;
    }

    K2MEM_Zero(pNetDev, sizeof(NETDEV));
    pNetDev->mIfInstId = aIfInstId;

    stat = K2STAT_ERROR_UNKNOWN;

    pNetDev->mTokDoneGate = K2OS_Gate_Create(FALSE);
    if (NULL != pNetDev->mTokDoneGate)
    {
        K2OS_CritSec_Enter(&sgNetDevListSec);
        K2LIST_AddAtTail(&sgNetDevList, &pNetDev->ListLink);
        K2OS_CritSec_Leave(&sgNetDevListSec);

        tokThread = K2OS_Thread_Create("NetDev", (K2OS_pf_THREAD_ENTRY)NetMgr_Adapter_Thread, pNetDev, NULL, &pNetDev->mThreadId);
        if (NULL == tokThread)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));

            K2OS_CritSec_Enter(&sgNetDevListSec);
            K2LIST_Remove(&sgNetDevList, &pNetDev->ListLink);
            K2OS_CritSec_Leave(&sgNetDevListSec);

            K2OS_Token_Destroy(pNetDev->mTokDoneGate);
        }
        else
        {
            K2OS_Token_Destroy(tokThread);
            stat = K2STAT_NO_ERROR;
        }
    }
    else
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pNetDev);
    }
}

void
NetMgr_Thread_NetIo_Departed(
    K2OS_IFINST_ID aIfInstId
)
{
    K2LIST_LINK *   pListLink;
    NETDEV *        pNetDev;

    K2OS_CritSec_Enter(&sgNetDevListSec);

    pNetDev = NULL;
    pListLink = sgNetDevList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pNetDev = K2_GET_CONTAINER(NETDEV, pListLink, ListLink);
            if (pNetDev->mIfInstId == aIfInstId)
            {
                K2LIST_Remove(&sgNetDevList, &pNetDev->ListLink);
                break;
            }
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&sgNetDevListSec);

    if (NULL == pListLink)
    {
        return;
    }
    
    K2_ASSERT(NULL != pNetDev);

    // as soon as this is set pNetDev should disable and deallocate on its thread
    K2OS_Signal_Set(pNetDev->mTokDoneGate);
}

UINT32
NetMgr_Thread(
    void *apArg
)
{
    static K2_GUID128 const sNetIoIfaceId = K2OS_IFACE_NETIO_DEVICE;
    K2OS_IFSUBS_TOKEN   tokSubs;
    K2OS_MSG            msg;
    K2OS_WaitResult     waitResult;

    sgNetMgrTokMailbox = K2OS_Mailbox_Create();
    if (NULL == sgNetMgrTokMailbox)
    {
        Debug_Printf("***NETMGR: could not create a mailbox\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    tokSubs = K2OS_IfSubs_Create(sgNetMgrTokMailbox, K2OS_IFACE_CLASSCODE_NETWORK_DEVICE, &sNetIoIfaceId, 12, FALSE, NULL);
    if (NULL == tokSubs)
    {
        Debug_Printf("***NETMGR: could not subscribe to netio device interface pop changes\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, sgNetMgrTokMailbox, K2OS_TIMEOUT_INFINITE))
            break;
        K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);
        if (K2OS_Mailbox_Recv(sgNetMgrTokMailbox, &msg))
        {
            if (msg.mMsgType == K2OS_SYSTEM_MSGTYPE_IFINST)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    // payload[0] = subscription context
                    // payload[1] = interface instance id
                    // payload[2] = process that published the interface
                    NetMgr_Thread_NetIo_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                }
                else if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    NetMgr_Thread_NetIo_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                }
                else
                {
                    Debug_Printf("***NETMGR: received unexpected IFINST message (%04X)\n", msg.mShort);
                }
            }
            else
            {
                Debug_Printf("***NETMGR: received unexpected message type (%04X)\n", msg.mMsgType);
            }
        }
    } while (1);

    return 0;
}

void
NetMgr_Init(
    void
)
{
    if (!K2OS_CritSec_Init(&sgNetDevListSec))
    {
        Debug_Printf("***NETMGR: Could not create cs for network device list\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    K2LIST_Init(&sgNetDevList);

    sgNetMgrTokThread = K2OS_Thread_Create("Network Manager", NetMgr_Thread, NULL, NULL, &sgNetMgrThreadId);
    if (NULL == sgNetMgrTokThread)
    {
        Debug_Printf("***NETMGR: thread coult not be started\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }
}

