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
#include "idlx.h"

typedef 
K2STAT 
(*pfLinkFunc)(
    UINT8 * apRelAddr,
    UINT32  aOrigRelAddr,
    UINT32  aRelocType,
    UINT32  aSymVal
    );

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

static
K2STAT
sUnlinkOneA32(
    UINT8 * apRelAddr,
    UINT32  aOrigRelAddr,
    UINT32  aRelocType,
    UINT32  aSymVal
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
        targetAddend = (valAtTarget & 0xFFFFFF)<<2;
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
        targetAddend -= ((aSymVal>>16) & 0xFFFF);
        valAtTarget = ((valAtTarget) & 0xFFF0F000) | ((targetAddend << 4) & 0xF0000) | (targetAddend & 0xFFF);
        break;

    case R_ARM_PREL31:
        // value is an addend already
        break;

    default:
        return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
    }
    K2MEM_Copy(apRelAddr, &valAtTarget, sizeof(UINT32));

    return 0;
}

static
K2STAT
sRelinkOneA32(
    UINT8 * apRelAddr,
    UINT32  aNewRelAddr,
    UINT32  aRelocType,
    UINT32  aSymVal
    )
{
    UINT32 valAtTarget;
    UINT32 targetAddend;
    UINT32 newVal;

    K2MEM_Copy(&valAtTarget, apRelAddr, sizeof(UINT32));
    switch (aRelocType)
    {
    case R_ARM_PC24:
    case R_ARM_CALL:
    case R_ARM_JUMP24:
        targetAddend = (valAtTarget & 0xFFFFFF)<<2;
        if (targetAddend & 0x2000000)
            targetAddend |= 0xFC000000;
        newVal = (aSymVal + targetAddend) - (aNewRelAddr + 8);
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
        newVal = ((((aSymVal & 0xFFFF0000) + (targetAddend<<16))) >> 16) & 0xFFFF;
        valAtTarget = (valAtTarget & 0xFFF0F000) | ((newVal << 4) & 0xF0000) | (newVal & 0xFFF);
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
    return 0;
}

#if (!INCLUDE_BOTH)
#define sUnlinkOne sUnlinkOneA32
#define sRelinkOne sRelinkOneA32
#endif
#endif

#if (K2_TARGET_ARCH_IS_INTEL || INCLUDE_BOTH)

static
K2STAT
sUnlinkOneX32(
    UINT8 * apRelAddr,
    UINT32  aOrigRelAddr,
    UINT32  aRelocType,
    UINT32  aSymVal
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
        return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
    }
    K2MEM_Copy(apRelAddr, &valAtTarget, sizeof(UINT32));

    return 0;
}

