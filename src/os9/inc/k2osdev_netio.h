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
#ifndef __K2OSDEV_NETIO_H
#define __K2OSDEV_NETIO_H

#include <k2osdev.h>
#include <spec/ether.h>
#include <spec/ipv4.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

// {85B09727-B29A-4F27-95AD-075C47A9E1F0}
#define K2OS_IFACE_NETIO_DEVICE   { 0x85b09727, 0xb29a, 0x4f27, { 0x95, 0xad, 0x7, 0x5c, 0x47, 0xa9, 0xe1, 0xf0 } }

#define K2OS_NETIO_NOTIFY_PHYS_CONNECTED_CHANGED   1

//
//------------------------------------------------------------------------
//

#define K2OS_NETIO_MSGTYPE  0x1AA1

typedef enum _K2OS_NetIoMsgShortType K2OS_NetIoMsgShortType;
enum _K2OS_NetIoMsgShortType
{
    K2OS_NetIoMsgShort_Invalid = 0,
    K2OS_NetIoMsgShort_Up,  
    K2OS_NetIoMsgShort_Down,
    K2OS_NetIoMsgShort_Recv,

    K2OS_NetIoMsgShortType_Count
};

typedef struct _K2OS_NETIO_ADAPTER_STATE    K2OS_NETIO_ADAPTER_STATE;
typedef struct _K2OS_NETIO_MSG              K2OS_NETIO_MSG;
typedef struct _K2OS_NETIO_BUFCOUNTS        K2OS_NETIO_BUFCOUNTS;

typedef struct _K2OS_NETIO_OPAQUE K2OS_NETIO_OPAQUE;
typedef K2OS_NETIO_OPAQUE * K2OS_NETIO;

K2_PACKED_PUSH
struct _K2OS_NETIO_MSG
{
    UINT16      mType;          // K2OS_NETIO_MSGTYPE
    UINT16      mShort;         // K2OS_NetIoMsgShortType
    void *      mpContext;      // value passed to Attach()
    UINT32      mPayload[2];    // for Recv this is virt addr of received buffer and bytes in that buffer
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

struct _K2OS_NETIO_ADAPTER_STATE
{
    BOOL    mIsPhysConnected;
    BOOL    mIsUp;
};

struct _K2OS_NETIO_BUFCOUNTS
{
    UINT32  mXmit;
    UINT32  mRecv;
};

K2OS_NETIO  K2OS_NetIo_Attach(K2OS_IFINST_ID aIfInstId, void *apContext, K2OS_MAILBOX_TOKEN aTokMailbox);
BOOL        K2OS_NetIo_Detach(K2OS_NETIO aNetIo);

BOOL        K2OS_NetIo_GetDesc(K2OS_NETIO aNetIo, K2_NET_ADAPTER_DESC *apRetDesc);
BOOL        K2OS_NetIo_GetState(K2OS_NETIO aNetIo, K2OS_NETIO_ADAPTER_STATE *apRetState);

BOOL        K2OS_NetIo_GetBufferStats(K2OS_NETIO aNetIo, K2OS_NETIO_BUFCOUNTS *apRetTotal, K2OS_NETIO_BUFCOUNTS *apRetAvail);

UINT8 *     K2OS_NetIo_AcqSendBuffer(K2OS_NETIO aNetIo, UINT32 *apRetMTU);
BOOL        K2OS_NetIo_Send(K2OS_NETIO aNetIo, UINT8 *apBuffer, UINT32 aSendBytes);
BOOL        K2OS_NetIo_RelBuffer(K2OS_NETIO aNetIo, UINT32 aBufVirtAddr);

BOOL        K2OS_NetIo_SetEnable(K2OS_NETIO aNetIo, BOOL aSetEnable);
BOOL        K2OS_NetIo_GetEnable(K2OS_NETIO aNetIo, BOOL *apRetEnable);

//
//------------------------------------------------------------------------
//

#define K2OS_NETIO_METHOD_CONFIG        1
// input
typedef struct _K2OS_NETIO_CONFIG_IN K2OS_NETIO_CONFIG_IN;
struct _K2OS_NETIO_CONFIG_IN
{
    void *              mpContext;
    K2OS_MAILBOX_TOKEN  mTokMailbox;
};
// output nothing

#define K2OS_NETIO_METHOD_GET_DESC      2
// input nothing
// output K2_NET_ADAPTER_DESC

#define K2OS_NETIO_METHOD_GET_STATE     3
// input nothing
// output K2OS_NETIO_ADAPTER_STATE 

#define K2OS_NETIO_METHOD_BUFSTATS      4
// input nothing
// output 2 x K2OS_NETIO_BUFCOUNTS, first is total second is available (not owned by user)

#define K2OS_NETIO_METHOD_ACQBUFFER     5
// input nothing
// output
typedef struct _K2OS_NETIO_ACQBUFFER_OUT K2OS_NETIO_ACQBUFFER_OUT;
struct _K2OS_NETIO_ACQBUFFER_OUT
{
    UINT32  mBufVirtAddr;
    UINT32  mMTU;
};

#define K2OS_NETIO_METHOD_SEND          6
// input
typedef struct _K2OS_NETIO_SEND_IN K2OS_NETIO_SEND_IN;
struct _K2OS_NETIO_SEND_IN
{
    UINT32  mBufVirtAddr;
    UINT32  mSendBytes;
};
// output nothing

#define K2OS_NETIO_METHOD_RELBUFFER     7
// input one UINT32 virtual buffer address
// output nothing

#define K2OS_NETIO_METHOD_SETENABLE     8
// input one BOOL enable state to set
// output nothing

#define K2OS_NETIO_METHOD_GETENABLE     9
// input nothing
// output one BOOL current enable state 

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSDEV_NETIO_H