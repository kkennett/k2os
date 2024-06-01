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

#include "k2osexec.h"

static K2OS_CRITSEC     sgNetIoListSec;
static K2LIST_ANCHOR    sgNetIoList;

BOOL
NetIo_FindPhysBuf(
    NETIO * apNetIo,
    UINT32  aType,
    UINT32  aBufAddr,
    UINT32 *apRetIx
)
{
    UINT32          l;
    UINT32          r;
    UINT32          m;
    UINT32          c;
    UINT32 const *  pBufAddr;

    //
    // find the physical buffer address in the (sorted) buffers array
    //
    pBufAddr = apNetIo->Register.mpBufsPhysAddrs;

    // 
    // 0 full range
    // 1 recv
    // 2 xmit
    //
    if (1 != aType)
    {
        r = apNetIo->mAllBufsCount;
    }
    else
    {
        r = apNetIo->Register.BufCounts.mRecv;
    }

    if (2 != aType)
    {
        l = 0;
    }
    else
    {
        l = apNetIo->Register.BufCounts.mRecv;
    }

    K2_ASSERT(l < r);

    do {
        m = (l + r) / 2;
        c = pBufAddr[m];
        if (c == aBufAddr)
        {
            *apRetIx = m;
            return TRUE;
        }
        if (c < aBufAddr)
        {
            l = m + 1;
        }
        else
        {
            r = m;
        }
    } while (l != r);

    return FALSE;
}

K2STAT 
NetIo_Config(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    UINT8 const *   apConfigIn
)
{
    K2STAT  stat;
    UINT32  pageCount;

    K2OS_CritSec_Enter(&apNetIo->Sec);

    if (NULL != apNetIo->mpControlUser)
    {
        stat = K2STAT_ERROR_ALREADY_OPEN;
    }
    else
    {
        K2MEM_Copy(&apNetIo->Control_ConfigIn, apConfigIn, sizeof(K2OS_NETIO_CONFIG_IN));

        if (NULL == apNetIo->Control_ConfigIn.mTokMailbox)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            if (apNetUser->mpProc->mProcessId == 0)
            {
                K2OS_Token_Clone(apNetIo->Control_ConfigIn.mTokMailbox, &apNetIo->Control_ConfigIn.mTokMailbox);
            }
            else
            {
                apNetIo->Control_ConfigIn.mTokMailbox = gKernDdk.UserToken_Clone(
                    apNetUser->mpProc->mProcessId, 
                    apNetIo->Control_ConfigIn.mTokMailbox
                );
            }

            if (NULL == apNetIo->Control_ConfigIn.mTokMailbox)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
            }
            else
            {
                K2_ASSERT(0 == apNetIo->BufRecvInUseList.mNodeCount);
                K2_ASSERT(apNetIo->Register.BufCounts.mXmit == apNetIo->BufXmitAvailList.mNodeCount);
                apNetIo->mpControlUser = apNetUser;
                stat = K2STAT_NO_ERROR;
            }
        }
    }

    K2OS_CritSec_Leave(&apNetIo->Sec);

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    pageCount = K2OS_PageArray_GetLength(apNetIo->mTokFramesPageArray);
    K2_ASSERT(0 != pageCount);

    //
    // map physical buffer space to this user's process
    //
    if (apNetUser->mpProc->mProcessId == 0)
    {
        //
        // controlling user is the kernel
        //
        do {
            apNetIo->mControl_BufsVirtBaseAddr = K2OS_Virt_Reserve(pageCount);
            if (0 == apNetIo->mControl_BufsVirtBaseAddr)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
            }

            apNetIo->mControl_TokBufsVirtMap = K2OS_VirtMap_Create(
                apNetIo->mTokFramesPageArray, 
                0, 
                pageCount, 
                apNetIo->mControl_BufsVirtBaseAddr,
                K2OS_MapType_Data_ReadWrite
            );
            if (NULL == apNetIo->mControl_TokBufsVirtMap)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OS_Virt_Release(apNetIo->mControl_BufsVirtBaseAddr);
                apNetIo->mControl_BufsVirtBaseAddr = 0;
            }
        } while (0);
    }
    else
    {
        //
        // controlling user is a process
        //
        stat = gKernDdk.UserMap(
            apNetUser->mpProc->mProcessId,
            apNetIo->mTokFramesPageArray,
            pageCount,
            &apNetIo->mControl_BufsVirtBaseAddr,
            &apNetIo->mControl_TokBufsVirtMap
        );
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_CritSec_Enter(&apNetIo->Sec);
        apNetIo->mpControlUser = NULL;
        K2OS_CritSec_Leave(&apNetIo->Sec);
        K2OS_Token_Destroy(apNetIo->Control_ConfigIn.mTokMailbox);
    }
    else
    {
        apNetIo->mControl_PageCount = pageCount;
    }

    return stat;
}

K2STAT
NetIo_GetDesc(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    UINT8 *         apRetDesc
)
{
    K2MEM_Copy(apRetDesc, &apNetIo->Register.Desc, sizeof(K2_NET_ADAPTER_DESC));
    return K2STAT_NO_ERROR;
}

