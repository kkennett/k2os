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

#include <lib/k2pppauto.h>
#include <lib/k2mem.h>

#if 0

//         | State
//         |    0         1         2         3         4         5        6         7         8           9
//   Events| Initial   Starting  Closed    Stopped   Closing   Stopping Req-Sent  Ack-Rcvd  Ack-Sent    Opened
//   ------+----------------------------------------------------------------------------------------------------
//    Up   |    2     irc,scr/6     -         -         -         -        -         -         -           -
//    Down |    -         -         0       tls/1       0         1        1         1         1         tld/1
//    Open |  tls/1       1     irc,scr/6     3r        5r        5r       6         7         8           9r
//    Close|    0         0         2         2         4         4    irc,str/4 irc,str/4 irc,str/4 tld,irc,str/4
//         |
//     TO+ |    -         -         -         -       str/4     str/5    scr/6     scr/6     scr/8         -
//     TO- |    -         -         -         -       tlf/2     tlf/3    tlf/3p    tlf/3p    tlf/3p        -
//         |
//    RCR+ |    -         -       sta/2 irc,scr,sca/8   4         5      sca/8   sca,tlu/9   sca/8   tld,scr,sca/8
//    RCR- |    -         -       sta/2 irc,scr,scn/6   4         5      scn/6     scn/7     scn/6   tld,scr,scn/6
//    RCA  |    -         -       sta/2     sta/3       4         5      irc/7     scr/6x  irc,tlu/9   tld,scr/6x
//    RCN  |    -         -       sta/2     sta/3       4         5    irc,scr/6   scr/6x  irc,scr/8   tld,scr/6x
//         |
//    RTR  |    -         -       sta/2     sta/3     sta/4     sta/5    sta/6     sta/6     sta/6   tld,zrc,sta/5
//    RTA  |    -         -         2         3       tlf/2     tlf/3      6         6         8       tld,scr/6
//         |
//    RUC  |    -         -       scj/2     scj/3     scj/4     scj/5    scj/6     scj/7     scj/8       scj/9
//    RXJ+ |    -         -         2         3         4         5        6         6         8           9
//    RXJ- |    -         -       tlf/2     tlf/3     tlf/2     tlf/3    tlf/3     tlf/3     tlf/3   tld,irc,str/5
//         |
//    RXR  |    -         -         2         3         4         5        6         7         8         ser/9

#endif

typedef enum _K2PPPAUTO_StateType K2PPPAUTO_StateType;
enum _K2PPPAUTO_StateType
{
    K2PPPAUTO_State_Initial0,
    K2PPPAUTO_State_Starting1,
    K2PPPAUTO_State_Closed2,
    K2PPPAUTO_State_Stopped3,
    K2PPPAUTO_State_Closing4,
    K2PPPAUTO_State_Stopping5,
    K2PPPAUTO_State_ReqSent6,
    K2PPPAUTO_State_AckRcvd7,
    K2PPPAUTO_State_AckSent8,
    K2PPPAUTO_State_Opened9,

    K2PPPAUTO_StateType_Count
};

typedef void (*K2PPPAUTO_pf_StateFunc)(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);

