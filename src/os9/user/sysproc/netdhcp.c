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

void NetDev_Dhcp_Event(NETDEV *apNetDev, NETDEV_Dhcp_EventType aType, UINT8 const *apDhcpPacket, UINT32 aDhcpPacketLen);

void 
NetDev_Dhcp_OnStart(
    NETDEV * apNetDev
)
{
    NetDev_Dhcp_Event(apNetDev, NETDEV_Dhcp_Event_Reset, NULL, 0);
}

void 
NetDev_Dhcp_OnRecv(
    NETDEV *                apNetDev,
    NETDEV_UDP_ADDR const * apSrcUdpAddr,
    NETDEV_UDP_ADDR const * apDstUdpAddr,
    UINT8 const *           apDhcpPacket,
    UINT32                  aDhcpPacketLen
)
{
    UINT32                  u32;
    UINT8 const *           pOptions;
    UINT32                  optLeft;
    UINT8                   opt;
    UINT8                   optLen;
    UINT8 const *           pOpt;
    UINT8                   msgType;
    NETDEV_Dhcp_EventType   evtType;
    UINT32                  hwLen;

    if (apDhcpPacket[DHCP_PACKET_OFFSET_OP] != DHCP_OP_RESPONSE)
    {
        // not a response
        return;
    }

    hwLen = apNetDev->Desc.Addr.mLen;

    if ((apDhcpPacket[DHCP_PACKET_OFFSET_HTYPE] != apNetDev->Proto.mpL2->mHTYPE) ||
        (apDhcpPacket[DHCP_PACKET_OFFSET_HLEN] != hwLen))
    {
        // no hw type match
        return;
    }

    if (0 != K2MEM_Compare(&apDhcpPacket[DHCP_PACKET_OFFSET_CHADDR], &apNetDev->Desc.Addr.mValue, hwLen))
    {
        // not for us
        return;
    }

    K2MEM_Copy(&u32, &apDhcpPacket[DHCP_PACKET_OFFSET_XID], sizeof(UINT32));
    if (K2_SWAP32(u32) != apNetDev->Proto.Ip.Udp.Dhcp.mLastXID)
    {
        // not the same transaction id as our last send
        return;
    }

    //
    // this is a response for us
    //

    //
    // process options
    //
    pOptions = apDhcpPacket + DHCP_PACKET_OFFSET_OPTIONS;
    optLeft = aDhcpPacketLen - DHCP_PACKET_OFFSET_OPTIONS;

    if ((0 == optLeft--) || ((*(pOptions++)) != DHCP_OPTIONS_COOKIE_0))
    {
        return;
    }
    if ((0 == optLeft--) || ((*(pOptions++)) != DHCP_OPTIONS_COOKIE_1))
    {
        return;
    }
    if ((0 == optLeft--) || ((*(pOptions++)) != DHCP_OPTIONS_COOKIE_2))
    {
        return;
    }
    if ((0 == optLeft--) || ((*(pOptions++)) != DHCP_OPTIONS_COOKIE_3))
    {
        return;
    }
    if (0 == optLeft)
    {
        return;
    }

    //
    // find the message type
    //
    msgType = 0;
    do {
        opt = *pOptions;
        pOptions++;
        --optLeft;
        if (opt != DHCP_OPTION_PAD)
        {
            if (opt == DHCP_OPTION_END)
                break;

            optLen = *pOptions;
            pOptions++;
            optLeft--;
            if (optLeft < optLen)
            {
                Debug_Printf("*** Dhcp Option length greater than remaining option space\n");
                msgType = 0;
                break;
            }

            pOpt = pOptions;
            pOptions += optLen;
            optLeft -= optLen;

            if (opt == DHCP_OPTION_MSGTYPE)
            {
                if (optLen != 1)
                {
                    Debug_Printf("*** Dhcp MessageType length is wrong\n");
                    msgType = 0;
                    break;
                }

                msgType = *pOpt;
                if (DHCP_MSGTYPE_COUNT <= msgType)
                {
                    msgType = 0;
                    break;
                }
            }
        }
    } while (0 != optLeft);
    if (0 == msgType)
    {
        Debug_Printf("*** Dhcp MessageType received is missing or invalid (%d)\n", msgType);
        return;
    }

    switch (msgType)
    {
    case DHCP_MSGTYPE_DHCPOFFER:
//        Debug_Printf("Dhcp-Recv-Offer\n");
        evtType = NETDEV_Dhcp_Event_Recv_DHCPOFFER;
        break;

    case DHCP_MSGTYPE_DHCPACK:
//        Debug_Printf("Dhcp-Recv-Ack\n");
        evtType = NETDEV_Dhcp_Event_Recv_DHCPACK;
        break;

    case DHCP_MSGTYPE_DHCPNAK:
//        Debug_Printf("Dhcp-Recv-Nack\n");
        evtType = NETDEV_Dhcp_Event_Recv_DHCPNACK;
        break;

    default:
        Debug_Printf("*** Unexpected DHCP reply. Ignored\n");
        return;
    }

    NetDev_Dhcp_Event(apNetDev, evtType, apDhcpPacket, aDhcpPacketLen);
}

