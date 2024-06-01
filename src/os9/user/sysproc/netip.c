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

typedef struct _IPPACK_TARGET IPPACK_TARGET;
struct _IPPACK_TARGET
{
    UINT32  mSrcIp;
    UINT16  mIdent;
    UINT16  mAlign;
};

typedef struct _IPPACK_HOLE IPPACK_HOLE;
struct _IPPACK_HOLE
{
    UINT32  mLength;
    UINT32  mNextAddr;
};
K2_STATIC_ASSERT(sizeof(IPPACK_HOLE) <= 8);

typedef struct _IPPACK_MF IPPACK_MF;
struct _IPPACK_MF
{
    K2TREE_NODE     TreeNode;
    IPPACK_TARGET   Target;
    UINT32          mTotLen;
    UINT32          mMsTTL;
    UINT32          mFirstHoleAddr;
    UINT8           mData[8];
};

UINT16  
NetDev_Ip_CalcHdrChecksum(
    UINT16 *apIpHdr
)
{
    UINT32 result;
    UINT32 len;
    UINT16 u16;

    //
    // make number of 16-bit words after checksum field needed to include 
    // in the header calculation
    //
    len = (((UINT32)(apIpHdr[IPV4_HDR_OFFSET_VERSION_IHL] & 0xF)) * 2) - 6;

    //
    // ones complement addition with carry to lowest bit on overflow
    //
    u16 = (*apIpHdr); apIpHdr++;
    result = K2_SWAP16(u16);

    u16 = (*apIpHdr); apIpHdr++;
    result += K2_SWAP16(u16);

    u16 = (*apIpHdr); apIpHdr++;
    result += K2_SWAP16(u16);

    u16 = (*apIpHdr); apIpHdr++;
    result += K2_SWAP16(u16);

    u16 = (*apIpHdr); apIpHdr++;
    result += K2_SWAP16(u16);

    apIpHdr++;    // hdr checksum field ignored

    do {
        u16 = (*apIpHdr); apIpHdr++;
        result += K2_SWAP16(u16);
    } while (--len);

    do {
        result = ((result >> 16) + result) & 0xFFFF;
    } while (0 != (result & 0xFFFF0000));

    //
    // complement is the value that should be in the header
    //
    return ~((UINT16)result);
}

void
NetDev_Ip_GetCurrent(
    NETDEV *    apNetDev,
    UINT32 *    apRetCurrentIp,
    UINT32 *    apRetCurrentBcastIp,
    UINT8 *     apRetCurrentTTL
)
{
    UINT32  bcastIp;
    UINT32  ourIp;
    UINT8   useTTL;

    if (!apNetDev->Proto.Ip.Adapter.Config.mUseDhcp)
    {
        bcastIp = apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mBroadcastAddr;
        ourIp = apNetDev->Proto.Ip.Adapter.Config.Static.Host.mIpAddress;
        useTTL = apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mInitialTTL;
    }
    else
    {
        if (apNetDev->Proto.Ip.Adapter.Current.Dhcp.mIsBound)
        {
            bcastIp = apNetDev->Proto.Ip.Adapter.Current.IpParam.mBroadcastAddr;
            ourIp = apNetDev->Proto.Ip.Adapter.Current.Host.mIpAddress;
            useTTL = apNetDev->Proto.Ip.Adapter.Current.IpParam.mInitialTTL;
        }
        else
        {
            bcastIp = apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mBroadcastAddr;
            ourIp = apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress;
            useTTL = apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mInitialTTL;
        }
    }

    if (NULL != apRetCurrentIp)
    {
        *apRetCurrentIp = ourIp;
    }
    if (NULL != apRetCurrentBcastIp)
    {
        *apRetCurrentBcastIp = bcastIp;
    }
    if (NULL != apRetCurrentTTL)
    {
        *apRetCurrentTTL = useTTL;
    }
}

void 
NetDev_Ip_OnStart(
    NETDEV * apNetDev
)
{
    NetDev_Icmp_OnStart(apNetDev);
    NetDev_Udp_OnStart(apNetDev);
    NetDev_Tcp_OnStart(apNetDev);
}

