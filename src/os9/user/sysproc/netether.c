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

#include "netmgr.h"

NETDEV_BUFFER *
NetDev_Ether_AcqSendBuffer(
    NETDEV *    apNetDev,
    UINT32 *    apRetMTU
)
{
    UINT32          mtu;
    UINT8 *         pDataBuffer;
    NETDEV_BUFFER * pBuffer;

    K2_ASSERT(apNetDev->Proto.mpL2->mIsStarted);

    if (NULL != apRetMTU)
    {
        *apRetMTU = 0;
    }

    pBuffer = NetDev_BufferGet(apNetDev);
    if (NULL == pBuffer)
        return NULL;

    pDataBuffer = K2OS_NetIo_AcqSendBuffer(apNetDev->mNetIo, &mtu);
    if (NULL == pDataBuffer)
    {
        NetDev_BufferPut(pBuffer);
        return NULL;
    }

    if (mtu < ETHER_FRAME_MTU)
    {
        K2OS_NetIo_RelBuffer(apNetDev->mNetIo, (UINT32)pDataBuffer);
        NetDev_BufferPut(pBuffer);
        return NULL;
    }

    pBuffer->mpData = pDataBuffer + ETHER_FRAME_HDR_LENGTH;
    pBuffer->mDataLen = mtu - ETHER_FRAME_HDR_LENGTH;

    if (NULL != apRetMTU)
    {
        *apRetMTU = mtu - ETHER_FRAME_HDR_LENGTH;
    }

    return pBuffer;
}

BOOL    
NetDev_Ether_SendBuffer(
    NETDEV *            apNetDev,
    UINT8 const *       apDstHwAddr,
    UINT32              aDstProtoId,
    NETDEV_BUFFER *     apBuffer
)
{
    UINT16  etherType;
    UINT8 * pEtherFrame;
    UINT32  etherLen;

    K2_ASSERT(apNetDev->Proto.mpL2->mIsStarted);

    K2_ASSERT(apBuffer->mDataLen <= ETHER_FRAME_MTU);
    K2_ASSERT(NULL != apDstHwAddr);

    pEtherFrame = apBuffer->mpData - ETHER_FRAME_HDR_LENGTH;
    etherLen = apBuffer->mDataLen + ETHER_FRAME_HDR_LENGTH;

    K2MEM_Copy(&pEtherFrame[ETHER_FRAME_SRCMAC_OFFSET], apNetDev->Desc.Addr.mValue, ETHER_FRAME_MAC_LENGTH);
    K2MEM_Copy(&pEtherFrame[ETHER_FRAME_DESTMAC_OFFSET], apDstHwAddr, ETHER_FRAME_MAC_LENGTH);

    etherType = (UINT16)aDstProtoId;
    etherType = K2_SWAP16(etherType);
    K2MEM_Copy(&pEtherFrame[ETHER_FRAME_TYPE_OR_LENGTH_OFFSET], &etherType, sizeof(UINT16));
#if 0
    UINT32  ix;
    Debug_Printf("\nSEND %d\n", etherLen);
    for (ix = 0; ix < etherLen; ix++)
    {
        Debug_Printf("%02X ", pEtherFrame[ix]);
        if (15 == (ix & 15))
        {
            Debug_Printf("\n");
        }
    }
    if (0 != (ix & 15))
    {
        Debug_Printf("\n");
    }
#endif

    if (K2OS_NetIo_Send(apNetDev->mNetIo, pEtherFrame, etherLen))
    {
        NetDev_BufferPut(apBuffer);
        return TRUE;
    }

    return FALSE;
}

void    
NetDev_Ether_OnRecv(
    NETDEV *        apNetDev,
    UINT8 const *   apEtherFrame,
    UINT32          aEtherLen
)
{
    UINT16              etherType;
    NETDEV_L3_pf_OnRecv fRecv;

    K2_ASSERT(apNetDev->Proto.mpL2->mIsStarted);

    if (aEtherLen <= (ETHER_FRAME_HDR_LENGTH + ETHER_FRAME_FCS_LENGTH))
    {
        // invalid or useless
        return;
    }

    K2MEM_Copy(&etherType, &apEtherFrame[ETHER_FRAME_TYPE_OR_LENGTH_OFFSET], sizeof(UINT16));
    etherType = K2_SWAP16(etherType);

//    Debug_Printf("NetEther_Recv(%04X) src %02X-%02X-%02X-%02X-%02X-%02X dst %02X-%02X-%02X-%02X-%02X-%02X\n",
//        etherType,
//        apEtherFrame[6], apEtherFrame[7], apEtherFrame[8], apEtherFrame[9], apEtherFrame[10], apEtherFrame[11],
//        apEtherFrame[0], apEtherFrame[1], apEtherFrame[2], apEtherFrame[3], apEtherFrame[4], apEtherFrame[5]);

    if (etherType == ETHER_TYPE_IPV4)
    {
        fRecv = NetDev_Ip_OnRecv;
    }
    else if (etherType == ETHER_TYPE_ARP)
    {
        fRecv = NetDev_Arp_OnRecv;
    }
    else
    {
        return;
    }
    
    fRecv(
        apNetDev,
        apEtherFrame + ETHER_FRAME_SRCMAC_OFFSET,
        apEtherFrame + ETHER_FRAME_HDR_LENGTH,
        aEtherLen - (ETHER_FRAME_HDR_LENGTH + ETHER_FRAME_FCS_LENGTH)
    );
}

