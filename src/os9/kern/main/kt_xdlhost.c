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
#include "../../../shared/lib/k2xdl/xdl_struct.h"

UINT32  
KernXdl_Threaded_CreateRwSegment(
    UINT32              aPageCount,
    K2OSKERN_OBJREF *   apRetRef
)
{
    K2OSKERN_OBJREF refPageArray;
    K2STAT          stat;
    UINT32          virtAddr;

    refPageArray.AsAny = NULL;
    stat = KernPageArray_CreateSparse(aPageCount, 0, &refPageArray);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return 0;
    }

    virtAddr = K2OS_Virt_Reserve(aPageCount);
    if (0 == virtAddr)
    {
        K2_ASSERT(K2STAT_IS_ERROR(K2OS_Thread_GetLastStatus()));
    }
    else
    {
        stat = KernVirtMap_Create(refPageArray.AsPageArray, 0, aPageCount, virtAddr, K2OS_MapType_Data_ReadWrite, apRetRef);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Virt_Release(virtAddr);
        }
    }
    
    KernObj_ReleaseRef(&refPageArray);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return 0;
    }

    return virtAddr;
}

void
KernXdl_Remap_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);
    KTRACE(apThisCore, 1, KTRACE_XDL_REMAP_CLEAN_CHECK_DPC);

    if (0 == pThread->TlbShoot.mCoresRemaining)
    {
        KTRACE(apThisCore, 1, KTRACE_XDL_REMAP_CLEAN_DONE);
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernXdl_Remap_CheckComplete;
        KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
    }
}

void
KernXdl_Remap_SendIci(
    K2OSKERN_CPUCORE volatile *apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    UINT32                  sentMask;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pThread->mIciSendMask);

    KTRACE(apThisCore, 1, KTRACE_XDL_REMAP_CLEAN_SENDICI_DPC);

    sentMask = KernArch_SendIci(
        apThisCore,
        pThread->mIciSendMask,
        KernIci_TlbInv,
        &pThread->TlbShoot
    );

    pThread->mIciSendMask &= ~sentMask;

    if (0 == pThread->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pThread->Hdr.ObjDpc.Func = KernXdl_Remap_CheckComplete;
    }
    else
    {
        pThread->Hdr.ObjDpc.Func = KernXdl_Remap_SendIci;
    }

    KernCpu_QueueDpc(&pThread->Hdr.ObjDpc.Dpc, &pThread->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
}

