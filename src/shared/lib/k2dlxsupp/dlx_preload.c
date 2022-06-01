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

static void
sPrepPreload(
    DLX *           apDlx,
    UINT8 const *   apData,
    UINT32          aFileOffset,
    UINT32          aBaseAddr,
    UINT32 *        apRetEndAddr
)
{
    DLX_INFO *      pInfo;
    UINT32          crc;
    UINT8 *         pWork;
    UINT32          left;
    UINT32          segBytes;
    UINT32          linkAddr;
    UINT32          nameLen;

    pInfo = apDlx->mpInfo;
    pWork = (UINT8 *)pInfo;
    left = aFileOffset - apDlx->mHdrBytes;
    K2_ASSERT(left == pInfo->SegInfo[DlxSeg_Info].mFileBytes);

    // check the dlx info segment crc
    crc = pInfo->SegInfo[DlxSeg_Info].mCRC32;
    pInfo->SegInfo[DlxSeg_Info].mCRC32 = 0;
    if (K2CRC_Calc32(0, pInfo, left) != crc)
    {
        K2_ASSERT(0);
        return;
    }
    // dlx info segment is good
    pInfo->SegInfo[DlxSeg_Info].mCRC32 = crc;

    // set up apDlx->SegAlloc
    K2_ASSERT(aBaseAddr == pInfo->SegInfo[DlxSeg_Text].mLinkAddr);
    apDlx->SegAlloc.Segment[DlxSeg_Text].mDataAddr = apDlx->SegAlloc.Segment[DlxSeg_Text].mLinkAddr = aBaseAddr;
    segBytes = (pInfo->SegInfo[DlxSeg_Text].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK;
    linkAddr = pInfo->SegInfo[DlxSeg_Text].mLinkAddr + segBytes;
    K2_ASSERT(linkAddr == pInfo->SegInfo[DlxSeg_Read].mLinkAddr);

    apDlx->SegAlloc.Segment[DlxSeg_Read].mDataAddr = apDlx->SegAlloc.Segment[DlxSeg_Read].mLinkAddr = linkAddr;
    segBytes = (pInfo->SegInfo[DlxSeg_Read].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK;
    linkAddr = pInfo->SegInfo[DlxSeg_Read].mLinkAddr + segBytes;
    K2_ASSERT(linkAddr == pInfo->SegInfo[DlxSeg_Data].mLinkAddr);

    apDlx->SegAlloc.Segment[DlxSeg_Data].mDataAddr = apDlx->SegAlloc.Segment[DlxSeg_Data].mLinkAddr = linkAddr;
    segBytes = (pInfo->SegInfo[DlxSeg_Data].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK;
    linkAddr = pInfo->SegInfo[DlxSeg_Data].mLinkAddr + segBytes;
    K2_ASSERT(linkAddr == pInfo->SegInfo[DlxSeg_Sym].mLinkAddr);

    apDlx->SegAlloc.Segment[DlxSeg_Sym].mDataAddr = apDlx->SegAlloc.Segment[DlxSeg_Sym].mLinkAddr = linkAddr;
    segBytes = (pInfo->SegInfo[DlxSeg_Sym].mMemActualBytes + (K2_VA_MEMPAGE_BYTES - 1)) & K2_VA_PAGEFRAME_MASK;
    *apRetEndAddr = pInfo->SegInfo[DlxSeg_Sym].mLinkAddr + segBytes;

    // update exports DATA addresses now 
    if (apDlx->mpExpCodeDataAddr != ((DLX_EXPORTS_SECTION *)-1))
        apDlx->mpExpCodeDataAddr = (DLX_EXPORTS_SECTION *)(((UINT8 *)apDlx->mpExpCodeDataAddr) + apDlx->SegAlloc.Segment[DlxSeg_Read].mDataAddr);
    else
        apDlx->mpExpCodeDataAddr = NULL;
    if (apDlx->mpExpReadDataAddr != ((DLX_EXPORTS_SECTION *)-1))
        apDlx->mpExpReadDataAddr = (DLX_EXPORTS_SECTION *)(((UINT8 *)apDlx->mpExpReadDataAddr) + apDlx->SegAlloc.Segment[DlxSeg_Read].mDataAddr);
    else
        apDlx->mpExpReadDataAddr = NULL;
    if (apDlx->mpExpDataDataAddr != ((DLX_EXPORTS_SECTION *)-1))
        apDlx->mpExpDataDataAddr = (DLX_EXPORTS_SECTION *)(((UINT8 *)apDlx->mpExpDataDataAddr) + apDlx->SegAlloc.Segment[DlxSeg_Read].mDataAddr);
    else
        apDlx->mpExpDataDataAddr = NULL;

    pWork += sizeof(DLX_INFO) - sizeof(UINT32);
    left -= sizeof(DLX_INFO) - sizeof(UINT32);
    apDlx->mpIntName = pInfo->mFileName;
    apDlx->mIntNameLen = nameLen = K2ASC_Len(pInfo->mFileName);
    nameLen = K2_ROUNDUP(nameLen + 1, 4);
    apDlx->mIntNameFieldLen = nameLen;
}

static
void
sPopulateSymTrees(
    DLX*    apDlx
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

    sectionCount = apDlx->mpElf->e_shnum;
    for (secIx = 0; secIx < sectionCount; secIx++)
    {
        pSymSecHdr = &apDlx->mpSecHdr[secIx];

        if (pSymSecHdr->sh_type != SHT_SYMTAB)
            continue;

        K2_ASSERT(pSymSecHdr->sh_entsize >= sizeof(K2DLX_SYMTREE_NODE));

        entBytes = pSymSecHdr->sh_entsize;
        symCount = pSymSecHdr->sh_size / entBytes;
        symStrAddr = apDlx->mpSecHdr[pSymSecHdr->sh_link].sh_addr;
        pSymBase = (UINT8 *)pSymSecHdr->sh_addr;
        for (symIx = 1; symIx < symCount; symIx++)
        {
            pSym = (Elf32_Sym *)(pSymBase + (symIx * entBytes));
            
            if ((pSym->st_shndx == 0) ||
                (pSym->st_shndx >= sectionCount))
                continue;

            pTrgSecHdr = &apDlx->mpSecHdr[pSym->st_shndx];

            if ((pTrgSecHdr->sh_flags & DLX_SHF_TYPE_MASK) == DLX_SHF_TYPE_IMPORTS)
                continue;

            if ((pTrgSecHdr->sh_addralign < DlxSeg_Text) ||
                (pTrgSecHdr->sh_addralign > DlxSeg_Data))
                continue;

            pAnchor = &apDlx->SymTree[pTrgSecHdr->sh_addralign - DlxSeg_Text];

            symAddr = pSym->st_value;

            pSymTreeNode = (K2DLX_SYMTREE_NODE *)pSym;
            pSymTreeNode->mpSymName = (char *)symStrAddr + pSym->st_name;
            pSymTreeNode->TreeNode.mUserVal = symAddr;
            K2TREE_Insert(pAnchor, symAddr, &pSymTreeNode->TreeNode);
        }
    }
}

static void
sExecPreload(
    DLX *           apDlx,
    UINT8 const *   apData,
    UINT32          aFileOffset
)
{
    Elf32_Ehdr *        pElf;
    Elf32_Shdr *        pSecHdr;
    UINT32              segIx;
    UINT32              secIx;
    UINT32              count;
    DLX_INFO *          pInfo;
    K2DLX_SECTOR *      pSector;
    UINT32              startAddr;
    UINT32              endAddr;
    UINT32              linkAddr;
    UINT32              secSegOffset;

    pElf = apDlx->mpElf;
    pSecHdr = apDlx->mpSecHdr;
    pInfo = apDlx->mpInfo;
    pSector = K2_GET_CONTAINER(K2DLX_SECTOR, apDlx, Module);
    K2MEM_Zero(&pSector->mSecAddr[3], (pElf->e_shnum - 3) * sizeof(UINT32));
    K2MEM_Zero(&pSector->mSecAddr[3 + pElf->e_shnum], (pElf->e_shnum - 3) * sizeof(UINT32));

    secIx = 3;

    for (segIx = DlxSeg_Text; segIx < DlxSeg_Sym; segIx++)
    {
        count = K2_ROUNDUP(pInfo->SegInfo[segIx].mFileBytes, DLX_SECTOR_BYTES);
        if (count > 0)
        {
            startAddr = pInfo->SegInfo[segIx].mLinkAddr;
            endAddr = startAddr + pInfo->SegInfo[segIx].mMemActualBytes;
            while ((pSecHdr[secIx].sh_addr >= startAddr) &&
                    (pSecHdr[secIx].sh_addr < endAddr))
            {
                // addralign is segment index for section
                pSecHdr[secIx].sh_addralign = segIx;
                secSegOffset = pSecHdr[secIx].sh_addr - startAddr;
                pSector->mSecAddr[secIx] = secSegOffset + apDlx->SegAlloc.Segment[segIx].mDataAddr;
                linkAddr = apDlx->SegAlloc.Segment[segIx].mLinkAddr;
                if (linkAddr != 0)
                    pSector->mSecAddr[secIx + apDlx->mpElf->e_shnum] = secSegOffset + linkAddr;
                secIx++;
            }
        }
    }

    //
    // populate symbol trees (text, read, data)
    //
    sPopulateSymTrees(apDlx);

    K2LIST_Remove(&gpK2DLXSUPP_Vars->AcqList, &apDlx->ListLink);
    K2LIST_AddAtTail(&gpK2DLXSUPP_Vars->LoadedList, &apDlx->ListLink);
    apDlx->mFlags |= K2DLXSUPP_FLAG_FULLY_LOADED | K2DLXSUPP_FLAG_PERMANENT;
}

void
iK2DLXSUPP_Preload(
    K2DLXSUPP_PRELOAD * apPreload
)
{
    K2STAT                  status;
    K2DLX_PAGE *            pPage;
    DLX *                   pDlx;
    UINT32                  fileOffset;
    Elf32_Ehdr *            pHdr;
    UINT32                  dlxInfoEnd;
    UINT32                  secIx;
    BOOL                    hasExports;
    DLX_INFO *              pInfo;
    UINT32                  readSegStart;
    UINT32                  readSegEnd;
    UINT32                  chkAddr;
    UINT8 const *           pData;

    pPage = (K2DLX_PAGE *)apPreload->mpDlxPage;
    K2MEM_Zero(pPage, sizeof(K2DLX_PAGE));

    pDlx = &pPage->ModuleSector.Module;
    pDlx->mSectorCount = (apPreload->mDlxFileSizeBytes + (DLX_SECTOR_BYTES-1)) / DLX_SECTOR_BYTES;
    K2_ASSERT(pDlx->mSectorCount > 1);
    pDlx->mFlags = gpK2DLXSUPP_Vars->mKeepSym ? K2DLXSUPP_FLAG_KEEP_SYMBOLS : 0;
    pDlx->mLinkAddr = (UINT32)apPreload->mpDlxPage;

    for (secIx = 0;secIx < 3;secIx++)
        K2TREE_Init(&pDlx->SymTree[secIx], iK2DLXSUPP_CompareUINT32);

    pData = (UINT8 const *)apPreload->mpDlxFileData;
    K2MEM_Copy(pPage->mHdrSectorsBuffer, pData, DLX_SECTOR_BYTES);
    pData += DLX_SECTOR_BYTES;
    fileOffset = DLX_SECTOR_BYTES;

    pHdr = pDlx->mpElf = (Elf32_Ehdr *)pPage->mHdrSectorsBuffer;

    status = K2ELF32_ValidateHeader(
        (UINT8 *)pHdr, 
        sizeof(Elf32_Ehdr),
        pDlx->mSectorCount * DLX_SECTOR_BYTES);
    K2_ASSERT(!K2STAT_IS_ERROR(status));

    if ((pHdr->e_ehsize != sizeof(Elf32_Ehdr)) ||
        (pHdr->e_shoff != sizeof(Elf32_Ehdr)) ||
        (pHdr->e_type != DLX_ET_DLX) ||
        (pHdr->e_ident[EI_OSABI] != DLX_ELFOSABI_K2) ||
        (pHdr->e_ident[EI_ABIVERSION] != DLX_ELFOSABIVER_DLX) ||
        (pHdr->e_shstrndx != 2))
    {
        K2_ASSERT(0);
        return;
    }

    // two copies of section address - one is data address and one is link address
    if ((pHdr->e_shnum * 2) > K2DLX_MAX_SECTION_COUNT)
    {
        // this should never happen
        K2_ASSERT(0);
        return;
    }

    pDlx->mpSecHdr = (Elf32_Shdr *)(((UINT8 *)pHdr) + sizeof(Elf32_Ehdr));

    // at LEAST the first two section headers came in the first disk sector
    pDlx->mHdrBytes = sizeof(Elf32_Ehdr) + (sizeof(Elf32_Shdr) * pHdr->e_shnum);

    if ((pDlx->mpSecHdr[1].sh_offset != pDlx->mHdrBytes) ||
        (pDlx->mpSecHdr[1].sh_size < sizeof(DLX_INFO)) ||
        ((pDlx->mpSecHdr[1].sh_flags & DLX_SHF_TYPE_MASK) != DLX_SHF_TYPE_DLXINFO))
    {
        K2_ASSERT(0);
        return;
    }

    if ((pDlx->mpSecHdr[2].sh_type != SHT_STRTAB) ||
        (pDlx->mpSecHdr[2].sh_offset != (pDlx->mpSecHdr[1].sh_offset + pDlx->mpSecHdr[1].sh_size)))
    {
        K2_ASSERT(0);
        return;
    }

    dlxInfoEnd = pDlx->mpSecHdr[2].sh_offset + pDlx->mpSecHdr[2].sh_size;
    dlxInfoEnd = K2_ROUNDUP(dlxInfoEnd, DLX_SECTOR_BYTES);

    if (dlxInfoEnd > K2DLX_PAGE_HDRSECTORS_BYTES)
    {
        // too big!  DLX info + file headers are more than 7 sectors long (3.5kB)
        K2_ASSERT(0);
        return;
    }

    if (dlxInfoEnd != pDlx->mpSecHdr[3].sh_offset)
    {
        // section data is not in correct order
        K2_ASSERT(0);
        return;
    }

    if (dlxInfoEnd > fileOffset)
    {
        K2MEM_Copy(&pPage->mHdrSectorsBuffer[fileOffset], pData, dlxInfoEnd - fileOffset);
        pData += dlxInfoEnd - fileOffset;
        fileOffset = dlxInfoEnd;
    }
    K2_ASSERT((fileOffset & DLX_SECTOROFFSET_MASK) == 0);
    pInfo = pDlx->mpInfo = (DLX_INFO *)(((UINT8 *)pHdr) + pDlx->mpSecHdr[1].sh_offset);
    K2_ASSERT(0 == pDlx->mpInfo->mImportCount);

    if (pInfo->mElfCRC != K2CRC_Calc32(0, pHdr, pDlx->mpSecHdr[1].sh_offset))
    {
        K2_ASSERT(0);
        return;
    }

    // data addresses
    pPage->ModuleSector.mSecAddr[1] = (UINT32)pInfo;
    pPage->ModuleSector.mSecAddr[2] = (((UINT32)pHdr) + pDlx->mpSecHdr[2].sh_offset);

    // link addresses are same as data addresses
    pPage->ModuleSector.mSecAddr[1 + pHdr->e_shnum] = pPage->ModuleSector.mSecAddr[1];
    pPage->ModuleSector.mSecAddr[2 + pHdr->e_shnum] = pPage->ModuleSector.mSecAddr[2];

    // addralign is used as a segment index
    pDlx->mpSecHdr[1].sh_addralign = DlxSeg_Info;
    pDlx->mpSecHdr[2].sh_addralign = DlxSeg_Info;

    for (secIx = 3; secIx < pHdr->e_shnum;secIx++)
    {
        if (pDlx->mpSecHdr[secIx].sh_type == SHT_SYMTAB)
        {
            chkAddr = pDlx->mpSecHdr[secIx].sh_entsize;
            if ((chkAddr < sizeof(K2DLX_SYMTREE_NODE)) || (chkAddr & 3))
            {
                K2_ASSERT(0);
                return;
            }
        }
        else if (pDlx->mpSecHdr[secIx].sh_type == SHT_REL)
            pDlx->mRelocSectionCount++;
    }

    secIx = pDlx->mRelocSectionCount;

    //
    // validate exports are in the right section and
    // create the DATA addresses of exports as just offsets into
    // the read segment
    //
    readSegStart = pInfo->SegInfo[DlxSeg_Read].mLinkAddr;
    readSegEnd = readSegStart + pInfo->SegInfo[DlxSeg_Read].mMemActualBytes;
    hasExports = FALSE;

    chkAddr = (UINT32)pInfo->mpExpCode;
    if (chkAddr != 0)
    {
        // must be code relocs
        if (secIx == 0)
        {
            K2_ASSERT(0);
            return;
        }
        if ((chkAddr < readSegStart) || (chkAddr >= readSegEnd))
        {
            K2_ASSERT(0);
            return;
        }
        pDlx->mpExpCodeDataAddr = (DLX_EXPORTS_SECTION *)(chkAddr - readSegStart);
        hasExports = TRUE;
        secIx--;
    }
    else
        pDlx->mpExpCodeDataAddr = (DLX_EXPORTS_SECTION *)-1;

    chkAddr = (UINT32)pInfo->mpExpRead;
    if (chkAddr != 0)
    {
        // must be read relocs
        if (secIx == 0)
        {
            K2_ASSERT(0);
            return;
        }
        if ((chkAddr < readSegStart) || (chkAddr >= readSegEnd))
        {
            K2_ASSERT(0);
            return;
        }
        pDlx->mpExpReadDataAddr = (DLX_EXPORTS_SECTION *)(chkAddr - readSegStart);
        hasExports = TRUE;
        secIx--;
    }
    else
        pDlx->mpExpReadDataAddr = (DLX_EXPORTS_SECTION *)-1;

    chkAddr = (UINT32)pInfo->mpExpData;
    if (chkAddr != 0)
    {
        // must be data relocs
        if (secIx == 0)
        {
            K2_ASSERT(0);
            return;
        }
        if ((chkAddr < readSegStart) || (chkAddr >= readSegEnd))
        {
            K2_ASSERT(0);
            return;
        }
        pDlx->mpExpDataDataAddr = (DLX_EXPORTS_SECTION *)(chkAddr - readSegStart);
        hasExports = TRUE;
        secIx--;
    }
    else
        pDlx->mpExpDataDataAddr = (DLX_EXPORTS_SECTION *)-1;

    if (hasExports)
    {
        // must be dlx info relocs
        if (secIx == 0)
        {
            K2_ASSERT(0);
            return;
        }
    }

    // all dlx info should be available now to let us get resources
    // and load dependencies
    sPrepPreload(pDlx, pData, fileOffset, apPreload->mDlxBase, &apPreload->mDlxMemEndOut);

    K2LIST_AddAtTail(&gpK2DLXSUPP_Vars->AcqList, &pDlx->ListLink);

    sExecPreload(pDlx, pData, fileOffset);

    apPreload->mpListAnchorOut = &gpK2DLXSUPP_Vars->LoadedList;
}
