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

static
K2STAT
sAcquire(
    void *              apAcqContext,
    char const *        apSpec,
    char const *        apName,
    UINT32              aNameLen,
    K2_GUID128 const *  apMatchId,
    DLX **              appRetDlx
    );

static
K2STAT
sPrepModule(
    void *              apAcqContext,
    DLX *               apDlx,
    K2_GUID128 const *  apMatchId,
    UINT32              aFileOffset
    )
{
    DLX_INFO32 *    pInfo;
    UINT32          crc;
    UINT8 *         pWork;
    UINT32          left;
    UINT32          nameLen;
    DLX_IMPORT *    pImport;
    K2STAT          status;
    DLX *           pSubModule;
    UINT32          impIx;

    pInfo = apDlx->mpInfo;
    pWork = (UINT8 *)pInfo;
    left = aFileOffset - apDlx->mHdrBytes;

    // size of dlx info segment must be equal to data range from
    // end of file+section headers to current file offset
    if (left != pInfo->SegInfo[DlxSeg_Info].mFileBytes)
        return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);

    // it a required matching id was provided we must test it
    if (apMatchId != NULL)
    {
        if (0 != K2MEM_Compare(apMatchId, &pInfo->ID, sizeof(K2_GUID128)))
            return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_ID_MISMATCH);
    }

    // check the dlx info segment crc
    crc = pInfo->SegInfo[DlxSeg_Info].mCRC32;
    pInfo->SegInfo[DlxSeg_Info].mCRC32 = 0;
    if (K2CRC_Calc32(0, pInfo, left) != crc)
        return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_CRC_WRONG);
    // dlx info segment is good
    pInfo->SegInfo[DlxSeg_Info].mCRC32 = crc;

    // allocate segments in memory
    if (gpK2DLXSUPP_Vars->Host.Prepare == NULL)
        return K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_NOT_IMPL);
    status = gpK2DLXSUPP_Vars->Host.Prepare(
        apAcqContext,
        apDlx->mHostFile, 
        pInfo, left, 
        (apDlx->mFlags & K2DLXSUPP_FLAG_KEEP_SYMBOLS) ? TRUE : FALSE,
        &apDlx->SegAlloc);
    if (K2STAT_IS_ERROR(status))
        return K2DLXSUPP_ERRORPOINT(status);

    // update exports DATA addresses now (not loaded yet but who cares)
    // the ones in the DLX info will be updated to their LINK addresses by the relocation code
    if (apDlx->mpExpCodeDataAddr != ((DLX_EXPORTS32_SECTION *)-1))
        apDlx->mpExpCodeDataAddr = (DLX_EXPORTS32_SECTION *)(((UINT8 *)apDlx->mpExpCodeDataAddr) + apDlx->SegAlloc.Segment[DlxSeg_Read].mDataAddr);
    else
        apDlx->mpExpCodeDataAddr = NULL;
    if (apDlx->mpExpReadDataAddr != ((DLX_EXPORTS32_SECTION *)-1))
        apDlx->mpExpReadDataAddr = (DLX_EXPORTS32_SECTION *)(((UINT8 *)apDlx->mpExpReadDataAddr) + apDlx->SegAlloc.Segment[DlxSeg_Read].mDataAddr);
    else
        apDlx->mpExpReadDataAddr = NULL;
    if (apDlx->mpExpDataDataAddr != ((DLX_EXPORTS32_SECTION *)-1))
        apDlx->mpExpDataDataAddr = (DLX_EXPORTS32_SECTION *)(((UINT8 *)apDlx->mpExpDataDataAddr) + apDlx->SegAlloc.Segment[DlxSeg_Read].mDataAddr);
    else
        apDlx->mpExpDataDataAddr = NULL;

    // move on to imports
    pWork += sizeof(DLX_INFO32) - sizeof(UINT32);
    left -= sizeof(DLX_INFO32) - sizeof(UINT32);
    apDlx->mpIntName = pInfo->mFileName;
    apDlx->mIntNameLen = nameLen = K2ASC_Len(pInfo->mFileName);
    nameLen = K2_ROUNDUP(nameLen + 1, 4);
    apDlx->mIntNameFieldLen = nameLen;
    pWork += nameLen;
    left -= nameLen;

    // acquire all imports
    for (impIx = 0; impIx < pInfo->mImportCount; impIx++)
    {
        pImport = (DLX_IMPORT *)pWork;

        if ((pImport->mSizeBytes < sizeof(DLX_IMPORT)) || (pImport->mSizeBytes > left))
            return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);

        status = sAcquire(
            apAcqContext,
            pImport->mFileName,
            pImport->mFileName,
            K2ASC_Len(pImport->mFileName),
            &pImport->ID,
            &pSubModule);

        if (K2STAT_IS_ERROR(status))
        {
            // walk back the acquires
            iK2DLXSUPP_ReleaseImports(apDlx, impIx);
            return status;
        }

        pImport->mReserved = (UINT32)pSubModule;

        pWork += pImport->mSizeBytes;
        left -= pImport->mSizeBytes;
    }

    apDlx->mRefs = 1;

    return K2STAT_OK;
}