void 
NetDev_Dhcp_OnStop(
    NETDEV * apNetDev
)
{
    if (NULL != apNetDev->Proto.Ip.Udp.Dhcp.mpTimer)
    {
        NetDev_DelTimer(apNetDev, apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);
    }

    K2MEM_Zero(&apNetDev->Proto.Ip.Udp.Dhcp, sizeof(NETDEV_DHCP_PROTO));
}

void 
NetDev_Dhcp_Deinit(
    NETDEV * apNetDev
)
{

}

BOOL
NetDev_Dhcp_Init(
    NETDEV *apNetDev
)
{
    K2MEM_Zero(&apNetDev->Proto.Ip.Udp.Dhcp, sizeof(NETDEV_DHCP_PROTO));
    return TRUE;
}

void
Dhcp_Timer_Callback(
    NETDEV *        apNetDev,
    NETDEV_TIMER *  apTimer
)
{
//    Debug_Printf("Dhcp-Timeout(%d)\n", apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter);
    K2_ASSERT(apTimer == apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);
    NetDev_Dhcp_Event(apNetDev, NETDEV_Dhcp_Event_Timeout, NULL, 0);
}

void
Dhcp_Enter_Init(
    NETDEV * apNetDev
)
{
    UINT16  u16;
    UINT32  u32;
    UINT8 * pUdpHdr;
    UINT8 * pDhcpPacket;
    UINT8 * pOptions;
    UINT32  optLeft;
    UINT8 * pThisOpt;
    UINT32  packetMax;
    UINT16  hwLen;

    Debug_Printf("NetMgr: DHCP discovery is (re)started\n");

    apNetDev->Proto.Ip.Udp.Dhcp.mState = NETDEV_Dhcp_State_Init;

    K2_ASSERT(NULL == apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);

    packetMax = UDP_HDR_LENGTH + DHCP_PACKET_OFFSET_OPTIONS + K2OS_IPV4_ADAPTER_HOST_NAME_MAX_LEN + 32;

    pUdpHdr = (UINT8 *)K2OS_Heap_Alloc(packetMax);
    if (NULL == pUdpHdr)
    {
        return;
    }

//    Debug_Printf("Dhcp-Discover\n");

    pDhcpPacket = pUdpHdr + UDP_HDR_LENGTH;
    pDhcpPacket[DHCP_PACKET_OFFSET_OP] = DHCP_OP_REQUEST;

    hwLen = (UINT16)apNetDev->Desc.Addr.mLen;

    pDhcpPacket[DHCP_PACKET_OFFSET_HTYPE] = apNetDev->Proto.mpL2->mHTYPE;
    pDhcpPacket[DHCP_PACKET_OFFSET_HLEN] = hwLen;
    pDhcpPacket[DHCP_PACKET_OFFSET_HOPS] = 0;
    u32 = ++apNetDev->Proto.Ip.Udp.Dhcp.mLastXID;
    u32 = K2_SWAP32(u32);
    K2MEM_Copy(&pDhcpPacket[DHCP_PACKET_OFFSET_XID], &u32, sizeof(UINT32));
    pDhcpPacket[DHCP_PACKET_OFFSET_SECS] = 0;
    pDhcpPacket[DHCP_PACKET_OFFSET_FLAGS_HI] = DHCP_FLAGS_HI_BROADCAST;
    pDhcpPacket[DHCP_PACKET_OFFSET_FLAGS_LO] = 0;

    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_CIADDR], sizeof(UINT32));
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_YIADDR], sizeof(UINT32));
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_SIADDR], sizeof(UINT32));
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_GIADDR], sizeof(UINT32));
    K2MEM_Copy(&pDhcpPacket[DHCP_PACKET_OFFSET_CHADDR], &apNetDev->Desc.Addr.mValue, hwLen);
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_CHADDR + hwLen], DHCP_PACKET_CHADDR_LENGTH - hwLen);
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_SNAME], DHCP_PACKET_SNAME_LENGTH);
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_FILE], DHCP_PACKET_FILE_LENGTH);

    pOptions = pDhcpPacket + DHCP_PACKET_OFFSET_OPTIONS;
    optLeft = packetMax - (UINT32)(pOptions - pUdpHdr);

    *pOptions = DHCP_OPTIONS_COOKIE_0;
    pOptions++;
    optLeft--;
    *pOptions = DHCP_OPTIONS_COOKIE_1;
    pOptions++;
    optLeft--;
    *pOptions = DHCP_OPTIONS_COOKIE_2;
    pOptions++;
    optLeft--;
    *pOptions = DHCP_OPTIONS_COOKIE_3;
    pOptions++;
    optLeft--;

    *pOptions = DHCP_OPTION_MSGTYPE;
    pOptions++;
    optLeft--;
    *pOptions = 1;  // length of payload is 1
    pOptions++;
    optLeft--;
    *pOptions = DHCP_MSGTYPE_DHCPDISCOVER;
    pOptions++;
    optLeft--;

    if (apNetDev->Proto.Ip.Adapter.Config.mHostName[0] != 0)
    {
        apNetDev->Proto.Ip.Adapter.Config.mHostName[K2OS_IPV4_ADAPTER_HOST_NAME_MAX_LEN] = 0;
        *pOptions = DHCP_OPTION_HOST_NAME;
        pOptions++;
        optLeft--;
        u32 = K2ASC_Len(apNetDev->Proto.Ip.Adapter.Config.mHostName);
        *pOptions = (UINT8)(u32 + 1);
        pOptions++;
        optLeft--;
        K2ASC_Copy((char *)pOptions, apNetDev->Proto.Ip.Adapter.Config.mHostName);
        pOptions += u32;
        optLeft -= u32;
        *pOptions = 0;
        pOptions++;
        optLeft--;
    }

    pThisOpt = pOptions;
    *pOptions = DHCP_OPTION_PARAM_REQ;
    pOptions++;
    optLeft--;
    pOptions++; // bypass option length. we know it is at pThisOpt[1]
    optLeft--;
    *pOptions = DHCP_OPTION_SUBNET_MASK; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_ROUTERS; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_DNS_SERVERS; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_DOMAIN_NAME; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_LEASE_TIME; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_T1_VALUE; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_T2_VALUE; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_SERVER_ID; pOptions++; optLeft--;

    // set option length
    pThisOpt[1] = (UINT8)(pOptions - (pThisOpt + 2));

    // end of options
    *pOptions = DHCP_OPTION_END;
    pOptions++;

    // total length of udp packet
    u16 = (UINT16)(pOptions - pUdpHdr);
    if (0 != (u16 & 1))
    {
        *pOptions = DHCP_OPTION_PAD;
        pOptions++;
        u16++;
    }

    if (!NetDev_Udp_Send(apNetDev, NULL, UDP_PORT_DHCP_CLIENT, UDP_PORT_DHCP_SERVER, FALSE, pUdpHdr, u16))
    {
        Debug_Printf("*** Dhcp send failed\n");
        apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter = 0;
    }
    else
    {
        apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter = 8;
    }
    apNetDev->Proto.Ip.Udp.Dhcp.mpTimer = NetDev_AddTimer(apNetDev, 1000, Dhcp_Timer_Callback);
}

