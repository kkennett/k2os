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
#include "crt32.h"

K2STAT
K2_CALLCONV_REGS
K2OS_Thread_GetLastStatus(
    void
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    return pThreadPage->mLastStatus;
}

K2STAT
K2_CALLCONV_REGS
K2OS_Thread_SetLastStatus(
    K2STAT aStatus
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT32                  result;

    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    result = pThreadPage->mLastStatus;
    pThreadPage->mLastStatus = aStatus;

    return result;
}

void
K2_CALLCONV_REGS
K2OS_Thread_Exit(
    UINT_PTR aExitCode
)
{
    CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_EXIT, aExitCode);
}

K2OS_THREAD_TOKEN  
K2OS_Thread_Create(
    K2OS_pf_THREAD_ENTRY            aEntryPoint,
    void *                          apArg,
    K2OS_USER_THREAD_CONFIG const * apConfig,
    UINT_PTR *                      apRetThreadId
)
{
    UINT32                  config;
    UINT32                  newThreadStackAddr;
    K2OS_THREAD_TOKEN       tokThread;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT32 *                pClear;

    if (NULL != apConfig)
    {
        config = apConfig->mStackPages;
        if (0 == config)
            config = K2OS_THREAD_DEFAULT_STACK_PAGES;
        else if (config > 128)
            config = 128;
        config |= ((UINT32)apConfig->mPriority) << 8;
        config |= ((UINT32)apConfig->mAffinityMask) << 16;
        config |= ((UINT32)apConfig->mQuantum) << 24;
    }
    else
    {
        config = K2OS_THREAD_DEFAULT_STACK_PAGES;
    }

    newThreadStackAddr = CrtMem_CreateStackSegment(config & 0xFF);
    if (0 == newThreadStackAddr)
        return NULL;

    pClear = (UINT32 *)(newThreadStackAddr + ((config & 0xFF) * K2_VA_MEMPAGE_BYTES) - sizeof(UINT32));
    *pClear = 0;
    pClear--;
    *pClear = 0;

    tokThread = (K2OS_THREAD_TOKEN)CrtKern_SysCall5(K2OS_SYSCALL_ID_THREAD_CREATE, (UINT_PTR)CrtThread_EntryPoint, (UINT_PTR)aEntryPoint, (UINT_PTR)apArg, newThreadStackAddr, config);
    if (NULL == tokThread)
    {
        CrtMem_FreeSegment(newThreadStackAddr, FALSE);
        return NULL;
    }
    //
    // thread starts to run immediately  (i.e. its already running here)
    //

    //
    // kernel will own thread's stack now so we can do this
    //
    CrtMem_FreeSegment(newThreadStackAddr, TRUE);

    if (NULL != apRetThreadId)
    {
        pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
        if (NULL != apRetThreadId)
            *apRetThreadId = pThreadPage->mSysCall_Arg7_Result0;
    }

    return tokThread;
}

UINT_PTR
K2_CALLCONV_REGS
K2OS_Thread_GetId(
    void
)
{
    return CRT_GET_CURRENT_THREAD_INDEX;
}

void 
K2_CALLCONV_REGS 
CrtThread_EntryPoint(
    K2OS_pf_THREAD_ENTRY    aUserEntry,
    void *                  apArgument
)
{
    K2OS_Thread_Exit(aUserEntry(apArgument));
}

UINT_PTR
K2_CALLCONV_REGS
K2OS_Thread_GetCpuCoreAffinityMask(
    void
)
{
    return CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_SETAFF, (UINT_PTR)-1);
}

UINT_PTR
K2_CALLCONV_REGS
K2OS_Thread_SetCpuCoreAffinityMask(
    UINT_PTR aNewAffinity
)
{
    return CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_SETAFF, aNewAffinity);
}

BOOL
K2_CALLCONV_REGS
K2OS_Thread_Sleep(
    UINT_PTR aTimeoutMs
)
{
    if (K2OS_TIMEOUT_INFINITE == aTimeoutMs)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (K2STAT_ERROR_TIMEOUT == K2OS_Wait_Many(0, NULL, FALSE, aTimeoutMs))
        return TRUE;

    return FALSE;
}

void
K2_CALLCONV_REGS
K2OS_Thread_StallMicroseconds(
    UINT32 aMicroseconds
)
{
    UINT32 work;
    UINT32 last;
    UINT32 v;

    while (aMicroseconds > 1000000)
    {
        K2OS_Thread_StallMicroseconds(1000000);
        aMicroseconds -= 1000000;
    }

    if (0 == aMicroseconds)
        return;

    work = (UINT32)((((UINT64)K2OS_System_GetHfFreq()) * ((UINT64)aMicroseconds)) / 1000000ull);

    if (0 == work)
        return;

    last = K2OS_System_GetMsTick32();
    do {
        v = K2OS_System_GetMsTick32();
        last = v - last;
        if (work <= last)
            break;
        work -= last;
        last = v;
    } while (1);
}

UINT_PTR    
K2_CALLCONV_REGS 
K2OS_Thread_GetExitCode(
    K2OS_THREAD_TOKEN aThreadToken
)
{
    return CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_GETEXITCODE, (UINT_PTR)-1);
}