int
NetDev_Ip_MfTreeCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
)
{
    IPPACK_MF *     pNode;
    IPPACK_TARGET * pKey;

    pNode = K2_GET_CONTAINER(IPPACK_MF, apNode, TreeNode);
    pKey = (IPPACK_TARGET *)aKey;

    if (pNode->Target.mSrcIp != pKey->mSrcIp)
    {
        if (pNode->Target.mSrcIp < pKey->mSrcIp)
        {
            return 1;
        }
        return -1;
    }

    if (pNode->Target.mIdent == pKey->mIdent)
        return 0;

    if (pNode->Target.mIdent < pKey->mIdent)
    {
        return 1;
    }
    return -1;
}

void
NetDev_Ip_RecvMf(
    NETDEV *            apNetDev,
    NETDEV_L4_pf_OnRecv afOnRecv,
    UINT32              aFragOffsetBytes,
    UINT32              aSrcIpAddr,
    UINT32              aDstIpAddr,
    UINT8 const *       apData,
    UINT32              aPayloadLen, // data after the IP header
    BOOL                aIsLastFragment
)
{
    K2TREE_NODE *   pTreeNode;
    IPPACK_TARGET   target;
    IPPACK_MF *     pMf;
    IPPACK_HOLE *   pHole;
    UINT32          holeAddr;
    UINT32          fragAddr;
    UINT32          prevHole;
    UINT32          hdrLen;
    UINT32          dist;
    UINT32          holeLen;
    UINT32          nextHoleAddr;

    target.mSrcIp = aSrcIpAddr;
    K2MEM_Copy(&target.mIdent, &apData[IPV4_HDR_OFFSET_IDENT_HI], sizeof(UINT16));
    target.mIdent = K2_SWAP16(target.mIdent);

    pTreeNode = K2TREE_Find(&apNetDev->Proto.Ip.MfTree, (UINT_PTR)&target);
    if (NULL == pTreeNode)
    {
        pMf = (IPPACK_MF *)K2OS_Heap_Alloc((sizeof(IPPACK_MF) - 8) + IPV4_MAX_PACKET_LENGTH - IPV4_HDR_STD_LENGTH);
        if (NULL == pMf)
            return;

        pMf->Target.mSrcIp = target.mSrcIp;
        pMf->Target.mIdent = target.mIdent;
        pMf->mMsTTL = apNetDev->Proto.Ip.mMsInitialMfTTL;
        pMf->mTotLen = 0;
        pHole = (IPPACK_HOLE *)&pMf->mData;
        pHole->mNextAddr = 0;
        pHole->mLength = IPV4_MAX_PACKET_LENGTH - IPV4_HDR_STD_LENGTH;
        pMf->mFirstHoleAddr = (UINT32)&pMf->mData[0];
        K2TREE_Insert(&apNetDev->Proto.Ip.MfTree, (UINT_PTR)(&pMf->Target), &pMf->TreeNode);
    }
    else
    {
        pMf = K2_GET_CONTAINER(IPPACK_MF, pTreeNode, TreeNode);
    }

    if (aIsLastFragment)
    {
        K2_ASSERT(0 == pMf->mTotLen);
        pMf->mTotLen = aFragOffsetBytes + aPayloadLen;

        // find the last hole and truncate it to be the length of the overall packet
        holeAddr = pMf->mFirstHoleAddr;
        K2_ASSERT(0 != holeAddr);
        do {
            nextHoleAddr = ((IPPACK_HOLE *)holeAddr)->mNextAddr;
            if (0 == nextHoleAddr)
                break;
            holeAddr = nextHoleAddr;
        } while (1);

        ((IPPACK_HOLE *)holeAddr)->mLength = ((UINT32)(&pMf->mData[pMf->mTotLen])) - holeAddr;
    }

    hdrLen = ((apData[IPV4_HDR_OFFSET_VERSION_IHL] & 0xF) * 4);
    apData += hdrLen;

    // find the hole that this fragment goes into
    fragAddr = (UINT32)&pMf->mData[aFragOffsetBytes];
    prevHole = 0;
    holeAddr = pMf->mFirstHoleAddr;
    do {
        holeLen = ((IPPACK_HOLE *)holeAddr)->mLength;
        nextHoleAddr = ((IPPACK_HOLE *)holeAddr)->mNextAddr;

        if (fragAddr < holeAddr)
        {
            dist = holeAddr - fragAddr;
            if (dist >= aPayloadLen)
            {
                // fragment comes all before the hole and does not intersect
                return;
            }

            // eat data we already have 
            apData += dist;
            aPayloadLen -= dist;
            fragAddr += dist;
            K2_ASSERT(fragAddr == holeAddr);
            K2_ASSERT(aPayloadLen > 0);

            if (aPayloadLen < holeLen)
            {
                // fragment doesn't fill the hole
                K2MEM_Copy((void *)holeAddr, apData, aPayloadLen);
                holeAddr += aPayloadLen;
                if (0 == prevHole)
                {
                    pMf->mFirstHoleAddr = holeAddr;
                }
                else
                {
                    ((IPPACK_HOLE *)prevHole)->mNextAddr = holeAddr;
                }
                ((IPPACK_HOLE *)holeAddr)->mLength = holeLen - aPayloadLen;
                ((IPPACK_HOLE *)holeAddr)->mNextAddr = nextHoleAddr;
                return;
            }

            // fragment fills the hole
            K2MEM_Copy((void *)holeAddr, apData, holeLen);
            apData += holeLen;
            aPayloadLen -= holeLen;
            holeAddr = nextHoleAddr;
            if (0 == prevHole)
            {
                pMf->mFirstHoleAddr = nextHoleAddr;
            }
            else
            {
                ((IPPACK_HOLE *)prevHole)->mNextAddr = nextHoleAddr;
            }

            if (0 == aPayloadLen)
                break;
        }
        else if (fragAddr > holeAddr)
        {
            dist = fragAddr - holeAddr;
            prevHole = holeAddr;
            if (dist < holeLen)
            {
                // frag starts in the middle of a hole
                // so cut this into two holes
                holeAddr += dist;
                ((IPPACK_HOLE *)prevHole)->mLength = dist;
                ((IPPACK_HOLE *)prevHole)->mNextAddr = holeAddr;
                ((IPPACK_HOLE *)holeAddr)->mLength = holeLen - dist;
                ((IPPACK_HOLE *)holeAddr)->mNextAddr = nextHoleAddr;
            }
            else
            {
                // frag starts after the end of this hole
                holeAddr = nextHoleAddr;
            }
        }
        else
        {
            // fragAddr == holeAddr
            if (aPayloadLen < holeLen)
            {
                // fragment does not fully fill the hole
                if (0 == prevHole)
                {
                    pMf->mFirstHoleAddr = holeAddr + aPayloadLen;
                }
                else
                {
                    ((IPPACK_HOLE *)prevHole)->mNextAddr = holeAddr + aPayloadLen;
                }
                K2MEM_Copy((void *)holeAddr, apData, aPayloadLen);
                holeAddr += aPayloadLen;
                ((IPPACK_HOLE *)holeAddr)->mLength = holeLen - aPayloadLen;
                ((IPPACK_HOLE *)holeAddr)->mNextAddr = nextHoleAddr;
                return;
            }

            // fragment fully fills the hole
            if (0 == prevHole)
            {
                pMf->mFirstHoleAddr = nextHoleAddr;
            }
            else
            {
                ((IPPACK_HOLE *)prevHole)->mNextAddr = nextHoleAddr;
            }
            K2MEM_Copy((void *)holeAddr, apData, holeLen);
            apData += holeLen;
            aPayloadLen -= holeLen;
            holeAddr = nextHoleAddr;
        }

    } while (0 != holeAddr);

    if ((0 == pMf->mTotLen) ||
        (0 != pMf->mFirstHoleAddr))
        return;

    // 
    // entire reconstructed packet has arrived
    //
    K2TREE_Remove(&apNetDev->Proto.Ip.MfTree, &pMf->TreeNode);

    afOnRecv(apNetDev, aSrcIpAddr, aDstIpAddr, pMf->mData, pMf->mTotLen);

    K2OS_Heap_Free(pMf);
}

