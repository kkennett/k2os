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

typedef struct _NETDEV_ARP_ENTRY NETDEV_ARP_ENTRY;
struct _NETDEV_ARP_ENTRY
{
    K2TREE_NODE     EntryTreeNode;  // mUserVal is the Ip

    NETDEV_BUFFER * mpQueueHead;    // queued buffers to send to the resolved HwAddr when we find it
    NETDEV_BUFFER * mpQueueTail;    // if this is NULL then HwAddr should be valid and use mMsTTL to 
                                    // determine if ENTRY has aged out.

    UINT32          mMsTTL;         // only checked if mpQueue is empty

    UINT8           mHwAddr[4];     // first 4 of HwAddr. must be last in the structure
};

void
NetDev_Arp_Tick(
    NETDEV *        apNetDev,
    NETDEV_TIMER *  apTimer
)
{
    // TimeExpired does the work.  This is just here to make sure the callback happens once a second at least
#if 0
    K2TREE_ANCHOR *     pTree;
    K2TREE_NODE *       pTreeNode;
    NETDEV_ARP_ENTRY *  pArpEntry;

    Debug_Printf("ArpTick()\n");
    pTree = &apNetDev->Proto.Arp.EntryTree;
    Debug_Printf("  %d Entries\n", pTree->mNodeCount);
    pTreeNode = K2TREE_FirstNode(pTree);
    if (NULL != pTreeNode)
    {
        do {
            pArpEntry = K2_GET_CONTAINER(NETDEV_ARP_ENTRY, pTreeNode, EntryTreeNode);
            pTreeNode = K2TREE_NextNode(pTree, pTreeNode);

            Debug_Printf("  %d.%d.%d.%d  %02X-%02X-%02X-%02X-%02X-%02X %d\n",
                pArpEntry->EntryTreeNode.mUserVal & 0xFF,
                (pArpEntry->EntryTreeNode.mUserVal >> 8) & 0xFF,
                (pArpEntry->EntryTreeNode.mUserVal >> 16) & 0xFF,
                (pArpEntry->EntryTreeNode.mUserVal >> 24) & 0xFF,
                pArpEntry->mHwAddr[0],
                pArpEntry->mHwAddr[1],
                pArpEntry->mHwAddr[2],
                pArpEntry->mHwAddr[3],
                pArpEntry->mHwAddr[4],
                pArpEntry->mHwAddr[5],
                pArpEntry->mMsTTL
            );

        } while (NULL != pTreeNode);
    }
#endif

#if 0

    UINT8 dnsUdp[UDP_HDR_LENGTH + 48];
    UINT8 *pDns;
    UINT16 u16;
    pDns = &dnsUdp[UDP_HDR_LENGTH];

    u16 = 0x1a24;
    K2MEM_Copy(&pDns[DNS_HDR_OFFSET_IDENT], &u16, sizeof(UINT16));
    u16 = K2_SWAP16((DNS_OPCODE_QUERY << DNS_FLAGCODE_OPCODE_SHL) | DNS_FLAGCODE_REC_DESIRED);
    K2MEM_Copy(&pDns[DNS_HDR_OFFSET_FLAGCODE_HI], &u16, sizeof(UINT16));
    u16 = K2_SWAP16(1);
    K2MEM_Copy(&pDns[DNS_HDR_OFFSET_QUEST_COUNT], &u16, sizeof(UINT16));
    K2MEM_Set(&pDns[DNS_HDR_OFFSET_ANS_COUNT], 0, sizeof(UINT16));
    K2MEM_Set(&pDns[DNS_HDR_OFFSET_NSREC_COUNT], 0, sizeof(UINT16));
    K2MEM_Set(&pDns[DNS_HDR_OFFSET_ADD_COUNT], 0, sizeof(UINT16));

    UINT8 const sQueryStr[] = { 3, 'w', 'w', 'w', 6, 'g', 'o', 'o','g', 'l', 'e', 3, 'c','o','m', 0 };
    UINT32 offset = DNS_HDR_OFFSET_QUEST1 + sizeof(sQueryStr);

    K2MEM_Copy(&pDns[DNS_HDR_OFFSET_QUEST1], sQueryStr, sizeof(sQueryStr));
    u16 = K2_SWAP16(DNS_RRTYPE_ADDRESS);
    K2MEM_Copy(&pDns[offset], &u16, sizeof(UINT16));
    offset += sizeof(UINT16);
    u16 = K2_SWAP16(DNS_RRCLASS_INET);
    K2MEM_Copy(&pDns[offset], &u16, sizeof(UINT16));
    offset += sizeof(UINT16);

    UINT32 targetIp = apNetDev->Proto.Ip.Adapter.Current.Host.mPrimaryDNS;

    if (!NetDev_Udp_Send(apNetDev, &targetIp, 1030, UDP_PORT_DNS, 0, dnsUdp, offset + UDP_HDR_LENGTH))
    {
        Debug_Printf("Send of DNS query failed\n");
    }
#endif

}

