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
#include <lib/k2mem.h>
#include <lib/k2ppplink.h>

typedef struct _TIMERQ TIMERQ;
struct _TIMERQ
{
    void *                  mpContext;
    UINT32                  mLeft;
    UINT32                  mPeriod;
    K2NET_pf_OnTimerExpired mfTimerExpired;
    TIMERQ *                mpNext;
};
TIMERQ * gpTimers = NULL;

void *
MyMemAlloc(
    UINT32  aByteCount
)
{
    return malloc(aByteCount);
}

void
MyMemFree(
    void *  aPtr
)
{
    free(aPtr);
}

void
InsertTimer(
    TIMERQ *    apNew
)
{
    TIMERQ *pFind;
    TIMERQ *pPrev;

    if (NULL == gpTimers)
    {
        apNew->mpNext = NULL;
        gpTimers = apNew;
    }
    else
    {
        pFind = gpTimers;
        pPrev = NULL;
        do {
            if (apNew->mLeft < pFind->mLeft)
            {
                pFind->mLeft -= apNew->mLeft;
                apNew->mpNext = pFind;
                if (NULL == pPrev)
                {
                    gpTimers = apNew;
                }
                else
                {
                    pPrev->mpNext = apNew;
                }
                break;
            }
            apNew->mLeft -= pFind->mLeft;
            if (NULL == pFind->mpNext)
            {
                pFind->mpNext = apNew;
                apNew->mpNext = NULL;
                break;
            }
            pPrev = pFind;
            pFind = pFind->mpNext;
        } while (1);
    }
}

void
MyAddTimer(
    void *                  apContext,
    UINT32                  aPeriod,
    K2NET_pf_OnTimerExpired afTimerExpired
)
{
    TIMERQ *    pNew;

    pNew = (TIMERQ *)K2NET_MemAlloc(sizeof(TIMERQ));
    if (NULL == pNew)
        return;

    pNew->mpContext = apContext;
    pNew->mLeft = aPeriod;
    pNew->mPeriod = aPeriod;
    pNew->mfTimerExpired = afTimerExpired;

    InsertTimer(pNew);
}

void
MyDelTimer(
    void * apContext
)
{
    TIMERQ *    pFind;
    TIMERQ *    pPrev;

    if (NULL == gpTimers)
        return;

    pFind = gpTimers;
    pPrev = NULL;
    do {
        if (pFind->mpContext == apContext)
        {
            if (NULL == pPrev)
            {
                gpTimers = pFind->mpNext;
                if (NULL != gpTimers)
                {
                    gpTimers->mLeft += pFind->mLeft;
                }
            }
            else
            {
                pPrev->mpNext = pFind->mpNext;
                if (NULL != pFind->mpNext)
                {
                    pFind->mpNext->mLeft += pFind->mLeft;
                }
            }
            K2NET_MemFree(pFind);
            return;
        }
        pPrev = pFind;
        pFind = pFind->mpNext;
    } while (NULL != pFind);
}

BOOL Poll(void)
{
    // if nothing happens, return FALSE;
    return FALSE;
}

UINT8 sgRecvFrame[K2_PPP_MRU_MINIMUM];

void pppStreamOut(K2PPPF *apStream, UINT8 aByteOut)
{
    // loopback
//    printf("%02X ", aByteOut);
    apStream->mfDataIn(apStream, aByteOut);
}

typedef struct _PPPLCP_CONFIG PPPLCP_CONFIG;
struct _PPPLCP_CONFIG
{
    // config data for the LCP goes here
    UINT32 mTbd;
};

typedef struct _PPPLCP_PROTO_CHUNK PPPLCP_PROTO_CHUNK;
struct _PPPLCP_PROTO_CHUNK
{
    K2NET_STACK_PROTO_CHUNK_HDR ProtoHdr;    // 'LCPP', protocolId = K2PPP_PROTO_LCP, no subchunks
    PPPLCP_CONFIG               LcpConfig;
};

typedef struct _PPPIPCP_CONFIG PPPIPCP_CONFIG;
struct _PPPIPCP_CONFIG
{
    // config data for the IPCP goes here
    UINT32 mTbd;
};

typedef struct _PPPIPCP_PROTO_CHUNK PPPIPCP_PROTO_CHUNK;
struct _PPPIPCP_PROTO_CHUNK
{
    K2NET_STACK_PROTO_CHUNK_HDR ProtoHdr;
    PPPIPCP_CONFIG              IpcpConfig;
};

typedef struct _K2NET_IPV4_CONFIG K2NET_IPV4_CONFIG;
struct _K2NET_IPV4_CONFIG
{
    // config data for the IPV4 goes here
    UINT32 mTbd;
};

typedef struct _K2NET_IPV4_PROTO_CHUNK K2NET_IPV4_PROTO_CHUNK;
struct _K2NET_IPV4_PROTO_CHUNK
{
    K2NET_STACK_PROTO_CHUNK_HDR ProtoHdr;
    K2NET_IPV4_CONFIG           Ipv4Config;
};