void 
NetDev_Ip_OnRecv(
    NETDEV *        apNetDev,
    UINT8 const *   apFromL2Addr,
    UINT8 const *   apData,
    UINT32          aDataLen
)
{
    UINT32              srcIp;
    UINT32              dstIp;
    UINT16              u16;
    UINT8               ipFlags;
    UINT8               ipProto;
    UINT8               hdrLen;
    UINT32              fragOff;
    UINT16              totLen;
    BOOL                lastFragment;
    NETDEV_L4_pf_OnRecv fRecv;

//    Debug_Printf("NetIp_Recv(%d)\n", aDataLen);

    if (aDataLen < IPV4_HDR_STD_LENGTH)
        return;

    K2MEM_Copy(&u16, &apData[IPV4_HDR_OFFSET_CHKSUM_HI], sizeof(UINT16));
    if (K2_SWAP16(u16) != NetDev_Ip_CalcHdrChecksum((UINT16 *)apData))
        return;

    K2MEM_Copy(&srcIp, &apData[IPV4_HDR_OFFSET_SRC_IPADDR], sizeof(UINT32));
    if (srcIp != 0)
    {
        NetDev_Arp_Update(apNetDev, srcIp, apFromL2Addr);
    }

    ipProto = apData[IPV4_HDR_OFFSET_PROTOCOL];

//    Debug_Printf("NetIo_Recv(%d,0x%02X)\n", aDataLen, ipProto);

    if (ipProto == IPV4_PROTO_TCP)
    {
        fRecv = NetDev_Tcp_OnRecv;
    }
    else if (ipProto == IPV4_PROTO_UDP)
    {
        fRecv = NetDev_Udp_OnRecv;
    }
    else if (ipProto == IPV4_PROTO_ICMP)
    {
        fRecv = NetDev_Icmp_OnRecv;
    }
    else
    {
        return;
    }

    K2MEM_Copy(&totLen, &apData[IPV4_HDR_OFFSET_TOTLEN_HI], sizeof(UINT16));
    totLen = K2_SWAP16(totLen);
    if (totLen > aDataLen)
        return;

    K2MEM_Copy(&dstIp, &apData[IPV4_HDR_OFFSET_DST_IPADDR], sizeof(UINT32));

    K2MEM_Copy(&u16, &apData[IPV4_HDR_OFFSET_FLAGS_FRAGOFF_HI], sizeof(UINT16));
    u16 = K2_SWAP32(u16);
    ipFlags = (u16 >> 8) & 0xE0;
    fragOff = ((UINT32)(u16 & 0x1FFF)) * 8;

    hdrLen = ((apData[IPV4_HDR_OFFSET_VERSION_IHL] & 0xF) * 4);

    lastFragment = (0 == (ipFlags & IPV4_HDR_FLAGS_MF)) ? TRUE : FALSE;
    if (!lastFragment)
    {
        if (0 != ((totLen - hdrLen) & 7))
        {
            Debug_Printf("*** payload of non-last-fragment of IP packet did not have a total length that is divisible by 8!\n");
            return;
        }
    }
    if ((0 != fragOff) || (!lastFragment))
    {
        // this is part of a multifragment packet
        NetDev_Ip_RecvMf(apNetDev, fRecv, fragOff, srcIp, dstIp, apData, totLen - hdrLen, lastFragment);
        return;
    }

    //
    // this is an unfragmented packet
    //
    fRecv(apNetDev, srcIp, dstIp, apData + hdrLen, totLen - hdrLen);
}

