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

BOOL
K2PPP_ProtoOk(
    K2_PPP_LinkPhaseType    aLinkPhase,
    UINT32                  aProtocol
)
{
    // in network phase everything is allowed
    if (aLinkPhase != K2PPP_LinkPhase_Network)
    {
        // LCP is always allowed in any other non-dead phase
        if (aProtocol != K2_PPP_PROTO_LCP)
        {
            if ((aLinkPhase == K2PPP_LinkPhase_Establish) ||
                (aLinkPhase == K2PPP_LinkPhase_Terminate))
            {
                // only LCP allowed in establish and terminate phase
                return FALSE;
            }

            K2_ASSERT(aLinkPhase == K2PPP_LinkPhase_Authenticate);
            if ((aProtocol < K2_PPP_LCP_RANGE_FIRST) ||
                (aProtocol > K2_PPP_LCP_RANGE_LAST))
            {
                // only LCP range allowed in authenticate phase
                return FALSE;
            }
        }
    }

    return TRUE;
}

void
K2PPP_OID_Acquire(
    K2NET_HOSTAPI * apHostApi
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apHostApi, HostLayer.Host.HostApi);

    printf("%s_Acquire\n", pPppOid->HostLayer.Layer.mName);

    if (1 == ++pPppOid->mAcquireCount)
    {
        if (NULL != pPppOid->OnFirstAcquire)
        {
            pPppOid->OnFirstAcquire(pPppOid);
        }
    }
}

void
K2PPP_OID_Release(
    K2NET_HOSTAPI * apHostApi
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apHostApi, HostLayer.Host.HostApi);

    printf("%s_Release\n", pPppOid->HostLayer.Layer.mName);

    if (0 == --pPppOid->mAcquireCount)
    {
        if (NULL != pPppOid->OnLastRelease)
        {
            pPppOid->OnLastRelease(pPppOid);
        }
    }
}

BOOL
K2PPP_OID_OnAttach(
    K2NET_HOST *    apHost,
    K2NET_LAYER *   apLayerAttachFromAbove
)
{
    K2PPP_OID * pPppOid;

    K2_ASSERT(NULL == apLayerAttachFromAbove->mpHostBelowThis);
    K2_ASSERT(!apLayerAttachFromAbove->mHostInformedUp);
    K2LIST_AddAtTail(&apHost->LayersAboveList, &apLayerAttachFromAbove->HostLayersAboveListLink);
    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apHost, HostLayer.Host);

    printf("%s_OnAttach\n", pPppOid->HostLayer.Layer.mName);

    if (pPppOid->mIsUp)
    {
        printf("%s queue notify to %s HostIsUp\n", pPppOid->HostLayer.Layer.mName, apLayerAttachFromAbove->mName);
        K2NET_QueueNotify(apLayerAttachFromAbove, K2NET_Notify_HostIsUp, 0);
    }

    return TRUE;
}

void
K2PPP_OID_OnNotify(
    K2NET_PROTOAPI *    apProto,
    K2NET_NotifyType    aNotifyCode,
    UINT32              aNotifyData
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apProto, HostLayer.Layer.Api);

    printf("%s_OnNotify\n", pPppOid->HostLayer.Layer.mName);

    if (aNotifyCode == K2NET_Notify_HostIsUp)
    {
        K2PPP_OIDAUTO_Run(&pPppOid->Automaton, K2PPP_OidAutoEvent_LowerLayerIsUp);
    }
    else if (aNotifyCode == K2NET_Notify_HostIsDown)
    {
        K2PPP_OIDAUTO_Run(&pPppOid->Automaton, K2PPP_OidAutoEvent_LowerLayerIsDown);
    }
}

void
K2PPP_OID_OnFirstAcquire(
    K2PPP_OID * apPppOid
)
{
    printf("%s(%s)\n", __FUNCTION__, apPppOid->HostLayer.Layer.mName);
    K2PPP_OIDAUTO_Run(&apPppOid->Automaton, K2PPP_OidAutoEvent_AdminOpen);
}

void
K2PPP_OID_OnLastRelease(
    K2PPP_OID * apPppOid
)
{
    printf("%s(%s)\n", __FUNCTION__, apPppOid->HostLayer.Layer.mName);
    K2PPP_OIDAUTO_Run(&apPppOid->Automaton, K2PPP_OidAutoEvent_AdminClose);
}

