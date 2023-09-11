//   
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
#include "K2OsLoader.h"

#if K2_TARGET_ARCH_IS_ARM
static CHAR16 const * const sgpKernSubDir = L"k2os\\a32\\kern";
#else
static CHAR16 const * const sgpKernSubDir = L"k2os\\x32\\kern";
#endif

static EFI_GUID const sgEfiFileInfoId = EFI_FILE_INFO_ID;

static EFI_LOADED_IMAGE_PROTOCOL *      sgpLoadedImage = NULL;
static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sgpFileProt = NULL;
static EFI_FILE_PROTOCOL *              sgpRootDir = NULL;
static EFI_FILE_PROTOCOL *              sgpKernDir = NULL;

static EFI_STATUS sLoadRofs(void)
{
    EFI_STATUS              efiStatus;
    EFI_FILE_PROTOCOL *     pProt;
    UINTN                   infoSize;
    EFI_FILE_INFO *         pFileInfo;
    EFI_PHYSICAL_ADDRESS    pagesPhys;
    UINTN                   bytesToRead;

    //
    // load the builtin ROFS now from sgpKernDir.  shove physical address into gData.LoadInfo.mBuiltinRofsPhys
    //
    efiStatus = sgpKernDir->Open(
        sgpKernDir,
        &pProt,
        L"k2oskern.img",
        EFI_FILE_MODE_READ,
        0);
    if (EFI_ERROR(efiStatus))
        return efiStatus;

    do {
        infoSize = 0;
        efiStatus = pProt->GetInfo(
            pProt,
            (EFI_GUID *)&sgEfiFileInfoId,
            &infoSize, NULL);
        if (efiStatus != EFI_BUFFER_TOO_SMALL)
        {
            K2Printf(L"*** could not retrieve size of file info, status %r\n", efiStatus);
            break;
        }

        efiStatus = gBS->AllocatePool(EfiLoaderData, infoSize, (void **)&pFileInfo);
        if (efiStatus != EFI_SUCCESS)
        {
            K2Printf(L"*** could not allocate memory for file info, status %r\n", efiStatus);
            break;
        }

        do
        {
            efiStatus = pProt->GetInfo(
                pProt,
                (EFI_GUID *)&sgEfiFileInfoId,
                &infoSize, pFileInfo);
            if (efiStatus != EFI_SUCCESS)
            {
                K2Printf(L"*** could not get file info, status %r\n", efiStatus);
                break;
            }

            bytesToRead = (UINTN)pFileInfo->FileSize;

            gData.mRofsPages = (UINT32)(K2_ROUNDUP(bytesToRead, K2_VA_MEMPAGE_BYTES) / K2_VA_MEMPAGE_BYTES);

            efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_BUILTIN, gData.mRofsPages, &pagesPhys);
            if (efiStatus != EFI_SUCCESS)
            {
                K2Printf(L"*** AllocatePages for builtin failed with status %r\n", efiStatus);
                break;
            }

            do {
                gData.LoadInfo.mBuiltinRofsPhys = ((UINT32)pagesPhys);

                K2MEM_Zero((void *)gData.LoadInfo.mBuiltinRofsPhys, gData.mRofsPages * K2_VA_MEMPAGE_BYTES);

                efiStatus = pProt->Read(pProt, &bytesToRead, (void *)((UINT32)pagesPhys));
                if (EFI_ERROR(efiStatus))
                {
                    K2Printf(L"*** ReadSectors failed with efi Status %r\n", efiStatus);
                }

            } while (0);

            if (EFI_ERROR(efiStatus))
            {
                gBS->FreePages(pagesPhys, gData.mRofsPages);
            }

        } while (0);

        gBS->FreePool(pFileInfo);

    } while (0);

    pProt->Close(pProt);

    return efiStatus;
}

