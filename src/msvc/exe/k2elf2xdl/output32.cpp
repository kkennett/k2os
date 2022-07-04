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

K2STAT
CreateOutputFile32(
    K2ELF32PARSE *  apParse
)
{
    UINT_PTR            loadBase;
    UINT_PTR            loadWork;
    UINT_PTR            fileSectors;
    UINT_PTR            align;
    XDL_FILE_HEADER     workHdr;
    UINT64 *            pSrcPtr;
    UINT_PTR            ixSec;
    Elf32_Shdr *        pSecHdr;
    UINT_PTR *          pSecAddr;
    UINT_PTR            dataEndSectorAligned;
    UINT_PTR            sectorOffset[XDLSegmentIx_Count];

    pSecAddr = new UINT_PTR[apParse->mpRawFileData->e_shnum];
    if (NULL == pSecAddr)
    {
        printf("*** Out of memory\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    K2MEM_Zero(pSecAddr, sizeof(UINT_PTR) * apParse->mpRawFileData->e_shnum);

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
    workHdr.Section[XDLSegmentIx_Text].mLinkAddr = loadWork;
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
        K2_ASSERT(0 == pSecAddr[ixSec]);
        pSecAddr[ixSec] = loadWork;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_link, pSecHdr->sh_size, pSecAddr[ixSec]);
    }
    workHdr.Section[XDLSegmentIx_Text].mMemActualBytes = loadWork - workHdr.Section[XDLSegmentIx_Text].mLinkAddr;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    if (gOut.mUsePlacement)
    {
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    }
    workHdr.Section[XDLSegmentIx_Text].mSectorCount = ((loadWork - workHdr.Section[XDLSegmentIx_Text].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Section[XDLSegmentIx_Text].mSectorCount;

    //
    // read
    //
    printf("READ:\n");
    sectorOffset[XDLSegmentIx_Read] = fileSectors;
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    workHdr.Section[XDLSegmentIx_Read].mLinkAddr = loadWork;
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
        K2_ASSERT(0 == pSecAddr[ixSec]);
        pSecAddr[ixSec] = loadWork;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecAddr[ixSec]);
    }
    workHdr.Section[XDLSegmentIx_Read].mMemActualBytes = loadWork - workHdr.Section[XDLSegmentIx_Read].mLinkAddr;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    if (gOut.mUsePlacement)
    {
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    }
    workHdr.Section[XDLSegmentIx_Read].mSectorCount = ((loadWork - workHdr.Section[XDLSegmentIx_Read].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Section[XDLSegmentIx_Read].mSectorCount;

    //
    // data
    //
    printf("DATA:\n");
    sectorOffset[XDLSegmentIx_Data] = fileSectors;
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    workHdr.Section[XDLSegmentIx_Data].mLinkAddr = loadWork;
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
        K2_ASSERT(0 == pSecAddr[ixSec]);
        pSecAddr[ixSec] = loadWork;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecAddr[ixSec]);
    }
    dataEndSectorAligned = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    workHdr.Section[XDLSegmentIx_Data].mSectorCount = ((dataEndSectorAligned - workHdr.Section[XDLSegmentIx_Data].mLinkAddr) / XDL_SECTOR_BYTES);

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
        K2_ASSERT(0 == pSecAddr[ixSec]);
        pSecAddr[ixSec] = loadWork;
        loadWork += pSecHdr->sh_size;
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecAddr[ixSec]);
    }
    workHdr.Section[XDLSegmentIx_Data].mMemActualBytes = loadWork - workHdr.Section[XDLSegmentIx_Data].mLinkAddr;

    if (gOut.mUsePlacement)
    {
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
        workHdr.Section[XDLSegmentIx_Data].mSectorCount = ((loadWork - workHdr.Section[XDLSegmentIx_Data].mLinkAddr) / XDL_SECTOR_BYTES);
    }
    fileSectors += (UINT_PTR)workHdr.Section[XDLSegmentIx_Data].mSectorCount;

    //
    // everything else is discarable, so align-up to next page boundary regardless of whether in placement mode or not
    //

    //
    // debug info
    //
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    sectorOffset[XDLSegmentIx_DebugInfo] = fileSectors;
    workHdr.Section[XDLSegmentIx_DebugInfo].mLinkAddr = loadWork;
    workHdr.Section[XDLSegmentIx_DebugInfo].mMemActualBytes = 0;
    workHdr.Section[XDLSegmentIx_DebugInfo].mSectorCount = 0;
    fileSectors += (UINT_PTR)workHdr.Section[XDLSegmentIx_Data].mSectorCount;

    //
    // relocations
    //
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    sectorOffset[XDLSegmentIx_Relocs] = fileSectors;
    workHdr.Section[XDLSegmentIx_Relocs].mLinkAddr = loadWork;
    workHdr.Section[XDLSegmentIx_Relocs].mMemActualBytes = 0;
    workHdr.Section[XDLSegmentIx_Relocs].mSectorCount = 0;
    fileSectors += (UINT_PTR)workHdr.Section[XDLSegmentIx_Relocs].mSectorCount;

    //
    // import records
    //
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    sectorOffset[XDLSegmentIx_Imports] = fileSectors;
    workHdr.Section[XDLSegmentIx_Imports].mLinkAddr = loadWork;
    workHdr.Section[XDLSegmentIx_Imports].mMemActualBytes = 0;
    workHdr.Section[XDLSegmentIx_Imports].mSectorCount = 0;
    fileSectors += (UINT_PTR)workHdr.Section[XDLSegmentIx_Imports].mSectorCount;

    //
    // other
    //
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    sectorOffset[XDLSegmentIx_Other] = fileSectors;
    workHdr.Section[XDLSegmentIx_Other].mLinkAddr = loadWork;
    workHdr.Section[XDLSegmentIx_Other].mMemActualBytes = 0;
    workHdr.Section[XDLSegmentIx_Other].mSectorCount = 0;
    fileSectors += (UINT_PTR)workHdr.Section[XDLSegmentIx_Other].mSectorCount;



    printf("plot map:\n");
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecAddr[ixSec]);
    }

    printf("xdlsec map:\n");
    printf(" 0: Placement 0x%08X\n", (UINT_PTR)workHdr.mPlacement);
    for (ixSec = XDLSegmentIx_Text; ixSec < XDLSegmentIx_Count; ixSec++)
    {
        printf("%2d: @%08X z%08X %4d (%08X)\n", 
            ixSec,
            (UINT_PTR)workHdr.Section[ixSec].mLinkAddr, 
            (UINT_PTR)workHdr.Section[ixSec].mMemActualBytes, 
            (UINT_PTR)workHdr.Section[ixSec].mSectorCount, 
            (UINT_PTR)workHdr.Section[ixSec].mSectorCount * XDL_SECTOR_BYTES);
    }



    return K2STAT_NO_ERROR; //  K2STAT_ERROR_NOT_IMPL;
}

