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

typedef struct _IPCP_OPEN IPCP_OPEN;
struct _IPCP_OPEN
{
    K2NET_PROTO_INSTOPEN *  mpInstOpen;
    BOOL                    mInformedOfTimeout;
    K2LIST_LINK             ListLink;
};

void
K2PPPIpcp_OnDoorbell(
    void * apContext
)
{
    K2PPP_IPCP_OID *    pIpcpOid;
    void *              pLcpOpen;
    IPCP_OPEN *         pOpen;
    K2LIST_LINK *       pListLink;
    K2NET_MSG           netMsg;

    //
    // callback on our Open() of Lcp)
    //
    pIpcpOid = (K2PPP_IPCP_OID *)apContext;

    do {
        if (!K2NET_Proto_GetMsg(pIpcpOid->mpLcpOpen, &netMsg))
            return;

        if (netMsg.mShort == K2NET_MsgShort_Timeout)
        {
            pLcpOpen = pIpcpOid->mpLcpOpen;
            if (NULL != pLcpOpen)
            {
                //
                // this will stop LCP from trying to reconnect *FOR US*
                //
                pIpcpOid->mpLcpOpen = NULL;
                K2NET_Proto_Close(pLcpOpen);    // this will queue a hostDown notification to us
            }

            //
            // queue a transatory timeout to anybody who has opened us
            //
            pListLink = pIpcpOid->OpenList.mpHead;
            if (NULL != pListLink)
            {
                do {
                    pOpen = K2_GET_CONTAINER(IPCP_OPEN, pListLink, ListLink);
                    pListLink = pListLink->mpNext;
                    if (!pOpen->mInformedOfTimeout)
                    {
                        pOpen->mInformedOfTimeout = TRUE;
                        K2NET_ProtoOpen_QueueNetMsg(pOpen->mpInstOpen, K2NET_MsgShort_Timeout);
                    }
                } while (NULL != pListLink);
            }
        }
    } while (1);
}

void
K2PPPIpcp_OnStart(
    K2NET_LAYER *  apLayer
)
{
    K2PPP_IPCP_OID *    pIpcpOid;
    K2PPP_LINK *        pLink;
    K2LIST_LINK *       pListLink;
    K2PPP_OID *         pOid;

    pIpcpOid = K2_GET_CONTAINER(K2PPP_IPCP_OID, apLayer, PppOid.HostLayer.Layer);
    pLink = K2_GET_CONTAINER(K2PPP_LINK, apLayer->mpHostBelowThis, NetHostLayer.Host.HostApi);

    //
    // find the LCP and open it.  this will start the link
    //
    pOid = NULL;
    pListLink = pLink->NetHostLayer.Host.LayersAboveList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOid = K2_GET_CONTAINER(K2PPP_OID, pListLink, HostLayer.Layer.HostLayersAboveListLink);
            if (pOid->HostLayer.Layer.mProtocolId == K2_PPP_PROTO_LCP)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    if (NULL != pOid)
    {
        pIpcpOid->mpLcpOpen = K2NET_Proto_Open(pOid->HostLayer.mInstanceId, pIpcpOid, K2PPPIpcp_OnDoorbell);
    }
}

