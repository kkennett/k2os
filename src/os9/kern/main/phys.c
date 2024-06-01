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

#define PHYS_AUDIT 0

#define DESC_BUFFER_BYTES   (((sizeof(K2EFI_MEMORY_DESCRIPTOR) * 2) + 4) & ~3)

#define FIRST_BUCKET_INDEX  K2_VA_MEMPAGE_BYTES_POW2
#define BUDDY_BUCKET_COUNT  (31 - FIRST_BUCKET_INDEX)

static K2LIST_ANCHOR sgPhysFreeList[BUDDY_BUCKET_COUNT];

#if PHYS_AUDIT 

static char const * sgK2TypeNames[] =
{
    "PAGING          ",
    "XDL_TEXT        ",
    "XDL_READ        ",
    "XDL_DATA        ",
    "XDL_PAGE        ",
    "XDL_LOADER      ",
    "ZERO            ",
    "CORES           ",
    "PHYS_TRACK      ",
    "TRANSITION      ",
    "EFI_MAP         ",
    "FW_TABLES       ",
    "ARCH_SPEC       ",
    "BUILTIN         ",
    "THREADPTRS      ",
};

static char const * sgEfiTypeNames[K2EFI_MEMTYPE_COUNT] =
{
    "EFI_Reserved    ",
    "EFI_Loader_Code ",
    "EFI_Loader_Data ",
    "EFI_Boot_Code   ",
    "EFI_Boot_Data   ",
    "EFI_Run_Code    ",
    "EFI_Run_Data    ",
    "EFI_Conventional",
    "EFI_Unusable    ",
    "EFI_ACPI_Reclaim",
    "EFI_ACPI_NVS    ",
    "EFI_MappedIO    ",
    "EFI_MappedIOPort",
    "EFI_PAL         ",
    "EFI_NV          ",
};

void
KernPhys_PrintDescType(
    UINT32 aDescType
)
{
    char const *pName;

    if (aDescType & 0x80000000)
    {
        aDescType -= 0x80000000;
        if (aDescType > K2OS_EFIMEMTYPE_LAST)
            pName = "*UNKNOWN K2*    ";
        else
            pName = sgK2TypeNames[aDescType];
    }
    else
    {
        if (aDescType < K2EFI_MEMTYPE_COUNT)
            pName = sgEfiTypeNames[aDescType];
        else
            pName = "*UNKNOWN EFI*   ";
    }

    K2OSKERN_Debug("%s", pName);
}

void
KernPhys_PrintDesc(
    UINT32                      aIx,
    K2EFI_MEMORY_DESCRIPTOR *   apDesc,
    BOOL                        aIsFree
)
{
    K2OSKERN_Debug("%03d: ", aIx);

    KernPhys_PrintDescType(apDesc->Type);

    K2OSKERN_Debug(": p%08X v%08X z%08X (%6d) a%08X%08X %c\n",
        (UINT32)(((UINT64)apDesc->PhysicalStart) & 0x00000000FFFFFFFFull),
        (UINT32)(((UINT64)apDesc->VirtualStart) & 0x00000000FFFFFFFFull),
        (UINT32)(apDesc->NumberOfPages * K2_VA_MEMPAGE_BYTES),
        (UINT32)apDesc->NumberOfPages,
        (UINT32)(((UINT64)apDesc->Attribute) >> 32), (UINT32)(((UINT64)apDesc->Attribute) & 0x00000000FFFFFFFFull),
        aIsFree ? '*' : ' ');
}

void
KernPhys_DumpEfiMap(
    void
)
{
    UINT32                  entCount;
    K2EFI_MEMORY_DESCRIPTOR desc;
    UINT8 const *           pWork;
    BOOL                    reuse;
    UINT32                  ix;
    UINT32                  physStart;
    UINT32                  lastEnd;

    K2OSKERN_Debug("\nDumpEfiMap:\n----\n");
    entCount = gData.mpShared->LoadInfo.mEfiMapSize / gData.mpShared->LoadInfo.mEfiMemDescSize;
    pWork = (UINT8 const *)K2OS_KVA_EFIMAP_BASE;
    ix = 0;
    lastEnd = 0;
    do
    {
        K2MEM_Copy(&desc, pWork, sizeof(K2EFI_MEMORY_DESCRIPTOR));

        physStart = (UINT32)(UINT64)desc.PhysicalStart;

        if (physStart != lastEnd)
        {
            K2OSKERN_Debug("---:  HOLE             : p%08X           z%08X (%6d)\n",
                lastEnd,
                (physStart - lastEnd),
                (physStart - lastEnd) / K2_VA_MEMPAGE_BYTES);
        }

        lastEnd = physStart + (desc.NumberOfPages * K2_VA_MEMPAGE_BYTES);

        if (desc.Attribute & K2EFI_MEMORYFLAG_RUNTIME)
            reuse = FALSE;
        else
        {
            if (desc.Type & 0x80000000)
                reuse = FALSE;
            else
            {
                if ((desc.Type != K2EFI_MEMTYPE_Reserved) &&
                    (desc.Type != K2EFI_MEMTYPE_Unusable) &&
                    (desc.Type < K2EFI_MEMTYPE_ACPI_NVS))
                    reuse = TRUE;
                else
                    reuse = FALSE;
            }
        }

        KernPhys_PrintDesc(ix, &desc, reuse);

        pWork += gData.mpShared->LoadInfo.mEfiMemDescSize;
        ix++;
    } while (--entCount > 0);

    K2OSKERN_Debug("----\n");
}