static
K2STAT
sPrep(
    void *              apAcqContext,
    char const *        apSpec,
    char const *        apName,
    UINT32              aNameLen,
    K2_GUID128 const *  apMatchId,
    DLX **              appRetDlx
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
    DLX_INFO32 *            pInfo;
    UINT32                  readSegStart;
    UINT32                  readSegEnd;
    UINT32                  chkAddr;
    K2DLXSUPP_OPENRESULT    openResult;

    if (gpK2DLXSUPP_Vars->Host.Open == NULL)
        return K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_NOT_IMPL);
    status = gpK2DLXSUPP_Vars->Host.Open(apAcqContext, apSpec, apName, aNameLen, &openResult);
    if (K2STAT_IS_ERROR(status))
        return K2DLXSUPP_ERRORPOINT(status);

    pPage = (K2DLX_PAGE *)openResult.mModulePageDataAddr;
    pDlx = &pPage->ModuleSector.Module;
    pDlx->mHostFile = openResult.mHostFile;
    pDlx->mSectorCount = openResult.mFileSectorCount;
    pDlx->mFlags = gpK2DLXSUPP_Vars->mKeepSym ? K2DLXSUPP_FLAG_KEEP_SYMBOLS : 0;
    pDlx->mLinkAddr = openResult.mModulePageLinkAddr;

    for (secIx = 0;secIx < 3;secIx++)
        K2TREE_Init(&pDlx->SymTree[secIx], iK2DLXSUPP_CompareUINT32);

    do
    {
        if (pDlx->mSectorCount == 1)
        {
            // not possible to be this small
            status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            break;
        }

        if (gpK2DLXSUPP_Vars->Host.ReadSectors == NULL)
        {
            status = K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_NOT_IMPL);
            break;
        }
        status = gpK2DLXSUPP_Vars->Host.ReadSectors(apAcqContext, pDlx->mHostFile, pPage->mHdrSectorsBuffer, 1);
        if (K2STAT_IS_ERROR(status))
        {
            status = K2DLXSUPP_ERRORPOINT(status);
            break;
        }
        fileOffset = DLX_SECTOR_BYTES;

        pHdr = pDlx->mpElf = (Elf32_Ehdr *)pPage->mHdrSectorsBuffer;

        status = K2ELF32_ValidateHeader(
            (UINT8 *)pHdr, 
            sizeof(Elf32_Ehdr),
            pDlx->mSectorCount * DLX_SECTOR_BYTES);
        if (K2STAT_IS_ERROR(status))
        {
            status = K2DLXSUPP_ERRORPOINT(status);
            break;
        }

        if ((pHdr->e_ehsize != sizeof(Elf32_Ehdr)) ||
            (pHdr->e_shoff != sizeof(Elf32_Ehdr)) ||
            (pHdr->e_type != DLX_ET_DLX) ||
            (pHdr->e_ident[EI_OSABI] != DLX_ELFOSABI_K2) ||
            (pHdr->e_ident[EI_ABIVERSION] != DLX_ELFOSABIVER_DLX) ||
            (pHdr->e_shstrndx != 2))
        {
            status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            break;
        }

        // two copies of section address - one is data address and one is link address
        if ((pHdr->e_shnum * 2) > K2DLX_MAX_SECTION_COUNT)
        {
            // this should never happen
            status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            break;
        }

        pDlx->mpSecHdr = (Elf32_Shdr *)(((UINT8 *)pHdr) + sizeof(Elf32_Ehdr));

        // at LEAST the first two section headers came in the first disk sector
        pDlx->mHdrBytes = sizeof(Elf32_Ehdr) + (sizeof(Elf32_Shdr) * pHdr->e_shnum);

        if ((pDlx->mpSecHdr[1].sh_offset != pDlx->mHdrBytes) ||
            (pDlx->mpSecHdr[1].sh_size < sizeof(DLX_INFO32)) ||
            ((pDlx->mpSecHdr[1].sh_flags & DLX_SHF_TYPE_MASK) != DLX_SHF_TYPE_DLXINFO))
        {
            status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            break;
        }

        if ((pDlx->mpSecHdr[2].sh_type != SHT_STRTAB) ||
            (pDlx->mpSecHdr[2].sh_offset != (pDlx->mpSecHdr[1].sh_offset + pDlx->mpSecHdr[1].sh_size)))
        {
            status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            break;
        }

        dlxInfoEnd = pDlx->mpSecHdr[2].sh_offset + pDlx->mpSecHdr[2].sh_size;
        dlxInfoEnd = K2_ROUNDUP(dlxInfoEnd, DLX_SECTOR_BYTES);

        if (dlxInfoEnd > K2DLX_PAGE_HDRSECTORS_BYTES)
        {
            // too big!  DLX info + file headers are more than 7 sectors long (3.5kB)
            status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            break;
        }

        if (dlxInfoEnd != pDlx->mpSecHdr[3].sh_offset)
        {
            // section data is not in correct order
            status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            break;
        }

        if (dlxInfoEnd > fileOffset)
        {
            if (gpK2DLXSUPP_Vars->Host.ReadSectors == NULL)
            {
                status = K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_NOT_IMPL);
                break;
            }
            status = gpK2DLXSUPP_Vars->Host.ReadSectors(
                apAcqContext,
                pDlx->mHostFile,
                &pPage->mHdrSectorsBuffer[fileOffset],
                (dlxInfoEnd - fileOffset) / DLX_SECTOR_BYTES);
            if (K2STAT_IS_ERROR(status))
            {
                status = K2DLXSUPP_ERRORPOINT(status);
                break;
            }
            fileOffset = dlxInfoEnd;
        }
        K2_ASSERT((fileOffset & DLX_SECTOROFFSET_MASK) == 0);
        pDlx->mCurSector = fileOffset / DLX_SECTOR_BYTES;
        pInfo = pDlx->mpInfo = (DLX_INFO32 *)(((UINT8 *)pHdr) + pDlx->mpSecHdr[1].sh_offset);

        if (pInfo->mElfCRC != K2CRC_Calc32(0, pHdr, pDlx->mpSecHdr[1].sh_offset))
        {
            status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_CRC_WRONG);
            break;
        }

        // data addresses
        pPage->ModuleSector.mSecAddr[1] = (UINT32)pInfo;
        pPage->ModuleSector.mSecAddr[2] = (((UINT32)pHdr) + pDlx->mpSecHdr[2].sh_offset);
        // link addresses are same as data addresses for these two right now
        pPage->ModuleSector.mSecAddr[1 + pHdr->e_shnum] = pPage->ModuleSector.mSecAddr[1];
        pPage->ModuleSector.mSecAddr[2 + pHdr->e_shnum] = pPage->ModuleSector.mSecAddr[2];
        // addralign is used as a segment index
        pDlx->mpSecHdr[1].sh_addralign = DlxSeg_Info;
        pDlx->mpSecHdr[2].sh_addralign = DlxSeg_Info;

        for (secIx = 3; secIx < pHdr->e_shnum;secIx++)
        {
            if ((pDlx->mpSecHdr[secIx].sh_flags & DLX_SHF_TYPE_MASK) == DLX_SHF_TYPE_IMPORTS)
            {
                if ((pDlx->mpSecHdr[secIx].sh_link == 0) ||
                    (pDlx->mpSecHdr[secIx].sh_link > pInfo->mImportCount))
                {
                    status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
                    break;
                }
            }
            else if (pDlx->mpSecHdr[secIx].sh_type == SHT_SYMTAB)
            {
                chkAddr = pDlx->mpSecHdr[secIx].sh_entsize;
                if ((chkAddr < sizeof(K2DLX_SYMTREE_NODE)) || (chkAddr & 3))
                {
                    status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
                    break;
                }
            }
            else if (pDlx->mpSecHdr[secIx].sh_type == SHT_REL)
                pDlx->mRelocSectionCount++;
        }
        if (secIx < pHdr->e_shnum)
            break;

        secIx = pDlx->mRelocSectionCount;

        //
        // validate exports are in the right section and
        // create the DATA addresses of exports as just offsets into
        // the read segment (updated when read segment actually populated)
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
                status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
                break;
            }
            if ((chkAddr < readSegStart) || (chkAddr >= readSegEnd))
                return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            pDlx->mpExpCodeDataAddr = (DLX_EXPORTS32_SECTION *)(chkAddr - readSegStart);
            hasExports = TRUE;
            secIx--;
        }
        else
            pDlx->mpExpCodeDataAddr = (DLX_EXPORTS32_SECTION *)-1;
        chkAddr = (UINT32)pInfo->mpExpRead;
        if (chkAddr != 0)
        {
            // must be read relocs
            if (secIx == 0)
            {
                status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
                break;
            }
            if ((chkAddr < readSegStart) || (chkAddr >= readSegEnd))
                return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            pDlx->mpExpReadDataAddr = (DLX_EXPORTS32_SECTION *)(chkAddr - readSegStart);
            hasExports = TRUE;
            secIx--;
        }
        else
            pDlx->mpExpReadDataAddr = (DLX_EXPORTS32_SECTION *)-1;
        chkAddr = (UINT32)pInfo->mpExpData;
        if (chkAddr != 0)
        {
            // must be data relocs
            if (secIx == 0)
            {
                status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
                break;
            }
            if ((chkAddr < readSegStart) || (chkAddr >= readSegEnd))
                return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
            pDlx->mpExpDataDataAddr = (DLX_EXPORTS32_SECTION *)(chkAddr - readSegStart);
            hasExports = TRUE;
            secIx--;
        }
        else
            pDlx->mpExpDataDataAddr = (DLX_EXPORTS32_SECTION *)-1;

        if (hasExports)
        {
            // must be dlx info relocs
            if (secIx == 0)
            {
                status = K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
                break;
            }
        }

        // all dlx info should be available now to let us get resources
        // and load dependencies
        status = sPrepModule(apAcqContext, pDlx, apMatchId, fileOffset);
        if (!K2STAT_IS_ERROR(status))
            K2LIST_AddAtTail(&gpK2DLXSUPP_Vars->AcqList, &pDlx->ListLink);

    } while (0);

    if (K2STAT_IS_ERROR(status))
    {
        if (gpK2DLXSUPP_Vars->Host.Purge != NULL)
            gpK2DLXSUPP_Vars->Host.Purge(openResult.mHostFile);
        return status;
    }

    *appRetDlx = pDlx;

    return status;
}

