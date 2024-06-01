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

#ifndef __K2PPPAUTO_H
#define __K2PPPAUTO_H

/* --------------------------------------------------------------------------------- */

#include <k2basetype.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _K2PPPAUTO_InputEventType K2PPPAUTO_InputEventType;
enum _K2PPPAUTO_InputEventType
{
    K2PPPAUTO_InputEvent_Invalid = 0,

    K2PPPAUTO_InputEvent_LowerLayerIsUp,
    K2PPPAUTO_InputEvent_LowerLayerIsDown,
    K2PPPAUTO_InputEvent_AdminOpen,
    K2PPPAUTO_InputEvent_AdminClose,
    K2PPPAUTO_InputEvent_TO_NONZERO,       // Timeout when counter > 0
    K2PPPAUTO_InputEvent_TO_ZERO,          // Timeout with counter expired
    K2PPPAUTO_InputEvent_RCR_GOOD,         // Receive-Configure-Request (good)
    K2PPPAUTO_InputEvent_RCR_BAD,          // Receive-Configure-Request (bad)
    K2PPPAUTO_InputEvent_RCA,              // Receive-Configure-Ack
    K2PPPAUTO_InputEvent_RCN,              // Receive-Configure-Nak/rej
    K2PPPAUTO_InputEvent_RTR,              // Receive Terminate Request
    K2PPPAUTO_InputEvent_RTA,              // Receive Terminate Ack
    K2PPPAUTO_InputEvent_RUC,              // Receive-Unknown-Code
    K2PPPAUTO_InputEvent_RXJ_GOOD,         // Receive code reject (permitted) or Receive protocol reject
    K2PPPAUTO_InputEvent_RXJ_BAD,          // Receive code reject (catastrophic) or receive protocol reject
    K2PPPAUTO_InputEvent_RXR,              // Receive echo request or receive echo reply or receive discard request

    K2PPPAUTO_InputEventType_Count
};

typedef enum _K2PPPAUTO_OutputEventType K2PPPAUTO_OutputEventType;
enum _K2PPPAUTO_OutputEventType
{
    K2PPPAUTO_OutputEvent_Invalid = 0,

    K2PPPAUTO_OutputEvent_OnUp,
    K2PPPAUTO_OutputEvent_OnDown,
    K2PPPAUTO_OutputEvent_OnStart,
    K2PPPAUTO_OutputEvent_OnFinish,

    K2PPPAUTO_OutputEventType_Count
};

typedef enum _K2PPPAUTO_SendType K2PPPAUTO_SendType;
enum _K2PPPAUTO_SendType
{
    K2PPPAUTO_SendInvalid = 0,
    K2PPPAUTO_SendConfigureRequest = K2_PPP_AUTO_CONFIGURE_REQUEST,
    K2PPPAUTO_SendConfigureAck = K2_PPP_AUTO_CONFIGURE_ACK,
    K2PPPAUTO_SendConfigureNack = K2_PPP_AUTO_CONFIGURE_NACK,
    K2PPPAUTO_SendConfigureReject = K2_PPP_AUTO_CONFIGURE_REJECT,
    K2PPPAUTO_SendTerminateRequest = K2_PPP_AUTO_TERMINATE_REQUEST,
    K2PPPAUTO_SendTerminateAck = K2_PPP_AUTO_TERMINATE_ACK,
    K2PPPAUTO_SendCodeReject = K2_PPP_AUTO_CODE_REJECT,
    K2PPPAUTO_SendProtocolReject = K2_PPP_AUTO_PROTOCOL_REJECT,
    K2PPPAUTO_SendEchoRequest = K2_PPP_AUTO_ECHO_REQUEST,
    K2PPPAUTO_SendEchoReply = K2_PPP_AUTO_ECHO_REPLY,
    K2PPPAUTO_SendDiscardRequest = K2_PPP_AUTO_DISCARD_REQUEST,

    K2PPPAUTO_SendType_Count
};

typedef struct _K2PPPAUTO K2PPPAUTO;

typedef void (*K2PPPAUTO_pf_TimerCallback)(K2PPPAUTO *apAuto);

typedef void (*K2PPPAUTO_pf_Send)(K2PPPAUTO *apAuto, K2PPPAUTO_SendType aSendType);
typedef void (*K2PPPAUTO_pf_OutputEvent)(K2PPPAUTO *apAuto, K2PPPAUTO_OutputEventType aEventType);
typedef void (*K2PPPAUTO_pf_StartTimer)(K2PPPAUTO *apAuto, UINT32 aPeriod);
typedef void (*K2PPPAUTO_pf_StopTimer)(K2PPPAUTO *apAuto);

typedef struct _K2PPPAUTO_CONFIG K2PPPAUTO_CONFIG;
struct _K2PPPAUTO_CONFIG
{
    K2PPPAUTO_pf_Send           mfSender;
    K2PPPAUTO_pf_OutputEvent    mfEvent;
    K2PPPAUTO_pf_StartTimer     mfStartTimer;
    K2PPPAUTO_pf_StopTimer      mfStopTimer;
    BOOL                        mNoNegontiateStage;
    BOOL                        mEnableResetOption;
    UINT32                      mTimer_Period;
    UINT32                      mMaxConfigureCount;
    UINT32                      mMaxTerminateCount;
};

typedef enum _K2PPPAUTO_FinishReasonType K2PPPAUTO_FinishReasonType;
enum _K2PPPAUTO_FinishReasonType
{
    K2PPPAUTO_FinishReason_Invalid = 0,

    K2PPPAUTO_FinishReason_Terminated,
    K2PPPAUTO_FinishReason_TimedOut,
    K2PPPAUTO_FinishReason_Rejected,
    K2PPPAUTO_FinishReason_Closed,

    K2PPPAUTO_FinishReasonType_Count
};

struct _K2PPPAUTO
{
    UINT32                      mState;
    UINT32                      mRestartCount;
    BOOL                        mTimerIsOn;
    BOOL                        mResetStarted;
    BOOL                        mInReset;
    K2PPPAUTO_FinishReasonType  mLastFinishReason;
    K2PPPAUTO_CONFIG            Config;
};

K2STAT
K2PPPAUTO_Init(
    K2PPPAUTO *                 apAuto,
    K2PPPAUTO_CONFIG const *    apConfig
);

void
K2PPPAUTO_InputEvent(
    K2PPPAUTO *                 apAuto,
    K2PPPAUTO_InputEventType    aInputEventType
);

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif // __K2PPPF_H
