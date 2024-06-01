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

#ifndef __NETMGR_H
#define __NETMGR_H

#include "sysproc.h"
#include <k2osnet.h>

#if __cplusplus
extern "C" {
#endif

//
// -------------------------------------------------------------------------
// 

typedef enum _NETDEV_Dhcp_StateType NETDEV_Dhcp_StateType;
enum _NETDEV_Dhcp_StateType
{
    NETDEV_Dhcp_State_Invalid = 0,

    NETDEV_Dhcp_State_Init_Reboot,
    NETDEV_Dhcp_State_Init,
    NETDEV_Dhcp_State_Selecting,
    NETDEV_Dhcp_State_ArpCheck,
    NETDEV_Dhcp_State_Bound,
    NETDEV_Dhcp_State_Renewing,
    NETDEV_Dhcp_State_Rebinding,

    NETDEV_Dhcp_StateType_Count
};

typedef enum _NETDEV_Dhcp_EventType NETDEV_Dhcp_EventType;
enum _NETDEV_Dhcp_EventType
{
    NETDEV_Dhcp_Event_Invalid = 0,

    NETDEV_Dhcp_Event_Reset,
    NETDEV_Dhcp_Event_Timeout,
    NETDEV_Dhcp_Event_Recv_DHCPOFFER,
    NETDEV_Dhcp_Event_Recv_DHCPACK,
    NETDEV_Dhcp_Event_Recv_DHCPNACK,

    NETDEV_Dhcp_EventType_Count
};

typedef struct _NETDEV                  NETDEV;
typedef struct _NETDEV_TIMER            NETDEV_TIMER;
typedef struct _NETDEV_PROTO            NETDEV_PROTO;
typedef struct _NETDEV_BUFFER           NETDEV_BUFFER;

typedef struct _NETDEV_L2_PROTO         NETDEV_L2_PROTO;

typedef struct _NETDEV_IP_PROTO         NETDEV_IP_PROTO;
typedef struct _NETDEV_ARP_PROTO        NETDEV_ARP_PROTO;

typedef struct _NETDEV_UDP_PROTO        NETDEV_UDP_PROTO;
typedef struct _NETDEV_TCP_PROTO        NETDEV_TCP_PROTO;
typedef struct _NETDEV_ICMP_PROTO       NETDEV_ICMP_PROTO;

typedef struct _NETDEV_DHCP_PROTO       NETDEV_DHCP_PROTO;
typedef struct _NETDEV_DNS_PROTO        NETDEV_DNS_PROTO;

typedef struct _NETDEV_UDP_ADDR         NETDEV_UDP_ADDR;

typedef void            (*NETDEV_TIMER_pf_Callback)(NETDEV *apNetDev, NETDEV_TIMER *apTimer);

typedef void            (*NETDEV_L2_pf_OnStart)(NETDEV *apNetDev);
typedef NETDEV_BUFFER * (*NETDEV_L2_pf_AcqSendBuffer)(NETDEV *apNetDev, UINT32 *apRetMTU);
typedef BOOL            (*NETDEV_L2_pf_SendBuffer)(NETDEV *apNetDev, UINT8 const *apDstHwAddr, UINT32 aDstProtoId, NETDEV_BUFFER *apBuffer);
typedef void            (*NETDEV_L2_pf_OnRecv)(NETDEV *apNetDev, UINT8 const *apData, UINT32 aDataLen);
typedef void            (*NETDEV_L2_pf_OnTimeExpired)(NETDEV *apNetDev, UINT32 aExpiredMs);
typedef void            (*NETDEV_L2_pf_OnStop)(NETDEV *apNetDev);
typedef void            (*NETDEV_L2_pf_Deinit)(NETDEV *apNetDev);
typedef void            (*NETDEV_L2_pf_RelBuffer)(NETDEV_BUFFER *apBuffer);

typedef void            (*NETDEV_L3_pf_OnRecv)(NETDEV *apNetDev, UINT8 const *apL2SrcAddr, UINT8 const *apData, UINT32 aDataLen);

typedef void            (*NETDEV_L4_pf_OnRecv)(NETDEV *apNetDev, UINT32 aSrcIpAddr, UINT32 aDstIpAddr, UINT8 const *apData, UINT32 aDataLen);

struct _NETDEV_TIMER
{
    UINT32                      mLeft;
    UINT32                      mPeriod;
    NETDEV_TIMER_pf_Callback    mfCallback;
    NETDEV_TIMER *              mpNext;
};

struct _NETDEV_BUFFER
{
    NETDEV *        mpNetDev;
    UINT32          mMsTTL;
    UINT8 *         mpData;
    UINT32          mDataLen;
    NETDEV_BUFFER * mpNext;
};

struct _NETDEV_UDP_ADDR
{
    UINT32  mIpAddr;
    UINT16  mPort;
    UINT16  mAlign;
};

struct _NETDEV_DNS_PROTO
{
    int dummy;
};

struct _NETDEV_DHCP_PROTO
{
    NETDEV_Dhcp_StateType       mState;
    UINT32                      mDhcpServerIp;
    IPV4_HOST                   Host;
    K2OS_IPV4_PARAM             IpParam;
    UINT32                      mMsT1;
    UINT32                      mMsT2;
    UINT32                      mRetryCounter;
    UINT32                      mLastXID;
    NETDEV_TIMER *              mpTimer;
};

struct _NETDEV_UDP_PROTO
{
    UINT8               mInitialTTL;
    NETDEV_DHCP_PROTO   Dhcp;
    NETDEV_DNS_PROTO    Dns;
};

struct _NETDEV_TCP_PROTO
{
    UINT8   mInitialTTL;
};

struct _NETDEV_ICMP_PROTO
{
    UINT16  mNextIdent;
    UINT16  mNextSeqNo;
};

struct _NETDEV_ARP_PROTO
{
    UINT32              mL2ProtoId;         // L2-type-relative protocol id (PTYPE)
    K2TREE_ANCHOR       EntryTree;          // Tree of NETDEV_ARP_ENTRY.EntryTreeNode; mUserVal is ip address
    UINT32              mMsInitialTTL;
    NETDEV_TIMER *      mpTimer;
};

struct _NETDEV_IP_PROTO
{
    K2OS_IPV4_ADAPTER   Adapter;
    UINT16              mNextIdent;
    UINT32              mMsInitialMfTTL;
    K2TREE_ANCHOR       MfTree;
    UINT32              mL2ProtoId;         // L2-type-relative protocol id (PTYPE)
    NETDEV_UDP_PROTO    Udp;
    NETDEV_TCP_PROTO    Tcp;
    NETDEV_ICMP_PROTO   Icmp;
};

struct _NETDEV_L2_PROTO
{
    NETDEV *    mpNetDev;
    BOOL        mIsStarted;
    UINT32      mHTYPE;         // should match mpNetDev->Desc.mType
    UINT32      mClientMTU;
    UINT8       mHwBroadcastAddr[K2_NET_ADAPTER_ADDR_MAX_LEN];
    struct {
        NETDEV_L2_pf_OnStart        OnStart;
        NETDEV_L2_pf_AcqSendBuffer  AcqSendBuffer;
        NETDEV_L2_pf_SendBuffer     SendBuffer;
        NETDEV_L2_pf_OnRecv         OnRecv;
        NETDEV_L2_pf_OnTimeExpired  OnTimeExpired;
        NETDEV_L2_pf_OnStop         OnStop;
        NETDEV_L2_pf_Deinit         Deinit;
        NETDEV_L2_pf_RelBuffer      RelBuffer;
    } Iface;
};

struct _NETDEV_PROTO
{
    NETDEV_L2_PROTO *   mpL2;
    NETDEV_ARP_PROTO    Arp;
    NETDEV_IP_PROTO     Ip;
};

struct _NETDEV
{
    K2OS_IFINST_ID      mIfInstId;
    K2OS_NETIO          mNetIo;
    K2_NET_ADAPTER_DESC Desc;
    K2LIST_LINK         ListLink;
    K2OS_MAILBOX_TOKEN  mTokMailbox;
    K2OS_SIGNAL_TOKEN   mTokDoneGate;
    UINT32              mThreadId;
    BOOL                mStop;
    NETDEV_TIMER *      mpTimers;
    NETDEV_BUFFER *     mpFreeBufFirst;
    NETDEV_BUFFER *     mpFreeBufLast;
    NETDEV_PROTO        Proto;
};

BOOL                NetDev_Init(NETDEV *apNetDev, K2OS_IPV4_ADAPTER_CONFIG const *apConfig);
void                NetDev_Start(NETDEV * apNetDev);
void                NetDev_OnTimeExpired(NETDEV *apNetDev, UINT32 aMsTicks);
void                NetDev_Recv_NetIo(NETDEV * apNetDev, K2OS_NETIO_MSG const *  apNetIoMsg);
void                NetDev_Recv_RpcNotify(NETDEV *apNetDev, K2OS_MSG const *apRpcNotifyMsg);
void                NetDev_Stopped(NETDEV * apNetDev);
void                NetDev_Deinit(NETDEV * apNetDev);
NETDEV_TIMER *      NetDev_AddTimer(NETDEV *apNetDev, UINT32 aPeriod, NETDEV_TIMER_pf_Callback afCallback);
void                NetDev_DelTimer(NETDEV *apNetDev, NETDEV_TIMER *apTimer);
NETDEV_BUFFER *     NetDev_BufferGet(NETDEV *apNetDev);
void                NetDev_BufferPut(NETDEV_BUFFER *apBuffer);
void                NetDev_RelBuffer(NETDEV_BUFFER *apBuffer);

NETDEV_L2_PROTO * NetDev_Ether_Create(NETDEV *apNetDev, K2OS_IPV4_ADAPTER_CONFIG const *apConfig);
NETDEV_L2_PROTO * NetDev_PPP_Create(NETDEV *apNetDev, K2OS_IPV4_ADAPTER_CONFIG const *apConfig);

BOOL    NetDev_Arp_Init(NETDEV *apNetDev, UINT32 aL2ProtoId);
void    NetDev_Arp_OnStart(NETDEV *apNetDev);
void    NetDev_Arp_OnRecv(NETDEV *apNetDev, UINT8 const *apFromL2Addr, UINT8 const *apData, UINT32 aDataLen);
void    NetDev_Arp_OnTimeExpired(NETDEV *apNetDev, UINT32 aExpiredMs);
void    NetDev_Arp_Update(NETDEV *apNetDev, UINT32 aIpAddr, UINT8 const *apHwAddr);
void    NetDev_Arp_Expire(NETDEV *apNetDev, UINT32 aExpireMs);
void    NetDev_Arp_Clear(NETDEV *apNetDev, UINT32 aIpAddr);
BOOL    NetDev_Arp_Resolve(NETDEV *apNetDev, UINT32 aIpAddr, UINT8 const **appRetHwAddr, BOOL aNoSend);
BOOL    NetDev_Arp_Queue(NETDEV *apNetDev, UINT32 aTargetIpAddr, NETDEV_BUFFER *apBuffer);
void    NetDev_Arp_OnStop(NETDEV *apNetDev);
void    NetDev_Arp_Deinit(NETDEV *apNetDev);

BOOL    NetDev_Ip_Init(NETDEV *apNetDev, UINT32 aL2ProtoId, K2OS_IPV4_ADAPTER_CONFIG const *apConfig);
UINT16  NetDev_Ip_CalcHdrChecksum(UINT16 *apIpHdr);
void    NetDev_Ip_GetCurrent(NETDEV *apNetDev, UINT32 *apRetCurrentIp, UINT32 *apRetCurrentBcastIp, UINT8 *apRetCurrentTTL);
void    NetDev_Ip_OnStart(NETDEV *apNetDev);
void    NetDev_Ip_OnRecv(NETDEV *apNetDev, UINT8 const *apFromL2Addr, UINT8 const *apData, UINT32 aDataLen);
BOOL    NetDev_Ip_Send(NETDEV *apNetDev, UINT32 const *apTargetIp, UINT8 aTargetIpProto, BOOL aDoNotFragment, UINT8 const *apData, UINT32 aDataLen);
void    NetDev_Ip_OnTimeExpired(NETDEV *apNetDev, UINT32 aExpiredMs);
void    NetDev_Ip_OnStop(NETDEV *apNetDev);
void    NetDev_Ip_Deinit(NETDEV *apNetDev);
UINT32  NetDev_Ip_GetRoute(NETDEV *apNetDev, UINT32 aTargetIp);

BOOL    NetDev_Udp_Init(NETDEV *apNetDev);
UINT16  NetDev_Udp_CalcHdrChecksum(UINT16 *apPseudo, UINT16 *apUdpPacket, UINT16 aUdpLength);
void    NetDev_Udp_OnStart(NETDEV *apNetDev);
void    NetDev_Udp_OnRecv(NETDEV *apNetDev, UINT32 aSrcIpAddr, UINT32 aDstIpAddr, UINT8 const *apData, UINT32 aDataLen);
BOOL    NetDev_Udp_Send(NETDEV *apNetDev, UINT32 const *apTargetIp, UINT16 aSrcPort, UINT16 aDstPort, BOOL aDoNotFragment, UINT8 *apFullUdpPacketBuffer, UINT16 aFullUdpPacketLen);

void    NetDev_Udp_OnStop(NETDEV *apNetDev);
void    NetDev_Udp_Deinit(NETDEV *apNetDev);

BOOL    NetDev_Tcp_Init(NETDEV *apNetDev);
void    NetDev_Tcp_OnStart(NETDEV *apNetDev);
void    NetDev_Tcp_OnRecv(NETDEV *apNetDev, UINT32 aSrcIpAddr, UINT32 aDstIpAddr, UINT8 const *apData, UINT32 aDataLen);
void    NetDev_Tcp_OnStop(NETDEV *apNetDev);
void    NetDev_Tcp_Deinit(NETDEV *apNetDev);

BOOL    NetDev_Icmp_Init(NETDEV *apNetDev);
UINT16  NetDev_Icmp_CalcPacketChecksum(UINT16 *apIcmpPacket, UINT16 aLength);
void    NetDev_Icmp_OnStart(NETDEV *apNetDev);
void    NetDev_Icmp_OnRecv(NETDEV *apNetDev, UINT32 aSrcIpAddr, UINT32 aDstIpAddr, UINT8 const *apData, UINT32 aDataLen);
void    NetDev_Icmp_OnStop(NETDEV *apNetDev);
void    NetDev_Icmp_Deinit(NETDEV *apNetDev);
void    NetDev_Icmp_SendPing(NETDEV *apNetDev, UINT32 aTargetIpAddr);

BOOL    NetDev_Dhcp_Init(NETDEV *apNetDev);
void    NetDev_Dhcp_OnStart(NETDEV *apNetDev);
void    NetDev_Dhcp_OnRecv(NETDEV *apNetDev, NETDEV_UDP_ADDR const *apSrcUdpAddr, NETDEV_UDP_ADDR const *apDstUdpAddr, UINT8 const *apData, UINT32 aDataLen);
void    NetDev_Dhcp_OnStop(NETDEV *apNetDev);
void    NetDev_Dhcp_Deinit(NETDEV *apNetDev);

BOOL    NetDev_Dns_Init(NETDEV *apNetDev);
void    NetDev_Dns_OnStart(NETDEV *apNetDev);
void    NetDev_Dns_OnRecv(NETDEV *apNetDev, NETDEV_UDP_ADDR const *apSrcUdpAddr, NETDEV_UDP_ADDR const *apDstUdpAddr, UINT8 const *apData, UINT32 aDataLen);
void    NetDev_Dns_OnStop(NETDEV *apNetDev);
void    NetDev_Dns_Deinit(NETDEV *apNetDev);

//
// -------------------------------------------------------------------------
// 

#if __cplusplus
}
#endif

#endif // __NETMGR_H