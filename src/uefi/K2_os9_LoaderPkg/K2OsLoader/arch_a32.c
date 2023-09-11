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

#define ARM_PERIPH_OFFSET_GLOBAL_TIMER   0x0200

EFI_STATUS Loader_InitArch(void)
{
    UINTN                                   outIx;
    UINTN                                   tableCount;
    UINT32                                  matchAddr;
    EFI_ACPI_SDT_HEADER *                   pInputDt;
    EFI_ACPI_SDT_HEADER *                   pHdr;
    UINT32                                  left;
    UINT64 *                                p64;
    UINT32 *                                p32;
    ACPI_DESC_HEADER *                      pCSRT;
    ACPI_CSRT_RESOURCE_GROUP_HEADER *       pGroup;
    ACPI_CSRT_RESOURCE_DESCRIPTOR_HEADER *  pDesc;
    ACPI_CSRT_RD_TIMER *                    pTimer;

    //
    // get where the global timer should be so we can match with it in the CSRT table
    //
    matchAddr = ArmReadCbar() + ARM_PERIPH_OFFSET_GLOBAL_TIMER;

    //
    // find the CSRT in the ACPI tables and stop if its not there
    //
    pCSRT = NULL;
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
        p64 = (UINT64 *)(((UINT8 *)pHdr) + sizeof(EFI_ACPI_SDT_HEADER));
        for (outIx = 0; outIx < tableCount; outIx++)
        {
            pHdr = (EFI_ACPI_SDT_HEADER *)((UINT32)(*p64));
            if (pHdr->Signature == ACPI_CSRT_SIGNATURE)
            {
                if (pCSRT != NULL)
                {
                    K2Printf(L"*** More than one CSRT found.\n");
                    return K2STAT_ERROR_UNKNOWN;
                }
                pCSRT = (ACPI_DESC_HEADER *)pHdr;
            }
            p64++;
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
        p32 = (UINT32 *)(((UINT8 *)pHdr) + sizeof(EFI_ACPI_SDT_HEADER));
        for (outIx = 0; outIx < tableCount; outIx++)
        {
            pHdr = (EFI_ACPI_SDT_HEADER *)((UINT32)(*p32));
            if (pHdr->Signature == ACPI_CSRT_SIGNATURE)
            {
                if (pCSRT != NULL)
                {
                    K2Printf(L"*** More than one CSRT found.\n");
                    return K2STAT_ERROR_UNKNOWN;
                }
                pCSRT = (ACPI_DESC_HEADER *)pHdr;
            }
            p32++;
        }
    }

    if (NULL == pCSRT)
    {
        K2Printf(L"*** No CSRT found in ACPI tables\n");
        return EFI_NOT_FOUND;
    }

    left = pCSRT->Length - sizeof(ACPI_DESC_HEADER);
    pGroup = (ACPI_CSRT_RESOURCE_GROUP_HEADER *)(pCSRT + 1);
    do {
        if (left < pGroup->Length)
            break;
        tableCount = pGroup->Length - sizeof(ACPI_CSRT_RESOURCE_GROUP_HEADER);
        pDesc = (ACPI_CSRT_RESOURCE_DESCRIPTOR_HEADER *)(pGroup + 1);
        do {
            if ((pDesc->ResourceType == ACPI_CSRT_RD_TYPE_TIMER) &&
                (pDesc->ResourceSubType == ACPI_CSRT_RD_SUBTYPE_TIMER))
            {
                pTimer = (ACPI_CSRT_RD_TIMER *)pDesc;
                if (pTimer->BaseAddress == matchAddr)
                {
                    gData.LoadInfo.mArchTimerRate = pTimer->Frequency;
                    break;
                }
            }
            if (tableCount < pDesc->Length)
            {
                tableCount = 0;
            }
            else
            {
                tableCount -= pDesc->Length;
                pDesc = (ACPI_CSRT_RESOURCE_DESCRIPTOR_HEADER *)(((UINT8 *)pDesc) + pDesc->Length);
            }
        } while (0 != tableCount);
        left -= pGroup->Length;
        pGroup = (ACPI_CSRT_RESOURCE_GROUP_HEADER *)(((UINTN)pGroup) + pGroup->Length);
    } while (1);

    if (0 == gData.LoadInfo.mArchTimerRate)
    {
        K2Printf(L"*** ACPI tables did not declare ARM MP global timer at its CBAR+OFFSET address\n");
        return EFI_NOT_FOUND;
    }

    return EFI_SUCCESS;
}

void Loader_DoneArch(void)
{

}