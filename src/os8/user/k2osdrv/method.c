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
K2OSDRV_RunAcpiMethod(
    char const *apMethodName,
    UINT_PTR    aFlags,
    UINT_PTR    aIn,
    UINT_PTR *  apOut
)
{
    UINT_PTR                actualOut;
    K2STAT                  stat;
    K2OSRPC_CALLARGS        args;
    K2OSDRV_RUN_ACPI_IN     argsIn;
    K2OSDRV_RUN_ACPI_OUT    argsOut;
    char                    method[4];

    K2_ASSERT(NULL != apOut);
    *apOut = 0;

    if ((NULL == apMethodName) ||
        (0 == (*apMethodName)))
        return K2STAT_ERROR_BAD_ARGUMENT;

    method[0] = apMethodName[0];
    method[1] = apMethodName[1];
    if (0 != method[1])
    {
        method[2] = apMethodName[2];
        if (0 != method[2])
        {
            method[3] = apMethodName[3];
        }
        else
        {
            method[3] = 0;
        }
    }
    else
    {
        method[2] = 0;
        method[3] = 0;
    }
    K2MEM_Copy(&argsIn.mMethod, method, sizeof(UINT32));
    argsIn.mFlags = aFlags;
    argsIn.mIn = aIn;

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_RUN_ACPI;
    args.mpInBuf = (UINT8 const *)&argsIn;
    args.mInBufByteCount = sizeof(argsIn);
    args.mpOutBuf = (UINT8 *)&argsOut;
    args.mOutBufByteCount = sizeof(argsOut);
    stat = K2OSRPC_Object_Call(gHostUsage, &args, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    if (actualOut != sizeof(K2OSDRV_RUN_ACPI_OUT))
    {
        return K2STAT_ERROR_BAD_SIZE;
    }

    *apOut = argsOut.mResult;

    return K2STAT_NO_ERROR;
}