K2STAT 
NetIo_GetState(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    UINT8 *         apRetState
)
{
    K2STAT                      stat;
    K2OS_NETIO_ADAPTER_STATE    retState;
    BOOL                        result;

    K2MEM_Zero(&retState, sizeof(retState));

    K2OS_CritSec_Enter(&apNetIo->Sec);

    result = apNetIo->Register.GetState(
        apNetIo->mpDriverContext,
        &retState.mIsPhysConnected,
        &retState.mIsUp
    );

    K2OS_CritSec_Leave(&apNetIo->Sec);

    if (result)
    {
        K2MEM_Copy(apRetState, &retState, sizeof(K2OS_NETIO_ADAPTER_STATE));
        stat = K2STAT_NO_ERROR;
    }
    else
    {
        stat = K2STAT_ERROR_HARDWARE;
    }

    return stat;
}

K2STAT
NetIo_GetBufStats(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    UINT8 *         apRetTotals,
    UINT8 *         apRetAvails
)
{
    K2OS_NETIO_BUFCOUNTS avail;

    K2OS_CritSec_Enter(&apNetIo->Sec);

    K2MEM_Copy(&apRetTotals, &apNetIo->Register.BufCounts, sizeof(K2OS_NETIO_BUFCOUNTS));
    avail.mRecv = apNetIo->Register.BufCounts.mRecv - apNetIo->BufRecvInUseList.mNodeCount;
    avail.mXmit = apNetIo->BufXmitAvailList.mNodeCount;

    K2OS_CritSec_Leave(&apNetIo->Sec);

    K2MEM_Copy(apRetAvails, &avail, sizeof(avail));

    return K2STAT_NO_ERROR;
}

K2STAT
NetIo_AcqBuffer(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    UINT8 *         apRetAcqBuffer
)
{
    K2OS_NETIO_ACQBUFFER_OUT    retAcqBuffer;
    K2STAT                      stat;
    NETIO_BUFTRACK *            pTrack;
    K2LIST_LINK *               pHeadBufListLink;

    K2MEM_Zero(&retAcqBuffer, sizeof(retAcqBuffer));

    K2OS_CritSec_Enter(&apNetIo->Sec);

    if (apNetUser != apNetIo->mpControlUser)
    {
        stat = K2STAT_ERROR_NOT_OWNED;
    }
    else
    {
        pHeadBufListLink = apNetIo->BufXmitAvailList.mpHead;
        if (NULL == pHeadBufListLink)
        {
            stat = K2STAT_ERROR_EMPTY;
        }
        else
        {
            pTrack = K2_GET_CONTAINER(NETIO_BUFTRACK, pHeadBufListLink, ListLink);
            K2LIST_Remove(&apNetIo->BufXmitAvailList, &pTrack->ListLink);
            pTrack->mUserOwned = TRUE;

            retAcqBuffer.mMTU = apNetIo->Register.Desc.mPhysicalMTU;
            retAcqBuffer.mBufVirtAddr =
                (apNetIo->Register.mpBufsPhysAddrs[pTrack->mIx] - apNetIo->Register.mBufsPhysBaseAddr) +
                apNetIo->mControl_BufsVirtBaseAddr;

            stat = K2STAT_NO_ERROR;
        }
    }

    K2OS_CritSec_Leave(&apNetIo->Sec);

    if (!K2STAT_IS_ERROR(stat))
    {
        K2MEM_Copy(apRetAcqBuffer, &retAcqBuffer, sizeof(retAcqBuffer));
    }

    return stat;
}

K2STAT
NetIo_Send(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    UINT8 const *   apSendIn
)
{
    K2OS_NETIO_SEND_IN  sendIn;
    K2STAT              stat;
    NETIO_BUFTRACK *    pBufTrack;
    UINT32              bufIx;

    K2OS_CritSec_Enter(&apNetIo->Sec);

    if (apNetUser != apNetIo->mpControlUser)
    {
        stat = K2STAT_ERROR_NOT_ALLOWED;
    }
    else
    {
        K2MEM_Copy(&sendIn, apSendIn, sizeof(sendIn));

        if (sendIn.mBufVirtAddr < apNetIo->mControl_BufsVirtBaseAddr)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            sendIn.mBufVirtAddr -= apNetIo->mControl_BufsVirtBaseAddr;
            if (sendIn.mBufVirtAddr >= (apNetIo->mControl_PageCount * K2_VA_MEMPAGE_BYTES))
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
            }
            else
            {
                sendIn.mBufVirtAddr += apNetIo->Register.mBufsPhysBaseAddr;
                if (!NetIo_FindPhysBuf(apNetIo, 2, sendIn.mBufVirtAddr, &bufIx))
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                }
                else
                {
                    pBufTrack = &apNetIo->mpBufTrack[bufIx];
                    K2_ASSERT(pBufTrack->mIx == bufIx);
                    if (!pBufTrack->mUserOwned)
                    {
                        stat = K2STAT_ERROR_NOT_OWNED;
                    }
                    else
                    {
                        if (!apNetIo->Register.Xmit(apNetIo->mpDriverContext, bufIx, sendIn.mSendBytes))
                        {
                            stat = K2STAT_ERROR_HARDWARE;
                        }
                        else
                        {
                            stat = K2STAT_NO_ERROR;
                        }
                    }
                }
            }
        }
    }

    K2OS_CritSec_Leave(&apNetIo->Sec);

    return stat;
}