void
Dhcp_Enter_Rebinding(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.Ip.Udp.Dhcp.mState = NETDEV_Dhcp_State_Rebinding;
}

void
DhcpState_Rebinding(
    NETDEV *        apNetDev,
    NETDEV_Dhcp_EventType  aEvent,
    UINT8 const *   apDhcpPacket,
    UINT32          aDhcpPacketLen
)
{
    K2_ASSERT(NETDEV_Dhcp_State_Rebinding == apNetDev->Proto.Ip.Udp.Dhcp.mState);
    Debug_Printf("%s()\n", __FUNCTION__);
}

void
Dhcp_Enter_Renewing(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.Ip.Udp.Dhcp.mState = NETDEV_Dhcp_State_Renewing;
}

void
DhcpState_Renewing(
    NETDEV *        apNetDev,
    NETDEV_Dhcp_EventType  aEvent,
    UINT8 const *   apDhcpPacket,
    UINT32          aDhcpPacketLen
)
{
    K2_ASSERT(NETDEV_Dhcp_State_Renewing == apNetDev->Proto.Ip.Udp.Dhcp.mState);
    Debug_Printf("%s()\n", __FUNCTION__);
}

void
Dhcp_Enter_Bound(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.Ip.Udp.Dhcp.mState = NETDEV_Dhcp_State_Bound;

//    Debug_Printf("%s()\n", __FUNCTION__);

    Debug_Printf("\nNetMgr: DHCP is bound\n  HOST NAME:       \"%s\"\n", apNetDev->Proto.Ip.Adapter.Config.mHostName);
    Debug_Printf("  DOMAIN NAME:     \"%s\"\n", apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mDomain);

    Debug_Printf("  DHCP SERVER:     %d.%d.%d.%d\n",
        apNetDev->Proto.Ip.Udp.Dhcp.mDhcpServerIp & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.mDhcpServerIp >> 8) & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.mDhcpServerIp >> 16) & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.mDhcpServerIp >> 24) & 0xFF);

    Debug_Printf("  IP:              %d.%d.%d.%d\n",
        apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress >> 8) & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress >> 16) & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress >> 24) & 0xFF);

    Debug_Printf("  SUBNET MASK:     %d.%d.%d.%d\n",
        apNetDev->Proto.Ip.Udp.Dhcp.Host.mSubnetMask & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mSubnetMask >> 8) & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mSubnetMask >> 16) & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mSubnetMask >> 24) & 0xFF);

    Debug_Printf("  DEFAULT GATEWAY: %d.%d.%d.%d\n",
        apNetDev->Proto.Ip.Udp.Dhcp.Host.mDefaultGateway & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mDefaultGateway >> 8) & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mDefaultGateway >> 16) & 0xFF,
        (apNetDev->Proto.Ip.Udp.Dhcp.Host.mDefaultGateway >> 24) & 0xFF);

    if (0 != apNetDev->Proto.Ip.Udp.Dhcp.Host.mPrimaryDNS)
    {
        Debug_Printf("  PRIMARY DNS:     %d.%d.%d.%d\n",
            apNetDev->Proto.Ip.Udp.Dhcp.Host.mPrimaryDNS & 0xFF,
            (apNetDev->Proto.Ip.Udp.Dhcp.Host.mPrimaryDNS >> 8) & 0xFF,
            (apNetDev->Proto.Ip.Udp.Dhcp.Host.mPrimaryDNS >> 16) & 0xFF,
            (apNetDev->Proto.Ip.Udp.Dhcp.Host.mPrimaryDNS >> 24) & 0xFF);
        if (0 != apNetDev->Proto.Ip.Udp.Dhcp.Host.mSecondaryDNS)
            Debug_Printf("  SECONDARY DNS:   %d.%d.%d.%d\n",
                apNetDev->Proto.Ip.Udp.Dhcp.Host.mSecondaryDNS & 0xFF,
                (apNetDev->Proto.Ip.Udp.Dhcp.Host.mSecondaryDNS >> 8) & 0xFF,
                (apNetDev->Proto.Ip.Udp.Dhcp.Host.mSecondaryDNS >> 16) & 0xFF,
                (apNetDev->Proto.Ip.Udp.Dhcp.Host.mSecondaryDNS >> 24) & 0xFF);
    }

    Debug_Printf("  T1 = %d\n", apNetDev->Proto.Ip.Udp.Dhcp.mMsT1);
    Debug_Printf("  T2 = %d\n\n", apNetDev->Proto.Ip.Udp.Dhcp.mMsT2);

    K2MEM_Copy(&apNetDev->Proto.Ip.Adapter.Current.Host, &apNetDev->Proto.Ip.Udp.Dhcp.Host, sizeof(IPV4_HOST));
    K2MEM_Copy(&apNetDev->Proto.Ip.Adapter.Current.IpParam, &apNetDev->Proto.Ip.Udp.Dhcp.IpParam, sizeof(K2OS_IPV4_PARAM));
    apNetDev->Proto.Ip.Adapter.Current.Dhcp.mIsBound = TRUE;
}