EFI_STATUS sysXDL_Init(IN EFI_HANDLE ImageHandle)
{
    EFI_STATUS status;

    K2LIST_Init(&gData.EfiFileList);

    status = gBS->OpenProtocol(ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&sgpLoadedImage,
        ImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    if (status != EFI_SUCCESS)
    {
        K2Printf(L"*** Open LoadedImageProt failed with %r\n", status);
        return status;
    }

    do
    {
        status = gBS->OpenProtocol(sgpLoadedImage->DeviceHandle,
            &gEfiSimpleFileSystemProtocolGuid,
            (void **)&sgpFileProt,
            ImageHandle,
            NULL, 
            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

        if (status != EFI_SUCCESS)
        {
            K2Printf(L"*** Open FileSystemProt failed with %r\n", status);
            break;
        }
        
        do
        {
            status = sgpFileProt->OpenVolume(sgpFileProt, &sgpRootDir);
            if (status != EFI_SUCCESS)
            {
                K2Printf(L"*** OpenVolume failed with %r\n", status);
                break;
            }

            do
            {
                status = sgpRootDir->Open(sgpRootDir, &sgpKernDir, (CHAR16 *)sgpKernSubDir, EFI_FILE_MODE_READ, 0);
                if (status != EFI_SUCCESS)
                {
                    K2Printf(L"*** Open %s failed with %r\n", sgpKernSubDir, status);
                    break;
                }

                status = sLoadRofs();
                if (status != EFI_SUCCESS)
                {
                    K2Printf(L"*** Could not load builtin filesystem from %s with error %r\n", sgpKernSubDir, status);
                    sgpKernDir->Close(sgpKernDir);
                    sgpKernDir = NULL;
                    break;
                }

                gData.mLoaderImageHandle = ImageHandle;

            } while (0);

            if (gData.mLoaderImageHandle == NULL)
                sgpRootDir->Close(sgpRootDir);

        } while (0);

        if (gData.mLoaderImageHandle == NULL)
        {
            gBS->CloseProtocol(sgpLoadedImage->DeviceHandle,
                &gEfiSimpleFileSystemProtocolGuid,
                ImageHandle,
                NULL);
            sgpFileProt = NULL;
        }

    } while (0);

    if (gData.mLoaderImageHandle == NULL)
    {
        gBS->CloseProtocol(ImageHandle,
            &gEfiLoadedImageProtocolGuid,
            ImageHandle, 
            NULL);
        sgpLoadedImage = NULL;
        return EFI_NOT_FOUND;
    }

    return EFI_SUCCESS;
}

static
void
sDoneFile(
    EFIFILE *apFile
)
{
    EFIXDL *                pXdl;
    EFI_PHYSICAL_ADDRESS    physAddr;
    UINTN                   segIx;
    UINTN                   segBytes;

    pXdl = apFile->mpXdl;
    if (pXdl != NULL)
    {
        for (segIx = XDLSegmentIx_Text; segIx < XDLSegmentIx_Relocs; segIx++)
        {
            segBytes = pXdl->mSegBytes[segIx];
            segBytes = K2_ROUNDUP(segBytes, K2_VA_MEMPAGE_BYTES);
            if (segBytes > 0)
            {
                physAddr = pXdl->mSegPhys[segIx];
                if (physAddr != 0)
                    gBS->FreePages(physAddr, segBytes / K2_VA_MEMPAGE_BYTES);
                pXdl->mSegBytes[segIx] = 0;
            }
        }

        gBS->FreePool(pXdl);
    }

    physAddr = apFile->mPageAddr_Phys;
    gBS->FreePages(physAddr, 1);
    apFile->mPageAddr_Phys = 0;

    gBS->FreePool(apFile->mpFileInfo);
    apFile->mpFileInfo = NULL;

    apFile->mpProt->Close(apFile->mpProt);
    apFile->mpProt = NULL;

    K2LIST_Remove(&gData.EfiFileList, &apFile->ListLink);

    K2MEM_Zero(apFile, sizeof(EFI_FILE));

    gBS->FreePool(apFile);
}

void 
sysXDL_Done(
    void
)
{
    K2LIST_LINK *           pLink;
    EFIFILE *               pFile;
    EFI_PHYSICAL_ADDRESS    pagesPhys;

    if (gData.mLoaderImageHandle == NULL)
        return;

    do
    {
        pLink = gData.EfiFileList.mpHead;
        if (pLink == NULL)
            break;
        pFile = K2_GET_CONTAINER(EFIFILE, pLink, ListLink);
        sDoneFile(pFile);
    } while (1);

    sgpKernDir->Close(sgpKernDir);
    sgpKernDir = NULL;

    sgpRootDir->Close(sgpRootDir);
    sgpRootDir = NULL;

    gBS->CloseProtocol(sgpLoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        gData.mLoaderImageHandle,
        NULL);
    sgpFileProt = NULL;

    gBS->CloseProtocol(gData.mLoaderImageHandle,
        &gEfiLoadedImageProtocolGuid,
        gData.mLoaderImageHandle,
        NULL);
    sgpLoadedImage = NULL;

    pagesPhys = gData.LoadInfo.mBuiltinRofsPhys;
    gBS->FreePages(pagesPhys, gData.mRofsPages);

    gData.mLoaderImageHandle = NULL;
}

K2STAT      
sysXDL_Open(
    K2XDL_OPENARGS const *  apArgs,
    K2XDL_HOST_FILE *       apRetHostFile,
    UINT_PTR *              apRetModuleDataAddr,
    UINT_PTR *              apRetModuleLinkAddr
)
{
    EFIFILE *               pFile;
    EFIFILE *               pCheckFile;
    K2LIST_LINK *           pLink;
    K2STAT                  status;
    EFI_STATUS              efiStatus;
    EFI_PHYSICAL_ADDRESS    pagePhys;
    UINTN                   infoSize;
    UINTN                   nameLen;
    CHAR16                  uniFileName[XDL_NAME_MAX_LEN];

    nameLen = apArgs->mNameLen;
    if (nameLen > XDL_NAME_MAX_LEN)
        nameLen = XDL_NAME_MAX_LEN - 1;

    status = K2STAT_ERROR_UNKNOWN;
    do
    {
        efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_XDL_PAGE, 1, &pagePhys);
        if (efiStatus != EFI_SUCCESS)
        {
            K2Printf(L"*** AllocatePages for xdl failed with status %r\n", efiStatus);
            status = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }
        *apRetModuleDataAddr = ((UINT32)pagePhys);
        K2MEM_Zero((void *)(*apRetModuleDataAddr), K2_VA_MEMPAGE_BYTES);

        do
        {
            efiStatus = gBS->AllocatePool(EfiLoaderData, sizeof(EFIFILE), (void **)&pFile);
            if (efiStatus != EFI_SUCCESS)
            {
                K2Printf(L"*** AllocatePool for file track failed with status %r\n", efiStatus);
                status = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }
            K2MEM_Zero(pFile, sizeof(EFIFILE));

            do
            {
                K2MEM_Copy(pFile->mFileName, apArgs->mpNamePart, nameLen);
                pFile->mFileName[nameLen] = 0;

                pLink = gData.EfiFileList.mpHead;
                while (pLink != NULL)
                {
                    pCheckFile = K2_GET_CONTAINER(EFIFILE, pLink, ListLink);
                    if (0 == K2ASC_CompIns(pFile->mFileName, pCheckFile->mFileName))
                        break;
                    pLink = pLink->mpNext;
                }

                if (pLink != NULL)
                {
                    K2Printf(L"*** File already open\n");
                    status = K2STAT_ERROR_ALREADY_OPEN;
                    break;
                }

                UnicodeSPrintAsciiFormat(uniFileName, sizeof(CHAR16) * XDL_NAME_MAX_LEN, "%.*a.xdl", nameLen, apArgs->mpNamePart);
                uniFileName[XDL_NAME_MAX_LEN - 1] = 0;

                efiStatus = sgpKernDir->Open(
                    sgpKernDir,
                    &pFile->mpProt,
                    uniFileName,
                    EFI_FILE_MODE_READ,
                    0);
                if (efiStatus != EFI_SUCCESS)
                {
                    K2Printf(L"*** Open failed with efi status %r\n", efiStatus);
                    status = K2STAT_ERROR_NOT_FOUND;
                    break;
                }

                do
                {
                    infoSize = 0;
                    efiStatus = pFile->mpProt->GetInfo(
                        pFile->mpProt,
                        (EFI_GUID *)&sgEfiFileInfoId,
                        &infoSize, NULL);
                    if (efiStatus != EFI_BUFFER_TOO_SMALL)
                    {
                        K2Printf(L"*** could not retrieve size of file info, status %r\n", efiStatus);
                        status = K2STAT_ERROR_UNKNOWN;
                        break;
                    }

                    efiStatus = gBS->AllocatePool(EfiLoaderData, infoSize, (void **)&pFile->mpFileInfo);
                    if (efiStatus != EFI_SUCCESS)
                    {
                        K2Printf(L"*** could not allocate memory for file info, status %r\n", efiStatus);
                        status = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    do
                    {
                        efiStatus = pFile->mpProt->GetInfo(
                            pFile->mpProt,
                            (EFI_GUID *)&sgEfiFileInfoId,
                            &infoSize, pFile->mpFileInfo);
                        if (efiStatus != EFI_SUCCESS)
                        {
                            K2Printf(L"*** could not get file info, status %r\n", efiStatus);
                            status = K2STAT_ERROR_UNKNOWN;
                            break;
                        }

                        pFile->mPageAddr_Phys = pagePhys;

                        K2LIST_AddAtHead(&gData.EfiFileList, &pFile->ListLink);

                        *apRetHostFile = (K2XDL_HOST_FILE)pFile;

                        status = K2STAT_OK;

                    } while (0);

                    if (K2STAT_IS_ERROR(status))
                        gBS->FreePool(pFile->mpFileInfo);

                } while (0);

                if (K2STAT_IS_ERROR(status))
                {
                    pFile->mpProt->Close(pFile->mpProt);
                    pFile->mpProt = NULL;
                }

            } while (0);

            if (K2STAT_IS_ERROR(status))
                gBS->FreePool(pFile);
            else
            {
                gData.LoadInfo.mKernArenaHigh -= K2_VA_MEMPAGE_BYTES;
                pFile->mPageAddr_Virt = gData.LoadInfo.mKernArenaHigh;
                *apRetModuleLinkAddr = pFile->mPageAddr_Virt;
            }

        } while (0);

        if (K2STAT_IS_ERROR(status))
            gBS->FreePages(pagePhys, 1);

    } while (0);

    return status;
}

K2STAT      
sysXDL_ReadSectors(
    K2XDL_LOADCTX const *   apLoadCtx,
    void *                  apBuffer,
    UINT64 const *          apSectorCount
)
{
    K2STAT      status;
    EFIFILE *   pFile;
    UINTN       bytesToRead;
    UINT64      bytesLeft;
    EFI_STATUS  efiStatus;

    status = K2STAT_ERROR_UNKNOWN;
    do
    {
        pFile = (EFIFILE *)apLoadCtx->mHostFile;

        bytesLeft = pFile->mpFileInfo->FileSize - pFile->mCurPos;
        bytesToRead = ((UINTN)*apSectorCount) * XDL_SECTOR_BYTES;

        if (bytesToRead > (UINTN)bytesLeft)
            bytesToRead = (UINTN)bytesLeft;

        efiStatus = pFile->mpProt->Read(pFile->mpProt, &bytesToRead, apBuffer);
        if (efiStatus != EFI_SUCCESS)
        {
            K2Printf(L"*** ReadSectors failed with efi Status %r\n", efiStatus);
            status = K2STAT_ERROR_UNKNOWN;
            break;
        }

        status = K2STAT_OK;

    } while (0);

    return status;
}

K2STAT 
sysXDL_Prepare(
    K2XDL_LOADCTX const *   apLoadCtx, 
    BOOL                    aKeepSymbols, 
    XDL_FILE_HEADER const * apFileHdr, 
    K2XDL_SEGMENT_ADDRS *   apRetSegAddrs
)
{
    K2STAT                  status;
    EFIFILE *               pFile;
    EFIXDL *                pOut;
    UINTN                   segIx;
    UINTN                   memType;
    EFI_PHYSICAL_ADDRESS    physAddr;
    UINTN                   allocSize;
    EFI_STATUS              efiStatus;
    UINTN                   endSeg;

    status = K2STAT_ERROR_UNKNOWN;

    do {
        pFile = (EFIFILE *)apLoadCtx->mHostFile;

        efiStatus = gBS->AllocatePool(EfiLoaderData, sizeof(EFIXDL), (void **)&pOut);
        if (efiStatus != EFI_SUCCESS)
        {
            K2Printf(L"*** Allocation of EFI XDL track failed\n");
            status = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }

        do {
            K2MEM_Zero(pOut, sizeof(EFIXDL));

            for (segIx = XDLSegmentIx_Text; segIx < XDLSegmentIx_Count; segIx++)
            {
                pOut->mSegBytes[segIx] = allocSize = (UINTN)apFileHdr->Segment[segIx].mMemActualBytes;
                if (allocSize > 0)
                {
                    allocSize = K2_ROUNDUP(allocSize, K2_VA_MEMPAGE_BYTES);
                    if (segIx == XDLSegmentIx_Text)
                        memType = K2OS_EFIMEMTYPE_XDL_TEXT;
                    else if (segIx == XDLSegmentIx_Data)
                        memType = K2OS_EFIMEMTYPE_XDL_DATA;
                    else
                        memType = K2OS_EFIMEMTYPE_XDL_READ;

                    efiStatus = gBS->AllocatePages(
                        AllocateAnyPages,
                        memType,
                        allocSize / K2_VA_MEMPAGE_BYTES,
                        &physAddr);

                    if (efiStatus != EFI_SUCCESS)
                    {
                        pOut->mSegBytes[segIx] = 0;
                        K2Printf(L"*** Allocate %d pages for segment failed with %r\n", allocSize / K2_VA_MEMPAGE_BYTES, efiStatus);
                        status = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    pOut->mSegPhys[segIx] = apRetSegAddrs->mData[segIx] = (UINTN)physAddr;
                }
            }
            if (segIx < XDLSegmentIx_Count)
            {
                if (segIx > XDLSegmentIx_Text)
                {
                    do
                    {
                        segIx--;
                        physAddr = apRetSegAddrs->mData[segIx];
                        if (0 != physAddr)
                        {
                            apRetSegAddrs->mData[segIx] = 0;
                            pOut->mSegPhys[segIx] = 0;
                            allocSize = (UINTN)apFileHdr->Segment[segIx].mMemActualBytes;
                            allocSize = K2_ROUNDUP(allocSize, K2_VA_MEMPAGE_BYTES);
                            gBS->FreePages(physAddr, allocSize / K2_VA_MEMPAGE_BYTES);
                        }
                    } while (segIx > XDLSegmentIx_Text);
                }
                break;
            }

            // if we get here we are good and need virtual space for each segment we allocated
            // in physical space
            endSeg = XDLSegmentIx_Relocs;
            if (!aKeepSymbols)
                endSeg--;

            for (segIx = XDLSegmentIx_Text; segIx < endSeg; segIx++)
            {
                allocSize = (UINTN)apFileHdr->Segment[segIx].mMemActualBytes;
                if (allocSize > 0)
                {
                    allocSize = K2_ROUNDUP(allocSize, K2_VA_MEMPAGE_BYTES);
                    //
                    // XDL must go high as on A32, exception vectors at 0xFFFF0000 must be able
                    // to jump into kernel text segment directly (max 26-bit signed offset)
                    // and FFFF0000 to 814xxxxx is too far.
                    //
                    gData.LoadInfo.mKernArenaHigh -= allocSize;
                    pOut->mSegTargetLink[segIx] = apRetSegAddrs->mLink[segIx] = gData.LoadInfo.mKernArenaHigh;
//                    K2Printf(L"%08X Segment %d Link Addr 0x%08X\n", pOut, segIx, pOut->mSegTargetLink[segIx]);
                }
            }

            pFile->mpXdl = pOut;

            status = K2STAT_OK;

        } while (0);

        if (K2STAT_IS_ERROR(status))
        {
            gBS->FreePool(pOut);
        }

    } while (0);

    return status;
}

K2STAT      
sysXDL_Finalize(
    K2XDL_LOADCTX const *   apLoadCtx, 
    BOOL                    aKeepSymbols, 
    XDL_FILE_HEADER const * apFileHdr, 
    K2XDL_SEGMENT_ADDRS *   apUpdateSegAddrs
)
{
    UINTN                   segIx;
    EFIFILE *               pFile;
    EFIXDL *                pXdl;
    UINTN                   segSize;
    K2STAT                  status;
    EFI_PHYSICAL_ADDRESS    physAddr;

    status = K2STAT_ERROR_UNKNOWN;

    do
    {
        pFile = (EFIFILE *)apLoadCtx->mHostFile;

        pXdl = pFile->mpXdl;

        segIx = XDLSegmentIx_Relocs;
        if (!aKeepSymbols)
            segIx--;
        do {
            segSize = pXdl->mSegBytes[segIx];
            if (segSize > 0)
            {
                segSize = K2_ROUNDUP(segSize, K2_VA_MEMPAGE_BYTES);
                physAddr = pXdl->mSegPhys[segIx];
                K2MEM_Zero((void *)((UINTN)physAddr), segSize);
                gBS->FreePages(physAddr, segSize / K2_VA_MEMPAGE_BYTES);

                pXdl->mSegPhys[segIx] = 0;
                pXdl->mSegBytes[segIx] = 0;
                apUpdateSegAddrs->mData[segIx] = 0;
            }
        } while (++segIx < XDLSegmentIx_Count);

        status = K2STAT_OK;

    } while (0);

    return status;
}

K2STAT      
sysXDL_Purge(
    K2XDL_LOADCTX const *       apLoadCtx, 
    K2XDL_SEGMENT_ADDRS const * apSegAddrs
)
{
    sDoneFile((EFIFILE *)apLoadCtx->mHostFile);
    return K2STAT_NO_ERROR;
}

static
BOOL
sConvertLoadPtrInXDL(
    EFIXDL *    apXDL,
    UINTN *     apAddr
)
{
    UINTN ixSeg;
    UINTN addr;
    UINTN segAddrBase;
    UINTN segAddrEnd;
    UINTN endSeg;

    addr = *apAddr;

    endSeg = XDLSegmentIx_Relocs;
    if (0 == apXDL->mSegTargetLink[XDLSegmentIx_Symbols])
        endSeg--;

    for (ixSeg = XDLSegmentIx_Text; ixSeg < endSeg; ixSeg++)
    {
        segAddrBase = apXDL->mSegPhys[ixSeg];
        if (segAddrBase != 0)
        {
            segAddrEnd = segAddrBase + apXDL->mSegBytes[ixSeg];
            if ((addr >= segAddrBase) && (addr < segAddrEnd))
            {
                addr -= segAddrBase;
                addr += apXDL->mSegTargetLink[ixSeg];
                *apAddr = addr;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL        
sysXDL_ConvertLoadPtr(
    UINT32 * apAddr
)
{
    K2LIST_LINK *   pLink;
    EFIFILE *       pFile;
    UINTN           addr;
    UINTN           pagePhys;

    addr = *apAddr;
    if (0 == addr)
        return TRUE;

    pLink = gData.EfiFileList.mpHead;
    while (pLink != NULL)
    {
        // is address inside this file's data?
        pFile = K2_GET_CONTAINER(EFIFILE, pLink, ListLink);
        if (sConvertLoadPtrInXDL(pFile->mpXdl, apAddr))
            return TRUE;

        // is address inside this file's node page?
        pagePhys = (UINTN)pFile->mPageAddr_Phys;
        if ((addr >= pagePhys) &&
            (addr < pagePhys + K2_VA_MEMPAGE_BYTES))
        {
            *apAddr = (addr - pagePhys) + pFile->mPageAddr_Virt;
            return TRUE;
        }
        pLink = pLink->mpNext;
    }

    // is this address inside the var page?
    pagePhys = (UINTN)gData.mLoaderPagePhys;
    if ((addr >= pagePhys) &&
        (addr < pagePhys + K2_VA_MEMPAGE_BYTES))
    {
        *apAddr = (addr - pagePhys) + K2OS_KVA_LOADERPAGE_BASE;
        return TRUE;
    }

    return FALSE;
}