void 
NetDev_Arp_OnStart(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.Arp.mpTimer = NetDev_AddTimer(apNetDev, 1000, NetDev_Arp_Tick);
}

void
NetDev_Arp_Dequeue(
    NETDEV *            apNetDev,
    NETDEV_ARP_ENTRY *  apEntry
)
{
    NETDEV_BUFFER *     pBuf;
    NETDEV_BUFFER *     pSend;
    NETDEV_L2_PROTO *   pL2;

    pBuf = apEntry->mpQueueHead;
    if (NULL == pBuf)
        return;

    pL2 = apNetDev->Proto.mpL2;

    pSend = pBuf;
    do {
        pBuf = pBuf->mpNext;
        if (!pL2->Iface.SendBuffer(apNetDev, apEntry->mHwAddr, apNetDev->Proto.Ip.mL2ProtoId, pSend))
        {
            NetDev_RelBuffer(pSend);
        }
        pSend = pBuf;
    } while (NULL != pSend);

    apEntry->mpQueueHead = apEntry->mpQueueTail = NULL;
}

void
NetDev_Arp_Update(
    NETDEV *        apNetDev,
    UINT32          aIpAddr,
    UINT8 const *   apHwAddr
)
{
    K2TREE_NODE *       pTreeNode;
    NETDEV_ARP_ENTRY *  pArpEntry;
    UINT32              entryBytes;
    K2TREE_ANCHOR *     pTree;
    UINT32              hwLen;

    hwLen = apNetDev->Desc.Addr.mLen;
    pTree = &apNetDev->Proto.Arp.EntryTree;

    pTreeNode = K2TREE_Find(pTree, aIpAddr);
    if (NULL != pTreeNode)
    {
        //
        // existing entry
        //
        pArpEntry = K2_GET_CONTAINER(NETDEV_ARP_ENTRY, pTreeNode, EntryTreeNode);
        K2MEM_Copy(pArpEntry->mHwAddr, apHwAddr, hwLen);
        pArpEntry->mMsTTL = apNetDev->Proto.Arp.mMsInitialTTL;
        if (NULL != pArpEntry->mpQueueHead)
        {
            NetDev_Arp_Dequeue(apNetDev, pArpEntry);
        }
        return;
    }

    //
    // new entry
    //
    entryBytes = (sizeof(NETDEV_ARP_ENTRY) - 4) + hwLen;

    pArpEntry = (NETDEV_ARP_ENTRY *)K2OS_Heap_Alloc(entryBytes);
    if (NULL == pArpEntry)
        return;

    pArpEntry->EntryTreeNode.mUserVal = aIpAddr;
    pArpEntry->mMsTTL = apNetDev->Proto.Arp.mMsInitialTTL;
    K2MEM_Copy(&pArpEntry->mHwAddr, apHwAddr, hwLen);
    pArpEntry->mpQueueHead = NULL;

    K2TREE_Insert(pTree, aIpAddr, &pArpEntry->EntryTreeNode);
}

