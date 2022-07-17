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

#include "k2elf2xdl.h"

K2_STATIC_ASSERT(0 == (K2_VA_MEMPAGE_BYTES % XDL_SECTOR_BYTES));

struct SymTreeNode
{
    K2TREE_NODE         TreeNode;
    UINT_PTR            mOldIx;
    Elf32_Sym const *   mpSym;
    bool                mUsedInReloc;
    bool                mImported;
};

struct SecTarget
{
    UINT_PTR    mAddr;
    UINT_PTR    mSegmentIx;
};

K2STAT
CreateOutputFile32(
    K2ELF32PARSE *  apParse
)
{
    UINT_PTR                            loadBase;
    UINT_PTR                            loadWork;
    UINT_PTR                            fileSectors;
    UINT_PTR                            align;
    XDL_FILE_HEADER                     workHdr;
    UINT64 *                            pSrcPtr;
    UINT_PTR                            ixSec;
    Elf32_Shdr *                        pSecHdr;
    SecTarget *                         pSecTarget;
    UINT_PTR                            dataEndSectorAligned;
    UINT_PTR                            sectorOffset[XDLSegmentIx_Count];
    XDL_EXPORTS_SEGMENT_HEADER const *  pExpHdr;
    char const *                        pImpName;
    Elf32_Shdr *                        pSymTabSecHdr;
    UINT_PTR                            symCount;
    UINT_PTR                            ixSym;
    Elf32_Sym const *                   pSym;
    Elf32_Shdr *                        pSymStrSecHdr;
    char const *                        pSymStr;
    K2TREE_ANCHOR                       symTree;
    UINT_PTR                            symEntSize;
    UINT_PTR                            symTabSecIx;
    UINT_PTR                            symNamesLen;

    K2TREE_Init(&symTree, TreeStrCompare);

    pSecTarget = new SecTarget[apParse->mpRawFileData->e_shnum];
    if (NULL == pSecTarget)
    {
        printf("*** Out of memory\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    K2MEM_Zero(pSecTarget, sizeof(SecTarget) * apParse->mpRawFileData->e_shnum);
    for (ixSec = 0; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Count;
    }

    K2MEM_Zero(sectorOffset, sizeof(UINT_PTR) * XDLSegmentIx_Count);

    K2MEM_Zero(&workHdr, sizeof(workHdr));
    workHdr.mMarker = XDL_FILE_HEADER_MARKER;

    if (gOut.mUsePlacement)
    {
        loadWork = loadBase = gOut.mPlacement & K2_VA_PAGEFRAME_MASK;
        //
        // file header as whole page
        //
        workHdr.mPlacement = loadBase + K2_FIELDOFFSET(XDL_FILE_HEADER, mPlacement);
        workHdr.mFirstSectionSectorOffset = fileSectors = (K2_VA_MEMPAGE_BYTES / XDL_SECTOR_BYTES);
        loadWork += K2_VA_MEMPAGE_BYTES;
    }
    else
    {
        loadWork = loadBase = 0x10000000;
        workHdr.mFirstSectionSectorOffset = fileSectors = 1;
    }
    workHdr.mHeaderSizeBytes = sizeof(XDL_FILE_HEADER);
    workHdr.mElfClass = apParse->mpRawFileData->e_ident[EI_CLASS];
    workHdr.mElfMachine = (UINT8)apParse->mpRawFileData->e_machine;
    if (gOut.mSpecKernel)
        workHdr.mFlags = XDL_FILE_HEADER_FLAG_KERNEL_ONLY;
    if (gOut.mSpecStack)
        workHdr.mEntryStackReq = gOut.mSpecStack;
    K2MEM_Copy(&workHdr.Id, &gOut.mpElfAnchor->Id, sizeof(K2_GUID128));

    pSrcPtr = (UINT64 *)&gOut.mpElfAnchor->mAnchor[0];
    for (ixSec = 0; ixSec < XDLExportType_Count; ixSec++)
    {
        if (0 != pSrcPtr[ixSec] != 0)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gOut.mElfRoSecIx * apParse->mSectionHeaderTableEntryBytes));
            K2_ASSERT(0 != pSecHdr->sh_size);
            workHdr.mReadExpOffset[ixSec] = pSrcPtr[ixSec] - ((UINT64)pSecHdr->sh_addr);
        }
    }
    workHdr.mNameLen = gOut.mTargetNameLen;
    K2MEM_Copy(workHdr.mName, gOut.mTargetName, gOut.mTargetNameLen);

    //
    // now get sizes of all XDL sections
    // 

    //
    // text
    //
    printf("TEXT:\n");
    sectorOffset[XDLSegmentIx_Text] = fileSectors;
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    workHdr.Segment[XDLSegmentIx_Text].mLinkAddr = loadWork;
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 == (pSecHdr->sh_flags & SHF_ALLOC)) ||
            (0 == pSecHdr->sh_size) ||
            (SHT_PROGBITS != pSecHdr->sh_type))
            continue;
        if (0 == (pSecHdr->sh_flags & SHF_EXECINSTR))
            continue;
        if (0 != (pSecHdr->sh_flags & SHF_WRITE))
        {
            printf("*** Input Elf file has non-read-only text section!\n");
            return K2STAT_ERROR_CORRUPTED;
        }
        if (XDL_ELF_SHF_TYPE_IMPORTS == (pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
            continue;
        K2_ASSERT(pSecHdr->sh_addralign != 0);
        align = pSecHdr->sh_addralign;
        loadWork = ((loadWork + align - 1) / align) * align;
        K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
        pSecTarget[ixSec].mAddr = loadWork;
        pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Text;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_link, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    workHdr.Segment[XDLSegmentIx_Text].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Text].mLinkAddr;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    if (gOut.mUsePlacement)
    {
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    }
    workHdr.Segment[XDLSegmentIx_Text].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Text].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Text].mSectorCount;

    //
    // read
    //
    printf("READ:\n");
    sectorOffset[XDLSegmentIx_Read] = fileSectors;
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    workHdr.Segment[XDLSegmentIx_Read].mLinkAddr = loadWork;
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 == (pSecHdr->sh_flags & SHF_ALLOC)) ||
            (0 == pSecHdr->sh_size) ||
            (SHT_PROGBITS != pSecHdr->sh_type))
            continue;
        if (0 != (pSecHdr->sh_flags & SHF_EXECINSTR))
            continue;
        if (0 != (pSecHdr->sh_flags & SHF_WRITE))
            continue;
        if (XDL_ELF_SHF_TYPE_IMPORTS == (pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
            continue;
        K2_ASSERT(pSecHdr->sh_addralign != 0);
        align = pSecHdr->sh_addralign;
        loadWork = ((loadWork + align - 1) / align) * align;
        K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
        pSecTarget[ixSec].mAddr = loadWork;
        pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Read;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    workHdr.Segment[XDLSegmentIx_Read].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Read].mLinkAddr;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    if (gOut.mUsePlacement)
    {
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    }
    workHdr.Segment[XDLSegmentIx_Read].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Read].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Read].mSectorCount;

    //
    // data
    //
    printf("DATA:\n");
    sectorOffset[XDLSegmentIx_Data] = fileSectors;
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    workHdr.Segment[XDLSegmentIx_Data].mLinkAddr = loadWork;
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 == (pSecHdr->sh_flags & SHF_ALLOC)) ||
            (0 == pSecHdr->sh_size) ||
            (SHT_PROGBITS != pSecHdr->sh_type))
            continue;
        if (0 != (pSecHdr->sh_flags & SHF_EXECINSTR))
            continue;
        if (0 == (pSecHdr->sh_flags & SHF_WRITE))
            continue;
        if (XDL_ELF_SHF_TYPE_IMPORTS == (pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
            continue;

        K2_ASSERT(pSecHdr->sh_addralign != 0);
        align = pSecHdr->sh_addralign;
        loadWork = ((loadWork + align - 1) / align) * align;
        K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
        pSecTarget[ixSec].mAddr = loadWork;
        pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Data;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    dataEndSectorAligned = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    workHdr.Segment[XDLSegmentIx_Data].mSectorCount = ((dataEndSectorAligned - workHdr.Segment[XDLSegmentIx_Data].mLinkAddr) / XDL_SECTOR_BYTES);

    printf("DATA(BSS):\n");
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 == (pSecHdr->sh_flags & SHF_ALLOC)) ||
            (0 == pSecHdr->sh_size) ||
            (SHT_NOBITS != pSecHdr->sh_type))
            continue;
        if (0 != (pSecHdr->sh_flags & SHF_EXECINSTR))
            continue;
        if (0 == (pSecHdr->sh_flags & SHF_WRITE))
            continue;
        if (XDL_ELF_SHF_TYPE_IMPORTS == (pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
            continue;
        K2_ASSERT(pSecHdr->sh_addralign != 0);
        align = pSecHdr->sh_addralign;
        loadWork = ((loadWork + align - 1) / align) * align;
        K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
        pSecTarget[ixSec].mAddr = loadWork;
        pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Data;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    workHdr.Segment[XDLSegmentIx_Data].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Data].mLinkAddr;

    if (gOut.mUsePlacement)
    {
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
        workHdr.Segment[XDLSegmentIx_Data].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Data].mLinkAddr) / XDL_SECTOR_BYTES);
    }
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mSectorCount;

    //
    // everything else is discarable, so align-up to next page boundary regardless of whether in placement mode or not
    //

    //
    // symbol table
    //
    printf("SYMBOLS:\n");
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    sectorOffset[XDLSegmentIx_Symbols] = fileSectors;
    workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr = loadWork;
    pSymTabSecHdr = NULL;
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 != pSecHdr->sh_size) &&
            (SHT_SYMTAB != pSecHdr->sh_type))
            continue;
        if (NULL != pSymTabSecHdr)
        {
            printf("*** Input ELF file has more than one symbol table, which is not allowed\n");
            return K2STAT_ERROR_CORRUPTED;
        }
        pSymTabSecHdr = pSecHdr;
        symTabSecIx = ixSec;
        pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Symbols;
    }
    if (NULL == pSymTabSecHdr)
    {
        printf("*** Elf file has no symbol table\n");
        return K2STAT_ERROR_CORRUPTED;
    }
    printf("%2d: %4d f%08X l%08X z%08X\n", symTabSecIx, pSymTabSecHdr->sh_type, pSymTabSecHdr->sh_flags, pSymTabSecHdr->sh_addr, pSymTabSecHdr->sh_size);
    pSymStrSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (pSymTabSecHdr->sh_link * apParse->mSectionHeaderTableEntryBytes));
    pSymStr = ((char const *)apParse->mpRawFileData) + pSymStrSecHdr->sh_offset;
    symEntSize = pSymTabSecHdr->sh_entsize;
    if (0 == symEntSize)
        symEntSize = sizeof(Elf32_Sym);
    symCount = pSymTabSecHdr->sh_size / symEntSize;
    pSym = (Elf32_Sym const *)(((UINT8 const *)apParse->mpRawFileData) + pSymTabSecHdr->sh_offset);
    symNamesLen = 1;
    for (ixSym = 0; ixSym < symCount; ixSym++)
    {
        if (0 != pSym->st_name)
        {
            symNamesLen += 1 + K2ASC_Len(pSymStr + pSym->st_name);
        }
        pSym = (Elf32_Sym const *)(((UINT8 const *)pSym) + symEntSize);
    }
    workHdr.Segment[XDLSegmentIx_Symbols].mMemActualBytes = (symCount * sizeof(Elf32_Sym)) + symNamesLen;
    loadWork += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mMemActualBytes;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    workHdr.Segment[XDLSegmentIx_Symbols].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Read].mSectorCount;

    //
    // relocations
    // sh_link is symbol table to use
    // sh_info is target section
    //
    printf("RELOCS:\n");
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    sectorOffset[XDLSegmentIx_Relocs] = fileSectors;
    workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr = loadWork;

    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 != (pSecHdr->sh_flags & SHF_ALLOC)) ||
            (0 == pSecHdr->sh_size) ||
            (SHT_REL != pSecHdr->sh_type))
            continue;
        K2_ASSERT(0 == (pSecHdr->sh_flags & SHF_EXECINSTR));
        K2_ASSERT(0 == (pSecHdr->sh_flags & SHF_WRITE));
        K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
        pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Relocs;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size);
    }
    workHdr.Segment[XDLSegmentIx_Read].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Read].mLinkAddr;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;


    workHdr.Segment[XDLSegmentIx_Relocs].mMemActualBytes = 0;
    workHdr.Segment[XDLSegmentIx_Relocs].mSectorCount = 0;
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mSectorCount;

    //
    // import records
    //
    printf("IMPORTS:\n");
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    sectorOffset[XDLSegmentIx_Imports] = fileSectors;
    workHdr.Segment[XDLSegmentIx_Imports].mLinkAddr = loadWork;
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 == (pSecHdr->sh_flags & SHF_ALLOC)) ||
            (0 == pSecHdr->sh_size) ||
            (SHT_PROGBITS != pSecHdr->sh_type))
            continue;
        if (XDL_ELF_SHF_TYPE_IMPORTS != (pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
            continue;
        K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
        pSecTarget[ixSec].mAddr = (UINT_PTR)workHdr.Segment[XDLSegmentIx_Imports].mLinkAddr;  // all imports 'placed' at the start of the imports section
        pExpHdr = (XDL_EXPORTS_SEGMENT_HEADER const *)LoadAddrToDataPtr32(apParse, pSecHdr->sh_addr, NULL);
        pImpName = ((char const *)pExpHdr) +
            sizeof(XDL_EXPORTS_SEGMENT_HEADER) +
            (pExpHdr->mCount * sizeof(XDL_EXPORT32_REF)) +
            sizeof(K2_GUID128);
        workHdr.mImportCount++;
        loadWork += sizeof(XDL_IMPORT) - 4;
        loadWork += (K2ASC_Len(pImpName) + 4) & ~3;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    workHdr.Segment[XDLSegmentIx_Imports].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Imports].mLinkAddr;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    workHdr.Segment[XDLSegmentIx_Imports].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Imports].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Imports].mSectorCount;

    //
    // other
    //
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    sectorOffset[XDLSegmentIx_Other] = fileSectors;
    workHdr.Segment[XDLSegmentIx_Other].mLinkAddr = loadWork;
    workHdr.Segment[XDLSegmentIx_Other].mMemActualBytes = 0;
    workHdr.Segment[XDLSegmentIx_Other].mSectorCount = 0;
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Other].mSectorCount;



    printf("plot map:\n");
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }

    printf("xdlsec map:\n");
    printf(" 0: Placement 0x%08X\n", (UINT_PTR)workHdr.mPlacement);
    for (ixSec = XDLSegmentIx_Text; ixSec < XDLSegmentIx_Count; ixSec++)
    {
        printf("%2d: @%08X z%08X %4d (%08X)\n", 
            ixSec,
            (UINT_PTR)workHdr.Segment[ixSec].mLinkAddr, 
            (UINT_PTR)workHdr.Segment[ixSec].mMemActualBytes, 
            (UINT_PTR)workHdr.Segment[ixSec].mSectorCount, 
            (UINT_PTR)workHdr.Segment[ixSec].mSectorCount * XDL_SECTOR_BYTES);
    }



    return K2STAT_NO_ERROR; //  K2STAT_ERROR_NOT_IMPL;
}

