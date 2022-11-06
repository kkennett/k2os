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

#define BASE_ADDR_RELOC32   0x10000000

struct SecTarget
{
    UINT_PTR    mAddr;
    UINT_PTR    mSegmentIx;
    UINT_PTR    mSegOffset;
    UINT8 *     mpData;
    UINT_PTR    mRelocSymSrcCount;
};

struct SymTrack
{
    UINT_PTR            mInIx;
    Elf32_Sym const *   mpIn;
    UINT_PTR            mOutIx;
    Elf32_Sym *         mpOut;
    INT_PTR             mProgDataTreeTarget;
    bool                mInImportSection;
    bool                mUsedInReloc;
    char const *        mpName;
    K2TREE_NODE         TreeNode;
};

struct SecIx
{
    UINT_PTR    mText;
    UINT_PTR    mRead;
    UINT_PTR    mData;
    UINT_PTR    mBss;

    UINT_PTR    mSym;
    UINT_PTR    mSymStr;

    UINT_PTR    mTextRel;
    UINT_PTR    mReadRel;
    UINT_PTR    mDataRel;

};

UINT_PTR            gSectionCount;
SecTarget *         gpSectionTarget;
SecIx               gSecIx;
char const *        gpSymStr;
UINT8 const *       gpSymIn;
UINT_PTR            gSymEntSize;
UINT_PTR            gSymInCount;
UINT_PTR            gSymOutCount;
SymTrack *          gpSymTrack;
UINT_PTR            gSymStrSize;
K2TREE_ANCHOR       gTreeAbs;
K2TREE_ANCHOR       gTreeSeg[XDLProgDataType_Count];
K2TREE_ANCHOR       gTreeImp;
UINT_PTR            gFileSectors;
UINT_PTR            gImportCount;

UINT8 *             gpOutFile;


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

