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
    UINT_PTR    mSegOffset;
    UINT8 *     mpData;
};

struct SymTrack
{
    UINT_PTR            mInIx;
    Elf32_Sym const *   mpIn;
    UINT_PTR            mOutIx;
    Elf32_Sym *         mpOut;
    bool                mImported;
    bool                mUsedInReloc;
};

void
RelocOne32_X32(
    UINT_PTR            aRelType,
    UINT_PTR            aOldTargetAddr,
    UINT_PTR            aNewTargetAddr,
    UINT8 *             apTarget,
    Elf32_Sym const *   apSrcSym,
    UINT_PTR            aNewSymValue
)
{
    UINT32 valAtTarget;

    //    printf("Reloc %d: @0x%08X->@0x%08X (data 0x%08X) sym %08X->%08X)\n", aRelType, aOldTargetAddr, aNewTargetAddr, (UINT_PTR)apTarget, apSrcSym->st_value, aNewSymValue);

    K2MEM_Copy(&valAtTarget, apTarget, sizeof(UINT32));

    switch (aRelType)
    {
    case R_386_32:
        // value at target is address of symbol + some addend
        valAtTarget -= apSrcSym->st_value;
        valAtTarget += aNewSymValue;
        break;

    case R_386_PC32:
        // value at target is offset from 4 bytes
        // after target to symbol value + some addend
        valAtTarget -= (apSrcSym->st_value - (aOldTargetAddr + 4));
        valAtTarget += (aNewSymValue - (aNewTargetAddr + 4));
        break;

    default:
        printf("**** UNKNOWN RELOCATION TYPE FOUND\n");
        break;
    }

    K2MEM_Copy(apTarget, &valAtTarget, sizeof(UINT32));
}

void
RelocOne32_A32(
    UINT_PTR            aRelType,
    UINT_PTR            aOldTargetAddr,
    UINT_PTR            aNewTargetAddr,
    UINT8 *             apTarget,
    Elf32_Sym const *   apSrcSym,
    UINT_PTR            aNewSymValue
)
{
//    printf("Reloc %d: @0x%08X->@0x%08X (data 0x%08X) sym %08X->%08X)\n", aRelType, aOldTargetAddr, aNewTargetAddr, (UINT_PTR)apTarget, apSrcSym->st_value, aNewSymValue);

    UINT32 valAtTarget;
    UINT32 targetAddend;

    K2MEM_Copy(&valAtTarget, apTarget, sizeof(UINT32));
    switch (aRelType)
    {
    case R_ARM_PC24:
    case R_ARM_CALL:
    case R_ARM_JUMP24:
        targetAddend = (valAtTarget & 0xFFFFFF) << 2;
        if (targetAddend & 0x2000000)
            targetAddend |= 0xFC000000;

        targetAddend -= (apSrcSym->st_value - (aOldTargetAddr + 8));
        targetAddend += (aNewSymValue - (aNewTargetAddr + 8));

        targetAddend /= sizeof(UINT32);
        if (targetAddend & 0x80000000)
        {
            if ((targetAddend & 0xFF000000) != 0xFF000000)
            {
                printf("*** 24-bit reloc blows range (-)\n");
            }
        }
        else
        {
            if ((targetAddend & 0xFF000000) != 0)
            {
                printf("*** 24-bit reloc blows range (+)\n");
            }
        }
        valAtTarget = (valAtTarget & 0xFF000000) | (targetAddend & 0xFFFFFF);

        break;

    case R_ARM_ABS32:
        valAtTarget -= apSrcSym->st_value;
        valAtTarget += aNewSymValue;
        break;

    case R_ARM_MOVW_ABS_NC:
        targetAddend = ((valAtTarget & 0xF0000) >> 4) | (valAtTarget & 0xFFF);
        targetAddend -= (apSrcSym->st_value & 0xFFFF);
        targetAddend += (aNewSymValue & 0xFFFF);
        valAtTarget = ((valAtTarget) & 0xFFF0F000) | ((targetAddend << 4) & 0xF0000) | (targetAddend & 0xFFF);
        break;

    case R_ARM_MOVT_ABS:
        targetAddend = ((valAtTarget & 0xF0000) >> 4) | (valAtTarget & 0xFFF);
        targetAddend -= ((apSrcSym->st_value >> 16) & 0xFFFF);
        targetAddend += ((aNewSymValue >> 16) & 0xFFFF);
        valAtTarget = ((valAtTarget) & 0xFFF0F000) | ((targetAddend << 4) & 0xF0000) | (targetAddend & 0xFFF);
        break;

    case R_ARM_PREL31:
        targetAddend = valAtTarget + apSrcSym->st_value - aNewTargetAddr;
        valAtTarget = targetAddend & 0x7FFFFFFF;
        break;

    default:
        printf("**** UNKNOWN RELOCATION TYPE FOUND\n");
        break;
    }

    K2MEM_Copy(apTarget, &valAtTarget, sizeof(UINT32));
}

