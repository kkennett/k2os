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

#ifndef __K2PPPLINK_H
#define __K2PPPLINK_H

/* --------------------------------------------------------------------------------- */

#include <k2systype.h>
#include <lib/k2net.h>
#include <lib/k2pppf.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// STRUCTURE DECLARATIONS
//

typedef struct _K2PPP_OID       K2PPP_OID;
typedef struct _K2PPP_IPV4_OID  K2PPP_IPV4_OID;
typedef struct _K2PPP_IPCP_OID  K2PPP_IPCP_OID;
typedef struct _K2PPP_LCP_OID   K2PPP_LCP_OID;
typedef struct _K2PPP_LINK      K2PPP_LINK;
typedef struct _K2PPP_HW        K2PPP_HW;

//
// FUNCTION POINTER TYPES
//

typedef void (*K2PPP_OID_pf_OnAction)(K2PPP_OID *apOid);

//
// STRUCTURES
//

struct _K2PPP_OIDAUTO
{
    void *  mpState;
    UINT32  mRestartCount;
    BOOL    mTimerIsOn;
    BOOL    mNoConfig;
    BOOL    mEnableResetOption;
    BOOL    mResetStarted;
    BOOL    mInReset;
};

struct _K2PPP_OID
{
    K2NET_HOST_LAYER        HostLayer;
    K2PPP_OIDAUTO           Automaton;

    BOOL                    mIsUp;
    UINT32                  mAcquireCount;
    UINT32                  mNextClientId;
    K2PPP_OID_pf_OnAction   OnFirstAcquire;
    K2PPP_OID_pf_OnAction   OnLastRelease;

    UINT32                  mTimer_Period;
    UINT32                  mMaxConfigureCount;
    UINT32                  mMaxTerminateCount;

    K2PPP_OID_pf_OnAction   mfSend_ConfigRequest;
    K2PPP_OID_pf_OnAction   mfSend_ConfigAck;
    K2PPP_OID_pf_OnAction   mfSend_ConfigNack;
    K2PPP_OID_pf_OnAction   mfSend_TermRequest;
    K2PPP_OID_pf_OnAction   mfSend_TermAck;
    K2PPP_OID_pf_OnAction   mfSend_CodeReject;
    K2PPP_OID_pf_OnAction   mfSend_EchoReply;
};

struct _K2PPP_LCP_OID
{
    K2PPP_OID       PppOid;
    K2LIST_ANCHOR   OpenList;
};

struct _K2PPP_IPV4_OID
{
    K2PPP_OID   PppOid;
    K2PPP_OID * mpIpcpOid;
    void *      mpIpcpOpen;
    BOOL        mTimedOutSentToAbove;
};

struct _K2PPP_IPCP_OID
{
    K2PPP_OID       PppOid;
    K2LIST_ANCHOR   OpenList;
    void *          mpLcpOpen;
};

struct _K2PPP_LINK
{
    K2NET_HOST_LAYER        NetHostLayer;

    UINT32                  mAcquireCount;

    K2_PPP_LinkPhaseType    mLinkPhase;

    K2NET_HOST_LAYER *      mpLcpHostLayer;
    K2NET_LAYER *           mpAuthLayer;
    K2NET_LAYER *           mpQualLayer;

    UINT8 *                 mpLastRecvConfigReq;
    UINT32                  mLastRecvConfigReqLen;

    UINT16                  mOptionVal_MRU;    // K2PPP_LCPOPT_MRU 
    UINT32                  mOptionVal_ACCM;   // K2PPP_LCPOPT_ACCM
    UINT32                  mOptionVal_MAGIC;  // K2PPP_LCPOPT_MAGIC
    BOOL                    mOptionVal_PFC;    // K2PPP_LCPOPT_PFC
    BOOL                    mOptionVal_ACFC;   // K2PPP_LCPOPT_ACFC

    UINT8                   mConfigIdent;
};

typedef struct _K2PPP_SERIALIF K2PPP_SERIALIF;

typedef BOOL (*K2PPP_pf_Serial_Open)(K2PPP_SERIALIF *apSerialIf);
typedef BOOL (*K2PPP_pf_Serial_IsConnected)(K2PPP_SERIALIF *apSerialIf);
typedef void (*K2PPP_pf_Serial_DataOut)(K2PPP_SERIALIF *apSerialIf, UINT8 aByteOut);
typedef void (*K2PPP_pf_Serial_Close)(K2PPP_SERIALIF *apSerialIf);

struct _K2PPP_SERIALIF
{
    char                            mName[K2_NET_ADAPTER_NAME_MAX_LEN + 1];
    UINT32                          mAddrLen;
    UINT8                           mAddr[K2_NET_ADAPTER_ADDR_MAX_LEN];
    K2PPP_pf_Serial_Open            Open;
    K2PPP_pf_Serial_IsConnected     IsConnected;
    K2PPP_pf_Serial_DataOut         DataOut;
    K2PPP_pf_Serial_Close           Close;
};

struct _K2PPP_HW
{
    K2PPPF              Framer;
    K2NET_PHYS_LAYER    NetPhysLayer;
    UINT32              mOpens;
    BOOL                mAcquired;
    BOOL                mIsUp;
    K2PPP_SERIALIF *    mpSerialIf;
    K2PPP_SERIALIF      SerialIf;
    void *              mpContext;
    UINT8               mFrameBuffer[K2_PPP_MRU_MINIMUM];
};

//
// APIs
//

K2STAT  K2PPP_Init(void);
K2STAT  K2PPP_RegisterHw(K2PPP_SERIALIF *apSerialIf, void **appRetRecvContext);
void    K2PPP_DataReceived(void *apRecvContext, UINT8 aByteIn);
void    K2PPP_Done(void);

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif // __K2PPPLINK_H