BOOL
KernXdl_Threaded_RemapSegment(
    UINT32                  aSegAddr,
    K2OS_VirtToPhys_MapType aNewMapType
)
{
    K2TREE_NODE *               pTreeNode;
    K2OSKERN_OBJ_VIRTMAP *      pVirtMap;
    K2OSKERN_OBJREF             refVirtMap;
    UINT32                      virtAddr;
    UINT32                      pageCount;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    UINT32                      mapAttr;
    UINT32                      pageIx;

    refVirtMap.AsAny = NULL;

    disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);

    pTreeNode = K2TREE_Find(&gData.VirtMap.Tree, aSegAddr);
    if (NULL != pTreeNode)
    {
        pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
        KernObj_CreateRef(&refVirtMap, &pVirtMap->Hdr);
    }

    K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);

    if (NULL == refVirtMap.AsAny)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    K2_ASSERT(KernSeg_Xdl_Part == refVirtMap.AsVirtMap->Kern.mSegType);

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    //
    // break the mapping but don't mark the ptes as freed
    //
    virtAddr = refVirtMap.AsVirtMap->OwnerMapTreeNode.mUserVal;
    pageCount = refVirtMap.AsVirtMap->mPageCount;
    do {
        KernPte_BreakPageMap(NULL, virtAddr, K2OSKERN_PTE_MAP_CREATE);
        virtAddr += K2_VA_MEMPAGE_BYTES;
    } while (--pageCount);

    //
    // shoot it down
    //
    
    if (gData.mCpuCoreCount > 1)
    {
        pThisThread->TlbShoot.mVirtBase = refVirtMap.AsVirtMap->OwnerMapTreeNode.mUserVal;
        pThisThread->TlbShoot.mpProc = NULL;
        pThisThread->TlbShoot.mPageCount = refVirtMap.AsVirtMap->mPageCount;

        //
        // need to enter monitor for DPCs to do the shootdown
        //
        disp = K2OSKERN_SetIntr(FALSE);
        K2_ASSERT(disp);
        pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
        K2_ASSERT(pThisCore->mpActiveThread == pThisThread);

        pThisThread->TlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << pThisCore->mCoreIx);
        pThisThread->mIciSendMask = pThisThread->TlbShoot.mCoresRemaining;

        KernXdl_Remap_SendIci(pThisCore, &pThisThread->Hdr.ObjDpc.Func);

        KernArch_IntsOff_SaveKernelThreadStateAndEnterMonitor(pThisCore, pThisThread);
        //
        // this is return point from entering the monitor to do the shootdown
        // interrupts will be on
        //
        K2_ASSERT(K2OSKERN_GetIntr());
    }

    for (pageIx = 0; pageIx < pageCount; pageIx++)
    {
        KernArch_InvalidateTlbPageOnCurrentCore(virtAddr);
        virtAddr += K2_VA_MEMPAGE_BYTES;
    };


    //
    // remap the segment with the desired mapping type
    //
    virtAddr = refVirtMap.AsVirtMap->OwnerMapTreeNode.mUserVal;
    pageCount = refVirtMap.AsVirtMap->mPageCount;

    switch (aNewMapType)
    {
    case K2OS_MapType_Text:
        mapAttr = K2OS_MEMPAGE_ATTR_EXEC | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_Data_ReadOnly:
        mapAttr = K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_Data_ReadWrite:
    case K2OS_MapType_Thread_Stack:
        mapAttr = K2OS_MEMPAGE_ATTR_WRITEABLE | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_Write_Thru:
        mapAttr = K2OS_MEMPAGE_ATTR_WRITEABLE | K2OS_MEMPAGE_ATTR_READABLE | K2OS_MEMPAGE_ATTR_WRITE_THRU;
        break;
    case K2OS_MapType_MemMappedIo_ReadOnly:
        mapAttr = K2OS_MEMPAGE_ATTR_DEVICEIO | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_MemMappedIo_ReadWrite:
        mapAttr = K2OS_MEMPAGE_ATTR_DEVICEIO | K2OS_MEMPAGE_ATTR_READWRITE;
        break;
    default:
        K2OSKERN_Panic("KernXdl_RemapSegment unknown map type (%d)\n", aNewMapType);
        break;
    }

    for (pageIx = 0; pageIx < pageCount; pageIx++)
    {
        KernPte_MakePageMap(NULL, virtAddr, KernPageArray_PagePhys(refVirtMap.AsVirtMap->PageArrayRef.AsPageArray, pageIx), mapAttr);
        virtAddr += K2_VA_MEMPAGE_BYTES;
    };

    refVirtMap.AsVirtMap->mVirtToPhysMapType = aNewMapType;

    K2_CpuWriteBarrier();

    KernObj_ReleaseRef(&refVirtMap);

    return TRUE;
}