void
Reloc32(
    K2ELF32PARSE *  apParse,
    SecTarget *     apSecTarget,
    Elf32_Shdr *    apSecHdr,
    UINT_PTR        aNewSecBase,
    UINT8 *         apNewSecData,
    Elf32_Shdr *    apSymTabSecHdr,
    Elf32_Shdr *    apRelTabSecHdr
)
{
    UINT8 const *       pSymTable;
    Elf32_Sym const *   pSym;
    UINT_PTR            symEntSize;
    UINT_PTR            symSec;
    Elf32_Rel const *   pRel;
    UINT_PTR            relEntSize;
    UINT_PTR            relCount;
    UINT_PTR            targetOffset;
    UINT_PTR            newSymVal;
    Elf32_Shdr *        pSrcSecHdr;
    UINT8 *             pRelTarget;

    symEntSize = apSymTabSecHdr->sh_entsize;
    if (0 == symEntSize)
        symEntSize = sizeof(Elf32_Sym);
    pSymTable = LoadOffsetToDataPtr32(apParse, apSymTabSecHdr->sh_offset, NULL);
    relEntSize = apRelTabSecHdr->sh_entsize;
    if (0 == relEntSize)
        relEntSize = sizeof(Elf32_Rel);
    relCount = apRelTabSecHdr->sh_size / relEntSize;
    if (0 == relCount)
        return;
    pRel = (Elf32_Rel const *)LoadOffsetToDataPtr32(apParse, apRelTabSecHdr->sh_offset, NULL);
    do {
        targetOffset = pRel->r_offset - apSecHdr->sh_addr;

        pRelTarget = apNewSecData + targetOffset;

        pSym = (Elf32_Sym const *)(pSymTable + (symEntSize * ELF32_R_SYM(pRel->r_info)));
        
        symSec = pSym->st_shndx;
        
        newSymVal = pSym->st_value;
        if ((symSec > 0) &&
            (symSec < apParse->mpRawFileData->e_shnum))
        {
            pSrcSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (symSec * apParse->mSectionHeaderTableEntryBytes));
            newSymVal -= pSrcSecHdr->sh_addr;
            newSymVal += apSecTarget[symSec].mAddr;
        }

        if (apParse->mpRawFileData->e_machine == EM_X32)
            RelocOne32_X32(ELF32_R_TYPE(pRel->r_info), pRel->r_offset, aNewSecBase + targetOffset, pRelTarget, pSym, newSymVal);
        else
            RelocOne32_A32(ELF32_R_TYPE(pRel->r_info), pRel->r_offset, aNewSecBase + targetOffset, pRelTarget, pSym, newSymVal);

        pRel = (Elf32_Rel const *)(((UINT8 const *)pRel) + relEntSize);
    } while (--relCount);
}

K2STAT
CreateOutputFile32(
    K2ELF32PARSE *  apParse
)
{
    static char const * const sSegName[] = { "Hedr", "Text", "Read", "Data", "Symb", "Relo", "Othr" };
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
    UINT_PTR                            symInCount;
    UINT_PTR                            symOutCount;
    UINT_PTR                            ixSym;
    Elf32_Sym const *                   pSym;
    Elf32_Shdr *                        pSymStrSecHdr;
    char const *                        pSymStr;
    K2TREE_ANCHOR                       symTree;
    UINT_PTR                            symEntSize;
    UINT_PTR                            symTabSecIx;
    UINT_PTR                            symNamesLen;
    UINT64                              outOffset;
    XDL_FILE_HEADER *                   pOutput;
    UINT8 *                             pOutWork;
    UINT8 *                             pBase;
    XDL_RELOC_SEGMENT_HEADER            relSegHdr;
    Elf32_Shdr *                        pRelTargetSecHdr;
    char *                              pStrOut;
    SymTrack *                          pSymTrack;
    UINT_PTR                            symOutIx;
    Elf32_Rel *                         pRel;
    UINT_PTR                            ixRel;
    UINT_PTR                            relCount;
    UINT_PTR                            relBytes;
    XDL_IMPORT *                        pImport;
    UINT_PTR                            importsCount;

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

    fileSectors = 0;

    K2MEM_Zero(sectorOffset, sizeof(UINT_PTR) * XDLSegmentIx_Count);

    K2MEM_Zero(&workHdr, sizeof(workHdr));

    workHdr.mMarker = XDL_FILE_HEADER_MARKER;

    if (gOut.mUsePlacement)
    {
        loadWork = loadBase = gOut.mPlacement & K2_VA_PAGEFRAME_MASK;
    }
    else
    {
        loadWork = loadBase = 0x10000000;
    }

    workHdr.mHeaderBytes = sizeof(XDL_FILE_HEADER);

    workHdr.mImportsOffset = ((workHdr.mHeaderBytes + (sizeof(XDL_IMPORT) - 1)) / sizeof(XDL_IMPORT)) * sizeof(XDL_IMPORT);

    workHdr.mElfClass = apParse->mpRawFileData->e_ident[EI_CLASS];

    workHdr.mElfMachine = (UINT8)apParse->mpRawFileData->e_machine;

    if (gOut.mSpecKernel)
        workHdr.mFlags = XDL_FILE_HEADER_FLAG_KERNEL_ONLY;

    if (gOut.mSpecStack)
        workHdr.mEntryStackReq = gOut.mSpecStack;

    workHdr.mNameLen = gOut.mTargetNameLen;
    K2MEM_Copy(workHdr.mName, gOut.mTargetName, gOut.mTargetNameLen);

    K2MEM_Copy(&workHdr.Id, &gOut.mpElfAnchor->Id, sizeof(K2_GUID128));

    //
    // count imports
    //
    importsCount = 0;
    sectorOffset[XDLSegmentIx_Header] = fileSectors;
    workHdr.Segment[XDLSegmentIx_Header].mLinkAddr = loadWork;
    loadWork += workHdr.mImportsOffset;
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
        if (gOut.mUsePlacement)
        {
            printf("*** Files using placement cannot have imports, and an import record was found\n");
            return K2STAT_ERROR_NOT_ALLOWED;
        }
        pExpHdr = (XDL_EXPORTS_SEGMENT_HEADER const *)LoadAddrToDataPtr32(apParse, pSecHdr->sh_addr, NULL);
        pImpName = ((char const *)pExpHdr) +
            sizeof(XDL_EXPORTS_SEGMENT_HEADER) +
            (pExpHdr->mCount * sizeof(XDL_EXPORT32_REF)) +
            sizeof(K2_GUID128);
        importsCount++;
        pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Header;
        pSecTarget[ixSec].mAddr = loadBase;
        pSecTarget[ixSec].mSegOffset = loadWork - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Header].mLinkAddr;
        loadWork += sizeof(XDL_IMPORT);
