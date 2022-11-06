//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
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

#include "ik2osdrv.h"

K2STAT
K2OSDrv_MountChildDevice(
    UINT_PTR                aChildObjectId,
    UINT_PTR                aMountFlags,
    UINT64 const  *         apBusSpecificAddress,
    K2_DEVICE_IDENT const * apIdent,
    UINT_PTR *              apRetSystemDeviceInstanceId
)
{
    K2OSDRV_MOUNT_CHILD_IN  mountChildIn;
    K2OSDRV_MOUNT_CHILD_OUT mountChildOut;
    K2OSRPC_CALLARGS        args;
    K2STAT                  stat;
    
    K2MEM_Zero(&mountChildIn, sizeof(mountChildIn));
    mountChildIn.mChildObjectId = aChildObjectId;
    mountChildIn.mMountFlags = aMountFlags;
    mountChildIn.mBusSpecificAddress = *apBusSpecificAddress;
    K2MEM_Copy(&mountChildIn.Ident, apIdent, sizeof(K2_DEVICE_IDENT));

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_MOUNT_CHILD;
    args.mpInBuf = (UINT8 const *)&mountChildIn;
    args.mInBufByteCount = sizeof(mountChildIn);
    args.mpOutBuf = (UINT8 *)&mountChildOut;
    args.mOutBufByteCount = sizeof(mountChildOut);
    
    stat = K2OSRPC_Object_Call(gHostUsage, &args, NULL);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    *apRetSystemDeviceInstanceId = mountChildOut.mSystemDeviceInstanceId;

    return stat;
}

K2STAT
K2OSDRV_AddChildRes(
    K2OSDRV_ADD_CHILD_RES_IN const *    apIn
)
{
    K2OSRPC_CALLARGS args;

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_ADD_CHILD_RES;
    args.mpInBuf = (UINT8 const *)apIn;
    args.mInBufByteCount = sizeof(K2OSDRV_ADD_CHILD_RES_IN);
    args.mpOutBuf = NULL;
    args.mOutBufByteCount = 0;

    return K2OSRPC_Object_Call(gHostUsage, &args, NULL);
}

K2STAT
K2OSDRV_AddChildPhys(
    UINT_PTR        aChildObjectId,
    UINT64 const *  apPhysBase,
    UINT64 const *  apSizeBytes
)
{
    K2OSDRV_ADD_CHILD_RES_IN    addResIn;

    if ((NULL == apPhysBase) ||
        (0 != ((*apPhysBase) & 0xFFF)) ||
        (NULL == apSizeBytes) ||
        (0 == (*apSizeBytes)) ||
        (0 != ((*apSizeBytes) & 0xFFF)))
        return K2STAT_ERROR_BAD_ARGUMENT;

    K2MEM_Zero(&addResIn, sizeof(addResIn));
    addResIn.mChildObjectId = aChildObjectId;
    addResIn.mResType = K2OSDRV_RESTYPE_PHYS;
    addResIn.Desc.PhysRange.mBaseAddr = (UINT_PTR)*apPhysBase;
    addResIn.Desc.PhysRange.mSizeBytes = (UINT_PTR)*apSizeBytes;

    return K2OSDRV_AddChildRes(&addResIn);
}

K2STAT
K2OSDRV_AddChildIo(
    UINT_PTR        aChildObjectId,
    UINT_PTR        aIoBase,
    UINT_PTR        aIoSizeBytes
)
{
    K2OSDRV_ADD_CHILD_RES_IN    addResIn;

    if ((0 == aIoSizeBytes) ||
        (aIoBase > 0xFFFF) ||
        ((0x10000 - aIoBase) < aIoSizeBytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

    K2MEM_Zero(&addResIn, sizeof(addResIn));
    addResIn.mChildObjectId = aChildObjectId;
    addResIn.mResType = K2OSDRV_RESTYPE_IO;
    addResIn.Desc.IoPortRange.mBasePort = (UINT16)(aIoBase & 0xFFFF);
    addResIn.Desc.IoPortRange.mSizeBytes = (UINT16)(aIoSizeBytes & 0xFFFF);

    return K2OSDRV_AddChildRes(&addResIn);
}

K2STAT
K2OSDRV_AddChildIrq(
    UINT_PTR                aChildObjectId,
    K2OS_IRQ_CONFIG const * apIrqConfig
)
{
    K2OSDRV_ADD_CHILD_RES_IN    addResIn;

    if (NULL == apIrqConfig)
        return K2STAT_ERROR_BAD_ARGUMENT;

    K2MEM_Zero(&addResIn, sizeof(addResIn));
    addResIn.mChildObjectId = aChildObjectId;
    addResIn.mResType = K2OSDRV_RESTYPE_IRQ;
    K2MEM_Copy(&addResIn.Desc.IrqConfig, apIrqConfig, sizeof(K2OS_IRQ_CONFIG));

    return K2OSDRV_AddChildRes(&addResIn);
}

