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

#include <lib/k2win32.h>
#include "ik2ppplink.h"

char const * gK2PPP_OidAutoEventNames[K2PPP_OidAutoEventType_Count] =
{
    "Invalid",
    "LowerLayerIsUp",
    "LowerLayerIsDown",
    "AdminOpen",
    "AdminClose",
    "TO_NONZERO",
    "TO_ZERO",
    "RCR_GOOD",
    "RCR_BAD",
    "RCA",
    "RCN",
    "RTR",
    "RTA",
    "RUC",
    "RXJ_GOOD",
    "RXJ_BAD",
    "RXR",
};

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

void K2PPP_OIDAUTO_State_Initial0(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_Starting1(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_Closed2(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_Stopped3(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_Closing4(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_Stopping5(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_ReqSent6(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_AckRcvd7(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_AckSent8(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);
void K2PPP_OIDAUTO_State_Opened9(K2PPP_OIDAUTO *apAuto, K2PPP_OidAutoEventType aOidAutoEvent);

void
K2PPPOidAuto_OnTimerExpired(
    void *apContext
)
{
    K2PPP_OIDAUTO *         pAuto;
    K2PPP_OidAutoEventType  evt;
    K2PPP_OID *             pPppOid;

    pAuto = (K2PPP_OIDAUTO *)apContext;
    pPppOid = K2_GET_CONTAINER(K2PPP_OID, pAuto, Automaton);
    printf("%s(%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName);

    if (pAuto->mRestartCount == 0)
    {
        evt = K2PPP_OidAutoEvent_TO_ZERO;
    }
    else
    {
        pAuto->mRestartCount--;
        evt = K2PPP_OidAutoEvent_TO_NONZERO;
    }

    ((K2PPP_OIDAUTO_pf_StateFunc)pAuto->mpState)(pAuto, evt);
}

void
K2PPPOidAuto_InitRestartCount(
    K2PPP_OIDAUTO * apAuto,
    UINT32          aInitVal
)
{
    K2PPP_OID * pOid;

    apAuto->mRestartCount = aInitVal;
    apAuto->mTimerIsOn = TRUE;

    pOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);
    printf("%s(%s, %d)\n", __FUNCTION__, pOid->HostLayer.Layer.mName, aInitVal);

    K2NET_AddTimer(apAuto, pOid->mTimer_Period, K2PPPOidAuto_OnTimerExpired);
}

void
K2PPPOidAuto_TurnTimerOff(
    K2PPP_OIDAUTO * apAuto
)
{
    K2PPP_OID * pPppOid;

    K2_ASSERT(apAuto->mTimerIsOn);

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    printf("%s(%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName);

    apAuto->mTimerIsOn = FALSE;

    K2NET_DelTimer(apAuto);
}

void
K2PPP_OIDAUTO_State_Initial0(
    K2PPP_OIDAUTO *         apAuto, 
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(!apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // 2
        K2_ASSERT(!apAuto->mResetStarted);
        apAuto->mInReset = FALSE;
        printf("%s : INITIAL -> CLOSED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closed2;
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // tls/1
        printf("%s : INITIAL -> STARTING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Starting1;
        printf("%s.OnStart\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->HostLayer.Layer.OnStart(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // 0
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCA:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCN:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RTR:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RTA:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RUC:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RXR:
        // -
        K2_ASSERT(0);
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_State_Starting1(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(!apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        K2_ASSERT(!apAuto->mResetStarted);
        apAuto->mInReset = FALSE;
        if (!apAuto->mNoConfig)
        {
            // irc,scr/6
            printf("%s : STARTING -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
            K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxConfigureCount);
            printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->mfSend_ConfigRequest(pPppOid);
        }
        else
        {
            // tlu/9
            printf("%s : STARTING -> OPENED\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_Opened9;
            printf("%s.OnUp\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->HostLayer.Layer.Api.OnUp(&pPppOid->HostLayer.Layer.Api);
        }
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // 1
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // 0
        printf("%s : STARTING -> INITIAL\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Initial0;
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCA:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCN:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RTR:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RTA:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RUC:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RXR:
        // -
        K2_ASSERT(0);
        break;

    default:
        K2_ASSERT(0);
        break;
    }
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
K2PPP_OIDAUTO_State_Closed2(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(!apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // 0
        printf("%s : CLOSED -> INITIAL\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Initial0;
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        if (!apAuto->mNoConfig)
        {
            // irc,scr/6
            printf("%s : CLOSED -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
            K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxConfigureCount);
            printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->mfSend_ConfigRequest(pPppOid);
        }
        else
        {
            printf("%s : CLOSED -> OPENED\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_Opened9;
            printf("%s.OnUp\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->HostLayer.Layer.Api.OnUp(&pPppOid->HostLayer.Layer.Api);
        }
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // 2
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // sta/2
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // sta/2
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RCA:
        // sta/2
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RCN:
        // sta/2
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTR:
        // sta/2
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTA:
        // 2
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RUC:
        // scj/2
        printf("%s.Send_CodeReject\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_CodeReject(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // 2
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // tlf/2
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Rejected;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXR:
        // 2
        // quietly stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_State_Stopped3(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(!apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // tls/1
        if (!apAuto->mInReset)
        {
            printf("%s : STOPPED -> STARTING\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_Starting1;
            printf("%s.OnStart\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->HostLayer.Layer.OnStart(&pPppOid->HostLayer.Layer);
            if (apAuto->mResetStarted)
            {
                apAuto->mResetStarted = FALSE;
                K2PPP_OIDAUTO_Run(apAuto, K2PPP_OidAutoEvent_LowerLayerIsUp);
            }
        }
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // 3r
        // stay here
        if (apAuto->mEnableResetOption)
        {
            apAuto->mResetStarted = TRUE;
            K2PPP_OIDAUTO_Run(apAuto, K2PPP_OidAutoEvent_LowerLayerIsDown);
        }
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // 2
        printf("%s : STOPPED -> CLOSED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closed2;
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // irc,scr,sca/8
        printf("%s : STOPPED -> ACKSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_AckSent8;
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxConfigureCount);
        printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigRequest(pPppOid);
        printf("%s.Send_ConfigAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigAck(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // irc,scr,scn/6
        printf("%s : STOPPED -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxConfigureCount);
        printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigRequest(pPppOid);
        printf("%s.Send_ConfigNack\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigNack(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RCA:
        // sta/3
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RCN:
        // sta/3
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTR:
        // sta/3
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTA:
        // 3
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RUC:
        // scj/3
        printf("%s.Send_CodeReject\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_CodeReject(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // 3
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // tlf/3
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Rejected;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXR:
        // 3
        // quietly stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_State_Closing4(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // 0
        if (!apAuto->mInReset)
        {
            K2PPPOidAuto_TurnTimerOff(apAuto);
            printf("%s : CLOSING -> INITIAL\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_Initial0;
            if (apAuto->mResetStarted)
            {
                apAuto->mResetStarted = FALSE;
                K2PPP_OIDAUTO_Run(apAuto, K2PPP_OidAutoEvent_LowerLayerIsUp);
            }
        }
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // 5r
        printf("%s : CLOSING -> STOPPING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopping5;
        if (apAuto->mEnableResetOption)
        {
            apAuto->mResetStarted = TRUE;
            K2PPP_OIDAUTO_Run(apAuto, K2PPP_OidAutoEvent_LowerLayerIsDown);
        }
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // 4
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // str/4
        printf("%s.Send_TermRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermRequest(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // tlf/2
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : CLOSING -> CLOSED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closed2;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_TimeOut;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // 4
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // 4
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RCA:
        // 4
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RCN:
        // 4
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RTR:
        // sta/4
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTA:
        // tlf/2
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : CLOSING -> CLOSED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closed2;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Terminate;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RUC:
        // scj/4
        printf("%s.Send_CodeReject\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_CodeReject(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // 4
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // tlf/2
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : CLOSING -> CLOSED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closed2;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Rejected;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RXR:
        // 4
        // quietly stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_State_Stopping5(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // 1
        if (!apAuto->mInReset)
        {
            K2PPPOidAuto_TurnTimerOff(apAuto);
            printf("%s : STOPPING -> STARTING\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_Starting1;
            if (apAuto->mResetStarted)
            {
                apAuto->mResetStarted = FALSE;
                K2PPP_OIDAUTO_Run(apAuto, K2PPP_OidAutoEvent_LowerLayerIsUp);
            }
        }
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // 5r
        // stay here
        if (apAuto->mEnableResetOption)
        {
            apAuto->mResetStarted = TRUE;
            K2PPP_OIDAUTO_Run(apAuto, K2PPP_OidAutoEvent_LowerLayerIsDown);
        }
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // 4
        printf("%s : STOPPING -> CLOSING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closing4;
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // str/5
        printf("%s.Send_TermRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermRequest(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // tlf/3
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : STOPPING -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_TimeOut;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // 5
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // 5
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RCA:
        // 5
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RCN:
        // 5
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RTR:
        // sta/5
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTA:
        // tlf/3
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : STOPPING -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Terminate;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RUC:
        // scj/5
        printf("%s.Send_CodeReject\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_CodeReject(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // 5
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // tlf/3
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : STOPPING -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Rejected;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RXR:
        // 5
        // quietly stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_State_ReqSent6(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // 1
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : REQSENT -> STARTING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Starting1;
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // 6
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // irc,str/4
        printf("%s : REQSENT -> CLOSING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closing4;
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxTerminateCount);
        printf("%s.Send_TermRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermRequest(pPppOid);
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // scr/6
        printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigRequest(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // tlf/3p
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : REQSENT -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_TimeOut;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // sca/8
        printf("%s : REQSENT -> ACKSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_AckSent8;
        printf("%s.Send_ConfigAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigAck(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // scn/6
        printf("%s.Send_ConfigNack\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigNack(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RCA:
        // irc/7
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxConfigureCount);
        printf("%s : REQSENT -> ACKRCVD\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_AckRcvd7;
        break;

    case K2PPP_OidAutoEvent_RCN:
        // irc,scr/6
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxConfigureCount);
        printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigRequest(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTR:
        // sta/6
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTA:
        // 6
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RUC:
        // scj/6
        printf("%s.Send_CodeReject\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_CodeReject(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // 6
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // tlf/3
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : REQSENT -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Rejected;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RXR:
        // 6
        // quietly stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_State_AckRcvd7(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // 1
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : ACKRCVD -> STARTING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Starting1;
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // 7
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // irc,str/4
        printf("%s : ACKRCVD -> CLOSING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closing4;
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxTerminateCount);
        printf("%s.Send_TermRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermRequest(pPppOid);
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // scr/6
        printf("%s : ACKRCVD -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
        printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigRequest(pPppOid);
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // tlf/3p
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : ACKRCVD -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_TimeOut;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // tlu,sca/9
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : ACKRCVD -> OPENED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Opened9;
        printf("%s.OnUp\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->HostLayer.Layer.Api.OnUp(&pPppOid->HostLayer.Layer.Api);
        printf("%s.Send_ConfigAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigAck(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // scn/7
        printf("%s.Send_ConfigNack\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigNack(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RCA:
        // scr/6x
        // ignore stay here
        break;

    case K2PPP_OidAutoEvent_RCN:
        // scr/6x
        // ignore stay here
        break;

    case K2PPP_OidAutoEvent_RTR:
        // sta/6
        printf("%s : ACKRCVD -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RTA:
        // 6
        printf("%s : ACKRCVD -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
        break;

    case K2PPP_OidAutoEvent_RUC:
        // scj/7
        printf("%s.Send_CodeReject\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_CodeReject(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // 6
        printf("%s : ACKRCVD -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // tlf/3
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : ACKRCVD -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Rejected;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RXR:
        // 7
        // quietly stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_State_AckSent8(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // 1
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : ACKSENT -> STARTING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Starting1;
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // 8
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // irc,str/4
        printf("%s : ACKSENT -> CLOSING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closing4;
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxTerminateCount);
        printf("%s.Send_TermRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermRequest(pPppOid);
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // scr/8
        printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigRequest(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // tlf/3p
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : ACKSENT -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_TimeOut;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        // sca/8
        printf("%s.Send_ConfigAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigAck(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        // scn/6
        printf("%s : ACKSENT -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
        printf("%s.Send_ConfigNack\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigNack(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RCA:
        // tlu/9   (irc,tlu/9  - why irc?)
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : ACKSENT -> OPENED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Opened9;
        printf("%s.OnUp\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->HostLayer.Layer.Api.OnUp(&pPppOid->HostLayer.Layer.Api);
        break;

    case K2PPP_OidAutoEvent_RCN:
        // irc,scr/8
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxConfigureCount);
        printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigRequest(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RTR:
        // sta/6
        printf("%s : ACKSENT -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RTA:
        // 8
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RUC:
        // scj/8
        printf("%s.Send_CodeReject\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_CodeReject(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // 8
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // tlf/3
        K2PPPOidAuto_TurnTimerOff(apAuto);
        printf("%s : ACKSENT -> STOPPED\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopped3;
        pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Rejected;
        printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
        pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        break;

    case K2PPP_OidAutoEvent_RXR:
        // 8
        // quietly stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_State_Opened9(
    K2PPP_OIDAUTO *         apAuto,
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    K2PPP_OID * pPppOid;

    pPppOid = K2_GET_CONTAINER(K2PPP_OID, apAuto, Automaton);

    K2_ASSERT(!apAuto->mTimerIsOn);

    printf("%s(%s:%s)\n", __FUNCTION__, pPppOid->HostLayer.Layer.mName, gK2PPP_OidAutoEventNames[aOidAutoEvent]);
    switch (aOidAutoEvent)
    {
    case K2PPP_OidAutoEvent_LowerLayerIsUp:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_LowerLayerIsDown:
        // tld/1
        if (!apAuto->mInReset)
        {
            printf("%s : OPENED -> STARTING\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_Starting1;
            printf("%s.OnDown\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->HostLayer.Layer.Api.OnDown(&pPppOid->HostLayer.Layer.Api);
            if (apAuto->mResetStarted)
            {
                apAuto->mResetStarted = FALSE;
                K2PPP_OIDAUTO_Run(apAuto, K2PPP_OidAutoEvent_LowerLayerIsUp);
            }
        }
        break;

    case K2PPP_OidAutoEvent_AdminOpen:
        // 9r
        // else stay here
        if (apAuto->mEnableResetOption)
        {
            apAuto->mResetStarted = TRUE;
            K2PPP_OIDAUTO_Run(apAuto, K2PPP_OidAutoEvent_LowerLayerIsDown);
        }
        break;

    case K2PPP_OidAutoEvent_AdminClose:
        // tld,irc,str/4
        printf("%s : OPENED -> CLOSING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Closing4;
        printf("%s.OnDown\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->HostLayer.Layer.Api.OnDown(&pPppOid->HostLayer.Layer.Api);
        if (!apAuto->mNoConfig)
        {
            K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxTerminateCount);
            printf("%s.Send_TermRequest\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->mfSend_TermRequest(pPppOid);
        }
        else
        {
            printf("%s : CLOSING -> CLOSED\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_Closed2;
            pPppOid->HostLayer.Layer.mLastFinishReason = K2NET_LayerFinishReason_Closed;
            printf("%s.OnFinish(%s)\n", pPppOid->HostLayer.Layer.mName, K2NET_gLayerFinishNames[pPppOid->HostLayer.Layer.mLastFinishReason]);
            pPppOid->HostLayer.Layer.OnFinish(&pPppOid->HostLayer.Layer);
        }
        break;

    case K2PPP_OidAutoEvent_TO_NONZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_TO_ZERO:
        // -
        K2_ASSERT(0);
        break;

    case K2PPP_OidAutoEvent_RCR_GOOD:
        if (!apAuto->mNoConfig)
        {
            // tld,scr,sca/8
            printf("%s : OPENED -> ACKSENT\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_AckSent8;
            printf("%s.OnDown\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->HostLayer.Layer.Api.OnDown(&pPppOid->HostLayer.Layer.Api);
            printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->mfSend_ConfigRequest(pPppOid);
            printf("%s.Send_ConfigAck\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->mfSend_ConfigAck(pPppOid);
        }
        break;

    case K2PPP_OidAutoEvent_RCR_BAD:
        if (!apAuto->mNoConfig)
        {
            // tld,scr,scn/6
            printf("%s : OPENED -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
            apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
            printf("%s.OnDown\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->HostLayer.Layer.Api.OnDown(&pPppOid->HostLayer.Layer.Api);
            printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->mfSend_ConfigRequest(pPppOid);
            printf("%s.Send_ConfigNack\n", pPppOid->HostLayer.Layer.mName);
            pPppOid->mfSend_ConfigNack(pPppOid);
        }
        break;

    case K2PPP_OidAutoEvent_RCA:
        // tld,scr,irc/6x
        // ignore stay here
        break;

    case K2PPP_OidAutoEvent_RCN:
        // tld,scr,irc/6x
        // ignore stay here
        break;

    case K2PPP_OidAutoEvent_RTR:
        // tld,zrc,sta/5
        printf("%s : OPENED -> STOPPING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopping5;
        printf("%s.OnDown\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->HostLayer.Layer.Api.OnDown(&pPppOid->HostLayer.Layer.Api);
        K2PPPOidAuto_InitRestartCount(apAuto, 0);    // zrc
        printf("%s.Send_TermAck\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermAck(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RTA:
        // tld,scr,irc/6
        printf("%s : OPENED -> REQSENT\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_ReqSent6;
        printf("%s.OnDown\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->HostLayer.Layer.Api.OnDown(&pPppOid->HostLayer.Layer.Api);
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxTerminateCount);
        printf("%s.Send_ConfigRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_ConfigRequest(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RUC:
        // scj/9
        printf("%s.Send_CodeReject\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_CodeReject(pPppOid);
        // stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_GOOD:
        // 9
        // quietly stay here
        break;

    case K2PPP_OidAutoEvent_RXJ_BAD:
        // tld,irc,str/5
        printf("%s : OPENED -> STOPPING\n", pPppOid->HostLayer.Layer.mName);
        apAuto->mpState = K2PPP_OIDAUTO_State_Stopping5;
        printf("%s.OnDown\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->HostLayer.Layer.Api.OnDown(&pPppOid->HostLayer.Layer.Api);
        K2PPPOidAuto_InitRestartCount(apAuto, pPppOid->mMaxTerminateCount);
        printf("%s.Send_TermRequest\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_TermRequest(pPppOid);
        break;

    case K2PPP_OidAutoEvent_RXR:
        // ser/9
        printf("%s.Send_EchoReply\n", pPppOid->HostLayer.Layer.mName);
        pPppOid->mfSend_EchoReply(pPppOid);
        // stay here
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
K2PPP_OIDAUTO_Init(
    K2PPP_OIDAUTO *apAuto
)
{
    K2MEM_Zero(apAuto, sizeof(K2PPP_OIDAUTO));
    apAuto->mpState = (K2PPP_OIDAUTO_pf_StateFunc)K2PPP_OIDAUTO_State_Initial0;
}

void 
K2PPP_OIDAUTO_Run(
    K2PPP_OIDAUTO *         apAuto, 
    K2PPP_OidAutoEventType  aOidAutoEvent
)
{
    ((K2PPP_OIDAUTO_pf_StateFunc)apAuto->mpState)(apAuto, aOidAutoEvent);
}