//        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    workHdr.Segment[XDLSegmentIx_Header].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Header].mLinkAddr;
    if (gOut.mUsePlacement)
    {
        //
        // page align segment end before sectorcount calculation
        //
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    }
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    workHdr.Segment[XDLSegmentIx_Header].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Header].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Header].mSectorCount;

    K2_ASSERT(importsCount == (UINT_PTR)((workHdr.Segment[XDLSegmentIx_Header].mMemActualBytes - workHdr.mImportsOffset) / sizeof(XDL_IMPORT)));

    //
    // set exports offsets in read-only section
    //
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

    //
    // text
    //
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
        pSecTarget[ixSec].mSegOffset = loadWork - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Text].mLinkAddr;
        loadWork += pSecHdr->sh_size;
//        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_link, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    workHdr.Segment[XDLSegmentIx_Text].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Text].mLinkAddr;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    if (gOut.mUsePlacement)
    {
        //
        // page align segment end before sectorcount calculation
        //
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    }
    workHdr.Segment[XDLSegmentIx_Text].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Text].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Text].mSectorCount;

    //
    // read
    //
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
        pSecTarget[ixSec].mSegOffset = loadWork - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Read].mLinkAddr;
        loadWork += pSecHdr->sh_size;
//        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
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
    // data and bss
    //
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
        pSecTarget[ixSec].mSegOffset = loadWork - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mLinkAddr;
        loadWork += pSecHdr->sh_size;
//        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    dataEndSectorAligned = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    workHdr.Segment[XDLSegmentIx_Data].mSectorCount = ((dataEndSectorAligned - workHdr.Segment[XDLSegmentIx_Data].mLinkAddr) / XDL_SECTOR_BYTES);

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
        pSecTarget[ixSec].mSegOffset = loadWork - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mLinkAddr;
        loadWork += pSecHdr->sh_size;