K2STAT
NetIo_RelBuffer(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    UINT32          aBufAddr
)
{
    UINT32          bufIx;
    NETIO_BUFTRACK *pBufTrack;
    K2STAT          stat;
    BOOL            isRecv;

    isRecv = FALSE;
    bufIx = 0xFFFFFFFF;

    K2OS_CritSec_Enter(&apNetIo->Sec);

    if (apNetUser != apNetIo->mpControlUser)
    {
        stat = K2STAT_ERROR_NOT_ALLOWED;
    }
    else if ((aBufAddr < apNetIo->mControl_BufsVirtBaseAddr) ||
             ((aBufAddr - apNetIo->mControl_BufsVirtBaseAddr) > (apNetIo->mControl_PageCount * K2_VA_MEMPAGE_BYTES)))
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        //
        // translate to physical address
        //
        aBufAddr = (aBufAddr - apNetIo->mControl_BufsVirtBaseAddr) + apNetIo->Register.mBufsPhysBaseAddr;

        if (!NetIo_FindPhysBuf(apNetIo, 0, aBufAddr, &bufIx))
        {
            stat = K2STAT_ERROR_NOT_FOUND;
        }
        else
        {
            pBufTrack = &apNetIo->mpBufTrack[bufIx];
            K2_ASSERT(pBufTrack->mIx == bufIx);
            if (!pBufTrack->mUserOwned)
            {
                K2_ASSERT(0);
                stat = K2STAT_ERROR_NOT_OWNED;
            }
            else
            {
                pBufTrack->mUserOwned = FALSE;
                if (bufIx < apNetIo->Register.BufCounts.mRecv)
                {
                    //
                    // this is a receive buffer
                    //
                    isRecv = TRUE;
                    K2LIST_Remove(&apNetIo->BufRecvInUseList, &pBufTrack->ListLink);
                }
                else
                {
                    // 
                    // this is a transmit buffer
                    //
                    K2LIST_AddAtTail(&apNetIo->BufXmitAvailList, &pBufTrack->ListLink);
                }
            }
        }
    }

    K2OS_CritSec_Leave(&apNetIo->Sec);

    if (isRecv)
    {
        apNetIo->Register.DoneRecv(apNetIo->mpDriverContext, bufIx);
    }

    return stat;
}

K2STAT 
NetIo_SetEnable(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    BOOL            aSetEnable
)
{
    BOOL    oldEnable;
    K2STAT  stat;

    K2OS_CritSec_Enter(&apNetIo->Sec);

    if (apNetUser != apNetIo->mpControlUser)
    {
        stat = K2STAT_ERROR_NOT_ALLOWED;
    }
    else
    {
        oldEnable = apNetIo->mIsEnabled;
        apNetIo->mIsEnabled = aSetEnable;
        if (!apNetIo->Register.SetEnable(apNetIo->mpDriverContext, aSetEnable))
        {
            apNetIo->mIsEnabled = oldEnable;
            stat = K2STAT_ERROR_HARDWARE;
        }
        else
        {
            stat = K2STAT_NO_ERROR;
        }
    }

    K2OS_CritSec_Leave(&apNetIo->Sec);

    return stat;
}

K2STAT 
NetIo_GetEnable(
    NETIO *         apNetIo,
    NETIO_USER *    apNetUser,
    BOOL *          apRetEnabled
)
{
    *apRetEnabled = apNetIo->mIsEnabled;

    return K2STAT_NO_ERROR;
}

//
// ----------------------------------------------------------------------------------
//

