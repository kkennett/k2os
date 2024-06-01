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

#include "fatfs.h"

static K2OS_RPC_CLASS sgRpcClass = NULL;
static K2OS_RPC_OBJ_CLASSDEF const sgClassDef =
{
    FATFS_OBJ_CLASS_GUID,
    FATFS_RpcObj_Create,
    FATFS_RpcObj_OnAttach,
    FATFS_RpcObj_OnDetach,
    FATFS_RpcObj_Call,
    FATFS_RpcObj_Delete
};

UINT32 
FatFs_Thread(
    FATFS *apFs
)
{
    K2OS_WaitResult     waitResult;
    K2OS_MSG            msg;
    K2OS_SIGNAL_TOKEN   tokStart;

    tokStart = apFs->mTokStart;
    apFs->mTokStart = NULL;

    do {
        apFs->mTokMailbox = K2OS_Mailbox_Create();
        if (NULL == apFs->mTokMailbox)
        {
            apFs->mLastStatus = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(apFs->mLastStatus));
            K2OSKERN_Debug("FATFS(%d): Failed to create mailbox (%08X)\n", apFs->mVolInstId, apFs->mLastStatus);
            K2OS_Gate_Open(tokStart);
            break;
        }

        do {
            apFs->mVol = K2OS_Vol_Attach(
                apFs->mVolInstId,
                (apFs->VolInfo.mAttributes & K2OS_STORAGE_VOLUME_ATTRIB_READ_ONLY) ? K2OS_ACCESS_R : K2OS_ACCESS_RW,
                K2OS_ACCESS_R,
                apFs->mTokMailbox);

            if (NULL == apFs->mVol)
            {
                apFs->mLastStatus = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(apFs->mLastStatus));
                K2OSKERN_Debug("FATFS(%d): Failed to attach to volume (%08X)\n", apFs->mVolInstId, apFs->mLastStatus);
                K2OS_Gate_Open(tokStart);
                break;
            }

            do {
                K2OS_CritSec_Enter(&gFsSec);

                if (0 == gFsList.mNodeCount)
                {
                    //
                    // first one
                    //
                    K2_ASSERT(NULL == sgRpcClass);
                    sgRpcClass = K2OS_RpcServer_Register(&sgClassDef, 0);
                    if (NULL == sgRpcClass)
                    {
                        K2OS_CritSec_Leave(&gFsSec);
                        apFs->mLastStatus = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(apFs->mLastStatus));
                        K2OSKERN_Debug("FATFS(%d): Failed to register RPC class (%08X)\n", apFs->mVolInstId, apFs->mLastStatus);
                        K2OS_Gate_Open(tokStart);
                        break;
                    }
                }

                apFs->mhRpcObj = K2OS_Rpc_CreateObj(0, &sgClassDef.ClassId, (UINT32)apFs);
                if (NULL == apFs->mhRpcObj)
                {
                    apFs->mLastStatus = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(apFs->mLastStatus));
                    if (0 == gFsList.mNodeCount)
                    {
                        K2OS_RpcServer_Deregister(sgRpcClass);
                        sgRpcClass = NULL;
                    }
                    K2OS_CritSec_Leave(&gFsSec);
                    K2OSKERN_Debug("FATFS(%d): Failed to register RPC class (%08X)\n", apFs->mVolInstId, apFs->mLastStatus);
                    K2OS_Gate_Open(tokStart);
                    break;
                }

                K2_ASSERT(NULL != apFs->mRpcObj);
                K2LIST_AddAtTail(&gFsList, &apFs->ListLink);

                K2OS_CritSec_Leave(&gFsSec);

                apFs->mLastStatus = K2STAT_NO_ERROR;
                K2OS_Gate_Open(tokStart);
                K2OSKERN_Debug("FATFS(%d): Runs\n", apFs->mVolInstId);
                do {
                    if (!K2OS_Thread_WaitOne(&waitResult, apFs->mTokMailbox, K2OS_TIMEOUT_INFINITE))
                    {
                        K2OSKERN_Debug("FATFS(%d): Notify thread wait failed\n", apFs->mVolInstId);
                        break;
                    }
                    else if (waitResult == K2OS_Wait_Signalled_0)
                    {
                        if (K2OS_Mailbox_Recv(apFs->mTokMailbox, &msg, 0))
                        {
                            K2OSKERN_Debug("FATFS(%d): Recv Msg %d/%d\n", apFs->mVolInstId, msg.mType, msg.mShort);
                            if (msg.mType == K2OS_SYSTEM_MSGTYPE_DDK)
                            {
                                if (msg.mShort == K2OS_SYSMSG_DDK_SHORT_FSSTOP)
                                {
                                    if ((msg.mPayload[0] == apFs->mVolInstId) &&
                                        (msg.mPayload[1] == (UINT32)apFs) &&
                                        (msg.mPayload[2] == (UINT32)apFs->mTokThread))
                                    {
                                        K2OSKERN_Debug("FATFS(%d): Stopping\n", apFs->mVolInstId);
                                        break;
                                    }

                                    K2_RaiseException(K2STAT_EX_UNKNOWN);
                                }

                                K2OSKERN_Debug("FATFS(%d): Recevied unexpected DDK message short %d\n", apFs->mVolInstId, msg.mShort);
                            }
                            else
                            {
                                K2OSKERN_Debug("FATFS(%d): Recevied unexpected message type %d\n", apFs->mVolInstId, msg.mType);
                            }
                        }
                    }
                    else
                    {
                        K2_RaiseException(K2STAT_EX_UNKNOWN);
                    }
                } while (1);

                K2OSKERN_Debug("FATFS(%d): Stopped (%08X)\n", apFs->mVolInstId, apFs->mLastStatus);

            } while (0);

            K2OS_Vol_Detach(apFs->mVol);
            apFs->mVol = NULL;

        } while (0);

        K2OS_Token_Destroy(apFs->mTokMailbox);
        apFs->mTokMailbox = NULL;

    } while (0);

    return 0;
}