void
KernPhys_DumpInitPhysTrack(
    void
)
{
    K2OSKERN_PHYSTRACK *pTrack;
    UINT32              lastAddr;
    UINT32              lastProp;
    UINT32              lastType;
    UINT32              thisAddr;
    UINT32              thisProp;
    UINT32              thisType;
    UINT32              pageCount;
    UINT32              pagesLeft;

    K2OSKERN_Debug("\nDumpInitPhysTrack:\n----\n");

    thisAddr = 0;
    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(thisAddr);
    pagesLeft = K2_VA32_PAGEFRAMES_FOR_4G;

    lastAddr = thisAddr;
    lastProp = pTrack->Flags.mAsUINT32;
    lastType = (UINT32)pTrack->mpOwnerProc;
    pageCount = 1;
    pTrack++;
    thisAddr += K2_VA_MEMPAGE_BYTES;
    pagesLeft--;

    do
    {
        thisProp = pTrack->Flags.mAsUINT32;
        thisType = (UINT32)pTrack->mpOwnerProc;
        if ((thisProp != lastProp) ||
            (thisType != lastType))
        {
            K2OSKERN_Debug("%08X - %08X %08X z %08X\n", lastAddr, lastProp, lastType, pageCount * K2_VA_MEMPAGE_BYTES);
            lastAddr = thisAddr;
            lastProp = thisProp;
            lastType = thisType;
            pageCount = 1;
        }
        else
            pageCount++;
        thisAddr += K2_VA_MEMPAGE_BYTES;
        pTrack++;
    } while (--pagesLeft > 0);
    if (pageCount > 0)
    {
       K2OSKERN_Debug("%08X - %08X %08X z %08X\n", lastAddr, lastProp, lastType, pageCount * K2_VA_MEMPAGE_BYTES);
    }
}

void
KernPhys_DumpPhysTrack(
    void
)
{
    K2OSKERN_PHYSTRACK *pTrack;
    UINT32              physAddr;
    UINT32              pagesLeft;
    UINT32              blockBytes;
    UINT32              pageCount;

    K2OSKERN_Debug("\nDumpPhysTrack:\n----\n");

    physAddr = 0;
    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(physAddr);
    pagesLeft = K2_VA32_PAGEFRAMES_FOR_4G;

    do
    {
        if (pTrack->Flags.Field.Exists)
        {
            blockBytes = 1 << pTrack->Flags.Field.BlockSize;
            pageCount = blockBytes / K2_VA_MEMPAGE_BYTES;
            K2_ASSERT(pagesLeft >= pageCount);

            K2OSKERN_Debug("%08X %08X %s\n", 
                physAddr, blockBytes,
                (pTrack->Flags.Field.Free) ? "FREE" : "USED"
            );

            physAddr += blockBytes;
            pTrack += pageCount;
            pagesLeft -= pageCount;
        }
        else
        {
            pageCount = 0;
            do
            {
                pTrack++;
                pageCount++;
            } while ((--pagesLeft) && (0 == pTrack->Flags.Field.Exists));

            K2OSKERN_Debug("%08X %08X NOT EXIST\n", physAddr, pageCount * K2_VA_MEMPAGE_BYTES);
            physAddr += pageCount * K2_VA_MEMPAGE_BYTES;
        }

    } while (pagesLeft > 0);
    K2_ASSERT(0 == physAddr);
}

#endif

void 
KernPhys_ChangePhysTrackToConventional(
    UINT32  aPhysAddr,
    UINT32  aPageCount
)
{
    K2OS_PHYSTRACK_UEFI * pTrack;

    K2_ASSERT(aPageCount > 0);

    pTrack = (K2OS_PHYSTRACK_UEFI *)K2OS_PHYS32_TO_PHYSTRACK(aPhysAddr);
    do {
        pTrack->mType = K2EFI_MEMTYPE_Conventional;
        pTrack++;
    } while (--aPageCount);
}

