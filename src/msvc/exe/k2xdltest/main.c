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
    K2XDL_LOADCTX const *   apLoadCtx, 
    K2XDL_OPENRESULT *      apRetResult
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myReadSectors(
    K2XDL_LOADCTX const *   apLoadCtx, 
    K2XDL_HOST_FILE         aHostFile, 
    void *                  apBuffer, 
    UINT_PTR                aSectorCount
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myPrepare(
    K2XDL_LOADCTX const *   apLoadCtx, 
    K2XDL_HOST_FILE         aHostFile, 
    XDL_FILE_HEADER const * apFileHdr, 
    BOOL                    aKeepSymbols, 
    K2XDL_SECTION_ADDRS *   apRetSectionDataAddrs
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myFinalize(
    K2XDL_LOADCTX const *   apLoadCtx, 
    K2XDL_HOST_FILE         aHostFile, 
    K2XDL_SECTION_ADDRS *   apRetSectionDataAddrs
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
myPurge(
    K2XDL_HOST_FILE aHostFile
)
{
    return K2STAT_ERROR_NOT_IMPL;
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
    gHost.Finalize = myFinalize;
    gHost.Purge = myPurge;

    pAlloc = malloc(K2_VA_MEMPAGE_BYTES * 2);
    pPage = (UINT8 *)((((UINT_PTR)pAlloc) + (K2_VA_MEMPAGE_BYTES - 1) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES);
    stat = K2XDL_Init(pPage, &gHost, TRUE, FALSE);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    XDL *kernXdl;
    stat = XDL_Acquire("C:\\repo\\k2os\\bld\\out\\gcc\\xdl\\kern\\X32\\Debug\\k2oskern.xdl", 0, &kernXdl);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    return stat;
}