//        printf("%2d: %4d f%08X l%08X z%08X -- @%08X\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr);
    }
    workHdr.Segment[XDLSegmentIx_Data].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Data].mLinkAddr;

    if (gOut.mUsePlacement)
    {
        loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
        workHdr.Segment[XDLSegmentIx_Data].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Data].mLinkAddr) / XDL_SECTOR_BYTES);
    }
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mSectorCount;

    //
    // everything else is discarable, so link addresses don't matter but are loadWork for file creation logistics
    //

    //
    // symbol table
    //
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
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
//    printf("%2d: %4d f%08X l%08X z%08X\n", symTabSecIx, pSymTabSecHdr->sh_type, pSymTabSecHdr->sh_flags, pSymTabSecHdr->sh_addr, pSymTabSecHdr->sh_size);
    pSymStrSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (pSymTabSecHdr->sh_link * apParse->mSectionHeaderTableEntryBytes));
    pSymStr = ((char const *)apParse->mpRawFileData) + pSymStrSecHdr->sh_offset;
    symEntSize = pSymTabSecHdr->sh_entsize;
    if (0 == symEntSize)
        symEntSize = sizeof(Elf32_Sym);
    symInCount = pSymTabSecHdr->sh_size / symEntSize;
    pSym = (Elf32_Sym const *)(((UINT8 const *)apParse->mpRawFileData) + pSymTabSecHdr->sh_offset);
    symNamesLen = 1;
    symOutCount = 0;
    for (ixSym = 0; ixSym < symInCount; ixSym++)
    {
        align = pSym->st_shndx;
        if (0 != align)
        {
            if ((align >= apParse->mpRawFileData->e_shnum) ||
                (pSecTarget[align].mSegmentIx < XDLSegmentIx_Count))
            {
                symOutCount++;
                if (0 != pSym->st_name)
                {
                    symNamesLen += 1 + K2ASC_Len(pSymStr + pSym->st_name);
                }
            }
//            else
//            {
//                printf("tossing symbol with value 0x%08X in unused section 0x%04X %s\n", pSym->st_value, pSym->st_shndx, pSym->st_name ? (pSymStr + pSym->st_name) : "");
//            }
        }
        pSym = (Elf32_Sym const *)(((UINT8 const *)pSym) + symEntSize);
    }
    pSecTarget[symTabSecIx].mAddr = loadWork;
    workHdr.Segment[XDLSegmentIx_Symbols].mMemActualBytes = (symOutCount * sizeof(Elf32_Sym)) + symNamesLen;
    loadWork += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mMemActualBytes;
    loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
    workHdr.Segment[XDLSegmentIx_Symbols].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr) / XDL_SECTOR_BYTES);
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mSectorCount;

    //
    // input file must have relocs even if it is in placement
    //
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 == (pSecHdr->sh_flags & SHF_ALLOC)) &&
            (0 != pSecHdr->sh_size) &&
            (SHT_REL == pSecHdr->sh_type) &&
            (pSecTarget[pSecHdr->sh_info].mSegmentIx != XDLSegmentIx_Count))
        {
            // relocs target a used section
            break;
        }
    }
    if (ixSec == apParse->mpRawFileData->e_shnum)
    {
        printf("*** No targeted relocations found in input file.\n");
        return K2STAT_ERROR_CORRUPTED;
    }

    if (!gOut.mUsePlacement)
    {
        //
        // relocations
        // sh_link is symbol table to use
        // sh_info is target section
        //
        K2MEM_Zero(&relSegHdr, sizeof(relSegHdr));
        loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
        sectorOffset[XDLSegmentIx_Relocs] = fileSectors;
        workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr = loadWork;
        loadWork += sizeof(XDL_RELOC_SEGMENT_HEADER);
        for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
            if ((0 != (pSecHdr->sh_flags & SHF_ALLOC)) ||
                (0 == pSecHdr->sh_size) ||
                (SHT_REL != pSecHdr->sh_type) ||
                (pSecTarget[pSecHdr->sh_info].mSegmentIx == XDLSegmentIx_Count))
                continue;
            K2_ASSERT(0 == (pSecHdr->sh_flags & SHF_EXECINSTR));
            K2_ASSERT(0 == (pSecHdr->sh_flags & SHF_WRITE));
            K2_ASSERT(symTabSecIx == pSecHdr->sh_link);
            pRelTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (pSecHdr->sh_info * apParse->mSectionHeaderTableEntryBytes));
            if (0 == (pRelTargetSecHdr->sh_flags & SHF_EXECINSTR))
                continue;
            pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Relocs;
            K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
            pSecTarget[ixSec].mAddr = loadWork;
            pSecTarget[ixSec].mSegOffset = loadWork - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr;

            relBytes = pSecHdr->sh_entsize;
            if (0 == relBytes)
                relBytes = sizeof(Elf32_Rel);
            relCount = (pSecHdr->sh_size / relBytes) * sizeof(Elf32_Rel);
            loadWork += relCount;
            relSegHdr.mTextRelocsBytes += relCount;
//            printf("%2d: %4d f%08X l%08X z%08X TARGET %d (TEXT)\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecHdr->sh_info);
        }
        for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
            if ((0 != (pSecHdr->sh_flags & SHF_ALLOC)) ||
                (0 == pSecHdr->sh_size) ||
                (SHT_REL != pSecHdr->sh_type) ||
                (pSecTarget[pSecHdr->sh_info].mSegmentIx == XDLSegmentIx_Count))
                continue;
            pRelTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (pSecHdr->sh_info * apParse->mSectionHeaderTableEntryBytes));
            if (0 != (pRelTargetSecHdr->sh_flags & SHF_EXECINSTR))
                continue;
            if (0 != (pRelTargetSecHdr->sh_flags & SHF_WRITE))
                continue;
            pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Relocs;
            K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
            pSecTarget[ixSec].mAddr = loadWork;
            pSecTarget[ixSec].mSegOffset = loadWork - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr;
            relBytes = pSecHdr->sh_entsize;
            if (0 == relBytes)
                relBytes = sizeof(Elf32_Rel);
            relCount = (pSecHdr->sh_size / relBytes) * sizeof(Elf32_Rel);
            loadWork += relCount;
            relSegHdr.mReadRelocsBytes += relCount;
