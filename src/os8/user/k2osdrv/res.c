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
K2OSDrv_EnumRes(
    K2OSDRV_ENUM_RES_OUT * apEnum
)
{
    UINT_PTR            actualOut;
    K2STAT              stat;
    K2OSRPC_CALLARGS    args;

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_ENUM_RES;
    args.mpInBuf = NULL;
    args.mInBufByteCount = 0; 
    args.mpOutBuf = (UINT8 *)apEnum;
    args.mOutBufByteCount = sizeof(K2OSDRV_ENUM_RES_OUT);
    stat = K2OSRPC_Object_Call(gHostUsage, &args, &actualOut);
    
    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    if (actualOut != sizeof(K2OSDRV_ENUM_RES_OUT))
    {
        return K2STAT_ERROR_BAD_SIZE;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSDrv_GetAcpiInfo(
    char *      apOutBuf,
    UINT_PTR    aOutBufBytes
)
{
    UINT_PTR            actualOut;
    K2STAT              stat;
    K2OSDRV_GET_RES_IN  getRes;
    K2OSRPC_CALLARGS    args;

    K2MEM_Zero(&getRes, sizeof(getRes));
    getRes.mResType = K2OSDRV_RESTYPE_ACPI;
    getRes.mIndex = 0;

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_GET_RES;
    args.mpInBuf = (UINT8 *)&getRes;
    args.mInBufByteCount = sizeof(getRes);
    args.mpOutBuf = (UINT8 *)apOutBuf;
    args.mOutBufByteCount = aOutBufBytes;
    stat = K2OSRPC_Object_Call(gHostUsage, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    if (actualOut < aOutBufBytes)
    {
        apOutBuf[actualOut] = 0;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSDrv_GetResIo(
    UINT_PTR            aIndex,
    K2OSDRV_RES_IO *    apRetResIo
)
{
    UINT_PTR            actualOut;
    K2STAT              stat;
    K2OSDRV_GET_RES_IN  getRes;
    K2OSRPC_CALLARGS    args;

    K2MEM_Zero(&getRes, sizeof(getRes));
    getRes.mResType = K2OSDRV_RESTYPE_IO;
    getRes.mIndex = aIndex;

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_GET_RES;
    args.mpInBuf = (UINT8 *)&getRes;
    args.mInBufByteCount = sizeof(getRes);
    args.mpOutBuf = (UINT8 *)apRetResIo;
    args.mOutBufByteCount = sizeof(K2OSDRV_RES_IO);
    stat = K2OSRPC_Object_Call(gHostUsage, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    if (actualOut != sizeof(K2OSDRV_RES_IO))
    {
        return K2STAT_ERROR_BAD_SIZE;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSDrv_GetResPhys(
    UINT_PTR            aIndex,
    K2OSDRV_RES_PHYS *  apRetResPhys
)
{
    UINT_PTR            actualOut;
    K2STAT              stat;
    K2OSDRV_GET_RES_IN  getRes;
    K2OSRPC_CALLARGS    args;

    K2MEM_Zero(&getRes, sizeof(getRes));
    getRes.mResType = K2OSDRV_RESTYPE_PHYS;
    getRes.mIndex = aIndex;

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_GET_RES;
    args.mpInBuf = (UINT8 *)&getRes;
    args.mInBufByteCount = sizeof(getRes);
    args.mpOutBuf = (UINT8 *)apRetResPhys;
    args.mOutBufByteCount = sizeof(K2OSDRV_RES_PHYS);
    stat = K2OSRPC_Object_Call(gHostUsage, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    if (actualOut != sizeof(K2OSDRV_RES_PHYS))
    {
        return K2STAT_ERROR_BAD_SIZE;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSDrv_GetResIrq(
    UINT_PTR            aIndex,
    K2OSDRV_RES_IRQ *   apRetResIrq
)
{
    UINT_PTR            actualOut;
    K2STAT              stat;
    K2OSDRV_GET_RES_IN  getRes;
    K2OSRPC_CALLARGS    args;

    K2MEM_Zero(&getRes, sizeof(getRes));
    getRes.mResType = K2OSDRV_RESTYPE_IRQ;
    getRes.mIndex = aIndex;

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_GET_RES;
    args.mpInBuf = (UINT8 *)&getRes;
    args.mInBufByteCount = sizeof(getRes);
    args.mpOutBuf = (UINT8 *)apRetResIrq;
    args.mOutBufByteCount = sizeof(K2OSDRV_RES_IRQ);
    stat = K2OSRPC_Object_Call(gHostUsage, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    if (actualOut != sizeof(K2OSDRV_RES_IRQ))
    {
        return K2STAT_ERROR_BAD_SIZE;
    }

    return K2STAT_NO_ERROR;
}