void
DhcpState_Bound(
    NETDEV *                apNetDev,
    NETDEV_Dhcp_EventType   aEvent,
    UINT8 const *           apDhcpPacket,
    UINT32                  aDhcpPacketLen
)
{
    K2_ASSERT(NETDEV_Dhcp_State_Bound == apNetDev->Proto.Ip.Udp.Dhcp.mState);
    Debug_Printf("%s(%d)\n", __FUNCTION__, aEvent);
}

void
Dhcp_Enter_ArpCheck(
    NETDEV * apNetDev
)
{
    UINT32 dhcpIp;

//    Debug_Printf("%s()\n", __FUNCTION__);

    apNetDev->Proto.Ip.Udp.Dhcp.mState = NETDEV_Dhcp_State_ArpCheck;

    K2_ASSERT(NULL == apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);

    dhcpIp = apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress;
    NetDev_Arp_Clear(apNetDev, dhcpIp);
    if (NetDev_Arp_Resolve(apNetDev, dhcpIp, NULL, FALSE))
    {
        K2_ASSERT(0);
    }

    apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter = 1;
    apNetDev->Proto.Ip.Udp.Dhcp.mpTimer = NetDev_AddTimer(apNetDev, 1000, Dhcp_Timer_Callback);
}

void
DhcpState_ArpCheck(
    NETDEV *                apNetDev,
    NETDEV_Dhcp_EventType   aEvent,
    UINT8 const *           apDhcpPacket,
    UINT32                  aDhcpPacketLen
)
{
    UINT8 const *pHwAddr;

//    Debug_Printf("%s()\n", __FUNCTION__);

    K2_ASSERT(NULL != apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);

    // this is the only thing that should happen in this state
    if (aEvent != NETDEV_Dhcp_Event_Timeout)
    {
        return;
    }

    if (0 == apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter)
    {
        NetDev_DelTimer(apNetDev, apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);

        pHwAddr = NULL;
        if (NetDev_Arp_Resolve(apNetDev, apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress, &pHwAddr, TRUE))
        {
            Debug_Printf("*** DHCP gratuitous ARP failed\n");
            NetDev_Dhcp_Event(apNetDev, NETDEV_Dhcp_Event_Reset, NULL, 0);
            return;
        }

        Dhcp_Enter_Bound(apNetDev);

        return;
    }

    --apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter;
}