BOOL    
KernXdl_Threaded_FreeSegment(
    UINT32  aSegAddr
)
{
    K2TREE_NODE *               pTreeNode;
    K2OSKERN_OBJ_VIRTMAP *      pVirtMap;
    K2OSKERN_OBJREF             refVirtMap;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_XDL_TRACK *        pTrack;
    K2OSKERN_HOST_FILE *        pHostFile;

    if (0 != (aSegAddr & K2_VA_MEMPAGE_OFFSET_MASK))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    refVirtMap.AsAny = NULL;

    disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);

    pTreeNode = K2TREE_Find(&gData.VirtMap.Tree, aSegAddr);
    if (NULL != pTreeNode)
    {
        pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
        KernObj_CreateRef(&refVirtMap, &pVirtMap->Hdr);
    }

    K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);

    if (NULL == refVirtMap.AsAny)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    if (refVirtMap.AsVirtMap->Kern.mSegType == KernSeg_Xdl_Page)
    {
        pHostFile = (K2OSKERN_HOST_FILE *)refVirtMap.AsVirtMap->Kern.XdlPage.mOwnerHostFile;
        K2_ASSERT(pHostFile->RefXdlPageVirtMap.AsVirtMap == refVirtMap.AsVirtMap);
        KernObj_ReleaseRef(&pHostFile->RefXdlPageVirtMap);
    }
    else
    {
        K2_ASSERT(refVirtMap.AsVirtMap->Kern.mSegType == KernSeg_Xdl_Part);
        pTrack = refVirtMap.AsVirtMap->Kern.XdlPart.mpOwnerTrack;
        KernObj_ReleaseRef(&pTrack->SegVirtMapRef[refVirtMap.AsVirtMap->Kern.XdlPart.mXdlSegmentIx]);
    }

    KernObj_ReleaseRef(&refVirtMap);

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);
    //
    // need to enter monitor for DPCs to run
    //
    disp = K2OSKERN_SetIntr(FALSE);
    K2_ASSERT(disp);
    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    K2_ASSERT(pThisCore->mpActiveThread == pThisThread);
    KernArch_IntsOff_SaveKernelThreadStateAndEnterMonitor(pThisCore, pThisThread);
    //
    // this is return point from entering the monitor to do the shootdown
    // interrupts will be on
    K2_ASSERT(K2OSKERN_GetIntr());

    K2OS_Virt_Release(aSegAddr);

    return TRUE;
}