typedef struct _PPPIPV4_CONFIG PPPIPV4_CONFIG;
struct _PPPIPV4_CONFIG
{
    // config data for the IPV4-to-PPP goes here
    UINT32 mTbd;
};

typedef struct _PPPIPV4_PROTO_CHUNK PPPIPV4_PROTO_CHUNK;
struct _PPPIPV4_PROTO_CHUNK
{
    K2NET_STACK_PROTO_CHUNK_HDR ProtoHdr;    // 'LCPP', protocolId = K2PPP_PROTO_LCP, no subchunks
    PPPIPV4_CONFIG              PppIpv4Config;
    K2NET_IPV4_PROTO_CHUNK      NetIpv4;
};

typedef struct _PPPSTACK_PROTO_CHUNK PPPSTACK_PROTO_CHUNK;
struct _PPPSTACK_PROTO_CHUNK
{
    K2NET_STACK_PROTO_CHUNK_HDR ProtoHdr;    // 'PPPS', protocolId = K2NET_PROTOCOLID_PPP, Lcp Subchunk only
    PPPLCP_PROTO_CHUNK          Lcp;
    PPPIPCP_PROTO_CHUNK         Ipcp;
    PPPIPV4_PROTO_CHUNK         PppIpv4;
};

typedef struct _PPPSTACK_CONFIG PPPSTACK_CONFIG;
struct _PPPSTACK_CONFIG
{
    K2_CHUNK_HDR            Hdr;
    PPPSTACK_PROTO_CHUNK    StackProto;
};

typedef struct _PPPSTACK PPPSTACK;
struct _PPPSTACK
{
    K2_NetAdapterType   mPhysType;
    K2_NET_ADAPTER_ADDR PhysAdapterAddr;
    PPPSTACK_CONFIG     ConfigChunk;
};

PPPSTACK gPppStackDef =
{
    // PPPSTACK.mPhysType
    K2_NetAdapter_PPP,
    // PPPSTACK.PhysAdapterAddr
    {
        1,
        { 1, }
    },
    // PPPSTACK.ConfigChunk
    {
        // PPPSTACK.ConfigChunk.Hdr
        {
            K2_MAKEID4('S','T','A','K'),
            sizeof(PPPSTACK_CONFIG),
            K2_FIELDOFFSET(PPPSTACK_CONFIG, StackProto)
        },
        // PPPSTACK.ConfigChunk.StackProto
        {
            // PPPSTACK.ConfigChunk.StackProto.ProtoHdr
            {
                // PPPSTACK.ConfigChunk.StackProto.ProtoHdr.Hdr
                {
                    K2_MAKEID4('P','P','P','S'),
                    sizeof(PPPSTACK_PROTO_CHUNK),
                    K2_FIELDOFFSET(PPPSTACK_PROTO_CHUNK, Lcp)
                },
                // PPPSTACK.ConfigChunk.StackProto.ProtoHdr.mProtocolId
                K2_IANA_PROTOCOLID_PPP
            },
            // PPPSTACK.ConfigChunk.StackProto.Lcp
            {
                // PPPSTACK.ConfigChunk.StackProto.Lcp.ProtoHdr
                {
                    // PPPSTACK.ConfigChunk.StackProto.Lcp.ProtoHdr.Hdr
                    {
                        K2_MAKEID4('L','C','P','P'),
                        sizeof(PPPLCP_PROTO_CHUNK),
                        0   // no subchunks
                    },
                    // PPPSTACK.ConfigChunk.StackProto.Lcp.ProtoHdr.mProtocolId
                    K2_PPP_PROTO_LCP
                },
                // PPPSTACK.ConfigChunk.StackProto.Lcp.LcpConfig
                {
                    // mTbd
                    0
                }
            },
            // PPPSTACK.ConfigChunk.StackProto.Ipcp
            {
                // PPPSTACK.ConfigChunk.StackProto.Ipcp.ProtoHdr
                {
                    // PPPSTACK.ConfigChunk.StackProto.Ipcp.ProtoHdr.Hdr
                    {
                        K2_MAKEID4('I','P','C','P'),
                        sizeof(PPPIPCP_PROTO_CHUNK),
                        0   // no subchunks
                    },
                    // PPPSTACK.ConfigChunk.StackProto.Ipcp.ProtoHdr.mProtocolId
                    K2_PPP_PROTO_IPCP
                },
                // PPPSTACK.ConfigChunk.StackProto.Ipcp.IpcpConfig
                {
                    // mTbd
                    0
                }
            },
            // PPPSTACK.ConfigChunk.StackProto.PppIpv4
            {
                // PPPSTACK.ConfigChunk.StackProto.PppIpv4.ProtoHdr
                {
                    // PPPSTACK.ConfigChunk.StackProto.PppIpv4.ProtoHdr.Hdr
                    {
                        K2_MAKEID4('P','I','P','4'),
                        sizeof(PPPIPV4_PROTO_CHUNK),
                        K2_FIELDOFFSET(PPPIPV4_PROTO_CHUNK, NetIpv4)
                    },
                    // PPPSTACK.ConfigChunk.StackProto.PppIpv4.ProtoHdr.mProtocolId
                    K2_PPP_PROTO_IPV4
                },
                // PPPSTACK.ConfigChunk.StackProto.PppIpv4.PppIpv4Config
                {
                    // mTbd
                    0
                },
                // PPPSTACK.ConfigChunk.StackProto.PppIpv4.NetIpv4
                {
                    // PPPSTACK.ConfigChunk.StackProto.PppIpv4.NetIpv4.ProtoHdr
                    {
                        // PPPSTACK.ConfigChunk.StackProto.PppIpv4.NetIpv4.ProtoHdr.Hdr
                        {
                            K2_MAKEID4('I','P','V','4'),
                            sizeof(K2NET_IPV4_PROTO_CHUNK),
                            0   // no subchunks
                        },
                        // PPPSTACK.ConfigChunk.StackProto.PppIpv4.NetIpv4.ProtoHdr.mProtocolId
                        K2_IANA_PROTOCOLID_IPV4
                    },
                    // PPPSTACK.ConfigChunk.StackProto.PppIpv4.NetIpv4.Ipv4Config
                    {
                        // mTbd
                        0
                    }
                }
            }
        }
    }
};

