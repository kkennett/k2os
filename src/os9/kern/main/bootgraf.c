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
#include <lib/k2graph.h>

#define ENT_CACHE_COUNT 16

typedef struct _BOOTCACHE BOOTCACHE;
struct _BOOTCACHE
{
    K2RECTENT_CACHE     RectCache;
    K2RECTENT           Entries[ENT_CACHE_COUNT];
    K2LIST_ANCHOR       FreeList;
};

K2RECTENT * 
KernBootGraf_RectEnt_Get(
    K2RECTENT_CACHE *   apCache
)
{
    BOOTCACHE * pBootCache;
    K2RECTENT * pRet;

    pBootCache = K2_GET_CONTAINER(BOOTCACHE, apCache, RectCache);
    K2_ASSERT(pBootCache->FreeList.mNodeCount > 0);

    pRet = (K2RECTENT *)pBootCache->FreeList.mpHead;
    K2LIST_Remove(&pBootCache->FreeList, (K2LIST_LINK *)pRet);
    return pRet;
}

void        
KernBootGraf_RectEnt_Put(
    K2RECTENT_CACHE *   apCache, 
    K2RECTENT *         apEnt
)
{
    BOOTCACHE * pBootCache;

    pBootCache = K2_GET_CONTAINER(BOOTCACHE, apCache, RectCache);
    K2LIST_AddAtTail(&pBootCache->FreeList, (K2LIST_LINK *)apEnt);
}