void K2PPPAUTO_StateFunc_Initial0(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_Starting1(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_Closed2(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_Stopped3(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_Closing4(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_Stopping5(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_ReqSent6(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_AckRcvd7(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_AckSent8(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);
void K2PPPAUTO_StateFunc_Opened9(K2PPPAUTO *apAuto, K2PPPAUTO_InputEventType aInputEvent);

static const K2PPPAUTO_pf_StateFunc sgStateFunc[K2PPPAUTO_StateType_Count] =
{
    K2PPPAUTO_StateFunc_Initial0,
    K2PPPAUTO_StateFunc_Starting1,
    K2PPPAUTO_StateFunc_Closed2,
    K2PPPAUTO_StateFunc_Stopped3,
    K2PPPAUTO_StateFunc_Closing4,
    K2PPPAUTO_StateFunc_Stopping5,
    K2PPPAUTO_StateFunc_ReqSent6,
    K2PPPAUTO_StateFunc_AckRcvd7,
    K2PPPAUTO_StateFunc_AckSent8,
    K2PPPAUTO_StateFunc_Opened9
};

void
K2PPPAUTO_InitRestartCount(
    K2PPPAUTO * apAuto,
    UINT32      aInitVal
)
{
    apAuto->mRestartCount = aInitVal;
    apAuto->mTimerIsOn = TRUE;
    apAuto->Config.mfStartTimer(apAuto, apAuto->Config.mTimer_Period);
}

void
K2PPPAUTO_TurnTimerOff(
    K2PPPAUTO * apAuto
)
{
    K2_ASSERT(apAuto->mTimerIsOn);
    apAuto->mTimerIsOn = FALSE;
    apAuto->Config.mfStopTimer(apAuto);
}

//
// order of operations:
// 
// timer off
// state change
// callback
// init restart count
// send(s)
//

void
K2PPPAUTO_StateFunc_Initial0(
    K2PPPAUTO *                 apAuto,
    K2PPPAUTO_InputEventType    aInputEvent
)
{
    K2_ASSERT(!apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsUp:
        // 2
        K2_ASSERT(!apAuto->mResetStarted);
        apAuto->mInReset = FALSE;
        apAuto->mState = K2PPPAUTO_State_Closed2;
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
        // tls/1
        apAuto->mState = K2PPPAUTO_State_Starting1;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnStart);
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
        // 0
        // quietly stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_Starting1(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(!apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsUp:
        K2_ASSERT(!apAuto->mResetStarted);
        apAuto->mInReset = FALSE;
        if (!apAuto->Config.mNoNegontiateStage)
        {
            // irc,scr/6
            apAuto->mState = K2PPPAUTO_State_ReqSent6;
            K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxConfigureCount);
            apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        }
        else
        {
            // tlu/9
            apAuto->mState = K2PPPAUTO_State_Opened9;
            apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnUp);
        }
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
        // 1
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
        // 0
        apAuto->mState = K2PPPAUTO_State_Initial0;
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_Closed2(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(!apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsDown:
        // 0
        apAuto->mState = K2PPPAUTO_State_Initial0;
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
        if (!apAuto->Config.mNoNegontiateStage)
        {
            // irc,scr/6
            apAuto->mState = K2PPPAUTO_State_ReqSent6;
            K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxConfigureCount);
            apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        }
        else
        {
            apAuto->mState = K2PPPAUTO_State_Opened9;
            apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnUp);
        }
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
    case K2PPPAUTO_InputEvent_RTA:
    case K2PPPAUTO_InputEvent_RXJ_GOOD:
    case K2PPPAUTO_InputEvent_RXR:
        // 2
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_RCR_GOOD:
    case K2PPPAUTO_InputEvent_RCR_BAD:
    case K2PPPAUTO_InputEvent_RCA:
    case K2PPPAUTO_InputEvent_RCN:
    case K2PPPAUTO_InputEvent_RTR:
        // sta/2
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateAck);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RUC:
        // scj/2
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendCodeReject);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RXJ_BAD:
        // tlf/2
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_Rejected;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        // stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_Stopped3(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(!apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsDown:
        // tls/1
        if (!apAuto->mInReset)
        {
            apAuto->mState = K2PPPAUTO_State_Starting1;
            apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnStart);
            if (apAuto->mResetStarted)
            {
                apAuto->mResetStarted = FALSE;
                K2PPPAUTO_InputEvent(apAuto, K2PPPAUTO_InputEvent_LowerLayerIsUp);
            }
        }
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
        // 3r
        // stay here
        if (apAuto->Config.mEnableResetOption)
        {
            apAuto->mResetStarted = TRUE;
            K2PPPAUTO_InputEvent(apAuto, K2PPPAUTO_InputEvent_LowerLayerIsDown);
        }
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
        // 2
        apAuto->mState = K2PPPAUTO_State_Closed2;
        break;

    case K2PPPAUTO_InputEvent_RCR_GOOD:
        // irc,scr,sca/8
        apAuto->mState = K2PPPAUTO_State_AckSent8;
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxConfigureCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureAck);
        break;

    case K2PPPAUTO_InputEvent_RCR_BAD:
        // irc,scr,scn/6
        apAuto->mState = K2PPPAUTO_State_ReqSent6;
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxConfigureCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureNack);
        break;

    case K2PPPAUTO_InputEvent_RCA:
    case K2PPPAUTO_InputEvent_RCN:
    case K2PPPAUTO_InputEvent_RTR:
        // sta/3
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateAck);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RTA:
    case K2PPPAUTO_InputEvent_RXJ_GOOD:
    case K2PPPAUTO_InputEvent_RXR:
        // 3
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_RUC:
        // scj/3
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendCodeReject);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RXJ_BAD:
        // tlf/3
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_Rejected;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        // stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_Closing4(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsDown:
        // 0
        if (!apAuto->mInReset)
        {
            K2PPPAUTO_TurnTimerOff(apAuto);
            apAuto->mState = K2PPPAUTO_State_Initial0;
            if (apAuto->mResetStarted)
            {
                apAuto->mResetStarted = FALSE;
                K2PPPAUTO_InputEvent(apAuto, K2PPPAUTO_InputEvent_LowerLayerIsUp);
            }
        }
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
        // 5r
        apAuto->mState = K2PPPAUTO_State_Stopping5;
        if (apAuto->Config.mEnableResetOption)
        {
            apAuto->mResetStarted = TRUE;
            K2PPPAUTO_InputEvent(apAuto, K2PPPAUTO_InputEvent_LowerLayerIsDown);
        }
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
    case K2PPPAUTO_InputEvent_RCR_GOOD:
    case K2PPPAUTO_InputEvent_RCR_BAD:
    case K2PPPAUTO_InputEvent_RCA:
    case K2PPPAUTO_InputEvent_RCN:
    case K2PPPAUTO_InputEvent_RXJ_GOOD:
    case K2PPPAUTO_InputEvent_RXR:
        // 4
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_TO_NONZERO:
        // str/4
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateRequest);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_TO_ZERO:
    case K2PPPAUTO_InputEvent_RTA:
    case K2PPPAUTO_InputEvent_RXJ_BAD:
        // tlf/2
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Closed2;
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_TimedOut;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        break;

    case K2PPPAUTO_InputEvent_RTR:
        // sta/4
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateAck);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RUC:
        // scj/4
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendCodeReject);
        // stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_Stopping5(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsDown:
        // 1
        if (!apAuto->mInReset)
        {
            K2PPPAUTO_TurnTimerOff(apAuto);
            apAuto->mState = K2PPPAUTO_State_Starting1;
            if (apAuto->mResetStarted)
            {
                apAuto->mResetStarted = FALSE;
                K2PPPAUTO_InputEvent(apAuto, K2PPPAUTO_InputEvent_LowerLayerIsUp);
            }
        }
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
        // 5r
        // stay here
        if (apAuto->Config.mEnableResetOption)
        {
            apAuto->mResetStarted = TRUE;
            K2PPPAUTO_InputEvent(apAuto, K2PPPAUTO_InputEvent_LowerLayerIsDown);
        }
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
        // 4
        apAuto->mState = K2PPPAUTO_State_Closing4;
        break;

    case K2PPPAUTO_InputEvent_TO_NONZERO:
        // str/5
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateRequest);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_TO_ZERO:
    case K2PPPAUTO_InputEvent_RTA:
    case K2PPPAUTO_InputEvent_RXJ_BAD:
        // tlf/3
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Stopped3;
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_TimedOut;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        break;

    case K2PPPAUTO_InputEvent_RCR_GOOD:
    case K2PPPAUTO_InputEvent_RCR_BAD:
    case K2PPPAUTO_InputEvent_RCA:
    case K2PPPAUTO_InputEvent_RCN:
    case K2PPPAUTO_InputEvent_RXJ_GOOD:
    case K2PPPAUTO_InputEvent_RXR:
        // 5
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_RTR:
        // sta/5
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateAck);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RUC:
        // scj/5
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendCodeReject);
        // stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_ReqSent6(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsDown:
        // 1
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Starting1;
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
    case K2PPPAUTO_InputEvent_RTA:
    case K2PPPAUTO_InputEvent_RXJ_GOOD:
    case K2PPPAUTO_InputEvent_RXR:
        // 6
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
        // irc,str/4
        apAuto->mState = K2PPPAUTO_State_Closing4;
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxTerminateCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateRequest);
        break;

    case K2PPPAUTO_InputEvent_TO_NONZERO:
        // scr/6
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_TO_ZERO:
        // tlf/3p
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Stopped3;
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_TimedOut;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        break;

    case K2PPPAUTO_InputEvent_RCR_GOOD:
        // sca/8
        apAuto->mState = K2PPPAUTO_State_AckSent8;
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureAck);
        break;

    case K2PPPAUTO_InputEvent_RCR_BAD:
        // scn/6
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureNack);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RCA:
        // irc/7
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxConfigureCount);
        apAuto->mState = K2PPPAUTO_State_AckRcvd7;
        break;

    case K2PPPAUTO_InputEvent_RCN:
        // irc,scr/6
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxConfigureCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RTR:
        // sta/6
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateAck);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RUC:
        // scj/6
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendCodeReject);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RXJ_BAD:
        // tlf/3
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Stopped3;
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_Rejected;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_AckRcvd7(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsDown:
        // 1
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Starting1;
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
    case K2PPPAUTO_InputEvent_RXR:
        // 7
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
        // irc,str/4
        apAuto->mState = K2PPPAUTO_State_Closing4;
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxTerminateCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateRequest);
        break;

    case K2PPPAUTO_InputEvent_TO_NONZERO:
        // scr/6
        apAuto->mState = K2PPPAUTO_State_ReqSent6;
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        break;

    case K2PPPAUTO_InputEvent_TO_ZERO:
        // tlf/3p
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Stopped3;
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_TimedOut;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        break;

    case K2PPPAUTO_InputEvent_RCR_GOOD:
        // tlu,sca/9
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Opened9;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnUp);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureAck);
        break;

    case K2PPPAUTO_InputEvent_RCR_BAD:
        // scn/7
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureNack);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RCA:
    case K2PPPAUTO_InputEvent_RCN:
        // scr/6x
        // ignore stay here
        break;

    case K2PPPAUTO_InputEvent_RTR:
        // sta/6
        apAuto->mState = K2PPPAUTO_State_ReqSent6;
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateAck);
        break;

    case K2PPPAUTO_InputEvent_RTA:
    case K2PPPAUTO_InputEvent_RXJ_GOOD:
        // 6
        apAuto->mState = K2PPPAUTO_State_ReqSent6;
        break;

    case K2PPPAUTO_InputEvent_RUC:
        // scj/7
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendCodeReject);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RXJ_BAD:
        // tlf/3
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Stopped3;
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_Rejected;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_AckSent8(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsDown:
        // 1
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Starting1;
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
    case K2PPPAUTO_InputEvent_RTA:
    case K2PPPAUTO_InputEvent_RXJ_GOOD:
    case K2PPPAUTO_InputEvent_RXR:
        // 8
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
        // irc,str/4
        apAuto->mState = K2PPPAUTO_State_Closing4;
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxTerminateCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateRequest);
        break;

    case K2PPPAUTO_InputEvent_TO_NONZERO:
        // scr/8
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_TO_ZERO:
        // tlf/3p
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Stopped3;
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_TimedOut;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        break;

    case K2PPPAUTO_InputEvent_RCR_GOOD:
        // sca/8
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureAck);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RCR_BAD:
        // scn/6
        apAuto->mState = K2PPPAUTO_State_ReqSent6;
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureNack);
        break;

    case K2PPPAUTO_InputEvent_RCA:
        // tlu/9   (irc,tlu/9  - why irc?)
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Opened9;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnUp);
        break;

    case K2PPPAUTO_InputEvent_RCN:
        // irc,scr/8
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxConfigureCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RTR:
        // sta/6
        apAuto->mState = K2PPPAUTO_State_ReqSent6;
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateAck);
        break;

    case K2PPPAUTO_InputEvent_RUC:
        // scj/8
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendCodeReject);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RXJ_BAD:
        // tlf/3
        K2PPPAUTO_TurnTimerOff(apAuto);
        apAuto->mState = K2PPPAUTO_State_Stopped3;
        apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_Rejected;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPPAUTO_StateFunc_Opened9(
    K2PPPAUTO *         apAuto,
    K2PPPAUTO_InputEventType  aInputEvent
)
{
    K2_ASSERT(!apAuto->mTimerIsOn);

    switch (aInputEvent)
    {
    case K2PPPAUTO_InputEvent_LowerLayerIsDown:
        // tld/1
        if (!apAuto->mInReset)
        {
            apAuto->mState = K2PPPAUTO_State_Starting1;
            apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnDown);
            if (apAuto->mResetStarted)
            {
                apAuto->mResetStarted = FALSE;
                K2PPPAUTO_InputEvent(apAuto, K2PPPAUTO_InputEvent_LowerLayerIsUp);
            }
        }
        break;

    case K2PPPAUTO_InputEvent_AdminOpen:
        // 9r
        if (apAuto->Config.mEnableResetOption)
        {
            apAuto->mResetStarted = TRUE;
            K2PPPAUTO_InputEvent(apAuto, K2PPPAUTO_InputEvent_LowerLayerIsDown);
        }
        break;

    case K2PPPAUTO_InputEvent_AdminClose:
        // tld,irc,str/4
        apAuto->mState = K2PPPAUTO_State_Closing4;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnDown);
        if (!apAuto->Config.mNoNegontiateStage)
        {
            K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxTerminateCount);
            apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateRequest);
        }
        else
        {
            apAuto->mState = K2PPPAUTO_State_Closed2;
            apAuto->mLastFinishReason = K2PPPAUTO_FinishReason_Closed;
            apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnFinish);
        }
        break;

    case K2PPPAUTO_InputEvent_RCR_GOOD:
        if (!apAuto->Config.mNoNegontiateStage)
        {
            // tld,scr,sca/8
            apAuto->mState = K2PPPAUTO_State_AckSent8;
            apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnDown);
            apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
            apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureAck);
        }
        break;

    case K2PPPAUTO_InputEvent_RCR_BAD:
        if (!apAuto->Config.mNoNegontiateStage)
        {
            // tld,scr,scn/6
            apAuto->mState = K2PPPAUTO_State_ReqSent6;
            apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnDown);
            apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
            apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureNack);
        }
        break;

    case K2PPPAUTO_InputEvent_RCA:
        // tld,scr,irc/6x
        // ignore stay here
        break;

    case K2PPPAUTO_InputEvent_RCN:
        // tld,scr,irc/6x
        // ignore stay here
        break;

    case K2PPPAUTO_InputEvent_RTR:
        // tld,zrc,sta/5
        apAuto->mState = K2PPPAUTO_State_Stopping5;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnDown);
        K2PPPAUTO_InitRestartCount(apAuto, 0);    // zrc
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateAck);
        break;

    case K2PPPAUTO_InputEvent_RTA:
        // tld,scr,irc/6
        apAuto->mState = K2PPPAUTO_State_ReqSent6;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnDown);
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxTerminateCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendConfigureRequest);
        break;

    case K2PPPAUTO_InputEvent_RUC:
        // scj/9
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendCodeReject);
        // stay here
        break;

    case K2PPPAUTO_InputEvent_RXJ_GOOD:
        // 9
        // quietly stay here
        break;

    case K2PPPAUTO_InputEvent_RXJ_BAD:
        // tld,irc,str/5
        apAuto->mState = K2PPPAUTO_State_Stopping5;
        apAuto->Config.mfEvent(apAuto, K2PPPAUTO_OutputEvent_OnDown);
        K2PPPAUTO_InitRestartCount(apAuto, apAuto->Config.mMaxTerminateCount);
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendTerminateRequest);
        break;

    case K2PPPAUTO_InputEvent_RXR:
        // ser/9
        apAuto->Config.mfSender(apAuto, K2PPPAUTO_SendEchoReply);
        // stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

