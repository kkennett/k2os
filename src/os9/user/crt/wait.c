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
#include "crtuser.h"

BOOL
K2OS_Thread_WaitMany(
    K2OS_WaitResult *           apRetResult,
    UINT32                      aCount,
    K2OS_WAITABLE_TOKEN const * apWaitableTokens,
    BOOL                        aWaitAll,
    UINT32                      aTimeoutMs
)
{
    UINT32              threadIx;
    K2OS_THREAD_PAGE *	pThreadPage;
    UINT32              ix;
    BOOL                success;

    if ((NULL == apRetResult) || (aCount > K2OS_THREAD_WAIT_MAX_ITEMS))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        if (NULL != apRetResult)
        {
            *apRetResult = K2OS_Wait_TooManyTokens;
        }
        return FALSE;
    }

    threadIx = CRT_GET_CURRENT_THREAD_INDEX;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));

    if (aCount > 0)
    {
        for (ix = 0; ix < aCount; ix++)
        {
            if (NULL == apWaitableTokens[ix])
            {
                K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
                *apRetResult = K2OS_Wait_Failed_0 + ix;
                return FALSE;
            }
            pThreadPage->mWaitToken[ix] = apWaitableTokens[ix];
        }
    }
    else
    {
        if (K2OS_TIMEOUT_INFINITE == aTimeoutMs)
        {
            *apRetResult = K2OS_Wait_Failed_SleepForever;
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }
        if (0 == aTimeoutMs)
        {
            *apRetResult = K2OS_Wait_TimedOut;
            return FALSE;
        }
    }

    success = CrtKern_SysCall3(K2OS_SYSCALL_ID_THREAD_WAIT, aCount, aWaitAll, aTimeoutMs);

    *apRetResult = (K2OS_WaitResult)pThreadPage->mSysCall_Arg7_Result0;

    return success;
}

BOOL 
K2OS_Thread_WaitOne(
    K2OS_WaitResult *   apRetResult, 
    K2OS_WAITABLE_TOKEN aToken, 
    UINT32              aTimeoutMs
)
{
    return K2OS_Thread_WaitMany(apRetResult, (NULL == aToken) ? 0 : 1, (NULL == aToken) ? NULL : &aToken, FALSE, aTimeoutMs);
}
