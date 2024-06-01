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

typedef struct _LCP_OPEN LCP_OPEN;
struct _LCP_OPEN
{
    K2NET_PROTO_INSTOPEN *  mpInstOpen;
    K2LIST_LINK             ListLink;
    BOOL                    mInformedOfTimeout;
};

void
K2PPPLcp_OnStart(
    K2NET_LAYER * apLayer
)
{
    K2PPP_LINK * pLink;

    //
    // we know we sit on top of the PPP stack directly
    //
    pLink = K2_GET_CONTAINER(K2PPP_LINK, apLayer->mpHostBelowThis, NetHostLayer.Host.HostApi);

    K2_ASSERT(K2PPP_LinkPhase_Dead == pLink->mLinkPhase);

    pLink->mLinkPhase = K2PPP_LinkPhase_Establish;

    apLayer->mpHostBelowThis->Acquire(apLayer->mpHostBelowThis);
}

void
K2PPPLcp_OnFinish(
    K2NET_LAYER * apLayer
)
{
    K2PPP_LINK *    pLink;
    K2PPP_LCP_OID * pLcpOid;
    K2LIST_LINK *   pListLink;
    K2NET_LAYER *   pLayer;
    LCP_OPEN *      pOpen;
    K2NET_MSGLINK * pMsgLink;

    apLayer->mpHostBelowThis->Release(apLayer->mpHostBelowThis);

    //
    // we know we sit on top of the PPP stack directly
    //
    pLink = K2_GET_CONTAINER(K2PPP_LINK, apLayer->mpHostBelowThis, NetHostLayer.Host.HostApi);

    K2_ASSERT(K2PPP_LinkPhase_Dead != pLink->mLinkPhase);

    pLink->mLinkPhase = K2PPP_LinkPhase_Dead;

    if (apLayer->mLastFinishReason == K2NET_LayerFinishReason_TimeOut)
    {
        pLcpOid = K2_GET_CONTAINER(K2PPP_LCP_OID, apLayer, PppOid.HostLayer.Layer);

        //
        // notify anybody who opened us that we timed out
        //
        pListLink = pLcpOid->OpenList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pOpen = K2_GET_CONTAINER(LCP_OPEN, pListLink, ListLink);
                pListLink = pListLink->mpNext;
                if (!pOpen->mInformedOfTimeout)
                {
                    pMsgLink = (K2NET_MSGLINK *)K2NET_MemAlloc(sizeof(K2NET_MSGLINK));
                    if (NULL != pMsgLink)
                    {
                        pOpen->mInformedOfTimeout = TRUE;
                        K2NET_ProtoOpen_QueueNetMsg(pOpen->mpInstOpen, K2NET_MsgShort_Timeout);
                    }
                }
            } while (NULL != pListLink);
        }

        //
        // tell anybody above us that we timed out
        //
        pListLink = pLcpOid->PppOid.HostLayer.Host.LayersAboveList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
                printf("%s queue notify to %s TimedOut\n", apLayer->mName, pLayer->mName);
                K2NET_QueueNotify(pLayer, K2NET_Notify_TimedOut, 0);
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
    }
}