static
K2STAT
sRelinkOneX32(
    UINT8 * apRelAddr,
    UINT32  aNewRelAddr,
    UINT32  aRelocType,
    UINT32  aSymVal
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
#define sUnlinkOne sUnlinkOneX32
#define sRelinkOne sRelinkOneX32
#endif
#endif


static
DLX *
sGetImportByIndex(
    DLX *   apDlx,
    UINT32  aIndex
    )
{
    DLX_INFO *      pInfo;
    UINT8 *         pWork;
    DLX_IMPORT *    pImport;

    pInfo = apDlx->mpInfo;
    
    pWork = ((UINT8 *)pInfo) + sizeof(DLX_INFO) - sizeof(UINT32) + apDlx->mIntNameFieldLen;
    pImport = (DLX_IMPORT *)pWork;
    while (aIndex--)
    {
        pWork += pImport->mSizeBytes;
        pImport = (DLX_IMPORT *)pWork;
    }
    return (DLX *)pImport->mReserved;
}

static
K2STAT
sLocateImports(
    K2DLX_SECTOR * apSector
    )
{
    Elf32_Shdr *            pSecHdrArray;
    Elf32_Shdr *            pSecHdr;
    UINT32                  secCount;
    UINT32                  secIx;
    DLX_EXPORTS_SECTION *   pExpSec;
    DLX_EXPORTS_SECTION *   pExpCheck;
    K2_GUID128 *            pGuid;
    DLX *                   pImportFrom;
    DLX_INFO *              pImportInfo;

    secCount = apSector->Module.mpElf->e_shnum;
    pSecHdrArray = apSector->Module.mpSecHdr;

    for (secIx = 3; secIx < secCount;secIx++)
    {
        pSecHdr = &pSecHdrArray[secIx];
        if ((pSecHdr->sh_flags & DLX_SHF_TYPE_MASK) == (DLX_SHF_TYPE_IMPORTS))
        {
            K2_ASSERT(pSecHdr->sh_link != 0);

            pExpSec = (DLX_EXPORTS_SECTION *)apSector->mSecAddr[secIx];

            pGuid = (K2_GUID128 *)(((UINT8 *)pExpSec) +
                (sizeof(DLX_EXPORTS_SECTION) - sizeof(DLX_EXPORT_REF)) +
                (sizeof(DLX_EXPORT_REF) * pExpSec->mCount));

            pImportFrom = sGetImportByIndex(&apSector->Module, pSecHdr->sh_link - 1);

            pImportInfo = pImportFrom->mpInfo;

            if (0 != K2MEM_Compare(pGuid, &pImportInfo->ID, sizeof(K2_GUID128)))
                return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);

            // set sh_link to DATA address of exports
            if (pSecHdr->sh_flags & SHF_EXECINSTR)
            {
                if (pImportInfo->mpExpCode == NULL)
                    return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_CODE_IMPORTS_NOT_FOUND);
                pExpCheck = pImportFrom->mpExpCodeDataAddr;
                K2_ASSERT(pExpCheck != NULL);
            }
            else if (pSecHdr->sh_flags & SHF_WRITE)
            {
                if (pImportInfo->mpExpData == NULL)
                    return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_DATA_IMPORTS_NOT_FOUND);
                pExpCheck = pImportFrom->mpExpDataDataAddr;
                K2_ASSERT(pExpCheck != NULL);
            }
            else
            {
                if (pImportInfo->mpExpRead == NULL)
                    return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_READ_IMPORTS_NOT_FOUND);
                pExpCheck = pImportFrom->mpExpReadDataAddr;
                K2_ASSERT(pExpCheck != NULL);
            }
            pSecHdr->sh_link = (Elf32_Word)pExpCheck;

            // sh_entsize is a flag of whether imports are fast 
            // via math or slow via symbol lookup
            if ((pExpCheck->mCount == pExpSec->mCount) &&
                (pExpCheck->mCRC32 == pExpSec->mCRC32))
                pSecHdr->sh_entsize = 1;
            else
                pSecHdr->sh_entsize = 0;
        }
    }

    return 0;
}