static
DLX *
sFindModuleOnList(
    K2LIST_ANCHOR *     apAnchor,
    char const *        apName,
    UINT32              aNameLen,
    K2_GUID128 const *  apMatchId
    )
{
    DLX *           pDlx;
    K2LIST_LINK *   pLink;

    pDlx = NULL;
    pLink = apAnchor->mpHead;
    while (pLink != NULL)
    {
        pDlx = K2_GET_CONTAINER(DLX, pLink, ListLink);
        if ((apMatchId == NULL) ||
            (0 == K2MEM_Compare(apMatchId, &pDlx->mpInfo->ID, sizeof(K2_GUID128))))
        {
            if (pDlx->mIntNameLen == aNameLen)
            {
                if (!K2ASC_CompInsLen(apName, pDlx->mpIntName, aNameLen))
                    break;
            }
        }
        pLink = pLink->mpNext;
    }
    if (pLink == NULL)
        return NULL;
    return pDlx;
}

static
K2STAT
sLoadModule(
    void *  apAcqContext,
    DLX *   apDlx
    )
{
    DLX_INFO32 *    pInfo;
    UINT8 *         pWork;
    UINT32          impIx;
    DLX_IMPORT *    pImport;
    DLX *           pSubModule;
    K2STAT          status;

    K2_ASSERT(!(apDlx->mFlags & K2DLXSUPP_FLAG_FULLY_LOADED));

    pInfo = apDlx->mpInfo;

    if (pInfo->mImportCount > 0)
    {
        pWork = (UINT8 *)pInfo;
        pWork += sizeof(DLX_INFO32) - 4 + apDlx->mIntNameFieldLen;
        for (impIx = 0; impIx < pInfo->mImportCount; impIx++)
        {
            pImport = (DLX_IMPORT *)pWork;
            pSubModule = (DLX *)pImport->mReserved;
            if (!(pSubModule->mFlags & K2DLXSUPP_FLAG_FULLY_LOADED))
            {
                status = sLoadModule(apAcqContext, pSubModule);
                if (K2STAT_IS_ERROR(status))
                    return status;
            }
            pWork += pImport->mSizeBytes;
        }
    }

    status = iK2DLXSUPP_LoadSegments(apAcqContext, apDlx);
    if (K2STAT_IS_ERROR(status))
        return status;

    status = iK2DLXSUPP_Link(apDlx);
    if (K2STAT_IS_ERROR(status))
        return status;

    if (gpK2DLXSUPP_Vars->Host.Finalize == NULL)
        return K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_NOT_IMPL);
    status = gpK2DLXSUPP_Vars->Host.Finalize(apAcqContext, apDlx->mHostFile, &apDlx->SegAlloc);
    if (K2STAT_IS_ERROR(status))
        return K2DLXSUPP_ERRORPOINT(status);

    iK2DLXSUPP_Cleanup(apDlx);

    status = iK2DLXSUPP_DoCallback(apAcqContext, apDlx, TRUE);
    if (!K2STAT_IS_ERROR(status))
    {
        K2LIST_Remove(&gpK2DLXSUPP_Vars->AcqList, &apDlx->ListLink);
        K2LIST_AddAtTail(&gpK2DLXSUPP_Vars->LoadedList, &apDlx->ListLink);
        apDlx->mFlags |= K2DLXSUPP_FLAG_FULLY_LOADED;
    }

    return status;
}

