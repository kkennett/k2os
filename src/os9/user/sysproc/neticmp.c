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
NetDev_Icmp_CalcPacketChecksum(
    UINT16 *apIcmpPacket,
    UINT16 aLength
)
{
    UINT32 result;
    UINT16 u16;

    result = 0;
    K2_ASSERT(4 < aLength);

    u16 = (*apIcmpPacket);
    apIcmpPacket++;
    result += K2_SWAP16(u16);
    aLength -= sizeof(UINT16);

    // bypass checksum field
    apIcmpPacket++;
    aLength -= sizeof(UINT16);

    do {
        u16 = (*apIcmpPacket);
        apIcmpPacket++;
        result += K2_SWAP16(u16);
        aLength -= sizeof(UINT16);
    } while (aLength > 2);

    u16 = (*apIcmpPacket);
    if (aLength == 1)
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
NetDev_Icmp_OnStart(
    NETDEV * apNetDev
)
{

}

void 
NetDev_Icmp_OnRecv(
    NETDEV *        apNetDev,
    UINT32          aSrcIpAddr,
    UINT32          aDstIpAddr,
    UINT8 const *   apData,
    UINT32          aDataLen
)
{
    Debug_Printf("%s(%d), %d bytes\n", __FUNCTION__, *apData, aDataLen);
}

void 
NetDev_Icmp_OnStop(
    NETDEV * apNetDev
)
{

}

void
NetDev_Icmp_SendPing(
    NETDEV *    apNetDev,
    UINT32      aTargetIpAddr
)
{
    UINT8 pingPacket[ICMP_PING_MIN_LENGTH];
    UINT16 u16;

    pingPacket[ICMP_HDR_OFFSET_TYPE] = ICMP_TYPE_ECHO_REQUEST;
    pingPacket[ICMP_HDR_OFFSET_CODE] = 0;
    pingPacket[ICMP_HDR_OFFSET_CHKSUM_HI] = 0;
    pingPacket[ICMP_HDR_OFFSET_CHKSUM_LO] = 0;
    u16 = apNetDev->Proto.Ip.Icmp.mNextIdent--;
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pingPacket[ICMP_PING_OFFSET_IDENT], &u16, sizeof(UINT16));
    u16 = apNetDev->Proto.Ip.Icmp.mNextSeqNo++;
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pingPacket[ICMP_PING_OFFSET_SEQNO], &u16, sizeof(UINT16));
    K2MEM_Copy(&pingPacket[ICMP_PING_OFFSET_PAYLOAD], "K2OS", 4);

    u16 = NetDev_Icmp_CalcPacketChecksum((UINT16 *)pingPacket, ICMP_PING_MIN_LENGTH);
    u16 = K2_SWAP16(u16);
    K2MEM_Copy(&pingPacket[ICMP_HDR_OFFSET_CHKSUM_HI], &u16, sizeof(UINT16));

    NetDev_Ip_Send(apNetDev, &aTargetIpAddr, IPV4_PROTO_ICMP, TRUE, pingPacket, ICMP_PING_MIN_LENGTH);
}

void 
NetDev_Icmp_Deinit(
    NETDEV * apNetDev
)
{

}

BOOL
NetDev_Icmp_Init(
    NETDEV *apNetDev
)
{
    K2MEM_Zero(&apNetDev->Proto.Ip.Icmp, sizeof(NETDEV_ICMP_PROTO));
    apNetDev->Proto.Ip.Icmp.mNextIdent = 0xFFFF;
    apNetDev->Proto.Ip.Icmp.mNextSeqNo = 1;
    return TRUE;
}