void
Dhcp_Selecting_SendRequest(
    NETDEV *apNetDev
)
{
    UINT16  u16;
    UINT32  u32;
    UINT8 * pUdpHdr;
    UINT8 * pDhcpPacket;
    UINT8 * pOptions;
    UINT32  optLeft;
    UINT8 * pThisOpt;
    UINT32  packetMax;
    UINT32  hwLen;

    K2_ASSERT(NETDEV_Dhcp_State_Selecting == apNetDev->Proto.Ip.Udp.Dhcp.mState);

    packetMax = UDP_HDR_LENGTH + DHCP_PACKET_OFFSET_OPTIONS + 32;

    pUdpHdr = (UINT8 *)K2OS_Heap_Alloc(packetMax);
    if (NULL == pUdpHdr)
        return;

//    Debug_Printf("Dhcp-Request\n");

    pDhcpPacket = pUdpHdr + UDP_HDR_LENGTH;

    hwLen = apNetDev->Desc.Addr.mLen;

    pDhcpPacket[DHCP_PACKET_OFFSET_OP] = DHCP_OP_REQUEST;
    pDhcpPacket[DHCP_PACKET_OFFSET_HTYPE] = apNetDev->Proto.mpL2->mHTYPE;
    pDhcpPacket[DHCP_PACKET_OFFSET_HLEN] = hwLen;
    pDhcpPacket[DHCP_PACKET_OFFSET_HOPS] = 0;
    u32 = apNetDev->Proto.Ip.Udp.Dhcp.mLastXID;
    u32 = K2_SWAP32(u32);
    K2MEM_Copy(&pDhcpPacket[DHCP_PACKET_OFFSET_XID], &u32, sizeof(UINT32));
    pDhcpPacket[DHCP_PACKET_OFFSET_SECS] = 0;
    pDhcpPacket[DHCP_PACKET_OFFSET_FLAGS_HI] = DHCP_FLAGS_HI_BROADCAST;
    pDhcpPacket[DHCP_PACKET_OFFSET_FLAGS_LO] = 0;

    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_CIADDR], sizeof(UINT32));
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_YIADDR], sizeof(UINT32));
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_SIADDR], sizeof(UINT32));
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_GIADDR], sizeof(UINT32));
    K2MEM_Copy(&pDhcpPacket[DHCP_PACKET_OFFSET_CHADDR], apNetDev->Desc.Addr.mValue, hwLen);
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_CHADDR + hwLen], DHCP_PACKET_CHADDR_LENGTH - hwLen);
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_SNAME], DHCP_PACKET_SNAME_LENGTH);
    K2MEM_Zero(&pDhcpPacket[DHCP_PACKET_OFFSET_FILE], DHCP_PACKET_FILE_LENGTH);

    pOptions = pDhcpPacket + DHCP_PACKET_OFFSET_OPTIONS;
    optLeft = packetMax - (UINT32)(pOptions - pUdpHdr);

    *pOptions = DHCP_OPTIONS_COOKIE_0;
    pOptions++;
    optLeft--;
    *pOptions = DHCP_OPTIONS_COOKIE_1;
    pOptions++;
    optLeft--;
    *pOptions = DHCP_OPTIONS_COOKIE_2;
    pOptions++;
    optLeft--;
    *pOptions = DHCP_OPTIONS_COOKIE_3;
    pOptions++;
    optLeft--;

    *pOptions = DHCP_OPTION_MSGTYPE;
    pOptions++;
    optLeft--;
    *pOptions = 1;  // length of payload is 1
    pOptions++;
    optLeft--;
    *pOptions = DHCP_MSGTYPE_DHCPREQUEST;
    pOptions++;
    optLeft--;

    *pOptions = DHCP_OPTION_SERVER_ID;
    pOptions++;
    optLeft--;
    *pOptions = 4; // length of payload is 4
    pOptions++;
    optLeft--;
    K2MEM_Copy(pOptions, &apNetDev->Proto.Ip.Udp.Dhcp.mDhcpServerIp, sizeof(UINT32));
    pOptions += 4;
    optLeft -= 4;

    *pOptions = DHCP_OPTION_REQUEST_IP;
    pOptions++;
    optLeft--;
    *pOptions = 4; // length of payload is 4
    pOptions++;
    optLeft--;
    K2MEM_Copy(pOptions, &apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress, sizeof(UINT32));
    pOptions += 4;
    optLeft -= 4;

    pThisOpt = pOptions;
    *pOptions = DHCP_OPTION_PARAM_REQ;
    pOptions++;
    optLeft--;
    pOptions++; // bypass option length. we know it is at pThisOpt[1]
    optLeft--;
    *pOptions = DHCP_OPTION_TIME_OFFSET; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_TIME_SERVERS; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_IP_TTL; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_INTERFACE_MTU; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_BROADCAST_ADDR; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_ARP_TIMEOUT; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_TCP_TTL; pOptions++; optLeft--;
    *pOptions = DHCP_OPTION_TCP_KEEPALIVE; pOptions++; optLeft--;
    // set option length
    pThisOpt[1] = (UINT8)(pOptions - (pThisOpt + 2));

    // end of options
    *pOptions = DHCP_OPTION_END;
    pOptions++;

    // put udp packet length into its spots in the udp header
    // and in the pseudo header.  size must be even
    u16 = (UINT16)(pOptions - pUdpHdr);
    if (0 != (u16 & 1))
    {
        *pOptions = DHCP_OPTION_PAD;
        pOptions++;
        u16++;
    }

    NetDev_Udp_Send(apNetDev, NULL, UDP_PORT_DHCP_CLIENT, UDP_PORT_DHCP_SERVER, FALSE, pUdpHdr, u16);
}

void
Dhcp_Enter_Selecting(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.Ip.Udp.Dhcp.mState = NETDEV_Dhcp_State_Selecting;

    K2_ASSERT(NULL == apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);

    Dhcp_Selecting_SendRequest(apNetDev);

    apNetDev->Proto.Ip.Udp.Dhcp.mpTimer = NetDev_AddTimer(apNetDev, 1000, Dhcp_Timer_Callback);
}

