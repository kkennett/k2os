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
#include <k2osdev_blockio.h>

static K2OS_THREAD_TOKEN    sgStorMgrTokThread;
static UINT32               sgStorMgrThreadId;
static K2OS_CRITSEC         sgStorDevListSec;
static K2LIST_ANCHOR        sgStorDevList;
static K2OS_MAILBOX_TOKEN   sgStorMgrTokMailbox;

typedef struct _STORDEV         STORDEV;
typedef struct _STORDEV_ACTION  STORDEV_ACTION;

typedef enum _StorDev_ActionType StorDev_ActionType;
enum _StorDev_ActionType
{
    StorDev_Action_Invalid = 0,

    StorDev_Action_Media_Changed,
    StorDev_Action_BlockIo_Departed,

    StorDev_ActionType_Count
};

struct _STORDEV_ACTION
{
    StorDev_ActionType          mAction;
    STORDEV_ACTION * volatile   mpNext;
};

struct _STORDEV
{
    K2OS_IFINST_ID              mIfInstId;
    UINT32                      mThreadId;
    K2LIST_LINK                 ListLink;
    K2OS_STORAGE_MEDIA          Media;
    K2OS_NOTIFY_TOKEN           mTokActionNotify;
    K2OS_BLOCKIO_TOKEN          mTokBlockIo;
    STORDEV_ACTION * volatile   mpActions;
};

void
StorDev_Mount(
    STORDEV *   apDev
)
{
    K2OS_BLOCKIO_RANGE  range;
    UINT8 *             pBlockBuffer;
    UINT8 *             pBlock;

    Debug_Printf("STORMGR: Thread %d IfInstId %d Mount \"%s\"\n", K2OS_Thread_GetId(), apDev->mIfInstId, apDev->Media.mFriendly);

    range = K2OS_BlockIo_RangeCreate(apDev->mTokBlockIo, 0, apDev->Media.mBlockCount, TRUE);
    if (NULL == range)
    {
        Debug_Printf("STORMGR: Failed to create private range (whole device)\n");
        return;
    }

    pBlockBuffer = (UINT8 *)K2OS_Heap_Alloc(apDev->Media.mBlockSizeBytes * 2);
    if (NULL == pBlockBuffer)
    {
        Debug_Printf("STORMGR: memory alloc failed\n");
    }
    else
    {
        // align to block size
        pBlock = (UINT8 *)(((((UINT32)pBlockBuffer) + (apDev->Media.mBlockSizeBytes - 1)) / apDev->Media.mBlockSizeBytes) * apDev->Media.mBlockSizeBytes);

        if (!K2OS_BlockIo_Read(apDev->mTokBlockIo, range, 0, pBlock, apDev->Media.mBlockSizeBytes))
        {
            Debug_Printf("STORMGR: read failed %08X\n", K2OS_Thread_GetLastStatus());
        }
        else
        {
            Debug_Printf("STORMGR: read block 0 ok\n", K2OS_Thread_GetLastStatus());
        }

        K2OS_Heap_Free(pBlockBuffer);
    }

    K2OS_BlockIo_RangeDelete(apDev->mTokBlockIo, range);
}

void
StorDev_Unmount(
    STORDEV *   apDev
)
{
    Debug_Printf("STORMGR: Thread %d IfInstId %d Unmount \"%s\"\n", K2OS_Thread_GetId(), apDev->mIfInstId, apDev->Media.mFriendly);
}

