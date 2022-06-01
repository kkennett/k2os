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

#include "kern.h"

void    
KernPageArray_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PAGEARRAY *    apPageArray
)
{
    K2_ASSERT(0 != (apPageArray->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    switch (apPageArray->mType)
    {
    case KernPageArray_Sparse:
        KernPhys_FreeTrackList(&apPageArray->Data.Sparse.TrackList);
        break;
    case KernPageArray_Spec:
        break;
    default:
        K2_ASSERT(0);
    }

    K2MEM_Zero(apPageArray, sizeof(K2OSKERN_OBJ_PAGEARRAY));

    KernHeap_Free(apPageArray);
}

K2STAT
KernPageArray_CreateSparse(
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

    if (!KernPhys_Reserve_Init(&pPageArray->Data.Sparse.Res, aPageCount))
    {
        KernHeap_Free(pPageArray);
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    //
    // all resources allocated. can't fail from here on in
    //
    pPageArray->mPageCount = aPageCount;
    pPageArray->mType = KernPageArray_Sparse;
    pPageArray->mUserPermit = aUserPermit;

    //
    // pages are reserved so this should not be able to fail
    //
    stat = KernPhys_AllocSparsePages(&pPageArray->Data.Sparse.Res, aPageCount, &pPageArray->Data.Sparse.TrackList);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

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

    pPageArray = (K2OSKERN_OBJ_PAGEARRAY *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_PAGEARRAY));
    if (NULL == pPageArray)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pPageArray, sizeof(K2OSKERN_OBJ_PAGEARRAY));

    pPageArray->Hdr.mObjType = KernObj_PageArray;
    K2LIST_Init(&pPageArray->Hdr.RefObjList);

    pPageArray->mType = KernPageArray_Spec;
    pPageArray->mPageCount = aPageCount;
    pPageArray->mUserPermit = aUserPermit;
    pPageArray->Data.Spec.mBasePhys = aPhysAddr;

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
    switch (apPageArray->mType)
    {
    case KernPageArray_Contig:
        result = K2OS_PHYSTRACK_TO_PHYS32((UINT32)apPageArray->Data.Contig.mpTrack);
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
    K2OS_USER_THREAD_PAGE *     pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    pageArrayRef.Ptr.AsHdr = NULL;

    stat = KernPageArray_CreateSparse(apCurThread->mSysCall_Arg0, K2OS_MEMPAGE_ATTR_READWRITE | K2OS_MEMPAGE_ATTR_EXEC, &pageArrayRef);
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
        return;
    }

    stat = KernProc_TokenCreate(apCurThread->ProcRef.Ptr.AsProc, pageArrayRef.Ptr.AsHdr, (K2OS_TOKEN *)&apCurThread->mSysCall_Result);

    KernObj_ReleaseRef(&pageArrayRef);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
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
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pProc = apCurThread->ProcRef.Ptr.AsProc;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    pageArrayRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &pageArrayRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_PageArray != pageArrayRef.Ptr.AsHdr->mObjType)
        {
//            K2OSKERN_Debug("KernPageArray_SysCall_GetLen - token does not refer to a page array\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            apCurThread->mSysCall_Result = pageArrayRef.Ptr.AsPageArray->mPageCount;
        }
        KernObj_ReleaseRef(&pageArrayRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void
KernPageArray_SysCall_Root_CreateAt(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                      stat;
    K2OSKERN_OBJREF             pageArrayRef;
    K2OS_USER_THREAD_PAGE *     pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    //
    // range should not be in allocatable memory area
    //
    if (KernPhys_InAllocatableRange(apCurThread->mSysCall_Arg0, pThreadPage->mSysCall_Arg1))
    {
//        K2OSKERN_Debug("***Requested spec pageArray(%08X,%d) is in allocatable range\n", apCurThread->mSysCall_Arg0, pThreadPage->mSysCall_Arg1);
        stat = K2STAT_ERROR_OUT_OF_BOUNDS;
    }
    else
    {
        pageArrayRef.Ptr.AsHdr = NULL;
        stat = KernPageArray_CreateSpec(
            apCurThread->mSysCall_Arg0, 
            pThreadPage->mSysCall_Arg1, 
            K2OS_MEMPAGE_ATTR_READWRITE | K2OS_MEMPAGE_ATTR_UNCACHED | K2OS_MEMPAGE_ATTR_DEVICEIO, 
            &pageArrayRef);
        if (!K2STAT_IS_ERROR(stat))
        {
            stat = KernProc_TokenCreate(apCurThread->ProcRef.Ptr.AsProc, pageArrayRef.Ptr.AsHdr, (K2OS_TOKEN *)&apCurThread->mSysCall_Result);
            KernObj_ReleaseRef(&pageArrayRef);
        }
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}
