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
#include "K2OsLoader.h"

#define DEBUG_FUNCINFO  0

#if DEBUG_FUNCINFO
#define FUNCINFO(x)     K2Printf x
#else
#define FUNCINFO(x) 
#endif

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

static
BOOL
sConvertLoadPtrInDLX(
    EFIDLX *    apDLX,
    UINTN *     apAddr
    )
{
    UINTN ixSeg;
    UINTN addr;
    UINTN segAddrBase;
    UINTN segAddrEnd;

    addr = *apAddr;
    for (ixSeg = DlxSeg_Text; ixSeg < apDLX->mFirstNonLink; ixSeg++)
    {
        segAddrBase = apDLX->SegAlloc.Segment[ixSeg].mDataAddr;
        if (segAddrBase != 0)
        {
            segAddrEnd = segAddrBase + apDLX->Info.SegInfo[ixSeg].mMemActualBytes;
            if ((addr >= segAddrBase) && (addr < segAddrEnd))
            {
                addr -= segAddrBase;
                addr += apDLX->SegAlloc.Segment[ixSeg].mLinkAddr;
                *apAddr = addr;
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL sysDLX_ConvertLoadPtr(UINTN * apAddr)
{
    K2LIST_LINK *   pLink;
    EFIFILE *       pFile;
    UINTN           addr;
    UINTN           pagePhys;

    addr = *apAddr;

    pLink = gData.EfiFileList.mpHead;
    while (pLink != NULL)
    {
        // is address inside this file's data?
        pFile = K2_GET_CONTAINER(EFIFILE, pLink, ListLink);
        if (sConvertLoadPtrInDLX(pFile->mpDlx, apAddr))
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

static EFIFILE * sFindFile(K2DLXSUPP_HOST_FILE aHostFile)
{
    K2LIST_LINK * pLink;

    pLink = gData.EfiFileList.mpHead;
    while (pLink != NULL)
    {
        if (pLink == (K2LIST_LINK *)aHostFile)
            break;
        pLink = pLink->mpNext;
    }
    return (EFIFILE *)pLink;
}

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

EFI_STATUS sysDLX_Init(IN EFI_HANDLE ImageHandle)
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
sDoneDlx(
    EFIDLX *apDLX
    )
{
    UINTN                   segIx;
    UINTN                   segBytes;
    EFI_PHYSICAL_ADDRESS    physAddr;

    for (segIx = DlxSeg_Text; segIx < DlxSeg_Count; segIx++)
    {
        segBytes = K2_ROUNDUP(apDLX->Info.SegInfo[segIx].mMemActualBytes, K2_VA_MEMPAGE_BYTES);
        if (segBytes > 0)
        {
            physAddr = apDLX->SegAlloc.Segment[segIx].mDataAddr;
            if (physAddr != 0)
                gBS->FreePages(physAddr, segBytes / K2_VA_MEMPAGE_BYTES);
        }
    }

    gBS->FreePool(apDLX);
}

static
void
sDoneFile(
    EFIFILE *apFile
    )
{
    EFIDLX *                pDlx;
    EFI_PHYSICAL_ADDRESS    physAddr;

    //
    // allocations for runtime memory will be in the
    // memory map
    //

    pDlx = apFile->mpDlx;
    if (pDlx != NULL)
    {
        sDoneDlx(pDlx);
        apFile->mpDlx = NULL;
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

void sysDLX_Done(void)
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

K2STAT sysDLX_Open(void *apAcqContext, char const * apFileSpec, char const *apNamePart, UINT32 aNamePartLen, K2DLXSUPP_OPENRESULT *apRetResult)
{
    EFIFILE *               pFile;
    EFIFILE *               pCheckFile;
    K2LIST_LINK *           pLink;
    K2STAT                  status;
    EFI_STATUS              efiStatus;
    EFI_PHYSICAL_ADDRESS    pagePhys;
    UINTN                   infoSize;
    CHAR16                  uniFileName[MAX_EFI_FILE_NAME_LEN];

    if (aNamePartLen > MAX_EFI_FILE_NAME_LEN)
        aNamePartLen = MAX_EFI_FILE_NAME_LEN - 1;

    FUNCINFO((L"+sysDLX_Open(\"%.*a\", %d, %08X, 0x%08X)\n", aNamePartLen, apNamePart, aNamePartLen, apContext, apRetResult));
    status = K2STAT_ERROR_UNKNOWN;
    do
    {
        efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_DLX_PAGE, 1, &pagePhys);
        if (efiStatus != EFI_SUCCESS)
        {
            K2Printf(L"*** AllocatePages for dlx failed with status %r\n", efiStatus);
            status = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }
        apRetResult->mModulePageDataAddr = ((UINT32)pagePhys);
        K2MEM_Zero((void *)apRetResult->mModulePageDataAddr, K2_VA_MEMPAGE_BYTES);

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
                K2MEM_Copy(pFile->mFileName, apNamePart, aNamePartLen);
                pFile->mFileName[aNamePartLen] = 0;

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

                UnicodeSPrintAsciiFormat(uniFileName, sizeof(CHAR16)*MAX_EFI_FILE_NAME_LEN, "%.*a.dlx", aNamePartLen, apNamePart);
                uniFileName[MAX_EFI_FILE_NAME_LEN-1] = 0;

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

                        apRetResult->mHostFile = (K2DLXSUPP_HOST_FILE)pFile;
                        apRetResult->mFileSectorCount = (UINT32)(K2_ROUNDUP(pFile->mpFileInfo->FileSize, DLX_SECTOR_BYTES) / DLX_SECTOR_BYTES);

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
                pFile->mPageAddr_Virt = gData.LoadInfo.mKernArenaLow;
                apRetResult->mModulePageLinkAddr = pFile->mPageAddr_Virt;
                gData.LoadInfo.mKernArenaLow += K2_VA_MEMPAGE_BYTES;
            }

        } while (0);

        if (K2STAT_IS_ERROR(status))
            gBS->FreePages(pagePhys, 1);

    } while (0);

    FUNCINFO((L"-sysDLX_Open:%08X\n", status));

    return status;
}

K2STAT sysDLX_ReadSectors(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, void *apBuffer, UINTN aSectorCount)
{
    K2STAT      status;
    EFIFILE *   pFile;
    UINTN       bytesToRead;
    UINT64      bytesLeft;
    EFI_STATUS  efiStatus;

    FUNCINFO((L"+sysDLX_ReadSectors(0x%08X, 0x%08X, %d)\n", aHostFile, apBuffer, aSectorCount));
    status = K2STAT_ERROR_UNKNOWN;
    do
    {
        pFile = sFindFile(aHostFile);
        if (pFile == NULL)
        {
            K2Printf(L"*** ReadSectors for unknown file\n");
            status = K2STAT_ERROR_NOT_FOUND;
            break;
        }

        bytesLeft = pFile->mpFileInfo->FileSize - pFile->mCurPos;
        bytesToRead = aSectorCount * DLX_SECTOR_BYTES;
    
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
    FUNCINFO((L"-sysDLX_ReadSectors:%08X\n", status));
    return status;
}

K2STAT sysDLX_Prepare(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, DLX_INFO *apInfo, UINTN aInfoSize, BOOL aKeepSym, K2DLXSUPP_SEGALLOC *apRetAlloc)
{
    K2STAT                  status;
    EFIFILE *               pFile;
    EFIDLX *                pOut;
    UINTN                   allocSize;
    UINTN                   segIx;
    EFI_STATUS              efiStatus;
    UINTN                   memType;
    EFI_PHYSICAL_ADDRESS    physAddr;

    FUNCINFO((L"+sysDLX_Prepare(%08X, 0x%08X, 0x%08X, %d, %s, %08X)\n", apAcqContext, aHostFile, apInfo, aInfoSize, aKeepSym?L"TRUE":L"FALSE", apRetAlloc));
    status = K2STAT_ERROR_UNKNOWN;
    do
    {
        K2MEM_Zero(apRetAlloc, sizeof(K2DLXSUPP_SEGALLOC));

        if (aInfoSize < sizeof(DLX_INFO))
        {
            K2Printf(L"*** Prepare given bad DLX info size\n");
            status = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        pFile = sFindFile(aHostFile);
        if (pFile == NULL)
        {
            K2Printf(L"*** Prepare for unknown DLX\n");
            status = K2STAT_ERROR_NOT_FOUND;
            break;
        }

        if (pFile->mpDlx != NULL)
        {
            K2Printf(L"*** Prepare for DLX already prepared\n");
            status = K2STAT_ERROR_API_ORDER;
            break;
        }

        allocSize = sizeof(EFIDLX) - sizeof(DLX_INFO) + aInfoSize;

        efiStatus = gBS->AllocatePool(EfiLoaderData, K2_ROUNDUP(allocSize, 4), (void **)&pOut);
        if (efiStatus != EFI_SUCCESS)
        {
            K2Printf(L"*** Allocation of EFI DLX track failed\n");
            status = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }

        do
        {
            pOut->mFirstNonLink = aKeepSym ? DlxSeg_Reloc : DlxSeg_Sym;
            pOut->mEfiDlxBytes = allocSize;
            K2MEM_Zero(&pOut->SegAlloc, sizeof(K2DLXSUPP_SEGALLOC));
            K2MEM_Copy(&pOut->Info, apInfo, aInfoSize);

            // first segment is text and that is runtime code
            for (segIx = DlxSeg_Text; segIx < DlxSeg_Count; segIx++)
            {
                allocSize = K2_ROUNDUP(pOut->Info.SegInfo[segIx].mMemActualBytes, K2_VA_MEMPAGE_BYTES);
                if (allocSize > 0)
                {
                    if (segIx == DlxSeg_Text)
                        memType = K2OS_EFIMEMTYPE_DLX_TEXT;
                    else if (segIx == DlxSeg_Data)
                        memType = K2OS_EFIMEMTYPE_DLX_DATA;
                    else
                        memType = K2OS_EFIMEMTYPE_DLX_READ;

                    efiStatus = gBS->AllocatePages(
                        AllocateAnyPages,
                        memType,
                        allocSize / K2_VA_MEMPAGE_BYTES,
                        &physAddr);

                    if (efiStatus != EFI_SUCCESS)
                    {
                        K2Printf(L"*** Allocate %d pages for segment failed with %r\n", allocSize / K2_VA_MEMPAGE_BYTES, efiStatus);
                        status = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    pOut->SegAlloc.Segment[segIx].mDataAddr = (UINTN)physAddr;
                }
            }
            if (segIx < DlxSeg_Count)
            {
                if (segIx > DlxSeg_Text)
                {
                    do
                    {
                        segIx--;
                        allocSize = K2_ROUNDUP(pOut->Info.SegInfo[segIx].mMemActualBytes, K2_VA_MEMPAGE_BYTES);
                        physAddr = pOut->SegAlloc.Segment[segIx].mDataAddr;
                        gBS->FreePages(physAddr, allocSize / K2_VA_MEMPAGE_BYTES);
                    } while (segIx > DlxSeg_Text);
                }
                break;
            }

            // if we get here we are good and need virtual space for each segment we allocated
            // in physical space
            for (segIx = DlxSeg_Text; segIx < pOut->mFirstNonLink; segIx++)
            {
                allocSize = K2_ROUNDUP(pOut->Info.SegInfo[segIx].mMemActualBytes, K2_VA_MEMPAGE_BYTES);
                if (allocSize > 0)
                {
                    //
                    // DLX must go high as on A32, exception vectors at 0xFFFF0000 must be able
                    // to jump into kernel text segment directly (max 26-bit signed offset)
                    // and FFFF0000 to 814xxxxx is too far.
                    //
                    gData.LoadInfo.mKernArenaHigh -= allocSize;
//                    K2Printf(L"Segment %d Link Addr 0x%08X\n", segIx, gData.mKernArenaHigh);
                    pOut->SegAlloc.Segment[segIx].mLinkAddr = gData.LoadInfo.mKernArenaHigh;
                }
            }

            K2MEM_Copy(apRetAlloc, &pOut->SegAlloc, sizeof(K2DLXSUPP_SEGALLOC));

            pFile->mpDlx = pOut;

            status = K2STAT_OK;

        } while (0);

        if (K2STAT_IS_ERROR(status))
            gBS->FreePool(pOut);

    } while (0);
    FUNCINFO((L"-sysDLX_Prepare:%08X\n", status));
    return status;
}

BOOL sysDLX_PreCallback(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, BOOL aIsLoad, DLX *apDlx)
{
    EFIFILE *   pFile;
    pFile = sFindFile(aHostFile);
    K2_ASSERT(pFile != NULL);
//    K2Printf(L"%a %s\n", pFile->mFileName, aIsLoad?L"LOADED":L"UNLOADING");
    return FALSE;
}

K2STAT sysDLX_PostCallback(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, K2STAT aUserStatus, DLX *apDlx)
{
    // should never be called
    K2_ASSERT(0);
    return K2STAT_ERROR_UNKNOWN;
}

K2STAT sysDLX_Finalize(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, K2DLXSUPP_SEGALLOC *apUpdateAlloc)
{
    UINTN                   segIx;
    EFIFILE *               pFile;
    EFIDLX *                pDlx;
    UINTN                   segSize;
    K2STAT             status;
    EFI_PHYSICAL_ADDRESS    physAddr;

    FUNCINFO((L"+sysDLX_Finalize(0x%08X, 0x%08X)\n", aHostFile, apUpdateAlloc));
    status = K2STAT_ERROR_UNKNOWN;
    do
    {
        pFile = sFindFile(aHostFile);
        if (pFile == NULL)
        {
            K2Printf(L"*** Finalize for unknown DLX\n");
            status = K2STAT_ERROR_NOT_FOUND;
            break;
        }

        if (pFile->mpDlx == NULL)
        {
            K2Printf(L"*** Finalize on DLX not prepared\n");
            status = K2STAT_ERROR_API_ORDER;
            break;
        }

        pDlx = pFile->mpDlx;

        for (segIx = pDlx->mFirstNonLink; segIx < DlxSeg_Count; segIx++)
        {
            segSize = K2_ROUNDUP(pDlx->Info.SegInfo[segIx].mMemActualBytes, K2_VA_MEMPAGE_BYTES);
            if (segSize > 0)
            {
                physAddr = pDlx->SegAlloc.Segment[segIx].mDataAddr;
                K2MEM_Zero((void *)((UINTN)physAddr), segSize);
                gBS->FreePages(physAddr, segSize / K2_VA_MEMPAGE_BYTES);
                pDlx->SegAlloc.Segment[segIx].mDataAddr = 0;
                pDlx->Info.SegInfo[segIx].mMemActualBytes = 0;
                if (apUpdateAlloc != NULL)
                    apUpdateAlloc->Segment[segIx].mDataAddr = 0;
            }
        }

        status = K2STAT_OK;

    } while (0);
    FUNCINFO((L"-sysDLX_Finalize:%08X\n", status));
    return status;
}

K2STAT sysDLX_Purge(K2DLXSUPP_HOST_FILE aHostFile)
{
    K2STAT status;
    EFIFILE *   pFile;

    FUNCINFO((L"+sysDLX_Purge(0x%08X)\n", aHostFile));
    status = K2STAT_ERROR_UNKNOWN;
    do
    {
        pFile = sFindFile(aHostFile);
        if (pFile == NULL)
        {
            K2Printf(L"*** Purge for unknown DLX\n");
            status = K2STAT_ERROR_NOT_FOUND;
            break;
        }

        sDoneFile(pFile);

        status = K2STAT_OK;

    } while (0);
    FUNCINFO((L"-sysDLX_Purge:%08X\n", status));
    return status;
}

void DumpFile(CHAR16 const *apFileName, void *apData, UINTN aDataBytes)
{
    EFI_STATUS          efiStatus;
    EFI_FILE_PROTOCOL * pDump;

    efiStatus = sgpKernDir->Open(
        sgpKernDir,
        &pDump,
        (CHAR16 *)apFileName,
        (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE),
        0);
    if (efiStatus != EFI_SUCCESS)
    {
        K2Printf(L"*** Open \"%s\" failed with efi status %r\n", apFileName, efiStatus);
        return;
    }

    efiStatus = pDump->Write(pDump, &aDataBytes, apData);
    if (efiStatus != EFI_SUCCESS)
    {
        K2Printf(L"*** Write to \"%s\" failed with efi status %r\n", apFileName, efiStatus);
        return;
    }

    pDump->Close(pDump);
}


