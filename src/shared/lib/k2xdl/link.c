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

#include "ixdl.h"

typedef K2STAT(*pfLinkFunc)(UINT8 *apTarget, UINT_PTR aAddr, UINT_PTR aRelType, UINT_PTR aSymAddr);

#if K2_TOOLCHAIN_IS_MS

// tools and test framework 
// these do either arch (will load but never exec for a32)

static pfLinkFunc sUnlinkOne;
static pfLinkFunc sRelinkOne;

#define INCLUDE_BOTH    1

#else
#define INCLUDE_BOTH    0
#endif

#if (K2_TARGET_ARCH_IS_ARM || INCLUDE_BOTH)

#if K2_TARGET_ARCH_IS_32BIT

K2STAT
IXDL_UnlinkOneA32(
    UINT8 *     apRelAddr,
    UINT_PTR    aOrigRelAddr,
    UINT_PTR    aRelocType,
    UINT_PTR    aSymVal
)
{
    UINT32 valAtTarget;
    UINT32 targetAddend;

    K2MEM_Copy(&valAtTarget, apRelAddr, sizeof(UINT32));

    switch (aRelocType)
    {
    case R_ARM_PC24:
    case R_ARM_CALL:
    case R_ARM_JUMP24:
        targetAddend = (valAtTarget & 0xFFFFFF) << 2;
        if (targetAddend & 0x2000000)
            targetAddend |= 0xFC000000;
        targetAddend -= (aSymVal - (aOrigRelAddr + 8));
        valAtTarget = (valAtTarget & 0xFF000000) | ((targetAddend >> 2) & 0xFFFFFF);
        break;

    case R_ARM_ABS32:
        valAtTarget -= aSymVal;
        break;

    case R_ARM_MOVW_ABS_NC:
        targetAddend = ((valAtTarget & 0xF0000) >> 4) | (valAtTarget & 0xFFF);
        targetAddend -= (aSymVal & 0xFFFF);
        valAtTarget = ((valAtTarget) & 0xFFF0F000) | ((targetAddend << 4) & 0xF0000) | (targetAddend & 0xFFF);
        break;

    case R_ARM_MOVT_ABS:
        targetAddend = ((valAtTarget & 0xF0000) >> 4) | (valAtTarget & 0xFFF);
        targetAddend -= ((aSymVal >> 16) & 0xFFFF);
        valAtTarget = ((valAtTarget) & 0xFFF0F000) | ((targetAddend << 4) & 0xF0000) | (targetAddend & 0xFFF);
        break;

    case R_ARM_V4BX:
        // ignored
        break;

    case R_ARM_PREL31:
        // value is an addend already
        break;

    default:
        return K2STAT_ERROR_CORRUPTED;
    }

    K2MEM_Copy(apRelAddr, &valAtTarget, sizeof(UINT32));

    return K2STAT_NO_ERROR;
}