static
K2STAT
sExecLoads(
    void *apAcqContext
    )
{
    K2STAT status;
    do
    {
        status = sLoadModule(apAcqContext, K2_GET_CONTAINER(DLX, gpK2DLXSUPP_Vars->AcqList.mpHead, ListLink));
        if (K2STAT_IS_ERROR(status))
            break;
    } while (gpK2DLXSUPP_Vars->AcqList.mpHead != NULL);
    return status;
}

static
K2STAT
sAcquire(
    void *              apAcqContext,
    char const *        apSpec,
    char const *        apName,
    UINT32              aNameLen,
    K2_GUID128 const *  apMatchId,
    DLX **              appRetDlx
    )
{
    DLX *   pDlx;
    K2STAT  status;

    //
    // apMatchId will only be NULL for top-level load
    // 
    pDlx = sFindModuleOnList(&gpK2DLXSUPP_Vars->LoadedList, apName, aNameLen, apMatchId);
    if (pDlx != NULL)
    {
        //
        // already fully loaded dlx
        //
        if (0 == (pDlx->mFlags & K2DLXSUPP_FLAG_PERMANENT))
        {
            if (apMatchId == NULL)
            {
                //
                // top level load of module already loaded
                //
                if (gpK2DLXSUPP_Vars->Host.AcqAlreadyLoaded != NULL)
                    gpK2DLXSUPP_Vars->Host.AcqAlreadyLoaded(apAcqContext, pDlx->mHostFile);
            }
            else
            {
                //
                // not a top level load, but module already loaded.  just increase its import reference
                //
                if (gpK2DLXSUPP_Vars->Host.RefChange != NULL)
                    gpK2DLXSUPP_Vars->Host.RefChange(pDlx->mHostFile, pDlx, 1);
            }
        }
        pDlx->mRefs++;
        *appRetDlx = pDlx;
        return K2STAT_OK;
    }

    if (apMatchId != NULL)
    {
        //
        // check for partially loaded dlx
        //
        pDlx = sFindModuleOnList(&gpK2DLXSUPP_Vars->AcqList, apName, aNameLen, apMatchId);
        if (pDlx != NULL)
        {
            if (0 == (pDlx->mFlags & K2DLXSUPP_FLAG_PERMANENT))
            {
                if (gpK2DLXSUPP_Vars->Host.RefChange != NULL)
                    gpK2DLXSUPP_Vars->Host.RefChange(pDlx->mHostFile, pDlx, 1);
                pDlx->mRefs++;
            }
            *appRetDlx = pDlx;
            return K2STAT_OK;
        }
    }

    status = sPrep(apAcqContext, apSpec, apName, aNameLen, apMatchId, appRetDlx);
    if (K2STAT_IS_ERROR(status))
    {
        *appRetDlx = NULL;
        return status;
    }

    if (apMatchId == NULL)
    {
        //
        // top level load
        //
        status = sExecLoads(apAcqContext);

        // if we failed we only need to release the instigating node
        // and it will transitively release all the other ones that
        // have been acquired or are pending load
        if (K2STAT_IS_ERROR(status))
        {
            pDlx = *appRetDlx;
            iK2DLXSUPP_ReleaseModule(pDlx);
            *appRetDlx = NULL;
        }
    }

    return status;
}

