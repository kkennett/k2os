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

#include "ik2net.h"

static K2NET_pf_MemAlloc sgfMemAlloc;
static K2NET_pf_MemFree  sgfMemFree;
static K2NET_pf_AddTimer sgfAddTimer;
static K2NET_pf_DelTimer sgfDelTimer;

UINT32          gNextProtoInstanceId;
K2TREE_ANCHOR   gRegProtoTree;
K2TREE_ANCHOR   gProtoInstTree;

void *
K2NET_MemAlloc(
    UINT32 aByteCount
)
{
    return sgfMemAlloc(aByteCount);
}

void
K2NET_MemFree(
    void *aPtr
)
{
    sgfMemFree(aPtr);
}

void
K2NET_AddTimer(
    void *                  apContext,
    UINT32                  aPeriod,
    K2NET_pf_OnTimerExpired afTimerExpired
)
{
    sgfAddTimer(apContext, aPeriod, afTimerExpired);
}

void
K2NET_DelTimer(
    void *apContext
)
{
    sgfDelTimer(apContext);
}

K2NET_USAGE 
K2NET_AcquireProtocolInstance(
    UINT32              aInstanceId,
    K2NET_USAGE_CONTEXT aUsageContext,
    K2NET_pf_Doorbell   afDoorbell
)
{
    K2TREE_NODE *   pTreeNode;
    K2NET_PROTO *   pProto;
    K2NET_OPEN *    pOpen;

    pOpen = (K2NET_OPEN *)K2NET_MemAlloc(sizeof(K2NET_OPEN));
    if (NULL == pOpen)
    {
        return NULL;
    }

    pTreeNode = K2TREE_Find(&gProtoInstTree, aInstanceId);
    if (NULL == pTreeNode)
    {
        K2NET_MemFree(pOpen);
        return NULL;
    }

    K2MEM_Zero(pOpen, sizeof(K2NET_OPEN));
    pOpen->mRefs = 1;
    pOpen->mfDoorbell = afDoorbell;
    pOpen->mUsageContext = aUsageContext;

    pProto = K2_GET_CONTAINER(K2NET_PROTO, pTreeNode, TreeNode);

    pOpen->mOpenContext = pProto->Instance.OnOpen(pProto->mProtoContext, (K2NET_OPEN_INSTANCE)pOpen);

    if (NULL == pOpen->mOpenContext)
    {
        K2NET_MemFree(pOpen);
        return NULL;
    }

    pOpen->mpProto = pProto;
    K2ATOMIC_Inc(&pProto->mRefs);

    pOpen->TreeNode.mUserVal = gNextProtoInstanceId++;
    K2TREE_Insert(&pProto->TreeAnchor, pOpen->TreeNode.mUserVal, &pOpen->TreeNode);

    return (K2NET_USAGE)pOpen;
}

K2NET_USAGE_CONTEXT 
K2NET_Usage_GetContext(
    K2NET_USAGE aUsage
)
{
    return ((K2NET_OPEN *)aUsage)->mUsageContext;
}

void                
K2NET_Usage_AddRef(
    K2NET_USAGE aUsage
)
{
    K2ATOMIC_Inc(&((K2NET_OPEN *)aUsage)->mRefs);
}

void                
K2NET_Usage_Release(
    K2NET_USAGE aUsage
)
{
    K2NET_OPEN *    pOpen;
    K2NET_PROTO *   pProto;
    K2NET_MSGLINK * pMsgLink;

    pOpen = (K2NET_OPEN *)aUsage;
    if (0 == K2ATOMIC_Dec(&pOpen->mRefs))
    {
        pProto = pOpen->mpProto;
        pOpen->mpProto->Instance.OnClose(pOpen->mOpenContext);
        K2TREE_Remove(&pProto->TreeAnchor, &pOpen->TreeNode);
        pMsgLink = pOpen->mpFirstMsg;
        if (NULL != pMsgLink)
        {
            do {
                pOpen->mpFirstMsg = pMsgLink->mpNext;
                K2NET_MemFree(pMsgLink);
                pMsgLink = pOpen->mpFirstMsg;
            } while (NULL != pMsgLink);
            pOpen->mpLastMsg = NULL;
        }
        K2NET_Proto_Release(pProto);
    }

    K2NET_MemFree(pOpen);
}