void 
KernPhys_CompactEfiMap(
    void
)
{
    UINT32                      entCount;
    UINT32                      outCount;
    UINT8                       inBuffer[DESC_BUFFER_BYTES];
    K2EFI_MEMORY_DESCRIPTOR *   pInDesc = (K2EFI_MEMORY_DESCRIPTOR *)inBuffer;
    UINT8                       outBuffer[DESC_BUFFER_BYTES];
    K2EFI_MEMORY_DESCRIPTOR *   pOutDesc = (K2EFI_MEMORY_DESCRIPTOR *)outBuffer;
    UINT8 *                     pIn;
    UINT8 *                     pOut;
    UINT32                      ixIn;
    UINT32                      lastEnd;
    UINT32                      physStart;
    UINT32                      physEnd;

    K2_ASSERT(gData.mpShared->LoadInfo.mEfiMemDescSize <= DESC_BUFFER_BYTES);

    entCount = gData.mpShared->LoadInfo.mEfiMapSize / gData.mpShared->LoadInfo.mEfiMemDescSize;
    outCount = 0;

    pIn = (UINT8 *)K2OS_KVA_EFIMAP_BASE;
    pOut = pIn;

    ixIn = 0;
    lastEnd = 0;
    do {
        //
        // find next free memory entry
        //
        do
        {
            //
            // copy into 4-byte aligned buffer
            //
            if (pOut != pIn)
            {
                K2MEM_Copy(pOut, pIn, gData.mpShared->LoadInfo.mEfiMemDescSize);
            }
            K2MEM_Copy(pInDesc, pIn, gData.mpShared->LoadInfo.mEfiMemDescSize);

            physStart = (UINT32)(UINT64)pInDesc->PhysicalStart;
            physEnd = physStart + (pInDesc->NumberOfPages * K2_VA_MEMPAGE_BYTES);
            if (physStart != lastEnd)
            {
                //
                // HOLE IN EFI MAP
                //
                //K2OSKERN_Debug("HOLE IN EFI MAP %08X for %08X\n", lastEnd, physStart - lastEnd);
            }

            if ((!(pInDesc->Attribute & K2EFI_MEMORYFLAG_RUNTIME)) &&
                (0 == (pInDesc->Type & 0x80000000)) &&
                (pInDesc->Type != K2EFI_MEMTYPE_Reserved) &&
                (pInDesc->Type != K2EFI_MEMTYPE_Unusable) &&
                (pInDesc->Type < K2EFI_MEMTYPE_ACPI_NVS))
                break;

            lastEnd = physEnd;

            pIn += gData.mpShared->LoadInfo.mEfiMemDescSize;
            ixIn++;
            pOut += gData.mpShared->LoadInfo.mEfiMemDescSize;
            outCount++;
            
        } while (--entCount > 0);
        if (entCount == 0)
            break;

        //
        // pIn points to a descriptor of free memory
        // pInDesc points to a 4-byte aligned copy of the pIn descriptor on the stack
        // pOut points to identical descriptor to the one at pIn at its final destination
        // pOutDesc points to a 4-byte aligned buffer of garbage on the stack
        //
        K2MEM_Copy(pOutDesc, pInDesc, gData.mpShared->LoadInfo.mEfiMemDescSize);
        pOutDesc->Type = K2EFI_MEMTYPE_Conventional;
        
        //
        // convert outdesc range of phystrack to conventional memory type
        //
        KernPhys_ChangePhysTrackToConventional(
            (UINT32)((UINT64)pOutDesc->PhysicalStart),
            (UINT32)pOutDesc->NumberOfPages);

        if (--entCount > 0)
        {
            do {
                pIn += gData.mpShared->LoadInfo.mEfiMemDescSize;
                ixIn++;

                K2MEM_Copy(pInDesc, pIn, gData.mpShared->LoadInfo.mEfiMemDescSize);

                physStart = (UINT32)(UINT64)pInDesc->PhysicalStart;

                if (physStart != physEnd)
                {
                    // HOLE
                    break;
                }

                if ((pInDesc->Attribute & K2EFI_MEMORYFLAG_RUNTIME) ||
                    (0 != (pInDesc->Type & 0x80000000)) ||
                    (pInDesc->Type == K2EFI_MEMTYPE_Reserved) ||
                    (pInDesc->Type == K2EFI_MEMTYPE_Unusable) || 
                    (pInDesc->Type >= K2EFI_MEMTYPE_ACPI_NVS))
                {
                    break;
                }

                pOutDesc->NumberOfPages += pInDesc->NumberOfPages;
                physEnd = physStart + (pInDesc->NumberOfPages * K2_VA_MEMPAGE_BYTES);

                //
                // convert indesc range of phystrack to conventional memory type
                //
                KernPhys_ChangePhysTrackToConventional(
                    (UINT32)((UINT64)pInDesc->PhysicalStart),
                    (UINT32)pInDesc->NumberOfPages);

            } while (--entCount > 0);
        }

        K2MEM_Copy(pOut, pOutDesc, gData.mpShared->LoadInfo.mEfiMemDescSize);
        pOut += gData.mpShared->LoadInfo.mEfiMemDescSize;
        outCount++;

        lastEnd = physEnd;

    } while (entCount > 0);

    K2MEM_Zero(pOut, (ixIn + 1 - outCount) * gData.mpShared->LoadInfo.mEfiMemDescSize);
    
    gData.mpShared->LoadInfo.mEfiMapSize = outCount * gData.mpShared->LoadInfo.mEfiMemDescSize;
}

void
KernPhys_InitFreeBlock(
    UINT32 aChunkAddr,
    UINT32 aChunkBytes
)
{
    K2OSKERN_PHYSTRACK *    pTrack;
    UINT32                  blockSizeBitIndex;
    UINT32                  buddyAddr;

    // set K2OSKERN_PHYSTRACK_FLAGS_FREE on power-of-two chunk starts as you add them
    // also must set proper size in flags field BlockSize
    K2_ASSERT(KernBit_IsPowerOfTwo(aChunkBytes));


    // add to global free page count
    gData.Phys.mPagesLeft += aChunkBytes / K2_VA_MEMPAGE_BYTES;

    blockSizeBitIndex = KernBit_LowestSet_Index(aChunkBytes);

    K2_ASSERT(blockSizeBitIndex >= FIRST_BUCKET_INDEX);

    //
    // buddy must not be same size if it exists
    //
    buddyAddr = aChunkAddr ^ aChunkBytes;
    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(buddyAddr);
    K2_ASSERT((0 == pTrack->Flags.Field.Exists) || 
              (0 == pTrack->Flags.Field.Free) ||
              (pTrack->Flags.Field.BlockSize != blockSizeBitIndex));

    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(aChunkAddr);
    K2_ASSERT(0 != pTrack->Flags.Field.Exists);

    pTrack->Flags.Field.Free = 1;
    pTrack->Flags.Field.BlockSize = blockSizeBitIndex;

    K2LIST_AddAtTail(&sgPhysFreeList[blockSizeBitIndex - FIRST_BUCKET_INDEX], &pTrack->ListLink);
}

void 
KernPhys_InitAddFreeChunkToBuddyHeap(
    UINT32 aPhysAddr,
    UINT32 aPageCount
)
{
    UINT32 chunkBytes;
    UINT32 workBit;

    if (0 == aPageCount)
        return;

    chunkBytes = aPageCount * K2_VA_MEMPAGE_BYTES;

    if (aPhysAddr == 0)
    {
        workBit = KernBit_ExtractHighestSet(chunkBytes);
        KernPhys_InitFreeBlock(aPhysAddr, workBit);
        aPhysAddr += workBit;
        chunkBytes -= workBit;
        if (0 == chunkBytes)
            return;
    }
    do
    {
        workBit = KernBit_ExtractLowestSet(aPhysAddr);
        while (workBit > chunkBytes)
            workBit >>= 1;
        KernPhys_InitFreeBlock(aPhysAddr, workBit);
        aPhysAddr += workBit;
        chunkBytes -= workBit;
    } while (0 != chunkBytes);
}