void
NetDev_Ip_OnTimeExpired(
    NETDEV *    apNetDev,
    UINT32      aExpiredMs
)
{
    K2TREE_ANCHOR * pTree;
    K2TREE_NODE *   pTreeNode;
    IPPACK_MF *     pMf;

    //
    // prune any MF packets that have timed out without being completely received
    //
    pTree = &apNetDev->Proto.Ip.MfTree;
    pTreeNode = K2TREE_FirstNode(pTree);
    if (NULL != pTreeNode)
    {
        do {
            pMf = K2_GET_CONTAINER(IPPACK_MF, pTreeNode, TreeNode);
            pTreeNode = K2TREE_NextNode(pTree, pTreeNode);
            if (pMf->mMsTTL <= aExpiredMs)
            {
                K2TREE_Remove(pTree, &pMf->TreeNode);
                K2OS_Heap_Free(pMf);
            }
            else
            {
                pMf->mMsTTL -= aExpiredMs;
            }
        } while (NULL != pTreeNode);
    }
}

void 
NetDev_Ip_OnStop(
    NETDEV * apNetDev
)
{
    NetDev_Tcp_OnStop(apNetDev);
    NetDev_Udp_OnStop(apNetDev);
    NetDev_Icmp_OnStop(apNetDev);
}