K2STAT
Output32(
    K2ELF32PARSE *          apParse,
    XDL_FILE_HEADER const * apWorkHdr
)
{
    XDL_FILE_HEADER *   pHeader;
    UINT64 *            pSrcPtr;
    UINT_PTR            ix;
    Elf32_Shdr const *  pSecHdr;
    Elf32_Shdr const *  pSrcSecHdr;
    UINT64              curSector;
    XDL_IMPORT *        pImport;
    UINT8 const *       pSrcData;
    UINT_PTR            importBase;
    Elf32_Sym *         pSym;
    UINT_PTR            secIx;
    char *              pSymStrOut;
    XDL_RELOC_SEGMENT_HEADER *  pRelHdr;
    UINT_PTR            relEntBytes;
    Elf32_Rel *         pRel;
    Elf32_Rel const *   pSrcRel;
    UINT32              targetOffset;
    UINT8 *             pRelTarget;
    Elf32_Sym const *   pSrcSym;
    UINT_PTR            symSec;
    UINT32              newSymVal;

    gpOutFile = (UINT8 *)malloc((gFileSectors + 1) * XDL_SECTOR_BYTES);
    if (NULL == gpOutFile)
    {
        printf("*** out of memory\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    gpOutFile = (UINT8 *)(((((UINT_PTR)gpOutFile) + (XDL_SECTOR_BYTES - 1)) / XDL_SECTOR_BYTES) * XDL_SECTOR_BYTES);
    K2MEM_Zero(gpOutFile, gFileSectors * XDL_SECTOR_BYTES);
    K2MEM_Copy(gpOutFile, apWorkHdr, sizeof(XDL_FILE_HEADER));
    pHeader = (XDL_FILE_HEADER *)gpOutFile;
    curSector = 0;

    //
    // header segment content
    //

    pHeader->mMarker = XDL_FILE_HEADER_MARKER;
    pHeader->mHeaderBytes = sizeof(XDL_FILE_HEADER);
    pHeader->mElfClass = apParse->mpRawFileData->e_ident[EI_CLASS];
    pHeader->mElfMachine = (UINT8)apParse->mpRawFileData->e_machine;
    if (gOut.mSpecKernel)
        pHeader->mFlags = XDL_FILE_HEADER_FLAG_KERNEL_ONLY;
    if (gOut.mSpecStack)
        pHeader->mEntryStackReq = gOut.mSpecStack;
    pHeader->mNameLen = gOut.mTargetNameLen;
    K2MEM_Copy(pHeader->mName, gOut.mTargetName, gOut.mTargetNameLen);
    K2MEM_Copy(&pHeader->Id, &gOut.mpElfAnchor->Id, sizeof(K2_GUID128));
    if (0 != gSecIx.mText)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mText * apParse->mSectionHeaderTableEntryBytes));
        pHeader->mEntryPoint = pHeader->Segment[XDLSegmentIx_Text].mLinkAddr + (apParse->mpRawFileData->e_entry - pSecHdr->sh_addr);
    }
    if (0 != gSecIx.mRead)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mRead * apParse->mSectionHeaderTableEntryBytes));
        K2_ASSERT(0 != pSecHdr->sh_size);
        pSrcPtr = (UINT64 *)&gOut.mpElfAnchor->mAnchor[0];
        for (ix = 0; ix < XDLProgDataType_Count; ix++)
        {
            if (0 != pSrcPtr[ix])
            {
                pHeader->mReadExpOffset[ix] = pSrcPtr[ix] - ((UINT64)pSecHdr->sh_addr);
            }
        }
    }
    if (0 != gImportCount)
    {
        pImport = (XDL_IMPORT *)(gpOutFile + pHeader->mImportsOffset);
        importBase = (UINT_PTR)pHeader->Segment[XDLSegmentIx_Header].mLinkAddr;
        for (ix = 0; ix < gSectionCount; ix++)
        {
            if (XDLSegmentIx_Header != gpSectionTarget[ix].mSegmentIx)
                continue;
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (ix * apParse->mSectionHeaderTableEntryBytes));
            pSrcData = (UINT8 const *)LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
            K2MEM_Copy(&pImport->ExpHdr, pSrcData, sizeof(XDL_EXPORTS_SEGMENT_HEADER));
            pImport->mSectionFlags = pSecHdr->sh_flags;
            pSrcData += sizeof(XDL_EXPORTS_SEGMENT_HEADER);
            pSrcData += pImport->ExpHdr.mCount * sizeof(XDL_EXPORT32_REF);
            K2MEM_Copy(&pImport->ID, pSrcData, sizeof(K2_GUID128));
            pSrcData += sizeof(K2_GUID128);
            gpSectionTarget[ix].mSegOffset = ((UINT64)pImport) - ((UINT64)gpOutFile);
            pImport->mOrigLinkAddr = gpSectionTarget[ix].mAddr = importBase;
            importBase += pImport->ExpHdr.mCount * sizeof(XDL_EXPORT32_REF);
            pImport->mNameLen = K2ASC_CopyLen(pImport->mName, (char const *)pSrcData, XDL_NAME_MAX_LEN);
            pImport->mName[XDL_NAME_MAX_LEN] = 0;
            pImport++;
        }
    }
    curSector += pHeader->Segment[XDLSegmentIx_Header].mSectorCount;

    //
    // text, read, data segment contents
    //
    if (0 != gSecIx.mText)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mText * apParse->mSectionHeaderTableEntryBytes));
        pSrcData = (UINT8 const *)LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
        gpSectionTarget[gSecIx.mText].mpData = gpOutFile + ((UINT_PTR)(curSector * XDL_SECTOR_BYTES));
        K2MEM_Copy(gpSectionTarget[gSecIx.mText].mpData, pSrcData, pSecHdr->sh_size);
        curSector += pHeader->Segment[XDLSegmentIx_Text].mSectorCount;
    }
    if (0 != gSecIx.mRead)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mRead * apParse->mSectionHeaderTableEntryBytes));
        pSrcData = (UINT8 const *)LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
        gpSectionTarget[gSecIx.mRead].mpData = gpOutFile + ((UINT_PTR)(curSector * XDL_SECTOR_BYTES));
        K2MEM_Copy(gpSectionTarget[gSecIx.mRead].mpData, pSrcData, pSecHdr->sh_size);
        curSector += pHeader->Segment[XDLSegmentIx_Read].mSectorCount;
    }
    if (0 != gSecIx.mData)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mData * apParse->mSectionHeaderTableEntryBytes));
        pSrcData = (UINT8 const *)LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
        gpSectionTarget[gSecIx.mData].mpData = gpOutFile + ((UINT_PTR)(curSector * XDL_SECTOR_BYTES));
        K2MEM_Copy(gpSectionTarget[gSecIx.mData].mpData, pSrcData, pSecHdr->sh_size);
        curSector += pHeader->Segment[XDLSegmentIx_Data].mSectorCount;
    }

    //
    // assemble target symbol table now
    //
    pSym = (Elf32_Sym *)(gpOutFile + ((UINT_PTR)(curSector * XDL_SECTOR_BYTES)));
    pSymStrOut = ((char *)pSym) + (gSymOutCount * sizeof(Elf32_Sym));
    for (ix = 0; ix < gSymInCount; ix++)
    {
        if (NULL == gpSymTrack[ix].mpIn)
            continue;

        K2_ASSERT(NULL == gpSymTrack[ix].mpOut);
        gpSymTrack[ix].mpOut = pSym + gpSymTrack[ix].mOutIx;
        K2MEM_Copy(gpSymTrack[ix].mpOut, gpSymTrack[ix].mpIn, sizeof(Elf32_Sym));
        secIx = gpSymTrack[ix].mpIn->st_shndx;
        if (0 != gpSymTrack[ix].mpOut->st_name)
        {
            gpSymTrack[ix].mpOut->st_name = (Elf32_Word)(pSymStrOut - (char *)pSym);
            pSymStrOut += K2ASC_Copy(pSymStrOut, gpSymTrack[ix].mpName) + 1;
        }

        if (-1 == gpSymTrack[ix].mProgDataTreeTarget)
        {
            //
            // absolute symbol
            //
            gpSymTrack[ix].mpOut->st_shndx = XDLSegmentIx_Count;
        }
        else if (XDLProgDataType_Count == gpSymTrack[ix].mProgDataTreeTarget)
        {
            //
            // imported symbol
            //
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (secIx * apParse->mSectionHeaderTableEntryBytes));
            gpSymTrack[ix].mpOut->st_value -= (pSecHdr->sh_addr + sizeof(XDL_EXPORTS_SEGMENT_HEADER));
            gpSymTrack[ix].mpOut->st_value += gpSectionTarget[secIx].mAddr;
            gpSymTrack[ix].mpOut->st_shndx = XDLSegmentIx_Header;
        }
        else
        {
            //
            // text, read, or data symbol
            //
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (secIx * apParse->mSectionHeaderTableEntryBytes));
            gpSymTrack[ix].mpOut->st_value -= pSecHdr->sh_addr;
            gpSymTrack[ix].mpOut->st_value += gpSectionTarget[secIx].mAddr;
            if (secIx == gSecIx.mText)
            {
                gpSymTrack[ix].mpOut->st_shndx = XDLSegmentIx_Text;
            }
            else if (secIx == gSecIx.mRead)
            {
                gpSymTrack[ix].mpOut->st_shndx = XDLSegmentIx_Read;
            }
            else
            {
                K2_ASSERT((secIx == gSecIx.mData) || (secIx == gSecIx.mBss));
                gpSymTrack[ix].mpOut->st_shndx = XDLSegmentIx_Data;
            }
        }
    }
    curSector += pHeader->Segment[XDLSegmentIx_Symbols].mSectorCount;

    //
    // relocate text, read, data segments in output file to target locations now
    //
    if (0 != gSecIx.mTextRel)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mTextRel * apParse->mSectionHeaderTableEntryBytes));
        relEntBytes = pSecHdr->sh_entsize;
        if (0 == relEntBytes)
            relEntBytes = sizeof(Elf32_Rel);
        pSrcData = LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
        ix = pSecHdr->sh_size / relEntBytes;
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mText * apParse->mSectionHeaderTableEntryBytes));

        pSrcRel = (Elf32_Rel const *)pSrcData;
        do {
            targetOffset = pSrcRel->r_offset - pSecHdr->sh_addr;
            pRelTarget = gpSectionTarget[gSecIx.mText].mpData + targetOffset;
            newSymVal = ELF32_R_SYM(pSrcRel->r_info);
            pSrcSym = (Elf32_Sym const *)gpSymTrack[newSymVal].mpIn;
            symSec = gpSymTrack[newSymVal].mpIn->st_shndx;
            newSymVal = pSrcSym->st_value;
            if ((symSec > 0) && (symSec < gSectionCount))
            {
                pSrcSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (symSec * apParse->mSectionHeaderTableEntryBytes));
                newSymVal -= pSrcSecHdr->sh_addr;
                newSymVal += gpSectionTarget[symSec].mAddr;
            }
            if (pHeader->mElfMachine == EM_X32)
                RelocOne32_X32(ELF32_R_TYPE(pSrcRel->r_info), pSrcRel->r_offset, gpSectionTarget[gSecIx.mText].mAddr + targetOffset, pRelTarget, pSrcSym, newSymVal);
            else
                RelocOne32_A32(ELF32_R_TYPE(pSrcRel->r_info), pSrcRel->r_offset, gpSectionTarget[gSecIx.mText].mAddr + targetOffset, pRelTarget, pSrcSym, newSymVal);
            pSrcRel = (Elf32_Rel const *)(((UINT8 const *)pSrcRel) + relEntBytes);
        } while (--ix);
    }
    if (0 != gSecIx.mReadRel)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mReadRel * apParse->mSectionHeaderTableEntryBytes));
        relEntBytes = pSecHdr->sh_entsize;
        if (0 == relEntBytes)
            relEntBytes = sizeof(Elf32_Rel);
        pSrcData = LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
        ix = pSecHdr->sh_size / relEntBytes;
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mRead * apParse->mSectionHeaderTableEntryBytes));

        pSrcRel = (Elf32_Rel const *)pSrcData;
        do {
            targetOffset = pSrcRel->r_offset - pSecHdr->sh_addr;
            pRelTarget = gpSectionTarget[gSecIx.mRead].mpData + targetOffset;
            newSymVal = ELF32_R_SYM(pSrcRel->r_info);
            pSrcSym = (Elf32_Sym const *)gpSymTrack[newSymVal].mpIn;
            symSec = gpSymTrack[newSymVal].mpIn->st_shndx;
            newSymVal = pSrcSym->st_value;
            if ((symSec > 0) && (symSec < gSectionCount))
            {
                pSrcSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (symSec * apParse->mSectionHeaderTableEntryBytes));
                newSymVal -= pSrcSecHdr->sh_addr;
                newSymVal += gpSectionTarget[symSec].mAddr;
            }
            if (pHeader->mElfMachine == EM_X32)
                RelocOne32_X32(ELF32_R_TYPE(pSrcRel->r_info), pSrcRel->r_offset, gpSectionTarget[gSecIx.mRead].mAddr + targetOffset, pRelTarget, pSrcSym, newSymVal);
            else
                RelocOne32_A32(ELF32_R_TYPE(pSrcRel->r_info), pSrcRel->r_offset, gpSectionTarget[gSecIx.mRead].mAddr + targetOffset, pRelTarget, pSrcSym, newSymVal);
            pSrcRel = (Elf32_Rel const *)(((UINT8 const *)pSrcRel) + relEntBytes);
        } while (--ix);
    }
    if (0 != gSecIx.mDataRel)
    {
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mDataRel * apParse->mSectionHeaderTableEntryBytes));
        relEntBytes = pSecHdr->sh_entsize;
        if (0 == relEntBytes)
            relEntBytes = sizeof(Elf32_Rel);
        pSrcData = LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
        ix = pSecHdr->sh_size / relEntBytes;
        pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mData * apParse->mSectionHeaderTableEntryBytes));
        
        pSrcRel = (Elf32_Rel const *)pSrcData;
        do {
            targetOffset = pSrcRel->r_offset - pSecHdr->sh_addr;
            pRelTarget = gpSectionTarget[gSecIx.mData].mpData + targetOffset;
            newSymVal = ELF32_R_SYM(pSrcRel->r_info);
            pSrcSym = (Elf32_Sym const *)gpSymTrack[newSymVal].mpIn;
            symSec = gpSymTrack[newSymVal].mpIn->st_shndx;
            newSymVal = pSrcSym->st_value;
            if ((symSec > 0) && (symSec < gSectionCount))
            {
                pSrcSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (symSec * apParse->mSectionHeaderTableEntryBytes));
                newSymVal -= pSrcSecHdr->sh_addr;
                newSymVal += gpSectionTarget[symSec].mAddr;
            }
            if (pHeader->mElfMachine == EM_X32)
                RelocOne32_X32(ELF32_R_TYPE(pSrcRel->r_info), pSrcRel->r_offset, gpSectionTarget[gSecIx.mData].mAddr + targetOffset, pRelTarget, pSrcSym, newSymVal);
            else
                RelocOne32_A32(ELF32_R_TYPE(pSrcRel->r_info), pSrcRel->r_offset, gpSectionTarget[gSecIx.mData].mAddr + targetOffset, pRelTarget, pSrcSym, newSymVal);
            pSrcRel = (Elf32_Rel const *)(((UINT8 const *)pSrcRel) + relEntBytes);
        } while (--ix);
    }

    if (!gOut.mUsePlacement)
    {
        //
        // update and output relocations
        //
        pRelHdr = (XDL_RELOC_SEGMENT_HEADER *)(gpOutFile + ((UINT_PTR)(curSector * XDL_SECTOR_BYTES)));
        pRel = (Elf32_Rel *)(((UINT8 *)pRelHdr) + sizeof(XDL_RELOC_SEGMENT_HEADER));
        if (0 != gSecIx.mTextRel)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mTextRel * apParse->mSectionHeaderTableEntryBytes));
            relEntBytes = pSecHdr->sh_entsize;
            if (0 == relEntBytes)
                relEntBytes = sizeof(Elf32_Rel);
            pRelHdr->mTextRelocsBytes = (pSecHdr->sh_size / relEntBytes);
            pSrcData = LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mText * apParse->mSectionHeaderTableEntryBytes));
            for (ix = 0; ix < pRelHdr->mTextRelocsBytes; ix++)
            {
                K2MEM_Copy(pRel, pSrcData + (ix * relEntBytes), sizeof(Elf32_Rel));
                secIx = ELF32_R_SYM(pRel->r_info);
                K2_ASSERT(NULL != gpSymTrack[secIx].mpOut);
                pRel->r_offset -= pSecHdr->sh_addr;
                pRel->r_offset += gpSectionTarget[gSecIx.mText].mAddr;
                pRel->r_info = ELF32_R_INFO(gpSymTrack[secIx].mOutIx, ELF32_R_TYPE(pRel->r_info));
                pRel++;
            }
            pRelHdr->mTextRelocsBytes *= sizeof(Elf32_Rel);
        }

        if (0 != gSecIx.mReadRel)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mReadRel * apParse->mSectionHeaderTableEntryBytes));
            relEntBytes = pSecHdr->sh_entsize;
            if (0 == relEntBytes)
                relEntBytes = sizeof(Elf32_Rel);
            pRelHdr->mReadRelocsBytes = (pSecHdr->sh_size / relEntBytes);
            pSrcData = LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mRead * apParse->mSectionHeaderTableEntryBytes));
            for (ix = 0; ix < pRelHdr->mReadRelocsBytes; ix++)
            {
                K2MEM_Copy(pRel, pSrcData + (ix * relEntBytes), sizeof(Elf32_Rel));
                secIx = ELF32_R_SYM(pRel->r_info);
                K2_ASSERT(NULL != gpSymTrack[secIx].mpOut);
                pRel->r_offset -= pSecHdr->sh_addr;
                pRel->r_offset += gpSectionTarget[gSecIx.mRead].mAddr;
                pRel->r_info = ELF32_R_INFO(gpSymTrack[secIx].mOutIx, ELF32_R_TYPE(pRel->r_info));
                pRel++;
            }
            pRelHdr->mReadRelocsBytes *= sizeof(Elf32_Rel);
        }

        if (0 != gSecIx.mDataRel)
        {
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mDataRel * apParse->mSectionHeaderTableEntryBytes));
            relEntBytes = pSecHdr->sh_entsize;
            if (0 == relEntBytes)
                relEntBytes = sizeof(Elf32_Rel);
            pRelHdr->mDataRelocsBytes = (pSecHdr->sh_size / relEntBytes);
            pSrcData = LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
            pSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (gSecIx.mData * apParse->mSectionHeaderTableEntryBytes));
            for (ix = 0; ix < pRelHdr->mDataRelocsBytes; ix++)
            {
                K2MEM_Copy(pRel, pSrcData + (ix * relEntBytes), sizeof(Elf32_Rel));
                secIx = ELF32_R_SYM(pRel->r_info);
                K2_ASSERT(NULL != gpSymTrack[secIx].mpOut);
                pRel->r_offset -= pSecHdr->sh_addr;
                pRel->r_offset += gpSectionTarget[gSecIx.mData].mAddr;
                pRel->r_info = ELF32_R_INFO(gpSymTrack[secIx].mOutIx, ELF32_R_TYPE(pRel->r_info));
                pRel++;
            }
            pRelHdr->mDataRelocsBytes *= sizeof(Elf32_Rel);
        }

        K2_ASSERT(pHeader->Segment[XDLSegmentIx_Relocs].mMemActualBytes == (((UINT_PTR)pRel) - ((UINT_PTR)pRelHdr)));

        curSector += pHeader->Segment[XDLSegmentIx_Relocs].mSectorCount;
    }

    K2_ASSERT(curSector == gFileSectors);

    //
    // set crcs everywhere
    //
    curSector = pHeader->Segment[XDLSegmentIx_Header].mSectorCount;
    for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Count; ix++)
    {
        if (pHeader->Segment[ix].mSectorCount > 0)
        {
            pHeader->Segment[ix].mCrc32 = K2CRC_Calc32(
                0, 
                ((UINT8 *)gpOutFile) + (UINT_PTR)(curSector * XDL_SECTOR_BYTES), 
                (UINT_PTR)(pHeader->Segment[ix].mSectorCount * XDL_SECTOR_BYTES));
            curSector += pHeader->Segment[ix].mSectorCount;
        }
    }
    K2_ASSERT(curSector == gFileSectors);
    pHeader->Segment[XDLSegmentIx_Header].mCrc32 = K2CRC_Calc32(0, gpOutFile, (UINT_PTR)(pHeader->Segment[XDLSegmentIx_Header].mSectorCount * XDL_SECTOR_BYTES));
    pHeader->mHeaderCrc32 = K2CRC_Calc32(0, &pHeader->mMarker, sizeof(XDL_FILE_HEADER) - sizeof(UINT32));

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
    BOOL bOk = WriteFile(hFile, gpOutFile, ((DWORD)gFileSectors) * XDL_SECTOR_BYTES, &wrote, NULL);
    CloseHandle(hFile);
    if ((!bOk) || (wrote != ((DWORD)gFileSectors) * XDL_SECTOR_BYTES))
    {
        printf("*** Failed to write output file\n");
        DeleteFile(gOut.mpOutputFilePath);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return K2STAT_NO_ERROR;
}