void
StorDev_OnMediaChange(
    STORDEV *   apDev
)
{
    K2STAT  stat;

    if (apDev->Media.mBlockSizeBytes > 0)
    {
        StorDev_Unmount(apDev);
    }

    // media should be empty here
    K2_ASSERT(0 == apDev->Media.mBlockSizeBytes);

    if (K2OS_BlockIo_GetMedia(apDev->mTokBlockIo, &apDev->Media))
    {
        if ((apDev->Media.mBlockSizeBytes > 0) &&
            (apDev->Media.mBlockCount > 0))
        {
            StorDev_Mount(apDev);
        }
        else
        {
            Debug_Printf("STORMGR: Thread %d IfInstId %d -- no blocks or 0 byte blocks\n", K2OS_Thread_GetId(), apDev->mIfInstId);
            apDev->Media.mBlockSizeBytes = 0;
        }
    }
    else
    {
        stat = K2OS_Thread_GetLastStatus();
        if (stat == K2STAT_ERROR_NO_MEDIA)
        {
            Debug_Printf("STORMGR: Thread %d IfInstId %d -- No media\n", K2OS_Thread_GetId(), apDev->mIfInstId);
        }
        else
        {
            Debug_Printf("STORMGR: Thread %d IfInstId %d returned %08X to GetMedia()\n", K2OS_Thread_GetId(), apDev->mIfInstId, stat);
        }
    }
}

void
StorDev_OnDepart(
    STORDEV *   apDev
)
{
    if (apDev->Media.mBlockSizeBytes > 0)
    {
        StorDev_Unmount(apDev);
    }

    apDev->mIfInstId = 0;

    K2OS_Token_Destroy(apDev->mTokBlockIo);
    apDev->mTokBlockIo = NULL;
}

UINT32
StorMgr_DevThread(
    void *apArg
)
{
    STORDEV *           pThisDev;
    K2OS_WaitResult     waitResult;
    STORDEV_ACTION *    pActions;
    STORDEV_ACTION *    pLast;
    STORDEV_ACTION *    pNext;

    pThisDev = (STORDEV *)apArg;

//    Debug_Printf("STORMGR: Thread %d is servicing device with ifinstid %d\n", K2OS_Thread_GetId(), pThisDev->mIfInstId);

    StorDev_OnMediaChange(pThisDev);

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, pThisDev->mTokActionNotify, K2OS_TIMEOUT_INFINITE))
            break;
        K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);

        do {
            //
            // snapshot actions
            //
            pActions = (STORDEV_ACTION *)K2ATOMIC_Exchange((volatile UINT32 *)&pThisDev->mpActions, 0);
            if (NULL == pActions)
                break;

            // reverse the list
            pLast = NULL;
            do {
                pNext = pActions->mpNext;
                pActions->mpNext = pLast;
                pLast = pActions;
                pActions = pNext;
            } while (NULL != pActions);
            pActions = pLast;

            do {
                pNext = pActions;
                pActions = pActions->mpNext;

                switch (pNext->mAction)
                {
                    case StorDev_Action_Media_Changed:
                        StorDev_OnMediaChange(pThisDev);
                        break;

                    case StorDev_Action_BlockIo_Departed:
                        StorDev_OnDepart(pThisDev);
                        break;

                    default:
                        K2_ASSERT(0);
                        break;
                }

                K2OS_Heap_Free(pNext);

            } while (NULL != pActions);

        } while (NULL != pThisDev->mTokBlockIo);

    } while (1);

    K2OS_Token_Destroy(pThisDev->mTokActionNotify);

    K2OS_CritSec_Enter(&sgStorDevListSec);
    K2LIST_Remove(&sgStorDevList, &pThisDev->ListLink);
    K2OS_CritSec_Leave(&sgStorDevListSec);

    K2OS_Heap_Free(pThisDev);

    return 0;
}

