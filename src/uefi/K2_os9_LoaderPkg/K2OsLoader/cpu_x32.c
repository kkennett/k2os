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
#include "K2OsLoader.h"

typedef struct {
    UINT8   Type;
    UINT8   Length;
} APIC_TABLE_ENTRY_HDR;

EFI_STATUS 
Loader_CountCpus(
    void
)
{
    EFI_ACPI_SDT_HEADER *pHdr;
    UINT32 *p32;
    UINT64 *p64;
    UINT32 left;
    EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *pMADT;
    APIC_TABLE_ENTRY_HDR *pTabEnt;
    EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_STRUCTURE *pProc;

    if (gData.mpAcpi->XsdtAddress & 0xFFFFFFFF00000000ull)
    {
        K2Printf(L"Xsdt is too high (>4G) for us to see.\n");
        return EFI_DEVICE_ERROR;
    }

    //
    // find the APIC ACPI table
    //

    if (gData.mpAcpi->XsdtAddress != 0)
    {
        //
        // 64-bit XSDT
        //
        pHdr = (EFI_ACPI_SDT_HEADER *)((UINT32)gData.mpAcpi->XsdtAddress);
        left = pHdr->Length;
        if (left < sizeof(EFI_ACPI_SDT_HEADER) + sizeof(UINT64))
        {
            K2Printf(L"***Xsdt is bad length.\n");
            return EFI_DEVICE_ERROR;
        }
        left -= sizeof(EFI_ACPI_SDT_HEADER);
        if ((left % sizeof(UINT64)) != 0)
        {
            K2Printf(L"***Xsdt content is not divisible by size of a 64-bit address.\n");
            return EFI_DEVICE_ERROR;
        }
        p64 = (UINT64 *)(((UINT8 *)pHdr) + sizeof(EFI_ACPI_SDT_HEADER));
        do {
            pMADT = (EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)((UINT32)(*p64));
            if (pMADT->Header.Signature == 0x43495041)
                break;
            pMADT = NULL;
            p64++;
            left -= sizeof(UINT64);
        } while (left > 0);
    }
    else
    {
        //
        // 32-bit RSDT
        //
        if (gData.mpAcpi->RsdtAddress == 0)
        {
            K2Printf(L"***RsdtAddress and XsdtAddress are both zero.\n");
            return EFI_DEVICE_ERROR;
        }
        pHdr = (EFI_ACPI_SDT_HEADER *)((UINT32)gData.mpAcpi->RsdtAddress);
        left = pHdr->Length;
        if (left < sizeof(EFI_ACPI_SDT_HEADER) + sizeof(UINT32))
        {
            K2Printf(L"***Rsdt is bad length.\n");
            return EFI_DEVICE_ERROR;
        }
        left -= sizeof(EFI_ACPI_SDT_HEADER);
        if ((left % sizeof(UINT32)) != 0)
        {
            K2Printf(L"***Rsdt content is not divisible by size of a 32-bit address.\n");
            return EFI_DEVICE_ERROR;
        }
        p32 = (UINT32 *)(((UINT8 *)pHdr) + sizeof(EFI_ACPI_SDT_HEADER));
        do {
            pMADT = (EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)(*p32);
            if (pMADT->Header.Signature == 0x43495041)
                break;
            pMADT = NULL;
            p32++;
            left -= sizeof(UINT32);
        } while (left > 0);
    }

    if (pMADT == NULL)
    {
        K2Printf(L"---No APIC table found. Assume CpuId 0\n");
        gData.LoadInfo.mCpuCoreCount++;
    }
    else
    {
        left = pMADT->Header.Length;
        if (left < sizeof(EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) + sizeof(APIC_TABLE_ENTRY_HDR))
        {
            K2Printf(L"*** MADT size is invalid (%d)\n", left);
        }

        gData.LoadInfo.mCpuCoreCount = 0;

        left -= sizeof(EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
        pTabEnt = (APIC_TABLE_ENTRY_HDR *)(((UINT8 *)pMADT) + sizeof(EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER));
        do {
            if (left < pTabEnt->Length)
            {
                K2Printf(L"*** Table error\n");
                return EFI_DEVICE_ERROR;
            }
            if (pTabEnt->Type == EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC)
            {
                pProc = (EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_STRUCTURE *)pTabEnt;
                if (pProc->Flags & EFI_ACPI_5_0_LOCAL_APIC_ENABLED)
                {
                    //
                    // processor is enabled
                    //
                    gData.LoadInfo.mCpuCoreCount++;
                    if (gData.LoadInfo.mCpuCoreCount == K2OS_MAX_CPU_COUNT)
                    {
                        K2Printf(L"!!! Hit max # of supported CPUs.\n");
                        break;
                    }
                }
            }

            left -= pTabEnt->Length;
            pTabEnt = (APIC_TABLE_ENTRY_HDR *)(((UINT8 *)pTabEnt) + pTabEnt->Length);
        } while (left > 0);
    }

    if (gData.LoadInfo.mCpuCoreCount == 0)
    {
        K2Printf(L"*** No APICs found in MADT - no processors are described\n");
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}