void
DhcpState_Selecting(
    NETDEV *                apNetDev,
    NETDEV_Dhcp_EventType   aEvent,
    UINT8 const *           apDhcpPacket,
    UINT32                  aDhcpPacketLen
)
{
    UINT8 const *   pOptions;
    UINT8           opt;
    UINT32          optLeft;
    UINT8 const *   pOpt;
    UINT32          optLen;
    UINT32          u32;
    UINT32          ix;
    UINT16          u16;

    K2_ASSERT(NETDEV_Dhcp_State_Selecting == apNetDev->Proto.Ip.Udp.Dhcp.mState);

    if ((NETDEV_Dhcp_Event_Recv_DHCPNACK == aEvent) ||
        ((NETDEV_Dhcp_Event_Timeout == aEvent) && (0 == apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter)))
    {
        NetDev_Dhcp_Event(apNetDev, NETDEV_Dhcp_Event_Reset, NULL, 0);
        return;
    }

    if (NETDEV_Dhcp_Event_Timeout == aEvent)
    {
        if (0 != apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter)
        {
            --apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter;
        }
        else
        {
            K2_ASSERT(0);
        }
        return;
    }

    K2_ASSERT(NETDEV_Dhcp_Event_Recv_DHCPACK == aEvent);

    pOptions = apDhcpPacket + DHCP_PACKET_OFFSET_OPTIONS;
    optLeft = aDhcpPacketLen - DHCP_PACKET_OFFSET_OPTIONS;

    if ((0 == optLeft--) || ((*(pOptions++)) != DHCP_OPTIONS_COOKIE_0))
    {
        return;
    }
    if ((0 == optLeft--) || ((*(pOptions++)) != DHCP_OPTIONS_COOKIE_1))
    {
        return;
    }
    if ((0 == optLeft--) || ((*(pOptions++)) != DHCP_OPTIONS_COOKIE_2))
    {
        return;
    }
    if ((0 == optLeft--) || ((*(pOptions++)) != DHCP_OPTIONS_COOKIE_3))
    {
        return;
    }
    if (0 == optLeft)
    {
        return;
    }

    do {
        opt = *pOptions;
        pOptions++;
        --optLeft;
        if (opt != DHCP_OPTION_PAD)
        {
            if (opt == DHCP_OPTION_END)
                break;

            optLen = *pOptions;
            pOptions++;
            optLeft--;
            if (optLeft < optLen)
            {
                Debug_Printf("*** Dhcp Option length greater than remaining option space\n");
                return;
            }

            pOpt = pOptions;
            pOptions += optLen;
            optLeft -= optLen;

            switch (opt)
            {
            case DHCP_OPTION_TIME_OFFSET:
                if (optLen != 4)
                {
                    Debug_Printf("*** Dhcp TIME_OFFSET option had bad length\n");
                    break;
                }
                K2MEM_Copy(&u32, pOpt, sizeof(INT32));
                u32 = K2_SWAP32(u32);
                apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mTimeOffset = (INT32)u32;
                break;

            case DHCP_OPTION_TIME_SERVERS:
                if (optLen < 4)
                {
                    Debug_Printf("*** Dhcp time servers option had bad length\n");
                    break;
                }
                ix = 0;
                do {
                    K2MEM_Copy(&apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mTimeServers[ix], pOpt, sizeof(UINT32));
                    apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mNumTimeServers = ++ix;
                    optLen -= 4;
                } while ((optLen >= 4) && (ix < K2OS_IPV4_PARAM_MAX_NUM_TIME_SERVERS));
                break;

            case DHCP_OPTION_IP_TTL:
                if (optLen != 1)
                {
                    Debug_Printf("*** Dhcp Ip default TTL value length is incorrect\n");
                    break;
                }
                if (0 == *pOpt)
                {
                    Debug_Printf("*** Dhcp Ip default TTL received from server is zero (ignored)\n");
                    break;
                }
                apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mInitialTTL = *pOpt;
                break;

            case DHCP_OPTION_INTERFACE_MTU:
                if (optLen != 2)
                {
                    Debug_Printf("*** Dhcp interface MTU value length is not correct\n");
                    break;
                }
                K2MEM_Copy(&u16, pOpt, sizeof(UINT16));
                u16 = K2_SWAP16(u16);
                if (u16 < 68)
                {
                    Debug_Printf("*** Dhcp interface MTU value is less than minimum\n");
                    break;
                }
                if (u16 > apNetDev->Desc.mPhysicalMTU)
                {
                    u16 = (UINT16)apNetDev->Desc.mPhysicalMTU;
                }
                apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mInterfaceMTU = u16;
                break;

            case DHCP_OPTION_BROADCAST_ADDR:
                if (optLen != 4)
                {
                    Debug_Printf("*** Dhcp broadcast address value length is incorrect\n");
                    break;
                }
                K2MEM_Copy(&apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mBroadcastAddr, pOpt, sizeof(UINT32));
                break;

            case DHCP_OPTION_ARP_TIMEOUT:
                if (optLen != 4)
                {
                    Debug_Printf("*** Dhcp Arp timeout value length is incorrect\n");
                    break;
                }
                K2MEM_Copy(&u32, pOpt, sizeof(UINT32));
                u32 = K2_SWAP32(u32);
                if (u32 == 0)
                    u32 = 1;
                apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mArpTimeoutMs = u32 * 1000;
                break;

            case DHCP_OPTION_TCP_TTL:
                if (optLen != 1)
                {
                    Debug_Printf("*** Dhcp Tcp TTL value length is incorrect\n");
                    break;
                }
                if (0 == *pOpt)
                {
                    Debug_Printf("*** Dhcp Tcp TTL received from server is zero (ignored)\n");
                    break;
                }
                apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mTcpInitialTTL = *pOpt;
                break;

            case DHCP_OPTION_TCP_KEEPALIVE:
                if (optLen != 4)
                {
                    Debug_Printf("*** Dhcp Tcp keepalive value length is incorrect\n");
                    break;
                }
                K2MEM_Copy(&u32, pOpt, sizeof(UINT32));
                u32 = K2_SWAP32(u32);
                if (0 == u32)
                {
                    Debug_Printf("*** Dhcp Tcp keepalive value is zero (ignored)\n");
                }
                apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mTcpKeepAliveMsInterval = u32 * 1000;
                break;

            default:
                break;
            };
        }
    } while (0 != optLeft);

    NetDev_DelTimer(apNetDev, apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);
    apNetDev->Proto.Ip.Udp.Dhcp.mpTimer = NULL;

    Dhcp_Enter_ArpCheck(apNetDev);
}

