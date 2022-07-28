#include <lib/k2win32.h>
#include <lib/k2mem.h>
#include <lib/k2xdl.h>

CRITICAL_SECTION gSec;

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
    K2XDL_HOST_FILE *       appRetHostFile, 
    UINT_PTR *              apRetModuleDataAddr, 
    UINT_PTR *              apRetModuleLinkAddr
)
{
    HANDLE                  hFile;
    K2XDL_LOADCTX const *   pParent;
    UINT_PTR                pathLen;
    char const *            pUsePath;
    char *                  pPathAlloc;

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

    hFile = CreateFile(pUsePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (NULL != pPathAlloc)
    {
        free(pPathAlloc);
    }
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *apRetModuleDataAddr = (UINT_PTR)VirtualAlloc(NULL, K2_VA_MEMPAGE_BYTES, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (0 == (*apRetModuleDataAddr))
    {
        CloseHandle(hFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *appRetHostFile = (K2XDL_HOST_FILE)hFile;
    *apRetModuleLinkAddr = *apRetModuleDataAddr;
    
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
    BOOL    ok;
    UINT64  sectorsLeft;
    DWORD   chunk;
    DWORD   red;

    sectorsLeft = *apSectorCount;
    if (0 == sectorsLeft)
        return K2STAT_NO_ERROR;

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
        ok = ReadFile((HANDLE)apLoadCtx->mHostFile, apBuffer, chunk, &red, NULL);
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
    XDL_FILE_HEADER const * apFileHdr, 
    BOOL                    aKeepSymbols, 
    K2XDL_SEGMENT_ADDRS *   apRetSegmentDataAddrs
)
{
    UINT_PTR    ix;
    SIZE_T      pages;

    for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Count; ix++)
    {
        pages = (SIZE_T)((apFileHdr->Segment[ix].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES);
        if (0 != pages)
        {
            apRetSegmentDataAddrs->mSegAddr[ix] = (UINT64)VirtualAlloc(NULL, pages * K2_VA_MEMPAGE_BYTES, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (0 == apRetSegmentDataAddrs->mSegAddr[ix])
                break;
        }
    }
    if (ix != XDLSegmentIx_Count)
    {
        if (0 != ix)
        {
            do {
                --ix;
                if (0 != apRetSegmentDataAddrs->mSegAddr[ix])
                {
                    VirtualFree((void *)apRetSegmentDataAddrs->mSegAddr[ix], 0, MEM_RELEASE);
                    apRetSegmentDataAddrs->mSegAddr[ix] = 0;
                }
            } while (ix > 0);
        }
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    return K2STAT_NO_ERROR;
}

BOOL
myPreCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL aIsLoad,
    XDL *apXdl
)
{
    return FALSE;
}

K2STAT
myPostCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    K2STAT                  aUserStatus,
    XDL *                   apXdl
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myFinalize(
    K2XDL_LOADCTX const *   apLoadCtx, 
    K2XDL_SEGMENT_ADDRS *   apRetSegmentDataAddrs
)
{
    return K2STAT_NO_ERROR;
//    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myPurge(
    K2XDL_LOADCTX const *       apLoadCtx,
    K2XDL_SEGMENT_ADDRS const * apSegmentDataAddrs
)
{
    UINT_PTR ix;

    for (ix = 0; ix < XDLSegmentIx_Count; ix++)
    {
        if (0 != apSegmentDataAddrs->mSegAddr[ix])
        {
            VirtualFree((void *)(UINT_PTR)apSegmentDataAddrs->mSegAddr[ix], 0, MEM_RELEASE);
        }
    }
    CloseHandle((HANDLE)apLoadCtx->mHostFile);
    VirtualFree((void *)apLoadCtx->mModulePageDataAddr, 0, MEM_RELEASE);
    return K2STAT_NO_ERROR;
}

void
myAtReInit(
    XDL *               apXdl,
    UINT_PTR            aModulePageLinkAddr,
    K2XDL_HOST_FILE *   apInOutHostFile
)
{

}

K2XDL_HOST gHost;

int main(int argc, char **argv)
{
    K2STAT  stat;
    UINT8 * pAlloc;
    UINT8 * pPage;

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

    pAlloc = malloc(K2_VA_MEMPAGE_BYTES * 2);
    pPage = (UINT8 *)(((((UINT_PTR)pAlloc) + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES);
    stat = K2XDL_Init(pPage, &gHost, TRUE, FALSE);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    XDL *kernXdl;
    stat = XDL_Acquire("C:\\repo\\k2os\\bld\\out\\gcc\\xdl\\kern\\X32\\Debug\\k2oskern.xdl", 0, &kernXdl);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    return stat;
}