void
K2PPPLcp_OnUp(
    K2NET_PROTOAPI * apProtoApi
)
{
    K2PPP_LINK *        pLink;
    K2LIST_LINK *       pListLink;
    K2NET_LAYER *       pLayer;
    K2PPP_LCP_OID *     pLcpPppOid;
    LCP_OPEN *          pLcpOpen;

    pLcpPppOid = K2_GET_CONTAINER(K2PPP_LCP_OID, apProtoApi, PppOid.HostLayer.Layer.Api);

    pLcpPppOid->PppOid.mIsUp = TRUE;

    //
    // we know we sit on top of the PPP stack directly
    //
    pLink = K2_GET_CONTAINER(K2PPP_LINK, pLcpPppOid->PppOid.HostLayer.Layer.mpHostBelowThis, NetHostLayer.Host.HostApi);

    //
    // qual can come up now
    //
    if (NULL != pLink->mpQualLayer)
    {
        printf("%s queue notify to %s HostIsUp\n", pLcpPppOid->PppOid.HostLayer.Layer.mName, pLink->mpQualLayer->mName);
        K2NET_QueueNotify(pLink->mpQualLayer, K2NET_Notify_HostIsUp, 0);
    }

    //
    // move to auth or network depending on whether auth is being used
    //
    if (NULL != pLink->mpAuthLayer)
    {
        pLink->mLinkPhase = K2PPP_LinkPhase_Authenticate;
        printf("%s queue notify to %s HostIsUp\n", pLcpPppOid->PppOid.HostLayer.Layer.mName, pLink->mpAuthLayer->mName);
        K2NET_QueueNotify(pLink->mpAuthLayer, K2NET_Notify_HostIsUp, 0);
        // if/when auth comes up it will switch us to Network
        // and send the HostIsUp to the NCPs.
    }
    else
    {
        //
        // enter Network phase and send HostIsUp to NCPs
        // The NCPs will notify their associated NLPs that the
        // host is up when the NCPs finish connect/configure
        //
        pLink->mLinkPhase = K2PPP_LinkPhase_Network;
        pListLink = pLink->NetHostLayer.Host.LayersAboveList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
                if ((pLayer->mProtocolId >= K2_PPP_NCP_RANGE_FIRST) &&
                    (pLayer->mProtocolId <= K2_PPP_NCP_RANGE_LAST))
                {
                    printf("%s queue notify to %s HostIsUp\n", pLcpPppOid->PppOid.HostLayer.Layer.mName, pLayer->mName);
                    K2NET_QueueNotify(pLayer, K2NET_Notify_HostIsUp, 0);
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }

        //
        // notify all opens that we are up
        //
        pListLink = pLcpPppOid->OpenList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pLcpOpen = K2_GET_CONTAINER(LCP_OPEN, pListLink, ListLink);
                pListLink = pListLink->mpNext;
                K2NET_ProtoOpen_QueueNetMsg(pLcpOpen->mpInstOpen, K2NET_MsgShort_Up);
            } while (NULL != pListLink);
        }
    }
}

void
K2PPPLcp_OnDown(
    K2NET_PROTOAPI *    apProtoApi
)
{
    K2PPP_LINK *        pLink;
    K2LIST_LINK *       pListLink;
    K2NET_LAYER *       pLayer;
    K2PPP_LCP_OID *     pLcpPppOid;
    LCP_OPEN *          pLcpOpen;

    pLcpPppOid = K2_GET_CONTAINER(K2PPP_LCP_OID, apProtoApi, PppOid.HostLayer.Layer.Api);

    pLcpPppOid->PppOid.mIsUp = FALSE;

    //
    // we know we sit on top of the PPP stack directly
    //
    pLink = K2_GET_CONTAINER(K2PPP_LINK, pLcpPppOid->PppOid.HostLayer.Layer.mpHostBelowThis, NetHostLayer.Host.HostApi);

    pLink->mLinkPhase = K2PPP_LinkPhase_Terminate;

    //
    // if we go down we take everybody else on top of the stack down
    //
    pListLink = pLink->NetHostLayer.Host.LayersAboveList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pLayer = K2_GET_CONTAINER(K2NET_LAYER, pListLink, HostLayersAboveListLink);
            if (pLayer != &pLcpPppOid->PppOid.HostLayer.Layer)
            {
                if (pLayer->mHostInformedUp)
                {
                    printf("%s queue notify to %s HostIsDown\n", pLcpPppOid->PppOid.HostLayer.Layer.mName, pLayer->mName);
                    K2NET_QueueNotify(pLayer, K2NET_Notify_HostIsDown, 0);
                }
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    //
    // notify all opens that we are down
    //
    pListLink = pLcpPppOid->OpenList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pLcpOpen = K2_GET_CONTAINER(LCP_OPEN, pListLink, ListLink);
            pListLink = pListLink->mpNext;
            K2NET_ProtoOpen_QueueNetMsg(pLcpOpen->mpInstOpen, K2NET_MsgShort_Up);
        } while (NULL != pListLink);
    }
}

