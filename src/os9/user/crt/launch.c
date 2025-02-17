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

static K2OS_PROC_START_INFO sgLaunchInfo;

K2OS_PROC_START_INFO const * const gpLaunchInfo = &sgLaunchInfo;

static XDL *        sgpClientXdl;
static BOOL         sgFirstTime;
static UINT32       sgInitialStackAddr;
static UINT32       sgResizedStackAddr;

void CrtEntryPoint(void);

void
CrtLaunch(
    void
)
{
    UINT32 *pVal;

    sgInitialStackAddr = CrtMem_CreateStackSegment(K2OS_THREAD_DEFAULT_STACK_PAGES);
    K2_ASSERT(0 != sgInitialStackAddr);
    if (0 == sgInitialStackAddr)
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());

    sgResizedStackAddr = 0;
    sgFirstTime = TRUE;

    pVal = (UINT32 *)((sgInitialStackAddr + (K2OS_THREAD_DEFAULT_STACK_PAGES * K2_VA_MEMPAGE_BYTES)) - sizeof(UINT32));
    *pVal = 0;
    --pVal;
    *pVal = 0;

#if K2_TARGET_ARCH_IS_INTEL
    X32_ResetStackAndJump((UINT32)pVal, (UINT32)CrtEntryPoint);
#else
    A32_ResetStackAndJump((UINT32)pVal, (UINT32)CrtEntryPoint);
#endif
}

K2OS_pf_THREAD_ENTRY
CrtLaunch_Startup(
    void
)
{
    K2STAT                          stat;
    UINT32                          val32;
    UINT32 *                        pVal;
    UINT32                          result;
    K2OS_RPC_CALLARGS               args;
    K2OS_KERNELRPC_GETLAUNCHINFO_IN inParam;
    UINT32                          actualOut;

    if (sgFirstTime)
    {
        sgFirstTime = FALSE;

        inParam.TargetBufDesc.mAddress = (UINT32)&sgLaunchInfo;
        inParam.TargetBufDesc.mAttrib = 0;
        inParam.TargetBufDesc.mBytesLength = sizeof(sgLaunchInfo);

        args.mMethodId = K2OS_KernelRpc_Method_GetLaunchInfo;
        args.mpInBuf = (UINT8 const *)&inParam;
        args.mInBufByteCount = sizeof(inParam);
        args.mpOutBuf = NULL;
        args.mOutBufByteCount = 0;

        actualOut = 0;
        stat = K2OS_Rpc_Call(CrtRpc_Obj(), &args, &actualOut);
        if (K2STAT_IS_ERROR(stat))
        {
            CrtDbg_Printf("***Process %d failed to get its launch info\n", gProcessId);
            K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
        }

        val32 = 0;
        sgpClientXdl = NULL;
        stat = CrtXdl_Acquire(gpLaunchInfo->mPath, &sgpClientXdl, &val32, NULL);
//        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
        {
            CrtDbg_Printf("***Process %d failed to acquire its client xdl (0x%08X)\n", gProcessId, stat);
            K2OS_Process_Exit(stat);
        }

        if (0 == val32)
            val32 = K2OS_THREAD_DEFAULT_STACK_PAGES;
        else if (val32 > 128)
            val32 = 128;
        if (val32 != K2OS_THREAD_DEFAULT_STACK_PAGES)
        {
            //
            // switch to properly sized stack
            //
            sgResizedStackAddr = CrtMem_CreateStackSegment(val32);
            K2_ASSERT(0 != sgResizedStackAddr);
            if (0 == sgResizedStackAddr)
            {
                CrtDbg_Printf("***Process %d failed to adjust its default thread stack size\n", gProcessId);
                K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
            }

            pVal = (UINT32 *)((sgResizedStackAddr + (val32 * K2_VA_MEMPAGE_BYTES)) - sizeof(UINT32));
            *pVal = 0;
            --pVal;
            *pVal = 0;

#if K2_TARGET_ARCH_IS_INTEL
            X32_ResetStackAndJump((UINT32)pVal, (UINT32)CrtEntryPoint);
#else
            A32_ResetStackAndJump((UINT32)pVal, (UINT32)CrtEntryPoint);
#endif
        }
    }
    else
    {
        if (0 != sgResizedStackAddr)
        {
            //
            // we are on the 'resized' stack now. we can purge the initial one
            //
            CrtMem_FreeSegment(sgInitialStackAddr, FALSE);
            sgInitialStackAddr = sgResizedStackAddr;
        }
    }

    result = CrtKern_SysCall1(K2OS_SYSCALL_ID_PROCESS_START, sgInitialStackAddr);
    K2_ASSERT(0 != result);

    //
    // kernel will have taken ownership of stack segment so let this go
    //
    CrtMem_FreeSegment(sgInitialStackAddr, TRUE);

//    CrtDbg_Printf("Process %d \"%s\" starting (thread %d)\n", gProcessId, sgLaunchInfo.mPath, K2OS_Thread_GetId());

    stat = XDL_FindExport(sgpClientXdl, XDLProgData_Text, "MainThread", &val32);
    if (K2STAT_IS_ERROR(stat))
    {
        CrtDbg_Printf("***Process %d failed to get MainThread in client xdl\n", gProcessId);
        K2OS_Process_Exit(stat);
    }

    return (K2OS_pf_THREAD_ENTRY)val32;
}

void
CrtEntryPoint(
    void
)
{
    K2OS_Process_Exit((CrtLaunch_Startup())((void *)gpLaunchInfo->mArgStr));
}

