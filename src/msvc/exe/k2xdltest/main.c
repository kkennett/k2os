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
#include <lib/k2win32.h>
#include <lib/k2mem.h>
#include <lib/k2xdl.h>
#include <lib/k2list.h>

CRITICAL_SECTION gSec;

typedef struct _MyHostFile MyHostFile;
struct _MyHostFile
{
    HANDLE  mhFile;
    HANDLE  mhPageMap;
    HANDLE  mhSegmentMap[XDLSegmentIx_Count];
};

typedef struct _MemMap MemMap;
struct _MemMap
{
    UINT64      Physical;
    UINT64      Virtual;
    UINT_PTR    PageCount;
    K2LIST_LINK ListLink;
};

K2LIST_ANCHOR gMemMapList;

void
MakeMap(
    UINT64      aVirtual,
    UINT64      aPhysical,
    UINT_PTR    aPageCount
)
{
    MemMap *    pMap;

    pMap = (MemMap *)malloc(sizeof(MemMap));
    K2_ASSERT(NULL != pMap);
    pMap->Physical = aPhysical;
    pMap->Virtual = aVirtual;
    pMap->PageCount = aPageCount;
    K2LIST_AddAtTail(&gMemMapList, &pMap->ListLink);
}

void
BreakMap(
    UINT64  aVirtual
)
{
    MemMap *        pMap;
    K2LIST_LINK *   pListLink;

    pListLink = gMemMapList.mpHead;
    K2_ASSERT(NULL != pListLink);
    do {
        pMap = K2_GET_CONTAINER(MemMap, pListLink, ListLink);
        if (pMap->Virtual == aVirtual)
        {
            K2LIST_Remove(&gMemMapList, pListLink);
            free(pMap);
            break;
        }
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
    K2_ASSERT(NULL != pListLink);
}

BOOL
myConvertLoadPtr(
    UINT_PTR * apAddr
)
{
    UINT64          aAddr;
    MemMap *        pMap;
    K2LIST_LINK *   pListLink;

    aAddr = *apAddr;
    if (0 == aAddr)
        return TRUE;

    pListLink = gMemMapList.mpHead;

    do {
        pMap = K2_GET_CONTAINER(MemMap, pListLink, ListLink);

        if ((pMap->Physical <= (UINT64)aAddr) &&
            ((((UINT64)aAddr) - pMap->Physical) < (pMap->PageCount * K2_VA_MEMPAGE_BYTES)))
        {
            *apAddr = (UINT_PTR)(((UINT64)aAddr - pMap->Physical) + pMap->Virtual);
            return TRUE;
        }

        pListLink = pListLink->mpNext;

    } while (NULL != pListLink);

    return FALSE;
}

void
MapFreePhys(
    void
)
{
    MemMap *        pMap;
    K2LIST_LINK *   pListLink;

    pListLink = gMemMapList.mpHead;

    do {
        pMap = K2_GET_CONTAINER(MemMap, pListLink, ListLink);
        pListLink = pListLink->mpNext;
        K2LIST_Remove(&gMemMapList, &pMap->ListLink);
        UnmapViewOfFile((void *)pMap->Physical);
    } while (NULL != pListLink);

}


K2STAT 
myCritSec(
    BOOL aEnter
)
{
    if (aEnter)
    {
        EnterCriticalSection(&gSec);
    }
    else
    {
        LeaveCriticalSection(&gSec);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
myOpen(
    K2XDL_OPENARGS const *  apArgs, 
    K2XDL_HOST_FILE *       apRetHostFile, 
    UINT_PTR *              apRetModuleDataAddr, 
    UINT_PTR *              apRetModuleLinkAddr
)
{
    HANDLE                  hFile;
    K2XDL_LOADCTX const *   pParent;
    UINT_PTR                pathLen;
    char const *            pUsePath;
    char *                  pPathAlloc;
    MyHostFile *            pMyHostFile;
    
    pMyHostFile = (MyHostFile *)malloc(sizeof(MyHostFile));
    if (NULL == pMyHostFile)
        return K2STAT_ERROR_OUT_OF_MEMORY;
    K2MEM_Zero(pMyHostFile, sizeof(MyHostFile));

    pParent = apArgs->mpParentLoadCtx;
    pPathAlloc = NULL;
    if (NULL != pParent)
    {
        do {
            if (NULL == pParent->OpenArgs.mpParentLoadCtx)
                break;
            pParent = pParent->OpenArgs.mpParentLoadCtx;
        } while (1);

        pathLen = (UINT_PTR)(pParent->OpenArgs.mpNamePart - pParent->OpenArgs.mpPath);

        if (0 != pathLen)
        {
            pPathAlloc = malloc(pathLen + apArgs->mNameLen + 8);
            if (NULL == pPathAlloc)
            {
                free(pMyHostFile);
                return K2STAT_ERROR_OUT_OF_MEMORY;
            }
            sprintf_s(pPathAlloc, pathLen + apArgs->mNameLen + 8, "%.*s%.*s.xdl",
                pathLen,
                pParent->OpenArgs.mpPath,
                apArgs->mNameLen,
                apArgs->mpNamePart);
            pUsePath = pPathAlloc;
        }
        else
        {
            pUsePath = apArgs->mpPath;
        }
    }
    else
    {
        pUsePath = apArgs->mpPath;
    }

    printf("Load \"%s\"\n", pUsePath);

    hFile = CreateFile(pUsePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (NULL != pPathAlloc)
    {
        free(pPathAlloc);
    }
    if (INVALID_HANDLE_VALUE == hFile)
    {
        free(pMyHostFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pMyHostFile->mhPageMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, K2_VA_MEMPAGE_BYTES, NULL);
    if (NULL == pMyHostFile->mhPageMap)
    {
        CloseHandle(hFile);
        free(pMyHostFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    *apRetModuleDataAddr = (UINT_PTR)MapViewOfFile(pMyHostFile->mhPageMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (0 == (*apRetModuleDataAddr))
    {
        CloseHandle(pMyHostFile->mhPageMap);
        CloseHandle(hFile);
        free(pMyHostFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    *apRetModuleLinkAddr = (UINT_PTR)MapViewOfFile(pMyHostFile->mhPageMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (0 == (*apRetModuleLinkAddr))
    {
        UnmapViewOfFile((void *)*apRetModuleDataAddr);
        CloseHandle(pMyHostFile->mhPageMap);
        CloseHandle(hFile);
        free(pMyHostFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    MakeMap(*apRetModuleLinkAddr, *apRetModuleDataAddr, 1);

    pMyHostFile->mhFile = hFile;

    *apRetHostFile = (K2XDL_HOST_FILE)pMyHostFile;
    
    return K2STAT_NO_ERROR;
}

K2STAT
myResizeCopyModulePage(
    K2XDL_LOADCTX const *   apLoadCtx,
    UINT_PTR                aNewPageCount,
    UINT_PTR *              apRetNewModuleDataAddr,
    UINT_PTR *              apRetNewModuleLinkAddr
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myReadSectors(
    K2XDL_LOADCTX const *   apLoadCtx, 
    void *                  apBuffer, 
    UINT64 const *          apSectorCount
)
{
    BOOL            ok;
    UINT64          sectorsLeft;
    DWORD           chunk;
    DWORD           red;
    MyHostFile *    pMyHostFile;

    sectorsLeft = *apSectorCount;
    if (0 == sectorsLeft)
        return K2STAT_NO_ERROR;

    pMyHostFile = (MyHostFile *)apLoadCtx->mHostFile;

    do {
        if (sectorsLeft >= 0x3FFFFFull)
        {
            chunk = 0x3FFFFF * XDL_SECTOR_BYTES;
        }
        else
        {
            chunk = ((UINT_PTR)sectorsLeft) * XDL_SECTOR_BYTES;
        }
        red = 0;
        ok = ReadFile(pMyHostFile->mhFile, apBuffer, chunk, &red, NULL);
        if ((!ok) || (chunk != red))
        {
            if (!ok)
                return HRESULT_FROM_WIN32(GetLastError());
            return K2STAT_ERROR_BAD_SIZE;
        }

        sectorsLeft -= (chunk / XDL_SECTOR_BYTES);

    } while (0 != sectorsLeft);
   
    return K2STAT_NO_ERROR;
}

K2STAT 
myPrepare(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aKeepSymbols,
    XDL_FILE_HEADER const * apFileHdr,
    K2XDL_SEGMENT_ADDRS *   apSegAddrs
)
{
    UINT_PTR    ix;
    MyHostFile *pMyHostFile;
    SIZE_T      pages;
    K2STAT      stat;

    pMyHostFile = (MyHostFile *)apLoadCtx->mHostFile;

    //
    // make data maps first
    //
    for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Count; ix++)
    {
        pages = (SIZE_T)((apFileHdr->Segment[ix].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES);
        if (0 != pages)
        {
            pMyHostFile->mhSegmentMap[ix] = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0, K2_VA_MEMPAGE_BYTES * pages, NULL);
            if (NULL == pMyHostFile->mhSegmentMap[ix])
                break;
            apSegAddrs->mData[ix] = (UINT64)MapViewOfFile(pMyHostFile->mhSegmentMap[ix], FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
            if (0 == apSegAddrs->mData[ix])
            {
                CloseHandle(pMyHostFile->mhSegmentMap[ix]);
                pMyHostFile->mhSegmentMap[ix] = NULL;
                break;
            }
        }
    }
    if (ix != XDLSegmentIx_Count)
    {
        if (0 != ix)
        {
            do {
                --ix;
                if (0 != apSegAddrs->mData[ix])
                {
                    UnmapViewOfFile((void *)(UINT_PTR)apSegAddrs->mData[ix]);
                    apSegAddrs->mData[ix] = 0;
                    CloseHandle(pMyHostFile->mhSegmentMap[ix]);
                    pMyHostFile->mhSegmentMap[ix] = NULL;
                }
            } while (ix > 0);
        }
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    stat = K2STAT_NO_ERROR;

    //
    // link map text read/exec
    //
    if (0 != apSegAddrs->mData[XDLSegmentIx_Text])
    {
        apSegAddrs->mLink[XDLSegmentIx_Text] = (UINT64)MapViewOfFile(pMyHostFile->mhSegmentMap[XDLSegmentIx_Text], FILE_MAP_READ | FILE_MAP_EXECUTE, 0, 0, 0);
        if (0 == apSegAddrs->mLink[XDLSegmentIx_Text])
        {
            stat = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            pages = (SIZE_T)((apFileHdr->Segment[XDLSegmentIx_Text].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES);
            MakeMap(apSegAddrs->mLink[XDLSegmentIx_Text], apSegAddrs->mData[XDLSegmentIx_Text], pages);
        }
    }
    if (!K2STAT_IS_ERROR(stat))
    {
        do {
            //
            // link map read
            //
            if (0 != apSegAddrs->mData[XDLSegmentIx_Read])
            {
                apSegAddrs->mLink[XDLSegmentIx_Read] = (UINT64)MapViewOfFile(pMyHostFile->mhSegmentMap[XDLSegmentIx_Read], FILE_MAP_READ, 0, 0, 0);
                if (0 == apSegAddrs->mLink[XDLSegmentIx_Read])
                {
                    stat = HRESULT_FROM_WIN32(GetLastError());
                    break;
                }
                pages = (SIZE_T)((apFileHdr->Segment[XDLSegmentIx_Read].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES);
                MakeMap(apSegAddrs->mLink[XDLSegmentIx_Read], apSegAddrs->mData[XDLSegmentIx_Read], pages);
            }
            if (!K2STAT_IS_ERROR(stat))
            {
                do {
                    //
                    // link map data read/write
                    //
                    if (0 != apSegAddrs->mData[XDLSegmentIx_Data])
                    {
                        apSegAddrs->mLink[XDLSegmentIx_Data] = (UINT64)MapViewOfFile(pMyHostFile->mhSegmentMap[XDLSegmentIx_Data], FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
                        if (0 == apSegAddrs->mLink[XDLSegmentIx_Data])
                        {
                            stat = HRESULT_FROM_WIN32(GetLastError());
                            break;
                        }
                        pages = (SIZE_T)((apFileHdr->Segment[XDLSegmentIx_Data].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES);
                        MakeMap(apSegAddrs->mLink[XDLSegmentIx_Data], apSegAddrs->mData[XDLSegmentIx_Data], pages);
                    }
                    if (!K2STAT_IS_ERROR(stat))
                    {
                        if (aKeepSymbols)
                        {
                            //
                            // link map symbols read only
                            //
                            if (0 != apSegAddrs->mData[XDLSegmentIx_Symbols])
                            {
                                apSegAddrs->mLink[XDLSegmentIx_Symbols] = (UINT64)MapViewOfFile(pMyHostFile->mhSegmentMap[XDLSegmentIx_Symbols], FILE_MAP_READ, 0, 0, 0);
                                if (0 == apSegAddrs->mLink[XDLSegmentIx_Symbols])
                                {
                                    stat = HRESULT_FROM_WIN32(GetLastError());
                                }
                                else
                                {
                                    pages = (SIZE_T)((apFileHdr->Segment[XDLSegmentIx_Symbols].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES);
                                    MakeMap(apSegAddrs->mLink[XDLSegmentIx_Symbols], apSegAddrs->mData[XDLSegmentIx_Symbols], pages);
                                }
                            }
                        }
                    }
                    if (K2STAT_IS_ERROR(stat))
                    {
                        if (0 != apSegAddrs->mLink[XDLSegmentIx_Data])
                        {
                            BreakMap(apSegAddrs->mLink[XDLSegmentIx_Data]);
                            UnmapViewOfFile((void *)(UINT_PTR)apSegAddrs->mLink[XDLSegmentIx_Data]);
                            apSegAddrs->mLink[XDLSegmentIx_Data] = 0;
                        }
                    }

                } while (0);
            }
            if (K2STAT_IS_ERROR(stat))
            {
                if (0 != apSegAddrs->mLink[XDLSegmentIx_Read])
                {
                    BreakMap(apSegAddrs->mLink[XDLSegmentIx_Read]);
                    UnmapViewOfFile((void *)(UINT_PTR)apSegAddrs->mLink[XDLSegmentIx_Read]);
                    apSegAddrs->mLink[XDLSegmentIx_Read] = 0;
                }
            }

        } while (0);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        if (0 != apSegAddrs->mLink[XDLSegmentIx_Text])
        {
            BreakMap(apSegAddrs->mLink[XDLSegmentIx_Text]);
            UnmapViewOfFile((void *)(UINT_PTR)apSegAddrs->mLink[XDLSegmentIx_Text]);
            apSegAddrs->mLink[XDLSegmentIx_Text] = 0;
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Count; ix++)
        {
            if (0 != apSegAddrs->mData[ix])
            {
                UnmapViewOfFile((void *)(UINT_PTR)apSegAddrs->mData[ix]);
                apSegAddrs->mData[ix] = 0;
                CloseHandle(pMyHostFile->mhSegmentMap[ix]);
                pMyHostFile->mhSegmentMap[ix] = NULL;
            }
        }
    }

    return stat;
}

BOOL
myPreCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aIsLoad,
    XDL *                   apXdl
)
{
//    return TRUE;
    return FALSE;
}

void
myPostCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    K2STAT                  aUserStatus,
    XDL *                   apXdl
)
{
    
}

K2STAT 
myFinalize(
    K2XDL_LOADCTX const *   apLoadCtx, 
    BOOL                    aKeepSymbols,
    XDL_FILE_HEADER const * apFileHdr,
    K2XDL_SEGMENT_ADDRS *   apSegAddrs
)
{
    MyHostFile *pMyHostFile;

    pMyHostFile = (MyHostFile *)apLoadCtx->mHostFile;

    if (0 != apSegAddrs->mData[XDLSegmentIx_Symbols])
    {
        if (!aKeepSymbols)
        {
            if (0 != apSegAddrs->mLink[XDLSegmentIx_Symbols])
            {
                BreakMap(apSegAddrs->mLink[XDLSegmentIx_Symbols]);
                UnmapViewOfFile((void *)(UINT_PTR)apSegAddrs->mLink[XDLSegmentIx_Symbols]);
                apSegAddrs->mLink[XDLSegmentIx_Symbols] = 0;
            }
            UnmapViewOfFile((void *)(UINT_PTR)apSegAddrs->mData[XDLSegmentIx_Symbols]);
            apSegAddrs->mData[XDLSegmentIx_Symbols] = 0;
            CloseHandle(pMyHostFile->mhSegmentMap[XDLSegmentIx_Symbols]);
            pMyHostFile->mhSegmentMap[XDLSegmentIx_Symbols] = NULL;
        }
    }

    if (0 != apSegAddrs->mData[XDLSegmentIx_Relocs])
    {
        UnmapViewOfFile((void *)(UINT_PTR)apSegAddrs->mData[XDLSegmentIx_Relocs]);
        apSegAddrs->mData[XDLSegmentIx_Relocs] = 0;
        K2_ASSERT(0 == apSegAddrs->mLink[XDLSegmentIx_Relocs]);
        CloseHandle(pMyHostFile->mhSegmentMap[XDLSegmentIx_Relocs]);
        pMyHostFile->mhSegmentMap[XDLSegmentIx_Relocs] = NULL;
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
myPurge(
    K2XDL_LOADCTX const *       apLoadCtx,
    K2XDL_SEGMENT_ADDRS const * apSegmentDataAddrs
)
{
    UINT_PTR    ix;
    MyHostFile *pMyHostFile;

    pMyHostFile = (MyHostFile *)apLoadCtx->mHostFile;

    for (ix = 0; ix < XDLSegmentIx_Count; ix++)
    {
        if (0 != apSegmentDataAddrs->mData[ix])
        {
            UnmapViewOfFile((void *)(UINT_PTR)apSegmentDataAddrs->mData[ix]);
        }
        if (0 != apSegmentDataAddrs->mLink[ix])
        {
            BreakMap(apSegmentDataAddrs->mLink[ix]);
            UnmapViewOfFile((void *)(UINT_PTR)apSegmentDataAddrs->mLink[ix]);
        }
        if (NULL != pMyHostFile->mhSegmentMap[ix])
        {
            CloseHandle(pMyHostFile->mhSegmentMap[ix]);
        }
    }
    CloseHandle(pMyHostFile->mhFile);

    UnmapViewOfFile((void *)apLoadCtx->mModulePageDataAddr);
    BreakMap(apLoadCtx->mModulePageLinkAddr);
    UnmapViewOfFile((void *)apLoadCtx->mModulePageLinkAddr);
    CloseHandle(pMyHostFile->mhPageMap);

    free(pMyHostFile);

    return K2STAT_NO_ERROR;
}

void
myAtReInit(
    XDL *               apXdl,
    UINT_PTR            aModulePageLinkAddr,
    K2XDL_HOST_FILE *   apInOutHostFile
)
{
    XDL_FILE_HEADER const *pHeader;
    XDL_GetHeaderPtr(apXdl, &pHeader);
    printf("AtReInit(0x%08X, %.*s)\n", (UINT_PTR)apXdl, pHeader->mNameLen, pHeader->mName);
}

K2XDL_HOST gHost;


int main(int argc, char **argv)
{
    K2STAT  stat;
    UINT_PTR addr;
    XDL_pf_ENTRYPOINT entry;
    HANDLE  hGlobalMap;
    UINT64  globalPhys;
    UINT64  globalVirt;

    hGlobalMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, K2_VA_MEMPAGE_BYTES, NULL);
    K2_ASSERT(NULL != hGlobalMap);
    globalPhys = (UINT64)MapViewOfFile(hGlobalMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    K2_ASSERT(0 != globalPhys);
    globalVirt = (UINT64)MapViewOfFile(hGlobalMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    K2_ASSERT(0 != globalVirt);

    InitializeCriticalSection(&gSec);

    K2MEM_Zero(&gHost, sizeof(gHost));
    gHost.CritSec = myCritSec;
    gHost.Open = myOpen;
    gHost.ReadSectors = myReadSectors;
    gHost.Prepare = myPrepare;
    gHost.PreCallback = myPreCallback;
    gHost.PostCallback = myPostCallback;
    gHost.Finalize = myFinalize;
    gHost.Purge = myPurge;
    gHost.AtReInit = myAtReInit;

    K2LIST_Init(&gMemMapList);

    stat = K2XDL_Init((void *)(UINT_PTR)globalPhys, &gHost, TRUE, FALSE);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    XDL *kernXdl;
//    stat = XDL_Acquire("C:\\repo\\k2os\\bld\\out\\gcc\\xdl\\kern\\X32\\Debug\\k2oskern.xdl", 0, &kernXdl);
    stat = XDL_Acquire("C:\\repo\\k2os\\bld\\out\\gcc\\xdl\\kern\\X32\\Debug\\k2osplat.xdl", 0, &kernXdl);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = K2XDL_Handoff(&kernXdl, myConvertLoadPtr, &entry);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    //
    // virtual to physical transition happens here (lose access to physical memory)
    //
    MapFreePhys();
    UnmapViewOfFile((void *)(UINT_PTR)globalPhys);

    //
    // should be operating purely on virtual addresses now
    //
    stat = K2XDL_Init((void *)(UINT_PTR)globalVirt, &gHost, TRUE, TRUE);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    XDL *pKern2;
    stat = XDL_Acquire("k2oskern.xdl", 0, &pKern2);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = XDL_FindExport(kernXdl, XDLProgData_Text, "K2OSPLAT_Init", &addr);
    if (!K2STAT_IS_ERROR(stat))
    {
        char addrName[256];
        stat = XDL_FindAddrName(kernXdl, addr, addrName, 255);
        printf("%s\n", addrName);

    }

    return stat;
}