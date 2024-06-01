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

UINT32          gNextProtoInstanceId;
K2LIST_ANCHOR   gProtoList;
K2TREE_ANCHOR   gOpenInstTree;
K2LIST_ANCHOR   gRegProtoList;

BOOL    
K2NET_Proto_Register(
    UINT32                      aProtocolId,
    K2NET_pf_ProtocolFactory    afFactory
)
{
    K2NET_REGPROTO_TRACK * pTrack;
    K2LIST_LINK *   pListLink;

    if ((0 == aProtocolId) ||
        (NULL == afFactory))
    {
        return FALSE;
    }

    pListLink = gRegProtoList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTrack = K2_GET_CONTAINER(K2NET_REGPROTO_TRACK, pListLink, ListLink);
            if (pTrack->mProtocolId == aProtocolId)
            {
                // already exists
                return FALSE;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    pTrack = (K2NET_REGPROTO_TRACK *)K2NET_MemAlloc(sizeof(K2NET_REGPROTO_TRACK));
    if (NULL == pTrack)
        return FALSE;
    K2MEM_Zero(pTrack, sizeof(K2NET_REGPROTO_TRACK));

    pTrack->mProtocolId = aProtocolId;
    pTrack->mfFactory = afFactory;
    K2LIST_AddAtTail(&gRegProtoList, &pTrack->ListLink);

    return TRUE;
}

BOOL    
K2NET_Proto_Deregister(
    UINT32 aProtocolId
)
{
    K2NET_REGPROTO_TRACK * pTrack;
    K2LIST_LINK *   pListLink;

    pListLink = gRegProtoList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTrack = K2_GET_CONTAINER(K2NET_REGPROTO_TRACK, pListLink, ListLink);
            if (pTrack->mProtocolId == aProtocolId)
            {
                K2LIST_Remove(&gRegProtoList, &pTrack->ListLink);
                K2NET_MemFree(pTrack);
                return TRUE;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    return FALSE;
}

K2STAT  
K2NET_Proto_Enum(
    UINT32              aLimitProtoId,
    UINT32 *            apBufferBytes,
    K2NET_PROTO_PROP *  apRetBuffer
)
{
    UINT32          avail;
    K2LIST_LINK *   pListLink;
    K2NET_PROTO_INSTANCE *      pInstance;

    if (NULL == apBufferBytes)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }
    
    avail = *apBufferBytes;
    if (0 == avail)
    {
        if (0 == aLimitProtoId)
        {
            *apBufferBytes = gProtoList.mNodeCount * sizeof(K2NET_PROTO_PROP);
        }
        else
        {
            pListLink = gProtoList.mpHead;
            if (NULL != pListLink)
            {
                do {
                    pInstance = K2_GET_CONTAINER(K2NET_PROTO_INSTANCE, pListLink, ListLink);
                    if (pInstance->Prop.mProtocolId == aLimitProtoId)
                    {
                        (*apBufferBytes) = sizeof(K2NET_PROTO_PROP);
                        break;
                    }
                } while (NULL != pListLink);
            }
        }
        return K2STAT_ERROR_OUTBUF_TOO_SMALL;
    }

    if (NULL == apRetBuffer)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (0 == gProtoList.mNodeCount)
    {
        *apBufferBytes = 0;
        return K2STAT_NO_ERROR;
    }

    if (0 == aLimitProtoId)
    {
        if (avail < (sizeof(K2NET_PROTO_PROP) * gProtoList.mNodeCount))
        {
            return K2STAT_ERROR_OUTBUF_TOO_SMALL;
        }

        *apBufferBytes = (sizeof(K2NET_PROTO_PROP) * gProtoList.mNodeCount);

        pListLink = gProtoList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pInstance = K2_GET_CONTAINER(K2NET_PROTO_INSTANCE, pListLink, ListLink);
                K2MEM_Copy(apRetBuffer, &pInstance->Prop, sizeof(K2NET_PROTO_PROP));
                apRetBuffer++;
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
    }
    else
    {
        if (avail < sizeof(K2NET_PROTO_PROP))
        {
            return K2STAT_ERROR_OUTBUF_TOO_SMALL;
        }

        *apBufferBytes = 0;

        pListLink = gProtoList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pInstance = K2_GET_CONTAINER(K2NET_PROTO_INSTANCE, pListLink, ListLink);
                if (pInstance->Prop.mProtocolId == aLimitProtoId)
                {
                    K2MEM_Copy(apRetBuffer, &pInstance->Prop, sizeof(K2NET_PROTO_PROP));
                    (*apBufferBytes) = sizeof(K2NET_PROTO_PROP);
                    break;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
    }

    return K2STAT_NO_ERROR;
}

K2STAT  
K2NET_Proto_GetInstanceProp(
    UINT32              aInstanceId,
    UINT32 *            apBufferBytes,
    K2NET_PROTO_PROP *  apRetOneProp
)
{
    K2LIST_LINK *   pListLink;
    K2NET_PROTO_INSTANCE *      pInstance;
    UINT32          avail;

    if ((0 == aInstanceId) ||
        (NULL == apBufferBytes))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    avail = *apBufferBytes;
    if (avail < sizeof(K2NET_PROTO_PROP))
    {
        *apBufferBytes = sizeof(K2NET_PROTO_PROP);
        return K2STAT_ERROR_OUTBUF_TOO_SMALL;
    }

    if (NULL == apRetOneProp)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    *apBufferBytes = 0;

    pListLink = gProtoList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pInstance = K2_GET_CONTAINER(K2NET_PROTO_INSTANCE, pListLink, ListLink);
            if (pInstance->Prop.mInstanceId == aInstanceId)
            {
                K2MEM_Copy(apRetOneProp, &pInstance->Prop, sizeof(K2NET_PROTO_PROP));
                (*apBufferBytes) = sizeof(K2NET_PROTO_PROP);
                break;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    return K2STAT_NO_ERROR;
}

void *  
K2NET_Proto_Open(
    UINT32              aInstanceId,
    void *              apUserContext,
    K2NET_pf_Doorbell   afDoorbell
)
{
    K2LIST_LINK *           pListLink;
    K2NET_PROTO_INSTANCE *  pInstance;
    K2NET_PROTO_INSTOPEN *  pInstOpen;

    pListLink = gProtoList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pInstance = K2_GET_CONTAINER(K2NET_PROTO_INSTANCE, pListLink, ListLink);
            if (pInstance->Prop.mInstanceId == aInstanceId)
            {
                printf("%s(%s)\n", __FUNCTION__, pInstance->mpHostLayer->Layer.mName);

                if ((NULL == pInstance->mpHostLayer->Layer.Api.OnOpen) ||
                    (NULL == pInstance->mpHostLayer->Layer.Api.OnClose))
                {
                    return NULL;
                }
                pInstOpen = (K2NET_PROTO_INSTOPEN *)K2NET_MemAlloc(sizeof(K2NET_PROTO_INSTOPEN));
                if (NULL == pInstOpen)
                {
                    return NULL;
                }
                K2MEM_Zero(pInstOpen, sizeof(K2NET_PROTO_INSTOPEN));
                pInstOpen->mpUserContext = apUserContext;
                pInstOpen->mpHostLayer = pInstance->mpHostLayer;
                pInstOpen->mfDoorbell = afDoorbell;
                pInstOpen->TreeNode.mUserVal = (UINT32)pInstance->mpHostLayer->Layer.Api.OnOpen(
                    &pInstance->mpHostLayer->Layer.Api, 
                    pInstOpen
                );
                if (0 == pInstOpen->TreeNode.mUserVal)
                {
                    K2NET_MemFree(pInstOpen);
                    return NULL;
                }
                K2TREE_Insert(&gOpenInstTree, pInstOpen->TreeNode.mUserVal, &pInstOpen->TreeNode);
                return (void *)pInstOpen->TreeNode.mUserVal;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    return NULL;
}

BOOL    
K2NET_Proto_GetMsg(
    void *      apProto,
    K2NET_MSG * apRetMsg
)
{
    K2TREE_NODE *           pTreeNode;
    K2NET_PROTO_INSTOPEN *  pInstOpen;
    K2NET_MSGLINK *         pMsgLink;

    if ((NULL == apProto) ||
        (NULL == apRetMsg))
        return FALSE;

    pTreeNode = K2TREE_Find(&gOpenInstTree, (UINT32)apProto);
    if (NULL == pTreeNode)
        return FALSE;

    pInstOpen = K2_GET_CONTAINER(K2NET_PROTO_INSTOPEN, pTreeNode, TreeNode);

    pMsgLink = pInstOpen->mpMsgList;
    if (NULL == pMsgLink)
        return FALSE;

    pInstOpen->mpMsgList = pMsgLink->mpNext;
    if (NULL == pInstOpen->mpMsgList)
    {
        pInstOpen->mpMsgListEnd = NULL;
    }

    K2MEM_Copy(apRetMsg, &pMsgLink->Msg, sizeof(K2NET_MSG));

    K2NET_MemFree(pMsgLink);

    return TRUE;
}

BOOL
K2NET_Proto_Close(
    void *apProto
)
{
    K2TREE_NODE *           pTreeNode;
    K2NET_PROTO_INSTOPEN *  pInstOpen;

    pTreeNode = K2TREE_Find(&gOpenInstTree, (UINT32)apProto);
    if (NULL == pTreeNode)
        return FALSE;

    K2TREE_Remove(&gOpenInstTree, pTreeNode);

    pInstOpen = K2_GET_CONTAINER(K2NET_PROTO_INSTOPEN, pTreeNode, TreeNode);

    printf("%s(%s)\n", __FUNCTION__, pInstOpen->mpHostLayer->Layer.mName);

    pInstOpen->mpHostLayer->Layer.Api.OnClose(&pInstOpen->mpHostLayer->Layer.Api, apProto);

    K2NET_MemFree(pInstOpen);

    return TRUE;
}

BOOL    
K2NET_ProtoOpen_QueueNetMsg(
    K2NET_PROTO_INSTOPEN *  apInstOpen,
    K2NET_MsgShortType      aShort
)
{
    K2NET_MSGLINK *         pMsgLink;

    pMsgLink = (K2NET_MSGLINK *)K2NET_MemAlloc(sizeof(K2NET_MSGLINK));
    if (NULL == pMsgLink)
        return FALSE;

    pMsgLink->Msg.mType = K2NET_MSGTYPE;
    pMsgLink->Msg.mShort = aShort;
    pMsgLink->Msg.mpContext = apInstOpen->mpUserContext;
    pMsgLink->Msg.mSocket = 0;
    pMsgLink->Msg.mPayload = 0;
    pMsgLink->mpNext = NULL;

    if (NULL == apInstOpen->mpMsgList)
    {
        apInstOpen->mpMsgList = pMsgLink;
    }
    else
    {
        apInstOpen->mpMsgListEnd->mpNext = pMsgLink;
    }
    apInstOpen->mpMsgListEnd = pMsgLink;

    apInstOpen->mfDoorbell(apInstOpen->mpUserContext);

    return TRUE;
}
