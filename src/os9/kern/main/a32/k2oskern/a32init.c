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

#include "a32kern.h"

K2OSKERN_SEQLOCK                gA32Kern_IntrSeqLock;

UINT64 *                        gpA32Kern_AcpiTablePtrs = NULL;
UINT32                          gA32Kern_AcpiTableCount = 0;

ACPI_MADT *                     gpA32Kern_MADT = NULL;
ACPI_MADT_SUB_GIC *             gpA32Kern_MADT_GICC[K2OS_MAX_CPU_COUNT];
ACPI_MADT_SUB_GIC_DISTRIBUTOR * gpA32Kern_MADT_GICD = NULL;

UINT32                          gA32Kern_IntrCount = A32KERN_MAX_IRQ;

UINT32                          gA32Kern_VectorPagePhys = 0;
K2OSKERN_PHYSTRACK *            gpA32Kern_StartTtbPhysTrack;
K2LIST_ANCHOR                   gA32Kern_StartAreaPhysList = { 0, 0 };

K2OS_PHYSADDR_RANGE             sgPhysInfo;

void
KernArch_GetPhysTable(
    UINT32 *                    apRetCount,
    K2OS_PHYSADDR_RANGE const **appRetTable
)
{
    *apRetCount = 1;
    *appRetTable = &sgPhysInfo;
}

void
KernArch_GetIoTable(
    UINT32 *                  apRetCount,
    K2OS_IOPORT_RANGE const **appRetTable
)
{
    *apRetCount = 0;
    *appRetTable = NULL;
}

