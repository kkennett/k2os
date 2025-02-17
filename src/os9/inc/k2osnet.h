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
#ifndef __K2OSNET_H
#define __K2OSNET_H

#include <k2osdev_netio.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

// {0B3DDFBD-E569-415F-A5B9-CDE9623E832D}
#define K2OS_IFACE_NETWORK   { 0xb3ddfbd, 0xe569, 0x415f, { 0xa5, 0xb9, 0xcd, 0xe9, 0x62, 0x3e, 0x83, 0x2d } }

#define K2OS_IPV4_ADAPTER_HOST_NAME_MAX_LEN             31
#define K2OS_IPV4_PARAM_MAX_NUM_TIME_SERVERS            4
#define K2OS_IPV4_DEFAULT_IP_TTL_VALUE                  128
#define K2OS_IPV4_DEFAULT_TCP_TTL_VALUE                 255
#define K2OS_IPV4_DEFAULT_UDP_TTL_VALUE                 32
#define K2OS_IPV4_DEFAULT_TCP_KEEPALIVE_INTERVAL_MS     (2 * 60 * 60 * 1000)

typedef struct _K2OS_IPV4_ADAPTER_CONFIG    K2OS_IPV4_ADAPTER_CONFIG;
typedef struct _K2OS_IPV4_ADAPTER_ASSIGN    K2OS_IPV4_ADAPTER_ASSIGN;
typedef struct _K2OS_IPV4_STATIC_ROUTE      K2OS_IPV4_STATIC_ROUTE;
typedef struct _K2OS_IPV4_PARAM             K2OS_IPV4_PARAM;

typedef struct _K2OS_NET_OPAQUE     K2OS_NET_OPAQUE;
typedef K2OS_NET_OPAQUE *           K2OS_NET;

struct _K2OS_IPV4_STATIC_ROUTE
{
    UINT32      mDestIpAddr;    // send this
    UINT32      mRouterIp;      // here
    K2LIST_LINK ListLink;
};

struct _K2OS_IPV4_PARAM
{
    char            mDomain[K2OS_IPV4_ADAPTER_HOST_NAME_MAX_LEN + 1];
    INT32           mTimeOffset;        // seconds from UTC (+/-)
    UINT32          mNumTimeServers;
    UINT32          mTimeServers[K2OS_IPV4_PARAM_MAX_NUM_TIME_SERVERS];
    UINT8           mInitialTTL;
    UINT8           mEnableIpForwarding;
    UINT16          mInterfaceMTU;
    UINT32          mBroadcastAddr;
    UINT32          mArpTimeoutMs;
    UINT8           mTcpInitialTTL;
    UINT32          mTcpKeepAliveMsInterval;
    K2LIST_ANCHOR   StaticRouteList;
};

struct _K2OS_IPV4_ADAPTER_CONFIG
{
    char    mHostName[K2OS_IPV4_ADAPTER_HOST_NAME_MAX_LEN + 1];
    BOOL    mUseDhcp;

    // only valid if mUseDchp == FALSE
    struct {
        IPV4_HOST       Host;     
        K2OS_IPV4_PARAM IpParam;
    } Static;
};

struct _K2OS_IPV4_ADAPTER_ASSIGN
{
    K2_NET_ADAPTER_DESC         AdapterDesc;    // adapter matching this desc
    K2OS_IPV4_ADAPTER_CONFIG    Config;         // has this configuration

    K2OS_IFINST_ID              mNetIo_IfInstanceId;    // if 0 netadapter is NP
    K2OS_IFINST_ID              mIpv4_IfInstancdId;     // if 0 adapter is NP
};

K2OS_NET    K2OS_Net_Attach(K2OS_IFINST_ID aIfInstId);
BOOL        K2OS_Net_Assign(K2OS_NET aNet, K2OS_IFINST_ID aNetAdapterIfInstId, K2OS_IPV4_ADAPTER_CONFIG const *apConfig);
BOOL        K2OS_Net_Unassign(K2OS_NET aNet, K2OS_IFINST_ID aNetAdapterIfInstId);
BOOL        K2OS_Net_Enum(K2OS_NET aNet, UINT32 *apBufCountIo, K2OS_IPV4_ADAPTER_ASSIGN *apBuffer);
BOOL        K2OS_Net_Detach(K2OS_NET aNet);

//
//------------------------------------------------------------------------
//

// {26622CF9-D775-42E3-90E3-ABA0037326AD}
#define K2OS_IFACE_IPV4_DEVICE   { 0x26622cf9, 0xd775, 0x42e3, { 0x90, 0xe3, 0xab, 0xa0, 0x3, 0x73, 0x26, 0xad } }

typedef struct _K2OS_IPV4_ADAPTER   K2OS_IPV4_ADAPTER;

typedef struct _K2OS_IPV4_OPAQUE    K2OS_IPV4_OPAQUE;
typedef K2OS_IPV4_OPAQUE *          K2OS_IPV4;

struct _K2OS_IPV4_ADAPTER
{
    K2OS_IFINST_ID  mIfInstanceId;
    BOOL            mIsOperating;
    K2OS_IFINST_ID  mNetIo_IfInstanceId;

    // host name is always Config.mHostName regardless of DHCP

    struct {
        IPV4_HOST       Host;
        K2OS_IPV4_PARAM IpParam;
        struct {
            BOOL    mInUse;
            BOOL    mIsBound;
            UINT32  mServerIpAddress;
            UINT64  mLeaseTimeObtained;
            UINT64  mLeaseTimeExpires;
        } Dhcp;
    } Current;

    K2OS_IPV4_ADAPTER_CONFIG Config;
};

