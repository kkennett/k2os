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

LOADER_DATA gData;

#if K2_TARGET_ARCH_IS_ARM
static CHAR16 const * const sgpKernSubDirName = L"k2os\\a32\\kern";
#else
static CHAR16 const * const sgpKernSubDirName = L"k2os\\x32\\kern";
#endif

static K2DLXSUPP_HOST sgDlxHost =
{
    sizeof(K2DLXSUPP_HOST),

    sysDLX_CritSec,
    NULL,
    sysDLX_Open,
    NULL,
    sysDLX_ReadSectors,
    sysDLX_Prepare,
    sysDLX_PreCallback,
    sysDLX_PostCallback,
    sysDLX_Finalize,
    NULL,
    sysDLX_Purge,
    NULL
};


static
EFI_STATUS
sFindSmbiosAndAcpi(
    void
)
{
    EFI_STATUS status;

    status = EfiGetSystemConfigurationTable(&gEfiSmbiosTableGuid, (VOID**)&gData.mpSmbios);
    if (EFI_ERROR(status))
    {
        K2Printf(L"***Could not locate SMBIOS in Configuration Tables (%r)\n", status);
        return status;
    }
    if (gData.mpSmbios->Type0 == NULL)
    {
        K2Printf(L"***SMBIOS Type0 is NULL\n");
        return EFI_NOT_FOUND;
    }
    if (gData.mpSmbios->Type1 == NULL)
    {
        K2Printf(L"***SMBIOS Type1 is NULL\n");
        return EFI_NOT_FOUND;
    }
    if (gData.mpSmbios->Type2 == NULL)
    {
        K2Printf(L"***SMBIOS Type2 is NULL\n");
        return EFI_NOT_FOUND;
    }

    status = EfiGetSystemConfigurationTable(&gEfiAcpiTableGuid, (VOID **)&gData.mpAcpi);
    if (EFI_ERROR(status))
    {
        K2Printf(L"***Could not locate ACPI in Configuration Tables (%r)\n", status);
        return status;
    }

    if ((gData.mpAcpi->XsdtAddress == 0) && (gData.mpAcpi->RsdtAddress == 0))
    {
        K2Printf(L"***ACPI Xsdt and Rsdp addresses are zero.\n");
        return EFI_NOT_FOUND;
    }

    //    K2Printf(L"ACPI OEM ID:  %c%c%c%c%c%c\n",
    //        gData.mpAcpi->OemId[0], gData.mpAcpi->OemId[1], gData.mpAcpi->OemId[2],
    //        gData.mpAcpi->OemId[3], gData.mpAcpi->OemId[4], gData.mpAcpi->OemId[5]);

    return EFI_SUCCESS;
}

static
VOID
EFIAPI
sAddressChangeEvent(
    IN EFI_EVENT  Event,
    IN VOID *     Context
)
{
    EFI_STATUS Status;
    Status = gRT->ConvertPointer(0x0, (VOID **)&gData.LoadInfo.mpEFIST);
    ASSERT_EFI_ERROR(Status);
}

static
BOOLEAN
sVerifyPlat(
    DLX * apPlatDlx
)
{
    K2STAT  stat;
    UINT32  funcAddr;

    stat = DLX_FindExport(
        apPlatDlx,
        DlxSeg_Text,
        "K2OSPLAT_Init",
        &funcAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Required Plat export \"K2OSPLAT_Init\" missing\n");
        return FALSE;
    }

    stat = DLX_FindExport(
        apPlatDlx,
        DlxSeg_Text,
        "K2OSPLAT_DebugOut",
        &funcAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Required Plat export \"K2OSPLAT_DebugOut\" missing\n");
        return FALSE;
    }

    stat = DLX_FindExport(
        apPlatDlx,
        DlxSeg_Text,
        "K2OSPLAT_DebugIn",
        &funcAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Required Plat export \"K2OSPLAT_DebugIn\" missing\n");
        return FALSE;
    }

    stat = DLX_FindExport(
        apPlatDlx,
        DlxSeg_Text,
        "K2OSPLAT_ForcedDriverQuery",
        &funcAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Required Plat export \"K2OSPLAT_ForcedDriverQuery\" missing\n");
        return FALSE;
    }

    stat = DLX_FindExport(
        apPlatDlx,
        DlxSeg_Text,
        "K2OSPLAT_OnIrq",
        &funcAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Required Plat export \"K2OSPLAT_OnIrq\" missing\n");
        return FALSE;
    }

    stat = DLX_FindExport(
        apPlatDlx,
        DlxSeg_Text,
        "K2OSPLAT_GetResTable",
        &funcAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Required Plat export \"K2OSPLAT_GetResTable\" missing\n");
        return FALSE;
    }

    return TRUE;
}