void 
NetDev_Ip_Deinit(
    NETDEV * apNetDev
)
{
    NetDev_Tcp_Deinit(apNetDev);
    NetDev_Udp_Deinit(apNetDev);
    NetDev_Icmp_Deinit(apNetDev);
}

BOOL
NetDev_Ip_Init(
    NETDEV *                        apNetDev,
    UINT32                          aL2ProtoId,
    K2OS_IPV4_ADAPTER_CONFIG const *apConfig
)
{
    BOOL result;

    K2_ASSERT(apNetDev->Proto.mpL2->mClientMTU >= IPV4_MIN_PACKET_LENGTH);

    K2MEM_Zero(&apNetDev->Proto.Ip, sizeof(NETDEV_IP_PROTO));
    
    apNetDev->Proto.Ip.mL2ProtoId = aL2ProtoId;
    apNetDev->Proto.Ip.mNextIdent = 1;
    apNetDev->Proto.Ip.mMsInitialMfTTL = 20 * 1000; // 20 seconds
    
    K2TREE_Init(&apNetDev->Proto.Ip.MfTree, NetDev_Ip_MfTreeCompare);

    K2MEM_Copy(&apNetDev->Proto.Ip.Adapter.Config, apConfig, sizeof(K2OS_IPV4_ADAPTER_CONFIG));

    if (0 == apNetDev->Proto.Ip.Adapter.Config.mHostName[0])
    {
        K2ASC_Copy(apNetDev->Proto.Ip.Adapter.Config.mHostName, "host");
    }
    if (!apNetDev->Proto.Ip.Adapter.Config.mUseDhcp)
    {
        if (0 == apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mBroadcastAddr)
        {
            apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mBroadcastAddr = 0xFFFFFFFF;
        }
        if (0 == apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mInitialTTL)
        {
            apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mInitialTTL = K2OS_IPV4_DEFAULT_IP_TTL_VALUE;
        }
        if (0 == apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mInterfaceMTU)
        {
            apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mInterfaceMTU = apNetDev->Proto.mpL2->mClientMTU;
        }
    }
    apNetDev->Proto.Ip.Adapter.mNetIo_IfInstanceId = apNetDev->mIfInstId;
    apNetDev->Proto.Ip.Adapter.mIsOperating = TRUE;

    result = FALSE;
    if (NetDev_Icmp_Init(apNetDev))
    {
        if (NetDev_Udp_Init(apNetDev))
        {
            if (NetDev_Tcp_Init(apNetDev))
            {
                result = TRUE;
            }
            else
            {
                NetDev_Udp_Deinit(apNetDev);
            }
        }
        if (!result)
        {
            NetDev_Icmp_Deinit(apNetDev);
        }
    }

    return result;
}

typedef struct _IP_FRAGSEND IP_FRAGSEND;
struct _IP_FRAGSEND
{
    UINT32              mTargetIp;
    UINT32              mOurIp;
    NETDEV_BUFFER *     mpBuffer;
    UINT16              mIdent;
    UINT8               mFlags;
    UINT8               mUseTTL;
    UINT8               mIpProto;
    BOOL                mResolveOk;
    UINT32              mFragOffsetBytes;
    UINT8 const *       mpHwAddr;
};