K2STAT
K2OSEXEC_NetIoRpc_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    NETIO * pNetIo;

    if (0 != apCreate->mCreatorProcessId)
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pNetIo = (NETIO *)apCreate->mCreatorContext;
    pNetIo->mRpcObj = aObject;
    *apRetContext = (UINT32)pNetIo;

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_NetIoRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    NETIO *         pNetIo;
    NETIO_USER *    pUser;
    NETIO_PROC *    pProc;
    K2LIST_LINK *   pListLink;
    NETIO_PROC *    pOtherProc;

    pNetIo = (NETIO *)aObjContext;
    K2_ASSERT(pNetIo->mRpcObj == aObject);

    pUser = (NETIO_USER *)K2OS_Heap_Alloc(sizeof(NETIO_USER));
    if (NULL == pUser)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pProc = (NETIO_PROC *)K2OS_Heap_Alloc(sizeof(NETIO_PROC));
    if (NULL == pProc)
    {
        K2OS_Heap_Free(pUser);
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pProc, sizeof(NETIO_PROC));
    pProc->mProcessId = aProcessId;
    K2LIST_Init(&pProc->UserList);

    K2MEM_Zero(pUser, sizeof(NETIO_USER));
    *apRetUseContext = (UINT32)pUser;

    pOtherProc = NULL;

    K2OS_CritSec_Enter(&pNetIo->Sec);

    pListLink = pNetIo->ProcList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOtherProc = K2_GET_CONTAINER(NETIO_PROC, pListLink, InstanceProcListLink);
            if (pOtherProc->mProcessId == aProcessId)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    if (NULL == pListLink)
    {
        //
        // proc not found
        //
        pUser->mpProc = pProc;
        K2LIST_AddAtTail(&pNetIo->ProcList, &pProc->InstanceProcListLink);
        pProc = NULL;
    }
    else
    {
        pUser->mpProc = pOtherProc;
    }

    K2LIST_AddAtTail(&pUser->mpProc->UserList, &pUser->ProcUserListLink);

    K2OS_CritSec_Leave(&pNetIo->Sec);

    if (NULL != pProc)
    {
        K2OS_Heap_Free(pProc);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_NetIoRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    NETIO *             pNetIo;
    NETIO_USER *        pUser;
    NETIO_PROC *        pProc;
    BOOL                ok;
    UINT32              ix;
    NETIO_BUFTRACK *    pBufTrack;

    pNetIo = (NETIO *)aObjContext;
    K2_ASSERT(pNetIo->mRpcObj == aObject);

    pUser = (NETIO_USER *)aUseContext;
    pProc = pUser->mpProc;

    K2OS_CritSec_Enter(&pNetIo->Sec);

    if (pUser == pNetIo->mpControlUser)
    {
        ok = K2OS_Token_Destroy(pNetIo->Control_ConfigIn.mTokMailbox);
        K2_ASSERT(ok);

        //
        // yank back all outstanding transmit buffers
        //
        pBufTrack = pNetIo->mpBufTrack;
        for (ix = pNetIo->Register.BufCounts.mRecv; ix < pNetIo->mAllBufsCount; ix++)
        {
            if (pBufTrack[ix].mUserOwned)
            {
                K2LIST_AddAtTail(&pNetIo->BufXmitAvailList, &pBufTrack[ix].ListLink);
                pBufTrack[ix].mUserOwned = FALSE;
            }
        }

        //
        // yank back any unreleased receive buffers
        // and push them to the driver
        //
        for (ix = 0; ix < pNetIo->Register.BufCounts.mRecv; ix++)
        {
            if (pBufTrack[ix].mUserOwned)
            {
                K2LIST_Remove(&pNetIo->BufRecvInUseList, &pBufTrack[ix].ListLink);
                pBufTrack[ix].mUserOwned = FALSE;
                pNetIo->Register.DoneRecv(pNetIo->mpDriverContext, ix);
            }
        }

        pNetIo->mpControlUser = NULL;
    }

    K2LIST_Remove(&pProc->UserList, &pUser->ProcUserListLink);

    if (0 == pProc->UserList.mNodeCount)
    {
        //
        // last user in this process is detaching
        //
        K2LIST_Remove(&pNetIo->ProcList, &pProc->InstanceProcListLink);
    }
    else
    {
        pProc = NULL;
    }

    K2OS_CritSec_Leave(&pNetIo->Sec);

    if (NULL != pProc)
    {
        K2OS_Heap_Free(pProc);
    }

    K2OS_Heap_Free(pUser);

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSEXEC_NetIoRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    NETIO *         pNetIo;
    NETIO_USER *    pUser;
    UINT32          ui32;
    BOOL            bval;
    K2STAT          stat;

    pNetIo = (NETIO *)apCall->mObjContext;
    K2_ASSERT(pNetIo->mRpcObj == apCall->mObj);

    pUser = (NETIO_USER *)apCall->mUseContext;
    K2_ASSERT(pUser != NULL);

//    K2OSKERN_Debug("NetIo_Call(%08X, %d)\n", pNetIo, apCall->Args.mMethodId);
    
    switch (apCall->Args.mMethodId)
    {
    case K2OS_NETIO_METHOD_CONFIG:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            (sizeof(K2OS_NETIO_CONFIG_IN) != apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = NetIo_Config(pNetIo, pUser, apCall->Args.mpInBuf);
        }
        break;

    case K2OS_NETIO_METHOD_GET_DESC:
        if ((0 != apCall->Args.mInBufByteCount) ||
            (sizeof(K2_NET_ADAPTER_DESC) != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = NetIo_GetDesc(pNetIo, pUser, apCall->Args.mpOutBuf);
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2_NET_ADAPTER_DESC);
            }
        }
        break;

    case K2OS_NETIO_METHOD_GET_STATE:
        if ((0 != apCall->Args.mInBufByteCount) ||
            (sizeof(K2OS_NETIO_ADAPTER_STATE) != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = NetIo_GetState(pNetIo, pUser, apCall->Args.mpOutBuf);
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_NETIO_ADAPTER_STATE);
            }
        }
        break;

    case K2OS_NETIO_METHOD_BUFSTATS:
        if ((0 != apCall->Args.mInBufByteCount) ||
            ((sizeof(K2OS_NETIO_BUFCOUNTS) * 2) != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = NetIo_GetBufStats(pNetIo, pUser, 
                apCall->Args.mpOutBuf, 
                apCall->Args.mpOutBuf + sizeof(K2OS_NETIO_BUFCOUNTS));
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = (2 * sizeof(K2OS_NETIO_ADAPTER_STATE));
            }
        }
        break;

    case K2OS_NETIO_METHOD_ACQBUFFER:
        if ((0 != apCall->Args.mInBufByteCount) ||
            (sizeof(K2OS_NETIO_ACQBUFFER_OUT) != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = NetIo_AcqBuffer(pNetIo, pUser, apCall->Args.mpOutBuf);
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_NETIO_ACQBUFFER_OUT);
            }
        }
        break;

    case K2OS_NETIO_METHOD_SEND:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            (sizeof(K2OS_NETIO_SEND_IN) != apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = NetIo_Send(pNetIo, pUser, apCall->Args.mpInBuf);
        }
        break;

    case K2OS_NETIO_METHOD_RELBUFFER:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            (sizeof(UINT32) != apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            K2MEM_Copy(&ui32, apCall->Args.mpInBuf, sizeof(UINT32));
            stat = NetIo_RelBuffer(pNetIo, pUser, ui32);
        }
        break;

    case K2OS_NETIO_METHOD_SETENABLE:
        if ((0 != apCall->Args.mOutBufByteCount) ||
            (sizeof(BOOL) != apCall->Args.mInBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            K2MEM_Copy(&bval, apCall->Args.mpInBuf, sizeof(BOOL));
            stat = NetIo_SetEnable(pNetIo, pUser, bval);
        }
        break;

    case K2OS_NETIO_METHOD_GETENABLE:
        if ((0 != apCall->Args.mInBufByteCount) ||
            (sizeof(BOOL) != apCall->Args.mOutBufByteCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = NetIo_GetEnable(pNetIo, pUser, &bval);
            if (!K2STAT_IS_ERROR(stat))
            {
                K2MEM_Copy(apCall->Args.mpOutBuf, &bval, sizeof(BOOL));
                *apRetUsedOutBytes = sizeof(BOOL);
            }
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    }

    return stat;
}

K2STAT
K2OSEXEC_NetIoRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    NETIO *   pNetIo;

    pNetIo = (NETIO *)aObjContext;

    K2_ASSERT(aObject == pNetIo->mRpcObj);

    K2_ASSERT(NULL == pNetIo->mRpcObjHandle);
    K2_ASSERT(NULL == pNetIo->mRpcIfInst);
    K2_ASSERT(0 == pNetIo->mRpcIfInstId);
    K2_ASSERT(0 == pNetIo->ProcList.mNodeCount);

    pNetIo->mRpcObj = NULL;

    return K2STAT_NO_ERROR;
}

