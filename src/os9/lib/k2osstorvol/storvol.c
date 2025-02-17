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

#include <k2osstor.h>
#include <kern/k2osddk.h>

K2OSSTOR_VOLUME 
K2OS_Vol_Attach(
    K2OS_IFINST_ID      aVolIfInstId,
    UINT32              aAccess,
    UINT32              aShare,
    K2OS_MAILBOX_TOKEN  aTokNotifyMailbox
)
{
    K2OS_RPC_OBJ_HANDLE     hRpcObj;
    K2OS_RPC_CALLARGS       args;
    UINT32                  actualOut;
    K2STAT                  stat;
    K2OS_STORVOL_CONFIG_IN  configIn;

    if ((0 == aVolIfInstId) ||
        (0 == (aAccess & K2OS_ACCESS_RW)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    hRpcObj = K2OS_Rpc_AttachByIfInstId(aVolIfInstId, NULL);
    if (NULL == hRpcObj)
        return NULL;

    K2MEM_Zero(&args, sizeof(args));

    args.mpInBuf = (UINT8 const *)&configIn;
    args.mInBufByteCount = sizeof(configIn);
    args.mMethodId = K2OS_StoreVol_Method_Config;

    configIn.mAccess = aAccess;
    configIn.mShare = aShare;

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

    return (K2OSSTOR_VOLUME)hRpcObj;
}

K2OSSTOR_VOLUME 
K2OS_Vol_Create(
    K2_GUID128 const *  apVolumeId,
    UINT32              aShare,
    K2OS_IFINST_ID *    apRetIfInstId,
    K2OS_MAILBOX_TOKEN  aTokNotifyMailbox
)
{
    K2OS_IFENUM_TOKEN           tokEnum;
    K2OS_IFINST_DETAIL          detail;
    UINT32                      ioCount;
    BOOL                        ok;
    K2OS_RPC_OBJ_HANDLE         hVolMgr;
    K2STAT                      stat;
    K2OS_RPC_CALLARGS           callArgs;
    K2OS_STORVOLMGR_CREATE_IN   createIn;
    K2OS_STORVOLMGR_CREATE_OUT  createOut;
    UINT32                      actualOut;
    K2OSSTOR_VOLUME             result;

    tokEnum = K2OS_IfEnum_Create(FALSE, 0, K2OS_IFACE_CLASSCODE_STORAGE_VOLMGR, NULL);
    if (NULL == tokEnum)
    {
        return NULL;
    }

    ioCount = 1;
    ok = K2OS_IfEnum_Next(tokEnum, &detail, &ioCount);
    K2OS_Token_Destroy(tokEnum);

    if ((!ok) || (ioCount != 1))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NO_INTERFACE);
        return NULL;
    }

    hVolMgr = K2OS_Rpc_AttachByIfInstId(detail.mInstId, NULL);
    if (NULL == hVolMgr)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return NULL;
    }

    if ((NULL == apVolumeId) ||
        (NULL == apRetIfInstId) || 
        (K2MEM_VerifyZero(apVolumeId, sizeof(K2_GUID128))))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    K2MEM_Zero(&callArgs, sizeof(callArgs));
    callArgs.mMethodId = K2OS_StoreVolMgr_Method_Create;
    callArgs.mInBufByteCount = sizeof(K2OS_STORVOLMGR_CREATE_IN);
    callArgs.mpInBuf = (UINT8 const *)&createIn;
    callArgs.mOutBufByteCount = sizeof(K2OS_STORVOLMGR_CREATE_OUT);
    callArgs.mpOutBuf = (UINT8 *)&createOut;

    K2MEM_Copy(&createIn.mId, apVolumeId, sizeof(K2_GUID128));
    createIn.mShare = aShare;

    actualOut = 0;
    stat = K2OS_Rpc_Call(hVolMgr, &callArgs, &actualOut);

    K2OS_Rpc_Release(hVolMgr);

    if (actualOut != sizeof(K2OS_STORVOLMGR_CREATE_OUT))
    {
        stat = K2STAT_ERROR_BAD_SIZE;
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    result = K2OS_Vol_Attach(createOut.mIfInstId, K2OS_ACCESS_RW, aShare, aTokNotifyMailbox);
    if (NULL == result)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return NULL;
    }

    *apRetIfInstId = createOut.mIfInstId;

    return result;
}

BOOL            
K2OS_Vol_GetInfo(
    K2OSSTOR_VOLUME     aStorVol,
    K2_STORAGE_VOLUME * apRetVolumeInfo
)
{
    K2OS_RPC_CALLARGS   args;
    UINT32              actualOut;
    K2STAT              stat;
    K2_STORAGE_VOLUME   dummy;

    if (NULL == aStorVol)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (NULL == apRetVolumeInfo)
        apRetVolumeInfo = &dummy;

    K2MEM_Zero(&args, sizeof(args));

    args.mpOutBuf = (UINT8 *)apRetVolumeInfo;
    args.mOutBufByteCount = sizeof(K2_STORAGE_VOLUME);
    args.mMethodId = K2OS_StoreVol_Method_GetInfo;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorVol, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    if (sizeof(K2_STORAGE_VOLUME) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return FALSE;
    }

    return TRUE;
}

