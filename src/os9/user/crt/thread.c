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

K2STAT
K2OS_Thread_GetLastStatus(
    void
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    return pThreadPage->mLastStatus;
}

K2STAT
K2OS_Thread_SetLastStatus(
    K2STAT aStatus
)
{
    K2OS_THREAD_PAGE * pThreadPage;
    UINT32                  result;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    result = pThreadPage->mLastStatus;
    pThreadPage->mLastStatus = aStatus;

    return result;
}

void
K2OS_Thread_Exit(
    UINT32 aExitCode
)
{
    CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_EXIT, aExitCode);
}

K2OS_THREAD_TOKEN  
K2OS_Thread_Create(
    char const *                    apName,
    K2OS_pf_THREAD_ENTRY            aEntryPoint,
    void *                          apArg,
    K2OS_THREAD_CONFIG const * apConfig,
    UINT32 *                        apRetThreadId
)
{
    UINT32                  config;
    UINT32                  newThreadStackAddr;
    K2OS_THREAD_TOKEN       tokThread;
    K2OS_THREAD_PAGE * pThreadPage;
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

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    if (NULL != apName)
    {
        K2ASC_CopyLen((char *)pThreadPage->mMiscBuffer, apName, K2OS_THREAD_NAME_BUFFER_CHARS);
        pThreadPage->mMiscBuffer[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;
    }
    else
    {
        pThreadPage->mMiscBuffer[0] = 0;
    }

    tokThread = (K2OS_THREAD_TOKEN)CrtKern_SysCall5(K2OS_SYSCALL_ID_THREAD_CREATE, (UINT32)CrtThread_EntryPoint, (UINT32)aEntryPoint, (UINT32)apArg, newThreadStackAddr, config);
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
        if (NULL != apRetThreadId)
            *apRetThreadId = pThreadPage->mSysCall_Arg7_Result0;
    }

    return tokThread;
}

BOOL                
K2OS_Thread_SetName(
    char const *apName
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    if (NULL != apName)
    {
        K2ASC_CopyLen((char *)pThreadPage->mMiscBuffer, apName, K2OS_THREAD_NAME_BUFFER_CHARS);
        pThreadPage->mMiscBuffer[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;
    }
    else
    {
        pThreadPage->mMiscBuffer[0] = 0;
    }

    return (BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_SET_NAME, 0);
}

void        
K2OS_Thread_GetName(
    char *apRetNameBuffer
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    if (NULL == apRetNameBuffer)
        return;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_GET_NAME, 0);

    K2ASC_CopyLen(apRetNameBuffer, (char *)pThreadPage->mMiscBuffer, K2OS_THREAD_NAME_BUFFER_CHARS);
}

UINT32
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

UINT32
K2OS_Thread_GetCpuCoreAffinityMask(
    void
)
{
    return CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_SETAFF, (UINT32)-1);
}

UINT32
K2OS_Thread_SetCpuCoreAffinityMask(
    UINT32 aNewAffinity
)
{
    return CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_SETAFF, aNewAffinity);
}

BOOL
K2OS_Thread_Sleep(
    UINT32 aTimeoutMs
)
{
    K2OS_WaitResult waitResult;

    if (K2OS_TIMEOUT_INFINITE == aTimeoutMs)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (K2OS_Thread_WaitOne(&waitResult, NULL, aTimeoutMs))
    {
        return FALSE;
    }

    if (K2STAT_ERROR_TIMEOUT != K2OS_Thread_GetLastStatus())
    {
        return FALSE;
    }

    K2_ASSERT(waitResult == K2OS_Wait_TimedOut);

    return TRUE;
}

void
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

UINT32    
K2OS_Thread_GetExitCode(
    K2OS_THREAD_TOKEN aThreadToken
)
{
    return CrtKern_SysCall1(K2OS_SYSCALL_ID_THREAD_GETEXITCODE, (UINT32)-1);
}