//
// ----------------------------------------------------------------------------------
//

// {61B61508-14CA-4756-889A-69ACA9EA3F7C}
static K2OS_RPC_OBJ_CLASSDEF sgNetIoRpcClassDef =
{
    { 0x61b61508, 0x14ca, 0x4756, { 0x88, 0x9a, 0x69, 0xac, 0xa9, 0xea, 0x3f, 0x7c } },
    K2OSEXEC_NetIoRpc_Create,
    K2OSEXEC_NetIoRpc_OnAttach,
    K2OSEXEC_NetIoRpc_OnDetach,
    K2OSEXEC_NetIoRpc_Call,
    K2OSEXEC_NetIoRpc_Delete
};

void
NetIo_XmitDone(
    void *      apKey,
    K2OS_DEVCTX aDevCtx,
    void *      apDevice,
    UINT32      aBufIx
)
{
    NETIO *             pNetIo;
    NETIO_BUFTRACK *    pBufTrack;

    pNetIo = K2_GET_CONTAINER(NETIO, apKey, mfXmitDone);

    K2_ASSERT(aDevCtx == (K2OS_DEVCTX)pNetIo->mpDevNode);
    K2_ASSERT(apDevice == pNetIo->mpDriverContext);

    K2OS_CritSec_Enter(&pNetIo->Sec);

    pBufTrack = &pNetIo->mpBufTrack[aBufIx];
    K2_ASSERT(pBufTrack->mIx == aBufIx);
    K2_ASSERT(pBufTrack->mUserOwned);

    pBufTrack->mUserOwned = FALSE;
    K2LIST_AddAtTail(&pNetIo->BufXmitAvailList, &pBufTrack->ListLink);

    K2OS_CritSec_Leave(&pNetIo->Sec);
}