void
StorMgr_BlockIo_Arrived(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OS_BLOCKIO_TOKEN  tokBlockIo;
    STORDEV *           pStorDev;
    K2OS_THREAD_TOKEN   tokThread;
    char                threadName[K2OS_THREAD_NAME_BUFFER_CHARS];

    Debug_Printf("STORMGR: BlockIo ifInstId %d arrived\n", aIfInstId);

    tokBlockIo = K2OS_BlockIo_Attach(aIfInstId, K2OS_ACCESS_RW, K2OS_ACCESS_RW, sgStorMgrTokMailbox);
    if (NULL == tokBlockIo)
    {
        Debug_Printf("STORMGR: Could not attach to blockio device by interface instance id (%08X)\n", K2OS_Thread_GetLastStatus());
        return;
    }

    pStorDev = (STORDEV *)K2OS_Heap_Alloc(sizeof(STORDEV));
    if (NULL == pStorDev)
    {
        Debug_Printf("STORMGR: Failed to allocate memory for storage device with blockio ifinstid %d\n", aIfInstId);
        K2OS_Token_Destroy(tokBlockIo);
        return;
    }

    K2MEM_Zero(pStorDev, sizeof(STORDEV));
    pStorDev->mTokBlockIo = tokBlockIo;
    pStorDev->mIfInstId = aIfInstId;

    K2OS_CritSec_Enter(&sgStorDevListSec);
    K2LIST_AddAtTail(&sgStorDevList, &pStorDev->ListLink);
    K2OS_CritSec_Leave(&sgStorDevListSec);

    pStorDev->mTokActionNotify = K2OS_Notify_Create(FALSE);

    if (NULL != pStorDev->mTokActionNotify)
    {
        K2ASC_PrintfLen(threadName, K2OS_THREAD_NAME_BUFFER_CHARS - 1, "StorMgr Device Iface %d", aIfInstId);
        threadName[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;
        tokThread = K2OS_Thread_Create(threadName, StorMgr_DevThread, pStorDev, NULL, &pStorDev->mThreadId);
        if (NULL != tokThread)
        {
            K2OS_Token_Destroy(tokThread);
            return;
        }

        K2OS_Token_Destroy(pStorDev->mTokActionNotify);
    }

    Debug_Printf("STORMGR: failed to start thread for blockio device with ifinstid %d\n", aIfInstId);

    K2OS_CritSec_Enter(&sgStorDevListSec);
    K2LIST_Remove(&sgStorDevList, &pStorDev->ListLink);
    K2OS_CritSec_Leave(&sgStorDevListSec);

    K2OS_Token_Destroy(tokBlockIo);

    K2OS_Heap_Free(pStorDev);
}

void
StorMgr_BlockIo_ListLocked_Notify(
    STORDEV *   apStorDev,
    UINT32      aNotifyCode
)
{
    STORDEV_ACTION *    pAct;
    UINT32              v;
    K2OS_NOTIFY_TOKEN   tokNotify;

    pAct = (STORDEV_ACTION *)K2OS_Heap_Alloc(sizeof(STORDEV_ACTION));
    if (NULL == pAct)
        return;

    K2MEM_Zero(pAct, sizeof(STORDEV_ACTION));
    pAct->mAction = StorDev_Action_Media_Changed;

    tokNotify = apStorDev->mTokActionNotify;

    do {
        v = (UINT32)apStorDev->mpActions;
        pAct->mpNext = (STORDEV_ACTION *)v;
        K2_CpuWriteBarrier();
    } while (v != K2ATOMIC_CompareExchange((UINT32 volatile *)&apStorDev->mpActions, (UINT32)pAct, v));

    K2OS_Notify_Signal(tokNotify);
}

void
StorMgr_BlockIo_Departed(
    K2OS_IFINST_ID aIfInstId
)
{
    STORDEV_ACTION *    pAct;
    K2LIST_LINK *       pListLink;
    STORDEV *           pStorDev;
    UINT32              v;
    K2OS_NOTIFY_TOKEN   tokNotify;

    pAct = (STORDEV_ACTION *)K2OS_Heap_Alloc(sizeof(STORDEV_ACTION));
    if (NULL == pAct)
        return;

    K2OS_CritSec_Enter(&sgStorDevListSec);

    pListLink = sgStorDevList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pStorDev = K2_GET_CONTAINER(STORDEV, pListLink, ListLink);
            if (pStorDev->mIfInstId == aIfInstId)
                break;
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&sgStorDevListSec);

    if (NULL == pListLink)
    {
        K2OS_Heap_Free(pAct);
        return;
    }
    
    K2_ASSERT(NULL != pStorDev);

    K2MEM_Zero(pAct, sizeof(STORDEV_ACTION));
    pAct->mAction = StorDev_Action_BlockIo_Departed;

    tokNotify = pStorDev->mTokActionNotify;

    do {
        v = (UINT32)pStorDev->mpActions;
        pAct->mpNext = (STORDEV_ACTION *)v;
        K2_CpuWriteBarrier();
    } while (v != K2ATOMIC_CompareExchange((UINT32 volatile *)&pStorDev->mpActions, (UINT32)pAct, v));

    // pStorDev may be GONE as soon as action is latched.  we don't care if the following fails.
    K2OS_Notify_Signal(tokNotify);
}

