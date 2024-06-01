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

UINT16
NetDev_Udp_CalcHdrChecksum(
    UINT16 *    apPseudo,
    UINT16 *    apUdpPacket,
    UINT16      aUdpLength
)
{
    UINT32 result;
    UINT32 len;
    UINT16 u16;

    result = 0;

    len = UDP_PSEUDO_LENGTH;
    do {
        u16 = (*apPseudo);
        apPseudo++;
        result += K2_SWAP16(u16);
        len -= sizeof(UINT16);
    } while (len > 0);

    len = UDP_HDR_LENGTH - 2;   // checksum is last field
    do {
        u16 = (*apUdpPacket);
        apUdpPacket++;
        result += K2_SWAP16(u16);
        len -= sizeof(UINT16);
    } while (len > 0);

    apUdpPacket++;  // bypass checksum field

    len = aUdpLength - UDP_HDR_LENGTH;
    do {
        u16 = (*apUdpPacket);
        apUdpPacket++;
        result += K2_SWAP16(u16);
        len -= sizeof(UINT16);
    } while (len > 2);

    u16 = (*apUdpPacket);
    if (len == 1)
    {
        // packet is not an even length
        u16 &= 0x00FF;  // not swapped yet
    }
    result += K2_SWAP16(u16);

    do {
        result = ((result >> 16) + result) & 0xFFFF;
    } while (result & 0xFFFF0000);

    return ~((UINT16)result);
}

void 
NetDev_Udp_OnStart(
    NETDEV * apNetDev
)
{
    NetDev_Dhcp_OnStart(apNetDev);
}

void 
NetDev_Udp_OnRecv(
    NETDEV *        apNetDev,
    UINT32          aSrcIpAddr,
    UINT32          aDstIpAddr,
    UINT8 const *   apData,
    UINT32          aDataLen
)
{
    UINT16              u16;
    UINT16              chkSum;
    UINT8               udpPseudo[UDP_PSEUDO_LENGTH];
    UINT16              udpLen;
    NETDEV_UDP_ADDR     udpSrc;
    NETDEV_UDP_ADDR     udpDst;

    K2MEM_Copy(&u16, &apData[UDP_HDR_OFFSET_LENGTH_HI], sizeof(UINT16));
    udpLen = K2_SWAP16(u16);

    K2MEM_Copy(&udpDst.mPort, &apData[UDP_HDR_OFFSET_PORT_DST_HI], sizeof(UINT16));
    udpDst.mPort = K2_SWAP16(udpDst.mPort);

//    Debug_Printf("Recv UDP(dst port %d) length %d\n", udpDst.mPort, udpLen);

    if ((udpLen < UDP_HDR_LENGTH) ||
        (udpLen > aDataLen))
    {
        Debug_Printf("UDP hdr has packet size %d but length left is %d\n", udpLen, aDataLen);
        return;
    }

    K2MEM_Copy(&udpPseudo[UDP_PSEUDO_OFFSET_SRC_IPADDR], &aSrcIpAddr, sizeof(UINT32));
    K2MEM_Copy(&udpPseudo[UDP_PSEUDO_OFFSET_DST_IPADDR], &aDstIpAddr, sizeof(UINT32));
    udpPseudo[UDP_PSEUDO_OFFSET_ZERO] = 0;
    udpPseudo[UDP_PSEUDO_OFFSET_PROTO_NUM] = IPV4_PROTO_UDP;
    K2MEM_Copy(&udpPseudo[UDP_PSEUDO_OFFSET_LENGTH_HI], &apData[UDP_HDR_OFFSET_LENGTH_HI], sizeof(UINT16));

    K2MEM_Copy(&u16, &apData[UDP_HDR_OFFSET_CHKSUM_HI], sizeof(UINT16));
    u16 = K2_SWAP16(u16);
    if ((0 != u16) && (0xFFFF != u16))
    {
        chkSum = NetDev_Udp_CalcHdrChecksum((UINT16 *)&udpPseudo, (UINT16 *)apData, udpLen);
        if (u16 != chkSum)
        {
            Debug_Printf("UDP packet had bad checksum (had %04X not %04X)-------------------------\n", chkSum, u16);
            return;
        }
    }
//    else
//    {
//        Debug_Printf("UDP packet no checksum\n");
//    }

    udpDst.mIpAddr = aDstIpAddr;

    K2MEM_Copy(&udpSrc.mPort, &apData[UDP_HDR_OFFSET_PORT_SRC_HI], sizeof(UINT16));
    udpSrc.mPort = K2_SWAP16(udpSrc.mPort);
    udpSrc.mIpAddr = aSrcIpAddr;

    if (udpDst.mPort == UDP_PORT_DHCP_CLIENT)
    {
        NetDev_Dhcp_OnRecv(apNetDev, &udpSrc, &udpDst, apData + UDP_HDR_LENGTH, udpLen - UDP_HDR_LENGTH);
    }
    else if (udpSrc.mPort == UDP_PORT_DNS)
    {
        NetDev_Dns_OnRecv(apNetDev, &udpSrc, &udpDst, apData + UDP_HDR_LENGTH, udpLen - UDP_HDR_LENGTH);
    }
    else
    {
//        Debug_Printf("Recv UDP srcPort(%04X), dstPort(%04X), %d, no target\n", udpSrc.mPort, udpDst.mPort, udpLen);
    }
}

