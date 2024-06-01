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

#include <k2osdev_netio.h>
#include <k2osnet.h>
#include <kern/k2osddk.h>

K2OS_NETIO  
K2OS_NetIo_Attach(
    K2OS_IFINST_ID      aIfInstId,
    void *              apContext,
    K2OS_MAILBOX_TOKEN  aTokMailbox
)
{
    K2OS_RPC_OBJ_HANDLE     hRpcObj;
    K2OS_RPC_CALLARGS       args;
    UINT32                  actualOut;
    K2STAT                  stat;
    K2OS_NETIO_CONFIG_IN    configIn;

    if ((0 == aIfInstId) ||
        (NULL == aTokMailbox))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    hRpcObj = K2OS_Rpc_AttachByIfInstId(aIfInstId, NULL);
    if (NULL == hRpcObj)
        return NULL;

    K2MEM_Zero(&args, sizeof(args));

    args.mpInBuf = (UINT8 const *)&configIn;
    args.mInBufByteCount = sizeof(configIn);
    args.mMethodId = K2OS_NETIO_METHOD_CONFIG;

    configIn.mpContext = apContext;
    configIn.mTokMailbox = aTokMailbox;

    actualOut = 0;
    stat = K2OS_Rpc_Call(hRpcObj, &args, &actualOut);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Rpc_Release(hRpcObj);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    K2OS_Rpc_SetNotifyTarget(hRpcObj, aTokMailbox);

    return (K2OS_NETIO)hRpcObj;
}

BOOL        
K2OS_NetIo_Detach(
    K2OS_NETIO aNetIo
)
{
    return K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)aNetIo);
}