K2STAT
IXDL_RelinkOneA32(
    UINT8 *     apRelAddr,
    UINT_PTR    aNewRelAddr,
    UINT_PTR    aRelocType,
    UINT_PTR    aSymVal
)
{
    UINT32 valAtTarget;
    UINT32 targetAddend;
    UINT32 newVal;
    UINT32 distance;

    K2MEM_Copy(&valAtTarget, apRelAddr, sizeof(UINT32));

    switch (aRelocType)
    {
    case R_ARM_PC24:
    case R_ARM_CALL:
    case R_ARM_JUMP24:
        targetAddend = (valAtTarget & 0xFFFFFF) << 2;
        if (targetAddend & 0x2000000)
            targetAddend |= 0xFC000000;
        newVal = (aSymVal + targetAddend) - (aNewRelAddr + 8);
        distance = newVal;
        if (0 != (distance & 0x80000000))
        {
            distance = (~distance) + 1;
        }
        if (0 != (distance & 0xFC000000))
        {
            // jump is too far
            return K2STAT_ERROR_OUT_OF_BOUNDS;
        }
        valAtTarget = (valAtTarget & 0xFF000000) | ((newVal >> 2) & 0xFFFFFF);
        break;

    case R_ARM_ABS32:
        valAtTarget += aSymVal;
        break;

    case R_ARM_MOVW_ABS_NC:
        targetAddend = ((valAtTarget & 0xF0000) >> 4) | (valAtTarget & 0xFFF);
        newVal = ((aSymVal & 0xFFFF) + targetAddend) & 0xFFFF;
        valAtTarget = (valAtTarget & 0xFFF0F000) | ((newVal << 4) & 0xF0000) | (newVal & 0xFFF);
        break;

    case R_ARM_MOVT_ABS:
        targetAddend = ((valAtTarget & 0xF0000) >> 4) | (valAtTarget & 0xFFF);
        newVal = ((((aSymVal & 0xFFFF0000) + (targetAddend << 16))) >> 16) & 0xFFFF;
        valAtTarget = (valAtTarget & 0xFFF0F000) | ((newVal << 4) & 0xF0000) | (newVal & 0xFFF);
        break;

    case R_ARM_V4BX:
        // ignored
        break;

    case R_ARM_PREL31:
        targetAddend = valAtTarget + aSymVal - aNewRelAddr;
        valAtTarget = targetAddend & 0x7FFFFFFF;
        break;

    default:
        K2_ASSERT(0);
        return K2STAT_ERROR_UNKNOWN;
    }

    K2MEM_Copy(apRelAddr, &valAtTarget, sizeof(UINT32));

    return K2STAT_NO_ERROR;
}

#if (!INCLUDE_BOTH)
#define sUnlinkOne IXDL_UnlinkOneA32
#define sRelinkOne IXDL_RelinkOneA32
#endif
#else