BOOL
NetDev_Ip_SendFrag(
    NETDEV *            apNetDev,
    IP_FRAGSEND *       apFragSend
)
{
    NETDEV_BUFFER *     pBuffer;
    UINT8 *             pIpHdr;
    UINT16              u16;
    NETDEV_L2_PROTO *   pL2;
    BOOL                ok;

    pL2 = apNetDev->Proto.mpL2;
    pBuffer = apFragSend->mpBuffer;
    pIpHdr = pBuffer->mpData;

    pIpHdr[IPV4_HDR_OFFSET_VERSION_IHL] = 0x45;

    pIpHdr[IPV4_HDR_OFFSET_DSCP_ECN] = 0;

    u16 = apFragSend->mIdent;
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pIpHdr[IPV4_HDR_OFFSET_IDENT_HI], &u16, sizeof(UINT16));

    K2_ASSERT(0 == (apFragSend->mFragOffsetBytes & 7));
    u16 = (UINT16)(apFragSend->mFragOffsetBytes / 8);
    u16 |= ((UINT16)apFragSend->mFlags) << 8;
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pIpHdr[IPV4_HDR_OFFSET_FLAGS_FRAGOFF_HI], &u16, sizeof(UINT16));

    pIpHdr[IPV4_HDR_OFFSET_TTL] = apFragSend->mUseTTL;

    pIpHdr[IPV4_HDR_OFFSET_PROTOCOL] = apFragSend->mIpProto;

    K2MEM_Copy(&pIpHdr[IPV4_HDR_OFFSET_SRC_IPADDR], &apFragSend->mOurIp, sizeof(UINT32));
    K2MEM_Copy(&pIpHdr[IPV4_HDR_OFFSET_DST_IPADDR], &apFragSend->mTargetIp, sizeof(UINT32));

    u16 = (UINT16)pBuffer->mDataLen;
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pIpHdr[IPV4_HDR_OFFSET_TOTLEN_HI], &u16, sizeof(UINT16));

    u16 = NetDev_Ip_CalcHdrChecksum((UINT16 *)pIpHdr);
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pIpHdr[IPV4_HDR_OFFSET_CHKSUM_HI], &u16, sizeof(UINT16));

    if (!apFragSend->mResolveOk)
    {
        ok = NetDev_Arp_Queue(apNetDev, apFragSend->mTargetIp, pBuffer);
    }
    else
    {
        ok = pL2->Iface.SendBuffer(apNetDev, apFragSend->mpHwAddr, apNetDev->Proto.Ip.mL2ProtoId, pBuffer);
    }

    if (!ok)
    {
        NetDev_RelBuffer(pBuffer);
    }

    return ok;
}

