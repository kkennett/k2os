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
#ifndef __K2NET_H
#define __K2NET_H

#include <k2systype.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

#define K2NET_MSGTYPE 0x1AA1

typedef enum _K2NET_MsgShortType K2NET_MsgShortType;
enum _K2NET_MsgShortType
{
    K2NET_MsgShort_Invalid = 0,
    K2NET_MsgShort_Created,
    K2NET_MsgShort_Recv,          
    K2NET_MsgShort_Timeout,
    K2NET_MsgShort_Closed,
    K2NET_MsgShort_Destroyed,
    K2NET_MsgShort_Up,
    K2NET_MsgShort_Down,
    K2NET_MsgShort_Connected,
    K2NET_MsgShort_Disconnected,
    K2NET_MsgShort_Heard,

    K2NET_MsgShortType_Count
};

typedef struct _K2NET_MSG                   K2NET_MSG;

typedef struct _K2NET_USAGE_OPAQUE          K2NET_USAGE_OPAQUE;
typedef K2NET_USAGE_OPAQUE *                K2NET_USAGE;

typedef struct _K2NET_USAGE_CONTEXT_OPAQUE  K2NET_USAGE_CONTEXT_OPAQUE;
typedef K2NET_USAGE_CONTEXT_OPAQUE *        K2NET_USAGE_CONTEXT;

typedef void    (*K2NET_pf_Doorbell)(K2NET_USAGE_CONTEXT aUsageContext);

typedef void    (*K2NET_pf_OnTimerExpired)(void *apContext);

typedef void *  (*K2NET_pf_MemAlloc)(UINT32 aByteCount);
typedef void    (*K2NET_pf_MemFree)(void *aPtr);
typedef void    (*K2NET_pf_AddTimer)(void *apContext, UINT32 aPeriod, K2NET_pf_OnTimerExpired afTimerExpired);
typedef void    (*K2NET_pf_DelTimer)(void *apContext);

K2_PACKED_PUSH
struct _K2NET_MSG
{
    UINT16              mType;
    UINT16              mShort;
    K2NET_USAGE_CONTEXT mUsageContext;
    UINT32              mPayload[2];
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

void        K2NET_Init(K2NET_pf_MemAlloc afMemAlloc, K2NET_pf_MemFree afMemFree, K2NET_pf_AddTimer afAddTimer, K2NET_pf_DelTimer afDelTimer);

K2NET_USAGE K2NET_AcquireProtocolInstance(UINT32 aInstanceId, K2NET_USAGE_CONTEXT aUsageContext, K2NET_pf_Doorbell afDoorbell);

K2NET_USAGE_CONTEXT K2NET_Usage_GetContext(K2NET_USAGE aUsage);
void                K2NET_Usage_AddRef(K2NET_USAGE aUsage);
void                K2NET_Usage_Release(K2NET_USAGE aUsage);
BOOL                K2NET_Usage_RecvMsg(K2NET_USAGE aUsage, K2NET_MSG *apRetMsg);
UINT8 *             K2NET_Usage_GetSendBuffer(K2NET_USAGE aUsage, UINT32 aRequestMTU);
BOOL                K2NET_Usage_SendAndReleaseBuffer(UINT8 *apBuffer, UINT32 aBufferBytes);
void                K2NET_Usage_ReleaseBuffer(UINT8 *apBuffer);

void *      K2NET_MemAlloc(UINT32 aByteCount);
void        K2NET_MemFree(void *aPtr);
void        K2NET_AddTimer(void *apContext, UINT32 aPeriod, K2NET_pf_OnTimerExpired afTimerExpired);
void        K2NET_DelTimer(void *apContext);

void        K2NET_Done(void);

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif // __K2NET_H