void    
KernBootGraf_Init(
    void
)
{
    UINT32                ix;
    K2OS_BOOT_GRAPHICS *    pGraf;
    BOOTCACHE               cache;
    K2RECTLIST              mainList;
    K2RECTENT *             pEnt;
    K2RECT                  r;
    UINT32                  virtAddr;
    UINT32                  vWork;
    UINT32                  physAddr;
    UINT32                  mapAttr;
    UINT32                  bufferMapPages;
    UINT32                  pageCount;
    UINT32 *                pTopLeft;
    UINT32 *                pWork;
    UINT32                  linesLeft;
    UINT32                  clearBytes;

    pGraf = &gData.mpShared->LoadInfo.BootGraf;
    if ((0 == pGraf->mFrameBufferPhys) ||
        (0 == pGraf->mFrameBufferBytes) ||
        (0 == pGraf->ModeInfo.HorizontalResolution) ||
        (0 == pGraf->ModeInfo.VerticalResolution))
    {
        K2OSKERN_Debug("***Boot Graphics not specified.\n");
        return;
    }

#if 0
    K2OSKERN_Debug("graf.phys =   0x%08X%08X\n", ((UINT32)pGraf->mFrameBufferPhys >> 32) & 0xFFFFFFFF, (UINT32)(pGraf->mFrameBufferPhys & 0xFFFFFFFFull));
    K2OSKERN_Debug("graf.size =   0x%08X\n", pGraf->mFrameBufferBytes);
    K2OSKERN_Debug("graf.width =  %d\n", pGraf->ModeInfo.HorizontalResolution);
    K2OSKERN_Debug("graf.height = %d\n", pGraf->ModeInfo.VerticalResolution);
#endif

    K2MEM_Zero(&cache, sizeof(cache));
    cache.RectCache.Get = KernBootGraf_RectEnt_Get;
    cache.RectCache.Put = KernBootGraf_RectEnt_Put;
    K2LIST_Init(&cache.FreeList);
    pEnt = &cache.Entries[0];
    for (ix = 0; ix < ENT_CACHE_COUNT; ix++)
    {
        K2LIST_AddAtTail(&cache.FreeList, (K2LIST_LINK *)pEnt);
        pEnt++;
    }
    K2RECTLIST_Init(&mainList, &cache.RectCache);
    r.TopLeft.mX = r.TopLeft.mY = 0;
    r.BottomRight.mX = pGraf->ModeInfo.HorizontalResolution;
    r.BottomRight.mY = pGraf->ModeInfo.VerticalResolution;
    K2RECTLIST_MergeIn(&mainList, &r);

    if (NULL != gData.BootGraf.mpBGRT)
    {
#if 0
        K2OSKERN_Debug("BGRT.status = %08X\n", gData.BootGraf.mpBGRT->Status);
        K2OSKERN_Debug("BGRT.x =      %d\n", gData.BootGraf.mpBGRT->ImageOffsetX);
        K2OSKERN_Debug("BGRT.y =      %d\n", gData.BootGraf.mpBGRT->ImageOffsetY);
        K2OSKERN_Debug("BGRT.width =  %d\n", gData.mpShared->LoadInfo.BootGraf.mBgrtBmpWidth);
        K2OSKERN_Debug("BGRT.height = %d\n", gData.mpShared->LoadInfo.BootGraf.mBgrtBmpHeight);
#endif
        if (gData.BootGraf.mpBGRT->Status == ACPI_BGRT_STATUS_DISPLAYED)
        {
            r.TopLeft.mX = gData.BootGraf.mpBGRT->ImageOffsetX;
            r.TopLeft.mY = gData.BootGraf.mpBGRT->ImageOffsetY;
            r.BottomRight.mX = r.TopLeft.mX + gData.mpShared->LoadInfo.BootGraf.mBgrtBmpWidth;
            r.BottomRight.mY = r.TopLeft.mY + gData.mpShared->LoadInfo.BootGraf.mBgrtBmpHeight;
            K2RECTLIST_CutOut(&mainList, 0, 0, &r);
        }
    }
    else
    {
        K2OSKERN_Debug("No BGRT Found\n");
    }

    //
    // map the display buffer
    //
    physAddr = (UINT32)pGraf->mFrameBufferPhys;
    physAddr &= K2_VA_PAGEFRAME_MASK;
    bufferMapPages = (UINT32)(((pGraf->mFrameBufferPhys + pGraf->mFrameBufferBytes) + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    bufferMapPages = (bufferMapPages - physAddr) / K2_VA_MEMPAGE_BYTES;
    virtAddr = KernVirt_Reserve(bufferMapPages);
    if (0 != virtAddr)
    {
        pTopLeft = (UINT32 *)((((UINT32)pGraf->mFrameBufferPhys) - physAddr) + virtAddr);

        // make mapping
        mapAttr = (K2OS_MEMPAGE_ATTR_READWRITE | K2OS_MEMPAGE_ATTR_WRITE_THRU);
        vWork = virtAddr;
        pageCount = bufferMapPages;
        do {
            KernPte_MakePageMap(NULL, vWork, physAddr, mapAttr);
            vWork += K2_VA_MEMPAGE_BYTES;
            physAddr += K2_VA_MEMPAGE_BYTES;
        } while (--pageCount);

        //
        // clear all rects in the rect list
        //
        pEnt = mainList.mpHead;
        if (NULL != pEnt)
        {
            do {
#if 0
                K2OSKERN_Debug("BootGraf - Clear %d,%d for %d,%d\n",
                    pEnt->Rect.TopLeft.mX, pEnt->Rect.TopLeft.mY,
                    pEnt->Rect.BottomRight.mX - pEnt->Rect.TopLeft.mX,
                    pEnt->Rect.BottomRight.mY - pEnt->Rect.TopLeft.mY
                );
#endif
                pWork = pTopLeft + (pEnt->Rect.TopLeft.mY * pGraf->ModeInfo.PixelsPerScanLine) + pEnt->Rect.TopLeft.mX;
                clearBytes = (pEnt->Rect.BottomRight.mX - pEnt->Rect.TopLeft.mX) * sizeof(UINT32);
                linesLeft = pEnt->Rect.BottomRight.mY - pEnt->Rect.TopLeft.mY;
                do {
                    K2MEM_Set32(pWork, 0, clearBytes);
                    pWork += pGraf->ModeInfo.PixelsPerScanLine;
                } while (--linesLeft);
                pEnt = pEnt->mpNext;
            } while (NULL != pEnt);
        }

        // break mapping - we are single-core here
        vWork = virtAddr;
        pageCount = bufferMapPages;
        do {
            KernPte_BreakPageMap(NULL, vWork, 0);
            KernArch_InvalidateTlbPageOnCurrentCore(vWork);
            vWork += K2_VA_MEMPAGE_BYTES;
        } while (--pageCount);

        KernVirt_Release(virtAddr);
    }
}

