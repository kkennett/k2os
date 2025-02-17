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

void    
KernPageArray_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PAGEARRAY *    apPageArray
)
{
    K2_ASSERT(0 != (apPageArray->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    switch (apPageArray->mPageArrayType)
    {
    case KernPageArray_Track:
        KernPhys_FreeTrack(apPageArray->Data.Track.mpTrack);
        break;
    case KernPageArray_Sparse:
        KernPhys_FreeTrackList(&apPageArray->Data.Sparse.TrackList);
        break;
    case KernPageArray_Spec:
        break;
    default:
        K2OSKERN_Panic("*** KernPageArray_Cleanup - unknown pagearray type for cleanup\n");
        break;
    }

    // free takes care of sparse case 
    KernObj_Free(&apPageArray->Hdr);
}

K2STAT  
KernPageArray_CreateSparseFromRes(
    K2OSKERN_PHYSRES *  apRes,
    UINT32              aPageCount,
    UINT32              aUserPermit,
    K2OSKERN_OBJREF *   apRetRef
)
{
    UINT32                  rangeBytes;
    K2OSKERN_OBJ_PAGEARRAY *pPageArray;
    K2STAT                  stat;
    K2OSKERN_PHYSSCAN       physScan;
    UINT32                  ixPage;

    K2_ASSERT(aPageCount <= apRes->mPageCount);

    if (0 == aPageCount)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    rangeBytes = sizeof(K2OSKERN_OBJ_PAGEARRAY) + ((aPageCount - 1) * sizeof(UINT32));

    pPageArray = (K2OSKERN_OBJ_PAGEARRAY *)KernHeap_Alloc(rangeBytes);
    if (NULL == pPageArray)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pPageArray, rangeBytes);
    pPageArray->Hdr.mObjType = KernObj_PageArray;
    K2LIST_Init(&pPageArray->Hdr.RefObjList);

    //
    // all resources allocated. can't fail from here on in
    //
    pPageArray->mPageCount = aPageCount;
    pPageArray->mPageArrayType = KernPageArray_Sparse;
    pPageArray->mUserPermit = aUserPermit;

    //
    // pages are reserved so this should not be able to fail
    //
    stat = KernPhys_AllocSparsePages(apRes, aPageCount, &pPageArray->Data.Sparse.TrackList);
    if (K2STAT_IS_ERROR(stat))
    {
        KernHeap_Free(pPageArray);
        return stat;
    }

    KernPhys_ScanInit(&physScan, &pPageArray->Data.Sparse.TrackList, 0);
    for (ixPage = 0; ixPage < aPageCount; ixPage++)
    {
        pPageArray->Data.Sparse.mPages[ixPage] = KernPhys_ScanIter(&physScan);
    }

    //    K2OSKERN_Debug("Create PageArray %08X\n", pPageArray);
    KernObj_CreateRef(apRetRef, &pPageArray->Hdr);

    return K2STAT_NO_ERROR;
}

K2STAT
KernPageArray_CreateSparse(
    UINT32              aPageCount,
    UINT32              aUserPermit,
    K2OSKERN_OBJREF *   apRetRef
)
{
    K2OSKERN_PHYSRES    Res;
    K2STAT              stat;

    if (0 == aPageCount)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (!KernPhys_Reserve_Init(&Res, aPageCount))
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    stat = KernPageArray_CreateSparseFromRes(&Res, aPageCount, aUserPermit, apRetRef);
    if (K2STAT_IS_ERROR(stat))
    {
        KernPhys_Reserve_Release(&Res);
    }

    return stat;
}

K2STAT
KernPageArray_CreateSpec(
    UINT32              aPhysAddr,
    UINT32              aPageCount,
    UINT32              aUserPermit,
    K2OSKERN_OBJREF *   apRetRef
)
{
    K2OSKERN_OBJ_PAGEARRAY * pPageArray;

    if ((0 == aPageCount) ||
        (0 != (aPhysAddr & K2_VA_MEMPAGE_OFFSET_MASK)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pPageArray = (K2OSKERN_OBJ_PAGEARRAY *)KernObj_Alloc(KernObj_PageArray);
    if (NULL == pPageArray)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pPageArray->mPageArrayType = KernPageArray_Spec;
    pPageArray->mPageCount = aPageCount;
    pPageArray->mUserPermit = aUserPermit;
    pPageArray->Data.Spec.mBasePhys = aPhysAddr;

    KernObj_CreateRef(apRetRef, &pPageArray->Hdr);

    return K2STAT_NO_ERROR;
}

K2STAT  
KernPageArray_CreatePreMap(
    UINT32              aVirtAddr,
    UINT32              aPageCount,
    UINT32              aUserPermit,
    K2OSKERN_OBJREF *   apRetRef
)
{
    K2OSKERN_OBJ_PAGEARRAY * pPageArray;

    if ((0 == aPageCount) ||
        (0 != (aVirtAddr & K2_VA_MEMPAGE_OFFSET_MASK)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pPageArray = (K2OSKERN_OBJ_PAGEARRAY *)KernObj_Alloc(KernObj_PageArray);
    if (NULL == pPageArray)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pPageArray->mPageArrayType = KernPageArray_PreMap;
    pPageArray->mPageCount = aPageCount;
    pPageArray->mUserPermit = aUserPermit;
    pPageArray->Data.PreMap.mpPTEs = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(aVirtAddr);

    KernObj_CreateRef(apRetRef, &pPageArray->Hdr);

    return K2STAT_NO_ERROR;
}

K2STAT  
KernPageArray_CreateTrack(
    K2OSKERN_PHYSTRACK *apTrack,
    UINT32              aUserPermit,
    K2OSKERN_OBJREF *   apRetRef
)
{
    K2OSKERN_OBJ_PAGEARRAY * pPageArray;

    pPageArray = (K2OSKERN_OBJ_PAGEARRAY *)KernObj_Alloc(KernObj_PageArray);
    if (NULL == pPageArray)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pPageArray->mPageArrayType = KernPageArray_Track;
    pPageArray->mPageCount = (1 << apTrack->Flags.Field.BlockSize) / K2_VA_MEMPAGE_BYTES;
    pPageArray->mUserPermit = aUserPermit;
    pPageArray->Data.Track.mpTrack = apTrack;

    KernObj_CreateRef(apRetRef, &pPageArray->Hdr);

    return K2STAT_NO_ERROR;
}

UINT32
KernPageArray_PagePhys(
    K2OSKERN_OBJ_PAGEARRAY *apPageArray,
    UINT32                  aPageIx
)
{
    UINT32 result;

    K2_ASSERT(aPageIx < apPageArray->mPageCount);
    switch (apPageArray->mPageArrayType)
    {
    case KernPageArray_Track:
        result = K2OS_PHYSTRACK_TO_PHYS32((UINT32)apPageArray->Data.Track.mpTrack);
        result += aPageIx * K2_VA_MEMPAGE_BYTES;
        break;
    case KernPageArray_Spec:
        result = apPageArray->Data.Spec.mBasePhys;
        result += aPageIx * K2_VA_MEMPAGE_BYTES;
        break;
    case KernPageArray_PreMap:
        result = apPageArray->Data.PreMap.mpPTEs[aPageIx];
        break;
    case KernPageArray_Sparse:
        result = apPageArray->Data.Sparse.mPages[aPageIx];
        break;
    default:
        K2OSKERN_Panic(NULL);
        break;
    }

    return result & K2_VA_PAGEFRAME_MASK;
}

void
KernPageArray_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                      stat;
    K2OSKERN_OBJREF             pageArrayRef;
    K2OS_THREAD_PAGE *     pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    pageArrayRef.AsAny = NULL;

    stat = KernPageArray_CreateSparse(apCurThread->User.mSysCall_Arg0, K2OS_MEMPAGE_ATTR_READWRITE | K2OS_MEMPAGE_ATTR_EXEC, &pageArrayRef);
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
        return;
    }
    
    stat = KernProc_TokenCreate(apCurThread->RefProc.AsProc, pageArrayRef.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);

    KernObj_ReleaseRef(&pageArrayRef);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernPageArray_SysCall_GetLen(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJREF         pageArrayRef;
    K2OS_THREAD_PAGE * pThreadPage;

    pProc = apCurThread->RefProc.AsProc;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    pageArrayRef.AsAny = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &pageArrayRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_PageArray != pageArrayRef.AsAny->mObjType)
        {
//            K2OSKERN_Debug("KernPageArray_SysCall_GetLen - token does not refer to a page array\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            apCurThread->User.mSysCall_Result = pageArrayRef.AsPageArray->mPageCount;
        }
        KernObj_ReleaseRef(&pageArrayRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}