void
K2PPP_OID_Init(
    K2PPP_OID * apOid,
    UINT32      aProtocolId
)
{
    K2MEM_Zero(apOid, sizeof(K2PPP_OID));

    K2PPP_OIDAUTO_Init(&apOid->Automaton);

    apOid->mTimer_Period = K2_PPP_AUTO_TIMER_DEFAULT_MS;
    apOid->mMaxConfigureCount = K2_PPP_AUTO_MAX_CONFIGURE_COUNT;
    apOid->mMaxTerminateCount = K2_PPP_AUTO_MAX_TERMINATE_COUNT;
    apOid->HostLayer.Host.HostApi.Acquire = K2PPP_OID_Acquire;
    apOid->HostLayer.Host.HostApi.Release = K2PPP_OID_Release;
    K2LIST_Init(&apOid->HostLayer.Host.LayersAboveList);
    apOid->HostLayer.Host.OnAttach = K2PPP_OID_OnAttach;
    apOid->HostLayer.Layer.Api.OnNotify = K2PPP_OID_OnNotify;
    apOid->HostLayer.Layer.mProtocolId = aProtocolId;
    apOid->OnFirstAcquire = K2PPP_OID_OnFirstAcquire;
    apOid->OnLastRelease = K2PPP_OID_OnLastRelease;
    apOid->mNextClientId = 1;
}

K2STAT
K2PPP_Init(
    void
)
{
    BOOL ok;

    ok = FALSE;

    if (K2NET_Proto_Register(K2_IANA_PROTOCOLID_PPP, K2PPP_StackFactory))
    {
        if (K2NET_Proto_Register(K2_PPP_PROTO_LCP, K2PPP_LcpFactory))
        {
            if (K2NET_Proto_Register(K2_PPP_PROTO_IPCP, K2PPP_IpcpFactory))
            {
                ok = K2NET_Proto_Register(K2_PPP_PROTO_IPV4, K2PPP_Ipv4Factory);
                if (!ok)
                    K2NET_Proto_Deregister(K2_PPP_PROTO_IPCP);
            }
            if (!ok)
                K2NET_Proto_Deregister(K2_PPP_PROTO_LCP);
        }
        if (!ok)
            K2NET_Proto_Deregister(K2_IANA_PROTOCOLID_PPP);
    }

    return ok;
}

void 
K2PPP_FrameIn(
    K2PPPF *apStream,
    UINT8 **appFrame,
    UINT32  aByteCount
)
{
    K2PPP_HW *      pHw;
    K2LIST_LINK *   pListLink;
    K2NET_LAYER *   pLayer;

    pHw = K2_GET_CONTAINER(K2PPP_HW, apStream, Framer);

    pListLink = pHw->NetPhysLayer.Host.LayersAboveList.mpHead;
    if (NULL != pListLink)
    {
        pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
        pLayer->Api.OnRecv(&pLayer->Api, *appFrame, aByteCount);
    }
}

void
K2PPP_StreamOut(
    K2PPPF *apStream,
    UINT8   aByteOut
)
{
    K2PPP_HW *  pHw;
    pHw = K2_GET_CONTAINER(K2PPP_HW, apStream, Framer);
    pHw->SerialIf.DataOut(pHw->mpContext, aByteOut);
}

