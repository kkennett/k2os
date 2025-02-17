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

K2STAT 
FATFS_RpcObj_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    FATPROV *pProv;

    if (0 != apCreate->mCreatorProcessId)
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pProv = (FATPROV *)apCreate->mCreatorContext;

    if (pProv != &gFatProv)
        return K2STAT_ERROR_NOT_ALLOWED;

    gFatProv.mRpcObj = aObject;

    *apRetContext = (UINT32)&gFatProv;

    return K2STAT_NO_ERROR;
}

K2STAT 
FATFS_RpcObj_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    if (0 != aProcessId)
        return K2STAT_ERROR_NOT_ALLOWED;
    return K2STAT_NO_ERROR;
}

K2STAT 
FATFS_RpcObj_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    return K2STAT_NO_ERROR;
}

K2STAT 
FATFS_RpcObj_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    K2STAT  stat;
    UINT32  u32;

    stat = K2STAT_ERROR_UNKNOWN;

    switch (apCall->Args.mMethodId)
    {
    case K2OS_FsProv_Method_GetCount:
        if ((apCall->Args.mInBufByteCount > 0) ||
            (apCall->Args.mOutBufByteCount < sizeof(UINT32)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            u32 = FATFS_NUM_PROV;
            K2MEM_Copy(apCall->Args.mpOutBuf, &u32, sizeof(u32));
            *apRetUsedOutBytes = sizeof(UINT32);
            stat = K2STAT_NO_ERROR;
        }
        break;

    case K2OS_FsProv_Method_GetEntry:
        if ((apCall->Args.mInBufByteCount < sizeof(UINT32)) ||
            (apCall->Args.mOutBufByteCount < sizeof(void *)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            K2MEM_Copy(&u32, apCall->Args.mpInBuf, sizeof(UINT32));
            if (u32 >= FATFS_NUM_PROV)
            {
                stat = K2STAT_ERROR_OUT_OF_BOUNDS;
            }
            else
            {
                u32 = (UINT32)&gFatProv.KernFsProv[u32];
                K2MEM_Copy(apCall->Args.mpOutBuf, &u32, sizeof(void *));
                *apRetUsedOutBytes = sizeof(void *);
                stat = K2STAT_NO_ERROR;
            }
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    };

    return stat;
}

K2STAT 
FATFS_RpcObj_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    K2_ASSERT(aObjContext == (UINT32)&gFatProv);
    return K2STAT_NO_ERROR;
}