void
DhcpState_Init(
    NETDEV *                apNetDev,
    NETDEV_Dhcp_EventType   aEvent,
    UINT8 const *           apRecvData,
    UINT32                  aRecvLen
)
{
    UINT8 const *   pOptions;
    UINT32          optLeft;
    UINT8 const *   pOpt;
    UINT8           opt;
    UINT32          optLen;
    UINT32          u32;

    K2_ASSERT(NETDEV_Dhcp_State_Init == apNetDev->Proto.Ip.Udp.Dhcp.mState);

    if (aEvent == NETDEV_Dhcp_Event_Recv_DHCPOFFER)
    {
        K2MEM_Copy(&apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress, &apRecvData[DHCP_PACKET_OFFSET_YIADDR], sizeof(UINT32));
        if (0 == apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress)
        {
            Debug_Printf("*** Dhcp server sent zero as offered address\n");
            return;
        }
        NetDev_Ip_GetCurrent(apNetDev, NULL, &u32, NULL);
        if (u32 == apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress)
        {
            Debug_Printf("*** Dhcp server sent broadcast IP address as offered IP address\n");
            return;
        }
        
        // set up to scan options, bypassing magic cookie already verified by caller
        pOptions = apRecvData + DHCP_PACKET_OFFSET_OPTIONS + 4;
        optLeft = aRecvLen - (DHCP_PACKET_OFFSET_OPTIONS + 4);
        do {
            opt = *pOptions;
            pOptions++;
            --optLeft;
            if (opt != DHCP_OPTION_PAD)
            {
                if (opt == DHCP_OPTION_END)
                    break;

                optLen = *pOptions;
                pOptions++;
                optLeft--;
                if (optLeft < optLen)
                {
                    Debug_Printf("*** Dhcp Option length greater than remaining option space\n");
                    return;
                }

                pOpt = pOptions;
                pOptions += optLen;
                optLeft -= optLen;

                switch (opt)
                {
                case DHCP_OPTION_SUBNET_MASK:
                    if (optLen != 4)
                    {
                        Debug_Printf("*** Dhcp Server sent subnet mask that is wrong length\n");
                        return;
                    }
                    K2MEM_Copy(&apNetDev->Proto.Ip.Udp.Dhcp.Host.mSubnetMask, pOpt, sizeof(UINT32));
                    break;

                case DHCP_OPTION_ROUTERS:
                    if (optLen < 4)
                    {
                        Debug_Printf("*** Dhcp Server sent routers list that is too short\n");
                        return;
                    }
                    K2MEM_Copy(&apNetDev->Proto.Ip.Udp.Dhcp.Host.mDefaultGateway, pOpt, sizeof(UINT32));
                    break;

                case DHCP_OPTION_DNS_SERVERS:
                    if (optLen < 4)
                    {
                        Debug_Printf("*** Dhcp Server sent DNS servers list that is too short\n");
                        return;
                    }
                    K2MEM_Copy(&apNetDev->Proto.Ip.Udp.Dhcp.Host.mPrimaryDNS, pOpt, sizeof(UINT32));
                    pOpt += 4;
                    optLen -= 4;
                    if (optLen < 4)
                        break;
                    K2MEM_Copy(&apNetDev->Proto.Ip.Udp.Dhcp.Host.mSecondaryDNS, pOpt, sizeof(UINT32));
                    break;

                case DHCP_OPTION_DOMAIN_NAME:
                    u32 = optLen;
                    if (u32 > K2OS_IPV4_ADAPTER_HOST_NAME_MAX_LEN)
                        u32 = K2OS_IPV4_ADAPTER_HOST_NAME_MAX_LEN;
                    K2ASC_CopyLen(apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mDomain, (char const *)pOpt, u32);
                    apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mDomain[K2OS_IPV4_ADAPTER_HOST_NAME_MAX_LEN] = 0;
                    break;

                case DHCP_OPTION_SERVER_ID:
                    if (optLen != 4)
                    {
                        Debug_Printf("*** Dhcp Server sent Server IP that is wrong length\n");
                        return;
                    }
                    K2MEM_Copy(&apNetDev->Proto.Ip.Udp.Dhcp.mDhcpServerIp, pOpt, sizeof(UINT32));
                    break;

                case DHCP_OPTION_T1_VALUE:
                    if (optLen != 4)
                    {
                        Debug_Printf("*** Dhcp Server sent T1 value that is wrong length\n");
                        return;
                    }
                    K2MEM_Copy(&u32, pOpt, sizeof(UINT32));
                    u32 = K2_SWAP32(u32);
                    apNetDev->Proto.Ip.Udp.Dhcp.mMsT1 = 1000 * u32;
                    break;

                case DHCP_OPTION_T2_VALUE:
                    if (optLen != 4)
                    {
                        Debug_Printf("*** Dhcp Server sent T2 value that is wrong length\n");
                        return;
                    }
                    K2MEM_Copy(&u32, pOpt, sizeof(UINT32));
                    u32 = K2_SWAP32(u32);
                    apNetDev->Proto.Ip.Udp.Dhcp.mMsT2 = 1000 * u32;
                    break;

                default:
                    // ignore
                    break;
                }
            }
        } while (0 != optLeft);

        if ((0 == apNetDev->Proto.Ip.Udp.Dhcp.mDhcpServerIp) ||
            (0 == apNetDev->Proto.Ip.Udp.Dhcp.Host.mIpAddress) ||
            (0 == apNetDev->Proto.Ip.Udp.Dhcp.Host.mSubnetMask) ||
            (0 == apNetDev->Proto.Ip.Udp.Dhcp.Host.mDefaultGateway) ||
            (0 == apNetDev->Proto.Ip.Udp.Dhcp.mMsT1) ||
            (0 == apNetDev->Proto.Ip.Udp.Dhcp.mMsT2))
        {
            Debug_Printf("*** Dhcp Server did not give some required options\n");
            return;
        }

        NetDev_DelTimer(apNetDev, apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);
        apNetDev->Proto.Ip.Udp.Dhcp.mpTimer = NULL;

        Dhcp_Enter_Selecting(apNetDev);
        return;
    }

    K2_ASSERT(aEvent == NETDEV_Dhcp_Event_Timeout);

    K2_ASSERT(NULL != apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);
    if (0 == apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter)
    {
        NetDev_DelTimer(apNetDev, apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);
        apNetDev->Proto.Ip.Udp.Dhcp.mpTimer = NULL;
        Dhcp_Enter_Init(apNetDev);
    }
    else
    {
        --apNetDev->Proto.Ip.Udp.Dhcp.mRetryCounter;
    }
}