K2STAT
KernXdlHost_Threaded_CritSec(
    BOOL aEnter
)
{
    if (aEnter)
    {
        K2OS_CritSec_Enter(&gData.Xdl.Sec);
    }
    else
    {
        K2OS_CritSec_Leave(&gData.Xdl.Sec);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
KernXdlHost_Threaded_Open(
    K2XDL_OPENARGS const *  apArgs,
    K2XDL_HOST_FILE *       apRetHostFile,
    UINT32 *                apRetModuleDataAddr,
    UINT32 *                apRetModuleLinkAddr
)
{
    K2OSKERN_HOST_FILE *pHostFile;
    K2STAT              stat;
    char const *        pChk;
    UINT32              specLen;
    char *              pTemp;
    UINT32              xdlPageVirt;

    if (apArgs->mNameLen == 0)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pHostFile = (K2OSKERN_HOST_FILE *)K2OS_Heap_Alloc(sizeof(K2OSKERN_HOST_FILE));
    if (NULL == pHostFile)
    {
        return K2OS_Thread_GetLastStatus();
    }

    K2MEM_Zero(pHostFile, sizeof(K2OSKERN_HOST_FILE));

    if (0 != KernXdl_Threaded_CreateRwSegment(1, &pHostFile->RefXdlPageVirtMap))
    {
        pHostFile->RefXdlPageVirtMap.AsVirtMap->Kern.mSegType = KernSeg_Xdl_Page;
        pHostFile->RefXdlPageVirtMap.AsVirtMap->Kern.XdlPage.mOwnerHostFile = (K2XDL_HOST_FILE)pHostFile;
        
        xdlPageVirt = pHostFile->RefXdlPageVirtMap.AsVirtMap->OwnerMapTreeNode.mUserVal;

        pHostFile->mFile = K2OS_FsClient_OpenFile(
            gData.Xdl.mFsClient,
            apArgs->mpPath,
            K2OS_ACCESS_R,
            K2OS_ACCESS_R,
            K2OS_FileOpen_Existing,
            0, 0
        );

        if (NULL == pHostFile->mFile)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
        else
        {
            stat = K2STAT_NO_ERROR;
        }

        if (K2STAT_ERROR_NOT_FOUND == stat)
        {
            //
            // not found under name specified as a file
            // check to see if it has an extension. if not then
            // try loading it with the xdl exetnsion
            //
            pChk = apArgs->mpNamePart + apArgs->mNameLen;
            while (pChk != apArgs->mpNamePart)
            {
                if (*pChk == '.')
                    break;
                pChk--;
            }
            if (*pChk != '.')
            {
                //
                // file spec does not end in an extension. add xdl and try again
                //
                specLen = (UINT32)(apArgs->mpNamePart - apArgs->mpPath) + apArgs->mNameLen;
                pTemp = K2OS_Heap_Alloc(specLen + 8);
                if (NULL == pTemp)
                {
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                }
                else
                {
                    K2ASC_CopyLen(pTemp, apArgs->mpPath, specLen);
                    K2ASC_Copy(pTemp + specLen, ".xdl");

                    pHostFile->mFile = K2OS_FsClient_OpenFile(
                        gData.Xdl.mFsClient,
                        pTemp,
                        K2OS_ACCESS_R,
                        K2OS_ACCESS_R,
                        K2OS_FileOpen_Existing,
                        0, 0
                    );

                    K2OS_Heap_Free(pTemp);

                    if (NULL == pHostFile->mFile)
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(stat));
                    }
                    else
                    {
                        stat = K2STAT_NO_ERROR;
                    }
                }
            }
        }

        if (!K2STAT_IS_ERROR(stat))
        {
            //
            // file is open
            //
//            K2OSKERN_Debug("File is open\n");
//            KernFsNode_Dump();
            pHostFile->mKeepSymbols = TRUE;

            *apRetHostFile = (K2XDL_HOST_FILE)pHostFile;
            *apRetModuleDataAddr = xdlPageVirt;
            *apRetModuleLinkAddr = xdlPageVirt;

            pHostFile->Track.mpXdl = (XDL *)xdlPageVirt;

            stat = K2STAT_NO_ERROR;
        }

        if (K2STAT_IS_ERROR(stat))
        {
            KernXdl_Threaded_FreeSegment(xdlPageVirt);
        }
    }
    else
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pHostFile);
    }

    return stat;
}

K2STAT
KernXdlHost_Threaded_ReadSectors(
    K2XDL_LOADCTX const *   apLoadCtx,
    void *                  apBuffer,
    UINT64 const *          apSectorCount
)
{
    K2OSKERN_HOST_FILE *pHostFile;
    UINT32              sectorCount;
    UINT32              red;
    K2STAT              stat;

    pHostFile = (K2OSKERN_HOST_FILE *)apLoadCtx->mHostFile;

    sectorCount = (UINT32)(*apSectorCount);

    red = 0;
    if (!K2OS_File_Read(pHostFile->mFile, apBuffer, sectorCount * XDL_SECTOR_BYTES, &red))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
    }
    else
    {
        stat = K2STAT_NO_ERROR;
    }

    return stat;
}