#if 0
    for (ixSym = 0; ixSym < symCount; ixSym++)
    {
        if (STB_LOCAL != ELF32_ST_BIND(pSym->st_info))
        {
            if (0 != pSym->st_name)
            {
                pSymNode = new SymTreeNode;
                if (NULL == pSymNode)
                {
                    printf("*** out of memory\n");
                    return K2STAT_ERROR_OUT_OF_MEMORY;
                }
                K2MEM_Zero(pSymNode, sizeof(SymTreeNode));
                pSymNode->mOldIx = ixSym;
                pSymNode->mpSym = pSym;
                pSymNode->TreeNode.mUserVal = (UINT_PTR)(pSymStr + pSym->st_name);
                K2TREE_Insert(&symTree, pSymNode->TreeNode.mUserVal, &pSymNode->TreeNode);
            }
            else
            {
                printf("!!! Nameless global symbol at symbol table index %d\n", ixSym);
            }
        }
        pSym = (Elf32_Sym const *)(((UINT8 const *)pSym) + symEntSize);
    }

    //
    // find all symbols used for relocation
    //
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 != pSecHdr->sh_size) &&
            (SHT_REL != pSecHdr->sh_type))
            continue;
        K2_ASSERT(pSecHdr->sh_link == symTabSecIx);
        align = pSecHdr->sh_entsize;
        if (0 == align)
            align = sizeof(Elf32_Rel);
        relCount = pSecHdr->sh_size / align;
        if (0 == relCount)
            continue;
        pRel = (Elf32_Rel const *)(((UINT8 const *)apParse->mpRawFileData) + pSecHdr->sh_offset);
        for (ixRel = 0; ixRel < relCount; ixRel++)
        {
            ixSym = ELF32_R_SYM(pRel->r_info);
            if (0 != ixSym)
            {
                pSym = (Elf32_Sym const *)(((UINT8 const *)apParse->mpRawFileData) + pSymTabSecHdr->sh_offset + (symEntSize * ixSym));
                if (0 != pSym->st_name)
                {
                    pTreeNode = K2TREE_Find(&symTree, (UINT_PTR)(pSymStr + pSym->st_name));
                    if (NULL != pTreeNode)
                    {
                        pSymNode = K2_GET_CONTAINER(SymTreeNode, pTreeNode, TreeNode);
                        pSymNode->mUsedInReloc = true;
                        if ((0 != pSym->st_shndx) &&
                            (pSym->st_shndx < apParse->mpRawFileData->e_shnum))
                        {
                            pTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (pSym->st_shndx * apParse->mSectionHeaderTableEntryBytes));
                            if ((pTargetSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK) == XDL_ELF_SHF_TYPE_IMPORTS)
                            {
                                pSymNode->mImported = true;
                            }
                        }
                    }
                }
            }
            pRel = (Elf32_Rel const *)(((UINT8 const *)pRel) + align);
        }
    }

    printf("Symbols used in relocations:\n");
    pTreeNode = K2TREE_FirstNode(&symTree);
    do {
        pSymNode = K2_GET_CONTAINER(SymTreeNode, pTreeNode, TreeNode);
        if (pSymNode->mUsedInReloc)
        {
            if (pSymNode->mImported)
            {
                pTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (pSymNode->mpSym->st_shndx * apParse->mSectionHeaderTableEntryBytes));
                UINT8 const *pImpDetail = LoadAddrToDataPtr32(apParse, pTargetSecHdr->sh_addr, NULL);
                XDL_EXPORTS_SEGMENT_HEADER const * pExpSecHdr = (XDL_EXPORTS_SEGMENT_HEADER const *)pImpDetail;
                pImpDetail += sizeof(XDL_EXPORTS_SEGMENT_HEADER);
                pImpDetail += pExpSecHdr->mCount * sizeof(XDL_EXPORT32_REF);
                pImpDetail += sizeof(K2_GUID128);
                printf("  IMPORT %6d %s:%s\n",
                    (pSymNode->mpSym->st_value - pTargetSecHdr->sh_addr) / sizeof(XDL_EXPORT32_REF),
                    (char const *)pImpDetail,
                    (char const *)pSymNode->TreeNode.mUserVal);
            }
            else
            {
                if ((0 != pSymNode->mpSym->st_shndx) &&
                    (pSymNode->mpSym->st_shndx < apParse->mpRawFileData->e_shnum))
                {
                    printf("  %08X (%2d) %s\n",
                        pSymNode->mpSym->st_value, pSymNode->mpSym->st_shndx,
                        (char const *)pSymNode->TreeNode.mUserVal);
                }
                else
                {
                    printf("  %08X      %s\n", pSymNode->mpSym->st_value,
                        (char const *)pSymNode->TreeNode.mUserVal);
                }
            }
        }
        pTreeNode = K2TREE_NextNode(&symTree, pTreeNode);
    } while (NULL != pTreeNode);
#endif

    