void
K2PPPIpcp_OnFinish(
    K2NET_LAYER * apLayer
)
{
    K2PPP_IPCP_OID *    pIpcpOid;
    K2PPP_LINK *        pLink;
    K2LIST_LINK *       pListLink;
    K2NET_LAYER *       pLayer;
    IPCP_OPEN *         pOpen;
    void *              pLcpOpen;

    pIpcpOid = K2_GET_CONTAINER(K2PPP_IPCP_OID, apLayer, PppOid.HostLayer.Layer);
    pLink = K2_GET_CONTAINER(K2PPP_LINK, apLayer->mpHostBelowThis, NetHostLayer.Host.HostApi);

    if (NULL != pIpcpOid->mpLcpOpen)
    {
        //
        // this will stop LCP from trying to reconnect *FOR US*
        //
        pLcpOpen = pIpcpOid->mpLcpOpen;
        pIpcpOid->mpLcpOpen = NULL;
        K2NET_Proto_Close(pLcpOpen);    // this will queue a hostDown notification to us
    }

    if (pIpcpOid->PppOid.HostLayer.Layer.mLastFinishReason == K2NET_LayerFinishReason_TimeOut)
    {
        //
        // notify anybody who opened us that we timed out
        //
        pListLink = pIpcpOid->OpenList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pOpen = K2_GET_CONTAINER(IPCP_OPEN, pListLink, ListLink);
                pListLink = pListLink->mpNext;
                if (!pOpen->mInformedOfTimeout)
                {
                    pOpen->mInformedOfTimeout = TRUE;
                    K2NET_ProtoOpen_QueueNetMsg(pOpen->mpInstOpen, K2NET_MsgShort_Timeout);
                }
            } while (NULL != pListLink);
        }

        //
        // tell any oids above us that we timed out
        //
        pListLink = pIpcpOid->PppOid.HostLayer.Host.LayersAboveList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
                K2NET_QueueNotify(pLayer, K2NET_Notify_TimedOut, 0);
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
    }
}

void
K2PPPIpcp_OnUp(
    K2NET_PROTOAPI * apProtoApi
)
{
    K2PPP_IPCP_OID *pIpcpOid;
    K2PPP_LINK *    pLink;
    K2LIST_LINK *   pListLink;
    K2NET_LAYER *   pLayer;
    IPCP_OPEN *     pOpen;

    pIpcpOid = K2_GET_CONTAINER(K2PPP_IPCP_OID, apProtoApi, PppOid.HostLayer.Layer.Api);
    pLink = K2_GET_CONTAINER(K2PPP_LINK, pIpcpOid->PppOid.HostLayer.Layer.mpHostBelowThis, NetHostLayer.Host.HostApi);

    pIpcpOid->PppOid.mIsUp = TRUE;

    //
    // anything above us sent hostisup
    //
    pListLink = pIpcpOid->PppOid.HostLayer.Host.LayersAboveList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
            printf("%s queue notify to %s HostIsUp\n", pIpcpOid->PppOid.HostLayer.Layer.mName, pLayer->mName);
            K2NET_QueueNotify(pLayer, K2NET_Notify_HostIsUp, 0);
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    //
    // anybody who opened us is told we are up
    //
    pListLink = pIpcpOid->OpenList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOpen = K2_GET_CONTAINER(IPCP_OPEN, pListLink, ListLink);
            pListLink = pListLink->mpNext;
            K2NET_ProtoOpen_QueueNetMsg(pOpen->mpInstOpen, K2NET_MsgShort_Up);
        } while (NULL != pListLink);
    }
}

void
K2PPPIpcp_OnDown(
    K2NET_PROTOAPI *    apProtoApi
)
{
    K2PPP_IPCP_OID *pIpcpOid;
    K2PPP_LINK *    pLink;
    K2LIST_LINK *   pListLink;
    K2NET_LAYER *   pLayer;
    IPCP_OPEN *     pOpen;

    pIpcpOid = K2_GET_CONTAINER(K2PPP_IPCP_OID, apProtoApi, PppOid.HostLayer.Layer.Api);
    pLink = K2_GET_CONTAINER(K2PPP_LINK, pIpcpOid->PppOid.HostLayer.Layer.mpHostBelowThis, NetHostLayer.Host.HostApi);

    pIpcpOid->PppOid.mIsUp = FALSE;

    //
    // anything above us sent hostisDown
    //
    pListLink = pIpcpOid->PppOid.HostLayer.Host.LayersAboveList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
            printf("%s queue notify to %s HostIsDown\n", pIpcpOid->PppOid.HostLayer.Layer.mName, pLayer->mName);
            if (pLayer->mHostInformedUp)
            {
                K2NET_QueueNotify(pLayer, K2NET_Notify_HostIsDown, 0);
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    //
    // anybody who opened us is told we are down
    //
    pListLink = pIpcpOid->OpenList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOpen = K2_GET_CONTAINER(IPCP_OPEN, pListLink, ListLink);
            pListLink = pListLink->mpNext;
            K2NET_ProtoOpen_QueueNetMsg(pOpen->mpInstOpen, K2NET_MsgShort_Down);
        } while (NULL != pListLink);
    }
}