K2STAT
KernXdlHost_Threaded_Prepare(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aKeepSymbols,
    XDL_FILE_HEADER const * apFileHdr,
    K2XDL_SEGMENT_ADDRS *   apRetSegAddrs
)
{
    K2STAT              stat;
    UINT32              ix;
    UINT32              segPages;
    UINT32              segAddr;
    BOOL                ok;
    K2OSKERN_HOST_FILE *pHostFile;

    if (0 == (apFileHdr->mFlags & XDL_FILE_HEADER_FLAG_KERNEL_ONLY))
    {
        K2OSKERN_Debug("*** Attempt to load non-kernel XDL into kernel\n");
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    stat = K2STAT_NO_ERROR;

    pHostFile = (K2OSKERN_HOST_FILE *)apLoadCtx->mHostFile;

    K2MEM_Zero(apRetSegAddrs, sizeof(K2XDL_SEGMENT_ADDRS));

    for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Count; ix++)
    {
        segPages = (UINT32)apFileHdr->Segment[ix].mMemActualBytes;
        segPages = (segPages + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;
        if (0 != segPages)
        {
            segAddr = KernXdl_Threaded_CreateRwSegment(segPages, &pHostFile->Track.SegVirtMapRef[ix]);
            if (0 == segAddr)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
            }
            else
            {
                pHostFile->Track.SegVirtMapRef[ix].AsVirtMap->Kern.mSegType = KernSeg_Xdl_Part;
                pHostFile->Track.SegVirtMapRef[ix].AsVirtMap->Kern.mSizeBytes = apFileHdr->Segment[ix].mMemActualBytes;
                pHostFile->Track.SegVirtMapRef[ix].AsVirtMap->Kern.XdlPart.mpOwnerTrack = &pHostFile->Track;
                pHostFile->Track.SegVirtMapRef[ix].AsVirtMap->Kern.XdlPart.mXdlSegmentIx = ix;
            }
            apRetSegAddrs->mData[ix] = apRetSegAddrs->mLink[ix] = segAddr;
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        if (ix > 0)
        {
            do
            {
                --ix;
                ok = KernXdl_Threaded_FreeSegment(apRetSegAddrs->mData[ix]);
                K2_ASSERT(ok);
            } while (ix > 0);
        }
    }
    else
    {
        pHostFile->mKeepSymbols = aKeepSymbols;
        K2MEM_Copy(&pHostFile->ID, &apFileHdr->Id, sizeof(K2_GUID128));
    }

    return stat;
}

BOOL
KernXdlHost_Threaded_PreCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aIsLoad,
    XDL *                   apXdl
)
{
//    K2OSKERN_Debug("%.*s: %s\n", apLoadCtx->OpenArgs.mNameLen, apLoadCtx->OpenArgs.mpNamePart, aIsLoad ? "LOAD" : "UNLOAD");
    // dont care. keep going
    return TRUE;
}

void
KernXdlHost_Threaded_PostCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    K2STAT                  aUserStatus,
    XDL *                   apXdl
)
{
//    K2OSKERN_Debug("%.*s: STATUS 0x%08X\n", apLoadCtx->OpenArgs.mNameLen, apLoadCtx->OpenArgs.mpNamePart, aUserStatus);
    // dont care
}