void
KernPhys_InitPhysTrack(
    void
)
{
    UINT32                      entCount;
    UINT8                       descBuffer[DESC_BUFFER_BYTES];
    K2EFI_MEMORY_DESCRIPTOR *   pDesc = (K2EFI_MEMORY_DESCRIPTOR *)descBuffer;
    UINT8 *                     pWork;
    K2OS_PHYSTRACK_UEFI *       pTrackEFI;
    K2OSKERN_PHYSTRACK *        pTrackPage;
    UINT32                      memProp;
    UINT32                      pageCount;

    //
    // make sure uefi page properties and flags are in the same place 
    //
    K2_ASSERT(K2_FIELDOFFSET(K2OS_PHYSTRACK_UEFI, mProp) == K2_FIELDOFFSET(K2OSKERN_PHYSTRACK, Flags));

    entCount = gData.mpShared->LoadInfo.mEfiMapSize / gData.mpShared->LoadInfo.mEfiMemDescSize;
    pWork = (UINT8 *)K2OS_KVA_EFIMAP_BASE;
    do
    {
        K2MEM_Copy(pDesc, pWork, gData.mpShared->LoadInfo.mEfiMemDescSize);

        pTrackEFI = (K2OS_PHYSTRACK_UEFI *)K2OS_PHYS32_TO_PHYSTRACK((UINT32)((UINT64)pDesc->PhysicalStart));
        memProp = pTrackEFI->mProp;
        memProp = ((memProp & 0x00007000) >> 1) | ((memProp & 0x0000001F) << 2);
        memProp |= K2OSKERN_PHYSTRACK_FLAGS_EXISTS;

        pTrackPage = (K2OSKERN_PHYSTRACK *)pTrackEFI;
        pageCount = (UINT32)pDesc->NumberOfPages;

        //
        // mark all pages in the range with the specified property, zeroing out other fields
        //
        if ((!(pDesc->Attribute & K2EFI_MEMORYFLAG_RUNTIME)) &&
            (pDesc->Type == K2EFI_MEMTYPE_Conventional))
        {
//            K2OSKERN_Debug("FREE %08X [%08X]\n",
//                (UINT32)(pDesc->PhysicalStart & 0xFFFFFFFFull),
//                pageCount * K2_VA_MEMPAGE_BYTES);

            do
            {
                K2_ASSERT(pDesc->Type == ((K2OS_PHYSTRACK_UEFI *)pTrackPage)->mType);
                pTrackPage->Flags.mAsUINT32 = memProp;
                pTrackPage->mpOwnerProc = NULL;
                pTrackPage->ListLink.mpPrev = NULL;
                pTrackPage->ListLink.mpNext = NULL;
                pTrackPage++;
            } while (--pageCount);

            //
            // add as free chunks to buddy heap
            //
            KernPhys_InitAddFreeChunkToBuddyHeap((UINT32)((UINT64)pDesc->PhysicalStart), (UINT32)pDesc->NumberOfPages);
        }
        else
        {
            do
            {
                pTrackPage->Flags.mAsUINT32 = memProp;
                pTrackPage->Flags.Field.BlockSize = K2_VA_MEMPAGE_BYTES_POW2;
                pTrackPage->mpOwnerProc = NULL;
                pTrackPage++;
            } while (--pageCount);
        }

        pWork += gData.mpShared->LoadInfo.mEfiMemDescSize;
    } while (--entCount > 0);
}

void 
KernPhys_Init(
    void
)
{
    UINT32 ix;

    K2OSKERN_SeqInit(&gData.Phys.SeqLock);

    for (ix = 0; ix < BUDDY_BUCKET_COUNT; ix++)
    {
        K2LIST_Init(&sgPhysFreeList[ix]);
    }

    for (ix = 0; ix < KernPhysPageList_Count; ix++)
    {
        K2LIST_Init(&gData.Phys.PageList[ix]);
    }

#if PHYS_AUDIT
    KernPhys_DumpEfiMap();
    KernPhys_DumpInitPhysTrack();
#endif

    KernPhys_CompactEfiMap();

    KernPhys_InitPhysTrack();

#if PHYS_AUDIT
    KernPhys_DumpPhysTrack();
#endif
}

K2OSKERN_PHYSTRACK *
KernPhys_Locked_AllocPow2PagesChunk(
    UINT32  aFreeChunkBit,
    UINT32  aChunkBit
)
{
    K2OSKERN_PHYSTRACK *pTrack;
    UINT32              chunkAddr;
    UINT32              buddyAddr;
    K2OSKERN_PHYSTRACK *pBuddy;
    K2LIST_ANCHOR *     pList;

    // must carve out a block of (1<<aChunkBit) bytes
    // from a block of size (1<<aFreeChunkBit) which is
    // guaranteed to exist
    K2_ASSERT(aFreeChunkBit > aChunkBit);

    pList = &sgPhysFreeList[aFreeChunkBit - FIRST_BUCKET_INDEX];

    pTrack = (K2OSKERN_PHYSTRACK *)K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pList->mpHead, ListLink);

    K2_ASSERT(pTrack->Flags.Field.BlockSize == aFreeChunkBit);

    K2LIST_Remove(pList, &pTrack->ListLink);
    pTrack->Flags.Field.Free = 0;

    chunkAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
    
    do {
        aFreeChunkBit--;

        buddyAddr = (chunkAddr ^ (1 << aFreeChunkBit));
        pBuddy = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(buddyAddr);
        K2_ASSERT(0 != pBuddy->Flags.Field.Exists);
        K2_ASSERT(0 == pBuddy->Flags.Field.BlockSize);
        K2_ASSERT(0 == pBuddy->Flags.Field.Free);

        pTrack->Flags.Field.BlockSize = aFreeChunkBit;
        pBuddy->Flags.Field.BlockSize = aFreeChunkBit;
        pBuddy->Flags.Field.Free = 1;
        K2LIST_AddAtTail(&sgPhysFreeList[aFreeChunkBit - FIRST_BUCKET_INDEX], &pBuddy->ListLink);

    } while (aFreeChunkBit > aChunkBit);

    return pTrack;
}

