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

#ifndef __IK2PPPLINK_H
#define __IK2PPPLINK_H

#include <lib/k2win32.h>

#include <lib/k2mem.h>
#include <lib/k2ppplink.h>
#include <lib/k2nethost.h>

typedef enum _K2PPP_OidAutoEventType K2PPP_OidAutoEventType;
enum _K2PPP_OidAutoEventType
{
    K2PPP_OidAutoEvent_Invalid = 0,

    K2PPP_OidAutoEvent_LowerLayerIsUp,
    K2PPP_OidAutoEvent_LowerLayerIsDown,
    K2PPP_OidAutoEvent_AdminOpen,
    K2PPP_OidAutoEvent_AdminClose,
    K2PPP_OidAutoEvent_TO_NONZERO,       // Timeout when counter > 0
    K2PPP_OidAutoEvent_TO_ZERO,          // Timeout with counter expired
    K2PPP_OidAutoEvent_RCR_GOOD,         // Receive-Configure-Request (good)
    K2PPP_OidAutoEvent_RCR_BAD,          // Receive-Configure-Request (bad)
    K2PPP_OidAutoEvent_RCA,              // Receive-Configure-Ack
    K2PPP_OidAutoEvent_RCN,              // Receive-Configure-Nak/rej
    K2PPP_OidAutoEvent_RTR,              // Receive Terminate Request
    K2PPP_OidAutoEvent_RTA,              // Receive Terminate Ack
    K2PPP_OidAutoEvent_RUC,              // Receive-Unknown-Code
    K2PPP_OidAutoEvent_RXJ_GOOD,         // Receive code reject (permitted) or Receive protocol reject
    K2PPP_OidAutoEvent_RXJ_BAD,          // Receive code reject (catastrophic) or receive protocol reject
    K2PPP_OidAutoEvent_RXR,              // Receive echo request or receive echo reply or receive discard request

    K2PPP_OidAutoEventType_Count
};

typedef void (*K2PPP_OIDAUTO_pf_StateFunc)(K2PPP_OIDAUTO *apOidAuto, K2PPP_OidAutoEventType aOidAutoEvent);

void K2PPP_OIDAUTO_Init(K2PPP_OIDAUTO *apAuto);
void K2PPP_OIDAUTO_Run(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);

BOOL K2PPP_ProtoOk(K2_PPP_LinkPhaseType aLinkPhase, UINT32 aProtocol);

void K2PPP_OID_Init(K2PPP_OID * apOid, UINT32 aProtocolId);

K2NET_HOST_LAYER * K2PPP_StackFactory(UINT32 aProtocolId, K2NET_STACK_PROTO_CHUNK_HDR const * apConfig);
K2NET_HOST_LAYER * K2PPP_LcpFactory(UINT32 aProtocolId, K2NET_STACK_PROTO_CHUNK_HDR const * apConfig);
K2NET_HOST_LAYER * K2PPP_IpcpFactory(UINT32 aProtocolId, K2NET_STACK_PROTO_CHUNK_HDR const * apConfig);
K2NET_HOST_LAYER * K2PPP_Ipv4Factory(UINT32 aProtocolId, K2NET_STACK_PROTO_CHUNK_HDR const * apConfig);

extern char const * gK2PPP_OidAutoEventNames[K2PPP_OidAutoEventType_Count];

#endif // __IK2PPPLINK_H