static
K2STAT
sChangeAddresses(
    K2DLX_SECTOR * apSector
    )
{
    Elf32_Shdr *        pSecHdrArray;
    Elf32_Shdr *        pSymSecHdr;
    Elf32_Shdr *        pTrgSecHdr;
    char *              pSymStr;
    UINT32              sectionCount;
    UINT32              secIx;
    UINT8 *             pSymBase;
    UINT32              entBytes;
    UINT32              symIx;
    UINT32              symCount;
    Elf32_Sym *         pSym;
    Elf32_Word          symOffset;
    DLX_EXPORT_REF *    pRef;
    K2STAT              status;
    UINT32              entryAddr;
    DLX_INFO *          pInfo;

    sectionCount = apSector->Module.mpElf->e_shnum;
    pSecHdrArray = apSector->Module.mpSecHdr;
    for (secIx = 0; secIx < sectionCount; secIx++)
    {
        pSymSecHdr = &pSecHdrArray[secIx];

        if (pSymSecHdr->sh_type != SHT_SYMTAB)
            continue;

        entBytes = pSymSecHdr->sh_entsize;

        symCount = pSymSecHdr->sh_size / entBytes;

        pSymStr = (char *)apSector->mSecAddr[pSymSecHdr->sh_link];

        pSymBase = (UINT8 *)apSector->mSecAddr[secIx];

        for (symIx = 1; symIx < symCount; symIx++)
        {
            pSym = (Elf32_Sym *)(pSymBase + (symIx * entBytes));

            if ((pSym->st_shndx == 0) ||
                (pSym->st_shndx >= sectionCount))
                continue;

            // symbol targets section in this file
            pTrgSecHdr = &pSecHdrArray[pSym->st_shndx];

            symOffset = pSym->st_value - pTrgSecHdr->sh_addr;

            if ((pTrgSecHdr->sh_flags & DLX_SHF_TYPE_MASK) == DLX_SHF_TYPE_IMPORTS)
            {
                // this is an imported symbol
                // the target section header has:
                //   sh_link = pointer to DATA of EXPORTS section in DLX being imported from
                if (pTrgSecHdr->sh_entsize != 0)
                {
                    // fast link by offset
                    pRef = (DLX_EXPORT_REF *)(pTrgSecHdr->sh_link + symOffset);
                    pSym->st_value = pRef->mAddr;
                }
                else
                {
                    // slow link by lookup
                    status = iK2DLXSUPP_FindExport(
                        (DLX_EXPORTS_SECTION *)pTrgSecHdr->sh_link,
                        pSymStr + pSym->st_name,
                        &pSym->st_value);
                    if (K2STAT_IS_ERROR(status))
                        return status;
                }
            }
            else
            {
                pSym->st_value = apSector->mSecAddr[pSym->st_shndx + sectionCount] + symOffset;
            }
        }
    }

    // change addresses in section headers
    // and update the entrypoint address
    entryAddr = apSector->Module.mpElf->e_entry;
    symIx = 0;
    for (secIx = 1; secIx < sectionCount; secIx++)
    {
        pTrgSecHdr = &pSecHdrArray[secIx];
        if (symIx == 0)
        {
            symCount = pTrgSecHdr->sh_addr;
            if ((entryAddr >= symCount) &&
                (entryAddr < symCount + pTrgSecHdr->sh_size))
            {
                entryAddr -= symCount;
                entryAddr += apSector->mSecAddr[secIx + sectionCount];

                apSector->Module.mEntrypoint = (UINT32)entryAddr;
                apSector->Module.mpElf->e_entry = entryAddr;
                symIx = 1;
            }
        }
        pTrgSecHdr->sh_addr = apSector->mSecAddr[secIx + sectionCount];
    }

    //
    // change segment link addresses in info
    pInfo = apSector->Module.mpInfo;
    for (secIx = 0; secIx < DlxSeg_Count; secIx++)
        pInfo->SegInfo[secIx].mLinkAddr = apSector->Module.SegAlloc.Segment[secIx].mLinkAddr;

    return 0;
}

static
K2STAT
sProcessReloc(
    K2DLX_SECTOR * apSector,
    BOOL                aLink
    )
{
    Elf32_Shdr *    pSecHdrArray;
    Elf32_Shdr *    pRelSecHdr;
    Elf32_Shdr *    pSymSecHdr;
    Elf32_Shdr *    pTrgSecHdr;
    UINT32          secIx;
    UINT32          sectionCount;
    UINT32          relIx;
    UINT32          relEntBytes;
    UINT32          relCount;
    UINT32          symIx;
    UINT32          symEntBytes;
    UINT32          symCount;
    UINT8 *         pSymBase;
    Elf32_Rel *     pRel;
    UINT8 *         pTrgData;
    Elf32_Sym *     pSym;
    K2STAT          status;
    pfLinkFunc      linkFunc;

#if K2_TOOLCHAIN_IS_MS
    sUnlinkOne = (apSector->Module.mpElf->e_machine == EM_X32) ? sUnlinkOneX32 : sUnlinkOneA32;
    sRelinkOne = (apSector->Module.mpElf->e_machine == EM_X32) ? sRelinkOneX32 : sRelinkOneA32;
#endif
    if (!aLink)
        linkFunc = sUnlinkOne;
    else
        linkFunc = sRelinkOne;

    pSecHdrArray = apSector->Module.mpSecHdr;
    sectionCount = apSector->Module.mpElf->e_shnum;

    for (secIx = 3; secIx < sectionCount; secIx++)
    {
        pRelSecHdr = &pSecHdrArray[secIx];
        if (pRelSecHdr->sh_type != SHT_REL)
            continue;

        // get symbol info and data
        pSymSecHdr = &pSecHdrArray[pRelSecHdr->sh_link];
        symEntBytes = pSymSecHdr->sh_entsize;
        if (symEntBytes < sizeof(Elf32_Sym))
            return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
        symCount = pSymSecHdr->sh_size / symEntBytes;
        pSymBase = (UINT8 *)apSector->mSecAddr[pRelSecHdr->sh_link];

        // get relocations target information
        pTrgSecHdr = &pSecHdrArray[pRelSecHdr->sh_info];
        pTrgData = (UINT8 *)apSector->mSecAddr[pRelSecHdr->sh_info];

        // get relocations data
        relEntBytes = pRelSecHdr->sh_entsize;
        if (relEntBytes < sizeof(Elf32_Rel))
            return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
        relCount = pRelSecHdr->sh_size / relEntBytes;

        pRel = (Elf32_Rel *)apSector->mSecAddr[secIx];
        for (relIx = 0; relIx < relCount;relIx++)
        {
            symIx = ELF32_R_SYM(pRel->r_info);
            if ((symIx != 0) && (symIx < symCount))
            {
                // get symbol used in the relocation
                pSym = (Elf32_Sym *)(pSymBase + (symIx * symEntBytes));
                if ((pSym->st_shndx != 0) &&
                    (pSym->st_shndx < sectionCount))
                {
                    // unlink and update relocation to just hold offset into rel target section
                    if (!aLink)
                        pRel->r_offset -= pTrgSecHdr->sh_addr;
                    status = linkFunc(
                        pTrgData + pRel->r_offset, 
                        pTrgSecHdr->sh_addr + pRel->r_offset,
                        ELF32_R_TYPE(pRel->r_info),
                        pSym->st_value);
                    if (K2STAT_IS_ERROR(status))
                        return status;
                }
            }
            pRel = (Elf32_Rel *)(((UINT8 *)pRel) + relEntBytes);
        }
    }

    return 0;
}

