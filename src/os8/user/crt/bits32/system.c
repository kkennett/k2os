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
K2OS_System_GetInfo(
    UINT_PTR    aInfoId,
    UINT_PTR    aInfoIx,
    void *      apTargetBuffer,
    UINT_PTR *  apIoBytes
)
{
    K2STAT                  stat;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT32                  bytesIo;
    
    bytesIo = 0;

    if (NULL == apIoBytes)
    {
        if (NULL != apTargetBuffer)
            return K2STAT_ERROR_BAD_ARGUMENT;

        apIoBytes = &bytesIo;
    }

    bytesIo = *apIoBytes;
    if (0 == bytesIo)
    {
        apTargetBuffer = NULL;
    }

    if (NULL != apTargetBuffer)
    {
        CrtMem_Touch(apTargetBuffer, bytesIo);
    }

    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    stat = (K2STAT)CrtKern_SysCall4(K2OS_SYSCALL_ID_GET_SYSINFO, aInfoId, aInfoIx, (UINT_PTR)apTargetBuffer, bytesIo);

    *apIoBytes = pThreadPage->mSysCall_Arg7_Result0;

    return stat;
}

UINT_PTR
K2_CALLCONV_REGS
K2OS_System_GetCpuCoreCount(
    void
)
{
    K2STAT stat;
    UINT32 result;
    UINT32 bytesIo;

    bytesIo = sizeof(UINT32);
    result = 0;
    stat = K2OS_System_GetInfo(K2OS_SYSINFO_CPUCOUNT, 0, &result, &bytesIo);

    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    K2_ASSERT((bytesIo > 0) && (bytesIo <= sizeof(UINT32)));
    K2_ASSERT(0 != result);
    K2_ASSERT(K2OS_MAX_CPU_COUNT >= result);

    return result;
}

BOOL     
K2_CALLCONV_REGS   
K2OS_System_ProcessMsg(
    K2OS_MSG const *apMsg
)
{
    UINT_PTR control;

    if (NULL == apMsg)
        return FALSE;

    control = apMsg->mControl;

    if (0 == (control & K2OS_MSG_CONTROL_SYSTEM_FLAG))
        return FALSE;

    if (0 != (control & K2OS_MSG_CONTROL_SYSTEM_IPC))
        return K2OS_Ipc_ProcessMsg(apMsg);

    if (0 != (control & K2OS_MSG_CONTROL_SYSTEM_FUNC))
    {
        if (0 != apMsg->mPayload[0])
        {
            (*((CRT_pf_SysMsgRecv *)apMsg->mPayload[0]))((void *)apMsg->mPayload[0], apMsg->mControl, apMsg->mPayload[1], apMsg->mPayload[2]);
            return TRUE;
        }
    }

    //
    // any other system messages should be processed here
    // or ignored
    //

    return FALSE;
}
