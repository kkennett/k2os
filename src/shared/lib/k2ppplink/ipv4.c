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

#include <lib/k2win32.h>
#include "ik2ppplink.h"

void
K2PPPIpv4_OnStart(
    K2NET_LAYER *  apLayer
)
{
    K2PPP_OID *     pIpv4Oid;
    K2NET_LAYER *   pNetIpv4Above;

    // pass through

    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, apLayer, HostLayer.Layer);

    if (0 == pIpv4Oid->HostLayer.Host.LayersAboveList.mNodeCount)
        return;

    pNetIpv4Above = K2_GET_CONTAINER(K2NET_LAYER, pIpv4Oid->HostLayer.Host.LayersAboveList.mpHead, HostLayersAboveListLink);

    pNetIpv4Above->OnStart(pNetIpv4Above);
}

void
K2PPPIpv4_OnFinish(
    K2NET_LAYER * apLayer
)
{
    K2PPP_OID *     pIpv4Oid;
    K2NET_LAYER *   pNetIpv4Above;

    // pass through

    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, apLayer, HostLayer.Layer);

    if (0 == pIpv4Oid->HostLayer.Host.LayersAboveList.mNodeCount)
        return;

    pNetIpv4Above = K2_GET_CONTAINER(K2NET_LAYER, pIpv4Oid->HostLayer.Host.LayersAboveList.mpHead, HostLayersAboveListLink);

    pNetIpv4Above->OnFinish(pNetIpv4Above);
}

void
K2PPPIpv4_OnUp(
    K2NET_PROTOAPI * apProtoApi
)
{
    K2PPP_OID *     pIpv4Oid;
    K2NET_LAYER *   pNetIpv4Above;

    // pass through

    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, apProtoApi, HostLayer.Layer.Api);

    if (0 == pIpv4Oid->HostLayer.Host.LayersAboveList.mNodeCount)
        return;

    pNetIpv4Above = K2_GET_CONTAINER(K2NET_LAYER, pIpv4Oid->HostLayer.Host.LayersAboveList.mpHead, HostLayersAboveListLink);

    pNetIpv4Above->Api.OnUp(&pNetIpv4Above->Api);
}

void
K2PPPIpv4_OnDown(
    K2NET_PROTOAPI * apProtoApi
)
{
    K2PPP_OID *     pIpv4Oid;
    K2NET_LAYER *   pNetIpv4Above;

    // pass through

    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, apProtoApi, HostLayer.Layer.Api);

    if (0 == pIpv4Oid->HostLayer.Host.LayersAboveList.mNodeCount)
        return;

    pNetIpv4Above = K2_GET_CONTAINER(K2NET_LAYER, pIpv4Oid->HostLayer.Host.LayersAboveList.mpHead, HostLayersAboveListLink);

    pNetIpv4Above->Api.OnDown(&pNetIpv4Above->Api);
}

void
K2PPPIpv4_OnRecv(
    K2NET_PROTOAPI *apProto,
    UINT8 const *   apData,
    UINT32          aDataBytes
)
{
    K2PPP_OID *     pIpv4Oid;
    K2NET_LAYER *   pNetIpv4Above;

    // pass through

    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, apProto, HostLayer.Layer.Api);

    if (0 == pIpv4Oid->HostLayer.Host.LayersAboveList.mNodeCount)
        return;

    pNetIpv4Above = K2_GET_CONTAINER(K2NET_LAYER, pIpv4Oid->HostLayer.Host.LayersAboveList.mpHead, HostLayersAboveListLink);

    pNetIpv4Above->Api.OnRecv(&pNetIpv4Above->Api, apData, aDataBytes);
}

