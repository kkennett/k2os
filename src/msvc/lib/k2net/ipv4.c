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

#include "ik2net.h"

typedef struct _IPV4_LAYER IPV4_LAYER;
struct _IPV4_LAYER
{
    K2NET_HOST_LAYER    NetHostLayer;
    UINT32              mOpenCount;
    K2LIST_ANCHOR       OpenList;
};

void 
IPV4_HOST_Acquire(
    K2NET_HOSTAPI *apHostApi
)
{
    printf("%s(%08X)\n", __FUNCTION__, (UINT32)apHostApi);
}

void 
IPV4_HOST_Release(
    K2NET_HOSTAPI *apHostApi
)
{
    printf("%s(%08X)\n", __FUNCTION__, (UINT32)apHostApi);
}

K2NET_BUFFER * 
IPV4_HOST_GetSendBuffer(
    K2NET_PROTOAPI *apFromProtoAbove,
    UINT32 aNumLayersAboveSender,
    UINT32 aRequestMTU
)
{
    printf("%s()\n", __FUNCTION__);
    return NULL;
}

K2STAT 
IPV4_HOST_Send(
    K2NET_PROTOAPI *apFromProtoAbove,
    K2NET_BUFFER **appBuffer,
    UINT32 aSendByteCount
)
{
    printf("%s()\n", __FUNCTION__);
    return K2STAT_ERROR_NOT_IMPL;
}

void 
IPV4_HOST_ReleaseUnsent(
    K2NET_PROTOAPI *apFromProtoAbove,
    K2NET_BUFFER **appBuffer
)
{
    printf("%s()\n", __FUNCTION__);
}

typedef struct _IPV4_LAYER_OPEN IPV4_LAYER_OPEN;
struct _IPV4_LAYER_OPEN
{
    K2NET_PROTO_INSTOPEN *  mpInstOpen;
    K2LIST_LINK             ListLink;
};

void *
IPV4_LAYER_OnOpen(
    K2NET_PROTOAPI *        apProto,
    K2NET_PROTO_INSTOPEN *  apInstOpen
)
{
    IPV4_LAYER *        pIpv4Layer;
    IPV4_LAYER_OPEN *   pOpen;

    printf("%s(%08X)\n", __FUNCTION__, (UINT32)apProto);

    pIpv4Layer = K2_GET_CONTAINER(IPV4_LAYER, apProto, NetHostLayer.Layer.Api);

    pOpen = (IPV4_LAYER_OPEN *)K2NET_MemAlloc(sizeof(IPV4_LAYER_OPEN));
    if (NULL == pOpen)
        return NULL;

    pOpen->mpInstOpen = apInstOpen;
    K2LIST_AddAtTail(&pIpv4Layer->OpenList, &pOpen->ListLink);

    if (1 == ++pIpv4Layer->mOpenCount)
    {
        pIpv4Layer->NetHostLayer.Layer.mpHostBelowThis->Acquire(pIpv4Layer->NetHostLayer.Layer.mpHostBelowThis);
    }

    return pOpen;
}

void 
IPV4_LAYER_OnClose(
    K2NET_PROTOAPI *apProto,
    void *apContext
)
{
    IPV4_LAYER *        pIpv4Layer;
    K2LIST_LINK *       pListLink;
    IPV4_LAYER_OPEN *   pOpen;

    printf("%s(%08X)\n", __FUNCTION__, (UINT32)apProto);

    pIpv4Layer = K2_GET_CONTAINER(IPV4_LAYER, apProto, NetHostLayer.Layer.Api);

    pListLink = pIpv4Layer->OpenList.mpHead;
    pOpen = NULL;
    if (NULL != pListLink)
    {
        do {
            pOpen = K2_GET_CONTAINER(IPV4_LAYER_OPEN, pListLink, ListLink);
            if (apContext == (void*)pOpen)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
    if (NULL == pListLink)
        return;

    if (0 == --pIpv4Layer->mOpenCount)
    {
        pIpv4Layer->NetHostLayer.Layer.mpHostBelowThis->Release(pIpv4Layer->NetHostLayer.Layer.mpHostBelowThis);
    }

    K2LIST_Remove(&pIpv4Layer->OpenList, &pOpen->ListLink);

    K2NET_MemFree(pOpen);
}

void 
IPV4_LAYER_OnRecv(
    K2NET_PROTOAPI *apProto,
    UINT8 const *apData,
    UINT32 aDataBytes
)
{
    printf("%s()\n", __FUNCTION__);
}

void 
IPV4_LAYER_OnNotify(
    K2NET_PROTOAPI *    apProto,
    K2NET_NotifyType    aNotifyCode,
    UINT32              aNotifyData
)
{
    K2LIST_LINK *       pListLink;
    IPV4_LAYER *        pLayer;
    IPV4_LAYER_OPEN *   pOpen;
    K2NET_MsgShortType  shType;

    printf("%s()\n", __FUNCTION__);

    pLayer = K2_GET_CONTAINER(IPV4_LAYER, apProto, NetHostLayer.Layer.Api);

    pListLink = pLayer->OpenList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOpen = K2_GET_CONTAINER(IPV4_LAYER_OPEN, pListLink, ListLink);
            pListLink = pListLink->mpNext;
            switch (aNotifyCode)
            {
            case K2NET_Notify_HostIsUp:
                shType = K2NET_MsgShort_Up;
                break;
            case K2NET_Notify_HostIsDown:
                shType = K2NET_MsgShort_Down;
                break;
            case K2NET_Notify_AuthFailed:
                shType = K2NET_MsgShort_Disconnected;
                break;
            case K2NET_Notify_TimedOut:
                shType = K2NET_MsgShort_Timeout;
                break;
            default:
                shType = K2NET_MsgShort_Invalid;
                break;
            }

            K2NET_ProtoOpen_QueueNetMsg(pOpen->mpInstOpen, shType);

        } while (NULL != pListLink);
    }
}