EFI_STATUS
EFIAPI
K2OsLoaderEntryPoint(
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE *   SystemTable
)
{
    EFI_STATUS              efiStatus;
    EFI_EVENT               efiEvent;
    EFI_PHYSICAL_ADDRESS    physAddr;
    DLX *                   pDlxPlat;
    DLX *                   pDlxKern;
//    DLX *                   pDlxAcpi;
    K2STAT                  stat;

    K2Printf(L"\n\n\nK2Loader\n----------------\n");

    K2MEM_Zero(&gData, sizeof(gData));

    gData.mLoaderImageHandle = ImageHandle;
    gData.LoadInfo.mKernArenaLow = K2OS_KVA_KERNEL_ARENA_INIT_BOTTOM;
    gData.LoadInfo.mKernArenaHigh = K2OS_KVA_KERNEL_ARENA_INIT_TOP;
    gData.LoadInfo.mpEFIST = (K2EFI_SYSTEM_TABLE *)gST;

    efiStatus = Loader_InitArch();
    if (EFI_ERROR(efiStatus))
        return efiStatus;

    do
    {
        efiStatus = sFindSmbiosAndAcpi();
        if (EFI_ERROR(efiStatus))
        {
            K2Printf(L"*** Failed to validate SMBIOS/ACPI = %r\n\n\n", efiStatus);
            break;
        }

        efiStatus = Loader_CountCpus();
        if (EFI_ERROR(efiStatus))
        {
            K2Printf(L"*** Failed to count # of cpus (error = %r)\n\n\n", efiStatus);
            break;
        }

        efiStatus = gBS->CreateEventEx(
            EVT_NOTIFY_SIGNAL,
            TPL_NOTIFY,
            sAddressChangeEvent,
            NULL,
            &gEfiEventVirtualAddressChangeGuid,
            &efiEvent
        );
        if (efiStatus != EFI_SUCCESS)
        {
            K2Printf(L"*** Failed to allocate address change event with %r\n", efiStatus);
            break;
        }

        do {
            efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_EFI_MAP, K2OS_EFIMAP_PAGECOUNT, (EFI_PHYSICAL_ADDRESS *)&physAddr);
            if (efiStatus != EFI_SUCCESS)
            {
                K2Printf(L"*** Failed to allocate pages for EFI memory map, status %r\n", efiStatus);
                break;
            }

            gData.mpMemoryMap = (EFI_MEMORY_DESCRIPTOR *)(UINTN)physAddr;

            do {
                efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_DLX_LOADER, 1, &gData.mLoaderPagePhys);
                if (efiStatus != EFI_SUCCESS)
                {
                    K2Printf(L"*** Failed to allocate page for K2 loader with %r\n", efiStatus);
                    break;
                }

                efiStatus = sysDLX_Init(ImageHandle);
                if (efiStatus != EFI_SUCCESS)
                {
                    K2Printf(L"*** sysDLX_Init failed with %r\n", efiStatus);
                    break;
                }

                do {

                    K2_ASSERT(gData.mLoaderImageHandle == ImageHandle);

                    efiStatus = EFI_NOT_FOUND;

                    stat = K2DLXSUPP_Init((void *)((UINT32)gData.mLoaderPagePhys), &sgDlxHost, TRUE, FALSE, NULL);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2Printf(L"*** InitDLX returned status 0x%08X\n", stat);
                        break;
                    }

                    stat = DLX_Acquire("k2osplat.dlx", NULL, &pDlxPlat, NULL, NULL);
                    if (K2STAT_IS_ERROR(stat))
                        break;

                    do
                    {
                        if (!sVerifyPlat(pDlxPlat))
                            break;

//                        stat = DLX_Acquire("k2osacpi.dlx", NULL, &pDlxAcpi, NULL, NULL);
//                        if (K2STAT_IS_ERROR(stat))
//                            break;

                        do {
                            stat = DLX_Acquire("k2oskern.dlx", NULL, &pDlxKern, NULL, NULL);
                            if (K2STAT_IS_ERROR(stat))
                                break;

                            do
                            {
                                stat = DLX_Acquire("k2oscrt.dlx", NULL, &gData.LoadInfo.mpDlxCrt, NULL, NULL);
                                if (gData.LoadInfo.mpDlxCrt == NULL)
                                    break;

                                Loader_Boot(pDlxKern);
                                while (1);
                                gRT->ResetSystem(EfiResetCold, EFI_DEVICE_ERROR, 0, NULL);
                                while (1);

                                DLX_Release(gData.LoadInfo.mpDlxCrt);

                            } while (0);

                            DLX_Release(pDlxKern);

                        } while (0);

//                        DLX_Release(pDlxAcpi);

                    } while (0);

                    DLX_Release(pDlxPlat);

                } while (0);

                sysDLX_Done();

            } while (0);

            gBS->FreePages(gData.mLoaderPagePhys, 1);

        } while (0);

        gBS->FreePages((EFI_PHYSICAL_ADDRESS)((UINTN)gData.mpMemoryMap), K2OS_EFIMAP_PAGECOUNT);

    } while (0);

    gBS->CloseEvent(efiEvent);

    Loader_DoneArch();

    return efiStatus;
}
