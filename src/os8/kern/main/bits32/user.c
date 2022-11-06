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

K2OSKERN_OBJ_PAGEARRAY *
KernUser_AllocStockPageArray(
    UINT32          aPageCount,
    UINT8 const *   apInitData,
    UINT32          aInitDataBytes,
    UINT32          aUserPermit
)
{
    UINT32                  rangeBytes;
    K2OSKERN_OBJ_PAGEARRAY *pPageArray;
    K2OSKERN_PHYSSCAN       scan;
    UINT32                  ixPage;
    UINT32                  virtAddr;
    K2OSKERN_PHYSRES        res;
    BOOL                    ok;

    K2_ASSERT(aPageCount > 0);
    K2_ASSERT(aInitDataBytes <= (aPageCount * K2_VA_MEMPAGE_BYTES));

    rangeBytes = sizeof(K2OSKERN_OBJ_PAGEARRAY) + ((aPageCount - 1) * sizeof(UINT32));
    pPageArray = (K2OSKERN_OBJ_PAGEARRAY *)KernHeap_Alloc(rangeBytes);
    K2_ASSERT(NULL != pPageArray);

    ok = KernPhys_Reserve_Init(&res, aPageCount);
    K2_ASSERT(ok);

    K2MEM_Zero(pPageArray, rangeBytes);
    pPageArray->Hdr.mObjType = KernObj_PageArray;
    pPageArray->Hdr.mObjFlags = K2OSKERN_OBJ_FLAG_PERMANENT;
    K2LIST_Init(&pPageArray->Hdr.RefObjList);
    pPageArray->mType = KernPageArray_Sparse;
    pPageArray->mPageCount = aPageCount;
    pPageArray->mUserPermit = aUserPermit;
    KernPhys_AllocSparsePages(&res, aPageCount, &pPageArray->Data.Sparse.TrackList);
    KernPhys_ScanInit(&scan, &pPageArray->Data.Sparse.TrackList, 0);
    KernPhys_ScanToPhysPageArray(&scan, 0, &pPageArray->Data.Sparse.mPages[0]);
    // K2OSKERN_Debug("Create PageArray %08X (Stock)\n", pPageArray);

    //
    // now map the range to initialize it with data
    //
    virtAddr = KernVirt_AllocPages(pPageArray->mPageCount);
    K2_ASSERT(0 != virtAddr);

    //
    // map as R/W data
    //
    for (ixPage = 0; ixPage < pPageArray->mPageCount; ixPage++)
    {
        KernPte_MakePageMap(NULL, virtAddr + (ixPage * K2_VA_MEMPAGE_BYTES), KernPageArray_PagePhys(pPageArray, ixPage), K2OS_MAPTYPE_KERN_DATA);
    }

    //
    // copy in the source data and flush it
    //
    ixPage = aPageCount * K2_VA_MEMPAGE_BYTES;
    if (0 != aInitDataBytes)
    {
        K2MEM_Copy((UINT8 *)virtAddr, apInitData, aInitDataBytes);
        ixPage -= aInitDataBytes;
        if (0 != ixPage)
        {
            K2MEM_Zero((UINT8 *)(virtAddr + aInitDataBytes), ixPage);
        }
    }
    else
    {
        K2MEM_Zero((UINT8 *)virtAddr, ixPage);
    }
    K2OS_CacheOperation(K2OS_CACHEOP_FlushData, virtAddr, ixPage);

    //
    // break the map
    //
    for (ixPage = 0; ixPage < pPageArray->mPageCount; ixPage++)
    {
        KernPte_BreakPageMap(NULL, virtAddr + (ixPage * K2_VA_MEMPAGE_BYTES), 0);
        KernArch_InvalidateTlbPageOnCurrentCore(virtAddr + (ixPage * K2_VA_MEMPAGE_BYTES));
    }

    //
    // done with the virtual memory we used to initialize the data for the segment
    //
    KernVirt_FreePages(virtAddr);

    return pPageArray;
}

void
KernUser_Init(
    void
)
{
    K2OSKERN_PHYSTRACK *        pTrack;
    K2ROFS_FILE const *         pFile;
    UINT32                      memBytes;
    UINT32                      segAddr;
    K2OSKERN_PHYSRES            res;
    BOOL                        ok;
    K2OSKERN_OBJ_PAGEARRAY *    pPageArray;
    XDL_FILE_HEADER const *     pHeader;

    //
    // top init pt for user space is where the per-process top pagetable maps
    //
    gData.User.mTopInitPt = K2OS_UVA_PUBLICAPI_PAGETABLE_BASE;
    gData.User.mInitPageCount = (K2OS_KVA_KERN_BASE - K2OS_UVA_PUBLICAPI_PAGETABLE_BASE) / K2_VA32_PAGETABLE_MAP_BYTES;

    //
    // Create publicAPI range
    //
    pPageArray = &gData.User.PublicApiPageArray;
    K2MEM_Zero(pPageArray, sizeof(K2OSKERN_OBJ_PAGEARRAY));
    pPageArray->Hdr.mObjType = KernObj_PageArray;
    pPageArray->Hdr.mObjFlags = K2OSKERN_OBJ_FLAG_PERMANENT;
    K2LIST_Init(&pPageArray->Hdr.RefObjList);
    pPageArray->mType = KernPageArray_Sparse;
    pPageArray->mPageCount = 1;
    pPageArray->mUserPermit = K2OS_MEMPAGE_ATTR_READABLE | K2OS_MEMPAGE_ATTR_EXEC;
    ok = KernPhys_Reserve_Init(&res, pPageArray->mPageCount);
    K2_ASSERT(ok);
    KernPhys_AllocSparsePages(&res, pPageArray->mPageCount, &pPageArray->Data.Sparse.TrackList);
    K2_ASSERT(1 == pPageArray->Data.Sparse.TrackList.mNodeCount);
    pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pPageArray->Data.Sparse.TrackList.mpHead, ListLink);
    pPageArray->Data.Sparse.mPages[0] = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
