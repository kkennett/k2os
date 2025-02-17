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

K2OS_IFINST_TOKEN   
K2OS_IfInst_Create(
    UINT32          aContext,
    K2OS_IFINST_ID *apRetId
)
{
    K2OS_IFINST_TOKEN   tokResult;
    K2OS_THREAD_PAGE *  pThreadPage;

    tokResult = (K2OS_IFINST_TOKEN)CrtKern_SysCall1(K2OS_SYSCALL_ID_IFINST_CREATE, aContext);
    if (NULL == tokResult)
        return NULL;

    if (NULL != apRetId)
    {
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
        *apRetId = pThreadPage->mSysCall_Arg7_Result0;
    }

    return tokResult;
}

K2OS_IFINST_ID      
K2OS_IfInst_GetId(
    K2OS_IFINST_TOKEN aTokIfInst
)
{
    if (NULL == aTokIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return 0;
    }

    return CrtKern_SysCall1(K2OS_SYSCALL_ID_IFINST_GETID, (UINT32)aTokIfInst);
}

BOOL                
K2OS_IfInst_SetMailbox(
    K2OS_IFINST_TOKEN   aTokIfInst,
    K2OS_MAILBOX_TOKEN  aTokMailbox
)
{
    if (NULL == aTokIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return FALSE;
    }

    return (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_IFINST_SETMAILBOX, (UINT32)aTokIfInst, (UINT32)aTokMailbox);
}

BOOL                
K2OS_IfInst_GetContext(
    K2OS_IFINST_TOKEN   aTokIfInst,
    UINT32 *            apRetContext
)
{
    BOOL                result;
    K2OS_THREAD_PAGE *  pThreadPage;

    if (NULL == aTokIfInst) 
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return FALSE;
    }

    if (NULL == apRetContext)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    result = CrtKern_SysCall2(K2OS_SYSCALL_ID_IFINST_CONTEXTOP, (UINT32)aTokIfInst, 0);

    if (result)
    {
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
        *apRetContext = pThreadPage->mSysCall_Arg7_Result0;
    }

    return result;
}

BOOL                
K2OS_IfInst_SetContext(
    K2OS_IFINST_TOKEN   aTokIfInst,
    UINT32              aContext
)
{
    if (NULL == aTokIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return FALSE;
    }

    return (BOOL)CrtKern_SysCall3(K2OS_SYSCALL_ID_IFINST_CONTEXTOP, (UINT32)aTokIfInst, 1, aContext);
}

BOOL                
K2OS_IfInst_Publish(
    K2OS_IFINST_TOKEN   aTokIfInst,
    UINT32              aClassCode,
    K2_GUID128 const *  apSpecific
)
{
    K2OS_THREAD_PAGE *  pThreadPage;

    if (NULL == aTokIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return FALSE;
    }

    if ((0 == aClassCode) ||
        (NULL == apSpecific) ||
        (K2MEM_VerifyZero(apSpecific, sizeof(K2_GUID128))))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    K2MEM_Copy(pThreadPage->mMiscBuffer, apSpecific, sizeof(K2_GUID128));

    return (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_IFINST_PUBLISH, (UINT32)aTokIfInst, aClassCode);
}

BOOL 
K2OS_IfInstId_GetDetail(
    K2OS_IFINST_ID      aIfInstId,
    K2OS_IFINST_DETAIL *apRetDetail
)
{
    K2OS_THREAD_PAGE *  pThreadPage;
    BOOL                result;

    if ((0 == aIfInstId) ||
        (NULL == apRetDetail))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    result = (BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_IFINSTID_GETDETAIL, aIfInstId);
    if (result)
    {
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
        K2MEM_Copy(apRetDetail, pThreadPage->mMiscBuffer, sizeof(K2OS_IFINST_DETAIL));
    }

    return result;
}