K2STAT
StartFs(
    UINT32                      aVolInstId, 
    K2OS_STORAGE_VOLUME const * apVolInfo, 
    UINT8 const *               apSector0
)
{
    K2STAT              stat;
    K2FAT_PART          fatPart;
    FATFS *             pFs;
    K2OS_SIGNAL_TOKEN   tokGate;
    K2OS_WaitResult     waitResult;
    BOOL                ok;

    stat = K2FAT_DetermineFromBootSector((FAT_GENERIC_BOOTSECTOR const *)apSector0, &fatPart);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("FATFS(%d): Failed to determine fat type from boot sector (%08X)\n", aVolInstId, stat);
        return stat;
    }

    tokGate = K2OS_Gate_Create(FALSE);
    if (NULL == tokGate)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OSKERN_Debug("FATFS(%d): Failed to create startup gate (%08X)\n", aVolInstId, stat);
        return stat;
    }

    pFs = K2OS_Heap_Alloc(sizeof(FATFS));
    if (NULL == pFs)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Token_Destroy(tokGate);
        K2OSKERN_Debug("FATFS(%d): Failed to alloc memory for fs (%08X)\n", aVolInstId, stat);
        return stat;
    }

    K2MEM_Zero(pFs, sizeof(FATFS));
    if (!K2OS_CritSec_Init(&pFs->Sec))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Token_Destroy(tokGate);
        K2OSKERN_Debug("FATFS(%d): Failed to init critsec for fs (%08X)\n", aVolInstId, stat);
        K2OS_Heap_Free(pFs);
        return stat;
    }

    pFs->mVolInstId = aVolInstId;
    K2MEM_Copy(&pFs->VolInfo, apVolInfo, sizeof(K2OS_STORAGE_VOLUME));
    K2MEM_Copy(&pFs->FatPart, &fatPart, sizeof(K2FAT_PART));
    pFs->mTokStart = tokGate;

    pFs->mTokThread = K2OS_Thread_Create("FATFS", (K2OS_pf_THREAD_ENTRY)FatFs_Thread, pFs, NULL, &pFs->mThreadId);
    if (NULL == pFs->mTokThread)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Token_Destroy(tokGate);
        K2OS_CritSec_Done(&pFs->Sec);
        K2OS_Heap_Free(pFs);
        K2OSKERN_Debug("FATFS(%d): Failed to start fs thread (%08X)\n", aVolInstId, stat);
        return stat;
    }

    //
    // thread owns all resources now except for 'tokGate', pFs->Sec, and pFs->mTokThread
    //

    ok = K2OS_Thread_WaitOne(&waitResult, tokGate, K2OS_TIMEOUT_INFINITE);
    K2_ASSERT(ok);

    K2_ASSERT(NULL == pFs->mTokStart);

    stat = pFs->mLastStatus;
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("FATFS(%d): Thread failed during startup (%08X)\n", aVolInstId, stat);
        K2OS_Thread_WaitOne(&waitResult, pFs->mTokThread, K2OS_TIMEOUT_INFINITE);
        K2OS_Token_Destroy(pFs->mTokThread);
        pFs->mTokThread = NULL;
        K2OS_CritSec_Done(&pFs->Sec);
        K2OS_Heap_Free(pFs);
    }

    K2OS_Token_Destroy(tokGate);

    return stat;
}

K2STAT
StopFs(
    UINT32  aVolIfInstId
)
{
    K2LIST_LINK *   pListLink;
    FATFS *         pFs;
    K2OS_WaitResult waitResult;
    BOOL            ok;
    K2OS_MSG        msg;

    K2OS_CritSec_Enter(&gFsSec);
    
    pListLink = gFsList.mpHead;
    K2_ASSERT(NULL != pListLink);
    do {
        pFs = K2_GET_CONTAINER(FATFS, pListLink, ListLink);
        if (pFs->mVolInstId == aVolIfInstId)
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    K2_ASSERT(NULL != pListLink);

    K2LIST_Remove(&gFsList, &pFs->ListLink);
    if (0 == gFsList.mNodeCount)
    {
        K2OS_RpcServer_Deregister(sgRpcClass);
        sgRpcClass = NULL;
    }

    K2OS_CritSec_Leave(&gFsSec);

    //
    // send fs message to stop itself
    // 
    msg.mType = K2OS_SYSTEM_MSGTYPE_DDK;
    msg.mShort = K2OS_SYSMSG_DDK_SHORT_FSSTOP;
    msg.mPayload[0] = aVolIfInstId;
    msg.mPayload[1] = (UINT32)pFs;
    msg.mPayload[2] = (UINT32)pFs->mTokThread;
    ok = K2OS_Mailbox_Send(pFs->mTokMailbox, &msg);
    K2_ASSERT(ok);
    
    // 
    // wait for the thread to exit
    //
    ok = K2OS_Thread_WaitOne(&waitResult, pFs->mTokThread, K2OS_TIMEOUT_INFINITE);
    K2_ASSERT(ok);

    K2OS_Token_Destroy(pFs->mTokThread);

    K2OS_CritSec_Done(&pFs->Sec);

    K2OS_Heap_Free(pFs);

    return K2STAT_NO_ERROR;
}