BOOL            
K2OS_Vol_AddPartition(
    K2OSSTOR_VOLUME             aStorVol, 
    K2OSSTOR_VOLUME_PART const *apVolPart
)
{
    K2OS_RPC_CALLARGS   args;
    UINT32              actualOut;
    K2STAT              stat;

    if ((NULL == aStorVol) || 
        (NULL == apVolPart))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpInBuf = (UINT8 *)apVolPart;
    args.mInBufByteCount = sizeof(K2OSSTOR_VOLUME_PART);
    args.mMethodId = K2OS_StoreVol_Method_AddPart;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorVol, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL            
K2OS_Vol_RemovePartition(
    K2OSSTOR_VOLUME aStorVol, 
    K2OS_IFINST_ID  aPartitionIfInstId
)
{
    K2OS_RPC_CALLARGS       args;
    UINT32                  actualOut;
    K2STAT                  stat;
    K2OS_STORVOL_REMPART_IN remPartIn;

    if ((NULL == aStorVol) ||
        (0 == aPartitionIfInstId))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    remPartIn.mPartIfInstId = aPartitionIfInstId;

    args.mpInBuf = (UINT8 *)&remPartIn;
    args.mInBufByteCount = sizeof(K2OS_STORVOL_REMPART_IN);
    args.mMethodId = K2OS_StoreVol_Method_RemPart;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorVol, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL
K2OS_Vol_GetPartition(
    K2OSSTOR_VOLUME         aStorVol,
    UINT32                  aIxPart,
    K2OSSTOR_VOLUME_PART *  apRetPart
)
{
    K2OS_RPC_CALLARGS       args;
    UINT32                  actualOut;
    K2STAT                  stat;
    K2OS_STORVOL_GETPART_IN getPart;

    if ((NULL == aStorVol) ||
        (NULL == apRetPart))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpInBuf = (UINT8 const *)&getPart;
    args.mInBufByteCount = sizeof(K2OS_STORVOL_GETPART_IN);
    args.mpOutBuf = (UINT8 *)apRetPart;
    args.mOutBufByteCount = sizeof(K2OSSTOR_VOLUME_PART);
    args.mMethodId = K2OS_StoreVol_Method_GetPart;

    getPart.mIxPart = aIxPart;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorVol, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    if (sizeof(K2OSSTOR_VOLUME_PART) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return FALSE;
    }

    return TRUE;
}

BOOL
K2OS_Vol_Make(
    K2OSSTOR_VOLUME aStorVol
)
{
    K2OS_RPC_CALLARGS   args;
    UINT32              actualOut;
    K2STAT              stat;

    if (NULL == aStorVol)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mMethodId = K2OS_StoreVol_Method_Make;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorVol, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

K2STAT
K2OS_Vol_GetState(
    K2OSSTOR_VOLUME aStorVol,
    BOOL *          apRetState
)
{
    K2OS_RPC_CALLARGS           args;
    UINT32                      actualOut;
    K2STAT                      stat;
    K2OS_STORVOL_GETSTATE_OUT   getStateOut;

    if ((NULL == aStorVol) ||
        (NULL == apRetState))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mpOutBuf = (UINT8 *)&getStateOut;
    args.mOutBufByteCount = sizeof(K2OS_STORVOL_GETSTATE_OUT);
    args.mMethodId = K2OS_StoreVol_Method_GetState;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorVol, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    if (sizeof(K2OS_STORVOL_GETSTATE_OUT) != actualOut)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_SIZE);
        return FALSE;
    }

    *apRetState = getStateOut.mIsMade;

    return TRUE;
}

BOOL            
K2OS_Vol_Break(
    K2OSSTOR_VOLUME aStorVol
)
{
    K2OS_RPC_CALLARGS   args;
    UINT32              actualOut;
    K2STAT              stat;

    if (NULL == aStorVol)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    args.mMethodId = K2OS_StoreVol_Method_Break;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorVol, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

static
BOOL
Vol_Transfer(
    K2OSSTOR_VOLUME aStorVol,
    UINT64 const *  apBytesOffset,
    UINT32          aBufAddr,
    UINT32          aByteCount,
    BOOL            aIsWrite
)
{
    K2OS_RPC_CALLARGS           args;
    UINT32                      actualOut;
    K2STAT                      stat;
    K2OS_STORVOL_TRANSFER_IN    transIn;

    if ((NULL == aStorVol) ||
        (NULL == apBytesOffset) ||
        (0 == aBufAddr) ||
        (0 == aByteCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2MEM_Zero(&args, sizeof(args));

    transIn.mByteCount = aByteCount;
    transIn.mBytesOffset = *apBytesOffset;
    transIn.mMemAddr = aBufAddr;
    transIn.mIsWrite = aIsWrite;

    args.mpInBuf = (UINT8 *)&transIn;
    args.mInBufByteCount = sizeof(K2OS_STORVOL_TRANSFER_IN);
    args.mMethodId = K2OS_StoreVol_Method_Transfer;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aStorVol, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL            
K2OS_Vol_Read(
    K2OSSTOR_VOLUME aStorVol,
    UINT64 const *  apBytesOffset,
    void *          apBuffer,
    UINT32          aByteCount
)
{
    return Vol_Transfer(aStorVol, apBytesOffset, (UINT32)apBuffer, aByteCount, FALSE);
}

BOOL            
K2OS_Vol_Write(
    K2OSSTOR_VOLUME aStorVol,
    UINT64 const *  apBytesOffset,
    void const *    apBuffer,
    UINT32          aByteCount
)
{
    return Vol_Transfer(aStorVol, apBytesOffset, (UINT32)apBuffer, aByteCount, TRUE);
}

BOOL            
K2OS_Vol_Detach(
    K2OSSTOR_VOLUME aStorVol
)
{
    return K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)aStorVol);
}
