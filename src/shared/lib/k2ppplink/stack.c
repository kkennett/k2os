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

#include "ik2ppplink.h"

void    
K2PPP_OnStackRecv(
    K2NET_PROTOAPI *apProto,
    UINT8 const *   apData,
    UINT32          aDataBytes
)
{
    K2PPP_LINK *    pLink;
    UINT32          check;
    K2LIST_LINK *   pListLink;
    K2NET_LAYER *   pLayer;

    if (aDataBytes < 3)
        return;

    pLink = K2_GET_CONTAINER(K2PPP_LINK, apProto, NetHostLayer.Layer.Api);

    if (pLink->mLinkPhase == K2PPP_LinkPhase_Dead)
    {
        return;
    }

    K2MEM_Copy(&check, apData, sizeof(UINT16));
    check = ((check & 0xFF) << 8) | ((check >> 8) & 0xFF);

    apData += 2;
    aDataBytes -= 2;

    if (!K2PPP_ProtoOk(pLink->mLinkPhase, check))
    {
        return;
    }

    //
    // do the recv to a matching protocol
    //
    pListLink = pLink->NetHostLayer.Host.LayersAboveList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
            if (pLayer->mProtocolId == check)
            {
                if (NULL != pLayer->Api.OnRecv)
                {
                    pLayer->Api.OnRecv(&pLayer->Api, apData, aDataBytes);
                }
                break;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
}

void    
K2PPP_OnStackNotify(
    K2NET_PROTOAPI *    apProto,
    K2NET_NotifyType    aNotifyCode,
    UINT32              aNotifyData
)
{
    K2PPP_LINK * pLink;

    printf("STACK NOTIFY(%s)\n", K2NET_gNotifyNames[aNotifyCode]);

    if ((aNotifyCode != K2NET_Notify_HostIsUp) &&
        (aNotifyCode != K2NET_Notify_HostIsDown))
    {
        return;
    }

    //
    // the only OID we bring up or down is LCP.  It will handle all
    // the other OIDs that are on top of the stack host
    //

    pLink = K2_GET_CONTAINER(K2PPP_LINK, apProto, NetHostLayer.Layer.Api);

    pLink->mpLcpHostLayer->Layer.mHostInformedUp = (aNotifyCode == K2NET_Notify_HostIsUp) ? TRUE : FALSE;

    pLink->mpLcpHostLayer->Layer.Api.OnNotify(&pLink->mpLcpHostLayer->Layer.Api, aNotifyCode, aNotifyData);
}

void
K2PPP_StackAcquire(
    K2NET_HOSTAPI * apHostApi
)
{
    K2PPP_LINK * pLink;

    printf("STACK ACQUIRE\n");

    pLink = K2_GET_CONTAINER(K2PPP_LINK, apHostApi, NetHostLayer.Host.HostApi);

    if (1 == ++pLink->mAcquireCount)
    {
        pLink->NetHostLayer.Layer.mpHostBelowThis->Acquire(pLink->NetHostLayer.Layer.mpHostBelowThis);
    }
}

void
K2PPP_StackRelease(
    K2NET_HOSTAPI * apHostApi
)
{
    K2PPP_LINK *    pLink;

    printf("STACK RELEASE\n");

    pLink = K2_GET_CONTAINER(K2PPP_LINK, apHostApi, NetHostLayer.Host.HostApi);

    if (0 == --pLink->mAcquireCount)
    {
        pLink->NetHostLayer.Layer.mpHostBelowThis->Release(pLink->NetHostLayer.Layer.mpHostBelowThis);
    }
}

K2NET_BUFFER *  
K2PPP_StackGetSendBuffer(
    K2NET_PROTOAPI *apFromProtoAbove,
    UINT32          aNumLayersAboveSender,
    UINT32          aRequestMTU
)
{
    K2NET_LAYER *       pLayerAbove;
    K2PPP_LINK *        pLink;
    K2NET_BUFFER *      pBuffer;
    K2NET_BUFFER_LAYER *pMyBufferLayer;

    pLayerAbove = K2_GET_CONTAINER(K2NET_LAYER, apFromProtoAbove, Api);

    pLink = K2_GET_CONTAINER(K2PPP_LINK, pLayerAbove->mpHostBelowThis, NetHostLayer.Host.HostApi);

    if (!K2PPP_ProtoOk(pLink->mLinkPhase, pLayerAbove->mProtocolId))
    {
        return NULL;
    }

    pBuffer = pLink->NetHostLayer.Layer.mpHostBelowThis->GetSendBuffer(
        &pLink->NetHostLayer.Layer.Api,
        aNumLayersAboveSender + 1,
        aRequestMTU + 2
    );

    if (NULL == pBuffer)
        return NULL;

    pMyBufferLayer = &pBuffer->Layer[pBuffer->mCurrentLayer];

    ++pBuffer->mCurrentLayer;
    K2_ASSERT(pBuffer->mCurrentLayer != pBuffer->mLayerCount);

    pBuffer->Layer[pBuffer->mCurrentLayer].mpData = pMyBufferLayer->mpData + 2;
    pBuffer->Layer[pBuffer->mCurrentLayer].mDataBytes = pMyBufferLayer->mDataBytes - 2;

    return pBuffer;
}

K2STAT          
K2PPP_StackSend(
    K2NET_PROTOAPI *apFromProtoAbove,
    K2NET_BUFFER ** appBuffer,
    UINT32          aSendByteCount
)
{
    K2NET_LAYER *           pLayerAbove;
    K2PPP_LINK *            pLink;
    K2NET_BUFFER *          pBuffer;
    K2NET_BUFFER_LAYER *    pMyBufferLayer;
    K2STAT                  stat;
    UINT16                  proto;

    pLayerAbove = K2_GET_CONTAINER(K2NET_LAYER, apFromProtoAbove, Api);

    pLink = K2_GET_CONTAINER(K2PPP_LINK, pLayerAbove->mpHostBelowThis, NetHostLayer.Host.HostApi);

    if (!K2PPP_ProtoOk(pLink->mLinkPhase, pLayerAbove->mProtocolId))
    {
        if (pLink->mLinkPhase == K2PPP_LinkPhase_Dead)
        {
            return K2STAT_ERROR_NOT_CONNECTED;
        }
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pBuffer = *appBuffer;
    K2_ASSERT(0 != pBuffer->mCurrentLayer);
    --pBuffer->mCurrentLayer;

    pMyBufferLayer = &pBuffer->Layer[pBuffer->mCurrentLayer];

    proto = pLayerAbove->mProtocolId;
    proto = ((proto & 0xFF) << 8) | ((proto >> 8) & 0xFF);
    K2MEM_Copy(pMyBufferLayer->mpData, &proto, sizeof(UINT16));

    stat = pLink->NetHostLayer.Layer.mpHostBelowThis->Send(
        &pLink->NetHostLayer.Layer.Api,
        appBuffer,
        aSendByteCount + 2
    );

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(pBuffer == *appBuffer);
        pBuffer->mCurrentLayer++;
        return stat;
    }

    K2_ASSERT(NULL == *appBuffer);

    return stat;
}

void            
K2PPP_StackReleaseUnsent(
    K2NET_PROTOAPI *apFromProtoAbove,
    K2NET_BUFFER ** appBuffer
)
{
    K2NET_LAYER *   pLayerAbove;
    K2PPP_LINK *    pLink;

    pLayerAbove = K2_GET_CONTAINER(K2NET_LAYER, apFromProtoAbove, Api);

    pLink = K2_GET_CONTAINER(K2PPP_LINK, pLayerAbove->mpHostBelowThis, NetHostLayer.Host.HostApi);

    pLink->NetHostLayer.Layer.mpHostBelowThis->ReleaseUnsent(
        &pLink->NetHostLayer.Layer.Api,
        appBuffer
    );
}

BOOL
K2PPP_StackOnAttach(
    K2NET_HOST *    apHost,
    K2NET_LAYER *   apLayerAttachFromAbove
)
{
    K2PPP_LINK * pLink;

    pLink = K2_GET_CONTAINER(K2PPP_LINK, apHost, NetHostLayer.Host);
    if (pLink->mLinkPhase != K2PPP_LinkPhase_Dead)
        return FALSE;

    K2_ASSERT(NULL == apLayerAttachFromAbove->mpHostBelowThis);
    K2_ASSERT(!apLayerAttachFromAbove->mHostInformedUp);
    if (apLayerAttachFromAbove->mProtocolId == K2_PPP_PROTO_LCP)
    {
        pLink->mpLcpHostLayer = K2_GET_CONTAINER(K2NET_HOST_LAYER, apLayerAttachFromAbove, Layer);
    }
    apLayerAttachFromAbove->mpHostBelowThis = &apHost->HostApi;
    K2LIST_AddAtTail(&apHost->LayersAboveList, &apLayerAttachFromAbove->HostLayersAboveListLink);

    return TRUE;
}

void
K2PPP_StackOnUpDown_Stub(
    K2NET_PROTOAPI *apProtoApi
)
{
}

void
K2PPP_StackOnStartFinish_Stub(
    K2NET_LAYER *   apLayer
)
{
}

K2NET_HOST_LAYER *
K2PPP_StackFactory(
    UINT32                              aProtocolId,
    K2NET_STACK_PROTO_CHUNK_HDR const * apConfig
)
{
    K2PPP_LINK *                    pLink;
    UINT32                          offset;
    K2NET_STACK_PROTO_CHUNK_HDR *   pProtoChunk;
    BOOL                            ok;

    if (aProtocolId != K2_IANA_PROTOCOLID_PPP)
        return NULL;

    pLink = (K2PPP_LINK *)K2NET_MemAlloc(sizeof(K2PPP_LINK));
    if (NULL == pLink)
        return NULL;

    K2MEM_Zero(pLink, sizeof(K2PPP_LINK));

    K2ASC_CopyLen(pLink->NetHostLayer.Layer.mName, "PPP_STACK", K2NET_LAYER_NAME_MAX_LEN);

    pLink->mLinkPhase = K2PPP_LinkPhase_Dead;

    pLink->NetHostLayer.Layer.mProtocolId = K2_IANA_PROTOCOLID_PPP;
    pLink->NetHostLayer.Layer.Api.OnNotify = K2PPP_OnStackNotify;
    pLink->NetHostLayer.Layer.Api.OnRecv = K2PPP_OnStackRecv;
    pLink->NetHostLayer.Layer.Api.OnUp = K2PPP_StackOnUpDown_Stub;
    pLink->NetHostLayer.Layer.Api.OnDown = K2PPP_StackOnUpDown_Stub;
    pLink->NetHostLayer.Layer.OnStart = K2PPP_StackOnStartFinish_Stub;
    pLink->NetHostLayer.Layer.OnFinish = K2PPP_StackOnStartFinish_Stub;

    pLink->NetHostLayer.Host.HostApi.Acquire = K2PPP_StackAcquire;
    pLink->NetHostLayer.Host.HostApi.Release = K2PPP_StackRelease;
    pLink->NetHostLayer.Host.HostApi.GetSendBuffer = K2PPP_StackGetSendBuffer;
    pLink->NetHostLayer.Host.HostApi.Send = K2PPP_StackSend;
    pLink->NetHostLayer.Host.HostApi.ReleaseUnsent = K2PPP_StackReleaseUnsent;

    pLink->NetHostLayer.Host.OnAttach = K2PPP_StackOnAttach;
    K2LIST_Init(&pLink->NetHostLayer.Host.LayersAboveList);

    offset = apConfig->Hdr.mSubOffset;
    while (offset < (apConfig->Hdr.mLen - sizeof(K2NET_STACK_PROTO_CHUNK_HDR)))
    {
        pProtoChunk = (K2NET_STACK_PROTO_CHUNK_HDR *)(((UINT8 *)apConfig) + offset);
        ok = K2NET_StackBuild(&pLink->NetHostLayer.Host, pProtoChunk);
        if (!ok)
            break;
        offset += pProtoChunk->Hdr.mLen;
    }

    return &pLink->NetHostLayer;
}

