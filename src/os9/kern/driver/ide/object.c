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
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOofSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "ide.h"

K2STAT
IdeBlockIoDevice_Create(
    K2OS_RPC_OBJ                    aObj,
    K2OS_RPC_OBJECT_CREATE const *  apCreate,
    UINT32 *                        apRetContext,
    BOOL *                          apRetSingleUsage
)
{
    IDE_DEVICE * pDev;

    if (apCreate->mCreatorProcessId != K2OS_Process_GetId())
    {
        K2OSKERN_Debug("!!!IDE - Attempt to create nonlocal handle of device\n");
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pDev = (IDE_DEVICE *)apCreate->mCreatorContext;
    pDev->mRpcObj = aObj;
    *apRetContext = apCreate->mCreatorContext;
    *apRetSingleUsage = FALSE;

    return K2STAT_NO_ERROR;
}

K2STAT
IdeBlockIoDevice_Call(
    K2OS_RPC_OBJ                    aObj,
    UINT32                          aObjContext,
    K2OS_RPC_OBJECT_CALL const *    apCall,
    UINT32 *                        apRetUsedOutBytes
)
{
    IDE_DEVICE *    pDevice;
    K2STAT          stat;

    pDevice = (IDE_DEVICE *)aObjContext;
    K2_ASSERT(aObj == pDevice->mRpcObj);

    switch (apCall->Args.mMethodId)
    {
    case K2OS_BLOCKIO_METHOD_GET_MEDIA:
        if ((apCall->Args.mOutBufByteCount < sizeof(K2OS_STORAGE_MEDIA)) ||
            (apCall->Args.mInBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            K2OS_CritSec_Enter(&pDevice->Sec);
            if ((!pDevice->mIsRemovable) || (pDevice->mMediaPresent))
            {
                K2MEM_Copy(apCall->Args.mpOutBuf, &pDevice->Media, sizeof(K2OS_STORAGE_MEDIA));
                *apRetUsedOutBytes = sizeof(K2OS_STORAGE_MEDIA);
                stat = K2STAT_NO_ERROR;
            }
            else
            {
                stat = K2STAT_ERROR_NO_MEDIA;
            }
            K2OS_CritSec_Leave(&pDevice->Sec);
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    }

    return stat;
}

K2STAT
IdeBlockIoDevice_Delete(
    K2OS_RPC_OBJ    aObj,
    UINT32          aObjContext
)
{
    // this should never happen
    K2OS_RaiseException(K2STAT_EX_LOGIC);
    return K2STAT_NO_ERROR;
}
