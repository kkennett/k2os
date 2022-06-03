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

#define FADT_SIGNATURE  0x50434146  // FACP
#define DSDT_SIGNATURE  0x54445344  // DSDT
#define FACS_SIGNATURE  0x53434146  // FACS
#define BGRT_SIGNATURE  0x54524742  // BGRT

K2_PACKED_PUSH
struct _BMP_IMAGE_HEADER {
    UINT8   CharB;
    UINT8   CharM;
    UINT32  Size;
    UINT16  Reserved[2];
    UINT32  ImageOffset;
    UINT32  HeaderSize;
    UINT32  PixelWidth;
    UINT32  PixelHeight;
    UINT16  Planes;
    UINT16  BitPerPixel;
    UINT32  CompressionType;
    UINT32  ImageSize;
    UINT32  XPixelsPerMeter;
    UINT32  YPixelsPerMeter;
    UINT32  NumberOfColors;
    UINT32  ImportantColors;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _BMP_IMAGE_HEADER BMP_IMAGE_HEADER;

static
UINT8
AcpiTbChecksum(
    UINT8 * Buffer,
    UINT32  Length)
{
    UINT8   Sum = 0;
    UINT8 * End;

    Sum = 0;
    End = Buffer + Length;
    while (Buffer < End)
    {
        Sum = (UINT8)(Sum + *(Buffer++));
    }
    return Sum;
}

K2STAT Loader_AssembleAcpi(void)
{
    EFI_STATUS                                      efiStatus;
    K2STAT                                          stat;
    EFI_ACPI_SDT_HEADER *                           pHdr;
    UINT32 *                                        p32;
    UINT64 *                                        p64;
    UINT32                                          left;
    UINTN                                           totalTableSize;
    EFI_PHYSICAL_ADDRESS                            efiPhysAddr;
    UINTN                                           tableCount;
    UINT32                                          outAddr;
    EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *  pOutAcpi;
    EFI_ACPI_SDT_HEADER *                           pInputDt;
    EFI_ACPI_SDT_HEADER *                           pOutXsdt;
    UINT64 *                                        pOutTableAddr;
    UINT32                                          outIx;
    UINT32                                          virtMapAddr;
    UINT32                                          physMapAddr;
    UINT8                                           workSum;
    EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *     pFADT;
    BMP_IMAGE_HEADER *                              pBmp;

//    K2Printf(L"    ACPI.RSDP is at physical 0x%08X\n", gData.mpAcpi);
//    K2Printf(L"    ACPI.RSDP.XSDT = 0x%08X\n", (UINT32)gData.mpAcpi->XsdtAddress);
//    K2Printf(L"    ACPI.RSDP.RSDT = 0x%08X\n", gData.mpAcpi->RsdtAddress);

    totalTableSize = gData.mpAcpi->Length;
    totalTableSize = K2_ROUNDUP(totalTableSize, sizeof(UINT32));

    pFADT = NULL;
    if (gData.mpAcpi->XsdtAddress != 0)
    {
        //
        // 64-bit XSDT
        //
        pInputDt = (EFI_ACPI_SDT_HEADER *)((UINT32)gData.mpAcpi->XsdtAddress);
        pHdr = pInputDt;
        left = pHdr->Length;
        if (left < sizeof(EFI_ACPI_SDT_HEADER) + sizeof(UINT64))
        {
            K2Printf(L"***Xsdt is bad length.\n");
            return K2STAT_ERROR_UNKNOWN;
        }
        left -= sizeof(EFI_ACPI_SDT_HEADER);
        if ((left % sizeof(UINT64)) != 0)
        {
            K2Printf(L"***Xsdt content is not divisible by size of a 64-bit address.\n");
            return K2STAT_ERROR_UNKNOWN;
        }

        tableCount = left / sizeof(UINT64);

        totalTableSize += sizeof(EFI_ACPI_SDT_HEADER) + (tableCount * sizeof(UINT64));
        totalTableSize = K2_ROUNDUP(totalTableSize, sizeof(UINT32));

        p64 = (UINT64 *)(((UINT8 *)pHdr) + sizeof(EFI_ACPI_SDT_HEADER));
        for (outIx = 0; outIx < tableCount; outIx++)
        {
            pHdr = (EFI_ACPI_SDT_HEADER *)((UINT32)(*p64));
            if (pHdr->Signature == FADT_SIGNATURE)
            {
                if (pFADT != NULL)
                {
                    K2Printf(L"*** More than one FADT found.\n");
                    return K2STAT_ERROR_UNKNOWN;
                }
                pFADT = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE * )pHdr;
            }
            else if (pHdr->Signature == BGRT_SIGNATURE)
            {
                pBmp = (BMP_IMAGE_HEADER *)((UINT32)(((EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE *)pHdr)->ImageAddress));
                gData.LoadInfo.BootGraf.mBgrtBmpWidth = pBmp->PixelWidth;
                gData.LoadInfo.BootGraf.mBgrtBmpHeight = pBmp->PixelHeight;
            }
            p64++;
            totalTableSize += pHdr->Length;
            totalTableSize = K2_ROUNDUP(totalTableSize, sizeof(UINT32));
        }
    }
    else
    {
        //
        // 32-bit RSDT
        //
        if (gData.mpAcpi->RsdtAddress == 0)
        {
            K2Printf(L"***RsdtAddress and XsdtAddress are both zero.\n");
            return K2STAT_ERROR_UNKNOWN;
        }
        pInputDt = (EFI_ACPI_SDT_HEADER *)((UINT32)gData.mpAcpi->RsdtAddress);
        pHdr = pInputDt;
        left = pHdr->Length;
        if (left < sizeof(EFI_ACPI_SDT_HEADER) + sizeof(UINT32))
        {
            K2Printf(L"***Rsdt is bad length.\n");
            return K2STAT_ERROR_UNKNOWN;
        }
        left -= sizeof(EFI_ACPI_SDT_HEADER);
        if ((left % sizeof(UINT32)) != 0)
        {
            K2Printf(L"***Rsdt content is not divisible by size of a 32-bit address.\n");
            return K2STAT_ERROR_UNKNOWN;
        }

        tableCount = left / sizeof(UINT32);

        totalTableSize += sizeof(EFI_ACPI_SDT_HEADER) + (tableCount * sizeof(UINT64));
        totalTableSize = K2_ROUNDUP(totalTableSize, sizeof(UINT32));

        p32 = (UINT32 *)(((UINT8 *)pHdr) + sizeof(EFI_ACPI_SDT_HEADER));
        for (outIx = 0; outIx < tableCount; outIx++)
        {
            pHdr = (EFI_ACPI_SDT_HEADER *)((UINT32)(*p32));
            if (pHdr->Signature == FADT_SIGNATURE)
            {
                if (pFADT != NULL)
                {
                    K2Printf(L"*** More than one FADT found.\n");
                    return K2STAT_ERROR_UNKNOWN;
                }
                pFADT = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)pHdr;
            }
            else if (pHdr->Signature == BGRT_SIGNATURE)
            {
                pBmp = (BMP_IMAGE_HEADER *)((UINT32)(((EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE *)pHdr)->ImageAddress));
                gData.LoadInfo.BootGraf.mBgrtBmpWidth = pBmp->PixelWidth;
                gData.LoadInfo.BootGraf.mBgrtBmpHeight = pBmp->PixelHeight;
            }
            p32++;
            totalTableSize += pHdr->Length;
            totalTableSize = K2_ROUNDUP(totalTableSize, sizeof(UINT32));
        }
    }

    if (pFADT == NULL)
    {
        K2Printf(L"*** No FADT found in ACPI tables.\n");
        return K2STAT_ERROR_UNKNOWN;
    }

    //
    // check if the FACS exists
    //
    if (pFADT->FirmwareCtrl != 0)
    {
        pHdr = (EFI_ACPI_SDT_HEADER *)pFADT->FirmwareCtrl;
        if (pHdr->Signature != FACS_SIGNATURE)
        {
            K2Printf(L"*** table at FACS address does not have a valid signature.\n");
            return K2STAT_ERROR_UNKNOWN;
        }
        gData.LoadInfo.mFwFacsPhys = ((UINT32)pHdr) & K2_VA_PAGEFRAME_MASK;
        stat = K2VMAP32_MapPage(&gData.Map, K2OS_KVA_FACS_BASE, gData.LoadInfo.mFwFacsPhys, K2OS_MAPTYPE_KERN_DEVICEIO);
        if (K2STAT_IS_ERROR(stat))
        {
            K2Printf(L"*** MapPage for acpi FACS vaddr %08X failed\n", K2OS_KVA_FACS_BASE);
            return stat;
        }
    }
    else
    {
        gData.LoadInfo.mFwFacsPhys = 0;
    }

    //
    // check if the XFACS exists.  they can both exist and if they both do we need to map them both
    //
    if (pFADT->XFirmwareCtrl != 0)
    {
        pHdr = ((EFI_ACPI_SDT_HEADER *)(UINTN)pFADT->XFirmwareCtrl);
        if (pHdr->Signature != FACS_SIGNATURE)
        {
            K2Printf(L"*** table at xFACS address does not have a valid signature.\n");
            return K2STAT_ERROR_UNKNOWN;
        }
        gData.LoadInfo.mFwXFacsPhys = ((UINT32)pHdr) & K2_VA_PAGEFRAME_MASK;
        if (gData.LoadInfo.mFwXFacsPhys != gData.LoadInfo.mFwFacsPhys)
        {
            //
            // not on the same page as facs
            //
            stat = K2VMAP32_MapPage(&gData.Map, K2OS_KVA_XFACS_BASE, gData.LoadInfo.mFwXFacsPhys, K2OS_MAPTYPE_KERN_DEVICEIO);
            if (K2STAT_IS_ERROR(stat))
            {
                K2Printf(L"*** MapPage for acpi FACS vaddr %08X failed\n", K2OS_KVA_XFACS_BASE);
                return stat;
            }
        }
    }
    else
    {
        gData.LoadInfo.mFwXFacsPhys = 0;
    }

    //
    // space for DSDT and xDSDT
    //
    if (pFADT->Dsdt != 0)
    {
        pHdr = (EFI_ACPI_SDT_HEADER *)pFADT->Dsdt;
        if (pHdr->Signature != DSDT_SIGNATURE)
        {
            K2Printf(L"*** table at DSDT address does not have a valid signature.\n");
            return K2STAT_ERROR_UNKNOWN;
        }
        totalTableSize += pHdr->Length;
        totalTableSize = K2_ROUNDUP(totalTableSize, sizeof(UINT32));
    }
    else if (pFADT->XDsdt != 0)
    {
        pHdr = (EFI_ACPI_SDT_HEADER *)(UINTN)pFADT->XDsdt;
        if (pHdr->Signature != DSDT_SIGNATURE)
        {
            K2Printf(L"*** table at XDSDT address does not have a valid signature.\n");
            return K2STAT_ERROR_UNKNOWN;
        }
        totalTableSize += pHdr->Length;
        totalTableSize = K2_ROUNDUP(totalTableSize, sizeof(UINT32));
    }
    else
    {
        K2Printf(L"*** No DSDT or xDSDT\n");
        return K2STAT_ERROR_UNKNOWN;
    }

    //
    // now we can build our master table in one contiguous block of pages
    //
    gData.LoadInfo.mFwTabPageCount = K2_ROUNDUP(totalTableSize, K2_VA_MEMPAGE_BYTES) / K2_VA_MEMPAGE_BYTES;

    efiStatus = gBS->AllocatePages(
        AllocateAnyPages,
        K2OS_EFIMEMTYPE_FW_TABLES,
        gData.LoadInfo.mFwTabPageCount,
        &efiPhysAddr);
    if (EFI_ERROR(efiStatus))
    {
        K2Printf(L"*** Could not allocate %d bytes for ACPI/SMBIOS tables\n", totalTableSize);
        return K2STAT_ERROR_UNKNOWN;
    }
    gData.LoadInfo.mFwTabPagesPhys = (UINT32)efiPhysAddr;
    outAddr = gData.LoadInfo.mFwTabPagesPhys;
    pOutAcpi = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)outAddr;
    K2MEM_Copy(pOutAcpi, gData.mpAcpi, gData.mpAcpi->Length);
    outAddr += gData.mpAcpi->Length;
    outAddr = K2_ROUNDUP(outAddr, sizeof(UINT32));

    pOutAcpi->Checksum = 0;
    pOutAcpi->RsdtAddress = 0;
    pOutAcpi->XsdtAddress = (UINT64)outAddr;
    pOutAcpi->ExtendedChecksum = 0;
    pOutXsdt = (EFI_ACPI_SDT_HEADER *)outAddr;
    K2MEM_Copy(pOutXsdt, pInputDt, sizeof(EFI_ACPI_SDT_HEADER));
    pOutXsdt->Length = sizeof(EFI_ACPI_SDT_HEADER) + (tableCount * sizeof(UINT64));
    pOutXsdt->Checksum = 0;
    outAddr += sizeof(EFI_ACPI_SDT_HEADER);
    pOutTableAddr = (UINT64 *)outAddr;
    outAddr += tableCount * sizeof(UINT64);
    outIx = 0;

    pFADT = NULL;
    if (gData.mpAcpi->XsdtAddress != 0)
    {
        //
        // 64-bit XSDT
        //
        p64 = (UINT64 *)(((UINT8 *)pInputDt) + sizeof(EFI_ACPI_SDT_HEADER));
        for (outIx = 0; outIx < tableCount; outIx++)
        {
            pHdr = (EFI_ACPI_SDT_HEADER *)((UINT32)(*p64));
            pOutTableAddr[outIx] = (UINT64)outAddr;
            K2MEM_Copy((UINT8 *)outAddr, pHdr, pHdr->Length);
            if (pHdr->Signature == FADT_SIGNATURE)
                pFADT = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)outAddr;
//            K2Printf(L"    %c%c%c%c from 0x%08X to 0x%08X for 0x%08X\n",
//                pHdr->Signature & 0xFF,
//                (pHdr->Signature >> 8) & 0xFF,
//                (pHdr->Signature >> 16) & 0xFF,
//                (pHdr->Signature >> 24) & 0xFF,
//                pHdr, outAddr, pHdr->Length);
            p64++;
            outAddr += pHdr->Length;
            outAddr = K2_ROUNDUP(outAddr, sizeof(UINT32));
        }
    }
    else
    {
        //
        // 32-bit RSDT
        //
        p32 = (UINT32 *)(((UINT8 *)pInputDt) + sizeof(EFI_ACPI_SDT_HEADER));
        for (outIx = 0; outIx < tableCount; outIx++)
        {
            pHdr = (EFI_ACPI_SDT_HEADER *)((UINT32)(*p32));
            pOutTableAddr[outIx] = (UINT64)outAddr;
            K2MEM_Copy((UINT8 *)outAddr, pHdr, pHdr->Length);
            if (pHdr->Signature == FADT_SIGNATURE)
                pFADT = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)outAddr;
//            K2Printf(L"    %c%c%c%c from 0x%08X to 0x%08X for 0x%08X\n",
//                pHdr->Signature & 0xFF,
//                (pHdr->Signature >> 8) & 0xFF,
//                (pHdr->Signature >> 16) & 0xFF,
//                (pHdr->Signature >> 24) & 0xFF,
//                pHdr, outAddr, pHdr->Length);
            p32++;
            outAddr += pHdr->Length;
            outAddr = K2_ROUNDUP(outAddr, sizeof(UINT32));
        }
    }

    if (pFADT->Dsdt != 0)
    {
        pHdr = (EFI_ACPI_SDT_HEADER *)pFADT->Dsdt;
        K2MEM_Copy((UINT8 *)outAddr, pHdr, pHdr->Length);
//        K2Printf(L"    %c%c%c%c from 0x%08X to 0x%08X for 0x%08X\n",
//            pHdr->Signature & 0xFF,
//            (pHdr->Signature >> 8) & 0xFF,
//            (pHdr->Signature >> 16) & 0xFF,
//            (pHdr->Signature >> 24) & 0xFF,
//            pHdr, outAddr, pHdr->Length);
        pFADT->Dsdt = outAddr;
        pFADT->XDsdt = 0;
        outAddr += pHdr->Length;
        outAddr = K2_ROUNDUP(outAddr, sizeof(UINT32));
    } else if (pFADT->XDsdt != 0)
    {
        pHdr = (EFI_ACPI_SDT_HEADER *)(UINTN)pFADT->XDsdt;
        K2MEM_Copy((UINT8 *)outAddr, pHdr, pHdr->Length);
        //        K2Printf(L"    %c%c%c%c from 0x%08X to 0x%08X for 0x%08X\n",
        //            pHdr->Signature & 0xFF,
        //            (pHdr->Signature >> 8) & 0xFF,
        //            (pHdr->Signature >> 16) & 0xFF,
        //            (pHdr->Signature >> 24) & 0xFF,
        //            pHdr, outAddr, pHdr->Length);
        pFADT->Dsdt = 0;
        pFADT->XDsdt = outAddr;
        outAddr += pHdr->Length;
        outAddr = K2_ROUNDUP(outAddr, sizeof(UINT32));
    }

    if ((outAddr - gData.LoadInfo.mFwTabPagesPhys) != totalTableSize)
    {
        K2Printf(L"*** Internal error assembling ACPI\n");
        return K2STAT_ERROR_UNKNOWN;
    }

    //
    // ACPI tables have been assembled into an RSDP+XSDT+TABLES in a contiguous block of pages
    // so map that whole single chunk into the kernel space.
    //
    physMapAddr = gData.LoadInfo.mFwTabPagesPhys;
    virtMapAddr = gData.LoadInfo.mFwTabPagesVirt = gData.LoadInfo.mKernArenaLow;
    gData.LoadInfo.mKernArenaLow += gData.LoadInfo.mFwTabPageCount * K2_VA_MEMPAGE_BYTES;

    for (outIx = 0; outIx < gData.LoadInfo.mFwTabPageCount; outIx++)
    {
        //
        // ACPI data is read-only in the kernel
        //
        stat = K2VMAP32_MapPage(&gData.Map, virtMapAddr, physMapAddr, K2OS_MAPTYPE_KERN_READ);
        if (K2STAT_IS_ERROR(stat))
        {
            K2Printf(L"*** MapPage for acpi vaddr %08X failed\n", virtMapAddr);
            return stat;
        }
        virtMapAddr += K2_VA_MEMPAGE_BYTES;
        physMapAddr += K2_VA_MEMPAGE_BYTES;
    }

    //
    // we changed the RDST and XSDT entries in the RDSP
    //
    pOutAcpi->Checksum = 0;
    pOutAcpi->ExtendedChecksum = 0;
    workSum = AcpiTbChecksum((UINT8 *)pOutAcpi, 20);
    pOutAcpi->Checksum = (~workSum) + 1;
    ASSERT(0 == AcpiTbChecksum((UINT8 *)pOutAcpi, 20));
    workSum = AcpiTbChecksum((UINT8 *)pOutAcpi, pOutAcpi->Length);
    pOutAcpi->ExtendedChecksum = (~workSum) + 1;
    ASSERT(0 == AcpiTbChecksum((UINT8 *)pOutAcpi, pOutAcpi->Length));

    //
    // we changed the addresses of the sub-tables in the XSDT
    //
    pOutXsdt->Checksum = 0;
    workSum = AcpiTbChecksum((UINT8 *)pOutXsdt, pOutXsdt->Length);
    pOutXsdt->Checksum = (~workSum) + 1;
    ASSERT(0 == AcpiTbChecksum((UINT8 *)pOutXsdt, pOutXsdt->Length));

    //
    // we changed the address of the DSDT in the FADT
    //
    pFADT->Header.Checksum = 0;
    workSum = AcpiTbChecksum((UINT8 *)pFADT, pFADT->Header.Length);
    pFADT->Header.Checksum = (~workSum) + 1;
    ASSERT(0 == AcpiTbChecksum((UINT8 *)pFADT, pFADT->Header.Length));

    return 0;
}