K2STAT
OutputFixed32(
    K2ELF32PARSE *  apParse
)
{
    UINT_PTR            workAddr;
    UINT_PTR            align;
    Elf32_Shdr const *  pSecHdr;
    XDL_FILE_HEADER     workHdr;

    K2MEM_Zero(&workHdr, sizeof(workHdr));

    workAddr = (UINT_PTR)gOut.mPlacement;
    K2_ASSERT(0 == (workAddr & K2_VA_MEMPAGE_OFFSET_MASK));

    //
    // header segment
    //
    workHdr.Segment[XDLSegmentIx_Header].mLinkAddr = workAddr;
    workHdr.mPlacement = workAddr + K2_FIELDOFFSET(XDL_FILE_HEADER, mPlacement);
    workHdr.Segment[XDLSegmentIx_Header].mMemActualBytes = sizeof(XDL_FILE_HEADER);
    workAddr += K2_VA_MEMPAGE_BYTES;
    workHdr.Segment[XDLSegmentIx_Header].mSectorCount = gFileSectors = K2_VA_MEMPAGE_BYTES / XDL_SECTOR_BYTES;

    //
    // text segment if it exists
    //
    if (0 != gSecIx.mText)
    {
        pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mText * apParse->mSectionHeaderTableEntryBytes));
        gpSectionTarget[gSecIx.mText].mAddr = workAddr;
        workHdr.Segment[XDLSegmentIx_Text].mLinkAddr = workAddr;
        workAddr += pSecHdr->sh_size;
        workHdr.Segment[XDLSegmentIx_Text].mMemActualBytes = pSecHdr->sh_size;
        workAddr = K2_ROUNDUP(workAddr, K2_VA_MEMPAGE_BYTES);
        workHdr.Segment[XDLSegmentIx_Text].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Text].mLinkAddr) / XDL_SECTOR_BYTES;
        gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Text].mSectorCount;
    }

    //
    // read segment if it exists
    //
    if (0 != gSecIx.mRead)
    {
        pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mRead * apParse->mSectionHeaderTableEntryBytes));
        gpSectionTarget[gSecIx.mRead].mAddr = workAddr;
        workHdr.Segment[XDLSegmentIx_Read].mLinkAddr = workAddr;
        workAddr += pSecHdr->sh_size;
        workHdr.Segment[XDLSegmentIx_Read].mMemActualBytes = pSecHdr->sh_size;
        workAddr = K2_ROUNDUP(workAddr, K2_VA_MEMPAGE_BYTES);
        workHdr.Segment[XDLSegmentIx_Read].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Read].mLinkAddr) / XDL_SECTOR_BYTES;
        gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Read].mSectorCount;
    }

    //
    // data and bss segments if they exist
    //
    if ((0 != gSecIx.mData) || 
        (0 != gSecIx.mBss))
    {
        workHdr.Segment[XDLSegmentIx_Data].mLinkAddr = workAddr;
        if (0 != gSecIx.mData)
        {
            gpSectionTarget[gSecIx.mData].mAddr = workAddr;
            pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mData * apParse->mSectionHeaderTableEntryBytes));
            workAddr += pSecHdr->sh_size;
            if (0 != gSecIx.mBss)
            {
                pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mBss * apParse->mSectionHeaderTableEntryBytes));
                align = pSecHdr->sh_addralign;
                if (0 != align)
                {
                    workAddr = ((workAddr + (align - 1)) / align) * align;
                }
            }
        }
        if (0 != gSecIx.mBss)
        {
            gpSectionTarget[gSecIx.mBss].mAddr = workAddr;
            pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mBss * apParse->mSectionHeaderTableEntryBytes));
            workAddr += pSecHdr->sh_size;
        }
        workHdr.Segment[XDLSegmentIx_Data].mMemActualBytes = workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mLinkAddr;
        workAddr = K2_ROUNDUP(workAddr, K2_VA_MEMPAGE_BYTES);
        workHdr.Segment[XDLSegmentIx_Data].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mLinkAddr) / XDL_SECTOR_BYTES;
        gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mSectorCount;
    }

    //
    // symbol table
    //
    workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr = workAddr;
    workAddr += gSymOutCount * sizeof(Elf32_Sym);
    workHdr.mSymCount_Abs = gTreeAbs.mNodeCount;
    workHdr.mSymCount[XDLProgData_Text] = gTreeSeg[XDLProgData_Text].mNodeCount;
    workHdr.mSymCount[XDLProgData_Read] = gTreeSeg[XDLProgData_Read].mNodeCount;
    workHdr.mSymCount[XDLProgData_Data] = gTreeSeg[XDLProgData_Data].mNodeCount;
    workHdr.mSymCount_Imp = gTreeImp.mNodeCount;
    workAddr += gSymStrSize;
    workHdr.Segment[XDLSegmentIx_Symbols].mMemActualBytes = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr);
    workAddr = K2_ROUNDUP(workAddr, XDL_SECTOR_BYTES);
    workHdr.Segment[XDLSegmentIx_Symbols].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr) / XDL_SECTOR_BYTES;
    gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mSectorCount;

    return Output32(apParse, &workHdr);
}

