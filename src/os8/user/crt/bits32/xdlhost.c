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
#include "../../../../shared/lib/k2xdl/xdl_struct.h"

static K2XDL_HOST       sgXdlHost;
static K2ROFS const *   sgpROFS;
static K2OS_CRITSEC     sgXdlSec;

typedef struct _CRT_HOST_FILE CRT_HOST_FILE;
struct _CRT_HOST_FILE
{
    BOOL                mIsRofs;

    UINT32 volatile     mCurSector;
    UINT32              mXdlSegAddr;
    BOOL                mKeepSymbols;
    K2_GUID128          ID;
    UINT32              mEntryStackReq;

    K2ROFS_FILE const * mpRofsFile;
};

K2STAT 
CrtXdl_CritSec(
    BOOL aEnter
)
{
    if (aEnter)
        K2OS_CritSec_Enter(&sgXdlSec);
    else
        K2OS_CritSec_Leave(&sgXdlSec);
    return K2STAT_NO_ERROR;
}

K2STAT 
CrtXdl_Open(
    K2XDL_OPENARGS const *  apArgs,
    K2XDL_HOST_FILE *       apRetHostFile,
    UINT_PTR *              apRetModuleDataAddr,
    UINT_PTR *              apRetModuleLinkAddr
)
{
    K2ROFS_FILE const * pFile;
    UINT32              segAddr;
    CRT_HOST_FILE *     pCrtHostFile;
    char const *        pChk;
    UINT_PTR            specLen;
    char *              pTemp;

    if (0 == apArgs->mAcqContext)
    {
        if (apArgs->mNameLen == 0)
            return K2STAT_ERROR_BAD_ARGUMENT;

        //
        // file comes from internal built-in filesystem
        //
        pFile = K2ROFSHELP_SubFile(sgpROFS, K2ROFS_ROOTDIR(sgpROFS), apArgs->mpPath);
        if (NULL == pFile)
        {
            pChk = apArgs->mpNamePart + apArgs->mNameLen - 1;
            while (pChk != apArgs->mpNamePart)
            {
                if (*pChk == '.')
                    break;
                pChk--;
            }
            if (*pChk == '.')
            {
                return K2STAT_ERROR_NOT_FOUND;
            }

            //
            // file spec does not end in an extension. add dlx and try again
            //
            specLen = (UINT_PTR)(apArgs->mpNamePart - apArgs->mpPath) + apArgs->mNameLen;
            pTemp = K2OS_Heap_Alloc(specLen + 8);
            if (NULL == pTemp)
            {
                return K2STAT_ERROR_OUT_OF_MEMORY;
            }
            K2ASC_CopyLen(pTemp, apArgs->mpPath, specLen);
            K2ASC_Copy(pTemp + specLen, ".xdl");
            pFile = K2ROFSHELP_SubFile(sgpROFS, K2ROFS_ROOTDIR(sgpROFS), pTemp);
            K2OS_Heap_Free(pTemp);
            if (NULL == pFile)
            {
                return K2STAT_ERROR_NOT_FOUND;
            }
        }

        segAddr = CrtMem_CreateRwSegment(1);
        if (0 == segAddr)
        {
            return K2OS_Thread_GetLastStatus();
        }

        pCrtHostFile = (CRT_HOST_FILE *)K2OS_Heap_Alloc(sizeof(CRT_HOST_FILE));
        if (NULL == pCrtHostFile)
        {
            CrtMem_FreeSegment(segAddr, FALSE);
            return K2STAT_ERROR_OUT_OF_MEMORY;
        }

        pCrtHostFile->mIsRofs = TRUE;
        pCrtHostFile->mXdlSegAddr = segAddr;
        pCrtHostFile->mpRofsFile = pFile;
        pCrtHostFile->mCurSector = 0;
        pCrtHostFile->mKeepSymbols = FALSE;

        *apRetHostFile = (K2XDL_HOST_FILE)pCrtHostFile;
        *apRetModuleDataAddr = segAddr;
        *apRetModuleLinkAddr = segAddr;

        return K2STAT_NO_ERROR;
    }

    //
    // using real file system
    //

    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
CrtXdl_ResizeCopyModulePage(
    K2XDL_LOADCTX const *   apLoadCtx,
    UINT_PTR                aNewPageCount,
    UINT_PTR *              apModuleDataAddr,
    UINT_PTR *              apModuleLinkAddr
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
CrtXdl_ReadSectors(
    K2XDL_LOADCTX const *   apLoadCtx,
    void *                  apBuffer,
    UINT64 const *          apSectorCount
)
{
    K2ROFS_FILE const * pFile;
    UINT8 const *       pFileSectors;
    CRT_HOST_FILE *     pCrtHostFile;
    UINT32              curSector;
    UINT32              sectorCount;

    pCrtHostFile = (CRT_HOST_FILE *)apLoadCtx->mHostFile;

    sectorCount = (UINT_PTR)(*apSectorCount);

    if (pCrtHostFile->mIsRofs)
    {
        pFile = pCrtHostFile->mpRofsFile;

        pFileSectors = K2ROFS_FILEDATA(sgpROFS, pFile);

        do
        {
            curSector = pCrtHostFile->mCurSector;
            K2_CpuReadBarrier();

            if (K2ROFS_FILESECTORS(pFile) < curSector + sectorCount)
                return K2STAT_ERROR_OUT_OF_BOUNDS;

        } while (curSector != K2ATOMIC_CompareExchange(&pCrtHostFile->mCurSector, curSector + sectorCount, curSector));

        K2MEM_Copy(
            apBuffer,
            pFileSectors + (K2ROFS_SECTOR_BYTES * curSector),
            K2ROFS_SECTOR_BYTES * sectorCount
        );

        return K2STAT_NO_ERROR;
    }

    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
CrtXdl_Prepare(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aKeepSymbols,
    XDL_FILE_HEADER const * apFileHdr,
    K2XDL_SEGMENT_ADDRS *   apRetSegAddrs
)
{
    K2STAT          stat;
    UINT32          ix;
    UINT32          segPages;
    UINT32          segAddr;
    BOOL            ok;
    CRT_HOST_FILE * pCrtHostFile;

    stat = K2STAT_NO_ERROR;

    pCrtHostFile = (CRT_HOST_FILE *)apLoadCtx->mHostFile;

    K2MEM_Zero(apRetSegAddrs, sizeof(K2XDL_SEGMENT_ADDRS));

    for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Count; ix++)
    {
        segPages = (UINT_PTR)apFileHdr->Segment[ix].mMemActualBytes;
        segPages = (segPages + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;
        if (0 != segPages)
        {
            segAddr = CrtMem_CreateRwSegment(segPages);
            if (0 == segAddr)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
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
                ok = CrtMem_FreeSegment(apRetSegAddrs->mData[ix], FALSE);
                K2_ASSERT(ok);
            } while (ix > 0);
        }
    }
    else
    {
        pCrtHostFile->mKeepSymbols = aKeepSymbols;
        K2MEM_Copy(&pCrtHostFile->ID, &apFileHdr->Id, sizeof(K2_GUID128));
        pCrtHostFile->mEntryStackReq = apFileHdr->mEntryStackReq;
    }

    return stat;
}

BOOL   
CrtXdl_PreCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aIsLoad,
    XDL *                   apXdl
)
{
    // execute the callback
    return TRUE;
}

void   
CrtXdl_PostCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    K2STAT                  aUserStatus,
    XDL *                   apXdl
)
{
    // dont care
}