void
K2PPPLcp_SendConfigRequest(
    K2PPP_OID *apPppOid
)
{
    K2PPP_LINK *    pLink;
    K2NET_BUFFER *  pBuffer;
    K2STAT          stat;
    UINT8 *         pOut;
    UINT16          u16val;
    K2NET_HOSTAPI * pStackHostApi;
    UINT8 *         pBuf;

    //
    // we know we sit directy on top of the PPP stack
    //
    pStackHostApi = apPppOid->HostLayer.Layer.mpHostBelowThis;

    pLink = K2_GET_CONTAINER(K2PPP_LINK, pStackHostApi, NetHostLayer.Host.HostApi);

    u16val = sizeof(UINT8) +    // code
        sizeof(UINT8) +         // identifier
        sizeof(UINT16);         // length
    // MRU
    u16val += 4;
    // ACCM
    u16val += 6;
    // AUTH (empty)
    u16val += 4;
    // QUAL (empty)
    u16val += 4;
    // MAGIC
    u16val += 6;
    // no Protocol compression
    // no ACFC

    pBuffer = pStackHostApi->GetSendBuffer(&apPppOid->HostLayer.Layer.Api, 0, u16val);
    if (NULL == pBuffer)
        return;

    stat = K2STAT_ERROR_TOO_SMALL;

    pBuf = pOut = pBuffer->Layer[pBuffer->mCurrentLayer].mpData;

    //
    // code
    //
    *pOut = K2_PPP_LCP_CONFIGURE_REQUEST;
    pOut++;

    //
    // identifier
    //
    *pOut = ++pLink->mConfigIdent;
    pOut++;

    //
    // length
    //
    K2MEM_Copy(pOut, &u16val, sizeof(UINT16));
    pOut += sizeof(UINT16);

    // MRU
    *pOut = K2_PPP_LCPOPT_MRU;
    pOut++;
    *pOut = 4;
    pOut++;
    K2MEM_Copy(pOut, &pLink->mOptionVal_MRU, sizeof(UINT16));
    pOut += sizeof(UINT16);

    // ACCM
    *pOut = K2_PPP_LCPOPT_ACCM;
    pOut++;
    *pOut = 6;
    pOut++;
    K2MEM_Copy(pOut, &pLink->mOptionVal_ACCM, sizeof(UINT32));
    pOut += sizeof(UINT32);

    // AUTH
    *pOut = K2_PPP_LCPOPT_AUTH;
    pOut++;
    *pOut = 4;
    pOut++;
    if (NULL != pLink->mpAuthLayer)
    {
        K2MEM_Copy(pOut, &pLink->mpAuthLayer->mProtocolId, sizeof(UINT16));
        pOut += sizeof(UINT16);
    }
    else
    {
        *pOut = 0;
        pOut++;
        *pOut = 0;
        pOut++;
    }

    // QUAL
    *pOut = K2_PPP_LCPOPT_QUAL;
    pOut++;
    *pOut = 4;
    pOut++;
    if (NULL != pLink->mpQualLayer)
    {
        K2MEM_Copy(pOut, &pLink->mpQualLayer->mProtocolId, sizeof(UINT16));
        pOut += sizeof(UINT16);
    }
    else
    {
        *pOut = 0;
        pOut++;
        *pOut = 0;
        pOut++;
    }

    // MAGIC
    *pOut = K2_PPP_LCPOPT_MAGIC;
    pOut++;
    *pOut = 6;
    pOut++;
    K2MEM_Copy(pOut, &pLink->mOptionVal_MAGIC, sizeof(UINT32));
    pOut += sizeof(UINT32);

    stat = pStackHostApi->Send(
        &apPppOid->HostLayer.Layer.Api,
        &pBuffer,
        (UINT32)(pOut - pBuf)
    );

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL != pBuffer);
        pStackHostApi->ReleaseUnsent(
            &apPppOid->HostLayer.Layer.Api,
            &pBuffer
        );
    }

    K2_ASSERT(NULL == pBuffer);
}