void 
NetIo_Recv(
    void *      apKey,
    K2OS_DEVCTX aDevCtx,
    void *      apDevice,
    UINT32      aPhysBufAddr,
    UINT32      aByteCount
)
{
    NETIO *             pNetIo;
    NETIO_BUFTRACK *    pTrack;
    NETIO_USER *        pUser;
    K2OS_NETIO_MSG      netMsg;
    UINT32              bufIx;
    K2OS_MAILBOX_TOKEN  tokMailbox;

    pNetIo = K2_GET_CONTAINER(NETIO, apKey, mfRecv);

    K2_ASSERT(aDevCtx == (K2OS_DEVCTX)pNetIo->mpDevNode);
    K2_ASSERT(apDevice == pNetIo->mpDriverContext);

    if (!NetIo_FindPhysBuf(pNetIo, 1, aPhysBufAddr, &bufIx))
    {
        // driver sent us an untracked buffer!
        K2_ASSERT(0);
    }

    pTrack = &pNetIo->mpBufTrack[bufIx];
    K2_ASSERT(pTrack->mIx == bufIx);
    K2_ASSERT(!pTrack->mUserOwned);

    pTrack->mUserOwned = TRUE;

    K2OS_CritSec_Enter(&pNetIo->Sec);

    K2LIST_AddAtTail(&pNetIo->BufRecvInUseList, &pTrack->ListLink);
    pUser = pNetIo->mpControlUser;
    if (NULL != pUser)
    {
        tokMailbox = pNetIo->Control_ConfigIn.mTokMailbox;
        netMsg.mpContext = pNetIo->Control_ConfigIn.mpContext;
        netMsg.mPayload[0] = pNetIo->mControl_BufsVirtBaseAddr;
    }

    K2OS_CritSec_Leave(&pNetIo->Sec);

    if (NULL != pUser)
    {
        netMsg.mType = K2OS_NETIO_MSGTYPE;
        netMsg.mShort = K2OS_NetIoMsgShort_Recv;
        netMsg.mPayload[0] += (aPhysBufAddr - pNetIo->Register.mBufsPhysBaseAddr);
        netMsg.mPayload[1] = aByteCount;
        if (!K2OS_Mailbox_Send(tokMailbox, (K2OS_MSG *)&netMsg))
        {
            pUser = NULL;
        }
    }

    if (NULL == pUser)
    {
        //
        // give buffer back to the driver
        //
        pNetIo->Register.DoneRecv(pNetIo->mpDriverContext, bufIx);
    }
}

void
NetIo_Notify(
    void *      apKey,
    K2OS_DEVCTX aDevCtx,
    void *      apDevice,
    UINT32      aNotifyCode
)
{
    NETIO * pNetIo;

    pNetIo = K2_GET_CONTAINER(NETIO, apKey, mfNotify);

    K2_ASSERT(aDevCtx == (K2OS_DEVCTX)pNetIo->mpDevNode);
    K2_ASSERT(apDevice == pNetIo->mpDriverContext);

    K2OSKERN_Debug("NetIo_Notify(%08X, %d)\n", pNetIo, aNotifyCode);

    K2OS_CritSec_Enter(&pNetIo->Sec);

    K2OS_RpcObj_SendNotify(pNetIo->mRpcObj, 0, aNotifyCode, pNetIo->mRpcIfInstId);

    K2OS_CritSec_Leave(&pNetIo->Sec);
}

UINT32
NetIo_AddRef(
    NETIO *apNetIo
)
{
    return (UINT32)K2ATOMIC_Inc(&apNetIo->mRefCount);
}

UINT32
NetIo_Release(
    NETIO * apNetIo
)
{
    UINT32  result;

    result = K2ATOMIC_Dec(&apNetIo->mRefCount);
    if (0 != result)
    {
        return result;
    }

    K2_ASSERT(0 != (K2RUNDOWN_TRIGGER & apNetIo->Rundown.mState));
    K2_ASSERT(NULL == apNetIo->mpDevNode);
    K2_ASSERT(NULL == apNetIo->mpDriverContext);
    K2_ASSERT(NULL == apNetIo->mRpcIfInst);
    K2_ASSERT(0 == apNetIo->mRpcIfInstId);
    K2_ASSERT(NULL == apNetIo->mRpcObjHandle);
    K2_ASSERT(0 == apNetIo->ProcList.mNodeCount);

    K2OS_Heap_Free(apNetIo->mpBufTrack);

    K2OS_Token_Destroy(apNetIo->mTokFramesPageArray);
    apNetIo->mTokFramesPageArray = NULL;

    K2OS_CritSec_Enter(&sgNetIoListSec);
    K2LIST_Remove(&sgNetIoList, &apNetIo->GlobalListLink);
    K2OS_CritSec_Leave(&sgNetIoListSec);

    K2RUNDOWN_Done(&apNetIo->Rundown);

    K2OS_CritSec_Done(&apNetIo->Sec);

    for (result = 0; result < 5; result++)
    {
        K2OS_Xdl_Release(apNetIo->mHoldXdl[result]);
    }

    K2MEM_Zero(apNetIo, sizeof(NETIO));

    K2OS_Heap_Free(apNetIo);

    return 0;
}