BOOL        
K2OS_NetIo_GetDesc(
    K2OS_NETIO              aNetIo,
    K2_NET_ADAPTER_DESC *   apRetDesc
)
{
    K2OS_RPC_CALLARGS   args;
    UINT32              actualOut;
    K2STAT              stat;
    K2_NET_ADAPTER_DESC dummy;

    if (NULL == aNetIo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (NULL == apRetDesc)
        apRetDesc = &dummy;

    K2MEM_Zero(&args, sizeof(args));

    args.mpOutBuf = (UINT8 *)apRetDesc;
    args.mOutBufByteCount = sizeof(dummy);
    args.mMethodId = K2OS_NETIO_METHOD_GET_DESC;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aNetIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    if (sizeof(K2_NET_ADAPTER_DESC) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return FALSE;
    }

    return TRUE;
}

BOOL        
K2OS_NetIo_GetState(
    K2OS_NETIO                  aNetIo,
    K2OS_NETIO_ADAPTER_STATE *  apRetState
)
{
    K2OS_RPC_CALLARGS           args;
    UINT32                      actualOut;
    K2STAT                      stat;
    K2OS_NETIO_ADAPTER_STATE    dummy;

    if (NULL == aNetIo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (NULL == apRetState)
        apRetState = &dummy;

    K2MEM_Zero(&args, sizeof(args));

    args.mpOutBuf = (UINT8 *)apRetState;
    args.mOutBufByteCount = sizeof(dummy);
    args.mMethodId = K2OS_NETIO_METHOD_GET_STATE;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aNetIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    if (sizeof(K2OS_NETIO_ADAPTER_STATE) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return FALSE;
    }

    return TRUE;
}

BOOL        
K2OS_NetIo_GetBufferStats(
    K2OS_NETIO              aNetIo,
    K2OS_NETIO_BUFCOUNTS *  apRetTotal,
    K2OS_NETIO_BUFCOUNTS *  apRetAvail
)
{
    K2OS_NETIO_BUFCOUNTS        bufCounts[2];
    K2OS_RPC_CALLARGS           args;
    UINT32                      actualOut;
    K2STAT                      stat;

    if (NULL == aNetIo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpOutBuf = (UINT8 *)bufCounts;
    args.mOutBufByteCount = sizeof(K2OS_NETIO_BUFCOUNTS) * 2;
    args.mMethodId = K2OS_NETIO_METHOD_BUFSTATS;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aNetIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    if ((sizeof(K2OS_NETIO_BUFCOUNTS) * 2) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return FALSE;
    }

    if (NULL != apRetTotal)
    {
        K2MEM_Copy(apRetTotal, &bufCounts[0], sizeof(K2OS_NETIO_BUFCOUNTS));
    }

    if (NULL != apRetAvail)
    {
        K2MEM_Copy(apRetAvail, &bufCounts[1], sizeof(K2OS_NETIO_BUFCOUNTS));
    }

    return TRUE;
}

UINT8 *     
K2OS_NetIo_AcqSendBuffer(
    K2OS_NETIO  aNetIo,
    UINT32 *    apRetMTU
)
{
    K2OS_NETIO_ACQBUFFER_OUT    acqOut;
    K2OS_RPC_CALLARGS           args;
    UINT32                      actualOut;
    K2STAT                      stat;

    if (NULL == aNetIo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpOutBuf = (UINT8 *)&acqOut;
    args.mOutBufByteCount = sizeof(acqOut);
    args.mMethodId = K2OS_NETIO_METHOD_ACQBUFFER;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aNetIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    if (sizeof(K2OS_NETIO_ACQBUFFER_OUT) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return NULL;
    }

    if (NULL != apRetMTU)
    {
        *apRetMTU = acqOut.mMTU;
    }

    return (UINT8 *)acqOut.mBufVirtAddr;
}

BOOL        
K2OS_NetIo_Send(
    K2OS_NETIO  aNetIo,
    UINT8 *     apBuffer,
    UINT32      aSendBytes
)
{
    K2OS_RPC_CALLARGS   args;
    K2STAT              stat;
    UINT32              actualOut;
    K2OS_NETIO_SEND_IN  sendIn;

    if ((NULL == aNetIo) ||
        (NULL == apBuffer) ||
        (0 == aSendBytes))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpInBuf = (UINT8 *)&sendIn;
    args.mInBufByteCount = sizeof(sendIn);
    args.mMethodId = K2OS_NETIO_METHOD_SEND;

    sendIn.mBufVirtAddr = (UINT32)apBuffer;
    sendIn.mSendBytes = aSendBytes;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aNetIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL        
K2OS_NetIo_RelBuffer(
    K2OS_NETIO  aNetIo,
    UINT32      aBufVirtAddr
)
{
    K2OS_RPC_CALLARGS   args;
    K2STAT              stat;
    UINT32              actualOut;

    if ((NULL == aNetIo) ||
        (0 == aBufVirtAddr))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpInBuf = (UINT8 *)&aBufVirtAddr;
    args.mInBufByteCount = sizeof(UINT32);
    args.mMethodId = K2OS_NETIO_METHOD_RELBUFFER;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aNetIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL        
K2OS_NetIo_SetEnable(
    K2OS_NETIO  aNetIo,
    BOOL        aSetEnable
)
{
    K2OS_RPC_CALLARGS   args;
    K2STAT              stat;
    UINT32              actualOut;

    if (NULL == aNetIo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpInBuf = (UINT8 *)&aSetEnable;
    args.mInBufByteCount = sizeof(BOOL);
    args.mMethodId = K2OS_NETIO_METHOD_SETENABLE;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aNetIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL        
K2OS_NetIo_GetEnable(
    K2OS_NETIO  aNetIo,
    BOOL *      apRetEnable
)
{
    K2OS_RPC_CALLARGS   args;
    UINT32              actualOut;
    K2STAT              stat;
    BOOL                temp;

    if (NULL == aNetIo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpOutBuf = (UINT8 *)&temp;
    args.mOutBufByteCount = sizeof(temp);
    args.mMethodId = K2OS_NETIO_METHOD_GETENABLE;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aNetIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    if (sizeof(BOOL) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return FALSE;
    }

    if (NULL != apRetEnable)
    {
        *apRetEnable = temp;
    }

    return TRUE;
}