void
KernPhys_Locked_FreeOneTrack(
    K2OSKERN_PHYSTRACK * apTrack
)
{
    UINT32                  chunkAddr;
    UINT32                  buddyAddr;
    UINT32                  blockSize;
    UINT32                  newPageCount;
    K2OSKERN_PHYSTRACK *    pBuddy;
   
    K2_ASSERT(0 != apTrack->Flags.Field.Exists);
    K2_ASSERT(0 == apTrack->Flags.Field.Free);
    K2_ASSERT(KernPhysPageList_None == apTrack->Flags.Field.PageListIx);
    K2_ASSERT(NULL == apTrack->mpOwnerProc);

    chunkAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)apTrack);
    blockSize = apTrack->Flags.Field.BlockSize;

    // put onto global pages left count
    newPageCount = (1 << (blockSize - FIRST_BUCKET_INDEX));
    do
    {
        buddyAddr = gData.Phys.mPagesLeft;
    } while (buddyAddr != K2ATOMIC_CompareExchange(&gData.Phys.mPagesLeft, buddyAddr + newPageCount, buddyAddr));

    do
    {
        buddyAddr = chunkAddr ^ (1 << blockSize);
        pBuddy = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(buddyAddr);
        if (0 == pBuddy->Flags.Field.Exists)
            break;
        if (0 == pBuddy->Flags.Field.Free)
            break;
        if (pBuddy->Flags.Field.BlockSize != blockSize)
            break;

        // 
        // buddy exists and is free - merge with it
        //
        K2LIST_Remove(&sgPhysFreeList[blockSize - FIRST_BUCKET_INDEX], &pBuddy->ListLink);
        pBuddy->Flags.Field.Free = 0;

        blockSize++;
        if (buddyAddr < chunkAddr)
        {
//            K2OSKERN_Debug("Merge %08X <- %08X, to size %08X\n", buddyAddr, chunkAddr, 1 << blockSize);
            apTrack->Flags.Field.BlockSize = 0;
            apTrack = pBuddy;
            chunkAddr = buddyAddr;
        }
        else
        {
//            K2OSKERN_Debug("Merge %08X <- %08X, to size %08X\n", chunkAddr, buddyAddr, 1 << blockSize);
            pBuddy->Flags.Field.BlockSize = 0;
        }
        ++apTrack->Flags.Field.BlockSize;
    } while (1);

    apTrack->Flags.Field.Free = 1;
    K2LIST_AddAtTail(&sgPhysFreeList[blockSize - FIRST_BUCKET_INDEX], &apTrack->ListLink);
}

K2STAT
KernPhys_Locked_AllocSparsePages(
    UINT32          aPageCount,
    K2LIST_ANCHOR * apRetList
)
{
    UINT32                  bytesLeft;
    UINT32                  scanBit;
    UINT32                  lowSizeBit;
    K2LIST_ANCHOR *         pList;
    K2OSKERN_PHYSTRACK *    pTrack;

    K2_ASSERT(0 != aPageCount);

    K2LIST_Init(apRetList);

    bytesLeft = aPageCount * K2_VA_MEMPAGE_BYTES;

    pList = &sgPhysFreeList[0];
    for (scanBit = FIRST_BUCKET_INDEX; scanBit < 31; scanBit++)
    {
        if (0 != pList->mNodeCount)
            break;
        pList++;
    }
    if (scanBit == 31)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    lowSizeBit = KernBit_LowestSet_Index(bytesLeft);
    do
    {
        if (scanBit <= lowSizeBit)
        {
            //
            // take item off scanBit list
            //
            pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pList->mpHead, ListLink);
            K2LIST_Remove(pList, &pTrack->ListLink);
            pTrack->Flags.Field.Free = 0;
            bytesLeft -= (1 << scanBit);
            if (bytesLeft > 0)
            {
                //
                // more to alloc. check for empty list(s) and move to
                // next list that has stuff if there is some.  if nothing
                // else remains then we are out of memory
                //
                while (0 == pList->mNodeCount)
                {
                    if (++scanBit == 31)
                    {
                        //
                        // out of memory. undo all allocations
                        //
                        KernPhys_Locked_FreeOneTrack(pTrack);
                        do
                        {
                            pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, apRetList->mpHead, ListLink);
                            K2LIST_Remove(apRetList, &pTrack->ListLink);
                            KernPhys_Locked_FreeOneTrack(pTrack);
                        } while (apRetList->mNodeCount > 0);

                        return K2STAT_ERROR_OUT_OF_MEMORY;
                    }
                    pList++;
                }
                lowSizeBit = KernBit_LowestSet_Index(bytesLeft);
            }
        }
        else
        {
            //
            // allocate a contiguous chunk of lowSizeBit from
            // a chunk that is scanBit in size, which is guaranteed
            // to exist.
            //
            pTrack = KernPhys_Locked_AllocPow2PagesChunk(scanBit, lowSizeBit);
            bytesLeft -= (1 << lowSizeBit);
            // 
            // we must have added a power of 2 block to the same
            // list as lowSizeBit (the allocated block's now-free buddy)
            scanBit = lowSizeBit;
            pList = &sgPhysFreeList[scanBit - FIRST_BUCKET_INDEX];
            K2_ASSERT(pList->mNodeCount > 0);
        }
        pTrack->Flags.Field.PageListIx = KernPhysPageList_None;
        K2LIST_AddAtTail(apRetList, &pTrack->ListLink);
    } while (bytesLeft > 0);

    return TRUE;
}