K2STAT
K2OSDDK_NetIoRegister(
    K2OS_DEVCTX                     aDevCtx,
    void *                          apDevice,
    K2OSDDK_NETIO_REGISTER const *  apRegister,
    K2OS_PAGEARRAY_TOKEN            aTokFramesPageArray,
    K2OSDDK_pf_NetIo_RecvKey **     apRetRecvKey,
    K2OSDDK_pf_NetIo_XmitDoneKey ** apRetXmitDoneKey,
    K2OSDDK_pf_NetIo_NotifyKey **   apRetNotifyKey
)
{
    static K2_GUID128 const sNetIoIfaceId = K2OS_IFACE_NETIO_DEVICE;

    K2OSDDK_NETIO_REGISTER  regis;
    K2OS_XDL                holdXdl[4];
    K2STAT                  stat;
    NETIO *                 pNewNetIo;
    NETIO *                 pOtherNetIo;
    DEVNODE *               pDevNode;
    K2LIST_LINK *           pListLink;
    UINT32                  ix;
    K2OS_RPC_OBJ_HANDLE     hRpcObj;

    if (NULL == apRegister)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2MEM_Copy(&regis, apRegister, sizeof(K2OSDDK_NETIO_REGISTER));

    if ((regis.Desc.mType == K2_NetAdapter_Invalid) ||
        (regis.Desc.mType >= K2_NetAdapterType_Count) ||
        (regis.Desc.mPhysicalMTU == 0) ||
        (regis.Desc.Addr.mLen == 0) ||
        (NULL == regis.Xmit))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    ix = 0;
    do {
        holdXdl[ix] = K2OS_Xdl_AddRefContaining((UINT32)regis.SetEnable);
        if (NULL == holdXdl[ix])
        {
            stat = K2OS_Thread_GetLastStatus();
            break;
        }
        ++ix;
        holdXdl[ix] = K2OS_Xdl_AddRefContaining((UINT32)regis.GetState);
        if (NULL == holdXdl[ix])
        {
            stat = K2OS_Thread_GetLastStatus();
            break;
        }
        ++ix;
        holdXdl[ix] = K2OS_Xdl_AddRefContaining((UINT32)regis.DoneRecv);
        if (NULL == holdXdl[ix])
        {
            stat = K2OS_Thread_GetLastStatus();
            break;
        }
        ++ix;
        holdXdl[ix] = K2OS_Xdl_AddRefContaining((UINT32)regis.Xmit);
        if (NULL == holdXdl[ix])
        {
            stat = K2OS_Thread_GetLastStatus();
            break;
        }
        ++ix;

        stat = K2STAT_NO_ERROR;

    } while (0);

    if (ix < 4)
    {
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        if (ix > 0)
        {
            do {
                --ix;
                K2OS_Xdl_Release(holdXdl[ix]);
            } while (ix > 0);
        }
        return stat;
    }

    do {
        pNewNetIo = (NETIO *)K2OS_Heap_Alloc(sizeof(NETIO));
        if (NULL == pNewNetIo)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }
         
        do {
            K2MEM_Zero(pNewNetIo, sizeof(NETIO));

            if (!K2OS_CritSec_Init(&pNewNetIo->Sec))
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
            }

            do {
                if (!K2RUNDOWN_Init(&pNewNetIo->Rundown))
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    break;
                }

                do {
                    pNewNetIo->mAllBufsCount = regis.BufCounts.mRecv + regis.BufCounts.mXmit;
                    pNewNetIo->mpBufTrack = (NETIO_BUFTRACK *)K2OS_Heap_Alloc(pNewNetIo->mAllBufsCount * sizeof(NETIO_BUFTRACK));
                    if (NULL == pNewNetIo->mpBufTrack)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(stat));
                        pNewNetIo->mAllBufsCount = 0;
                        break;
                    }
                    K2MEM_Zero(pNewNetIo->mpBufTrack, sizeof(NETIO_BUFTRACK) * pNewNetIo->mAllBufsCount);
                    K2LIST_Init(&pNewNetIo->BufXmitAvailList);
                    K2LIST_Init(&pNewNetIo->BufRecvInUseList);
                    for (ix = 0; ix < regis.BufCounts.mRecv; ix++)
                    {
                        pNewNetIo->mpBufTrack[ix].mIx = ix;
                    }
                    for (ix = regis.BufCounts.mRecv; ix < pNewNetIo->mAllBufsCount; ix++)
                    {
                        pNewNetIo->mpBufTrack[ix].mIx = ix;
                        K2LIST_AddAtTail(&pNewNetIo->BufXmitAvailList, &pNewNetIo->mpBufTrack[ix].ListLink);
                    }

                    K2LIST_Init(&pNewNetIo->ProcList);

                    pNewNetIo->mRefCount = 1;
                    K2MEM_Copy(&pNewNetIo->Register, &regis, sizeof(K2OSDDK_NETIO_REGISTER));
                    pNewNetIo->mpDevNode = (DEVNODE *)aDevCtx;
                    pNewNetIo->mpDriverContext = apDevice;

                    pNewNetIo->mfNotify = NetIo_Notify;
                    pNewNetIo->mfRecv = NetIo_Recv;
                    pNewNetIo->mfXmitDone = NetIo_XmitDone;

                    *apRetNotifyKey = &pNewNetIo->mfNotify;
                    *apRetRecvKey = &pNewNetIo->mfRecv;
                    *apRetXmitDoneKey = &pNewNetIo->mfXmitDone;

                    pNewNetIo->mTokFramesPageArray = aTokFramesPageArray;

                    pNewNetIo->mRpcObjHandle = K2OS_Rpc_CreateObj(0, &sgNetIoRpcClassDef.ClassId, (UINT32)pNewNetIo);
                    if (NULL == pNewNetIo->mRpcObjHandle)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(stat));

                        K2OS_Heap_Free(pNewNetIo->mpBufTrack);
                        pNewNetIo->mpBufTrack = NULL;
                    }
                    else
                    {
                        K2_ASSERT(NULL != pNewNetIo->mRpcObj);

                        for (ix = 0; ix < 4; ix++)
                        {
                            pNewNetIo->mHoldXdl[ix] = holdXdl[ix];
                            K2OS_Xdl_AddRef(holdXdl[ix]);
                        }
                    }

                } while (0);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2RUNDOWN_Done(&pNewNetIo->Rundown);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_CritSec_Done(&pNewNetIo->Sec);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Heap_Free(pNewNetIo);
        }

    } while (0);

    for (ix = 0; ix < 4; ix++)
    {
        K2OS_Xdl_Release(holdXdl[ix]);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    //
    // try to add to devnode now
    //
    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    pListLink = pDevNode->InSec.NetIo.List.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOtherNetIo = K2_GET_CONTAINER(NETIO, pListLink, DevNodeNetIoListLink);
            if (apDevice == pOtherNetIo->mpDriverContext)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    if (NULL != pListLink)
    {
        stat = K2STAT_ERROR_ALREADY_EXISTS;
    }
    else
    {
        K2LIST_AddAtTail(&pDevNode->InSec.NetIo.List, &pNewNetIo->DevNodeNetIoListLink);
        stat = K2STAT_NO_ERROR;
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    if (!K2STAT_IS_ERROR(stat))
    {
        //
        // new netio track
        //
        K2OS_CritSec_Enter(&sgNetIoListSec);
        K2LIST_AddAtTail(&sgNetIoList, &pNewNetIo->GlobalListLink);
        K2OS_CritSec_Leave(&sgNetIoListSec);

        //
        // calls can come in before we exit this function
        //
        pNewNetIo->mRpcIfInst = K2OS_RpcObj_AddIfInst(
            pNewNetIo->mRpcObj,
            K2OS_IFACE_CLASSCODE_NETWORK_DEVICE,
            &sNetIoIfaceId,
            &pNewNetIo->mRpcIfInstId,
            TRUE
        );

        if (NULL == pNewNetIo->mRpcIfInst)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));

            K2RUNDOWN_Wait(&pNewNetIo->Rundown);

            K2OS_CritSec_Enter(&pDevNode->Sec);
            K2LIST_Remove(&pDevNode->InSec.NetIo.List, &pNewNetIo->DevNodeNetIoListLink);
            K2OS_CritSec_Leave(&pDevNode->Sec);
            pNewNetIo->mpDevNode = NULL;
            pNewNetIo->mpDriverContext = NULL;
        }
    }
    
    if (K2STAT_IS_ERROR(stat))
    {
        hRpcObj = pNewNetIo->mRpcObjHandle;
        pNewNetIo->mRpcObjHandle = NULL;
        K2OS_Rpc_Release(hRpcObj);
        K2_ASSERT(NULL == pNewNetIo->mRpcObj);
        NetIo_Release(pNewNetIo);
    }

    return stat;
}