void
K2PPP_AcquireHw(
    K2PPP_HW *apHw
)
{
    K2LIST_LINK *   pListLink;
    K2NET_LAYER *   pLayer;

    apHw->mAcquired = TRUE;

    if (NULL != apHw->SerialIf.IsConnected)
    {
        apHw->mIsUp = apHw->SerialIf.IsConnected(apHw->mpContext);
    }
    else
    {
        apHw->mIsUp = TRUE;
    }

    if (apHw->mIsUp)
    {
        if (NULL != apHw->SerialIf.Open)
        {
            if (!apHw->SerialIf.Open(apHw->mpContext))
            {
                apHw->mIsUp = FALSE;
                return;
            }
        }

        pListLink = apHw->NetPhysLayer.Host.LayersAboveList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
                if (!pLayer->mHostInformedUp)
                {
                    K2NET_QueueNotify(pLayer, K2NET_Notify_HostIsUp, 0);
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
    }
}

void
K2PPP_ReleaseHw(
    K2PPP_HW *apHw
)
{
    K2LIST_LINK *   pListLink;
    K2NET_LAYER *   pLayer;

    if (!apHw->mAcquired)
        return;

    apHw->mAcquired = FALSE;

    if (apHw->mIsUp)
    {
        apHw->mIsUp = FALSE;

        pListLink = apHw->NetPhysLayer.Host.LayersAboveList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
                if (NULL != pLayer->Api.OnNotify)
                {
                    K2NET_QueueNotify(pLayer, K2NET_Notify_HostIsDown, 0);
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
    }

    if (NULL != apHw->SerialIf.Close)
    {
        apHw->SerialIf.Close(apHw->mpContext);
    }
}

void
K2PPP_Acquire(
    K2NET_HOSTAPI * apThisHostApi
)
{
    K2PPP_HW *  pHw;

    pHw = K2_GET_CONTAINER(K2PPP_HW, apThisHostApi, NetPhysLayer.Host.HostApi);

    if (1 == ++pHw->mOpens)
    {
        K2PPP_AcquireHw(pHw);
    }
}

void
K2PPP_Release(
    K2NET_HOSTAPI * apThisHostApi
)
{
    K2PPP_HW *  pHw;

    pHw = K2_GET_CONTAINER(K2PPP_HW, apThisHostApi, NetPhysLayer.Host.HostApi);

    K2_ASSERT(pHw->mOpens > 0);

    if (0 == --pHw->mOpens)
    {
        K2PPP_ReleaseHw(pHw);
    }
}

K2NET_BUFFER *
K2PPP_GetSendBuffer(
    K2NET_PROTOAPI *    apFromProtoAbove,
    UINT32              aLayersAboveSender,
    UINT32              aRequestMTU
)
{
    K2NET_BUFFER *  pBuffer;
    UINT32          bufBytes;
    UINT32          layerIx;

    aRequestMTU = (aRequestMTU + 1) & ~1;

    if (aRequestMTU > K2_PPP_MRU_MINIMUM)
        return NULL;

    aLayersAboveSender++;   // add senders layer for calculation

    bufBytes = aLayersAboveSender * sizeof(K2NET_BUFFER_LAYER);
    bufBytes += sizeof(K2NET_BUFFER);

    pBuffer = (K2NET_BUFFER *)K2NET_MemAlloc(bufBytes + aRequestMTU);
    if (NULL == pBuffer)
        return NULL;

    //
    // initialize buffer
    //

    pBuffer->mLayerCount = aLayersAboveSender + 1;  // our layer is layer 0

    pBuffer->Layer[0].mThisLayerIx = 0;
    pBuffer->Layer[0].mDataBytes = aRequestMTU;
    pBuffer->Layer[0].mpData = ((UINT8 *)pBuffer) + bufBytes;

    //
    // set up layer above us
    //

    pBuffer->Layer[1].mpData = pBuffer->Layer[0].mpData;
    pBuffer->Layer[1].mDataBytes = pBuffer->Layer[0].mDataBytes;
    pBuffer->Layer[1].mThisLayerIx = 1;

    pBuffer->mCurrentLayer = 1;

    for (layerIx = 2; layerIx <= aLayersAboveSender; layerIx++)
    {
        pBuffer->Layer[layerIx].mDataBytes = 0;
        pBuffer->Layer[layerIx].mpData = NULL;
        pBuffer->Layer[layerIx].mThisLayerIx = layerIx;
    }

    return pBuffer;
}

K2STAT
K2PPP_Send(
    K2NET_PROTOAPI *    apFromProtoAbove,
    K2NET_BUFFER **     appSendBuffer,
    UINT32              aByteCount
)
{
    K2PPP_HW *      pHw;
    K2NET_BUFFER *  pSendBuffer;
    K2STAT          stat;
    K2NET_LAYER *   pLayer;

    pLayer = K2_GET_CONTAINER(K2NET_LAYER, apFromProtoAbove, Api);
    if (!pLayer->mHostInformedUp)
    {
        return K2STAT_ERROR_NOT_CONNECTED;
    }

    pHw = K2_GET_CONTAINER(K2PPP_HW, pLayer->mpHostBelowThis, NetPhysLayer.Host.HostApi);

    //
    // on entry, buffer current layer is the one ABOVE this layer
    //

    pSendBuffer = *appSendBuffer;
    K2_ASSERT(NULL != pSendBuffer);

    //
    // we must be the bottom layer
    //
    K2_ASSERT(pSendBuffer->mCurrentLayer == 1);
    pSendBuffer->mCurrentLayer--;

    //
    // must at least be 2 bytes for the protocol in PPP land
    //
    if (aByteCount < 2)
    {
        pSendBuffer->mCurrentLayer++;
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    stat = K2PPPF_FrameOut(&pHw->Framer, pSendBuffer->Layer[0].mpData, aByteCount);

    if (!K2STAT_IS_ERROR(stat))
    {
        K2NET_MemFree(pSendBuffer);
        *appSendBuffer = NULL;
    }
    else
    {
        pSendBuffer->mCurrentLayer++;
    }

    return stat;
}

BOOL
K2PPP_ReleaseUnsent(
    K2NET_PROTOAPI *    apFromProtoAbove,
    K2NET_BUFFER **     appSendBuffer
)
{
    K2NET_MemFree(*appSendBuffer);
    *appSendBuffer = NULL;

    return TRUE;
}

BOOL
K2PPP_OnAttach(
    K2NET_HOST *    apHost,
    K2NET_LAYER *   apLayerAttachFromAbove
)
{
    K2PPP_HW *  pHw;

    //
    // only one layer allowed to attach to us
    //
    if (apHost->LayersAboveList.mNodeCount > 0)
    {
        return FALSE;
    }

    pHw = K2_GET_CONTAINER(K2PPP_HW, apHost, NetPhysLayer.Host);

    K2_ASSERT(NULL == apLayerAttachFromAbove->mpHostBelowThis);

    apLayerAttachFromAbove->mpHostBelowThis = &pHw->NetPhysLayer.Host.HostApi;

    K2LIST_AddAtTail(&apHost->LayersAboveList, &apLayerAttachFromAbove->HostLayersAboveListLink);

    if (pHw->mIsUp)
    {
        K2NET_QueueNotify(apLayerAttachFromAbove, K2NET_Notify_HostIsUp, 0);
    }

    return TRUE;
}

K2STAT  
K2PPP_RegisterHw(
    K2PPP_SERIALIF *    apSerialIf,
    void **             appRetRecvContext
)
{
    K2PPP_HW *  pHw;
    K2STAT      stat;

    if ((NULL == apSerialIf) ||
        (NULL == apSerialIf->DataOut) ||
        (0 == apSerialIf->mAddrLen) ||
        (K2_NET_ADAPTER_ADDR_MAX_LEN < apSerialIf->mAddrLen) ||
        (NULL == appRetRecvContext))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pHw = K2NET_MemAlloc(sizeof(K2PPP_HW));
    if (NULL == pHw)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pHw, sizeof(K2PPP_HW));

    K2MEM_Copy(&pHw->SerialIf, apSerialIf, sizeof(K2PPP_SERIALIF));
    pHw->mpSerialIf = apSerialIf;

    pHw->NetPhysLayer.Adapter.Addr.mLen = apSerialIf->mAddrLen;
    K2MEM_Copy(&pHw->NetPhysLayer.Adapter.Addr.mValue, &pHw->SerialIf.mAddr, pHw->SerialIf.mAddrLen);
    pHw->NetPhysLayer.Adapter.mPhysicalMTU = K2_PPP_MRU_MINIMUM;
    K2ASC_CopyLen(pHw->NetPhysLayer.Adapter.mName, pHw->SerialIf.mName, K2_NET_ADAPTER_NAME_MAX_LEN);
    pHw->NetPhysLayer.Adapter.mName[K2_NET_ADAPTER_NAME_MAX_LEN] = 0;
    pHw->NetPhysLayer.Adapter.mType = K2_NetAdapter_PPP;

    stat = K2PPPF_Init(&pHw->Framer, pHw->mFrameBuffer, K2PPP_FrameIn, K2PPP_StreamOut);

    pHw->NetPhysLayer.Host.HostApi.Acquire = K2PPP_Acquire;
    pHw->NetPhysLayer.Host.HostApi.GetSendBuffer = K2PPP_GetSendBuffer;
    K2LIST_Init(&pHw->NetPhysLayer.Host.LayersAboveList);
    pHw->NetPhysLayer.Host.HostApi.Release = K2PPP_Release;
    pHw->NetPhysLayer.Host.HostApi.ReleaseUnsent = K2PPP_ReleaseUnsent;
    pHw->NetPhysLayer.Host.HostApi.Send = K2PPP_Send;
    pHw->NetPhysLayer.Host.OnAttach = K2PPP_OnAttach;

    stat = K2NET_RegisterPhysLayerInstance(&pHw->NetPhysLayer);
    if (K2STAT_IS_ERROR(stat))
    {
        K2NET_MemFree(pHw);
        return stat;
    }

    *appRetRecvContext = (void *)pHw;

    return K2STAT_NO_ERROR;
}

void    
K2PPP_DataReceived(
    void *apRecvContext,
    UINT8 aByteIn
)
{
    K2PPP_HW * pHw;
    pHw = (K2PPP_HW *)apRecvContext;
    pHw->Framer.mfDataIn(&pHw->Framer, aByteIn);
}

void
K2PPP_Done(
    void
)
{

}