void
K2PPPIpv4_OnNotify(
    K2NET_PROTOAPI *    apProto,
    K2NET_NotifyType    aNotifyCode,
    UINT32              aNotifyData
)
{
    K2PPP_OID *     pIpv4Oid;
    K2NET_LAYER *   pNetIpv4Above;

    // pass through up and down 
    if ((aNotifyCode != K2NET_Notify_HostIsUp) &&
        (aNotifyCode != K2NET_Notify_HostIsDown))
    {
        return;
    }

    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, apProto, HostLayer.Layer.Api);

    if (0 == pIpv4Oid->HostLayer.Host.LayersAboveList.mNodeCount)
        return;

    pNetIpv4Above = K2_GET_CONTAINER(K2NET_LAYER, pIpv4Oid->HostLayer.Host.LayersAboveList.mpHead, HostLayersAboveListLink);

    pNetIpv4Above->Api.OnNotify(&pNetIpv4Above->Api, aNotifyCode, aNotifyData);
}

void
K2PPPIpv4_OnDoorbell(
    void * apContext
)
{
    K2PPP_IPV4_OID *    pIpv4Oid;
    void *              pIpcpOpen;
    K2NET_LAYER *       pNetIpv4Above;
    K2NET_MSG           msg;

    pIpv4Oid = (K2PPP_IPV4_OID *)apContext;

    if (K2NET_Proto_GetMsg(pIpv4Oid->mpIpcpOpen, &msg))
    {
        K2_ASSERT(msg.mType == K2NET_MSGTYPE);
        if (msg.mShort == K2NET_MsgShort_Timeout)
        {
            pIpcpOpen = pIpv4Oid->mpIpcpOpen;
            if (NULL != pIpcpOpen)
            {
                //
                // this will stop IPCP from trying to reconnect *FOR US*
                //
                pIpv4Oid->mpIpcpOpen = NULL;
                K2NET_Proto_Close(pIpcpOpen);
            }

            //
            // we do not support open so there are no opens to notify transitorially (is that a word?)
            //

            //
            // inform NET IPV4 protocol above that we timed out
            //
            if (0 != pIpv4Oid->PppOid.HostLayer.Host.LayersAboveList.mNodeCount)
            {
                pIpv4Oid->mTimedOutSentToAbove = TRUE;
                pNetIpv4Above = K2_GET_CONTAINER(K2NET_LAYER, pIpv4Oid->PppOid.HostLayer.Host.LayersAboveList.mpHead, HostLayersAboveListLink);
                K2NET_QueueNotify(pNetIpv4Above, K2NET_Notify_TimedOut, 0);
            }
        }
    }
}

void
K2PPPIpv4_Acquire(
    K2NET_HOSTAPI * apHostApi
)
{
    K2PPP_IPV4_OID *    pIpv4Oid;
    K2PPP_LINK *        pLink;
    K2LIST_LINK *       pListLink;
    K2PPP_OID *         pOid;

    pIpv4Oid = K2_GET_CONTAINER(K2PPP_IPV4_OID, apHostApi, PppOid.HostLayer.Host.HostApi);
    pLink = K2_GET_CONTAINER(K2PPP_LINK, pIpv4Oid->PppOid.HostLayer.Layer.mpHostBelowThis, NetHostLayer.Host.HostApi);

    printf("%s_Acquire\n", pIpv4Oid->PppOid.HostLayer.Layer.mName);

    // 
    // find the IPCP attached to the stack
    //
    pOid = NULL;
    pListLink = pLink->NetHostLayer.Host.LayersAboveList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOid = K2_GET_CONTAINER(K2PPP_OID, pListLink, HostLayer.Layer.HostLayersAboveListLink);
            if (pOid->HostLayer.Layer.mProtocolId == K2_PPP_PROTO_IPCP)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    if (1 == ++pIpv4Oid->PppOid.mAcquireCount)
    {
        pIpv4Oid->PppOid.HostLayer.Layer.mpHostBelowThis->Acquire(pIpv4Oid->PppOid.HostLayer.Layer.mpHostBelowThis);
        if (NULL != pOid)
        {
            pIpv4Oid->mpIpcpOid = pOid;
            pIpv4Oid->mpIpcpOpen = K2NET_Proto_Open(pOid->HostLayer.mInstanceId, pIpv4Oid, K2PPPIpv4_OnDoorbell);
        }
    }
}