K2STAT
DLX_Acquire(
    char const *    apFileSpec,
    void *          apContext,
    DLX **          appRetDlx,
    UINT32 *        apRetEntryStackReq,
    K2_GUID128 *    apRetID
    )
{
    K2STAT          status;
    DLX *           pDlx;
    UINT32          nameLen;
    char            ch;
    char const *    pScan;

    if ((apFileSpec == NULL) || ((*apFileSpec)==0) || (appRetDlx == NULL))
    {
        return K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_BAD_ARGUMENT);
    }

    *appRetDlx = NULL;
    if (NULL != apRetEntryStackReq)
        *apRetEntryStackReq = 0;
    if (NULL != apRetID)
        K2MEM_Zero(apRetID, sizeof(K2_GUID128));
    
    nameLen = 0;
    pScan = apFileSpec;
    while (*pScan)
        pScan++;
    do
    {
        pScan--;
        ch = *pScan;
        if (ch == '.')
            nameLen = 0;
        else
        {
            if ((ch == '/') || (ch == '\\'))
            {
                pScan++;
                break;
            }
            nameLen++;
        }
    } while (pScan != apFileSpec);

    if (nameLen == 0)
    {
        return K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_BAD_ARGUMENT);
    }

    if (gpK2DLXSUPP_Vars->Host.CritSec != NULL)
    {
        status = gpK2DLXSUPP_Vars->Host.CritSec(TRUE);
        if (K2STAT_IS_ERROR(status))
        {
            return status;
        }
    }

    if (gpK2DLXSUPP_Vars->mAcqDisabled)
    {
        if (gpK2DLXSUPP_Vars->Host.CritSec != NULL)
            gpK2DLXSUPP_Vars->Host.CritSec(FALSE);
        return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_INSIDE_CALLBACK);
    }

    K2LIST_Init(&gpK2DLXSUPP_Vars->AcqList);

    status = sAcquire(apContext, apFileSpec, pScan, nameLen, NULL, &pDlx);

    if (gpK2DLXSUPP_Vars->Host.CritSec != NULL)
        gpK2DLXSUPP_Vars->Host.CritSec(FALSE);
        
    if (K2STAT_IS_ERROR(status))
    {
        return status;
    }

    *appRetDlx = pDlx;
    if (NULL != apRetEntryStackReq)
        *apRetEntryStackReq = pDlx->mpInfo->mEntryStackReq;
    if (NULL != apRetID)
        *apRetID = pDlx->mpInfo->ID;
    
    return K2STAT_OK;
}