BOOL                
K2NET_Usage_RecvMsg(
    K2NET_USAGE aUsage,
    K2NET_MSG * apRetMsg
)
{
    K2NET_OPEN *    pOpen;
    K2NET_MSGLINK * pMsgLink;

    pOpen = (K2NET_OPEN *)aUsage;
    pMsgLink = pOpen->mpFirstMsg;
    if (NULL == pMsgLink)
    {
        return FALSE;
    }
    pOpen->mpFirstMsg = pMsgLink->mpNext;
    if (NULL == pOpen->mpFirstMsg)
    {
        pOpen->mpLastMsg = NULL;
    }
    K2MEM_Copy(apRetMsg, &pMsgLink->Msg, sizeof(K2NET_MSG));
    K2NET_MemFree(pMsgLink);
    return TRUE;
}

UINT8 *             
K2NET_Usage_GetSendBuffer(
    K2NET_USAGE aUsage,
    UINT32      aMTU
)
{
    K2NET_OPEN *            pOpen;
    K2NET_PROTO *           pProto;
    UINT8 *                 pBuffer;
    K2NET_BUFFER_TRACK *    pBufferTrack;

    pBufferTrack = (K2NET_BUFFER_TRACK *)K2NET_MemAlloc(sizeof(K2NET_BUFFER_TRACK));
    if (NULL == pBufferTrack)
        return NULL;

    pOpen = (K2NET_OPEN *)aUsage;
    pProto = pOpen->mpProto;
    pBufferTrack->TreeNode.mUserVal = (UINT32)pProto->Instance.GetSendBuffer(pProto->mProtoContext, aMTU);
    if (0 == pBufferTrack->TreeNode.mUserVal)
    {
        K2NET_MemFree(pBufferTrack);
        return NULL;
    }

    pBufferTrack->mMTU = aMTU;
    pBufferTrack->mpProto = pProto;

    return (UINT8 *)pBufferTrack->TreeNode.mUserVal;
}

BOOL                
K2NET_Usage_SendAndReleaseBuffer(
    UINT8 * apBuffer,
    UINT32  aBufferBytes
)
{
    K2NET_OPEN *            pOpen;
    K2NET_PROTO *           pProto;
    K2NET_BUFFER_TRACK *    pBufferTrack;
    K2TREE_NODE *           pTreeNode;
    BOOL                    ok;

    pTreeNode = K2TREE_Find(&gBufferTree, (UINT32)apBuffer);
    if (NULL == pTreeNode)
    {
        return FALSE;
    }

    K2TREE_Remove(&gBufferTree, pTreeNode);

    pBufferTrack = K2_GET_CONTAINER(K2NET_BUFFER_TRACK, pTreeNode, TreeNode);

    pProto = pBufferTrack->mpProto;

    ok = pProto->SendAndReleaseBuffer(pProto->mProtoContext, apBuffer, aBufferBytes);
    if (!ok)
    {
        K2TREE_Insert(&gBufferTree, pTreeNode->mUserVal, pTreeNode);
        return FALSE;
    }

    K2NET_MemFree(pBufferTrack);

    return TRUE;
}

void                
K2NET_Usage_ReleaseBuffer(
    UINT8 * apBuffer
)
{
    K2NET_OPEN *            pOpen;
    K2NET_PROTO *           pProto;
    K2NET_BUFFER_TRACK *    pBufferTrack;
    K2TREE_NODE *           pTreeNode;
    BOOL                    ok;

    pTreeNode = K2TREE_Find(&gBufferTree, (UINT32)apBuffer);
    if (NULL == pTreeNode)
    {
        return FALSE;
    }

    K2TREE_Remove(&gBufferTree, pTreeNode);

    pBufferTrack = K2_GET_CONTAINER(K2NET_BUFFER_TRACK, pTreeNode, TreeNode);

    pProto = pBufferTrack->mpProto;

    pProto->ReleaseBuffer(pProto->mProtoContext, apBuffer);

    K2NET_MemFree(pBufferTrack);

    return TRUE;
}

void
K2NET_Init(
    K2NET_pf_MemAlloc afMemAlloc,
    K2NET_pf_MemFree  afMemFree,
    K2NET_pf_AddTimer afAddTimer,
    K2NET_pf_DelTimer afDelTimer
)
{
    sgfMemAlloc = afMemAlloc;
    sgfMemFree = afMemFree;
    sgfAddTimer = afAddTimer;
    sgfDelTimer = afDelTimer;

    gNextProtoInstanceId = 1;

    K2TREE_Init(&gRegProtoTree, NULL);
    K2TREE_Init(&gProtoInstTree, NULL);
    K2TREE_Init(&gBufferTree, NULL);
}

void
K2NET_Done(
    void
)
{

}