void 
NetDev_Arp_OnRecv(
    NETDEV *        apNetDev,
    UINT8 const *   apFromL2Addr,
    UINT8 const *   apData,
    UINT32          aDataLen
)
{
    UINT16              hwLen;
    UINT32              arpLen;
    UINT16              u16;
    NETDEV_BUFFER *     pSendBuffer;
    UINT8 const *       arp_pSHA;
    UINT32              arp_SPA;
    UINT8 const *       arp_pTHA;
    UINT32              arp_TPA;
    NETDEV_L2_PROTO *   pL2;
    UINT8 *             pOutBuf;
    UINT32              mtuL2;
    UINT32              ourIp;
    UINT32              bcastIp;

    pL2 = apNetDev->Proto.mpL2;
    hwLen = apNetDev->Desc.Addr.mLen;
    arpLen = ARP_PACKET_OFFSET_SHA + (2 * hwLen) + (2 * sizeof(UINT32));

#if 0
    Debug_Printf("ARPRECV dataLen = %d; arpLen = %d\n", aDataLen, arpLen);
    for (ourIp = 0; ourIp < arpLen; ourIp++)
    {
        Debug_Printf("%02X ", apData[ourIp]);
        if (15 == (ourIp & 15))
        {
            Debug_Printf("\n");
        }
    }
    if (0 != (ourIp & 15))
    {
        Debug_Printf("\n");
    }
#endif

    if (aDataLen < arpLen)
        return;

    K2MEM_Copy(&u16, &apData[ARP_PACKET_OFFSET_HTYPE], sizeof(u16));
    u16 = K2_SWAP16(u16);
    if (u16 != pL2->mHTYPE)
        return;

    K2MEM_Copy(&u16, &apData[ARP_PACKET_OFFSET_PTYPE], sizeof(u16));
    u16 = K2_SWAP16(u16);
    if ((u16 != apNetDev->Proto.Ip.mL2ProtoId) ||
        (apData[ARP_PACKET_OFFSET_HLEN] != hwLen) ||
        (apData[ARP_PACKET_OFFSET_PLEN] != sizeof(UINT32)))
        return;

    //
    // it goes SHA, SPA, THA, TPA 
    //
    arp_pSHA = apData + ARP_PACKET_OFFSET_SHA;
    K2MEM_Copy(&arp_SPA, arp_pSHA + hwLen, sizeof(UINT32));
    arp_pTHA = apData + ARP_PACKET_OFFSET_SHA + hwLen + 4;
    K2MEM_Copy(&arp_TPA, arp_pTHA + hwLen, sizeof(UINT32));

    K2MEM_Copy(&u16, &apData[ARP_PACKET_OFFSET_OPER], sizeof(UINT16));
    u16 = K2_SWAP16(u16);

    NetDev_Ip_GetCurrent(apNetDev, &ourIp, &bcastIp, NULL);

    if ((arp_SPA != bcastIp) && (arp_SPA != 0))
    {
        NetDev_Arp_Update(apNetDev, arp_SPA, arp_pSHA);
    }

    if (ARP_OPER_REPLY == u16)
    {
        //
        // received arp reply.  
        //
        return;
    }

    if (ARP_OPER_REQUEST != u16)
    {
        // dont know what this is
        return;
    }

    //
    // received arp request. is this for us?
    //
    if ((arp_TPA == bcastIp) ||
        (arp_TPA == 0) ||
        (arp_TPA != ourIp))
        return;

    //
    // ARP request is for us, so we need to send a reply
    //
//    Debug_Printf("---------SENDING ARP RESPONSE----------\n");
    pSendBuffer = pL2->Iface.AcqSendBuffer(apNetDev, &mtuL2);
    if (NULL == pSendBuffer)
        return;

    if (mtuL2 < arpLen)
    {
        NetDev_RelBuffer(pSendBuffer);
        return;
    }

    pOutBuf = pSendBuffer->mpData;
    K2_ASSERT(pSendBuffer->mDataLen >= arpLen);

    u16 = (UINT16)pL2->mHTYPE;
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pOutBuf[ARP_PACKET_OFFSET_HTYPE], &u16, sizeof(UINT16));

    u16 = (UINT16)apNetDev->Proto.Ip.mL2ProtoId;
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pOutBuf[ARP_PACKET_OFFSET_PTYPE], &u16, sizeof(UINT16));

    pOutBuf[ARP_PACKET_OFFSET_HLEN] = hwLen;
    pOutBuf[ARP_PACKET_OFFSET_PLEN] = sizeof(UINT32);
    u16 = K2_SWAP16(ARP_OPER_REPLY);
    K2MEM_Copy(&pOutBuf[ARP_PACKET_OFFSET_OPER], &u16, sizeof(UINT16));

    // SHA
    K2MEM_Copy(&pOutBuf[ARP_PACKET_OFFSET_SHA], apNetDev->Desc.Addr.mValue, hwLen);
    // SPA
    K2MEM_Copy(&pOutBuf[ARP_PACKET_OFFSET_SHA + hwLen], &arp_TPA, sizeof(UINT32));
    // THA
    K2MEM_Copy(&pOutBuf[ARP_PACKET_OFFSET_SHA + hwLen + sizeof(UINT32)], arp_pSHA, hwLen);
    // TPA
    K2MEM_Copy(&pOutBuf[ARP_PACKET_OFFSET_SHA + (2 * hwLen) + sizeof(UINT32)], &arp_SPA, sizeof(UINT32));

    pSendBuffer->mDataLen = arpLen;

    if (!pL2->Iface.SendBuffer(apNetDev, pL2->mHwBroadcastAddr, apNetDev->Proto.Arp.mL2ProtoId, pSendBuffer))
    {
        NetDev_RelBuffer(pSendBuffer);
    }
}