void
KernArch_AtXdlEntry(
    void
)
{
    UINT32              virtBase;
    UINT32              physBase;
    ACPI_RSDP *         pAcpiRdsp;
    UINT32              workAddr;

    ACPI_DESC_HEADER *  pHeader;
    UINT64 *            pTabAddr;
    UINT32              workCount;

    UINT8 *             pScan;
    UINT32              left;
    ACPI_MADT_SUB_GIC * pSubGic;
    UINT32              physCBAR;

    virtBase = gData.mpShared->LoadInfo.mFwTabPagesVirt;
    physBase = gData.mpShared->LoadInfo.mFwTabPagesPhys;

    //
    // set up to scan xsdt
    //
    pAcpiRdsp = (ACPI_RSDP *)virtBase;
    workAddr = (UINT32)pAcpiRdsp->XsdtAddress;
    workAddr = workAddr - physBase + virtBase;

    pHeader = (ACPI_DESC_HEADER *)workAddr;
    gA32Kern_AcpiTableCount = pHeader->Length - sizeof(ACPI_DESC_HEADER);
    gA32Kern_AcpiTableCount /= sizeof(UINT64);
    K2_ASSERT(gA32Kern_AcpiTableCount > 0);
    gpA32Kern_AcpiTablePtrs = (UINT64 *)(workAddr + sizeof(ACPI_DESC_HEADER));

    //
    // find MADT
    //
    pTabAddr = gpA32Kern_AcpiTablePtrs;
    workCount = gA32Kern_AcpiTableCount;
    do {
        workAddr = (UINT32)(*pTabAddr);
        workAddr = workAddr - physBase + virtBase;
        pHeader = (ACPI_DESC_HEADER *)workAddr;
        if (pHeader->Signature == ACPI_MADT_SIGNATURE)
            break;
        pTabAddr++;
    } while (--workCount);
    K2_ASSERT(workCount > 0);

    gpA32Kern_MADT = (ACPI_MADT *)pHeader;

    //
    // scan for GICD and GICC
    //
    K2MEM_Zero(gpA32Kern_MADT_GICC, sizeof(ACPI_MADT_SUB_GIC *) * K2OS_MAX_CPU_COUNT);

    pScan = ((UINT8 *)gpA32Kern_MADT) + sizeof(ACPI_MADT);
    left = gpA32Kern_MADT->Header.Length - sizeof(ACPI_MADT);
    K2_ASSERT(left > 0);
    do {
        if (*pScan == ACPI_MADT_SUB_TYPE_GIC)
        {
            pSubGic = (ACPI_MADT_SUB_GIC *)pScan;
            K2_ASSERT(pSubGic->GicId < K2OS_MAX_CPU_COUNT);
            K2_ASSERT(gpA32Kern_MADT_GICC[pSubGic->GicId] == NULL);
            if (pSubGic->Flags & ACPI_MADT_SUB_GIC_FLAGS_ENABLED)
            {
                gpA32Kern_MADT_GICC[pSubGic->GicId] = pSubGic;
            }
        }
        else if (*pScan == ACPI_MADT_SUB_TYPE_GICD)
        {
            K2_ASSERT(gpA32Kern_MADT_GICD == NULL);
            gpA32Kern_MADT_GICD = (ACPI_MADT_SUB_GIC_DISTRIBUTOR *)pScan;
        }
        K2_ASSERT(left >= pScan[1]);
        left -= pScan[1];
        pScan += pScan[1];
    } while (left > 0);

    K2_ASSERT(gpA32Kern_MADT_GICD != NULL);
    K2_ASSERT(gpA32Kern_MADT_GICC[0] != NULL);

    //
    // try to get the Configuration Base Address Register 
    //
    physCBAR = A32_ReadCBAR() & A32_MPCORE_CBAR_LOW32_PERIPH_MASK;
    K2_ASSERT(0 != physCBAR);
    //
    // sanity check we match ACPI with the CBAR
    //
    physBase = (UINT32)gpA32Kern_MADT_GICC[0]->PhysicalBaseAddress;
    K2_ASSERT((physBase & K2_VA_MEMPAGE_OFFSET_MASK) + (A32KERN_MP_CONFIGBASE_MAP_VIRT) == A32KERN_MP_GICC_VIRT);

    physBase = (UINT32)gpA32Kern_MADT_GICD->PhysicalBaseAddress;
    K2_ASSERT((physBase & K2_VA_MEMPAGE_OFFSET_MASK) + (A32KERN_MP_CONFIGBASE_MAP_VIRT + K2_VA_MEMPAGE_BYTES) == A32KERN_MP_GICD_VIRT);

    gData.Timer.mIoPhys = physCBAR +
        A32_PERIPH_OFFSET_GLOBAL_TIMER +
        A32_PERIF_GTIMER_OFFSET_COUNTLOW;

    sgPhysInfo.mBaseAddr = A32_ReadCBAR() & A32_MPCORE_CBAR_LOW32_PERIPH_MASK;
    sgPhysInfo.mSizeBytes = A32_PERIPH_PAGES * K2_VA_MEMPAGE_BYTES;

    //
    // map the pages from A32KERN_CONFIGBASE_MAP_VIRT for A32KERN_CONFIGBASE_MAP_PAGES
    // this gives us the global timer, private timers, and SCU as well as the GICC and GICD
    //
    virtBase = A32KERN_MP_CONFIGBASE_MAP_VIRT;
    physBase = physCBAR;
    left = A32_PERIPH_PAGES;
    do {
        KernPte_MakePageMap(NULL, virtBase, physBase, K2OS_MAPTYPE_KERN_DEVICEIO);
        virtBase += K2_VA_MEMPAGE_BYTES;
        physBase += K2_VA_MEMPAGE_BYTES;
    } while (--left);

    K2OSKERN_SeqInit(&gA32Kern_IntrSeqLock);
    A32Kern_IntrInitGicDist();

    //
    // find BGRT and update kernel data table to point to it if it is there
    //
    virtBase = gData.mpShared->LoadInfo.mFwTabPagesVirt;
    physBase = gData.mpShared->LoadInfo.mFwTabPagesPhys;
    pTabAddr = gpA32Kern_AcpiTablePtrs;
    workCount = gA32Kern_AcpiTableCount;
    do {
        workAddr = (UINT32)(*pTabAddr);
        workAddr = workAddr - physBase + virtBase;
        pHeader = (ACPI_DESC_HEADER *)workAddr;
        if (pHeader->Signature == ACPI_BGRT_SIGNATURE)
            break;
        pTabAddr++;
    } while (--workCount);
    if (workCount != 0)
    {
        gData.BootGraf.mpBGRT = (ACPI_BGRT const *)pHeader;
    }

    //
    // now we can init the stall counter
    //
    A32Kern_InitTiming();
}