BOOL
NetDev_Ip_Send(
    NETDEV *        apNetDev,
    UINT32 const *  apTargetIp,
    UINT8           aTargetIpProto,
    BOOL            aDoNotFragment,
    UINT8 const *   apData,
    UINT32          aDataLen
)
{
    NETDEV_L2_PROTO *   pL2;
    UINT32              parentMTU;
    UINT32              ixFrag;
    UINT32              numFrags;
    UINT32              fragBytes;
    IP_FRAGSEND         fragSend;
    UINT32              bcastIp;

    pL2 = apNetDev->Proto.mpL2;
    parentMTU = pL2->mClientMTU;

    NetDev_Ip_GetCurrent(apNetDev, &fragSend.mOurIp, &bcastIp, &fragSend.mUseTTL);

    if (NULL != apTargetIp)
    {
        fragSend.mTargetIp = *apTargetIp;
        if (0 == fragSend.mTargetIp)
            return FALSE;
        if (0xFFFFFFFF == fragSend.mTargetIp)
            fragSend.mTargetIp = bcastIp;
    }
    else
    {
        fragSend.mTargetIp = bcastIp;
    }

    fragSend.mIdent = apNetDev->Proto.Ip.mNextIdent++;

    fragSend.mIpProto = aTargetIpProto;

    if (fragSend.mTargetIp != bcastIp)
    {
        fragSend.mResolveOk = NetDev_Arp_Resolve(apNetDev, NetDev_Ip_GetRoute(apNetDev, fragSend.mTargetIp), &fragSend.mpHwAddr, FALSE);
    }
    else
    {
        // broadcast on L2
        fragSend.mResolveOk = TRUE;
        fragSend.mpHwAddr = pL2->mHwBroadcastAddr;
    }

    if (aDataLen <= parentMTU)
    {
        fragSend.mpBuffer = pL2->Iface.AcqSendBuffer(apNetDev, NULL);
        if (NULL == fragSend.mpBuffer)
            return FALSE;
        fragSend.mFlags = aDoNotFragment ? IPV4_HDR_FLAGS_DF : 0;
        fragSend.mFragOffsetBytes = 0;

        K2_ASSERT(fragSend.mpBuffer->mDataLen >= aDataLen + IPV4_HDR_STD_LENGTH);
        K2MEM_Copy(fragSend.mpBuffer->mpData + IPV4_HDR_STD_LENGTH, apData, aDataLen);
        fragSend.mpBuffer->mDataLen = IPV4_HDR_STD_LENGTH + aDataLen;

        return NetDev_Ip_SendFrag(apNetDev, &fragSend);
    }

    // need to fragment

    if (aDoNotFragment)
    {
        return FALSE;
    }

    // fragments need to be multiple of 8 bytes except for the last one
    parentMTU &= ~7;

    numFrags = aDataLen / parentMTU;
    if ((aDataLen - (numFrags * parentMTU)) != 0)
        numFrags++;

    ixFrag = 0;
    fragSend.mFlags = IPV4_HDR_FLAGS_MF;
    fragSend.mFragOffsetBytes = 0;
    do {
        fragSend.mpBuffer = pL2->Iface.AcqSendBuffer(apNetDev, NULL);
        if (NULL == fragSend.mpBuffer)
            break;

        if (aDataLen > parentMTU)
        {
            fragBytes = parentMTU;
        }
        else
        {
            fragSend.mFlags = 0;
            fragBytes = aDataLen;
        }

        K2MEM_Copy(fragSend.mpBuffer->mpData + IPV4_HDR_STD_LENGTH, apData, fragBytes);
        fragSend.mpBuffer->mDataLen = IPV4_HDR_STD_LENGTH + fragBytes;

        if (!NetDev_Ip_SendFrag(apNetDev, &fragSend))
            break;

        apData += fragBytes;
        aDataLen -= fragBytes;
        fragSend.mFragOffsetBytes += fragBytes;
        ixFrag++;
    } while (aDataLen > 0);

    return TRUE;
}

UINT32  
NetDev_Ip_GetRoute(
    NETDEV *apNetDev,
    UINT32  aTargetIp
)
{
    IPV4_HOST * pHost;
    UINT32      subMask;
    UINT32      bcastIp;

    if (apNetDev->Proto.Ip.Adapter.Config.mUseDhcp)
    {
        if (apNetDev->Proto.Ip.Adapter.Current.Dhcp.mIsBound)
        {
            pHost = &apNetDev->Proto.Ip.Adapter.Current.Host;
            bcastIp = apNetDev->Proto.Ip.Adapter.Current.IpParam.mBroadcastAddr;
        }
        else
        {
            pHost = &apNetDev->Proto.Ip.Udp.Dhcp.Host;
            bcastIp = apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mBroadcastAddr;
        }
    }
    else
    {
        pHost = &apNetDev->Proto.Ip.Adapter.Config.Static.Host;
        bcastIp = apNetDev->Proto.Ip.Adapter.Config.Static.IpParam.mBroadcastAddr;
    }

    if (aTargetIp == bcastIp)
        return bcastIp;

    subMask = pHost->mSubnetMask;
    if ((subMask & aTargetIp) ==
        (subMask & pHost->mIpAddress))
    {
        // host is on same subnet
        return aTargetIp;
    }

    //
    // parse routing tables to find the right place to send it
    //
    // if we can't find anything send the packet to the default gateway
    //

    Debug_Printf("Routing %d.%d.%d.%d to default gateway\n",
        aTargetIp & 0xFF,
        (aTargetIp >> 8) & 0xFF,
        (aTargetIp >> 16) & 0xFF,
        (aTargetIp >> 24) & 0xFF);

    return pHost->mDefaultGateway;
}