void
K2PPPLcp_SendConfigAck(
    K2PPP_OID *apPppOid
)
{
    K2PPP_LINK *    pLink;
    K2NET_BUFFER *  pBuffer;
    K2STAT          stat;
    K2NET_HOSTAPI * pStackHostApi;

    //
    // we know we sit directy on top of the PPP stack
    //
    pStackHostApi = apPppOid->HostLayer.Layer.mpHostBelowThis;
    pLink = K2_GET_CONTAINER(K2PPP_LINK, pStackHostApi, NetHostLayer.Host.HostApi);

    K2_ASSERT(NULL != pLink->mpLastRecvConfigReq);

    //
    // just change the code from what we received and send it back
    //
    pBuffer = pStackHostApi->GetSendBuffer(&apPppOid->HostLayer.Layer.Api, 0, pLink->mLastRecvConfigReqLen);
    if (NULL == pBuffer)
        return;

    stat = K2STAT_ERROR_TOO_SMALL;

    K2MEM_Copy(pBuffer->Layer[pBuffer->mCurrentLayer].mpData, pLink->mpLastRecvConfigReq, pLink->mLastRecvConfigReqLen);
    
    *pBuffer->Layer[pBuffer->mCurrentLayer].mpData = K2_PPP_LCP_CONFIGURE_ACK;

    stat = pStackHostApi->Send(
        &apPppOid->HostLayer.Layer.Api,
        &pBuffer,
        pLink->mLastRecvConfigReqLen
    );

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL != pBuffer);
        pStackHostApi->ReleaseUnsent(
            &apPppOid->HostLayer.Layer.Api,
            &pBuffer
        );
    }

    K2_ASSERT(NULL == pBuffer);
}