K2STAT
IXDL_UnlinkOneA64(
    UINT8 *     apRelAddr,
    UINT_PTR    aOrigRelAddr,
    UINT_PTR    aRelocType,
    UINT_PTR    aSymVal
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
IXDL_RelinkOneA64(
    UINT8 *     apRelAddr,
    UINT_PTR    aNewRelAddr,
    UINT_PTR    aRelocType,
    UINT_PTR    aSymVal
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

#if (!INCLUDE_BOTH)
#define sUnlinkOne IXDL_UnlinkOneA64
#define sRelinkOne IXDL_RelinkOneA64
#endif

#endif

#endif

#if (K2_TARGET_ARCH_IS_INTEL || INCLUDE_BOTH)

#if K2_TARGET_ARCH_IS_32BIT

K2STAT
IXDL_UnlinkOneX32(
    UINT8 *     apRelAddr,
    UINT_PTR    aOrigRelAddr,
    UINT_PTR    aRelocType,
    UINT_PTR    aSymVal
)
{
    UINT32 valAtTarget;

    K2MEM_Copy(&valAtTarget, apRelAddr, sizeof(UINT32));

    switch (aRelocType)
    {
    case R_386_32:
        // value at target is address of symbol + some addend
        valAtTarget = valAtTarget - aSymVal;
        break;

    case R_386_PC32:
        // value at target is offset from 4 bytes
        // after target to symbol value + some addend
        valAtTarget = valAtTarget - (aSymVal - (aOrigRelAddr + 4));
        break;

    default:
        return K2STAT_ERROR_CORRUPTED;
    }

    K2MEM_Copy(apRelAddr, &valAtTarget, sizeof(UINT32));

    return K2STAT_NO_ERROR;
}

K2STAT
IXDL_RelinkOneX32(
    UINT8 *     apRelAddr,
    UINT_PTR    aNewRelAddr,
    UINT_PTR    aRelocType,
    UINT_PTR    aSymVal
)
{
    UINT32 valAtTarget;

    K2MEM_Copy(&valAtTarget, apRelAddr, sizeof(UINT32));

    switch (aRelocType)
    {
    case R_386_32:
        // value at target is offset to add
        valAtTarget += aSymVal;
        break;

    case R_386_PC32:
        valAtTarget = (aSymVal + valAtTarget) - (aNewRelAddr + 4);
        break;

    default:
        K2_ASSERT(0);
        return K2STAT_ERROR_UNKNOWN;
    }

    K2MEM_Copy(apRelAddr, &valAtTarget, sizeof(UINT32));

    return 0;
}

#if (!INCLUDE_BOTH)
#define sUnlinkOne IXDL_UnlinkOneX32
#define sRelinkOne IXDL_RelinkOneX32
#endif
#else

K2STAT
IXDL_UnlinkOneX64(
    UINT8 *     apRelAddr,
    UINT_PTR    aOrigRelAddr,
    UINT_PTR    aRelocType,
    UINT_PTR    aSymVal
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
IXDL_RelinkOneX64(
    UINT8 *     apRelAddr,
    UINT_PTR    aNewRelAddr,
    UINT_PTR    aRelocType,
    UINT_PTR    aSymVal
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

#if (!INCLUDE_BOTH)
#define sUnlinkOne IXDL_UnlinkOneX64
#define sRelinkOne IXDL_RelinkOneX64
#endif

#endif

#endif

#if K2_TARGET_ARCH_IS_32BIT
#define ELFSYM_TYPE Elf32_Sym
#define ELFREL_TYPE Elf32_Rel
#define XDLEXP_TYPE XDL_EXPORT32_REF
#else
#define ELFSYM_TYPE Elf64_Sym
#define ELFREL_TYPE Elf64_Rel
#define XDLEXP_TYPE XDL_EXPORT64_REF
#endif

K2STAT
IXDL_ProcessReloc(
    XDL *               apXdl,
    XDL_FILE_HEADER *   apHeader,
    BOOL                aRelink
)
{
    K2STAT                      stat;
//    char const *                pSymName;
    ELFSYM_TYPE *               pSymSeg;
    ELFSYM_TYPE *               pSym;
    UINT_PTR                    symCount;

    XDL_RELOC_SEGMENT_HEADER *  pRelSeg;
    ELFREL_TYPE *               pRel;
    UINT_PTR                    relCount;

    UINT8 *                     pSegLink;
    UINT_PTR                    segLinkAddr;
    UINT_PTR                    symIx;
    UINT_PTR                    relType;
    UINT_PTR                    importCount;
    UINT_PTR                    importsLeft;
    UINT_PTR                    symValue;
    XDL_IMPORT *                pImport;
    pfLinkFunc                  linkFunc;

#if K2_TOOLCHAIN_IS_MS
#if K2_TARGET_ARCH_IS_32BIT
    sUnlinkOne = (apHeader->mElfMachine == EM_X32) ? IXDL_UnlinkOneX32 : IXDL_UnlinkOneA32;
    sRelinkOne = (apHeader->mElfMachine == EM_X32) ? IXDL_RelinkOneX32 : IXDL_RelinkOneA32;
#else
    sUnlinkOne = (apHeader->mElfMachine == EM_X64) ? IXDL_UnlinkOneX64 : IXDL_UnlinkOneA64;
    sRelinkOne = (apHeader->mElfMachine == EM_X64) ? IXDL_RelinkOneX64 : IXDL_RelinkOneA64;
#endif
#endif
    if (!aRelink)
        linkFunc = sUnlinkOne;
    else
        linkFunc = sRelinkOne;

    pSymSeg = (ELFSYM_TYPE *)(UINT_PTR)apXdl->SegAddrs.mData[XDLSegmentIx_Symbols];
    symCount = apHeader->mSymCount_Abs;
    for (symIx = 0; symIx < XDLProgDataType_Count; symIx++)
        symCount += apHeader->mSymCount[symIx];
    symCount += apHeader->mSymCount_Imp;
    pRelSeg = (XDL_RELOC_SEGMENT_HEADER *)(UINT_PTR)apXdl->SegAddrs.mData[XDLSegmentIx_Relocs];
    pRel = (ELFREL_TYPE *)(((UINT_PTR)pRelSeg) + sizeof(XDL_RELOC_SEGMENT_HEADER));

    importCount = ((UINT_PTR)(apHeader->Segment[XDLSegmentIx_Header].mMemActualBytes - apHeader->mImportsOffset)) / sizeof(XDL_IMPORT);

    //
    // text relocs go first 
    //
    relCount = (UINT_PTR)(pRelSeg->mTextRelocsBytes / sizeof(ELFREL_TYPE));
    if (0 != relCount)
    {
        pSegLink = (UINT8 *)(UINT_PTR)apXdl->SegAddrs.mData[XDLSegmentIx_Text];
        segLinkAddr = (UINT_PTR)apHeader->Segment[XDLSegmentIx_Text].mLinkAddr;
        do {
            relType = pRel->r_info;
            symIx = ELF32_R_SYM(relType);
            if (symIx >= symCount)
                return K2STAT_ERROR_CORRUPTED;
            pSym = &pSymSeg[symIx];
//            if (0 != pSym->st_name)
//                pSymName = ((char const *)pSymSeg) + pSym->st_name;
            symValue = pSym->st_value;
            if (!aRelink)
            {
                if (0 == pSym->st_shndx)
                {
                    //
                    // imported symbol
                    //
                    symValue -= (UINT_PTR)apHeader->Segment[XDLSegmentIx_Header].mLinkAddr;
                    pImport = apXdl->mpImports;
                    importsLeft = importCount;
                    do {
                        symIx = pImport->ExpHdr.mCount * sizeof(XDLEXP_TYPE);
                        if (symValue < symIx)
                            break;
                        pImport++;
                        symValue -= symIx;
                        if (0 == --importsLeft)
                            return K2STAT_ERROR_CORRUPTED;
                    } while (1);
                    symValue += ((UINT_PTR)pImport->mOrigLinkAddr) + sizeof(XDL_EXPORTS_SEGMENT_HEADER);
                }

                pRel->r_offset -= segLinkAddr;
            }
            stat = linkFunc(
                pSegLink + pRel->r_offset,
                segLinkAddr + pRel->r_offset,
                ELF32_R_TYPE(relType),
                symValue);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            pRel++;
        } while (--relCount);
    }

    //
    // then read
    //
    relCount = (UINT_PTR)(pRelSeg->mReadRelocsBytes / sizeof(ELFREL_TYPE));
    if (0 != relCount)
    {
        pSegLink = (UINT8 *)(UINT_PTR)apXdl->SegAddrs.mData[XDLSegmentIx_Read];
        segLinkAddr = (UINT_PTR)apHeader->Segment[XDLSegmentIx_Read].mLinkAddr;
        do {
            relType = pRel->r_info;
            symIx = ELF32_R_SYM(relType);
            if (symIx >= symCount)
                return K2STAT_ERROR_CORRUPTED;
            pSym = &pSymSeg[symIx];
//            if (0 != pSym->st_name)
//                pSymName = ((char const *)pSymSeg) + pSym->st_name;
            symValue = pSym->st_value;
            if (!aRelink)
            {
                if (0 == pSym->st_shndx)
                {
                    //
                    // imported symbol
                    //
                    symValue -= (UINT_PTR)apHeader->Segment[XDLSegmentIx_Header].mLinkAddr;
                    pImport = apXdl->mpImports;
                    importsLeft = importCount;
                    do {
                        symIx = pImport->ExpHdr.mCount * sizeof(XDLEXP_TYPE);
                        if (symValue < symIx)
                            break;
                        pImport++;
                        symValue -= symIx;
                        if (0 == --importsLeft)
                            return K2STAT_ERROR_CORRUPTED;
                    } while (1);
                    symValue += ((UINT_PTR)pImport->mOrigLinkAddr) + sizeof(XDL_EXPORTS_SEGMENT_HEADER);
                }

                pRel->r_offset -= segLinkAddr;
            }
            stat = linkFunc(
                pSegLink + pRel->r_offset,
                segLinkAddr + pRel->r_offset,
                ELF32_R_TYPE(relType),
                pSym->st_value);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            pRel++;
        } while (--relCount);
    }

    //
    // then data
    //
    relCount = (UINT_PTR)(pRelSeg->mDataRelocsBytes / sizeof(ELFREL_TYPE));
    if (0 != relCount)
    {
        pSegLink = (UINT8 *)(UINT_PTR)apXdl->SegAddrs.mData[XDLSegmentIx_Data];
        segLinkAddr = (UINT_PTR)apHeader->Segment[XDLSegmentIx_Data].mLinkAddr;
        do {
            relType = pRel->r_info;
            symIx = ELF32_R_SYM(relType);
            if (symIx >= symCount)
                return K2STAT_ERROR_CORRUPTED;
            pSym = &pSymSeg[symIx];
//            if (0 != pSym->st_name)
//                pSymName = ((char const *)pSymSeg) + pSym->st_name;
            symValue = pSym->st_value;
            if (!aRelink)
            {
                if (0 == pSym->st_shndx)
                {
                    //
                    // imported symbol
                    //
                    symValue -= (UINT_PTR)apHeader->Segment[XDLSegmentIx_Header].mLinkAddr;
                    pImport = apXdl->mpImports;
                    importsLeft = importCount;
                    do {
                        symIx = pImport->ExpHdr.mCount * sizeof(XDLEXP_TYPE);
                        if (symValue < symIx)
                            break;
                        pImport++;
                        symValue -= symIx;
                        if (0 == --importsLeft)
                            return K2STAT_ERROR_CORRUPTED;
                    } while (1);
                    symValue += ((UINT_PTR)pImport->mOrigLinkAddr) + sizeof(XDL_EXPORTS_SEGMENT_HEADER);
                }

                pRel->r_offset -= segLinkAddr;
            }
            stat = linkFunc(
                pSegLink + pRel->r_offset,
                segLinkAddr + pRel->r_offset,
                ELF32_R_TYPE(relType),
                pSym->st_value);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            pRel++;
        } while (--relCount);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
IXDL_Retarget(
    XDL *               apXdl,
    XDL_FILE_HEADER *   apHeader
)
{
    K2STAT                          stat;
    char const *                    pSymName;
    ELFSYM_TYPE *                   pSymSeg;
    ELFSYM_TYPE *                   pSym;
    UINT_PTR                        symCount;
    UINT_PTR                        targetSeg;
    UINT_PTR                        symOffset;
    XDL_IMPORT *                    pImport;
    UINT_PTR                        importCount;
    UINT_PTR                        importsLeft;
    UINT_PTR                        expSize;
    XDLProgDataType                 impType;
    XDL *                           pImportFrom;
    XDL_EXPORTS_SEGMENT_HEADER *    pImpExp;
    XDLEXP_TYPE *                   pRef;
//    char const *                    pCheckName;

    importCount = ((UINT_PTR)(apHeader->Segment[XDLSegmentIx_Header].mMemActualBytes - apHeader->mImportsOffset)) / sizeof(XDL_IMPORT);

    //
    // convert symbols to new addresses
    //
    pSymSeg = (ELFSYM_TYPE *)(UINT_PTR)apXdl->SegAddrs.mData[XDLSegmentIx_Symbols];

    symCount = apHeader->mSymCount_Abs;
    for (expSize = 0; expSize < XDLProgDataType_Count; expSize++)
        symCount += apHeader->mSymCount[expSize];
    symCount += apHeader->mSymCount_Imp;

    pSym = pSymSeg;
    do {
        if (0 != pSym->st_name)
            pSymName = ((char const *)(UINT_PTR)apXdl->SegAddrs.mData[XDLSegmentIx_Symbols]) + pSym->st_name;
        else
            pSymName = NULL;
        targetSeg = pSym->st_shndx;
        if (targetSeg < XDLSegmentIx_Count)
        {
            symOffset = pSym->st_value - (UINT_PTR)apHeader->Segment[targetSeg].mLinkAddr;
            if (targetSeg == XDLSegmentIx_Header)
            {
                //
                // imported symbol - using the offset determine where from
                //
                K2_ASSERT(NULL != pSymName);
                pImport = apXdl->mpImports;
                importsLeft = importCount;
                do {
                    expSize = pImport->ExpHdr.mCount * sizeof(XDLEXP_TYPE);
                    if (symOffset < expSize)
                        break;
                    pImport++;
                    symOffset -= expSize;
                    if (0 == --importsLeft)
                        return K2STAT_ERROR_CORRUPTED;
                } while (1);
                pImportFrom = (XDL *)(UINT_PTR)pImport->mReserved;
                if (0 != (pImport->mSectionFlags & SHF_EXECINSTR))
                {
                    impType = XDLProgData_Text;
                }
                else if (0 == (pImport->mSectionFlags & SHF_WRITE))
                {
                    impType = XDLProgData_Read;
                }
                else
                {
                    impType = XDLProgData_Data;
                }
                pImpExp = pImportFrom->mpExpHdr[impType];
                if ((pImpExp->mCount == pImport->ExpHdr.mCount) &&
                    (pImpExp->mCRC32 == pImport->ExpHdr.mCRC32))
                {
                    //
                    // fast link
                    //
                    pRef = (XDLEXP_TYPE *)(((UINT_PTR)pImpExp) + sizeof(XDL_EXPORTS_SEGMENT_HEADER));
//                    pCheckName = ((char const *)pImpExp) + pRef[symOffset / sizeof(XDLEXP_TYPE)].mNameOffset;
                    pSym->st_value = pRef[symOffset / sizeof(XDLEXP_TYPE)].mAddr;
                }
                else
                {
                    //
                    // slow link
                    //
                    stat = XDL_FindExport(pImportFrom, impType, pSymName, &pSym->st_value);
                    if (K2STAT_IS_ERROR(stat))
                        return stat;
                }
            }
            else
            {
                //
                // module symbol
                //
                pSym->st_value = ((UINT_PTR)apXdl->SegAddrs.mLink[targetSeg]) + symOffset;
            }
        }
        pSym++;
    } while (--symCount);

    //
    // adjust the entrypoint address
    //
    apHeader->mEntryPoint -= apHeader->Segment[XDLSegmentIx_Text].mLinkAddr;
    apHeader->mEntryPoint += apXdl->SegAddrs.mLink[XDLSegmentIx_Text];

    //
    // finally update header segment addresses to target
    //
    for (symOffset = 0; symOffset < XDLSegmentIx_Count; symOffset++)
    {
        apHeader->Segment[symOffset].mLinkAddr = apXdl->SegAddrs.mLink[symOffset];
    }

    return K2STAT_NO_ERROR;
}

K2STAT
IXDL_Link(
    XDL *   apXdl
)
{
    K2STAT              stat;
    XDL_FILE_HEADER *   pHeader;

    pHeader = apXdl->mpHeader;
    if (0 == pHeader->Segment[XDLSegmentIx_Relocs].mMemActualBytes)
        return K2STAT_NO_ERROR;

    if (sizeof(ELFSYM_TYPE) > pHeader->Segment[XDLSegmentIx_Symbols].mMemActualBytes)
        return K2STAT_ERROR_CORRUPTED;

    stat = IXDL_ProcessReloc(apXdl, pHeader, FALSE);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    stat = IXDL_Retarget(apXdl, pHeader);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    stat = IXDL_ProcessReloc(apXdl, pHeader, TRUE);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    return stat;
}