K2STAT
KernXdlHost_Threaded_Finalize(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aKeepSymbols,
    XDL_FILE_HEADER const * apFileHdr,
    K2XDL_SEGMENT_ADDRS *   apUpdateSegAddrs
)
{
    K2OSKERN_HOST_FILE *pHostFile;
    UINT32              segAddr;
    BOOL                ok;

    pHostFile = (K2OSKERN_HOST_FILE *)apLoadCtx->mHostFile;

    //
    // purge the relocations
    //
    segAddr = (UINT32)apUpdateSegAddrs->mData[XDLSegmentIx_Relocs];
    if (0 != segAddr)
    {
        ok = KernXdl_Threaded_FreeSegment(segAddr);
        K2_ASSERT(ok);
        apUpdateSegAddrs->mData[XDLSegmentIx_Relocs] = 0;
        apUpdateSegAddrs->mLink[XDLSegmentIx_Relocs] = 0;
    }

    //
    // remap text as executable read-only
    //
    K2OS_CacheOperation(K2OS_CACHEOP_FlushDataRange, apUpdateSegAddrs->mData[XDLSegmentIx_Text], apFileHdr->Segment[XDLSegmentIx_Text].mMemActualBytes);
    ok = KernXdl_Threaded_RemapSegment(apUpdateSegAddrs->mData[XDLSegmentIx_Text], K2OS_MapType_Text);
    K2_ASSERT(ok);
    K2OS_CacheOperation(K2OS_CACHEOP_InvalidateInstructionRange, apUpdateSegAddrs->mData[XDLSegmentIx_Text], apFileHdr->Segment[XDLSegmentIx_Text].mMemActualBytes);

    //
    // remap read as read-only
    //
    ok = KernXdl_Threaded_RemapSegment(apUpdateSegAddrs->mData[XDLSegmentIx_Read], K2OS_MapType_Data_ReadOnly);
    K2_ASSERT(ok);

    //
    // get rid of symbols or remap them as read-only
    //
    segAddr = (UINT32)apUpdateSegAddrs->mData[XDLSegmentIx_Symbols];
    if (0 != segAddr)
    {
        if (!pHostFile->mKeepSymbols)
        {
            ok = KernXdl_Threaded_FreeSegment(segAddr);
            K2_ASSERT(ok);
            apUpdateSegAddrs->mData[XDLSegmentIx_Symbols] = 0;
            apUpdateSegAddrs->mLink[XDLSegmentIx_Symbols] = 0;
        }
        else
        {
            ok = KernXdl_Threaded_RemapSegment(apUpdateSegAddrs->mData[XDLSegmentIx_Symbols], K2OS_MapType_Data_ReadOnly);
            K2_ASSERT(ok);
        }
    }

    K2OSKERN_Debug("KERNXDL: Loaded [%.*s]\n", apFileHdr->mNameLen, apFileHdr->mName);
//    K2OSKERN_Debug("   Text: %08X %08X\n", (UINT32)apUpdateSegAddrs->mData[XDLSegmentIx_Text], (UINT32)apFileHdr->Segment[XDLSegmentIx_Text].mMemActualBytes);
//    K2OSKERN_Debug("   Read: %08X %08X\n", (UINT32)apUpdateSegAddrs->mData[XDLSegmentIx_Read], (UINT32)apFileHdr->Segment[XDLSegmentIx_Read].mMemActualBytes);
//    K2OSKERN_Debug("   Data: %08X %08X\n", (UINT32)apUpdateSegAddrs->mData[XDLSegmentIx_Data], (UINT32)apFileHdr->Segment[XDLSegmentIx_Data].mMemActualBytes);

    return K2STAT_NO_ERROR;
}

K2STAT
KernXdlHost_Threaded_Purge(
    K2XDL_LOADCTX const *       apLoadCtx,
    K2XDL_SEGMENT_ADDRS const * apSegAddrs
)
{
    K2OSKERN_HOST_FILE *pHostFile;
    UINT32              ix;
    UINT32              segAddr;
    BOOL                ok;

    pHostFile = (K2OSKERN_HOST_FILE *)apLoadCtx->mHostFile;

    K2OSKERN_Debug("KERNXDL: Unload [%.*s]\n", apLoadCtx->OpenArgs.mNameLen, apLoadCtx->OpenArgs.mpNamePart);

    for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Count; ix++)
    {
        segAddr = (UINT32)apSegAddrs->mData[ix];
        if (0 != segAddr)
        {
            ok = KernXdl_Threaded_FreeSegment(segAddr);
            K2_ASSERT(ok);
        }
    }

    ok = KernXdl_Threaded_FreeSegment(pHostFile->RefXdlPageVirtMap.AsVirtMap->OwnerMapTreeNode.mUserVal);
    K2_ASSERT(ok);

    K2OS_File_Close(pHostFile->mFile);

    ok = K2OS_Heap_Free(pHostFile);
    K2_ASSERT(ok);

    return K2STAT_NO_ERROR;
}

