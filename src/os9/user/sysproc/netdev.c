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

#include "netmgr.h"

NETDEV_L2_PROTO * NetDev_PPP_Create(NETDEV *apNetDev, K2OS_IPV4_ADAPTER_CONFIG const *apConfig)
{
    return NULL;
}

BOOL
NetDev_Init(
    NETDEV *                            apNetDev,
    K2OS_IPV4_ADAPTER_CONFIG const *    apConfig
)
{
    NETDEV_L2_PROTO *   pL2;

    K2MEM_Copy(&apNetDev->Proto.Ip.Adapter.Config, apConfig, sizeof(K2OS_IPV4_ADAPTER_CONFIG));

    K2_ASSERT(NULL == apNetDev->Proto.mpL2);

    if (K2_NetAdapter_Ethernet == apNetDev->Desc.mType)
    {
        apNetDev->Proto.mpL2 = pL2 = NetDev_Ether_Create(apNetDev, apConfig);
    }
    else if (K2_NetAdapter_PPP == apNetDev->Desc.mType)
    {
        apNetDev->Proto.mpL2 = pL2 = NetDev_PPP_Create(apNetDev, apConfig);
    }
    else
    {
        Debug_Printf("***NetDev_Init - unknown/unsupported adapter type\n");
    }

    return (NULL == pL2) ? FALSE : TRUE;
}

void    
NetDev_Start(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.mpL2->Iface.OnStart(apNetDev);
}

void    
NetDev_Recv_NetIo(
    NETDEV *                apNetDev,
    K2OS_NETIO_MSG const *  apNetIoMsg
)
{
    NETDEV_L2_PROTO * pL2Proto;

    if (apNetIoMsg->mShort == K2OS_NetIoMsgShort_Recv)
    {
        pL2Proto = apNetDev->Proto.mpL2;
        pL2Proto->Iface.OnRecv(apNetDev, (UINT8 *)apNetIoMsg->mPayload[0], apNetIoMsg->mPayload[1]);
        K2OS_NetIo_RelBuffer(apNetDev->mNetIo, apNetIoMsg->mPayload[0]);
    }
}

void    
NetDev_Recv_RpcNotify(
    NETDEV *            apNetDev,
    K2OS_MSG const *    apRpcNotifyMsg
)
{
    K2_ASSERT(0);
}

void    
NetDev_Stopped(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.mpL2->Iface.OnStop(apNetDev);
}

void    
NetDev_Deinit(
    NETDEV * apNetDev
)
{
    apNetDev->Proto.mpL2->Iface.Deinit(apNetDev);
    K2_ASSERT(NULL == apNetDev->Proto.mpL2);
}

void
NetDev_RelBuffer(
    NETDEV_BUFFER * apBuffer
)
{
    NETDEV * pNetDev;

    pNetDev = apBuffer->mpNetDev;
    K2_ASSERT(NULL != pNetDev);
    pNetDev->Proto.mpL2->Iface.RelBuffer(apBuffer);
    NetDev_BufferPut(apBuffer);
}
