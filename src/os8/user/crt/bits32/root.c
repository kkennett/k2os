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

K2OS_PROCESS_TOKEN  
K2_CALLCONV_REGS
K2OS_Root_Launch(
    char const *    apPath,
    char const *    apArgs,
    UINT_PTR *      apRetId
)
{
    CRT_LAUNCH_INFO *       pLaunchInfo;
    K2OS_TOKEN              tokResult;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    if (gProcessId != 1)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ALLOWED);
        return NULL;
    }

    pLaunchInfo = (CRT_LAUNCH_INFO *)K2OS_Heap_Alloc(sizeof(CRT_LAUNCH_INFO));
    if (NULL == pLaunchInfo)
    {
        return NULL;
    }

    K2MEM_Zero(pLaunchInfo, sizeof(CRT_LAUNCH_INFO));
    if (NULL != apPath)
    {
        K2ASC_CopyLen(pLaunchInfo->mPath, apPath, 1024);
        pLaunchInfo->mPath[1023] = 0;
    }
    if (NULL != apArgs)
    {
        K2ASC_CopyLen(pLaunchInfo->mArgStr, apArgs, 1024);
        pLaunchInfo->mArgStr[1023] = 0;
    }

    tokResult = (K2OS_PROCESS_TOKEN)CrtKern_SysCall1(K2OS_SYSCALL_ID_ROOT_PROCESS_LAUNCH, (UINT32)pLaunchInfo);

    K2OS_Heap_Free(pLaunchInfo);

    if (NULL != tokResult)
    {
        if (NULL != apRetId)
        {
            pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
            *apRetId = pThreadPage->mSysCall_Arg7_Result0;
        }
    }

    return tokResult;
}

BOOL
K2OS_Root_AddIoRange(
    K2OS_PROCESS_TOKEN  aTokProcess, 
    UINT32              aIoBase,
    UINT32              aPortCount
)
{
    if (gProcessId != 1)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ALLOWED);
        return FALSE;
    }

    return (BOOL)CrtKern_SysCall3(K2OS_SYSCALL_ID_ROOT_IOPERMIT_ADD, (UINT32)aTokProcess, aIoBase, aPortCount);
}

K2OS_PAGEARRAY_TOKEN  
K2_CALLCONV_REGS
K2OS_Root_CreatePageArrayAt(
    UINT32  aPhysAddr,
    UINT32  aPageCount
)
{
    if (gProcessId != 1)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ALLOWED);
        return NULL;
    }

    return (K2OS_PAGEARRAY_TOKEN)CrtKern_SysCall2(K2OS_SYSCALL_ID_ROOT_PAGEARRAY_CREATEAT, aPhysAddr, aPageCount);
}

UINT_PTR                
K2_CALLCONV_REGS
K2OS_Root_CreateTokenForProcess(
    K2OS_PROCESS_TOKEN  aTokProcess,
    K2OS_TOKEN          aToken
)
{
    if (gProcessId != 1)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ALLOWED);
        return 0;
    }
    return CrtKern_SysCall2(K2OS_SYSCALL_ID_ROOT_TOKEN_EXPORT, (UINT_PTR)aTokProcess, (UINT_PTR)aToken);
}

K2OS_TOKEN           
K2_CALLCONV_REGS 
K2OS_Root_CreateTokenForRoot(
    K2OS_PROCESS_TOKEN  aTokProcess,
    UINT_PTR            aProcessTokenValue
)
{
    if (gProcessId != 1)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ALLOWED);
        return 0;
    }
    return (K2OS_TOKEN)CrtKern_SysCall2(K2OS_SYSCALL_ID_ROOT_TOKEN_IMPORT, (UINT_PTR)aTokProcess, aProcessTokenValue);
}


UINT_PTR                              
K2OS_Root_CreatePlatNode(
    UINT_PTR    aParentPlatContext,
    UINT_PTR    aNodeName,
    UINT_PTR    aRootContext,
    char *      apIoBuf, 
    UINT_PTR    aBufSizeBytes,
    UINT_PTR    *apIoBufBytes
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT_PTR                ioBytes;
    UINT_PTR                result;

    if (gProcessId != 1)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ALLOWED);
        return 0;
    }

    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    if (aBufSizeBytes != 0)
    {
        if ((apIoBuf == NULL) ||
            (apIoBufBytes == NULL))
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
            return 0;
        }

        ioBytes = *apIoBufBytes;

        if ((aBufSizeBytes > K2OS_THREAD_PAGE_BUFFER_BYTES) ||
            (ioBytes > aBufSizeBytes))
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
            return 0;
        }

        if (0 != ioBytes)
        {
            K2MEM_Copy(pThreadPage->mMiscBuffer, apIoBuf, ioBytes);
        }
    }
    else
    {
        ioBytes = 0;
    }

    result = CrtKern_SysCall4(K2OS_SYSCALL_ID_ROOT_PLATNODE_CREATE, aParentPlatContext, aNodeName, aRootContext, ioBytes);

    if (0 == result)
        return 0;

    if (0 == aBufSizeBytes)
        return result;

    ioBytes = pThreadPage->mSysCall_Arg7_Result0;
    K2_ASSERT(ioBytes < K2OS_THREAD_PAGE_BUFFER_BYTES);

    if (0 != ioBytes)
    {
        if (ioBytes > aBufSizeBytes)
        {
            ioBytes = aBufSizeBytes;
        }
        K2MEM_Copy(apIoBuf, pThreadPage->mMiscBuffer, ioBytes);
    }

    *apIoBufBytes = ioBytes;

    return result;
}

BOOL                                  
K2OS_Root_AddPlatResource(
    UINT_PTR aPlatContext,
    UINT_PTR aResType,
    UINT_PTR aResInfo1,
    UINT_PTR aResInfo2
)
{
    if (gProcessId != 1)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_ALLOWED);
        return 0;
    }
    return CrtKern_SysCall4(K2OS_SYSCALL_ID_ROOT_PLATNODE_ADDRES, aPlatContext, aResType, aResInfo1, aResInfo2);
}

K2OS_INTERRUPT_TOKEN
K2_CALLCONV_REGS
K2OS_Root_HookPlatIntr(
    UINT_PTR aPlatContext,
    UINT_PTR aIrqIndex
)
{
    return (K2OS_INTERRUPT_TOKEN)CrtKern_SysCall2(K2OS_SYSCALL_ID_ROOT_PLATNODE_HOOKINTR, aPlatContext, aIrqIndex);
}

UINT_PTR 
K2_CALLCONV_REGS
K2OS_Root_GetProcessExitCode(
    K2OS_PROCESS_TOKEN aProcessToken
)
{
    return CrtKern_SysCall1(K2OS_SYSCALL_ID_ROOT_GET_PROCEXIT, (UINT_PTR)aProcessToken);
}

BOOL
K2_CALLCONV_REGS
K2OS_Root_StopProcess(
    K2OS_PROCESS_TOKEN  aProcessToken,
    UINT_PTR            aExitCode,
    BOOL                aWait
)
{
    BOOL ok;
    
    ok = (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_ROOT_PROCESS_STOP, (UINT_PTR)aProcessToken, aExitCode);
    
    if (!ok)
        return FALSE;

    if (aWait)
    {
        K2OS_Wait_One(aProcessToken, K2OS_TIMEOUT_INFINITE);
    }

    return TRUE;
}