void
K2PPPLcp_SendConfigNack(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPLcp_SendTermRequest(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPLcp_SendTermAck(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPLcp_SendCodeReject(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPLcp_SendEchoReply(
    K2PPP_OID *apPppOid
)
{
}

void
K2PPPLcp_OnRecvConfigRequest(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
    UINT32          bytesLeft;
    UINT8 const *   pIn;
    BOOL            ackIt;
    K2PPP_OID *     pLcpOid;
    UINT8 *         pResponse;
    UINT16          check16;
    UINT32          check32;

    // ident must match the last one we transmitted
    if (apData[1] != apLink->mConfigIdent)
    {
        return;
    }

    bytesLeft = 0;
    K2MEM_Copy(&bytesLeft, &apData[2], sizeof(UINT16));
    if (bytesLeft != aDataBytes)
    {
        return;
    }

    //
    // we will save this request so it can be used with ack/nack
    //
    bytesLeft -= 4; // code, identifier, length
    pIn = apData + 4;

    ackIt = TRUE;

    while (bytesLeft > 0)
    {
        if ((0 == pIn[1]) || (bytesLeft < pIn[1]))
        {
            // invalid option (length too big or length zero
            return;
        }

        switch (*pIn)
        {
        case K2_PPP_LCPOPT_MRU:
            if (pIn[1] != 4)
            {
                // invalid option (wrong size)
                return;
            }

            K2MEM_Copy(&check16, &pIn[2], sizeof(UINT16));
            if (check16 > apLink->mOptionVal_MRU)
            {
                ackIt = FALSE;
            }
            break;

        case K2_PPP_LCPOPT_ACCM:
            // whatever, we don't care. just check for valid size
            if (pIn[1] != 6)
            {
                // invalid option (wrong size)
                return;
            }
            break;

        case K2_PPP_LCPOPT_AUTH:
            if (pIn[1] != 4)
            {
                // invalid option (wrong size)
                return;
            }

            // only no-auth is supported right now
            K2MEM_Copy(&check16, &pIn[2], sizeof(UINT16));
            if (check16 != 0)
            {
                ackIt = FALSE;
            }
            break;

        case K2_PPP_LCPOPT_QUAL:
            if (pIn[1] != 4)
            {
                // invalid option (wrong size)
                return;
            }

            // only no-qual is supported right now
            K2MEM_Copy(&check16, &pIn[2], sizeof(UINT16));
            if (check16 != 0)
            {
                ackIt = FALSE;
            }
            break;

        case K2_PPP_LCPOPT_MAGIC:
            if (pIn[1] != 6)
            {
                // invalid option (wrong size)
                return;
            }

            K2MEM_Copy(&check32, &pIn[2], sizeof(UINT32));
            if (check32 == apLink->mOptionVal_MAGIC)
            {
                // detected loopback
                printf("\nDETECTED LOOPBACK\n");
                //ackIt = FALSE;
            }
            break;

        default:
            // anything else we don't accept (boolean options)
            ackIt = FALSE;
            break;
        }

        bytesLeft -= pIn[1];
        pIn += pIn[1];
    }

    pResponse = (UINT8 *)K2NET_MemAlloc(aDataBytes);
    if (NULL == pResponse)
        return;

    K2MEM_Copy(pResponse, apData, aDataBytes);
    if (NULL != apLink->mpLastRecvConfigReq)
    {
        K2NET_MemFree(apLink->mpLastRecvConfigReq);
    }
    apLink->mpLastRecvConfigReq = pResponse;
    apLink->mLastRecvConfigReqLen = aDataBytes;

    pLcpOid = K2_GET_CONTAINER(K2PPP_OID, apLink->mpLcpHostLayer, HostLayer);

    if (ackIt)
    {
        bytesLeft = K2PPP_OidAutoEvent_RCR_GOOD;
    }
    else
    {
        bytesLeft = K2PPP_OidAutoEvent_RCR_BAD;
    }

    K2PPP_OIDAUTO_Run(&pLcpOid->Automaton, (K2PPP_OidAutoEventType)bytesLeft);
}

void
K2PPPLcp_OnRecvConfigAck(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
    K2PPP_OID * pLcpOid;

    //
    // confirm reequested id is the right one
    //
    if (apData[1] != apLink->mConfigIdent)
    {
        return;
    }

    pLcpOid = K2_GET_CONTAINER(K2PPP_OID, apLink->mpLcpHostLayer, HostLayer);

    K2PPP_OIDAUTO_Run(&pLcpOid->Automaton, K2PPP_OidAutoEvent_RCA);
}

void
K2PPPLcp_OnRecvConfigNack(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPLcp_OnRecvConfigReject(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPLcp_OnRecvTermRequest(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPLcp_OnRecvTermAck(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPLcp_OnRecvCodeReject(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPLcp_OnRecvProtoReject(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPLcp_OnRecvEchoRequest(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPLcp_OnRecvEchoReply(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

void
K2PPPLcp_OnRecvDiscardRequest(
    K2PPP_LINK *    apLink,
    UINT8 const *   apData,
    UINT32          aDataBytes,
    UINT32          aRecvBytes
)
{
}

typedef void (*K2PPPLcp_pf_OnRecv)(K2PPP_LINK *apLink, UINT8 const *apData, UINT32 aDataBytes, UINT32 aRecvBytes);

void
K2PPPLcp_OnRecv(
    K2NET_PROTOAPI *    apProtoApi,
    UINT8 const *       apData,
    UINT32              aDataBytes
)
{
    UINT16              u16val;
    K2PPP_LINK *        pLink;
    K2PPPLcp_pf_OnRecv  fRecv;
    K2PPP_OID *         pLcpOid;

    pLcpOid = K2_GET_CONTAINER(K2PPP_OID, apProtoApi, HostLayer.Layer.Api);

    //
    // we know we sit directy on top of the PPP stack
    //
    pLink = K2_GET_CONTAINER(K2PPP_LINK, pLcpOid->HostLayer.Layer.mpHostBelowThis, NetHostLayer.Host.HostApi);

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
    case K2_PPP_LCP_CONFIGURE_REQUEST:
        fRecv = K2PPPLcp_OnRecvConfigRequest;
        break;

    case K2_PPP_LCP_CONFIGURE_ACK:
        fRecv = K2PPPLcp_OnRecvConfigAck;
        break;

    case K2_PPP_LCP_CONFIGURE_NACK:
        fRecv = K2PPPLcp_OnRecvConfigNack;
        break;

    case K2_PPP_LCP_CONFIGURE_REJECT:
        fRecv = K2PPPLcp_OnRecvConfigReject;
        break;

    case K2_PPP_LCP_TERMINATE_REQUEST:
        fRecv = K2PPPLcp_OnRecvTermRequest;
        break;

    case K2_PPP_LCP_TERMINATE_ACK:
        fRecv = K2PPPLcp_OnRecvTermAck;
        break;

    case K2_PPP_LCP_CODE_REJECT:
        fRecv = K2PPPLcp_OnRecvCodeReject;
        break;

    case K2_PPP_LCP_PROTOCOL_REJECT:
        fRecv = K2PPPLcp_OnRecvProtoReject;
        break;

    case K2_PPP_LCP_ECHO_REQUEST:
        fRecv = K2PPPLcp_OnRecvEchoRequest;
        break;

    case K2_PPP_LCP_ECHO_REPLY:
        fRecv = K2PPPLcp_OnRecvEchoReply;
        break;

    case K2_PPP_LCP_DISCARD_REQUEST:
        fRecv = K2PPPLcp_OnRecvDiscardRequest;
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
K2PPPLcp_OnOpen(
    K2NET_PROTOAPI *        apProto,
    K2NET_PROTO_INSTOPEN *  apInstOpen
)
{
    K2PPP_LCP_OID * pLcp;
    LCP_OPEN *      pOpen;

    pLcp = K2_GET_CONTAINER(K2PPP_LCP_OID, apProto, PppOid.HostLayer.Layer.Api);

    pOpen = (LCP_OPEN *)K2NET_MemAlloc(sizeof(LCP_OPEN));
    if (NULL == pOpen)
        return NULL;
    pOpen->mpInstOpen = apInstOpen;
    K2LIST_AddAtTail(&pLcp->OpenList, &pOpen->ListLink);

    if (1 == ++pLcp->PppOid.mAcquireCount)
    {
        if (NULL != pLcp->PppOid.OnFirstAcquire)
        {
            pLcp->PppOid.OnFirstAcquire(&pLcp->PppOid);
        }
    }

    return pOpen;
}

void
K2PPPLcp_OnClose(
    K2NET_PROTOAPI *    apProto,
    void *              apContext
)
{
    K2PPP_LCP_OID * pLcp;
    K2LIST_LINK *   pListLink;
    LCP_OPEN *      pOpen;

    pLcp = K2_GET_CONTAINER(K2PPP_LCP_OID, apProto, PppOid.HostLayer.Layer.Api);

    pOpen = NULL;
    pListLink = pLcp->OpenList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pOpen = K2_GET_CONTAINER(LCP_OPEN, pListLink, ListLink);
            if (apContext == (void*)pOpen)
                break;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
    if (NULL == pListLink)
        return;

    K2LIST_Remove(&pLcp->OpenList, &pOpen->ListLink);
    K2NET_MemFree(pOpen);

    if (0 == --pLcp->PppOid.mAcquireCount)
    {
        if (NULL != pLcp->PppOid.OnLastRelease)
        {
            pLcp->PppOid.OnLastRelease(&pLcp->PppOid);
        }
    }
}

K2NET_HOST_LAYER * 
K2PPP_LcpFactory(
    UINT32                              aProtocolId,
    K2NET_STACK_PROTO_CHUNK_HDR const * apConfig
)
{
    K2PPP_LCP_OID * pLcp;

    if (aProtocolId != K2_PPP_PROTO_LCP)
        return NULL;

    pLcp = (K2PPP_LCP_OID *)K2NET_MemAlloc(sizeof(K2PPP_LCP_OID));
    if (NULL == pLcp)
        return NULL;

    K2PPP_OID_Init(&pLcp->PppOid, K2_PPP_PROTO_LCP);

    K2ASC_CopyLen(pLcp->PppOid.HostLayer.Layer.mName, "PPP_LCP", K2NET_LAYER_NAME_MAX_LEN);

    pLcp->PppOid.HostLayer.Layer.OnStart = K2PPPLcp_OnStart;
    pLcp->PppOid.HostLayer.Layer.OnFinish = K2PPPLcp_OnFinish;
    pLcp->PppOid.HostLayer.Layer.Api.OnUp = K2PPPLcp_OnUp;
    pLcp->PppOid.HostLayer.Layer.Api.OnDown = K2PPPLcp_OnDown;
    pLcp->PppOid.HostLayer.Layer.Api.OnRecv = K2PPPLcp_OnRecv;
    pLcp->PppOid.HostLayer.Layer.Api.OnOpen = K2PPPLcp_OnOpen;
    pLcp->PppOid.HostLayer.Layer.Api.OnClose = K2PPPLcp_OnClose;
    pLcp->PppOid.mfSend_CodeReject = K2PPPLcp_SendCodeReject;
    pLcp->PppOid.mfSend_ConfigAck = K2PPPLcp_SendConfigAck;
    pLcp->PppOid.mfSend_ConfigNack = K2PPPLcp_SendConfigNack;
    pLcp->PppOid.mfSend_ConfigRequest = K2PPPLcp_SendConfigRequest;
    pLcp->PppOid.mfSend_EchoReply = K2PPPLcp_SendEchoReply;
    pLcp->PppOid.mfSend_TermAck = K2PPPLcp_SendTermAck;
    pLcp->PppOid.mfSend_TermRequest = K2PPPLcp_SendTermRequest;
    K2LIST_Init(&pLcp->OpenList);

    pLcp->PppOid.Automaton.mEnableResetOption = TRUE;

    return &pLcp->PppOid.HostLayer;
}