//    K2OSKERN_Debug("Create PageArray %08X (publicapi)\n", pPageArray);

    //
    // map the publicApi page into the kernel as r/w data, clear it out, flush it
    //
    KernPte_MakePageMap(NULL, K2OS_KVA_PUBLICAPI_BASE, pPageArray->Data.Sparse.mPages[0], K2OS_MAPTYPE_KERN_DATA);
    K2MEM_Zero((void *)K2OS_KVA_PUBLICAPI_BASE, K2_VA_MEMPAGE_BYTES);
    K2OS_CacheOperation(K2OS_CACHEOP_FlushData, K2OS_KVA_PUBLICAPI_BASE, K2_VA_MEMPAGE_BYTES);

    //
    // set the timer fields in the publicapi page
    //
    *((UINT32 *)K2OS_KVA_PUBLICAPI_TIMER_FREQ) = gData.Timer.mFreq;
    *((UINT32 *)K2OS_KVA_PUBLICAPI_TIMER_ADDR) = K2OS_UVA_TIMER_IOPAGE_BASE + (gData.Timer.mIoPhys & K2_VA_MEMPAGE_OFFSET_MASK);

    //
    // Create timerio range
    //
    pPageArray = &gData.User.TimerPageArray;
    K2MEM_Zero(pPageArray, sizeof(K2OSKERN_OBJ_PAGEARRAY));
    pPageArray->Hdr.mObjType = KernObj_PageArray;
    pPageArray->Hdr.mObjFlags = K2OSKERN_OBJ_FLAG_PERMANENT;
    K2LIST_Init(&pPageArray->Hdr.RefObjList);
    pPageArray->mType = KernPageArray_Sparse;
    pPageArray->mPageCount = 1;
    pPageArray->mUserPermit = K2OS_MEMPAGE_ATTR_READABLE | K2OS_MEMPAGE_ATTR_DEVICEIO;
    pPageArray->Data.Sparse.mPages[0] = (gData.Timer.mIoPhys & K2_VA_PAGEFRAME_MASK);
