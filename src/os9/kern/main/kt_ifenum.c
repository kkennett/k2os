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

K2OS_IFENUM_TOKEN   
K2OS_IfEnum_Create(
    BOOL                aOneProc,
    UINT32              aProcessId,
    UINT32              aClassCode,
    K2_GUID128 const *  apSpecific
)
{
    K2STAT              stat;
    K2OSKERN_OBJREF     enumRef;
    K2OS_IFENUM_TOKEN   tokResult;

    enumRef.AsAny = NULL;
    stat = KernIfEnum_Create(aOneProc, aProcessId, aClassCode, apSpecific, &enumRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    tokResult = NULL;
    stat = KernToken_Create(enumRef.AsAny, &tokResult);

    KernObj_ReleaseRef(&enumRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        K2_ASSERT(NULL == tokResult);
    }
    else
    {
        K2_ASSERT(NULL != tokResult);
    }

    return tokResult;
}

BOOL                
K2OS_IfEnum_Reset(
    K2OS_IFENUM_TOKEN aIfEnumToken
)
{
    K2STAT          stat;
    K2OSKERN_OBJREF objRef;
    BOOL            disp;

    objRef.AsAny = NULL;
    stat = KernToken_Translate(aIfEnumToken, &objRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return FALSE;
    }
        
    if (objRef.AsAny->mObjType != KernObj_IfEnum)
    {
        stat = K2STAT_ERROR_BAD_TOKEN;
    }
    else
    {
        disp = K2OSKERN_SeqLock(&objRef.AsIfEnum->SeqLock);
        objRef.AsIfEnum->Locked.mCurrentIx = 0;
        K2OSKERN_SeqUnlock(&objRef.AsIfEnum->SeqLock, disp);
        stat = K2STAT_NO_ERROR;
    }

    KernObj_ReleaseRef(&objRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL                
K2OS_IfEnum_Next(
    K2OS_IFENUM_TOKEN   aIfEnumToken,
    K2OS_IFINST_DETAIL *apEntryBuffer,
    UINT32 *            apIoEntryBufferCount
)
{
    K2STAT          stat;
    K2OSKERN_OBJREF objRef;
    UINT32          userCount;

    if ((NULL == aIfEnumToken) ||
        (NULL == apEntryBuffer) ||
        (NULL == apIoEntryBufferCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    userCount = *apIoEntryBufferCount;
    if (userCount == 0)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    objRef.AsAny = NULL;
    stat = KernToken_Translate(aIfEnumToken, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (objRef.AsAny->mObjType != KernObj_IfEnum)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernIfEnum_Next(objRef.AsIfEnum, apEntryBuffer, &userCount);
            if (!K2STAT_IS_ERROR(stat))
            {
                K2_ASSERT(userCount > 0);
                K2_ASSERT(userCount <= *apIoEntryBufferCount);
                *apIoEntryBufferCount = userCount;
            }
        }

        KernObj_ReleaseRef(&objRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }
    
    return TRUE;
}