void
K2PPPIpv4_Release(
    K2NET_HOSTAPI * apHostApi
)
{
    K2PPP_IPV4_OID * pIpv4Oid;

    pIpv4Oid = K2_GET_CONTAINER(K2PPP_IPV4_OID, apHostApi, PppOid.HostLayer.Host.HostApi);

    printf("%s_Release\n", pIpv4Oid->PppOid.HostLayer.Layer.mName);

    if (0 == --pIpv4Oid->PppOid.mAcquireCount)
    {
        if (NULL != pIpv4Oid->mpIpcpOid)
        {
            if (NULL != pIpv4Oid->mpIpcpOpen)
            {
                K2NET_Proto_Close(pIpv4Oid->mpIpcpOpen);
                pIpv4Oid->mpIpcpOpen = NULL;
            }
            pIpv4Oid->mpIpcpOid = NULL;
        }
        pIpv4Oid->PppOid.HostLayer.Layer.mpHostBelowThis->Release(pIpv4Oid->PppOid.HostLayer.Layer.mpHostBelowThis);
    }
}

K2NET_BUFFER *
K2PPPIpv4_GetSendBuffer(
    K2NET_PROTOAPI *apFromProtoAbove,
    UINT32          aNumLayersAboveSender,
    UINT32          aRequestMTU
)
{
    K2NET_LAYER *   pLayerAbove;
    K2PPP_OID *     pIpv4Oid;

    // pass through

    pLayerAbove = K2_GET_CONTAINER(K2NET_LAYER, apFromProtoAbove, Api);
    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, pLayerAbove->mpHostBelowThis, HostLayer.Host.HostApi);

    return pIpv4Oid->HostLayer.Layer.mpHostBelowThis->GetSendBuffer(
        &pIpv4Oid->HostLayer.Layer.Api,
        aNumLayersAboveSender,
        aRequestMTU
    );
}

K2STAT
K2PPPIpv4_Send(
    K2NET_PROTOAPI *apFromProtoAbove,
    K2NET_BUFFER ** appBuffer,
    UINT32          aSendByteCount
)
{
    K2NET_LAYER *   pLayerAbove;
    K2PPP_OID *     pIpv4Oid;
    K2STAT          stat;

    // pass through

    pLayerAbove = K2_GET_CONTAINER(K2NET_LAYER, apFromProtoAbove, Api);
    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, pLayerAbove->mpHostBelowThis, HostLayer.Host.HostApi);

    stat = pIpv4Oid->HostLayer.Layer.mpHostBelowThis->Send(
        &pIpv4Oid->HostLayer.Layer.Api,
        appBuffer,
        aSendByteCount
    );

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL != *appBuffer);
    }
    else
    {
        K2_ASSERT(NULL == *appBuffer);
    }

    return stat;
}

void
K2PPPIpv4_ReleaseUnsent(
    K2NET_PROTOAPI *apFromProtoAbove,
    K2NET_BUFFER ** appBuffer
)
{
    K2NET_LAYER *   pLayerAbove;
    K2PPP_OID *     pIpv4Oid;

    // pass through

    pLayerAbove = K2_GET_CONTAINER(K2NET_LAYER, apFromProtoAbove, Api);
    pIpv4Oid = K2_GET_CONTAINER(K2PPP_OID, pLayerAbove->mpHostBelowThis, HostLayer.Host.HostApi);

    pIpv4Oid->HostLayer.Layer.mpHostBelowThis->ReleaseUnsent(
        &pIpv4Oid->HostLayer.Layer.Api,
        appBuffer
    );
}