//            printf("%2d: %4d f%08X l%08X z%08X TARGET %d (READ)\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecHdr->sh_info);
        }
        for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
            if ((0 != (pSecHdr->sh_flags & SHF_ALLOC)) ||
                (0 == pSecHdr->sh_size) ||
                (SHT_REL != pSecHdr->sh_type) ||
                (pSecTarget[pSecHdr->sh_info].mSegmentIx == XDLSegmentIx_Count))
                continue;
            pRelTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (pSecHdr->sh_info * apParse->mSectionHeaderTableEntryBytes));
            if (0 != (pRelTargetSecHdr->sh_flags & SHF_EXECINSTR))
                continue;
            if (0 == (pRelTargetSecHdr->sh_flags & SHF_WRITE))
                continue;
            pSecTarget[ixSec].mSegmentIx = XDLSegmentIx_Relocs;
            K2_ASSERT(0 == pSecTarget[ixSec].mAddr);
            pSecTarget[ixSec].mAddr = loadWork;
            pSecTarget[ixSec].mSegOffset = loadWork - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr;
            relBytes = pSecHdr->sh_entsize;
            if (0 == relBytes)
                relBytes = sizeof(Elf32_Rel);
            relCount = (pSecHdr->sh_size / relBytes) * sizeof(Elf32_Rel);
            loadWork += relCount;
            relSegHdr.mDataRelocsBytes += relCount;