static
void
sRetargetTreeNode(
    K2TREE_ANCHOR * apAnchor,
    K2TREE_NODE *   apTreeNode,
    UINT32          aOffset,
    K2TREE_NODE *   apNewNil
    )
{
    K2TREE_NODE *pNode;

    pNode = apTreeNode->mpLeftChild;
    if (pNode != &apAnchor->NilNode)
    {
        sRetargetTreeNode(apAnchor, pNode, aOffset, apNewNil);
        apTreeNode->mpLeftChild = (K2TREE_NODE *)(((UINT32)pNode) + aOffset);
    }
    else
        apTreeNode->mpLeftChild = apNewNil;

    pNode = apTreeNode->mpRightChild;
    if (pNode != &apAnchor->NilNode)
    {
        sRetargetTreeNode(apAnchor, pNode, aOffset, apNewNil);
        apTreeNode->mpRightChild = (K2TREE_NODE *)(((UINT32)pNode) + aOffset);
    }
    else
        apTreeNode->mpRightChild = apNewNil;

    pNode = K2TREE_NODE_PARENT(apTreeNode);
    if (pNode != &apAnchor->NilNode)
    {
        pNode = (K2TREE_NODE *)(((UINT32)pNode) + aOffset);
        K2TREE_NODE_SETPARENT(apTreeNode, pNode);
    }
    else
        K2TREE_NODE_SETPARENT(apTreeNode, apNewNil);
}

static
void 
sRetargetTree(
    K2DLX_SECTOR *  apSector,
    UINT32          aTreeIx
    )
{
    K2DLXSUPP_SEG * pSeg;
    K2TREE_ANCHOR * pAnchor;
    UINT32          segDiff;
    UINT32          nodeDiff;
    K2TREE_NODE *   pNewNilNode;

    pAnchor = &apSector->Module.SymTree[aTreeIx];
    if (pAnchor->mNodeCount == 0)
        return;

    pSeg = &apSector->Module.SegAlloc.Segment[DlxSeg_Sym];
    segDiff = pSeg->mLinkAddr - pSeg->mDataAddr;

    nodeDiff = apSector->Module.mLinkAddr - ((UINT32)&apSector->Module);
    pNewNilNode = (K2TREE_NODE *)(((UINT8 *)&pAnchor->NilNode) + nodeDiff);

    sRetargetTreeNode(pAnchor, pAnchor->RootNode.mpLeftChild, segDiff, pNewNilNode);

    pAnchor->RootNode.mParentBal = (UINT32)pNewNilNode;
    pAnchor->RootNode.mpLeftChild = (K2TREE_NODE *)(((UINT32)pAnchor->RootNode.mpLeftChild) + segDiff);
    pAnchor->RootNode.mpRightChild = pNewNilNode;

    pAnchor->NilNode.mParentBal = (UINT32)pNewNilNode;
    pAnchor->NilNode.mpLeftChild = pNewNilNode;
    pAnchor->NilNode.mpRightChild = pNewNilNode;
}

