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
    K2_DEVICE_IDENT const * apIdent
)
{
    K2OSDRV_MOUNT_CHILD_IN  mountChildIn;
    K2OSRPC_CALLARGS        args;
    
    K2MEM_Zero(&mountChildIn, sizeof(mountChildIn));
    mountChildIn.mChildObjectId = aChildObjectId;
    mountChildIn.mMountFlags = aMountFlags;
    mountChildIn.mBusSpecificAddress = *apBusSpecificAddress;
    K2MEM_Copy(&mountChildIn.Ident, apIdent, sizeof(K2_DEVICE_IDENT));

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_MOUNT_CHILD;
    args.mpInBuf = (UINT8 const *)&mountChildIn;
    args.mInBufByteCount = sizeof(mountChildIn);
    args.mpOutBuf = NULL;
    args.mOutBufByteCount = 0;
    return K2OSRPC_Object_Call(gHostUsage, &args, NULL);
}

