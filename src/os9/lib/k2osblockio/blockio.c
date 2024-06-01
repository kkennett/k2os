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

#include <k2osdev_blockio.h>
#include <k2osstor.h>
#include <kern/k2osddk.h>

K2OSSTOR_BLOCKIO        
K2OS_BlockIo_Attach(
    K2OS_IFINST_ID      aIfInstId,
    UINT32              aAccess,
    UINT32              aShare,
    K2OS_MAILBOX_TOKEN  aTokNotifyMailbox
)
{
    K2OS_RPC_OBJ_HANDLE     hRpcObj;
    K2OS_RPC_CALLARGS       args;
    UINT32                  actualOut;
    K2STAT                  stat;
    K2OS_BLOCKIO_CONFIG_IN  blockIoConfigIn;

    if ((0 == aIfInstId) ||
        (0 == (aAccess & K2OS_ACCESS_RW)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    hRpcObj = K2OS_Rpc_AttachByIfInstId(aIfInstId, NULL);
    if (NULL == hRpcObj)
        return NULL;

    K2MEM_Zero(&args, sizeof(args));

    args.mpInBuf = (UINT8 const *)&blockIoConfigIn;
    args.mInBufByteCount = sizeof(blockIoConfigIn);
    args.mMethodId = K2OS_BLOCKIO_METHOD_CONFIG;

    blockIoConfigIn.mAccess = aAccess;
    blockIoConfigIn.mShare = aShare;

    actualOut = 0;
    stat = K2OS_Rpc_Call(hRpcObj, &args, &actualOut);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Rpc_Release(hRpcObj);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    if (NULL != aTokNotifyMailbox)
    {
        K2OS_Rpc_SetNotifyTarget(hRpcObj, aTokNotifyMailbox);
    }

    return (K2OSSTOR_BLOCKIO)hRpcObj;
}

BOOL                    
K2OS_BlockIo_GetMedia(
    K2OSSTOR_BLOCKIO    aStorBlockIo,
    K2OS_STORAGE_MEDIA *apRetMedia
)
{
    K2OS_RPC_CALLARGS   args;
    UINT32              actualOut;
    K2STAT              stat;
    K2OS_STORAGE_MEDIA  dummy;

    if (NULL == aStorBlockIo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (NULL == apRetMedia)
        apRetMedia = &dummy;

    K2MEM_Zero(&args, sizeof(args));

    args.mpOutBuf = (UINT8 *)apRetMedia;
    args.mOutBufByteCount = sizeof(K2OS_STORAGE_MEDIA);
    args.mMethodId = K2OS_BLOCKIO_METHOD_GET_MEDIA;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorBlockIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    if (sizeof(K2OS_STORAGE_MEDIA) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return FALSE;
    }

    return TRUE;
}

K2OSSTOR_BLOCKIO_RANGE  
K2OS_BlockIo_RangeCreate(
    K2OSSTOR_BLOCKIO    aStorBlockIo,
    UINT64 const *      apRangeBaseBlock,
    UINT64 const *      apRangeBlockCount,
    BOOL                aMakePrivate
)
{
    K2OS_RPC_CALLARGS               args;
    UINT32                          actualOut;
    K2STAT                          stat;
    K2OS_BLOCKIO_RANGE_CREATE_IN    argsIn;
    K2OS_BLOCKIO_RANGE_CREATE_OUT   argsOut;

    if ((NULL == aStorBlockIo) ||
        (NULL == apRangeBaseBlock) ||
        (NULL == apRangeBlockCount) ||
        (0 == *apRangeBlockCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mMethodId = K2OS_BLOCKIO_METHOD_RANGE_CREATE;
    args.mpInBuf = (UINT8 const *)&argsIn;
    args.mInBufByteCount = sizeof(K2OS_BLOCKIO_RANGE_CREATE_IN);
    args.mpOutBuf = (UINT8 *)&argsOut;
    args.mOutBufByteCount = sizeof(K2OS_BLOCKIO_RANGE_CREATE_OUT);

    argsIn.mRangeBaseBlock = *apRangeBaseBlock;
    argsIn.mRangeBlockCount = *apRangeBlockCount;
    argsIn.mMakePrivate = aMakePrivate;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorBlockIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    K2_ASSERT(sizeof(K2OS_BLOCKIO_RANGE_CREATE_OUT) == actualOut);

    return (K2OSSTOR_BLOCKIO_RANGE)argsOut.mRange;
}

BOOL                    
K2OS_BlockIo_RangeDelete(
    K2OSSTOR_BLOCKIO        aStorBlockIo,
    K2OSSTOR_BLOCKIO_RANGE  aRange
)
{
    K2OS_RPC_CALLARGS               args;
    UINT32                          actualOut;
    K2STAT                          stat;
    K2OS_BLOCKIO_RANGE_DELETE_IN    argsIn;

    if ((NULL == aStorBlockIo) || (NULL == aRange))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mMethodId = K2OS_BLOCKIO_METHOD_RANGE_DELETE;
    args.mpInBuf = (UINT8 const *)&argsIn;
    args.mInBufByteCount = sizeof(K2OS_BLOCKIO_RANGE_DELETE_IN);

    argsIn.mRange = aRange;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorBlockIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL
K2OS_BlockIo_Transfer(
    K2OSSTOR_BLOCKIO        aStorBlockIo,
    K2OSSTOR_BLOCKIO_RANGE  aRange,
    UINT64 const *          apBytesOffset,
    UINT32                  aMemAddr,
    UINT32                  aByteCount,
    BOOL                    aIsWrite
)
{
    K2OS_RPC_CALLARGS           args;
    UINT32                      actualOut;
    K2STAT                      stat;
    K2OS_BLOCKIO_TRANSFER_IN    argsIn;

    if ((NULL == aStorBlockIo) || 
        (NULL == aRange) ||
        (NULL == apBytesOffset))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if ((0 != (aMemAddr & (K2OS_CACHELINE_BYTES - 1))) ||
        (0 != (aByteCount & (K2OS_CACHELINE_BYTES - 1))))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ALIGNMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mMethodId = K2OS_BLOCKIO_METHOD_TRANSFER;
    args.mpInBuf = (UINT8 const *)&argsIn;
    args.mInBufByteCount = sizeof(K2OS_BLOCKIO_TRANSFER_IN);

    argsIn.mIsWrite = aIsWrite;
    argsIn.mRange = aRange;
    argsIn.mBytesOffset = *apBytesOffset;
    argsIn.mByteCount = aByteCount;
    argsIn.mMemAddr = aMemAddr;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorBlockIo, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL                    
K2OS_BlockIo_Read(
    K2OSSTOR_BLOCKIO        aStorBlockIo,
    K2OSSTOR_BLOCKIO_RANGE  aRange,
    UINT64 const *          apBytesOffset,
    void *                  apBuffer,
    UINT32                  aByteCount
)
{
    UINT32  curAddr;
    UINT32  endAddr;
    UINT8   save;

    //
    // touch memory in case transfer is inside a copy on write map
    //
    curAddr = ((UINT32)apBuffer);
    endAddr = curAddr + aByteCount;
    do {
        save = *((UINT8 volatile *)curAddr);
        K2_CpuReadBarrier();
        *((UINT8 volatile *)curAddr) = save;
        curAddr += K2_VA_MEMPAGE_BYTES;
    } while (curAddr < endAddr);

    return K2OS_BlockIo_Transfer(aStorBlockIo, aRange, apBytesOffset, (UINT32)apBuffer, aByteCount, FALSE);
}

BOOL                    
K2OS_BlockIo_Write(
    K2OSSTOR_BLOCKIO        aStorBlockIo,
    K2OSSTOR_BLOCKIO_RANGE  aRange,
    UINT64 const *          apBytesOffset,
    void const *            apBuffer,
    UINT32                  aByteCount
)
{
    return K2OS_BlockIo_Transfer(aStorBlockIo, aRange, apBytesOffset, (UINT32)apBuffer, aByteCount, TRUE);
}

BOOL                    
K2OS_BlockIo_Detach(
    K2OSSTOR_BLOCKIO aStorBlockIo
)
{
    return K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)aStorBlockIo);
}