void 
IPV4_LAYER_OnAction(
    K2NET_PROTOAPI *apProto
)
{
    printf("%s()\n", __FUNCTION__);
}

BOOL 
IPV4_HOST_OnAttach(
    K2NET_HOST *apHost,
    K2NET_LAYER *apLayerAttachFromAbove
)
{
    printf("%s()\n", __FUNCTION__);
    return FALSE;
}

void 
IPV4_LAYER_OnUp(
    K2NET_PROTOAPI *apProto
)
{
    printf("%s()\n", __FUNCTION__);
}

void 
IPV4_LAYER_OnDown(
    K2NET_PROTOAPI *apProto
)
{
    printf("%s()\n", __FUNCTION__);
}

void 
IPV4_LAYER_OnStart(
    K2NET_LAYER *apLayer
)
{
    printf("%s()\n", __FUNCTION__);
}

void
IPV4_LAYER_OnFinish(
    K2NET_LAYER * apLayer
)
{
    printf("%s()\n", __FUNCTION__);
}

K2NET_HOST_LAYER * 
K2IPV4_ProtocolFactory(
    UINT32                              aProtocolId,
    K2NET_STACK_PROTO_CHUNK_HDR const * apConfig
)
{
    IPV4_LAYER * pIpv4;

    if (aProtocolId != K2_IANA_PROTOCOLID_IPV4)
        return NULL;

    pIpv4 = (IPV4_LAYER *)K2NET_MemAlloc(sizeof(IPV4_LAYER));
    if (NULL == pIpv4)
        return NULL;

    K2MEM_Zero(pIpv4, sizeof(IPV4_LAYER));

    K2ASC_CopyLen(pIpv4->NetHostLayer.Layer.mName, "K2IPV4", K2NET_LAYER_NAME_MAX_LEN);

    pIpv4->NetHostLayer.Host.HostApi.Acquire = IPV4_HOST_Acquire;
    pIpv4->NetHostLayer.Host.HostApi.Release = IPV4_HOST_Release;
    pIpv4->NetHostLayer.Host.HostApi.GetSendBuffer = IPV4_HOST_GetSendBuffer;
    pIpv4->NetHostLayer.Host.HostApi.Send = IPV4_HOST_Send;
    pIpv4->NetHostLayer.Host.HostApi.ReleaseUnsent = IPV4_HOST_ReleaseUnsent;
    K2LIST_Init(&pIpv4->NetHostLayer.Host.LayersAboveList);
    pIpv4->NetHostLayer.Host.OnAttach = IPV4_HOST_OnAttach;

    pIpv4->NetHostLayer.Layer.Api.OnUp = IPV4_LAYER_OnUp;
    pIpv4->NetHostLayer.Layer.Api.OnDown = IPV4_LAYER_OnDown;
    pIpv4->NetHostLayer.Layer.Api.OnRecv = IPV4_LAYER_OnRecv;
    pIpv4->NetHostLayer.Layer.Api.OnOpen = IPV4_LAYER_OnOpen;
    pIpv4->NetHostLayer.Layer.Api.OnClose = IPV4_LAYER_OnClose;
    pIpv4->NetHostLayer.Layer.Api.OnNotify = IPV4_LAYER_OnNotify;
    pIpv4->NetHostLayer.Layer.OnStart = IPV4_LAYER_OnStart;
    pIpv4->NetHostLayer.Layer.OnFinish = IPV4_LAYER_OnFinish;

    pIpv4->NetHostLayer.Layer.mProtocolId = K2_IANA_PROTOCOLID_IPV4;

    return &pIpv4->NetHostLayer;
}