void * gOpen = NULL;

void MyDoorbell(void *apContext)
{
    K2NET_MSG msg;

    if (K2NET_Proto_GetMsg(gOpen, &msg))
    {
        K2_ASSERT(msg.mType == K2NET_MSGTYPE);
        if (msg.mShort == K2NET_MsgShort_Timeout)
        {
            K2NET_Proto_Close(gOpen);
        }
    }
}

BOOL
MySerialOpen(
    K2PPP_SERIALIF *apSerialIf
)
{
    return TRUE;
}

void
MySerialClose(
    K2PPP_SERIALIF *apSerialIf
)
{
}

BOOL 
MySerialIsConnected(
    K2PPP_SERIALIF *apSerialIf
)
{
    return TRUE;
}

void * gpSerialRecvContext;

void
MySerialDataOut(
    K2PPP_SERIALIF *apSerialIf,
    UINT8           aByteOut
)
{
    // loop back
    K2PPP_DataReceived(gpSerialRecvContext, aByteOut);
}

K2PPP_SERIALIF MySerialIf = {
    "MySerial",
    1,
    { 0, },
    MySerialOpen,
    MySerialIsConnected,
    MySerialDataOut,
    MySerialClose
};

void
RunPppTest(
    void
)
{
    UINT32              v;
    TIMERQ *            pTimer;
    K2NET_PROTO_PROP    ipv4ProtoProp;
    BOOL                atLeastOne;

    K2NET_Init(MyMemAlloc, MyMemFree, MyAddTimer, MyDelTimer);

    K2PPP_Init();

    K2PPP_RegisterHw(&MySerialIf, &gpSerialRecvContext);
    
    //
    // will fail if phys layer specified isn't registered
    // or any of the protocols referenced in the def aren't registered
    //
    K2NET_StackDefine((K2NET_STACK_DEF *)&gPppStackDef);

    //
    // an instance of LCP should exist now that we derfined a PPP stack
    //

    //
    // open LCP so that stuff tries to connect
    //
    v = sizeof(K2NET_PROTO_PROP);
    K2NET_Proto_Enum(K2_IANA_PROTOCOLID_IPV4, &v, &ipv4ProtoProp);
    gOpen = K2NET_Proto_Open(ipv4ProtoProp.mInstanceId, NULL, MyDoorbell);

    printf("\n--------------------------------------------------------------\n\n");
    do {
        if (!Poll())
        {
            K2NET_DeliverNotifications();
            v = GetTickCount();
            Sleep(5);
            v = GetTickCount() - v;
            if (NULL != gpTimers)
            {
                atLeastOne = FALSE;
                do {
                    if (gpTimers->mLeft > v)
                    {
                        gpTimers->mLeft -= v;
                        break;
                    }
                    v -= gpTimers->mLeft;
                    gpTimers->mLeft = 0;
                    while (gpTimers->mLeft == 0)
                    {
                        pTimer = gpTimers;
                        gpTimers = gpTimers->mpNext;

                        pTimer->mLeft = pTimer->mPeriod;
                        InsertTimer(pTimer);

                        // this may remove the timer
                        atLeastOne = TRUE;
                        pTimer->mfTimerExpired(pTimer->mpContext);

                        if (NULL == gpTimers)
                            break;
                    }
                } while ((v > 0) && (NULL != gpTimers));
                if (atLeastOne)
                {
                    printf("\n--------------------------------------------------------------\n\n");
                }
            }
        }
    } while (1);
}