K2STAT
OutputReloc32(
    K2ELF32PARSE *  apParse
)
{
    UINT_PTR            workAddr;
    UINT_PTR            align;
    UINT_PTR            relCount;
    Elf32_Shdr const *  pSecHdr;
    XDL_FILE_HEADER     workHdr;

    K2MEM_Zero(&workHdr, sizeof(workHdr));

    workAddr = BASE_ADDR_RELOC32;

    //
    // header segment
    //
    workHdr.Segment[XDLSegmentIx_Header].mLinkAddr = workAddr;
    workAddr += sizeof(XDL_FILE_HEADER);
    workAddr = K2_ROUNDUP(workAddr, sizeof(XDL_IMPORT));
    workHdr.mImportsOffset = workAddr - BASE_ADDR_RELOC32;
    workAddr += gImportCount * sizeof(XDL_IMPORT);
    workHdr.Segment[XDLSegmentIx_Header].mMemActualBytes = workAddr - BASE_ADDR_RELOC32;
    workAddr = K2_ROUNDUP(workAddr, XDL_SECTOR_BYTES);
    workHdr.Segment[XDLSegmentIx_Header].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Header].mLinkAddr) / XDL_SECTOR_BYTES;
    gFileSectors = (UINT_PTR)workHdr.Segment[XDLSegmentIx_Header].mSectorCount;

    //
    // text segment if it exists
    //
    if (0 != gSecIx.mText)
    {
        workAddr = K2_ROUNDUP(workAddr, K2_VA_MEMPAGE_BYTES);
        pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mText * apParse->mSectionHeaderTableEntryBytes));
        gpSectionTarget[gSecIx.mText].mAddr = workAddr;
        workHdr.Segment[XDLSegmentIx_Text].mLinkAddr = workAddr;
        workAddr += pSecHdr->sh_size;
        workHdr.Segment[XDLSegmentIx_Text].mMemActualBytes = pSecHdr->sh_size;
        workAddr = K2_ROUNDUP(workAddr, XDL_SECTOR_BYTES);
        workHdr.Segment[XDLSegmentIx_Text].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Text].mLinkAddr) / XDL_SECTOR_BYTES;
        gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Text].mSectorCount;
    }

    //
    // read segment if it exists
    //
    if (0 != gSecIx.mRead)
    {
        workAddr = K2_ROUNDUP(workAddr, K2_VA_MEMPAGE_BYTES);
        pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mRead * apParse->mSectionHeaderTableEntryBytes));
        gpSectionTarget[gSecIx.mRead].mAddr = workAddr;
        workHdr.Segment[XDLSegmentIx_Read].mLinkAddr = workAddr;
        workAddr += pSecHdr->sh_size;
        workHdr.Segment[XDLSegmentIx_Read].mMemActualBytes = pSecHdr->sh_size;
        workAddr = K2_ROUNDUP(workAddr, XDL_SECTOR_BYTES);
        workHdr.Segment[XDLSegmentIx_Read].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Read].mLinkAddr) / XDL_SECTOR_BYTES;
        gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Read].mSectorCount;
    }

    //
    // data and bss segments if they exist
    //
    if ((0 != gSecIx.mData) ||
        (0 != gSecIx.mBss))
    {
        workAddr = K2_ROUNDUP(workAddr, K2_VA_MEMPAGE_BYTES);

        workHdr.Segment[XDLSegmentIx_Data].mLinkAddr = workAddr;
        if (0 != gSecIx.mData)
        {
            gpSectionTarget[gSecIx.mData].mAddr = workAddr;
            pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mData * apParse->mSectionHeaderTableEntryBytes));
            workAddr += pSecHdr->sh_size;
        }
        workHdr.Segment[XDLSegmentIx_Data].mSectorCount = K2_ROUNDUP(workAddr, XDL_SECTOR_BYTES);
        workHdr.Segment[XDLSegmentIx_Data].mSectorCount = (workHdr.Segment[XDLSegmentIx_Data].mSectorCount - workHdr.Segment[XDLSegmentIx_Data].mLinkAddr) / XDL_SECTOR_BYTES;
        gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mSectorCount;
        if (0 != gSecIx.mBss)
        {
            pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mBss * apParse->mSectionHeaderTableEntryBytes));
            align = pSecHdr->sh_addralign;
            if (0 != align)
            {
                workAddr = ((workAddr + (align - 1)) / align) * align;
            }
            gpSectionTarget[gSecIx.mBss].mAddr = workAddr;
            gpSectionTarget[gSecIx.mBss].mSegOffset = workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mLinkAddr;
            workAddr += pSecHdr->sh_size;
        }
        workHdr.Segment[XDLSegmentIx_Data].mMemActualBytes = workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Data].mLinkAddr;
        workAddr = K2_ROUNDUP(workAddr, XDL_SECTOR_BYTES);
    }

    //
    // everything else is discardable
    //

    //
    // symbol table
    //
    workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr = workAddr;
    workAddr += gSymOutCount * sizeof(Elf32_Sym);
    workHdr.mSymCount_Abs = gTreeAbs.mNodeCount;
    workHdr.mSymCount[XDLProgData_Text] = gTreeSeg[XDLProgData_Text].mNodeCount;
    workHdr.mSymCount[XDLProgData_Read] = gTreeSeg[XDLProgData_Read].mNodeCount;
    workHdr.mSymCount[XDLProgData_Data] = gTreeSeg[XDLProgData_Data].mNodeCount;
    workHdr.mSymCount_Imp = gTreeImp.mNodeCount;
    workAddr += gSymStrSize;
    workHdr.Segment[XDLSegmentIx_Symbols].mMemActualBytes = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr);
    workAddr = K2_ROUNDUP(workAddr, XDL_SECTOR_BYTES);
    workHdr.Segment[XDLSegmentIx_Symbols].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr) / XDL_SECTOR_BYTES;
    gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Symbols].mSectorCount;
    workHdr.Segment[XDLSegmentIx_Symbols].mLinkAddr = 0;

    //
    // relocations
    //
    if ((0 != gSecIx.mTextRel) ||
        (0 != gSecIx.mReadRel) ||
        (0 != gSecIx.mDataRel))
    {
        relCount = 0;
        workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr = workAddr;
        workAddr += sizeof(XDL_RELOC_SEGMENT_HEADER);
        if (0 != gSecIx.mTextRel)
        {
            gpSectionTarget[gSecIx.mTextRel].mAddr = workAddr;
            gpSectionTarget[gSecIx.mTextRel].mSegOffset = workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr;

            pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mTextRel * apParse->mSectionHeaderTableEntryBytes));
            align = pSecHdr->sh_entsize;
            if (0 == align)
                align = sizeof(Elf32_Rel);
            workAddr += (pSecHdr->sh_size / align) * sizeof(Elf32_Rel);
        }
        if (0 != gSecIx.mReadRel)
        {
            gpSectionTarget[gSecIx.mReadRel].mAddr = workAddr;
            gpSectionTarget[gSecIx.mReadRel].mSegOffset = workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr;

            pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mReadRel * apParse->mSectionHeaderTableEntryBytes));
            align = pSecHdr->sh_entsize;
            if (0 == align)
                align = sizeof(Elf32_Rel);
            workAddr += (pSecHdr->sh_size / align) * sizeof(Elf32_Rel);
        }
        if (0 != gSecIx.mDataRel)
        {
            gpSectionTarget[gSecIx.mDataRel].mAddr = workAddr;
            gpSectionTarget[gSecIx.mDataRel].mSegOffset = workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr;

            pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mDataRel * apParse->mSectionHeaderTableEntryBytes));
            align = pSecHdr->sh_entsize;
            if (0 == align)
                align = sizeof(Elf32_Rel);
            workAddr += (pSecHdr->sh_size / align) * sizeof(Elf32_Rel);
        }

        workHdr.Segment[XDLSegmentIx_Relocs].mMemActualBytes = workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr;
        workAddr = K2_ROUNDUP(workAddr, XDL_SECTOR_BYTES);
        workHdr.Segment[XDLSegmentIx_Relocs].mSectorCount = (workAddr - (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr) / XDL_SECTOR_BYTES;
        gFileSectors += (UINT_PTR)workHdr.Segment[XDLSegmentIx_Relocs].mSectorCount;

        workHdr.Segment[XDLSegmentIx_Relocs].mLinkAddr = 0;
    }

    return Output32(apParse, &workHdr);
}