K2STAT
KernPhys_AllocSparsePages(
    K2OSKERN_PHYSRES *  apRes,
    UINT32              aPageCount,
    K2LIST_ANCHOR *     apRetList
)
{
    BOOL    disp;
    K2STAT  result;
    UINT32  r;

    // take away from reservation
    do
    {
        r = apRes->mPageCount;
        if (r < aPageCount)
            return K2STAT_ERROR_OUT_OF_MEMORY;
    } while (r != K2ATOMIC_CompareExchange(&apRes->mPageCount, r - aPageCount, r));

    disp = K2OSKERN_SeqLock(&gData.Phys.SeqLock);

    result = KernPhys_Locked_AllocSparsePages(aPageCount, apRetList);

    K2OSKERN_SeqUnlock(&gData.Phys.SeqLock, disp);

    // if failed put back into reservation
    if (K2STAT_IS_ERROR(result))
    {
        do
        {
            r = apRes->mPageCount;
        } while (r != K2ATOMIC_CompareExchange(&apRes->mPageCount, r + aPageCount, r));
    }

    return result;
}

K2STAT
KernPhys_AllocPow2Bytes(
    K2OSKERN_PHYSRES *      apRes,
    UINT32                  aPow2Bytes,
    K2OSKERN_PHYSTRACK **   appRetTrack
)
{
    BOOL                    disp;
    K2STAT                  result;
    UINT32                  bitIndex;
    UINT32                  scanBit;
    UINT32                  r;
    UINT32                  pageCount;
    K2LIST_ANCHOR *         pList;
    K2OSKERN_PHYSTRACK *    pTrack;

    *appRetTrack = NULL;

    if (!KernBit_IsPowerOfTwo(aPow2Bytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

    bitIndex = KernBit_LowestSet_Index(aPow2Bytes);
    if ((bitIndex > 30) || (bitIndex < FIRST_BUCKET_INDEX))
        return K2STAT_ERROR_BAD_ARGUMENT;

    // take away from reservation
    pageCount = aPow2Bytes / K2_VA_MEMPAGE_BYTES;
    do
    {
        r = apRes->mPageCount;
        if (r < pageCount)
            return K2STAT_ERROR_OUT_OF_MEMORY;
    } while (r != K2ATOMIC_CompareExchange(&apRes->mPageCount, r - pageCount, r));

    result = K2STAT_ERROR_OUT_OF_MEMORY;

    disp = K2OSKERN_SeqLock(&gData.Phys.SeqLock);

    do
    {
        pList = &sgPhysFreeList[bitIndex - FIRST_BUCKET_INDEX];
        for (scanBit = bitIndex; scanBit < 31; scanBit++)
        {
            if (0 != pList->mNodeCount)
                break;
            pList++;
        }
        if (scanBit == 31)
            break;

        if (scanBit == bitIndex)
        {
            pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pList->mpHead, ListLink);
            K2LIST_Remove(pList, &pTrack->ListLink);
            pTrack->Flags.Field.Free = 0;
            *appRetTrack = pTrack;
        }
        else
        {
            *appRetTrack = KernPhys_Locked_AllocPow2PagesChunk(scanBit, bitIndex);
        }

        result = K2STAT_NO_ERROR;

    } while (0);

    K2OSKERN_SeqUnlock(&gData.Phys.SeqLock, disp);

    // if failed put back into reservation
    if (K2STAT_IS_ERROR(result))
    {
        do
        {
            r = apRes->mPageCount;
        } while (r != K2ATOMIC_CompareExchange(&apRes->mPageCount, r + pageCount, r));
    }

    return result;
}

void
KernPhys_FreeTrack(
    K2OSKERN_PHYSTRACK *    apTrack
)
{
    BOOL    disp;

    disp = K2OSKERN_SeqLock(&gData.Phys.SeqLock);

    KernPhys_Locked_FreeOneTrack(apTrack);

    K2OSKERN_SeqUnlock(&gData.Phys.SeqLock, disp);
}

void
KernPhys_FreeTrackList(
    K2LIST_ANCHOR * apTrackList
)
{
    BOOL                    disp;
    K2LIST_LINK *           pListLink;
    K2OSKERN_PHYSTRACK *    pTrack;

    if (0 == apTrackList->mNodeCount)
        return;

    pListLink = apTrackList->mpHead;

    disp = K2OSKERN_SeqLock(&gData.Phys.SeqLock);

    do
    {
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pListLink, ListLink);
        pListLink = pListLink->mpNext;

        K2LIST_Remove(apTrackList, &pTrack->ListLink);
        pTrack->Flags.Field.PageListIx = KernPhysPageList_None;
        KernPhys_Locked_FreeOneTrack(pTrack);
    } while (pListLink != NULL);

    K2OSKERN_SeqUnlock(&gData.Phys.SeqLock, disp);
}