void
NetDev_Arp_OnTimeExpired(
    NETDEV *    apNetDev,
    UINT32      aExpiredMs
)
{
    K2TREE_NODE *       pTreeNode;
    NETDEV_ARP_ENTRY *  pArpEntry;
    K2TREE_ANCHOR *     pTree;
    NETDEV_BUFFER *     pPrev;
    NETDEV_BUFFER *     pBuffer;
    NETDEV_BUFFER *     pKill;

    pTree = &apNetDev->Proto.Arp.EntryTree;

    pTreeNode = K2TREE_FirstNode(pTree);
    if (NULL != pTreeNode)
    {
        do {
            pArpEntry = K2_GET_CONTAINER(NETDEV_ARP_ENTRY, pTreeNode, EntryTreeNode);
            pTreeNode = K2TREE_NextNode(pTree, pTreeNode);

            pBuffer = pArpEntry->mpQueueHead;
            if (NULL != pBuffer)
            {
                pPrev = NULL;
                do {
                    if (pBuffer->mMsTTL <= aExpiredMs)
                    {
                        pKill = pBuffer;
                        pBuffer = pBuffer->mpNext;
                        if (NULL == pPrev)
                        {
                            pArpEntry->mpQueueHead = pKill->mpNext;
                        }
                        else
                        {
                            pPrev->mpNext = pKill->mpNext;
                        }
                        if (NULL == pBuffer)
                        {
                            pArpEntry->mpQueueTail = pPrev;
                        }
                        NetDev_RelBuffer(pKill);
                    }
                    else
                    {
                        pBuffer->mMsTTL -= aExpiredMs;
                        pPrev = pBuffer;
                        pBuffer = pBuffer->mpNext;
                    }
                } while (NULL != pBuffer);
            }

            if (aExpiredMs >= pArpEntry->mMsTTL)
            {
                // entry aged out
                K2_ASSERT(NULL == pArpEntry->mpQueueHead);
                K2TREE_Remove(pTree, &pArpEntry->EntryTreeNode);
                K2OS_Heap_Free(pArpEntry);
            }
            else
            {
                pArpEntry->mMsTTL -= aExpiredMs;
            }
        } while (NULL != pTreeNode);
    }
}

void 
NetDev_Arp_OnStop(
    NETDEV * apNetDev
)
{
    if (NULL != apNetDev->Proto.Arp.mpTimer)
    {
        NetDev_DelTimer(apNetDev, apNetDev->Proto.Arp.mpTimer);
        apNetDev->Proto.Arp.mpTimer = NULL;
    }
}