K2STAT 
CrtXdl_Finalize(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aKeepSymbols,
    XDL_FILE_HEADER const * apFileHdr,
    K2XDL_SEGMENT_ADDRS *   apUpdateSegAddrs
)
{
    CRT_HOST_FILE * pCrtHostFile;
    UINT32          segAddr;
    BOOL            ok;

    pCrtHostFile = (CRT_HOST_FILE *)apLoadCtx->mHostFile;

    //
    // purge the relocations
    //
    segAddr = (UINT32)apUpdateSegAddrs->mData[XDLSegmentIx_Relocs];
    if (0 != segAddr)
    {
        ok = CrtMem_FreeSegment(segAddr, FALSE);
        K2_ASSERT(ok);
        apUpdateSegAddrs->mData[XDLSegmentIx_Relocs] = 0;
        apUpdateSegAddrs->mLink[XDLSegmentIx_Relocs] = 0;
    }

    //
    // remap text as executable read-only
    //
    ok = CrtMem_RemapSegment(apUpdateSegAddrs->mData[XDLSegmentIx_Text], K2OS_MapType_Text);
    K2_ASSERT(ok);

    //
    // remap read as read-only
    //
    ok = CrtMem_RemapSegment(apUpdateSegAddrs->mData[XDLSegmentIx_Read], K2OS_MapType_Data_ReadOnly);
    K2_ASSERT(ok);

    //
    // get rid of symbols or remap them as read-only
    //
    segAddr = (UINT32)apUpdateSegAddrs->mData[XDLSegmentIx_Symbols];
    if (0 != segAddr)
    {
        if (!pCrtHostFile->mKeepSymbols)
        {
            ok = CrtMem_FreeSegment(segAddr, FALSE);
            K2_ASSERT(ok);
            apUpdateSegAddrs->mData[XDLSegmentIx_Symbols] = 0;
            apUpdateSegAddrs->mLink[XDLSegmentIx_Symbols] = 0;
        }
        else
        {
            ok = CrtMem_RemapSegment(apUpdateSegAddrs->mData[XDLSegmentIx_Symbols], K2OS_MapType_Data_ReadOnly);
            K2_ASSERT(ok);
        }
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
CrtXdl_Purge(
    K2XDL_LOADCTX const *       apLoadCtx,
    K2XDL_SEGMENT_ADDRS const * apSegAddrs
)
{
    CRT_HOST_FILE * pCrtHostFile;
    UINT32          ix;
    UINT32          segAddr;
    BOOL            ok;

    pCrtHostFile = (CRT_HOST_FILE *)apLoadCtx->mHostFile;

    for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Count; ix++)
    {
        segAddr = (UINT_PTR)apSegAddrs->mData[ix];
        if (0 != segAddr)
        {
            ok = CrtMem_FreeSegment(segAddr, FALSE);
            K2_ASSERT(ok);
        }
    }

    ok = CrtMem_FreeSegment(pCrtHostFile->mXdlSegAddr, FALSE);
    K2_ASSERT(ok);

    K2OS_Heap_Free(pCrtHostFile);
    K2_ASSERT(ok);

    return K2STAT_NO_ERROR;
}

void 
CrtXdl_Init(
    K2ROFS const * apROFS
)
{
    K2STAT              stat;
    K2ROFS_FILE const * pFile;
    BOOL                ok;

    ok = K2OS_CritSec_Init(&sgXdlSec);
    K2_ASSERT(ok);
    if (!ok)
    {
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    sgpROFS = apROFS;

    pFile = K2ROFSHELP_SubFile(sgpROFS, K2ROFS_ROOTDIR(sgpROFS), "k2oscrt.xdl");
    K2_ASSERT(NULL != pFile);

    K2MEM_Zero(&sgXdlHost, sizeof(sgXdlHost));

    sgXdlHost.CritSec = CrtXdl_CritSec;
    sgXdlHost.Open = CrtXdl_Open;
    sgXdlHost.ResizeCopyModulePage = CrtXdl_ResizeCopyModulePage;
    sgXdlHost.ReadSectors = CrtXdl_ReadSectors;
    sgXdlHost.Prepare = CrtXdl_Prepare;
    sgXdlHost.PreCallback = CrtXdl_PreCallback;
    sgXdlHost.PostCallback = CrtXdl_PostCallback;
    sgXdlHost.Finalize = CrtXdl_Finalize;
    sgXdlHost.Purge = CrtXdl_Purge;

    stat = K2XDL_InitSelf(
        (void *)K2OS_UVA_XDL_GLOBALS,
        (void *)K2OS_UVA_XDL_CRTPAGE,
        (XDL_FILE_HEADER const *)K2OS_UVA_CRTXDL_LOADPOINT,
        &sgXdlHost,
        TRUE);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    //
    // tell the kernel we have initialized xdl so it can find things for debugging 
    //
    CrtKern_SysCall1(K2OS_SYSCALL_ID_CRT_INITXDL, K2OS_UVA_XDL_GLOBALS);
}

K2STAT  
CrtXdl_Acquire(
    char const *    apFilePath,
    XDL **          appRetXdl,
    UINT32 *        apRetEntryStackReq,
    K2_GUID128 *    apRetID
)
{
    K2STAT                  stat;
    char                    chk;
    UINT_PTR                devPathLen;
    char const *            pScan;
    UINT_PTR                acqContext;
    XDL_FILE_HEADER const * pHeader;

    //
    // split path, find/acquire filesystem, use that as second arg
    //
    acqContext = 0;
    pScan = apFilePath - 1;
    do {
        pScan++;
        chk = *pScan;
        if (chk == ':')
            break;
    } while (chk != 0);
    if (chk != 0)
    {
        // chk points to colon.  file path is after that
        devPathLen = (UINT_PTR)(pScan - apFilePath);
        pScan++;
        // empty before colon is same as no colon at all
        if (devPathLen != 0)
        {
            // device specified in path that is 'devPathLen' in characters long
            K2_ASSERT(0);
        }
    }
    else
    {
        pScan = apFilePath;
    }

    stat = XDL_Acquire(pScan, acqContext, appRetXdl);

    if (!K2STAT_IS_ERROR(stat))
    {
        pHeader = (*appRetXdl)->mpHeader;

#if 0
        CrtDbg_Printf("\nProcess %d Loaded \"%.*s\" @ %08X, &hdr = %08X, hdr = %08X\n",
            gProcessId,
            pHeader->mNameLen, pHeader->mName,
            (*appRetXdl),
            &((*appRetXdl)->mpHeader),
            pHeader);

        XDL *                   pXdl;
        K2LIST_ANCHOR *         pList;
        K2LIST_LINK *           pListLink;

        pList = (K2LIST_ANCHOR *)K2OS_UVA_XDL_GLOBALS;
//        CrtDbg_Printf("anchor head = %08X\n", pList->mpHead);
//        CrtDbg_Printf("anchor tail = %08X\n", pList->mpTail);
        pListLink = pList->mpHead;
        do {
            pXdl = K2_GET_CONTAINER(XDL, pListLink, ListLink);
//            CrtDbg_Printf("  %08X : %08X %08X hdrptr %08X\n", pXdl, pXdl->ListLink.mpPrev, pXdl->ListLink.mpNext, pXdl->mpHeader);
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
        CrtDbg_Printf("\n");
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
K2_CALLCONV_REGS 
K2OS_Xdl_Acquire(
    char const *apFilePath
)
{
    K2STAT  stat;
    XDL *   pXdl;

    pXdl = NULL;

    stat = CrtXdl_Acquire(apFilePath, &pXdl, NULL, NULL);

    if (K2STAT_IS_ERROR(stat))
        return NULL;

    K2_ASSERT(NULL != pXdl);

    return pXdl;
}

BOOL
K2_CALLCONV_REGS 
K2OS_Xdl_AddRef(
    K2OS_XDL aXdl
)
{
    K2STAT  stat;

    stat = XDL_AddRef((XDL *)aXdl);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

K2OS_XDL    
K2_CALLCONV_REGS 
K2OS_Xdl_AddRefContaining(
    UINT32 aAddr
)
{
    K2STAT  stat;
    UINT32  segIx;
    XDL *   pXdl;

    stat = XDL_AcquireContaining(aAddr, &pXdl, &segIx);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    return pXdl;
}

BOOL
K2_CALLCONV_REGS 
K2OS_Xdl_Release(
    K2OS_XDL aXdl
)
{
    K2STAT stat;

    stat = XDL_Release((XDL *)aXdl);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

UINT32
K2OS_Xdl_FindExport(
    K2OS_XDL    aXdl,
    BOOL        aIsText,
    char const *apExportName
)
{
    K2STAT stat;
    UINT32 addr;

    addr = 0;

    stat = XDL_FindExport((XDL *)aXdl, aIsText ? XDLProgData_Text : XDLProgData_Data, apExportName, &addr);
    if (!K2STAT_IS_ERROR(stat))
        return addr;

    if (!aIsText)
    {
        stat = XDL_FindExport((XDL *)aXdl, XDLProgData_Read, apExportName, &addr);
        if (!K2STAT_IS_ERROR(stat))
            return addr;
    }

    K2OS_Thread_SetLastStatus(stat);

    return 0;
}

BOOL        
K2OS_Xdl_GetIdent(
    K2OS_XDL        aXdl,
    char *          apNameBuf,
    UINT32          aNameBufLen,
    K2_GUID128  *   apRetID
)
{
    K2STAT                  stat;
    XDL_FILE_HEADER const * pHeader;

    if (!K2OS_Xdl_AddRef(aXdl))
        return FALSE;

    stat = XDL_GetHeaderPtr((XDL *)aXdl, &pHeader);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        K2OS_Xdl_Release(aXdl);
        return FALSE;
    }

    if (NULL != apNameBuf)
    {
        K2_ASSERT(0 != aNameBufLen);
        if (aNameBufLen > pHeader->mNameLen)
            aNameBufLen = pHeader->mNameLen;
        K2MEM_Copy(apNameBuf, pHeader->mName, aNameBufLen);
    }

    if (NULL != apRetID)
    {
        K2MEM_Copy(apRetID, &pHeader->Id, sizeof(K2_GUID128));
    }
    
    return TRUE;
}