void
NetDev_Ether_OnTimeExpired(
    NETDEV *    apNetDev,
    UINT32      aExpiredMs
)
{
    K2_ASSERT(apNetDev->Proto.mpL2->mIsStarted);
    NetDev_Arp_OnTimeExpired(apNetDev, aExpiredMs);
    NetDev_Ip_OnTimeExpired(apNetDev, aExpiredMs);
}

void
NetDev_Ether_OnStart(
    NETDEV *    apNetDev
    )
{
    K2_ASSERT(!apNetDev->Proto.mpL2->mIsStarted);
    apNetDev->Proto.mpL2->mIsStarted = TRUE;
    NetDev_Arp_OnStart(apNetDev);
    NetDev_Ip_OnStart(apNetDev);
}

void            
NetDev_Ether_OnStop(
    NETDEV *    apNetDev
    )
{
    K2_ASSERT(apNetDev->Proto.mpL2->mIsStarted);
    NetDev_Ip_OnStop(apNetDev);
    NetDev_Arp_OnStart(apNetDev);
    apNetDev->Proto.mpL2->mIsStarted = FALSE;
}

void
NetDev_Ether_Deinit(
    NETDEV *    apNetDev
    )
{
    K2_ASSERT(!apNetDev->Proto.mpL2->mIsStarted);
    NetDev_Ip_Deinit(apNetDev);
    NetDev_Arp_Deinit(apNetDev);
    K2OS_Heap_Free(apNetDev->Proto.mpL2);
    apNetDev->Proto.mpL2 = NULL;
}

void
NetDev_Ether_RelBuffer(
    NETDEV_BUFFER *apBuffer
)
{
    K2OS_NetIo_RelBuffer(apBuffer->mpNetDev->mNetIo, (UINT32)(apBuffer->mpData - ETHER_FRAME_HDR_LENGTH));
}

static NETDEV_L2_PROTO const sgEtherIface =
{
    NULL, 
    FALSE,
    K2_IANA_HTYPE_ETHERNET,
    ETHER_FRAME_BYTES,
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    {
        NetDev_Ether_OnStart,
        NetDev_Ether_AcqSendBuffer,
        NetDev_Ether_SendBuffer,
        NetDev_Ether_OnRecv,
        NetDev_Ether_OnTimeExpired,
        NetDev_Ether_OnStop,
        NetDev_Ether_Deinit,
        NetDev_Ether_RelBuffer
    }
};

NETDEV_L2_PROTO *  
NetDev_Ether_Create(
    NETDEV *                        apNetDev, 
    K2OS_IPV4_ADAPTER_CONFIG const *apConfig
)
{
    NETDEV_L2_PROTO *   pL2;
    BOOL                ok;

    pL2 = (NETDEV_L2_PROTO *)K2OS_Heap_Alloc(sizeof(NETDEV_L2_PROTO));

    if (NULL == pL2)
        return NULL;

    K2MEM_Zero(pL2, sizeof(NETDEV_L2_PROTO));

    apNetDev->Proto.mpL2 = pL2;

    K2MEM_Copy(pL2, &sgEtherIface, sizeof(NETDEV_L2_PROTO));
    pL2->mpNetDev = apNetDev;

    ok = FALSE;
    if (NetDev_Arp_Init(apNetDev, ETHER_TYPE_ARP))
    {
        if (NetDev_Ip_Init(apNetDev, ETHER_TYPE_IPV4, apConfig))
        {
            ok = TRUE;
        }
        else
        {
            NetDev_Arp_Deinit(apNetDev);
        }
    }

    if (!ok)
    {
        K2OS_Heap_Free(apNetDev->Proto.mpL2);
        apNetDev->Proto.mpL2 = NULL;
    }

    return apNetDev->Proto.mpL2;
}
