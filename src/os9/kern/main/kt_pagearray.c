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

K2OS_PAGEARRAY_TOKEN
K2OSKERN_PageArray_CreateAt(
    UINT32 aPhysBase,
    UINT32 aPageCount
)
{
    K2STAT          stat;
    K2OSKERN_OBJREF pageArrayRef;
    K2OS_TOKEN      tokResult;

    //
    // range should not be in allocatable memory area
    //
    if (KernPhys_InAllocatableRange(aPhysBase, aPageCount))
    {
//        K2OSKERN_Debug("***Requested spec pageArray(%08X,%d) is in allocatable range\n", apCurThread->User.mSysCall_Arg0, pThreadPage->mSysCall_Arg1);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_BOUNDS);
        return NULL;
    }

    pageArrayRef.AsAny = NULL;
    stat = KernPageArray_CreateSpec(
        aPhysBase,
        aPageCount, 
        K2OS_MEMPAGE_ATTR_READWRITE | K2OS_MEMPAGE_ATTR_UNCACHED | K2OS_MEMPAGE_ATTR_DEVICEIO, 
        &pageArrayRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    stat = KernToken_Create(pageArrayRef.AsAny, &tokResult);

    KernObj_ReleaseRef(&pageArrayRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    return tokResult;
}

K2OS_PAGEARRAY_TOKEN
K2OS_PageArray_Create(
    UINT32 aPageCount
)
{
    K2STAT          stat;
    K2OSKERN_OBJREF pageArrayRef;
    K2OS_TOKEN      tokResult;

    pageArrayRef.AsAny = NULL;

    stat = KernPageArray_CreateSparse(aPageCount, K2OS_MEMPAGE_ATTR_READWRITE | K2OS_MEMPAGE_ATTR_EXEC, &pageArrayRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    tokResult = NULL;
    stat = KernToken_Create(pageArrayRef.AsAny, &tokResult);

    KernObj_ReleaseRef(&pageArrayRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    return tokResult;
}

UINT32
K2OS_PageArray_GetLength(
    K2OS_PAGEARRAY_TOKEN aTokPageArray
)
{
    K2OSKERN_OBJREF pageArrayRef;
    K2STAT          stat;
    UINT32          result;

    pageArrayRef.AsAny = NULL;
    stat = KernToken_Translate(aTokPageArray, &pageArrayRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return 0;
    }

    if (pageArrayRef.AsAny->mObjType == KernObj_PageArray)
    {
        result = pageArrayRef.AsPageArray->mPageCount;
    }
    else
    {
        result = 0;
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
    }

    KernObj_ReleaseRef(&pageArrayRef);

    return result;
}
