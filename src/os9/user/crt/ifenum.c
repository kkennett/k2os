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

K2OS_IFENUM_TOKEN
K2OS_IfEnum_Create(
    BOOL                aOneProc,
    UINT32              aProcessId,
    UINT32              aClassCode,
    K2_GUID128 const *  apSpecific
)
{
    K2_GUID128      guid;
    UINT32 const *  pId;

    if (NULL == apSpecific)
    {
        K2MEM_Zero(&guid, sizeof(guid));
        apSpecific = &guid;
    }

    pId = (UINT32 const *)apSpecific;

    return (K2OS_TOKEN)CrtKern_SysCall7(K2OS_SYSCALL_ID_IFENUM_CREATE,
        aOneProc,
        aProcessId,
        aClassCode,
        pId[0], pId[1], pId[2], pId[3]);
}

BOOL
K2OS_IfEnum_Reset(
    K2OS_IFENUM_TOKEN aIfEnumToken
)
{
    return (BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_IFENUM_RESET, (UINT32)aIfEnumToken);
}

BOOL
K2OS_IfEnum_Next(
    K2OS_IFENUM_TOKEN   aIfEnumToken,
    K2OS_IFINST_DETAIL *apEntryBuffer,
    UINT32 *            apIoEntryBufferCount
)
{
    UINT32              entryCountIn;
    BOOL                result;
    K2OS_THREAD_PAGE *  pThreadPage;

    if ((aIfEnumToken == NULL) ||
        (NULL == apEntryBuffer) ||
        (NULL == apIoEntryBufferCount) ||
        (0 == (*apIoEntryBufferCount)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    entryCountIn = *apIoEntryBufferCount;

    CrtMem_Touch(apEntryBuffer, entryCountIn * sizeof(K2OS_IFINST_DETAIL));

    result = CrtKern_SysCall3(K2OS_SYSCALL_ID_IFENUM_NEXT,
        (UINT32)aIfEnumToken,
        (UINT32)apEntryBuffer,
        entryCountIn);

    if (!result)
        return FALSE;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    *apIoEntryBufferCount = pThreadPage->mSysCall_Arg7_Result0;

    return TRUE;
}