//    K2OSKERN_Debug("Create PageArray %08X (TimerIo)\n", pPageArray);

    //
    // timer is already mapped into kernel in it's arch-specific address
    // for x32 this is K2OS_KVA_X32_HPET
    //

    //
    // ROFS is mapped right under the timer io page
    //
    gData.User.mTopInitVirtBar = K2OS_UVA_TIMER_IOPAGE_BASE - (gData.FileSys.PageArray.mPageCount * K2_VA_MEMPAGE_BYTES);

    segAddr = K2OS_UVA_LOW_BASE;

    //
    // per-process xdl global page and crt xdl page
    //
    gData.User.mInitPageCount += 2;
    segAddr += (2 * K2_VA_MEMPAGE_BYTES);

    //
    // get sizes of user crt segments from the k2oscrt user mode file header in the ROFS
    //
    pFile = K2ROFSHELP_SubFile(
        gData.FileSys.mpRofs,
        K2ROFS_ROOTDIR(gData.FileSys.mpRofs),
        "k2oscrt.xdl");
    K2_ASSERT(NULL != pFile);

    pHeader = (XDL_FILE_HEADER const *)K2ROFS_FILEDATA(gData.FileSys.mpRofs, pFile);

    K2_ASSERT(0 == pHeader->mImportsOffset);

    K2_ASSERT(segAddr == (UINT32)pHeader->Segment[XDLSegmentIx_Header].mLinkAddr);
    K2_ASSERT(segAddr == K2OS_UVA_CRTXDL_LOADPOINT);

    memBytes = K2_VA_MEMPAGE_BYTES;
    segAddr = ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Header].mLinkAddr) + memBytes;
    K2_ASSERT(segAddr == ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Text].mLinkAddr));

    //
    // calculate sizes of ranges for text, read, data, sym segments
    // 
    memBytes = (((UINT_PTR)pHeader->Segment[XDLSegmentIx_Text].mMemActualBytes) + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK;
    gData.User.mCrtTextPagesCount = memBytes / K2_VA_MEMPAGE_BYTES;
    segAddr = ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Text].mLinkAddr) + memBytes;
    K2_ASSERT(segAddr == ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Read].mLinkAddr));

    memBytes = (((UINT_PTR)pHeader->Segment[XDLSegmentIx_Read].mMemActualBytes) + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK;
    gData.User.mCrtReadPagesCount = memBytes / K2_VA_MEMPAGE_BYTES;
    segAddr = ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Read].mLinkAddr) + memBytes;
    K2_ASSERT(segAddr == ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Data].mLinkAddr));

    memBytes = (((UINT_PTR)pHeader->Segment[XDLSegmentIx_Data].mMemActualBytes) + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK;
    gData.User.mCrtDataPagesCount = memBytes / K2_VA_MEMPAGE_BYTES;
    segAddr = ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Data].mLinkAddr) + memBytes;
    K2_ASSERT(segAddr == ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Symbols].mLinkAddr));

    memBytes = (((UINT_PTR)pHeader->Segment[XDLSegmentIx_Symbols].mMemActualBytes) + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK;
    gData.User.mCrtSymPagesCount = memBytes / K2_VA_MEMPAGE_BYTES;
    segAddr = ((UINT_PTR)pHeader->Segment[XDLSegmentIx_Symbols].mLinkAddr) + memBytes;

    //
    // processes are going to map from K2OS_UVA_LOW_BASE tp segAddr - set up botinitpt and mInitPageCount
    //
    gData.User.mBotInitVirtBar = segAddr;

    //
    // will need a page for the tls pagetable for the process (maps 0->K2OS_UVA_LOW_BASE)
    //
    gData.User.mInitPageCount++;

    memBytes = segAddr - K2OS_UVA_LOW_BASE;
    gData.User.mBotInitPt = K2OS_UVA_LOW_BASE;
    do
    {
        gData.User.mInitPageCount++;
        gData.User.mBotInitPt += K2_VA32_PAGETABLE_MAP_BYTES;
        if (memBytes <= K2_VA32_PAGETABLE_MAP_BYTES)
            break;
        memBytes -= K2_VA32_PAGETABLE_MAP_BYTES;
    } while (1);

    //
    // need pages to cover k2oscrt copy-on-write data segment pages
    // 
    gData.User.mInitPageCount += gData.User.mCrtDataPagesCount;

    //
    // create the ranges for the segments and initialze them with the correct data
    // from the XDL file
    //

    // xdltrack is per-process, pagearray for it is allocated in raw proc create

    segAddr = 0;

    gData.User.mpKernUserCrtSeg[KernUserCrtSeg_XdlHeader] = KernUser_AllocStockPageArray(
        1,
        ((UINT8 const *)pHeader),
        (UINT_PTR)pHeader->Segment[XDLSegmentIx_Header].mMemActualBytes,
        K2OS_MEMPAGE_ATTR_READABLE
    );
    segAddr += (UINT_PTR)pHeader->Segment[XDLSegmentIx_Header].mSectorCount;

    gData.User.mpKernUserCrtSeg[KernUserCrtSeg_Text] = KernUser_AllocStockPageArray(
        gData.User.mCrtTextPagesCount,
        ((UINT8 const *)pHeader) + (segAddr * XDL_SECTOR_BYTES),
        (UINT_PTR)pHeader->Segment[XDLSegmentIx_Text].mMemActualBytes,
        K2OS_MEMPAGE_ATTR_EXEC | K2OS_MEMPAGE_ATTR_READABLE
        );
    segAddr += (UINT_PTR)pHeader->Segment[XDLSegmentIx_Text].mSectorCount;

    gData.User.mpKernUserCrtSeg[KernUserCrtSeg_Read] = KernUser_AllocStockPageArray(
        gData.User.mCrtReadPagesCount,
        ((UINT8 const *)pHeader) + (segAddr * XDL_SECTOR_BYTES),
        (UINT_PTR)pHeader->Segment[XDLSegmentIx_Read].mMemActualBytes,
        K2OS_MEMPAGE_ATTR_READABLE
        );
    segAddr += (UINT_PTR)pHeader->Segment[XDLSegmentIx_Read].mSectorCount;

    gData.User.mpKernUserCrtSeg[KernUserCrtSeg_Data] = KernUser_AllocStockPageArray(
        gData.User.mCrtDataPagesCount,
        ((UINT8 const *)pHeader) + (segAddr * XDL_SECTOR_BYTES),
        (UINT_PTR)pHeader->Segment[XDLSegmentIx_Data].mMemActualBytes,
        K2OS_MEMPAGE_ATTR_READWRITE
        );
    segAddr += (UINT_PTR)pHeader->Segment[XDLSegmentIx_Data].mSectorCount;

    gData.User.mpKernUserCrtSeg[KernUserCrtSeg_Sym] = KernUser_AllocStockPageArray(
        gData.User.mCrtSymPagesCount,
        ((UINT8 const *)pHeader) + (segAddr * XDL_SECTOR_BYTES),
        (UINT_PTR)pHeader->Segment[XDLSegmentIx_Symbols].mMemActualBytes,
        K2OS_MEMPAGE_ATTR_READABLE
        );

    //
    //  fill in public api page, and set idle threads' entry points and stack pointers in their contexts
    // 
    gData.User.mEntrypoint = pHeader->mEntryPoint;

    KernArch_UserInit();
}