UINT32
StorMgr_Thread(
    void *apArg
)
{
    static K2_GUID128 const sBlockIoIfaceId = K2OS_IFACE_BLOCKIO_DEVICE_CLASSID;
    K2OS_MAILBOX_TOKEN  tokMailbox;
    K2OS_IFSUBS_TOKEN   tokSubs;
    K2OS_MSG            msg;
    K2OS_WaitResult     waitResult;
    K2LIST_LINK *       pListLink;
    STORDEV *           pStorDev;

    tokMailbox = K2OS_Mailbox_Create();
    if (NULL == tokMailbox)
    {
        Debug_Printf("***StorMgr thread coult not create a mailbox\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    tokSubs = K2OS_IfSubs_Create(tokMailbox, K2OS_IFACE_CLASSCODE_STORAGE_DEVICE, &sBlockIoIfaceId, 12, FALSE, NULL);
    if (NULL == tokSubs)
    {
        Debug_Printf("***StorMgr thread coult not subscribe to blockio device interface pop changes\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, tokMailbox, K2OS_TIMEOUT_INFINITE))
            break;
        K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);
        if (K2OS_Mailbox_Recv(tokMailbox, &msg, 0))
        {
            if (msg.mType == K2OS_SYSTEM_MSGTYPE_IFINST)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    // payload[0] = subscription context
                    // payload[1] = interface instance id
                    // payload[2] = process that published the interface
                    StorMgr_BlockIo_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                }
                else if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    StorMgr_BlockIo_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                }
                else
                {
                    Debug_Printf("*** SysProc StorMgr received unexpected IFINST message (%04X)\n", msg.mShort);
                }
            }
            else if (msg.mType == K2OS_SYSTEM_MSGTYPE_BLOCKIO)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_BLOCKIO_SHORT_MEDIA_CHANGED)
                {
                    K2OS_CritSec_Enter(&sgStorDevListSec);

                    pListLink = sgStorDevList.mpHead;
                    if (NULL != pListLink)
                    {
                        do {
                            pStorDev = K2_GET_CONTAINER(STORDEV, pListLink, ListLink);
                            pListLink = pListLink->mpNext;
                            if (((UINT32)pStorDev->mIfInstId) == msg.mPayload[0])
                            {
                                StorMgr_BlockIo_ListLocked_Notify(pStorDev, msg.mPayload[1]);
                                break;
                            }
                        } while (NULL != pListLink);
                    }

                    K2OS_CritSec_Leave(&sgStorDevListSec);
                }
                else
                {
                    Debug_Printf("*** SysProc StorMgr received unexpected RPC message (%04X)\n", msg.mShort);
                }
            }
            else
            {
                Debug_Printf("*** SysProc StorMgr received unexpected message type (%04X)\n", msg.mType);
            }
        }

    } while (1);

    return 0;
}

void
StorMgr_Init(
    void
)
{
    if (!K2OS_CritSec_Init(&sgStorDevListSec))
    {
        Debug_Printf("STORMGR: Could not create cs for storage device list\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    K2LIST_Init(&sgStorDevList);

    sgStorMgrTokThread = K2OS_Thread_Create("Storage Manager", StorMgr_Thread, NULL, NULL, &sgStorMgrThreadId);
    if (NULL == sgStorMgrTokThread)
    {
        Debug_Printf("***StorMgr thread coult not be started\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }
}