K2STAT
K2OSDDK_NetIoDeregister(
    K2OS_DEVCTX aDevCtx,
    void *      apDevice
)
{
    DEVNODE *           pDevNode;
    NETIO *             pNetIo;
    K2LIST_LINK *       pListLink;
    K2STAT              stat;
    BOOL                ok;
    K2OS_RPC_OBJ_HANDLE hRpcObj;

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);
    
    pListLink = pDevNode->InSec.NetIo.List.mpHead;
    if (NULL != pListLink)
    {
        do {
            pNetIo = K2_GET_CONTAINER(NETIO, pListLink, DevNodeNetIoListLink);
            if (pNetIo->mpDriverContext == apDevice)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
    if (NULL == pListLink)
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        //
        // waiting inside devnode sec!
        //
        K2RUNDOWN_Wait(&pNetIo->Rundown);

        K2LIST_Remove(&pDevNode->InSec.NetIo.List, &pNetIo->DevNodeNetIoListLink);

        //
        // rundown will prevent use of any of this
        //
        pNetIo->mpDevNode = NULL;
        pNetIo->mpDriverContext = NULL;

        stat = K2STAT_NO_ERROR;
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    if (K2STAT_IS_ERROR(stat))
        return stat;

    K2OS_CritSec_Enter(&pNetIo->Sec);

    ok = K2OS_RpcObj_RemoveIfInst(pNetIo->mRpcObj, pNetIo->mRpcIfInst);
    pNetIo->mRpcIfInst = NULL;
    pNetIo->mRpcIfInstId = 0;
    K2_ASSERT(ok);
    hRpcObj = pNetIo->mRpcObjHandle;
    pNetIo->mRpcObjHandle = NULL;
    K2MEM_Zero(&pNetIo->Register, sizeof(K2OSDDK_NETIO_REGISTER));

    K2OS_CritSec_Leave(&pNetIo->Sec);

    K2OS_Rpc_Release(hRpcObj);

    NetIo_Release(pNetIo);

    return K2STAT_NO_ERROR;
}

void
NetIo_Init(
    void
)
{
    if (!K2OS_CritSec_Init(&sgNetIoListSec))
    {
        K2OSKERN_Panic("Could not init netio list cs\n");
    }

    K2LIST_Init(&sgNetIoList);

    if (NULL == K2OS_RpcServer_Register(&sgNetIoRpcClassDef, 0))
    {
        K2OSKERN_Panic("*** could not register netio class\n");
    }
}