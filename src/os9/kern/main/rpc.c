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

#include "kern.h"

K2STAT 
KernRpc_CreateObj(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    *apRetContext = apCreate->mCreatorProcessId;
    return K2STAT_NO_ERROR;
}

K2STAT 
KernRpc_Attach(
    K2OS_RPC_OBJ    aObject, 
    UINT32          aObjContext, 
    UINT32          aProcessId, 
    UINT32 *        apRetUseContext
)
{
    if (aProcessId != aObjContext)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    *apRetUseContext = aProcessId;

    return K2STAT_NO_ERROR;
}

K2STAT 
KernRpc_Detach(
    K2OS_RPC_OBJ    aObject, 
    UINT32          aObjContext, 
    UINT32          aUseContext
)
{
    K2_ASSERT(aObjContext == aUseContext);
    return K2STAT_NO_ERROR;
}

K2STAT
KernRpc_GetLaunchInfo(
    UINT32                                  aProcessId,
    K2OS_KERNELRPC_GETLAUNCHINFO_IN const * apIn
)
{
    K2OSKERN_OBJREF         refProc;
    K2OSKERN_MAPUSER        mapUser;
    K2STAT                  stat;
    K2OS_PROC_START_INFO *  pStartInfo;

    if ((0 != (apIn->TargetBufDesc.mAttrib & K2OS_BUFDESC_ATTRIB_READONLY)) ||
        (apIn->TargetBufDesc.mBytesLength < sizeof(K2OS_PROC_START_INFO)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    refProc.AsAny = NULL;
    stat = KernProc_FindAddRefById(aProcessId, &refProc);
    if (!K2STAT_IS_ERROR(stat))
    {
        mapUser = KernMapUser_Create(aProcessId, &apIn->TargetBufDesc, (UINT32 *)&pStartInfo);
        if (NULL == mapUser)
        {
            stat = K2OS_Thread_GetLastStatus();
        }
        else
        {
            if (1 == refProc.AsProc->mId)
            {
                //
                // hard coded that proc 1 is SysProct
                //
                K2ASC_Copy(pStartInfo->mPath, "user\\sysproc.xdl");
                pStartInfo->mArgStr[0] = 0;
            }
            else
            {
                //
                // copy in launch info from process that started this process
                //
                K2_ASSERT(NULL != refProc.AsProc->mpLaunchInfo);
                K2MEM_Copy(pStartInfo, refProc.AsProc->mpLaunchInfo, sizeof(K2OS_PROC_START_INFO));
            }

            KernMapUser_Destroy(mapUser);
        }

        KernObj_ReleaseRef(&refProc);
    }

    return stat;
}

K2STAT
KernRpc_IfEnumNext(
    UINT32                              aProcessId,
    K2OS_KERNELRPC_IFENUMNEXT_IN const *apIn,
    K2OS_KERNELRPC_IFENUMNEXT_OUT *     apOut
)
{
    K2OSKERN_OBJREF         refProc;
    K2OSKERN_OBJREF         refEnum;
    K2STAT                  stat;
    K2OSKERN_MAPUSER        mapUser;
    K2OS_IFINST_DETAIL *    pBuf;
    UINT32                  ioCount;

    ioCount = apIn->mCount;
    if ((0 == ioCount) ||
        ((ioCount * sizeof(K2OS_IFINST_DETAIL)) > apIn->TargetBufDesc.mBytesLength) ||
        (0 != (apIn->TargetBufDesc.mAttrib & K2OS_BUFDESC_ATTRIB_READONLY)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    refProc.AsAny = NULL;
    stat = KernProc_FindAddRefById(aProcessId, &refProc);
    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    if (KernObj_Process != refProc.AsAny->mObjType)
    {
        // bad param from crt
        stat = K2STAT_ERROR_CORRUPTED;
    }
    else
    {
        refEnum.AsAny = NULL;
        stat = KernProc_TokenTranslate(refProc.AsProc, apIn->mTokEnum, &refEnum);
        if (!K2STAT_IS_ERROR(stat))
        {
            if (KernObj_IfEnum != refEnum.AsAny->mObjType)
            {
                // bad token from user
                stat = K2STAT_ERROR_BAD_TOKEN;
            }
            else
            {
                mapUser = KernMapUser_Create(aProcessId, &apIn->TargetBufDesc, (UINT32 *)&pBuf);
                if (NULL != mapUser)
                {
                    stat = KernIfEnum_Next(refEnum.AsIfEnum, pBuf, &ioCount);
                    if (!K2STAT_IS_ERROR(stat))
                    {
                        K2_ASSERT(ioCount > 0);
                        K2_ASSERT(ioCount <= apIn->mCount);
                        apOut->mCountGot = ioCount;
                    }

                    KernMapUser_Destroy(mapUser);
                }
                else
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                }
            }

            KernObj_ReleaseRef(&refEnum);
        }
    }

    KernObj_ReleaseRef(&refProc);

    return stat;
}

K2STAT 
KernRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall, 
    UINT32 *                    apRetUsedOutBytes
)
{
    K2STAT stat;

    switch (apCall->Args.mMethodId)
    {
    case K2OS_KernelRpc_Method_IfEnumNext:
        if ((apCall->Args.mInBufByteCount != sizeof(K2OS_KERNELRPC_IFENUMNEXT_IN)) ||
            (apCall->Args.mOutBufByteCount != sizeof(K2OS_KERNELRPC_IFENUMNEXT_OUT)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = KernRpc_IfEnumNext(
                apCall->mUseContext,
                (K2OS_KERNELRPC_IFENUMNEXT_IN const *)apCall->Args.mpInBuf,
                (K2OS_KERNELRPC_IFENUMNEXT_OUT *)apCall->Args.mpOutBuf
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetUsedOutBytes = sizeof(K2OS_KERNELRPC_IFENUMNEXT_OUT);
            }
        }
        break;

    case K2OS_KernelRpc_Method_GetLaunchInfo:
        if ((apCall->Args.mInBufByteCount != sizeof(K2OS_KERNELRPC_GETLAUNCHINFO_IN)) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = KernRpc_GetLaunchInfo(apCall->mUseContext, (K2OS_KERNELRPC_GETLAUNCHINFO_IN const *)apCall->Args.mpInBuf);
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    }

    return stat;
}

K2STAT 
KernRpc_DeleteObj(
    K2OS_RPC_OBJ    aObject, 
    UINT32          aObjContext
)
{
    K2OSKERN_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    return K2STAT_NO_ERROR;
}


