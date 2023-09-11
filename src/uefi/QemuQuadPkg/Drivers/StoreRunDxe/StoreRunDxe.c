#include "StoreRunDxe.h"

STATIC EFI_EVENT    gStoreVirtualAddrChangeEvent;

static UINT8        sgTempBuf[SD_BLOCK_SIZE * 2];
static CARDINSTANCE sgCardInst;

UINTN               gShadowBase = QEMUQUAD_PHYSADDR_VARSHADOW;
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

    QemuQuadStoreRunDxe_Update();

    Status = gRT->ConvertPointer(0, (void **)&gShadowBase);
    ASSERT_EFI_ERROR(Status);
    
    Status = gRT->ConvertPointer(0, (void **)&gpTempBuf);
    ASSERT_EFI_ERROR(Status);
    
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst);
    ASSERT_EFI_ERROR(Status);

}

EFI_STATUS
QemuQuadStoreRunDxeInit(
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
        gShadowBase, QEMUQUAD_PHYSSIZE_VARSHADOW,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
        gShadowBase, QEMUQUAD_PHYSSIZE_VARSHADOW,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    //
    // init first so gpCardInstance is set up properly
    //
    Status = QemuQuadStoreRunDxe_Init(ImageHandle, SystemTable);
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