void
K2PPPIpcp_SendConfigRequest(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPIpcp_SendConfigAck(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPIpcp_SendConfigNack(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPIpcp_SendTermRequest(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPIpcp_SendTermAck(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPIpcp_SendCodeReject(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPIpcp_SendEchoReply(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPIpcp_OnRecvConfigRequest(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPIpcp_OnRecvConfigAck(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPIpcp_OnRecvConfigNack(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPIpcp_OnRecvConfigReject(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPIpcp_OnRecvTermRequest(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPIpcp_OnRecvTermAck(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPIpcp_OnRecvCodeReject(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

typedef void (*K2PPPIpcp_pf_OnRecv)(K2PPP_LINK *apLink, UINT8 const *apData, UINT32 aDataBytes, UINT32 aRecvBytes);

void
K2PPPIpcp_OnRecv(
    K2NET_PROTOAPI *    apProtoApi,
    UINT8 const *       apData,
    UINT32              aDataBytes
)
{
    K2PPP_OID *         pIpcpOid;
    K2PPP_LINK *        pLink;
    UINT16              u16val;
    K2PPPIpcp_pf_OnRecv fRecv;

    pIpcpOid = K2_GET_CONTAINER(K2PPP_OID, apProtoApi, HostLayer.Layer.Api);
    pLink = K2_GET_CONTAINER(K2PPP_LINK, pIpcpOid->HostLayer.Layer.mpHostBelowThis, NetHostLayer.Host.HostApi);

    if (aDataBytes < 2)
    {
        return;
    }

    // copy length to aligned var
    K2MEM_Copy(&u16val, apData + 2, sizeof(UINT16));
    if (u16val < 2)
    {
        return;
    }

    if (aDataBytes < (UINT32)u16val)
    {
        return;
    }

    fRecv = NULL;

    switch (*apData)
    {
    case K2_PPP_IPCP_CONFIGURE_REQUEST:
        fRecv = K2PPPIpcp_OnRecvConfigRequest;
        break;

    case K2_PPP_IPCP_CONFIGURE_ACK:
        fRecv = K2PPPIpcp_OnRecvConfigAck;
        break;

    case K2_PPP_IPCP_CONFIGURE_NACK:
        fRecv = K2PPPIpcp_OnRecvConfigNack;
        break;

    case K2_PPP_IPCP_CONFIGURE_REJECT:
        fRecv = K2PPPIpcp_OnRecvConfigReject;
        break;

    case K2_PPP_IPCP_TERMINATE_REQUEST:
        fRecv = K2PPPIpcp_OnRecvTermRequest;
        break;

    case K2_PPP_IPCP_TERMINATE_ACK:
        fRecv = K2PPPIpcp_OnRecvTermAck;
        break;

    case K2_PPP_IPCP_CODE_REJECT:
        fRecv = K2PPPIpcp_OnRecvCodeReject;
        break;

    default:
        break;
    }

    if (NULL != fRecv)
    {
        fRecv(pLink, apData, u16val, aDataBytes);
    }
}

void *
K2PPPIpcp_OnOpen(
    K2NET_PROTOAPI *        apProto,
    K2NET_PROTO_INSTOPEN *  apInstOpen
)
{
    K2PPP_IPCP_OID *    pIpcpOid;
    IPCP_OPEN *         pOpen;

    // this is the same as acquire

    pIpcpOid = K2_GET_CONTAINER(K2PPP_IPCP_OID, apProto, PppOid.HostLayer.Layer.Api);

    pOpen = (IPCP_OPEN *)K2NET_MemAlloc(sizeof(IPCP_OPEN));
    if (NULL == pOpen)
        return NULL;
    pOpen->mpInstOpen = apInstOpen;
    pOpen->mInformedOfTimeout = FALSE;
    K2LIST_AddAtTail(&pIpcpOid->OpenList, &pOpen->ListLink);

    if (1 == ++pIpcpOid->PppOid.mAcquireCount)
    {
        if (NULL != pIpcpOid->PppOid.OnFirstAcquire)
        {
            pIpcpOid->PppOid.OnFirstAcquire(&pIpcpOid->PppOid);
        }
    }

    return pOpen;
}

void    
K2PPPIpcp_OnClose(
    K2NET_PROTOAPI *apProto,
    void *          apContext
)
{
    K2PPP_IPCP_OID *    pIpcpOid;
    K2LIST_LINK *       pListLink;
    IPCP_OPEN *         pOpen;

    // this is the same as release

    pIpcpOid = K2_GET_CONTAINER(K2PPP_IPCP_OID, apProto, PppOid.HostLayer.Layer.Api);

    pOpen = NULL;
    pListLink = pIpcpOid->OpenList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOpen = K2_GET_CONTAINER(IPCP_OPEN, pListLink, ListLink);
            if (apContext == (void*)pOpen)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
    if (NULL == pListLink)
        return;

    K2LIST_Remove(&pIpcpOid->OpenList, &pOpen->ListLink);
    K2NET_MemFree(pOpen);

    if (0 == --pIpcpOid->PppOid.mAcquireCount)
    {
        if (NULL != pIpcpOid->PppOid.OnLastRelease)
        {
            pIpcpOid->PppOid.OnLastRelease(&pIpcpOid->PppOid);
        }
    }
}

K2NET_HOST_LAYER * 
K2PPP_IpcpFactory(
    UINT32                              aProtocolId,
    K2NET_STACK_PROTO_CHUNK_HDR const * apConfig
)
{
    K2PPP_IPCP_OID * pIpcpOid;

    if (aProtocolId != K2_PPP_PROTO_IPCP)
        return NULL;

    pIpcpOid = (K2PPP_IPCP_OID *)K2NET_MemAlloc(sizeof(K2PPP_IPCP_OID));
    if (NULL == pIpcpOid)
        return NULL;

    K2PPP_OID_Init(&pIpcpOid->PppOid, K2_PPP_PROTO_IPCP);

    K2ASC_CopyLen(pIpcpOid->PppOid.HostLayer.Layer.mName, "PPP_IPCP", K2NET_LAYER_NAME_MAX_LEN);

    pIpcpOid->PppOid.HostLayer.Layer.OnStart = K2PPPIpcp_OnStart;
    pIpcpOid->PppOid.HostLayer.Layer.OnFinish = K2PPPIpcp_OnFinish;
    pIpcpOid->PppOid.HostLayer.Layer.Api.OnUp = K2PPPIpcp_OnUp;
    pIpcpOid->PppOid.HostLayer.Layer.Api.OnDown = K2PPPIpcp_OnDown;
    pIpcpOid->PppOid.HostLayer.Layer.Api.OnRecv = K2PPPIpcp_OnRecv;
    pIpcpOid->PppOid.HostLayer.Layer.Api.OnOpen = K2PPPIpcp_OnOpen;
    pIpcpOid->PppOid.HostLayer.Layer.Api.OnClose = K2PPPIpcp_OnClose;

    pIpcpOid->PppOid.mfSend_CodeReject = K2PPPIpcp_SendCodeReject;
    pIpcpOid->PppOid.mfSend_ConfigAck = K2PPPIpcp_SendConfigAck;
    pIpcpOid->PppOid.mfSend_ConfigNack = K2PPPIpcp_SendConfigNack;
    pIpcpOid->PppOid.mfSend_ConfigRequest = K2PPPIpcp_SendConfigRequest;
    pIpcpOid->PppOid.mfSend_TermAck = K2PPPIpcp_SendTermAck;
    pIpcpOid->PppOid.mfSend_TermRequest = K2PPPIpcp_SendTermRequest;

    K2LIST_Init(&pIpcpOid->OpenList);

    return &pIpcpOid->PppOid.HostLayer;
}