void
KernPhys_ScanSet(
    K2OSKERN_PHYSSCAN *     apScan,
    K2OSKERN_PHYSTRACK *    apTrack
)
{
    apScan->mpCurTrack = apTrack;
    apScan->mPhysAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)apTrack);
    apScan->mTrackLeft = 1 << apTrack->Flags.Field.BlockSize;
}

void
KernPhys_ScanInit(
    K2OSKERN_PHYSSCAN * apScan,
    K2LIST_ANCHOR *     apTrackList,
    UINT32              aStartOffset
)
{
    if (NULL == apTrackList->mpHead)
    {
        apScan->mpCurTrack = NULL;
        apScan->mPhysAddr = (UINT32)-1;
        apScan->mTrackLeft = 0;
    }
    else
    {
        KernPhys_ScanSet(apScan, K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, apTrackList->mpHead, ListLink));
        if (aStartOffset > 0)
        {
            aStartOffset *= K2_VA_MEMPAGE_BYTES;

            do
            {
                if (apScan->mTrackLeft > aStartOffset)
                {
                    apScan->mPhysAddr += aStartOffset;
                    apScan->mTrackLeft -= aStartOffset;
                    break;
                }
                
                aStartOffset -= apScan->mTrackLeft;
                
                K2_ASSERT(apScan->mpCurTrack->ListLink.mpNext != NULL);
                
                KernPhys_ScanSet(apScan, K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, apScan->mpCurTrack->ListLink.mpNext, ListLink));

            } while (0 < aStartOffset);
        }
    }
}

UINT32  
KernPhys_ScanIter(
    K2OSKERN_PHYSSCAN *apScan
)
{
    K2OSKERN_PHYSTRACK *    pTrack;
    UINT32                  result;

    K2_ASSERT(0 != apScan->mTrackLeft);

    result = apScan->mPhysAddr;

    apScan->mTrackLeft -= K2_VA_MEMPAGE_BYTES;

    if (0 == apScan->mTrackLeft)
    {
        pTrack = apScan->mpCurTrack;
        if (NULL != pTrack->ListLink.mpNext)
        {
            KernPhys_ScanSet(apScan, K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pTrack->ListLink.mpNext, ListLink));
        }
    }
    else
        apScan->mPhysAddr += K2_VA_MEMPAGE_BYTES;

    return result;
}

void    
KernPhys_ScanToPhysPageArray(
    K2OSKERN_PHYSSCAN * apScan,
    UINT32              aSetLowBits,
    UINT32 *            apArray
)
{
    aSetLowBits &= K2_VA_MEMPAGE_OFFSET_MASK;
    while (apScan->mTrackLeft > 0)
    {
        *apArray = KernPhys_ScanIter(apScan) | aSetLowBits;
        apArray++;
    }
}

void    
KernPhys_ZeroPage(
    UINT32 aPhysAddr
)
{
    UINT32 virtAddr;

    virtAddr = K2OS_KVA_PERCOREWORKPAGES_BASE + ((K2OSKERN_GetCpuIndex() * K2OS_PERCOREWORKPAGES_PERCORE) *  K2_VA_MEMPAGE_BYTES);

    KernPte_MakePageMap(NULL, virtAddr, aPhysAddr, K2OS_MAPTYPE_KERN_DATA);

    K2MEM_Zero((void *)virtAddr, K2_VA_MEMPAGE_BYTES);

    K2OS_CacheOperation(K2OS_CACHEOP_FlushDataRange, virtAddr, K2_VA_MEMPAGE_BYTES);

    KernPte_BreakPageMap(NULL, virtAddr, 0);

    KernArch_InvalidateTlbPageOnCurrentCore(virtAddr);
}

void
KernPhys_CutTrackListIntoPages(
    K2LIST_ANCHOR *         apTrackList,
    BOOL                    aSetProc,
    K2OSKERN_OBJ_PROCESS *  apProc,
    BOOL                    aSetList, 
    KernPhysPageList        aList
)
{
    K2LIST_LINK *       pListLink;
    K2OSKERN_PHYSTRACK *pTrack;
    K2OSKERN_PHYSTRACK *pPageTrack;
    UINT32              bytesLeft;
    K2LIST_ANCHOR       outList;

    pListLink = apTrackList->mpHead;
    if (NULL == pListLink)
        return;

    K2LIST_Init(&outList);

    do
    {
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pListLink, ListLink);
        pListLink = pListLink->mpNext;
        K2LIST_Remove(apTrackList, &pTrack->ListLink);

        K2_ASSERT(0 != pTrack->Flags.Field.Exists);
        K2_ASSERT(0 == pTrack->Flags.Field.Free);

        bytesLeft = 1 << pTrack->Flags.Field.BlockSize;

        pTrack->Flags.Field.BlockSize = K2_VA_MEMPAGE_BYTES_POW2;
        if (aSetList)
            pTrack->Flags.Field.PageListIx = aList;
        if (aSetProc)
            pTrack->mpOwnerProc = apProc;
        bytesLeft -= K2_VA_MEMPAGE_BYTES;
        K2LIST_AddAtTail(&outList, &pTrack->ListLink);

        pPageTrack = pTrack + 1;
        while (bytesLeft > 0)
        {
            pPageTrack->Flags.mAsUINT32 = pTrack->Flags.mAsUINT32;
            pPageTrack->mpOwnerProc = pTrack->mpOwnerProc;  // always copied from first page in range
            K2LIST_AddAtTail(&outList, &pPageTrack->ListLink);
            pPageTrack++;
            bytesLeft -= K2_VA_MEMPAGE_BYTES;
        }

    } while (NULL != pListLink);

    K2MEM_Copy(apTrackList, &outList, sizeof(K2LIST_ANCHOR));
}

