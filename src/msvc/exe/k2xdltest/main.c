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
    return K2STAT_ERROR_NOT_IMPL;
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
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myPrepare(
    K2XDL_LOADCTX const *   apLoadCtx, 
    XDL_FILE_HEADER const * apFileHdr, 
    BOOL                    aKeepSymbols, 
    K2XDL_SEGMENT_ADDRS *   apRetSegmentDataAddrs
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
myPreCallback(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL aIsLoad,
    XDL *apXdl
)
{
    return K2STAT_ERROR_NOT_IMPL;
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
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myPurge(
    K2XDL_LOADCTX const *   apLoadCtx
)
{
    return K2STAT_ERROR_NOT_IMPL;
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