//            printf("%2d: %4d f%08X l%08X z%08X TARGET %d (DATA)\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecHdr->sh_info);
        }
        workHdr.Segment[XDLSegmentIx_Relocs].mMemActualBytes = loadWork - workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr;
        loadWork = ((loadWork + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES;
        workHdr.Segment[XDLSegmentIx_Relocs].mSectorCount = ((loadWork - workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr) / XDL_SECTOR_BYTES);
        fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mSectorCount;
    }

    //
    // other stuff goes here
    //
    loadWork = ((loadWork + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    sectorOffset[XDLSegmentIx_Other] = fileSectors;
    workHdr.Segment[XDLSegmentIx_Other].mLinkAddr = loadWork;
    workHdr.Segment[XDLSegmentIx_Other].mMemActualBytes = 0;
    workHdr.Segment[XDLSegmentIx_Other].mSectorCount = 0;
    fileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Other].mSectorCount;

#if 0
    printf("plot map:\n");
    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if (pSecHdr->sh_flags & SHF_ALLOC)
        {
            K2_ASSERT(XDLSegmentIx_Count != pSecTarget[ixSec].mSegmentIx);
        }
        printf("%2d: %4d f%08X l%08X z%08X -- @%08X (%d)\n", ixSec, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr, pSecHdr->sh_size, pSecTarget[ixSec].mAddr, pSecTarget[ixSec].mSegmentIx);
    }

    printf("xdlsec map:\n");
    printf(" Placement @ 0x%08X\n", (UINT_PTR)workHdr.mPlacement);
#endif
    outOffset = 0;
    for (ixSec = XDLSegmentIx_Header; ixSec < XDLSegmentIx_Count; ixSec++)
    {
        K2_ASSERT(sectorOffset[ixSec] == outOffset);
#if 0
        printf("%2d %s: @%08X z%08X @ %4d (%08X) for %4d (%08X) \n",
            ixSec,
            sSegName[ixSec],
            (UINT_PTR)workHdr.Segment[ixSec].mLinkAddr,
            (UINT_PTR)workHdr.Segment[ixSec].mMemActualBytes,
            (UINT_PTR)outOffset,
            (UINT_PTR)(outOffset * XDL_SECTOR_BYTES),
            (UINT_PTR)workHdr.Segment[ixSec].mSectorCount,
            (UINT_PTR)(workHdr.Segment[ixSec].mSectorCount * XDL_SECTOR_BYTES));
#endif
        outOffset += workHdr.Segment[ixSec].mSectorCount;
    }
    K2_ASSERT(fileSectors == outOffset);

    //
    // assemble output file
    //

    pOutput = (XDL_FILE_HEADER *)(new UINT8[fileSectors * XDL_SECTOR_BYTES]);
    if (NULL == pOutput)
    {
        printf("*** Out of memory\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    K2MEM_Zero(pOutput, fileSectors * XDL_SECTOR_BYTES);

    K2MEM_Copy(pOutput, &workHdr, sizeof(workHdr));
    pOutWork = (UINT8 *)pOutput;

    if ((!gOut.mUsePlacement) && (0 != importsCount))
    {
        pImport = (XDL_IMPORT *)(pOutWork + pOutput->mImportsOffset);
        for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
            if ((0 == (pSecHdr->sh_flags & SHF_ALLOC)) ||
                (0 == pSecHdr->sh_size) ||
                (SHT_PROGBITS != pSecHdr->sh_type))
                continue;
            if (XDL_ELF_SHF_TYPE_IMPORTS != (pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
                continue;
            K2_ASSERT(XDLSegmentIx_Header == pSecTarget[ixSec].mSegmentIx);
            K2_ASSERT(pSecTarget[ixSec].mSegOffset == (UINT_PTR)(((UINT8 *)pImport) - pOutWork));
            pSecTarget[ixSec].mpData = pOutWork + pSecTarget[ixSec].mSegOffset;
            K2_ASSERT(((UINT8 *)pImport) == pSecTarget[ixSec].mpData);
            pExpHdr = (XDL_EXPORTS_SEGMENT_HEADER const *)LoadAddrToDataPtr32(apParse, pSecHdr->sh_addr, NULL);
            K2MEM_Copy(&pImport->ExpHdr, pExpHdr, sizeof(XDL_EXPORTS_SEGMENT_HEADER));
            pImpName = ((char const *)pExpHdr) +
                sizeof(XDL_EXPORTS_SEGMENT_HEADER) +
                (pExpHdr->mCount * sizeof(XDL_EXPORT32_REF));
            K2MEM_Copy(&pImport->ID, pImpName, sizeof(K2_GUID128));
            pImpName += sizeof(K2_GUID128);
            pImport->mNameLen = K2ASC_CopyLen(pImport->mName, pImpName, XDL_NAME_MAX_LEN);
            pImport->mName[XDL_NAME_MAX_LEN] = 0;
            pImport->mSectionFlags = pSecHdr->sh_flags;
            pImport++;
        }
    }
    pOutWork += pOutput->Segment[XDLSegmentIx_Header].mSectorCount * XDL_SECTOR_BYTES;

    for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
        if ((0 == (pSecHdr->sh_flags & SHF_ALLOC)) ||
            (0 == pSecHdr->sh_size) ||
            (SHT_PROGBITS != pSecHdr->sh_type))
            continue;
        if (0 == (pSecHdr->sh_flags & SHF_EXECINSTR))
            continue;
        if (XDL_ELF_SHF_TYPE_IMPORTS == (pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
            continue;
        K2_ASSERT(XDLSegmentIx_Text == pSecTarget[ixSec].mSegmentIx);
        K2_ASSERT(pSecTarget[ixSec].mSegOffset == (UINT_PTR)(((UINT64)pSecTarget[ixSec].mAddr) - pOutput->Segment[XDLSegmentIx_Text].mLinkAddr));

        pSecTarget[ixSec].mpData = pOutWork + pSecTarget[ixSec].mSegOffset;
        K2MEM_Copy(pSecTarget[ixSec].mpData, LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL), pSecHdr->sh_size);

        if ((apParse->mpRawFileData->e_entry >= pSecHdr->sh_addr) &&
            ((apParse->mpRawFileData->e_entry - pSecHdr->sh_addr) < pSecHdr->sh_size))
        {
            K2_ASSERT(0 == workHdr.mEntryPoint);
            workHdr.mEntryPoint = (UINT_PTR)((apParse->mpRawFileData->e_entry - pSecHdr->sh_addr) + pSecTarget[ixSec].mAddr);
//            printf("Translating entry point from 0x%08X to 0x%08X\n", apParse->mpRawFileData->e_entry, (UINT_PTR)workHdr.mEntryPoint);
        }

        if (pSecHdr->sh_addr != pSecTarget[ixSec].mAddr)
        {
            //
            // find source relocs for this section and relocate it to the target
            //
            for (ixRel = 1; ixRel < apParse->mpRawFileData->e_shnum; ixRel++)
            {
                pRelTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixRel * apParse->mSectionHeaderTableEntryBytes));
                if (pRelTargetSecHdr->sh_type != SHT_REL)
                    continue;
                if (pRelTargetSecHdr->sh_info != ixSec)
                    continue;
                Reloc32(apParse, pSecTarget, pSecHdr, pSecTarget[ixSec].mAddr, pSecTarget[ixSec].mpData, pSymTabSecHdr, pRelTargetSecHdr);
            }
        }
    }
    if (0 == workHdr.mEntryPoint)
    {
        printf("*** ELF input file entrypoint not found inside a usable text section\n");
        return K2STAT_ERROR_CORRUPTED;
    }
    pOutWork += pOutput->Segment[XDLSegmentIx_Text].mSectorCount * XDL_SECTOR_BYTES;

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
        K2_ASSERT(XDLSegmentIx_Read == pSecTarget[ixSec].mSegmentIx);
        K2_ASSERT(pSecTarget[ixSec].mSegOffset == (UINT_PTR)(((UINT64)pSecTarget[ixSec].mAddr) - pOutput->Segment[XDLSegmentIx_Read].mLinkAddr));

        pSecTarget[ixSec].mpData = pOutWork + pSecTarget[ixSec].mSegOffset;
        K2MEM_Copy(pSecTarget[ixSec].mpData, LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL), pSecHdr->sh_size);

        if (pSecHdr->sh_addr != pSecTarget[ixSec].mAddr)
        {
            //
            // find source relocs for this section and relocate it to the target
            //
            for (ixRel = 1; ixRel < apParse->mpRawFileData->e_shnum; ixRel++)
            {
                pRelTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixRel * apParse->mSectionHeaderTableEntryBytes));
                if (pRelTargetSecHdr->sh_type != SHT_REL)
                    continue;
                if (pRelTargetSecHdr->sh_info != ixSec)
                    continue;
                Reloc32(apParse, pSecTarget, pSecHdr, pSecTarget[ixSec].mAddr, pSecTarget[ixSec].mpData, pSymTabSecHdr, pRelTargetSecHdr);
            }
        }
    }
    pOutWork += pOutput->Segment[XDLSegmentIx_Read].mSectorCount * XDL_SECTOR_BYTES;

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
        K2_ASSERT(XDLSegmentIx_Data == pSecTarget[ixSec].mSegmentIx);
        K2_ASSERT(pSecTarget[ixSec].mSegOffset == (UINT_PTR)(((UINT64)pSecTarget[ixSec].mAddr) - pOutput->Segment[XDLSegmentIx_Data].mLinkAddr));

        pSecTarget[ixSec].mpData = pOutWork + pSecTarget[ixSec].mSegOffset;
        K2MEM_Copy(pSecTarget[ixSec].mpData, LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL), pSecHdr->sh_size);

        if (pSecHdr->sh_addr != pSecTarget[ixSec].mAddr)
        {
            //
            // find source relocs for this section and relocate it to the target
            //
            for (ixRel = 1; ixRel < apParse->mpRawFileData->e_shnum; ixRel++)
            {
                pRelTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixRel * apParse->mSectionHeaderTableEntryBytes));
                if (pRelTargetSecHdr->sh_type != SHT_REL)
                    continue;
                if (pRelTargetSecHdr->sh_info != ixSec)
                    continue;
                Reloc32(apParse, pSecTarget, pSecHdr, pSecTarget[ixSec].mAddr, pSecTarget[ixSec].mpData, pSymTabSecHdr, pRelTargetSecHdr);
            }
        }
    }
    pOutWork += pOutput->Segment[XDLSegmentIx_Data].mSectorCount * XDL_SECTOR_BYTES;

    pSymTrack = new SymTrack[symInCount];
    if (NULL == pSymTrack)
    {
        printf("*** Out of memory\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    K2MEM_Zero(pSymTrack, symInCount * sizeof(SymTrack));
    pSecTarget[symTabSecIx].mpData = pOutWork;
    pBase = pOutWork;
    pSym = (Elf32_Sym const *)(((UINT8 const *)apParse->mpRawFileData) + pSymTabSecHdr->sh_offset);
    pStrOut = ((char *)pBase) + (symOutCount * sizeof(Elf32_Sym));
    pStrOut++;  // first char is always zero (null string)
    symOutIx = 0;
    for (ixSym = 0; ixSym < symInCount; ixSym++)
    {
        pSymTrack[ixSym].mInIx = ixSym;
        pSymTrack[ixSym].mpIn = pSym;
        align = pSym->st_shndx;
        if (0 != align)
        {
            if ((align >= apParse->mpRawFileData->e_shnum) ||
                (pSecTarget[align].mSegmentIx < XDLSegmentIx_Count))
            {
                pSymTrack[ixSym].mOutIx = symOutIx;
                symOutIx++;

                pSymTrack[ixSym].mpOut = (Elf32_Sym *)pBase;
                K2MEM_Copy(pBase, pSym, sizeof(Elf32_Sym));
                if (align < apParse->mpRawFileData->e_shnum)
                {
                    pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (align * apParse->mSectionHeaderTableEntryBytes));
                    pSymTrack[ixSym].mpOut->st_value -= pSecHdr->sh_addr;
                    pSymTrack[ixSym].mpOut->st_value += pSecTarget[align].mAddr;
                    pSymTrack[ixSym].mpOut->st_shndx = pSecTarget[align].mSegmentIx;
                }
                if (0 != pSym->st_name)
                {
                    pSymTrack[ixSym].mpOut->st_name = (Elf32_Word)(pStrOut - (char *)pOutWork);
                    pStrOut += K2ASC_Copy(pStrOut, pSymStr + pSym->st_name) + 1;
                }
                pBase += sizeof(Elf32_Sym);
            }
            else
            {
//                printf("tossing symbol with value 0x%08X in unused section 0x%04X %s\n", pSym->st_value, pSym->st_shndx, pSym->st_name ? (pSymStr + pSym->st_name) : "");
            }
        }
        pSym = (Elf32_Sym const *)(((UINT8 const *)pSym) + symEntSize);
    }
    K2_ASSERT(symOutIx == symOutCount);
    pOutWork += pOutput->Segment[XDLSegmentIx_Symbols].mSectorCount * XDL_SECTOR_BYTES;

    if (!gOut.mUsePlacement)
    {
        //
        // relocations
        // sh_link is symbol table to use
        // sh_info is target section
        //
        K2MEM_Copy(pOutWork, &relSegHdr, sizeof(relSegHdr));
        for (ixSec = 1; ixSec < apParse->mpRawFileData->e_shnum; ixSec++)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ixSec * apParse->mSectionHeaderTableEntryBytes));
            if ((0 != (pSecHdr->sh_flags & SHF_ALLOC)) ||
                (0 == pSecHdr->sh_size) ||
                (SHT_REL != pSecHdr->sh_type) ||
                (pSecTarget[pSecHdr->sh_info].mSegmentIx == XDLSegmentIx_Count))
                continue;
            K2_ASSERT(XDLSegmentIx_Relocs == pSecTarget[ixSec].mSegmentIx);
            pRelTargetSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (pSecHdr->sh_info * apParse->mSectionHeaderTableEntryBytes));
            K2_ASSERT(pSecTarget[ixSec].mSegOffset == (UINT_PTR)(((UINT64)pSecTarget[ixSec].mAddr) - pOutput->Segment[XDLSegmentIx_Relocs].mLinkAddr));
            pSecTarget[ixSec].mpData = pOutWork + pSecTarget[ixSec].mSegOffset;
            K2MEM_Copy(pSecTarget[ixSec].mpData, LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL), pSecHdr->sh_size);
            pRel = (Elf32_Rel *)pSecTarget[ixSec].mpData;
            relBytes = pSecHdr->sh_entsize;
            if (0 == relBytes)
                relBytes = sizeof(Elf32_Rel);
            relCount = pSecHdr->sh_size / relBytes;
            for (ixRel = 0; ixRel < relCount; ixRel++)
            {
                ixSym = ELF32_R_SYM(pRel->r_info);
                pRel->r_info = ELF32_R_INFO(pSymTrack[ixSym].mOutIx, ELF32_R_TYPE(pRel->r_info));
                pRel->r_offset -= pRelTargetSecHdr->sh_addr;
                pRel->r_offset += pSecTarget[pSecHdr->sh_info].mAddr;

                pRel = (Elf32_Rel *)(((UINT8 *)pRel) + relBytes);
            }
        }
        pOutWork += pOutput->Segment[XDLSegmentIx_Relocs].mSectorCount * XDL_SECTOR_BYTES;
    }

    //
    // other would go here but it is empty
    //

    K2_ASSERT(pOutWork == (((UINT8 *)pOutput) + (fileSectors * XDL_SECTOR_BYTES)));

    //
    // set crcs everywhere
    //
    outOffset = pOutput->Segment[XDLSegmentIx_Header].mSectorCount;
    for (ixSec = XDLSegmentIx_Text; ixSec < XDLSegmentIx_Count; ixSec++)
    {
        if (pOutput->Segment[ixSec].mSectorCount > 0)
        {
            pOutput->Segment[ixSec].mCrc32 = K2CRC_Calc32(0, ((UINT8 *)pOutput) + (outOffset * XDL_SECTOR_BYTES), (UINT_PTR)(pOutput->Segment[ixSec].mSectorCount * XDL_SECTOR_BYTES));
            outOffset += pOutput->Segment[ixSec].mSectorCount;
        }
    }
    K2_ASSERT(outOffset == fileSectors);
    pOutput->Segment[XDLSegmentIx_Header].mCrc32 = K2CRC_Calc32(0, pOutput, (UINT_PTR)(pOutput->Segment[XDLSegmentIx_Header].mSectorCount * XDL_SECTOR_BYTES));
    pOutput->mHeaderCrc32 = K2CRC_Calc32(0, &pOutput->mMarker, sizeof(XDL_FILE_HEADER) - sizeof(UINT32));

    //
    // write out the file now
    //
    HANDLE hFile = CreateFile(gOut.mpOutputFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (NULL == hFile)
    {
        printf("*** Failed to create output file \"%s\"\n", gOut.mpOutputFilePath);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    DWORD wrote = 0;
    BOOL bOk = WriteFile(hFile, pOutput, fileSectors * XDL_SECTOR_BYTES, &wrote, NULL);
    CloseHandle(hFile);
    if ((!bOk) || (wrote != fileSectors * XDL_SECTOR_BYTES))
    {
        printf("*** Failed to write output file\n");
        DeleteFile(gOut.mpOutputFilePath);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return K2STAT_NO_ERROR; //  K2STAT_ERROR_NOT_IMPL;
}