BOOL
NetDev_Arp_Resolve(
    NETDEV *        apNetDev,
    UINT32          aIpAddr,
    UINT8 const **  appRetHwAddr,
    BOOL            aNoSend
)
{
    K2TREE_NODE *       pTreeNode;
    NETDEV_ARP_ENTRY *  pArpEntry;
    NETDEV_BUFFER *     pBuffer;
    NETDEV_L2_PROTO *   pL2;
    UINT8 *             pData;
    UINT16              u16;
    UINT32              hLen;
    UINT32              ourIp;

    pL2 = apNetDev->Proto.mpL2;

    pTreeNode = K2TREE_Find(&apNetDev->Proto.Arp.EntryTree, aIpAddr);
    if (NULL == pTreeNode)
    {
        if (!aNoSend)
        {
            // 
            // send ARP request here
            //
            pBuffer = pL2->Iface.AcqSendBuffer(apNetDev, NULL);
            if (NULL == pBuffer)
                return FALSE;

            if (NULL == appRetHwAddr)
            {
                ourIp = 0;
            }
            else
            {
                NetDev_Ip_GetCurrent(apNetDev, &ourIp, NULL, NULL);
            }

            pData = pBuffer->mpData;

            u16 = pL2->mHTYPE;
            u16 = K2_SWAP16(u16);
            K2MEM_Copy(&pData[ARP_PACKET_OFFSET_HTYPE], &u16, sizeof(UINT16));

            u16 = apNetDev->Proto.Ip.mL2ProtoId;
            u16 = K2_SWAP16(u16);
            K2MEM_Copy(&pData[ARP_PACKET_OFFSET_PTYPE], &u16, sizeof(UINT16));

            hLen = apNetDev->Desc.Addr.mLen;
            pData[ARP_PACKET_OFFSET_HLEN] = (UINT8)hLen;
            pData[ARP_PACKET_OFFSET_PLEN] = 4;

            u16 = K2_SWAP16(ARP_OPER_REQUEST);
            K2MEM_Copy(&pData[ARP_PACKET_OFFSET_OPER], &u16, sizeof(UINT16));

            K2MEM_Copy(&pData[ARP_PACKET_OFFSET_SHA], apNetDev->Desc.Addr.mValue, hLen);
            K2MEM_Copy(&pData[ARP_PACKET_OFFSET_SHA + hLen], &ourIp, 4);
            K2MEM_Copy(&pData[ARP_PACKET_OFFSET_SHA + hLen + 4], pL2->mHwBroadcastAddr, hLen);
            K2MEM_Copy(&pData[ARP_PACKET_OFFSET_SHA + (hLen * 2) + 4], &aIpAddr, 4);

            pBuffer->mDataLen = ARP_PACKET_OFFSET_SHA + (hLen * 2) + 8;

            if (!pL2->Iface.SendBuffer(apNetDev, pL2->mHwBroadcastAddr, apNetDev->Proto.Arp.mL2ProtoId, pBuffer))
            {
                NetDev_RelBuffer(pBuffer);
            }
        }

        return FALSE;
    }

    pArpEntry = K2_GET_CONTAINER(NETDEV_ARP_ENTRY, pTreeNode, EntryTreeNode);

    if (NULL != pArpEntry->mpQueueHead)
    {
        //
        // ARP request is outstanding already
        //
        return FALSE;
    }

    if (NULL != appRetHwAddr)
    {
        *appRetHwAddr = pArpEntry->mHwAddr;
    }
    
    return TRUE;
}

void    
NetDev_Arp_Clear(
    NETDEV *apNetDev,
    UINT32  aIpAddr
)
{
    K2TREE_NODE *       pTreeNode;
    NETDEV_ARP_ENTRY *  pEntry;
    NETDEV_BUFFER *     pBuffer;

    pTreeNode = K2TREE_Find(&apNetDev->Proto.Arp.EntryTree, aIpAddr);
    if (NULL == pTreeNode)
        return;

    K2TREE_Remove(&apNetDev->Proto.Arp.EntryTree, pTreeNode);

    pEntry = K2_GET_CONTAINER(NETDEV_ARP_ENTRY, pTreeNode, EntryTreeNode);
    while (NULL != pEntry->mpQueueHead)
    {
        pBuffer = pEntry->mpQueueHead;
        pEntry->mpQueueHead = pBuffer->mpNext;
        NetDev_RelBuffer(pBuffer);
    }
    pEntry->mpQueueTail = NULL;
    K2OS_Heap_Free(pEntry);
}