static
void
sPopulateSymTrees(
    K2DLX_SECTOR *apSector
    )
{
    Elf32_Shdr *            pSymSecHdr;
    Elf32_Shdr *            pTrgSecHdr;
    UINT32                  symStrAddr;
    UINT32                  sectionCount;
    UINT32                  secIx;
    UINT8 *                 pSymBase;
    UINT32                  entBytes;
    UINT32                  symIx;
    UINT32                  symCount;
    Elf32_Sym *             pSym;
    Elf32_Word              symAddr;
    K2DLX_SYMTREE_NODE *    pSymTreeNode;
    K2TREE_ANCHOR *         pAnchor;

    sectionCount = apSector->Module.mpElf->e_shnum;
    for (secIx = 0; secIx < sectionCount; secIx++)
    {
        pSymSecHdr = &apSector->Module.mpSecHdr[secIx];

        if (pSymSecHdr->sh_type != SHT_SYMTAB)
            continue;

        K2_ASSERT(pSymSecHdr->sh_entsize >= sizeof(K2DLX_SYMTREE_NODE));

        entBytes = pSymSecHdr->sh_entsize;
        symCount = pSymSecHdr->sh_size / entBytes;
        symStrAddr = apSector->mSecAddr[pSymSecHdr->sh_link + sectionCount];
        pSymBase = (UINT8 *)apSector->mSecAddr[secIx];

        for (symIx = 1; symIx < symCount; symIx++)
        {
            pSym = (Elf32_Sym *)(pSymBase + (symIx * entBytes));
            
            if ((pSym->st_shndx == 0) ||
                (pSym->st_shndx >= sectionCount))
                continue;

            pTrgSecHdr = &apSector->Module.mpSecHdr[pSym->st_shndx];

            if ((pTrgSecHdr->sh_flags & DLX_SHF_TYPE_MASK) == DLX_SHF_TYPE_IMPORTS)
                continue;

            if ((pTrgSecHdr->sh_addralign < DlxSeg_Text) ||
                (pTrgSecHdr->sh_addralign > DlxSeg_Data))
                continue;

            pAnchor = &apSector->Module.SymTree[pTrgSecHdr->sh_addralign - DlxSeg_Text];

            symAddr = pSym->st_value;

            pSymTreeNode = (K2DLX_SYMTREE_NODE *)pSym;
            pSymTreeNode->mpSymName = (char *)symStrAddr + pSym->st_name;
            pSymTreeNode->TreeNode.mUserVal = symAddr;
            K2TREE_Insert(pAnchor, symAddr, &pSymTreeNode->TreeNode);
        }
    }

    for (secIx = 0;secIx < 3;secIx++)
        sRetargetTree(apSector, secIx);
}

K2STAT
iK2DLXSUPP_Link(
    DLX * apDlx
    )
{
    K2DLX_SECTOR *  pSector;
    K2STAT          status;

    if (apDlx->mRelocSectionCount == 0)
        return 0;

    pSector = K2_GET_CONTAINER(K2DLX_SECTOR, apDlx, Module);

    if (apDlx->mpInfo->mImportCount > 0)
    {
        status = sLocateImports(pSector);
        if (K2STAT_IS_ERROR(status))
            return status;
    }

    // change values at targets of relocations 
    // so that only offsets remain 
    status = sProcessReloc(pSector, FALSE);
    if (K2STAT_IS_ERROR(status))
        return status;

    // now update the symbol tables, section addresses to point to the new addresses
    status = sChangeAddresses(pSector);
    if (K2STAT_IS_ERROR(status))
        return status;

    // apply relocations using updated symbol tables
    status = sProcessReloc(pSector, TRUE);
    if (K2STAT_IS_ERROR(status))
        return status;

    if (pSector->Module.mFlags & K2DLXSUPP_FLAG_KEEP_SYMBOLS)
        sPopulateSymTrees(pSector);

    return K2STAT_OK;
}

