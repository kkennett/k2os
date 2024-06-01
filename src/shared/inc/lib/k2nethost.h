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
#ifndef __K2NETHOST_H
#define __K2NETHOST_H

#include <lib/k2net.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _K2NET_OPEN_CONTEXT_OPAQUE   K2NET_OPEN_CONTEXT_OPAQUE;
typedef struct _K2NET_PROTO_CONTEXT_OPAQUE  K2NET_PROTO_CONTEXT_OPAQUE;
typedef struct _K2NET_OPEN_INSTANCE_OPAQUE  K2NET_OPEN_INSTANCE_OPAQUE;
typedef struct _K2NET_STACK                 K2NET_STACK;
typedef struct _K2NET_PROTO_INSTANCE        K2NET_PROTO_INSTANCE;
typedef struct _K2NET_PROTO_INSTANCE_DESC   K2NET_PROTO_INSTANCE_DESC;
typedef struct _K2NET_PROTO_HOST            K2NET_PROTO_HOST;

typedef K2NET_OPEN_CONTEXT_OPAQUE *     K2NET_OPEN_CONTEXT;
typedef K2NET_PROTO_CONTEXT_OPAQUE *    K2NET_PROTO_CONTEXT;
typedef K2NET_OPEN_INSTANCE_OPAQUE *    K2NET_OPEN_INSTANCE;

typedef K2NET_PROTO_CONTEXT (*K2NET_PROTO_pf_Factory)(UINT32 aProtocolId, K2NET_PROTO_INSTANCE *apRetInstance);

typedef K2NET_OPEN_CONTEXT  (*K2NET_PROTO_pf_OnOpen)(K2NET_PROTO_CONTEXT apProtoContext, K2NET_OPEN_INSTANCE aOpenInstance);
typedef void                (*K2NET_PROTO_pf_OnClose)(K2NET_OPEN_CONTEXT aOpenContext);
typedef void                (*K2NET_PROTO_pf_OnDestroy)(K2NET_PROTO_CONTEXT aProtoContext);
typedef BOOL                (*K2NET_PROTO_pf_OnRecvMsg)(K2NET_PROTO_CONTEXT apProtoContext, K2NET_MSG *apRetMsg);
typedef UINT8 *             (*K2NET_PROTO_pf_GetSendBuffer)(K2NET_PROTO_CONTEXT apProtoContext, UINT32 aRequestMTU);
typedef BOOL                (*K2NET_PROTO_pf_SendAndReleaseBuffer)(UINT8 *apBuffer, UINT32 aBufferBytes);
typedef void                (*K2NET_PROTO_pf_ReleaseBuffer)(UINT8 *apBuffer);

struct _K2NET_PROTO_INSTANCE_DESC
{
    UINT32  mProtocolId;
    UINT32  mInstanceId;
};

struct _K2NET_PROTO_INSTANCE
{
    UINT32                      mProtocolId;
    K2NET_PROTO_pf_OnOpen       OnOpen;
    K2NET_PROTO_pf_OnClose      OnClose;
    K2NET_PROTO_pf_OnDestroy    OnDestroy;
    K2NET_PROTO_pf_OnRecvMsg    OnRecvMsg;
    K2LIST_LINK                 ChildListLink;
    K2LIST_ANCHOR               ChildList;
};

struct _K2NET_PROTO_HOST
{
    K2NET_PROTO_pf_GetSendBuffer            GetSendBuffer;
    K2NET_PROTO_pf_SendAndReleaseBuffer     SendAndReleaseBuffer;
    K2NET_PROTO_pf_ReleaseBuffer            ReleaseBuffer;
};

struct _K2NET_STACK_PROTO_CHUNK_HDR
{
    K2_CHUNK_HDR    Hdr;
    UINT32          mProtocolId;
};

struct _K2NET_STACK_DEF
{
    K2_NetAdapterType   mPhysType;
    K2_NET_ADAPTER_ADDR PhysAdapterAddr;
    K2_CHUNK_HDR        ConfigChunk;
    // rest of K2_CHUNK of the config follows
};


BOOL    K2NET_Proto_Register(UINT32 aProtocolId, K2NET_PROTO_pf_Factory afFactory);

BOOL    K2NET_Proto_GetParent(K2NET_PROTO_CONTEXT aProtoContext, K2NET_PROTO_HOST const **apRetHost, K2NET_PROTO_INSTANCE_DESC *apRetDesc);
UINT32  K2NET_Proto_GetPeerCount(K2NET_PROTO_CONTEXT aProtoContext);
BOOL    K2NET_Proto_GetPeerDesc(K2NET_PROTO_CONTEXT aProtoContext, UINT32 aIndex, K2NET_PROTO_INSTANCE_DESC *apRetDesc);
UINT32  K2NET_Proto_GetChildCount(K2NET_PROTO_CONTEXT aProtoContext);
BOOL    K2NET_Proto_GetChildDesc(K2NET_PROTO_CONTEXT aProtoContext, UINT32 aIndex, K2NET_PROTO_INSTANCE_DESC *apRetDesc);

BOOL    K2NET_QueueMessage(K2NET_OPEN_INSTANCE aOpenInstance, K2NET_MSG const *apMsg);

UINT32  K2NET_Stack_Enum(UINT32 aContainProtocolId, UINT32 *apBufferCount, K2NET_STACK *apRetStackBuffer);
void    K2NET_Stack_Release(K2NET_STACK aStack);
UINT32  K2NET_Stack_GetBaseInstanceId(K2NET_STACK, K2_NET_ADAPTER_DESC *apRetDesc);
UINT32  K2NET_Stack_EnumProto(K2NET_STACK aStack, UINT32 aMatchingId, UINT32 *apBufferCount, K2NET_PROTO_INSTANCE_DESC *apRetProtoInstBuffer);

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif // __K2NETHOST_H