void
KernXdl_Threaded_Init(
    void
)
{
    K2STAT  stat;
    BOOL    ok;

    ok = K2OS_CritSec_Init(&gData.Xdl.Sec);
    K2_ASSERT(ok);

    gData.Xdl.Host.CritSec = KernXdlHost_Threaded_CritSec;
    gData.Xdl.Host.Open = KernXdlHost_Threaded_Open;
    gData.Xdl.Host.ReadSectors = KernXdlHost_Threaded_ReadSectors;
    gData.Xdl.Host.Prepare = KernXdlHost_Threaded_Prepare;
    gData.Xdl.Host.PreCallback = KernXdlHost_Threaded_PreCallback;
    gData.Xdl.Host.PostCallback = KernXdlHost_Threaded_PostCallback;
    gData.Xdl.Host.Finalize = KernXdlHost_Threaded_Finalize;
    gData.Xdl.Host.Purge = KernXdlHost_Threaded_Purge;
    gData.Xdl.Host.AtReInit = NULL;

    stat = K2XDL_Init((void *)K2OS_KVA_LOADERPAGE_BASE, &gData.Xdl.Host, TRUE, TRUE);

    K2_ASSERT(!K2STAT_IS_ERROR(stat));
}

#define DIAG_LOAD 0

K2STAT
KernXdl_Threaded_Acquire(
    char const *    apFilePath,
    XDL **          appRetXdl,
    UINT32 *        apRetEntryStackReq,
    K2_GUID128 *    apRetID
)
{
    K2STAT                  stat;
    BOOL                    ok;
    XDL_FILE_HEADER const * pHeader;
#if DIAG_LOAD
    XDL *                   pXdl;
    K2LIST_LINK *           pListLink;
#endif

    if (NULL == gData.Xdl.mFsClient)
    {
        K2OS_CritSec_Enter(&gData.Xdl.Sec);
        if (NULL == gData.Xdl.mFsClient)
        {
            gData.Xdl.mFsClient = K2OS_FsClient_Create();
            K2_ASSERT(NULL != gData.Xdl.mFsClient);
            ok = K2OS_FsClient_SetBaseDir(gData.Xdl.mFsClient, "/fs/0/kern");
            K2_ASSERT(ok);
//            K2OSKERN_Debug("kernel xdl load path is open\n");
        }
        K2OS_CritSec_Leave(&gData.Xdl.Sec);
    }

    stat = XDL_Acquire(apFilePath, 0, appRetXdl);

    if (!K2STAT_IS_ERROR(stat))
    {
        pHeader = (*appRetXdl)->mpHeader;

#if DIAG_LOAD
        K2OSKERN_Debug("\nKernel loaded \"%.*s\" @ %08X, &hdr = %08X, hdr = %08X\n",
            pHeader->mNameLen, pHeader->mName,
            (*appRetXdl),
            &((*appRetXdl)->mpHeader),
            pHeader);

        K2OSKERN_Debug("anchor head = %08X\n", gData.Xdl.KernLoadedList.mpHead);
        K2OSKERN_Debug("anchor tail = %08X\n", gData.Xdl.KernLoadedList.mpTail);
        pListLink = gData.Xdl.KernLoadedList.mpHead;
        do {
            pXdl = K2_GET_CONTAINER(XDL, pListLink, ListLink);
            K2OSKERN_Debug("  %08X : %08X %08X hdrptr %08X\n", pXdl, pXdl->ListLink.mpPrev, pXdl->ListLink.mpNext, pXdl->mpHeader);
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
        K2OSKERN_Debug("\n");
#endif

        if (NULL != apRetEntryStackReq)
            *apRetEntryStackReq = pHeader->mEntryStackReq;

        if (NULL != apRetID)
            K2MEM_Copy(apRetID, &pHeader->Id, sizeof(K2_GUID128));
    }
    else
    {
        K2OS_Thread_SetLastStatus(stat);
    }

    return stat;
}

K2OS_XDL
K2OS_Xdl_Acquire(
    char const *apFilePath
)
{
    K2STAT  stat;
    XDL *   pXdl;

    pXdl = NULL;

    stat = KernXdl_Threaded_Acquire(apFilePath, &pXdl, NULL, NULL);

    if (K2STAT_IS_ERROR(stat))
        return NULL;

    K2_ASSERT(NULL != pXdl);

    return (K2OS_XDL)pXdl;
}
