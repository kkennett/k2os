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
#include "StoreRunDxe.h"

STATIC EFI_EVENT    gStoreVirtualAddrChangeEvent;

static UINT8        sgTempBuf[SD_BLOCK_SIZE * 2];
static CARDINSTANCE sgCardInst;

UINTN               gShadowBase = UDOO_PHYSADDR_VARSHADOW;
UINT8 *             gpTempBuf = sgTempBuf;
CARDINSTANCE *      gpCardInst = &sgCardInst;

VOID
EFIAPI
StoreRunDxeNotifyAddressChange(
    IN EFI_EVENT        Event,
    IN VOID             *Context
)
{
    EFI_STATUS Status;

    UdooStoreRunDxe_Update();

    Status = gRT->ConvertPointer(0, (void **)&gShadowBase);
    ASSERT_EFI_ERROR(Status);
    
    Status = gRT->ConvertPointer(0, (void **)&gpTempBuf);
    ASSERT_EFI_ERROR(Status);
    
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst);
    ASSERT_EFI_ERROR(Status);

}

EFI_STATUS
UdooStoreRunDxeInit(
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE *   SystemTable
    )
{
    EFI_STATUS Status;

    //
    // we will need the shadow region at runtime, configured as uncached
    //
    Status = gDS->AddMemorySpace(
        EfiGcdMemoryTypeReserved,
        gShadowBase, UDOO_PHYSSIZE_VARSHADOW,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
        gShadowBase, UDOO_PHYSSIZE_VARSHADOW,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    //
    // init first so gpCardInstance is set up properly
    //
    Status = UdooStoreRunDxe_Init(ImageHandle, SystemTable);
    ASSERT_EFI_ERROR(Status);

    // align temp buffer in memory to SD_BLOCK_SIZE boundary
    gpTempBuf = (UINT8 *)(((((UINTN)gpTempBuf) + (SD_BLOCK_SIZE - 1)) / SD_BLOCK_SIZE) * SD_BLOCK_SIZE);

    //
    // we need access to the GPT registers
    //
    Status = gDS->AddMemorySpace(
        EfiGcdMemoryTypeMemoryMappedIo,
        gpCardInst->Usdhc.mpGpt->mRegs_GPT, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
        gpCardInst->Usdhc.mpGpt->mRegs_GPT, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    //
    // we need access to the usdhc registers
    //
    Status = gDS->AddMemorySpace(
        EfiGcdMemoryTypeMemoryMappedIo,
        gpCardInst->Usdhc.mRegs_USDHC, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
        gpCardInst->Usdhc.mRegs_USDHC, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    //
    // we need access to the ccm registers
    //
    Status = gDS->AddMemorySpace(
        EfiGcdMemoryTypeMemoryMappedIo,
        gpCardInst->Usdhc.mRegs_CCM, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
        gpCardInst->Usdhc.mRegs_CCM, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    //
    // let us know when os launches
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    StoreRunDxeNotifyAddressChange,
                    NULL,
                    &gEfiEventVirtualAddressChangeGuid,
                    &gStoreVirtualAddrChangeEvent
                    );
    ASSERT_EFI_ERROR (Status);

    return Status;
}