K2STAT
CreateOutputFile32(
    K2ELF32PARSE *  apParse
)
{
    UINT_PTR            ixSection;
    Elf32_Shdr const *  pSecHdr;
    Elf32_Shdr const *  pOtherSecHdr;
    Elf32_Rel const *   pRel;
    UINT_PTR            relEntBytes;
    UINT_PTR            entLeft;
    Elf32_Sym const *   pSym;
    SymTrack *          pSymTrack;
    K2TREE_NODE *       pTreeNode;
    BOOL                takeSymbol;

    gSectionCount = apParse->mpRawFileData->e_shnum;
    gpSectionTarget = new SecTarget[gSectionCount];
    if (NULL == gpSectionTarget)
    {
        printf("*** Out of memory\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    K2MEM_Zero(gpSectionTarget, sizeof(SecTarget) * gSectionCount);
    K2MEM_Zero(&gSecIx, sizeof(gSecIx));
    gImportCount = 0;

    //
    // pass 1
    //
    for (ixSection = 0; ixSection < gSectionCount; ixSection++)
    {
        gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Count;

        pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (ixSection * apParse->mSectionHeaderTableEntryBytes));
        if (0 == pSecHdr->sh_size)
            continue;

        if (0 != (pSecHdr->sh_flags & SHF_ALLOC))
        {
            if (SHT_PROGBITS == pSecHdr->sh_type)
            {
                if (XDL_ELF_SHF_TYPE_IMPORTS == (pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
                {
                    gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Header;
                    gImportCount++;
                }
                else
                {
                    if (pSecHdr->sh_flags & SHF_EXECINSTR)
                    {
                        if (0 != gSecIx.mText)
                        {
                            printf("*** More than one text section found in input (%d and %d)\n", gSecIx.mText, ixSection);
                            return K2STAT_ERROR_BAD_FORMAT;
                        }
                        gSecIx.mText = ixSection;
                        gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Text;
                    }
                    else if (pSecHdr->sh_flags & SHF_WRITE)
                    {
                        if (0 != gSecIx.mData)
                        {
                            printf("*** More than one data section found in input (%d and %d)\n", gSecIx.mData, ixSection);
                            return K2STAT_ERROR_BAD_FORMAT;
                        }
                        gSecIx.mData = ixSection;
                        gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Data;
                    }
                    else
                    {
                        if (0 != gSecIx.mRead)
                        {
                            printf("*** More than one read section found in input (%d and %d)\n", gSecIx.mRead, ixSection);
                            return K2STAT_ERROR_BAD_FORMAT;
                        }
                        gSecIx.mRead = ixSection;
                        gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Read;
                    }
                }
            }
            else
            {
                if (pSecHdr->sh_type == SHT_NOBITS)
                {
                    if (0 != gSecIx.mBss)
                    {
                        printf("*** More than one bss section found in input (%d and %d)\n", gSecIx.mBss, ixSection);
                        return K2STAT_ERROR_BAD_FORMAT;
                    }
                    gSecIx.mBss = ixSection;
                    gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Data;
                }
                else
                {
                    //
                    // alloc section that is not PROGBITS
                    //
                    printf("Section %d in file is an ALLOC section that is not PROGBITS!\n", ixSection);
                    return K2STAT_ERROR_BAD_FORMAT;
                }
            }
        }
        else
        {
            // not an alloc section
            if (pSecHdr->sh_type == SHT_SYMTAB)
            {
                if (0 != gSecIx.mSym)
                {
                    printf("*** More than one symbol table section found in input (%d and %d)\n", gSecIx.mBss, ixSection);
                    return K2STAT_ERROR_BAD_FORMAT;
                }
                gSecIx.mSym = ixSection;
                gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Symbols;

                gSecIx.mSymStr = pSecHdr->sh_link;
            }
        }
    }

    if ((0 == gSecIx.mSym) || (0 == gSecIx.mSymStr))
    {
        printf("*** No symbol table or symbol string table found in input\n");
        return K2STAT_ERROR_BAD_FORMAT;
    }

    //
    // initialize symbol tracking
    //
    pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mSym * apParse->mSectionHeaderTableEntryBytes));
    gpSymIn = (UINT8 const *)LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
    gSymEntSize = pSecHdr->sh_entsize;
    if (0 == gSymEntSize)
        gSymEntSize = sizeof(Elf32_Sym);
    gSymInCount = pSecHdr->sh_size / gSymEntSize;
    pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (gSecIx.mSymStr * apParse->mSectionHeaderTableEntryBytes));
    gpSymStr = (char const *)LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
    gpSymTrack = new SymTrack[gSymInCount];
    if (NULL == gpSymTrack)
    {
        printf("*** out of memory\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    K2MEM_Zero(gpSymTrack, sizeof(SymTrack)*gSymInCount);
    pSym = (Elf32_Sym const *)gpSymIn;
    for (entLeft = 0; entLeft < gSymInCount; entLeft++)
    {
        gpSymTrack[entLeft].mInIx = entLeft;
        gpSymTrack[entLeft].mpIn = pSym;
        gpSymTrack[entLeft].mpName = gpSymStr + pSym->st_name;
        if ((pSym->st_shndx > 0) &&
            (pSym->st_shndx < gSectionCount))
        {
            if (XDLSegmentIx_Header == gpSectionTarget[pSym->st_shndx].mSegmentIx)
            {
                gpSymTrack[entLeft].mInImportSection = true;
            }
        }
        pSym = (Elf32_Sym const *)(((UINT8 const *)pSym) + gSymEntSize);
    }

    //
    // pass 2
    //
    for (ixSection = 0; ixSection < gSectionCount; ixSection++)
    {
        pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (ixSection * apParse->mSectionHeaderTableEntryBytes));
        if (0 == pSecHdr->sh_size)
            continue;

        if (pSecHdr->sh_type == SHT_REL)
        {
            if (gpSectionTarget[pSecHdr->sh_info].mSegmentIx != XDLSegmentIx_Count)
            {
                //
                // relocs for a section we are taking
                //
                K2_ASSERT(pSecHdr->sh_link == gSecIx.mSym);
                pOtherSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (pSecHdr->sh_info * apParse->mSectionHeaderTableEntryBytes));
                K2_ASSERT(SHT_PROGBITS == pOtherSecHdr->sh_type);
                if (XDL_ELF_SHF_TYPE_IMPORTS == (pOtherSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK))
                {
                    printf("*** Import section %d has relocation in section %d, and its not allowed to\n", pSecHdr->sh_info, ixSection);
                }
                else
                {
                    if (pOtherSecHdr->sh_flags & SHF_EXECINSTR)
                    {
                        if (0 != gSecIx.mTextRel)
                        {
                            printf("*** More than one reloc section for text found in input (%d and %d)\n", gSecIx.mTextRel, ixSection);
                            return K2STAT_ERROR_BAD_FORMAT;
                        }
                        gSecIx.mTextRel = ixSection;
                        gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Relocs;
                    }
                    else if (pOtherSecHdr->sh_flags & SHF_WRITE)
                    {
                        if (0 != gSecIx.mDataRel)
                        {
                            printf("*** More than one reloc section for data found in input (%d and %d)\n", gSecIx.mDataRel, ixSection);
                            return K2STAT_ERROR_BAD_FORMAT;
                        }
                        gSecIx.mDataRel = ixSection;
                        gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Relocs;
                    }
                    else
                    {
                        if (0 != gSecIx.mReadRel)
                        {
                            printf("*** More than one reloc section for read found in input (%d and %d)\n", gSecIx.mReadRel, ixSection);
                            return K2STAT_ERROR_BAD_FORMAT;
                        }
                        gSecIx.mReadRel = ixSection;
                        gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Relocs;
                    }
                }
            }
            if (gpSectionTarget[ixSection].mSegmentIx == XDLSegmentIx_Relocs)
            {
                //
                // we are taking this relocations section
                //
                relEntBytes = pSecHdr->sh_entsize;
                if (0 == relEntBytes)
                    relEntBytes = sizeof(Elf32_Rel);
                pRel = (Elf32_Rel const *)LoadOffsetToDataPtr32(apParse, pSecHdr->sh_offset, NULL);
                entLeft = pSecHdr->sh_size / sizeof(Elf32_Rel);
                do {
                    pSymTrack = &gpSymTrack[ELF32_R_SYM(pRel->r_info)];
                    pSymTrack->mUsedInReloc = true;
                    pSym = pSymTrack->mpIn;
                    if ((pSym->st_shndx > 0) &&
                        (pSym->st_shndx < gSectionCount))
                    {
                        //
                        // we must be taking the section the symbol is in
                        //
                        K2_ASSERT(XDLSegmentIx_Count != gpSectionTarget[pSym->st_shndx].mSegmentIx);

                        //
                        // inc count of symbols in this section that are used in relocs
                        //
                        gpSectionTarget[pSym->st_shndx].mRelocSymSrcCount++;
                    }
                    pRel = (Elf32_Rel const *)(((UINT8 const *)pRel) + relEntBytes);
                } while (--entLeft);
            }
        }
    }

    //
    // any imports sections that do not have relocs pointing at them can be removed
    //
    for (ixSection = 0; ixSection < gSectionCount; ixSection++)
    {
//        pSecHdr = (Elf32_Shdr const *)(apParse->mpSectionHeaderTable + (ixSection * apParse->mSectionHeaderTableEntryBytes));
//        printf("%2d: %2d f%08X @%08X\n", ixSection, pSecHdr->sh_type, pSecHdr->sh_flags, pSecHdr->sh_addr);

        if (XDLSegmentIx_Header != gpSectionTarget[ixSection].mSegmentIx)
            continue;
        if (0 == gpSectionTarget[ixSection].mRelocSymSrcCount)
        {
            //
            // this imports section has no relocs targeting it
            // so it is useless and can be removed from the output
            //
//            printf("Import section %d removed because it is not used\n", ixSection);
            gpSectionTarget[ixSection].mSegmentIx = XDLSegmentIx_Count;
            gImportCount--;
        }
    }

    //
    // take out unused symbols and sort the rest of them
    //
    K2TREE_Init(&gTreeAbs, NULL);
    for (entLeft = 0; entLeft < XDLProgDataType_Count; entLeft++)
    {
        K2TREE_Init(&gTreeSeg[entLeft], NULL);
    }
    K2TREE_Init(&gTreeImp, NULL);
    for (entLeft = 0; entLeft < gSymInCount; entLeft++)
    {
        pSym = gpSymTrack[entLeft].mpIn;
        relEntBytes = pSym->st_shndx;
        takeSymbol = TRUE;

        if (!gpSymTrack[entLeft].mUsedInReloc)
        {
            //
            // symbol is not used in relocations
            // if it is an absolute, null-sized, targeting an unused section, or a static we cull it
            //
            if ((relEntBytes >= gSectionCount) ||
                (pSym->st_size == 0) ||
                (XDLSegmentIx_Count == gpSectionTarget[relEntBytes].mSegmentIx) ||
                (STB_LOCAL == ELF32_ST_BIND(pSym->st_info)))
            {
                //
                // omit this superfluous symbol
                //
//                printf("Cull %d \"%s\"\n", gpSymTrack[entLeft].mpIn->st_value, gpSymTrack[entLeft].mpName);
                takeSymbol = FALSE;
                gpSymTrack[entLeft].mpIn = NULL;
                gpSymTrack[entLeft].mInIx = 0;
                gpSymTrack[entLeft].mpName = NULL;
            }
        }

        if (takeSymbol)
        {
            //
            // symbol used in relocations, or is a non-zero sized global in a used section
            //
            if ((0 == relEntBytes) ||
                (relEntBytes >= gSectionCount))
            {
                gpSymTrack[entLeft].mProgDataTreeTarget = -1;
                K2TREE_Insert(&gTreeAbs, gpSymTrack[entLeft].TreeNode.mUserVal, &gpSymTrack[entLeft].TreeNode);
            }
            else
            {
                gpSymTrack[entLeft].TreeNode.mUserVal = pSym->st_value;
                K2_ASSERT(XDLSegmentIx_Count != gpSectionTarget[relEntBytes].mSegmentIx);
                if (gpSymTrack[entLeft].mInImportSection)
                {
                    K2_ASSERT(XDLSegmentIx_Header == gpSectionTarget[relEntBytes].mSegmentIx);
                    gpSymTrack[entLeft].mProgDataTreeTarget = XDLProgDataType_Count;
                    K2TREE_Insert(&gTreeImp, gpSymTrack[entLeft].TreeNode.mUserVal, &gpSymTrack[entLeft].TreeNode);
                }
                else
                {
                    if (XDLSegmentIx_Text == gpSectionTarget[relEntBytes].mSegmentIx)
                    {
                        gpSymTrack[entLeft].mProgDataTreeTarget = XDLProgData_Text;
                        K2TREE_Insert(&gTreeSeg[XDLProgData_Text], gpSymTrack[entLeft].TreeNode.mUserVal, &gpSymTrack[entLeft].TreeNode);
                    }
                    else if (XDLSegmentIx_Read == gpSectionTarget[relEntBytes].mSegmentIx)
                    {
                        gpSymTrack[entLeft].mProgDataTreeTarget = XDLProgData_Read;
                        K2TREE_Insert(&gTreeSeg[XDLProgData_Read], gpSymTrack[entLeft].TreeNode.mUserVal, &gpSymTrack[entLeft].TreeNode);
                    }
                    else 
                    {
                        K2_ASSERT(XDLSegmentIx_Data == gpSectionTarget[relEntBytes].mSegmentIx);
                        gpSymTrack[entLeft].mProgDataTreeTarget = XDLProgData_Data;
                        K2TREE_Insert(&gTreeSeg[XDLProgData_Data], gpSymTrack[entLeft].TreeNode.mUserVal, &gpSymTrack[entLeft].TreeNode);
                    }
                }
            }
        }
    }

    relEntBytes = 0;
    gSymStrSize = 0;

//    printf("ABS: %d\n", gTreeAbs.mNodeCount);
    pTreeNode = K2TREE_FirstNode(&gTreeAbs);
    if (NULL != pTreeNode)
    {
        do {
            pSymTrack = K2_GET_CONTAINER(SymTrack, pTreeNode, TreeNode);
            pSymTrack->mOutIx = relEntBytes;
            gSymStrSize += K2ASC_Len(pSymTrack->mpName) + 1;
//            printf("  %08X \"%s\"\n", pSymTrack->mpIn->st_value, pSymTrack->mpName);
            pTreeNode = K2TREE_NextNode(&gTreeAbs, pTreeNode);
            relEntBytes++;
        } while (NULL != pTreeNode);
    }

//    printf("TEXT: %d\n", gTreeSeg[XDLProgData_Text].mNodeCount);
    pTreeNode = K2TREE_FirstNode(&gTreeSeg[XDLProgData_Text]);
    if (NULL != pTreeNode)
    {
        do {
            pSymTrack = K2_GET_CONTAINER(SymTrack, pTreeNode, TreeNode);
            pSymTrack->mOutIx = relEntBytes;
            gSymStrSize += K2ASC_Len(pSymTrack->mpName) + 1;
//            printf("  %08X \"%s\"\n", pSymTrack->mpIn->st_value, pSymTrack->mpName);
            pTreeNode = K2TREE_NextNode(&gTreeSeg[XDLProgData_Text], pTreeNode);
            relEntBytes++;
        } while (NULL != pTreeNode);
    }

//    printf("READ: %d\n", gTreeSeg[XDLProgData_Read].mNodeCount);
    pTreeNode = K2TREE_FirstNode(&gTreeSeg[XDLProgData_Read]);
    if (NULL != pTreeNode)
    {
        do {
            pSymTrack = K2_GET_CONTAINER(SymTrack, pTreeNode, TreeNode);
            pSymTrack->mOutIx = relEntBytes;
            gSymStrSize += K2ASC_Len(pSymTrack->mpName) + 1;
//            printf("  %08X \"%s\"\n", pSymTrack->mpIn->st_value, pSymTrack->mpName);
            pTreeNode = K2TREE_NextNode(&gTreeSeg[XDLProgData_Read], pTreeNode);
            relEntBytes++;
        } while (NULL != pTreeNode);
    }

//    printf("DATA: %d\n", gTreeSeg[XDLProgData_Data].mNodeCount);
    pTreeNode = K2TREE_FirstNode(&gTreeSeg[XDLProgData_Data]);
    if (NULL != pTreeNode)
    {
        do {
            pSymTrack = K2_GET_CONTAINER(SymTrack, pTreeNode, TreeNode);
            pSymTrack->mOutIx = relEntBytes;
            gSymStrSize += K2ASC_Len(pSymTrack->mpName) + 1;
//            printf("  %08X \"%s\"\n", pSymTrack->mpIn->st_value, pSymTrack->mpName);
            pTreeNode = K2TREE_NextNode(&gTreeSeg[XDLProgData_Data], pTreeNode);
            relEntBytes++;
        } while (NULL != pTreeNode);
    }

//    printf("IMPORTED: %d\n", gTreeImp.mNodeCount);
    pTreeNode = K2TREE_FirstNode(&gTreeImp);
    if (NULL != pTreeNode)
    {
        do {
            pSymTrack = K2_GET_CONTAINER(SymTrack, pTreeNode, TreeNode);
            pSymTrack->mOutIx = relEntBytes;
            gSymStrSize += K2ASC_Len(pSymTrack->mpName) + 1;
//            printf("  %08X \"%s\"\n", pSymTrack->mpIn->st_value, pSymTrack->mpName);
            pTreeNode = K2TREE_NextNode(&gTreeImp, pTreeNode);
            relEntBytes++;
        } while (NULL != pTreeNode);
    }
    gSymOutCount = relEntBytes;
//    printf("TOTAL IN:  %d\n", gSymInCount);
//    printf("TOTAL OUT: %d\n", gSymOutCount);

    gFileSectors = 0;

    if (gOut.mUsePlacement)
    {
        if (0 != gImportCount)
        {
            printf("*** placement specified and the input ELF has imports that are used, which is not allowed\n");
            return K2STAT_ERROR_BAD_FORMAT;
        }
        return OutputFixed32(apParse);
    }

    return OutputReloc32(apParse);
}