BOOL
NetDev_Arp_Queue(
    NETDEV *        apNetDev,
    UINT32          aTargetIpAddr,
    NETDEV_BUFFER * apBuffer
)
{
    K2TREE_NODE *       pTreeNode;
    NETDEV_ARP_ENTRY *  pArpEntry;
    UINT32              entBytes;
    UINT32              bcastIp;


    K2_ASSERT(aTargetIpAddr != 0);
    K2_ASSERT(aTargetIpAddr != 0xFFFFFFFF);
    K2_ASSERT(apBuffer->mpNetDev == apNetDev);

    NetDev_Ip_GetCurrent(apNetDev, NULL, &bcastIp, NULL);

    K2_ASSERT(aTargetIpAddr != bcastIp);

    pTreeNode = K2TREE_Find(&apNetDev->Proto.Arp.EntryTree, aTargetIpAddr);
    if (NULL == pTreeNode)
    {
        entBytes = sizeof(NETDEV_ARP_ENTRY) - 4 + apNetDev->Desc.Addr.mLen;
        pArpEntry = (NETDEV_ARP_ENTRY *)K2OS_Heap_Alloc(entBytes);
        if (NULL == pArpEntry)
            return FALSE;
        K2MEM_Zero(pArpEntry, entBytes);
        pArpEntry->EntryTreeNode.mUserVal = aTargetIpAddr;
        K2TREE_Insert(&apNetDev->Proto.Arp.EntryTree, aTargetIpAddr, &pArpEntry->EntryTreeNode);
    }
    else
    {
        pArpEntry = K2_GET_CONTAINER(NETDEV_ARP_ENTRY, pTreeNode, EntryTreeNode);
    }

    K2_ASSERT(K2MEM_VerifyZero(pArpEntry->mHwAddr, apBuffer->mpNetDev->Desc.Addr.mLen));

    apBuffer->mpNext = NULL;
    if (NULL == pArpEntry->mpQueueHead)
    {
        K2_ASSERT(NULL == pArpEntry->mpQueueTail);
        pArpEntry->mpQueueHead = apBuffer;
    }
    else
    {
        pArpEntry->mpQueueTail->mpNext = apBuffer;
    }
    pArpEntry->mpQueueTail = apBuffer;

    apBuffer->mMsTTL = 5 * 1000;    // five seconds to get response to ARP

    if (pArpEntry->mMsTTL < apBuffer->mMsTTL)
        pArpEntry->mMsTTL = apBuffer->mMsTTL;

    return TRUE;
}

void 
NetDev_Arp_Deinit(
    NETDEV * apNetDev
)
{
    K2TREE_NODE *       pTreeNode;
    NETDEV_ARP_ENTRY *  pEntry;
    NETDEV_BUFFER *     pBuffer;

    pTreeNode = K2TREE_FirstNode(&apNetDev->Proto.Arp.EntryTree);
    if (NULL == pTreeNode)
        return;

    do {
        pEntry = K2_GET_CONTAINER(NETDEV_ARP_ENTRY, pTreeNode, EntryTreeNode);
        pTreeNode = K2TREE_NextNode(&apNetDev->Proto.Arp.EntryTree, pTreeNode);
        K2TREE_Remove(&apNetDev->Proto.Arp.EntryTree, &pEntry->EntryTreeNode);
        while (NULL != pEntry->mpQueueHead)
        {
            pBuffer = pEntry->mpQueueHead;
            pEntry->mpQueueHead = pBuffer->mpNext;
            NetDev_RelBuffer(pBuffer);
        }
        pEntry->mpQueueTail = NULL;
        K2OS_Heap_Free(pEntry);
    } while (NULL != pTreeNode);
}

BOOL
NetDev_Arp_Init(
    NETDEV *apNetDev,
    UINT32  aL2ProtoId
)
{
    K2MEM_Zero(&apNetDev->Proto.Arp, sizeof(NETDEV_ARP_PROTO));

    K2TREE_Init(&apNetDev->Proto.Arp.EntryTree, NULL);

    apNetDev->Proto.Arp.mL2ProtoId = aL2ProtoId;
    apNetDev->Proto.Arp.mMsInitialTTL = (60 * 1000); // one minute

    return TRUE;
}