BOOL    
KernPhys_Reserve_Init(
    K2OSKERN_PHYSRES *  apRes,
    UINT32              aPageCount
)
{
    UINT32 l;

    if (0 != aPageCount)
    {
        do
        {
            l = gData.Phys.mPagesLeft;
            if (l < aPageCount)
            {
                K2_ASSERT(0);  // want to know
                return FALSE;
            }
            if (l == K2ATOMIC_CompareExchange(&gData.Phys.mPagesLeft, l - aPageCount, l))
                break;
        } while (1);
    }

    K2ATOMIC_Exchange(&apRes->mPageCount, aPageCount);

    return TRUE;
}

BOOL    
KernPhys_Reserve_Add(
    K2OSKERN_PHYSRES *  apRes,
    UINT32              aPageCount
)
{
    UINT32 l;

    if (0 == aPageCount)
        return TRUE;

    do
    {
        l = gData.Phys.mPagesLeft;
        if (l < aPageCount)
            return FALSE;
        if (l == K2ATOMIC_CompareExchange(&gData.Phys.mPagesLeft, l - aPageCount, l))
            break;
    } while (1);

    do
    {
        l = apRes->mPageCount;
    } while (l != K2ATOMIC_CompareExchange(&apRes->mPageCount, l + aPageCount, l));

    return TRUE;
}

void    
KernPhys_Reserve_Release(
    K2OSKERN_PHYSRES *  apRes
)
{
    UINT32 p;
    UINT32 l;

    p = K2ATOMIC_Exchange(&apRes->mPageCount, 0);

    if (0 == p)
        return;

    do
    {
        l = gData.Phys.mPagesLeft;
    } while (l != K2ATOMIC_CompareExchange(&gData.Phys.mPagesLeft, l + p, l));
}

BOOL    
KernPhys_InAllocatableRange(
    UINT32 aPhysBase,
    UINT32 aPageCount
)
{
    UINT32                      endAddr;
    UINT32                      entCount;
    UINT8                       descBuffer[DESC_BUFFER_BYTES];
    K2EFI_MEMORY_DESCRIPTOR *   pDesc = (K2EFI_MEMORY_DESCRIPTOR *)descBuffer;
    UINT8 *                     pWork;
    UINT32                      descBase;
    UINT32                      descEnd;

    K2_ASSERT(aPageCount > 0);

    endAddr = aPhysBase + (aPageCount * K2_VA_MEMPAGE_BYTES) - 1;

    K2_ASSERT(aPhysBase < endAddr);

    // descriptors are in ascending order by physical start
    entCount = gData.mpShared->LoadInfo.mEfiMapSize / gData.mpShared->LoadInfo.mEfiMemDescSize;
    pWork = (UINT8 *)K2OS_KVA_EFIMAP_BASE;
    do
    {
        K2MEM_Copy(pDesc, pWork, gData.mpShared->LoadInfo.mEfiMemDescSize);
        if ((!(pDesc->Attribute & K2EFI_MEMORYFLAG_RUNTIME)) &&
            (pDesc->Type == K2EFI_MEMTYPE_Conventional))
        {
            //
            // this chunk was added to allocatable space. check for overlap with input range
            //
            descBase = (UINT32)pDesc->PhysicalStart;
            if (descBase > endAddr)
            {
                // descriptor starts after end of range, list is sorted, so no overlap
                break;
            }

            if (endAddr > descBase)
            {
                descEnd = descBase + (((UINT32)pDesc->NumberOfPages) * K2_VA_MEMPAGE_BYTES) - 1;
                if (aPhysBase < descEnd)
                {
                    return TRUE;
                }
            }
        }
        pWork += gData.mpShared->LoadInfo.mEfiMemDescSize;
    } while (--entCount > 0);

    return FALSE;
}

K2STAT
KernPhys_GetEfiChunk(
    UINT32                  aInfoIx,
    K2OS_PHYSADDR_RANGE *   apChunkInfo
)
{
    UINT32                      entCount;
    UINT8                       descBuffer[DESC_BUFFER_BYTES];
    K2EFI_MEMORY_DESCRIPTOR *   pDesc = (K2EFI_MEMORY_DESCRIPTOR *)descBuffer;
    UINT8 *                     pWork;
    UINT32                      descBase;
    UINT32                      descEnd;
    UINT32                      nextStart;

    entCount = gData.mpShared->LoadInfo.mEfiMapSize / gData.mpShared->LoadInfo.mEfiMemDescSize;
    K2_ASSERT(1 < entCount);
    pWork = (UINT8 *)K2OS_KVA_EFIMAP_BASE;
    K2MEM_Copy(pDesc, pWork, gData.mpShared->LoadInfo.mEfiMemDescSize);

    descBase = (UINT32)pDesc->PhysicalStart;

    do {
        descEnd = descBase + (pDesc->NumberOfPages * K2_VA_MEMPAGE_BYTES);
        do {
            pWork += gData.mpShared->LoadInfo.mEfiMemDescSize;
            K2MEM_Copy(pDesc, pWork, gData.mpShared->LoadInfo.mEfiMemDescSize);
            nextStart = ((UINT32)pDesc->PhysicalStart);
            if ((0 == nextStart) || (descEnd != nextStart))
                break;
            descEnd += (pDesc->NumberOfPages * K2_VA_MEMPAGE_BYTES);
        } while (0 != --entCount);

        if (0 == aInfoIx)
        {
            apChunkInfo->mBaseAddr = descBase;
            apChunkInfo->mSizeBytes = descEnd - descBase;
            return K2STAT_NO_ERROR;
        }
        --aInfoIx;

        if (0 == entCount)
            break;

        descBase = nextStart;

    } while (0 != descBase);

    return K2STAT_ERROR_NO_MORE_ITEMS;
}

