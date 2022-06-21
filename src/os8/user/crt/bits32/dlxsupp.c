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

static UINT8            sgPageBuf[3 * K2_VA_MEMPAGE_BYTES];
static UINT8 *          sgpLoaderPage;
static K2DLXSUPP_HOST   sgDlxHost;
static K2ROFS const *   sgpROFS;
static K2OS_CRITSEC     sgDlxSec;

typedef struct _CRT_HOST_FILE CRT_HOST_FILE;
struct _CRT_HOST_FILE
{
    BOOL                mIsRofs;

    UINT32 volatile     mCurSector;
    UINT32              mDlxSegAddr;
    BOOL                mKeepSymbols;
    UINT32              mSegAddr[DlxSeg_Count];
    K2_GUID128          ID;
    UINT32              mEntryStackReq;

    K2ROFS_FILE const * mpRofsFile;
};

K2STAT 
CrtDlx_CritSec(
    BOOL aEnter
)
{
    if (aEnter)
        K2OS_CritSec_Enter(&sgDlxSec);
    else
        K2OS_CritSec_Leave(&sgDlxSec);
    return K2STAT_NO_ERROR;
}

K2STAT 
CrtDlx_AcqAlreadyLoaded(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
CrtDlx_Open(
    void *                  apAcqContext,
    char const *            apFileSpec,
    char const *            apNamePart,
    UINT32                  aNamePartLen,
    K2DLXSUPP_OPENRESULT *  apRetResult
)
{
    K2ROFS_FILE const * pFile;
    UINT32              segAddr;
    CRT_HOST_FILE *     pCrtHostFile;
    char const *        pChk;
    UINT_PTR            specLen;
    char *              pTemp;

//    CrtDbg_Printf("CrtDlx_Open(%08X, %s)\n", apAcqContext, apFileSpec);

    if (NULL == apAcqContext)
    {
        if (aNamePartLen == 0)
            return K2STAT_ERROR_BAD_ARGUMENT;

        //
        // file comes from internal built-in filesystem
        //
        pFile = K2ROFSHELP_SubFile(sgpROFS, K2ROFS_ROOTDIR(sgpROFS), apFileSpec);
        if (NULL == pFile)
        {
            pChk = apNamePart + aNamePartLen - 1;
            while (pChk != apNamePart)
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
            specLen = (UINT_PTR)(apNamePart - apFileSpec) + aNamePartLen;
            pTemp = K2OS_Heap_Alloc(specLen + 8);
            if (NULL == pTemp)
            {
                return K2STAT_ERROR_OUT_OF_MEMORY;
            }
            K2ASC_CopyLen(pTemp, apFileSpec, specLen);
            K2ASC_Copy(pTemp + specLen, ".dlx");
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
        pCrtHostFile->mDlxSegAddr = segAddr;
        pCrtHostFile->mpRofsFile = pFile;
        pCrtHostFile->mCurSector = 0;
        pCrtHostFile->mKeepSymbols = FALSE;
        K2MEM_Zero(pCrtHostFile->mSegAddr, sizeof(UINT32) * DlxSeg_Count);

        apRetResult->mHostFile = (K2DLXSUPP_HOST_FILE)pCrtHostFile;
        apRetResult->mFileSectorCount = K2ROFS_FILESECTORS(pFile);
        apRetResult->mModulePageDataAddr = segAddr;
        apRetResult->mModulePageLinkAddr = segAddr;

//        CrtDbg_Printf("CrtDlx_Open(%08X, %s) -> %08X\n", apAcqContext, apFileSpec, pCrtHostFile);

        return K2STAT_NO_ERROR;
    }

    //
    // using real file system
    //

    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
CrtDlx_ReadSectors(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile, 
    void *              apBuffer, 
    UINT32              aSectorCount
)
{
    K2ROFS_FILE const * pFile;
    UINT8 const *       pFileSectors;
    CRT_HOST_FILE *     pCrtHostFile;
    UINT32              curSector;

    pCrtHostFile = (CRT_HOST_FILE *)aHostFile;

    if (pCrtHostFile->mIsRofs)
    {
        pFile = pCrtHostFile->mpRofsFile;

        pFileSectors = K2ROFS_FILEDATA(sgpROFS, pFile);

        do
        {
            curSector = pCrtHostFile->mCurSector;
            K2_CpuReadBarrier();

            if (K2ROFS_FILESECTORS(pFile) < curSector + aSectorCount)
                return K2STAT_ERROR_OUT_OF_BOUNDS;

        } while (curSector != K2ATOMIC_CompareExchange(&pCrtHostFile->mCurSector, curSector + aSectorCount, curSector));

        K2MEM_Copy(
            apBuffer,
            pFileSectors + (K2ROFS_SECTOR_BYTES * curSector),
            K2ROFS_SECTOR_BYTES * aSectorCount
        );

        return K2STAT_NO_ERROR;
    }

    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
CrtDlx_Prepare(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile, 
    DLX_INFO *          apInfo, 
    UINT32              aInfoSize, 
    BOOL                aKeepSymbols, 
    K2DLXSUPP_SEGALLOC *apRetAlloc
)
{
    K2STAT          stat;
    UINT32          ix;
    UINT32          segPages;
    UINT32          segAddr;
    BOOL            ok;
    CRT_HOST_FILE * pCrtHostFile;

    stat = K2STAT_NO_ERROR;

    pCrtHostFile = (CRT_HOST_FILE *)aHostFile;

    K2MEM_Zero(apRetAlloc, sizeof(K2DLXSUPP_SEGALLOC));

    for (ix = DlxSeg_Text; ix < DlxSeg_Count; ix++)
    {
        segPages = apInfo->SegInfo[ix].mMemActualBytes;
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
//            CrtDbg_Printf("Segment %d - %08X, %d\n", ix, segAddr, segPages);
            pCrtHostFile->mSegAddr[ix] = segAddr;
            apRetAlloc->Segment[ix].mDataAddr = segAddr;
            apRetAlloc->Segment[ix].mLinkAddr = segAddr;
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        if (ix > 0)
        {
            do
            {
                --ix;
                ok = CrtMem_FreeSegment(apRetAlloc->Segment[ix].mDataAddr, FALSE);
                K2_ASSERT(ok);
            } while (ix > 0);
        }
    }
    else
    {
        pCrtHostFile->mKeepSymbols = aKeepSymbols;
        pCrtHostFile->ID = apInfo->ID;
        pCrtHostFile->mEntryStackReq = apInfo->mEntryStackReq;
    }

    return stat;
}

BOOL 
CrtDlx_PreCallback(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile, 
    BOOL                aIsLoad, 
    DLX *               apDlx
)
{
    return TRUE;
}

K2STAT 
CrtDlx_PostCallback(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile, 
    K2STAT              aUserStatus, 
    DLX *               apDlx
)
{
    return K2STAT_NO_ERROR;
}

K2STAT
CrtDlx_Finalize(
    void *              apAcqContext,
    K2DLXSUPP_HOST_FILE aHostFile,
    K2DLXSUPP_SEGALLOC *apUpdateAlloc
)
{
    CRT_HOST_FILE * pCrtHostFile;
    UINT32          segAddr;
    BOOL            ok;

    pCrtHostFile = (CRT_HOST_FILE *)aHostFile;

//    CrtDbg_Printf("Finalize(%08X)\n", aHostFile);

    //
    // purge the relocations
    //
    segAddr = pCrtHostFile->mSegAddr[DlxSeg_Reloc];
    if (0 != segAddr)
    {
        ok = CrtMem_FreeSegment(segAddr, FALSE);
        K2_ASSERT(ok);
        pCrtHostFile->mSegAddr[DlxSeg_Reloc] = 0;
        apUpdateAlloc->Segment[DlxSeg_Reloc].mDataAddr = 0;
        apUpdateAlloc->Segment[DlxSeg_Reloc].mLinkAddr = 0;
    }

    //
    // remap text as executable read-only
    //
    ok = CrtMem_RemapSegment(apUpdateAlloc->Segment[DlxSeg_Text].mDataAddr, K2OS_MapType_Text);
    K2_ASSERT(ok);

    //
    // remap read as read-only
    //
    ok = CrtMem_RemapSegment(apUpdateAlloc->Segment[DlxSeg_Read].mDataAddr, K2OS_MapType_Data_ReadOnly);
    K2_ASSERT(ok);

    //
    // get rid of symbols or remap them as read-only
    //
    segAddr = pCrtHostFile->mSegAddr[DlxSeg_Sym];
    if (0 != segAddr)
    {
        if (!pCrtHostFile->mKeepSymbols)
        {
            ok = CrtMem_FreeSegment(segAddr, FALSE);
            K2_ASSERT(ok);
            pCrtHostFile->mSegAddr[DlxSeg_Sym] = 0;
            apUpdateAlloc->Segment[DlxSeg_Sym].mDataAddr = 0;
            apUpdateAlloc->Segment[DlxSeg_Sym].mLinkAddr = 0;
        }
        else
        {
            ok = CrtMem_RemapSegment(apUpdateAlloc->Segment[DlxSeg_Sym].mDataAddr, K2OS_MapType_Data_ReadOnly);
            K2_ASSERT(ok);
        }
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
CrtDlx_RefChange(
    K2DLXSUPP_HOST_FILE aHostFile, 
    DLX *               apDlx, 
    INT32               aRefChange
)
{
    return K2STAT_NO_ERROR;
}

K2STAT 
CrtDlx_Purge(
    K2DLXSUPP_HOST_FILE aHostFile
)
{
    CRT_HOST_FILE * pCrtHostFile;
    UINT32          ix;
    UINT32          segAddr;
    BOOL            ok;

    pCrtHostFile = (CRT_HOST_FILE *)aHostFile;

    for (ix = DlxSeg_Text; ix < DlxSeg_Count; ix++)
    {
        segAddr = pCrtHostFile->mSegAddr[ix];
        if (0 != segAddr)
        {
            ok = CrtMem_FreeSegment(segAddr, FALSE);
            K2_ASSERT(ok);
        }
    }

    ok = CrtMem_FreeSegment(pCrtHostFile->mDlxSegAddr, FALSE);
    K2_ASSERT(ok);

    K2OS_Heap_Free(pCrtHostFile);
    K2_ASSERT(ok);

    return K2STAT_NO_ERROR;
}

K2STAT 
CrtDlx_ErrorPoint(
    char const *    apFile, 
    int             aLine, 
    K2STAT          aStatus
)
{
    if (aStatus == K2STAT_ERROR_NOT_FOUND)
    {
        if ((aLine == 188) &&
            (0 == K2ASC_CompIns(apFile, "dlx_acquire.c")))
            return aStatus;
    }
    CrtDbg_Printf("CRTDLX ERROR %s:%d (%08X)\n", apFile, aLine, aStatus);
    return aStatus;
}

void 
CrtDlx_Init(
    K2ROFS const * apROFS
)
{
    K2STAT              stat;
    K2DLXSUPP_PRELOAD   preloadSelf;
    K2ROFS_FILE const * pFile;
    BOOL                ok;

    ok = K2OS_CritSec_Init(&sgDlxSec);
    K2_ASSERT(ok);
    if (!ok)
    {
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    sgpROFS = apROFS;

    pFile = K2ROFSHELP_SubFile(sgpROFS, K2ROFS_ROOTDIR(sgpROFS), "k2oscrt.dlx");
    K2_ASSERT(NULL != pFile);

    sgpLoaderPage = (UINT8 *)((((UINT32)(&sgPageBuf[0])) + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK);

    preloadSelf.mpDlxPage = sgpLoaderPage + K2_VA_MEMPAGE_BYTES;
    preloadSelf.mpDlxFileData = K2ROFS_FILEDATA(sgpROFS, pFile);
    preloadSelf.mDlxFileSizeBytes = pFile->mSizeBytes;
    preloadSelf.mDlxBase = K2OS_UVA_LOW_BASE;
    preloadSelf.mpListAnchorOut = NULL;
    preloadSelf.mDlxMemEndOut = 0;

    K2MEM_Zero(&sgDlxHost, sizeof(sgDlxHost));
    sgDlxHost.mHostSizeBytes = sizeof(sgDlxHost);
    sgDlxHost.CritSec = CrtDlx_CritSec;
    sgDlxHost.AcqAlreadyLoaded = CrtDlx_AcqAlreadyLoaded;
    sgDlxHost.Open = CrtDlx_Open;
    sgDlxHost.ReadSectors = CrtDlx_ReadSectors;
    sgDlxHost.Prepare = CrtDlx_Prepare;
    sgDlxHost.PreCallback = CrtDlx_PreCallback;
    sgDlxHost.PostCallback = CrtDlx_PostCallback;
    sgDlxHost.Finalize = CrtDlx_Finalize;
    sgDlxHost.RefChange = CrtDlx_RefChange;
    sgDlxHost.Purge = CrtDlx_Purge;
    sgDlxHost.ErrorPoint = CrtDlx_ErrorPoint;

    stat = K2DLXSUPP_Init(sgpLoaderPage, &sgDlxHost, TRUE, FALSE, &preloadSelf);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    //
    // tell the kernel where everything is for us for debugging purposes
    //
    CrtKern_SysCall1(K2OS_SYSCALL_ID_CRT_INITDLX, (UINT_PTR)preloadSelf.mpListAnchorOut);
}

K2STAT  
CrtDlx_Acquire(
    char const *    apFilePath,
    DLX **          appRetDlx,
    UINT32 *        apRetEntryStackReq,
    K2_GUID128 *    apRetID
)
{
    K2STAT          stat;
    char            chk;
    UINT_PTR        devPathLen;
    char const *    pScan;
    void *          pAcqContext;

    //
    // split path, find/acquire filesystem, use that as second arg
    //
    pAcqContext = NULL;
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

    stat = DLX_Acquire(pScan, pAcqContext, appRetDlx, apRetEntryStackReq, apRetID);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
    }

    return stat;
}

K2OS_DLX    
K2_CALLCONV_REGS 
K2OS_Dlx_Acquire(
    char const *apFilePath
)
{
    K2STAT  stat;
    DLX *   pDlx;

    pDlx = NULL;

    stat = CrtDlx_Acquire(apFilePath, &pDlx, NULL, NULL);

    if (K2STAT_IS_ERROR(stat))
        return NULL;

    K2_ASSERT(NULL != pDlx);

    return pDlx;
}

BOOL
K2_CALLCONV_REGS 
K2OS_Dlx_AddRef(
    K2OS_DLX aDlx
)
{
    K2STAT  stat;

    stat = DLX_AddRef(aDlx);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

K2OS_DLX    
K2_CALLCONV_REGS 
K2OS_Dlx_AddRefContaining(
    UINT32 aAddr
)
{
    K2STAT  stat;
    UINT32  segIx;
    DLX *   pDlx;

    stat = DLX_AcquireContaining(aAddr, &pDlx, &segIx);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    return pDlx;
}

BOOL
K2_CALLCONV_REGS 
K2OS_Dlx_Release(
    K2OS_DLX aDlx
)
{
    K2STAT stat;

    stat = DLX_Release(aDlx);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

UINT32
K2OS_Dlx_FindExport(
    K2OS_DLX    aDlx,
    BOOL        aIsText,
    char const *apExportName
)
{
    K2STAT stat;
    UINT32 addr;

    addr = 0;

    stat = DLX_FindExport((DLX *)aDlx, aIsText ? DlxSeg_Text : DlxSeg_Data, apExportName, &addr);
    if (!K2STAT_IS_ERROR(stat))
        return addr;

    if (!aIsText)
    {
        stat = DLX_FindExport((DLX *)aDlx, DlxSeg_Read, apExportName, &addr);
        if (!K2STAT_IS_ERROR(stat))
            return addr;
    }

    K2OS_Thread_SetLastStatus(stat);

    return 0;
}

BOOL        
K2OS_Dlx_GetIdent(
    K2OS_DLX        aDlx,
    char *          apNameBuf,
    UINT32          aNameBufLen,
    K2_GUID128  *   apRetID
)
{
    K2STAT stat;

    stat = DLX_GetIdent((DLX *)aDlx, apNameBuf, aNameBufLen, apRetID);
    if (!K2STAT_IS_ERROR(stat))
        return TRUE;

    K2OS_Thread_SetLastStatus(stat);

    return FALSE;
}