void 
NetDev_Udp_OnStop(
    NETDEV * apNetDev
)
{
    NetDev_Dhcp_OnStop(apNetDev);
}

void 
NetDev_Udp_Deinit(
    NETDEV * apNetDev
)
{
    NetDev_Dns_Deinit(apNetDev);
    NetDev_Dhcp_Deinit(apNetDev);
}

BOOL
NetDev_Udp_Init(
    NETDEV *apNetDev
)
{
    K2MEM_Zero(&apNetDev->Proto.Ip.Udp, sizeof(NETDEV_UDP_PROTO));

    apNetDev->Proto.Ip.Udp.mInitialTTL = K2OS_IPV4_DEFAULT_UDP_TTL_VALUE;

    if (!NetDev_Dhcp_Init(apNetDev))
    {
        return FALSE;
    }

    if (!NetDev_Dns_Init(apNetDev))
    {
        NetDev_Dhcp_Deinit(apNetDev);
        return FALSE;
    }

    return TRUE;
}

BOOL
NetDev_Udp_Send(
    NETDEV *                apNetDev,
    UINT32 const *          apTargetIp,
    UINT16                  aSrcPort,
    UINT16                  aDstPort,
    BOOL                    aDoNotFragment,
    UINT8 *                 apFullUdpPacketBuffer,
    UINT16                  aFullUdpPacketLen
)
{
    UINT16  u16;
    UINT32  ourIp;
    UINT32  bcastIp;
    UINT32  targetIp;
    UINT8   pseudoHdr[UDP_PSEUDO_LENGTH];

    if (aFullUdpPacketLen < UDP_HDR_LENGTH)
        return FALSE;

    u16 = K2_SWAP16(aSrcPort);
    K2MEM_Copy(&apFullUdpPacketBuffer[UDP_HDR_OFFSET_PORT_SRC_HI], &u16, sizeof(UINT16));
    
    u16 = K2_SWAP16(aDstPort);
    K2MEM_Copy(&apFullUdpPacketBuffer[UDP_HDR_OFFSET_PORT_DST_HI], &u16, sizeof(UINT16));

    u16 = (UINT16)aFullUdpPacketLen;
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&apFullUdpPacketBuffer[UDP_HDR_OFFSET_LENGTH_HI], &u16, sizeof(UINT16));

    NetDev_Ip_GetCurrent(apNetDev, &ourIp, &bcastIp, NULL);

    if (NULL != apTargetIp)
    {
        targetIp = *apTargetIp;
        if (targetIp != bcastIp)
        {
            if (0 == targetIp)
                return FALSE;
            if (targetIp == 0xFFFFFFFF)
                targetIp = bcastIp;
        }
        if (targetIp == bcastIp)
        {
            apTargetIp = NULL;
        }
    }
    else
    {
        targetIp = bcastIp;
    }

    K2MEM_Copy(&pseudoHdr[UDP_PSEUDO_OFFSET_DST_IPADDR], &targetIp, sizeof(UINT32));
    K2MEM_Copy(&pseudoHdr[UDP_PSEUDO_OFFSET_SRC_IPADDR], &ourIp, sizeof(UINT32));
    pseudoHdr[UDP_PSEUDO_OFFSET_ZERO] = 0;
    pseudoHdr[UDP_PSEUDO_OFFSET_PROTO_NUM] = IPV4_PROTO_UDP;
    K2MEM_Copy(&pseudoHdr[UDP_PSEUDO_OFFSET_LENGTH_HI], &u16, sizeof(UINT16));

    u16 = NetDev_Udp_CalcHdrChecksum((UINT16 *)pseudoHdr, (UINT16 *)apFullUdpPacketBuffer, (UINT16)aFullUdpPacketLen);
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&apFullUdpPacketBuffer[UDP_HDR_OFFSET_CHKSUM_HI], &u16, sizeof(UINT16));

    return NetDev_Ip_Send(apNetDev, apTargetIp, IPV4_PROTO_UDP, aDoNotFragment, apFullUdpPacketBuffer, aFullUdpPacketLen);
}