BOOL
K2PPPIpv4_OnAttach(
    K2NET_HOST *    apHost,
    K2NET_LAYER *   apLayerAttachFromAbove
)
{
    //
    // only IPV4 can attach to us
    //
    if ((apHost->LayersAboveList.mNodeCount == 1) ||
        (apLayerAttachFromAbove->mProtocolId != K2_IANA_PROTOCOLID_IPV4))
    {
        return FALSE;
    }

    K2_ASSERT(NULL == apLayerAttachFromAbove->mpHostBelowThis);
    K2_ASSERT(!apLayerAttachFromAbove->mHostInformedUp);
    apLayerAttachFromAbove->mpHostBelowThis = &apHost->HostApi;
    K2LIST_AddAtTail(&apHost->LayersAboveList, &apLayerAttachFromAbove->HostLayersAboveListLink);

    return TRUE;
}

K2NET_HOST_LAYER * 
K2PPP_Ipv4Factory(
    UINT32                              aProtocolId,
    K2NET_STACK_PROTO_CHUNK_HDR const * apConfig
)
{
    K2PPP_IPV4_OID *                pIpv4Oid;
    UINT32                          offset;
    K2NET_STACK_PROTO_CHUNK_HDR *   pProtoChunk;
    BOOL                            ok;

    if (aProtocolId != K2_PPP_PROTO_IPV4)
        return NULL;

    pIpv4Oid = (K2PPP_IPV4_OID *)K2NET_MemAlloc(sizeof(K2PPP_IPV4_OID));
    if (NULL == pIpv4Oid)
        return NULL;

    K2PPP_OID_Init(&pIpv4Oid->PppOid, K2_PPP_PROTO_IPV4);

    K2ASC_CopyLen(pIpv4Oid->PppOid.HostLayer.Layer.mName, "PPP_IPV4", K2NET_LAYER_NAME_MAX_LEN);

    pIpv4Oid->PppOid.Automaton.mNoConfig = TRUE;

    pIpv4Oid->PppOid.HostLayer.Layer.OnStart = K2PPPIpv4_OnStart;
    pIpv4Oid->PppOid.HostLayer.Layer.OnFinish = K2PPPIpv4_OnFinish;
    pIpv4Oid->PppOid.HostLayer.Layer.Api.OnUp = K2PPPIpv4_OnUp;
    pIpv4Oid->PppOid.HostLayer.Layer.Api.OnDown = K2PPPIpv4_OnDown;
    pIpv4Oid->PppOid.HostLayer.Layer.Api.OnRecv = K2PPPIpv4_OnRecv;
    pIpv4Oid->PppOid.HostLayer.Host.HostApi.Acquire = K2PPPIpv4_Acquire;
    pIpv4Oid->PppOid.HostLayer.Host.HostApi.Release = K2PPPIpv4_Release;
    pIpv4Oid->PppOid.HostLayer.Host.HostApi.GetSendBuffer = K2PPPIpv4_GetSendBuffer;
    pIpv4Oid->PppOid.HostLayer.Host.HostApi.Send = K2PPPIpv4_Send;
    pIpv4Oid->PppOid.HostLayer.Host.HostApi.ReleaseUnsent = K2PPPIpv4_ReleaseUnsent;
    pIpv4Oid->mpIpcpOid = NULL;

    pIpv4Oid->PppOid.HostLayer.Host.OnAttach = K2PPPIpv4_OnAttach;
    K2LIST_Init(&pIpv4Oid->PppOid.HostLayer.Host.LayersAboveList);

    offset = apConfig->Hdr.mSubOffset;
    while (offset < (apConfig->Hdr.mLen - sizeof(K2NET_STACK_PROTO_CHUNK_HDR)))
    {
        pProtoChunk = (K2NET_STACK_PROTO_CHUNK_HDR *)(((UINT8 *)apConfig) + offset);
        ok = K2NET_StackBuild(&pIpv4Oid->PppOid.HostLayer.Host, pProtoChunk);
        if (!ok)
            break;
        offset += pProtoChunk->Hdr.mLen;
    }

    return &pIpv4Oid->PppOid.HostLayer;
}