K2OS_IPV4   K2OS_Ipv4_Attach(K2OS_IFINST_ID aIpv4AdapterIfInstId, void *apContext, K2OS_MAILBOX_TOKEN aTokMailbox);
BOOL        K2OS_Ipv4_Detach(K2OS_IPV4 aIpv4);
BOOL        K2OS_Ipv4_GetAdapter(K2OS_IPV4 aIpv4, K2OS_IPV4_ADAPTER *apRetAdapter);
BOOL        K2OS_Ipv4_SetAdapterConfig(K2OS_IPV4 aIpv4, K2OS_IPV4_ADAPTER_CONFIG const *apConfig);

//
//------------------------------------------------------------------------
//

typedef enum _K2OS_SocketType K2OS_SocketType;
enum _K2OS_SocketType {
    K2OS_SocketInvalid,

    K2OS_SocketTcp, // no header
    K2OS_SocketUdp, // send/recv frames have UDP header
    K2OS_SocketIp,  // send/recv frames have IP header

    K2OS_SocketType_Count
};

typedef struct _K2OS_SOCKET_STATE   K2OS_SOCKET_STATE;
typedef union  _K2OS_SOCKET_ADDR    K2OS_SOCKET_ADDR;

typedef struct _K2OS_SOCKET_OPAQUE  K2OS_SOCKET_OPAQUE;
typedef K2OS_SOCKET_OPAQUE *        K2OS_SOCKET;

struct _K2OS_SOCKET_STATE
{
    K2OS_SocketType mSocketType;
    void *          mpContext;
    BOOL            mIsBound;
    K2OS_IFINST_ID  mNetAdapter_IfInstanceId;   // 0 if unbound
    union {
        struct {
            UINT16  mPort;
        } Tcp;
        struct {
            UINT16  mPort;
        } Udp;
    } Param;
    UINT16          mAlign;
    struct {
        BOOL mIsListening;
        BOOL mIsConnected;
    } Tcp;
};

union _K2OS_SOCKET_ADDR
{
    struct {
        UINT32 mIpAddress;
        union {
            struct {
                UINT16 mPort;
            } Tcp;
            struct {
                UINT16 mPort;
            } Udp;
        };
        UINT16 mAlign;
    } Ipv4;
};

K2OS_SOCKET K2OS_Socket_Create(K2OS_SocketType aType, void *apContext, K2OS_MAILBOX_TOKEN aTokMailbox);
BOOL        K2OS_Socket_GetState(K2OS_SOCKET aSocket, K2OS_SOCKET_STATE *apRetState);

BOOL        K2OS_Socket_Bind(K2OS_SOCKET aSocket, K2OS_IFINST_ID aIpv4AdapterIfInstanceId, UINT32 aParam);  // aParam is Port for Ip socket types
BOOL        K2OS_Socket_Unbind(K2OS_SOCKET aSocket);

BOOL        K2OS_Socket_Shutdown(K2OS_SOCKET aSocket, BOOL aShutSend, BOOL aShutRecv);
BOOL        K2OS_Socket_Destroy(K2OS_SOCKET aSocket);

UINT8 *     K2OS_Socket_GetBuffer(K2OS_SOCKET aSocket, UINT32 *apRetMTU);
BOOL        K2OS_Socket_SendBuffer(UINT8 *apBuffer, UINT32 aSendBytes);
BOOL        K2OS_Socket_SendBufferTo(K2OS_SOCKET_ADDR const *apAddr, UINT8 *apBuffer, UINT32 aSendBytes);
BOOL        K2OS_Socket_ReleaseBuffer(UINT8 *apBuffer);

BOOL        K2OS_Socket_Connect(K2OS_SOCKET aSocket, K2OS_SOCKET_ADDR const *apRemoteAddr);

BOOL        K2OS_Socket_Listen(K2OS_SOCKET aSocket, UINT16 aPort, UINT32 aBacklog);
K2OS_SOCKET K2OS_Socket_Accept(K2OS_SOCKET aSocket, K2OS_SOCKET_ADDR *apRetRemoteAddr);

BOOL        K2OS_Socket_Disconnect(K2OS_SOCKET aSocket);

//
//------------------------------------------------------------------------
//

#define K2OS_NET_METHOD_ASSIGN      1
// input
typedef struct _K2OS_NET_ASSIGN_IN K2OS_NET_ASSIGN_IN;
struct _K2OS_NET_ASSIGN_IN
{
    K2OS_IFINST_ID              mNetAdapterIfInstId;
    K2OS_IPV4_ADAPTER_CONFIG    Config;
};
// output nothing

#define K2OS_NET_METHOD_UNASSIGN    2
// input single K2OS_IFINST_ID of NetAdapter IfInstance to unassign
// output nothing

#define K2OS_NET_METHOD_ENUM        3
// input single UINT32 max # to return
// output 
//     single UINT32 listing # of assigned adapters
//     followed by array of K2OS_IPV4_ADAPTER_ASSIGN up to maximum count listed in input

//
//------------------------------------------------------------------------
//

#define K2OS_IPV4_METHOD_CONFIG     1
// input
typedef struct _K2OS_IPV4_CONFIG_IN     K2OS_IPV4_CONFIG_IN;
struct _K2OS_IPV4_CONFIG_IN
{
    void *              mpContext;
    K2OS_MAILBOX_TOKEN  mTokMailbox;
};
// output nothing

#define K2OS_IPV4_METHOD_GETADAPTER 2
// input nothing
// output K2OS_IPV4_ADAPTER

#define K2OS_IPV4_METHOD_SETADAPTER 3
// input K2OS_IPV4_ADAPTER
// output nothing

#if __cplusplus
}
#endif

#endif // __K2OSNET_H