void
Dhcp_Enter_Init_Reboot(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.Ip.Udp.Dhcp.mState = NETDEV_Dhcp_State_Init_Reboot;
}

void
DhcpState_Init_Reboot(
    NETDEV *                apNetDev,
    NETDEV_Dhcp_EventType   aEvent,
    UINT8 const *           apRecvData,
    UINT32                  aRecvLen
)
{
    K2_ASSERT(NETDEV_Dhcp_State_Init_Reboot == apNetDev->Proto.Ip.Udp.Dhcp.mState);
}

void
NetDev_Dhcp_Event(
    NETDEV *                apNetDev,
    NETDEV_Dhcp_EventType   aEvent,
    UINT8 const *           apDhcpPacket,
    UINT32                  aDhcpPacketLen
)
{
    if (aEvent == NETDEV_Dhcp_Event_Reset)
    {
//        Debug_Printf("Dhcp-Reset\n");
        if (NULL != apNetDev->Proto.Ip.Udp.Dhcp.mpTimer)
        {
            NetDev_DelTimer(apNetDev, apNetDev->Proto.Ip.Udp.Dhcp.mpTimer);
        }
        K2MEM_Zero(&apNetDev->Proto.Ip.Udp.Dhcp, sizeof(NETDEV_DHCP_PROTO));
        apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mBroadcastAddr = 0xFFFFFFFF;
        apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mInitialTTL = K2OS_IPV4_DEFAULT_IP_TTL_VALUE;
        apNetDev->Proto.Ip.Udp.Dhcp.IpParam.mInterfaceMTU = apNetDev->Proto.mpL2->mClientMTU;
        apNetDev->Proto.Ip.Adapter.Current.Dhcp.mIsBound = FALSE;
        if (apNetDev->Proto.Ip.Adapter.Config.mUseDhcp)
        {
            apNetDev->Proto.Ip.Adapter.Current.Dhcp.mInUse = TRUE;
            Dhcp_Enter_Init(apNetDev);
        }
        return;
    }

    switch (apNetDev->Proto.Ip.Udp.Dhcp.mState)
    {
    case NETDEV_Dhcp_State_Init_Reboot:
        DhcpState_Init_Reboot(apNetDev, aEvent, apDhcpPacket, aDhcpPacketLen);
        break;

    case NETDEV_Dhcp_State_Init:
        DhcpState_Init(apNetDev, aEvent, apDhcpPacket, aDhcpPacketLen);
        break;

    case NETDEV_Dhcp_State_Selecting:
        DhcpState_Selecting(apNetDev, aEvent, apDhcpPacket, aDhcpPacketLen);
        break;

    case NETDEV_Dhcp_State_ArpCheck:
        DhcpState_ArpCheck(apNetDev, aEvent, apDhcpPacket, aDhcpPacketLen);
        break;

    case NETDEV_Dhcp_State_Bound:
        DhcpState_Bound(apNetDev, aEvent, apDhcpPacket, aDhcpPacketLen);
        break;

    case NETDEV_Dhcp_State_Renewing:
        DhcpState_Renewing(apNetDev, aEvent, apDhcpPacket, aDhcpPacketLen);
        break;

    case NETDEV_Dhcp_State_Rebinding:
        DhcpState_Rebinding(apNetDev, aEvent, apDhcpPacket, aDhcpPacketLen);
        break;

    default:
        K2_ASSERT(0);
    }
}
