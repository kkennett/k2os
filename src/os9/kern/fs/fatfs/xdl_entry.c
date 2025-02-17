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

#include "fatfs.h"

FATPROV gFatProv;

static K2OS_RPC_OBJ_CLASSDEF const  sgClassDef =
{
    FATPROV_OBJ_CLASS_GUID,
    FATFS_RpcObj_Create,
    FATFS_RpcObj_OnAttach,
    FATFS_RpcObj_OnDetach,
    FATFS_RpcObj_Call,
    FATFS_RpcObj_Delete
};
static K2_GUID128 sFsProvIfId = K2OS_IFACE_FSPROV;

K2STAT
OnLoad(
    void
)
{
    K2STAT stat;

    K2MEM_Zero(&gFatProv, sizeof(gFatProv));

    K2ASC_Copy(gFatProv.KernFsProv[0].mName, "FAT32");
    gFatProv.KernFsProv[0].mFlags = K2OSKERN_FSPROV_FLAG_USE_VOLUME_IFINSTID;
    gFatProv.KernFsProv[0].Ops.Probe = FAT32_Probe;
    gFatProv.KernFsProv[0].Ops.Attach = FAT32_Attach;

    K2ASC_Copy(gFatProv.KernFsProv[1].mName, "FAT16");
    gFatProv.KernFsProv[1].mFlags = K2OSKERN_FSPROV_FLAG_USE_VOLUME_IFINSTID;
    gFatProv.KernFsProv[1].Ops.Probe = FAT16_Probe;
    gFatProv.KernFsProv[1].Ops.Attach = FAT16_Attach;

    K2ASC_Copy(gFatProv.KernFsProv[2].mName, "FAT12");
    gFatProv.KernFsProv[2].mFlags = K2OSKERN_FSPROV_FLAG_USE_VOLUME_IFINSTID;
    gFatProv.KernFsProv[2].Ops.Probe = FAT12_Probe;
    gFatProv.KernFsProv[2].Ops.Attach = FAT12_Attach;

    if (!K2OS_CritSec_Init(&gFatProv.Sec))
    {
        return K2OS_Thread_GetLastStatus();
    }

    stat = K2STAT_NO_ERROR;
    K2LIST_Init(&gFatProv.FsList);

    gFatProv.mRpcClass = K2OS_RpcServer_Register(&sgClassDef, 0);
    if (NULL == gFatProv.mRpcClass)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
    }
    else
    {
        gFatProv.mRpcObjHandle = K2OS_Rpc_CreateObj(0, &sgClassDef.ClassId, (UINT32)&gFatProv);
        if (NULL == gFatProv.mRpcObjHandle)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
        else
        {
            gFatProv.mRpcIfInst = K2OS_RpcObj_AddIfInst(
                gFatProv.mRpcObj,
                K2OS_IFACE_CLASSCODE_FSPROV,
                &sFsProvIfId,
                &gFatProv.mIfInstId,
                TRUE
            );
            if (NULL == gFatProv.mRpcIfInst)
            {
                K2OS_Rpc_Release(gFatProv.mRpcObjHandle);
            }
        }

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_RpcServer_Deregister(gFatProv.mRpcClass);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_CritSec_Done(&gFatProv.Sec);
    }

    return stat;
}

K2STAT
OnUnload(
    void
)
{
    K2_ASSERT(0 == gFatProv.FsList.mNodeCount);

    K2OS_RpcObj_RemoveIfInst(gFatProv.mRpcObj, gFatProv.mRpcIfInst);
    K2OS_Rpc_Release(gFatProv.mRpcObjHandle);
    K2OS_RpcServer_Deregister(gFatProv.mRpcClass);

    K2OS_CritSec_Done(&gFatProv.Sec);

    return K2STAT_NO_ERROR;
}

K2STAT
K2_CALLCONV_REGS
xdl_entry(
    XDL *   apXdl,
    UINT32  aReason
)
{
    if (aReason == XDL_ENTRY_REASON_LOAD)
    {
        return OnLoad();
    }

    if (aReason == XDL_ENTRY_REASON_UNLOAD)
    {
        return OnUnload();
    }

    return K2STAT_ERROR_NOT_SUPPORTED;
}