K2STAT
K2PPPAUTO_Init(
    K2PPPAUTO *                 apAuto,
    K2PPPAUTO_CONFIG const *    apConfig
)
{
    if ((NULL == apAuto) ||
        (NULL == apConfig) ||
        (NULL == apConfig->mfStartTimer) ||
        (NULL == apConfig->mfStopTimer) ||
        (NULL == apConfig->mfEvent) ||
        (NULL == apConfig->mfSender))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2MEM_Zero(apAuto, sizeof(K2PPPAUTO));

    K2MEM_Copy(&apAuto->Config, apConfig, sizeof(K2PPPAUTO_CONFIG));

    if (0 == apConfig->mMaxConfigureCount)
        apAuto->Config.mMaxConfigureCount = K2_PPP_AUTO_MAX_CONFIGURE_COUNT;
    
    if (0 == apConfig->mMaxTerminateCount)
        apAuto->Config.mMaxTerminateCount = K2_PPP_AUTO_MAX_TERMINATE_COUNT;

    if (0 == apConfig->mTimer_Period)
        apAuto->Config.mTimer_Period = K2_PPP_AUTO_TIMER_DEFAULT_MS;

    return K2STAT_NO_ERROR;
}

void
K2PPPAUTO_InputEvent(
    K2PPPAUTO *                 apAuto,
    K2PPPAUTO_InputEventType    aInputEventType
)
{
    K2_ASSERT(apAuto->mState < K2PPPAUTO_StateType_Count);
    sgStateFunc[apAuto->mState](apAuto, aInputEventType);
}
