#include "StoreRunDxe.h"

#define VAR_FIRST_LBA64 ((UINT64)(QEMUQUAD_PHYSSIZE_SEC_FD / SD_BLOCK_SIZE))
#define VAR_LBA_COUNT32 (QEMUQUAD_PHYSSIZE_VARSHADOW / SD_BLOCK_SIZE)

static BOOLEAN AtRuntime = FALSE;

EFI_STATUS
QemuQuadStoreRunDxe_BlockIo_ReadBlocks(
    IN EFI_BLOCK_IO_PROTOCOL          *This,
    IN UINT32                         MediaId,
    IN EFI_LBA                        Lba,
    IN UINTN                          BufferSize,
    OUT VOID                          *Buffer
)
{
    if ((This != &gpCardInst->Protocol) ||
        (MediaId != gpCardInst->Media.MediaId))
        return EFI_DEVICE_ERROR;

    return IMX6_USDHC_DoTransfer(&gpCardInst->Usdhc, FALSE, Lba, BufferSize, Buffer);
}

EFI_STATUS
QemuQuadStoreRunDxe_BlockIo_WriteBlocks(
    IN EFI_BLOCK_IO_PROTOCOL          *This,
    IN UINT32                         MediaId,
    IN EFI_LBA                        Lba,
    IN UINTN                          BufferSize,
    IN VOID                           *Buffer
)
{
    if ((This != &gpCardInst->Protocol) ||
        (MediaId != gpCardInst->Media.MediaId))
        return EFI_DEVICE_ERROR;

    if (gpCardInst->Media.ReadOnly)
        return EFI_WRITE_PROTECTED;

    return IMX6_USDHC_DoTransfer(&gpCardInst->Usdhc, TRUE, Lba, BufferSize, Buffer);
}

EFI_STATUS
QemuQuadStoreRunDxe_BlockIo_Flush(
    IN EFI_BLOCK_IO_PROTOCOL  *This
)
{
    if (This != &gpCardInst->Protocol)
        return EFI_DEVICE_ERROR;
    return EFI_SUCCESS;
}

EFI_STATUS
QemuQuadStoreRunDxe_BlockIo_Reset(
    IN EFI_BLOCK_IO_PROTOCOL  *This,
    IN BOOLEAN                  Flag
)
{
    // reset only happens once at init
    // so always succeed on this protocol after that
    if ((This != &gpCardInst->Protocol) || (!gpCardInst->Media.MediaPresent))
        return EFI_DEVICE_ERROR;
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
QemuQuadStoreRunDxe_Fvb_GetAttributes(
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
    OUT       EFI_FVB_ATTRIBUTES_2                *Attributes
)
{
    EFI_FVB_ATTRIBUTES_2 FvbAttributes;

    ASSERT(Attributes != NULL);

    if (This != &gpCardInst->Protocol2)
        return EFI_INVALID_PARAMETER;

    FvbAttributes = (EFI_FVB_ATTRIBUTES_2)(
        EFI_FVB2_READ_ENABLED_CAP | // Reads may be enabled
        EFI_FVB2_READ_STATUS |      // Reads are currently enabled
        EFI_FVB2_STICKY_WRITE |     // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
        EFI_FVB2_MEMORY_MAPPED |    // It is memory mapped
        EFI_FVB2_ERASE_POLARITY     // After erasure all bits take this value (i.e. '1')
        );

    if (!gpCardInst->Media.ReadOnly)
        FvbAttributes |=
        EFI_FVB2_WRITE_STATUS |     // Writes are currently enabled
        EFI_FVB2_WRITE_ENABLED_CAP; // Writes may be enabled

    *Attributes = FvbAttributes;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
QemuQuadStoreRunDxe_Fvb_SetAttributes(
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
    IN OUT    EFI_FVB_ATTRIBUTES_2                *Attributes
)
{
    if (This != &gpCardInst->Protocol2)
        return EFI_INVALID_PARAMETER;

    // not supported
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
QemuQuadStoreRunDxe_Fvb_GetPhysicalAddress(
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
    OUT       EFI_PHYSICAL_ADDRESS                *Address
)
{
    if (This != &gpCardInst->Protocol2)
        return EFI_INVALID_PARAMETER;

    ASSERT(Address != NULL);

    *Address = QEMUQUAD_PHYSADDR_VARSHADOW;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
QemuQuadStoreRunDxe_Fvb_GetBlockSize(
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
    IN        EFI_LBA                             Lba,
    OUT       UINTN                               *BlockSize,
    OUT       UINTN                               *NumberOfBlocks
)
{
    if (This != &gpCardInst->Protocol2)
        return EFI_INVALID_PARAMETER;

    if (Lba >= VAR_LBA_COUNT32)
        return EFI_INVALID_PARAMETER;

    *BlockSize = SD_BLOCK_SIZE;
    *NumberOfBlocks = ((UINTN)VAR_LBA_COUNT32) - Lba;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
QemuQuadStoreRunDxe_Fvb_Read(
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
    IN        EFI_LBA                             Lba,
    IN        UINTN                               Offset,
    IN OUT    UINTN                               *NumBytes,
    IN OUT    UINT8                               *Buffer
)
{
    UINT8 *     pSource;

    if ((This != &gpCardInst->Protocol2) ||
        (NumBytes == NULL) ||
        (Buffer == NULL))
        return EFI_INVALID_PARAMETER;

    if (Lba >= (EFI_LBA)VAR_LBA_COUNT32)
        return EFI_INVALID_PARAMETER;

    if ((Offset >= SD_BLOCK_SIZE) ||
        ((*NumBytes) == 0) ||
        ((*NumBytes) > SD_BLOCK_SIZE) ||
        (Offset + (*NumBytes) > SD_BLOCK_SIZE))
        return EFI_BAD_BUFFER_SIZE;

    pSource = (UINT8 *)gShadowBase;
    pSource += (Lba * SD_BLOCK_SIZE);

    CopyMem(Buffer, pSource + Offset, (*NumBytes));

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
QemuQuadStoreRunDxe_Fvb_Write(
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
    IN        EFI_LBA                             Lba,
    IN        UINTN                               Offset,
    IN OUT    UINTN                               *NumBytes,
    IN        UINT8                               *Buffer
)
{
    EFI_STATUS  stat;
    UINT8 *     pSource;
    UINT8 *     pTarget;

    if ((This != &gpCardInst->Protocol2) ||
        (NumBytes == NULL) ||
        (Buffer == NULL))
        return EFI_INVALID_PARAMETER;

    if (gpCardInst->Media.ReadOnly)
        return EFI_ACCESS_DENIED;

    if (Lba >= (EFI_LBA)VAR_LBA_COUNT32)
        return EFI_INVALID_PARAMETER;

    if ((Offset >= SD_BLOCK_SIZE) ||
        ((*NumBytes) == 0) ||
        ((*NumBytes) > SD_BLOCK_SIZE) ||
        (Offset + (*NumBytes) > SD_BLOCK_SIZE))
        return EFI_BAD_BUFFER_SIZE;

    pTarget = (UINT8 *)gShadowBase;
    pTarget += (Lba * SD_BLOCK_SIZE);

    if ((Offset != 0) ||
        ((((UINTN)Buffer) & 3) != 0) ||
        ((*NumBytes) != SD_BLOCK_SIZE))
    {
        if ((Offset != 0) || ((*NumBytes) != SD_BLOCK_SIZE))
            CopyMem(gpTempBuf, pTarget, SD_BLOCK_SIZE);
        CopyMem(gpTempBuf + Offset, Buffer, (*NumBytes));
        pSource = gpTempBuf;
    }
    else
    {
        pSource = Buffer;
    }

    stat = IMX6_USDHC_DoTransfer(&gpCardInst->Usdhc, TRUE, VAR_FIRST_LBA64 + Lba, SD_BLOCK_SIZE, pSource);
    if (!EFI_ERROR(stat))
    {
        // write committed to sd ok
        if (pSource != pTarget)
        {
            CopyMem(pTarget, pSource, SD_BLOCK_SIZE);
        }
    }

    return stat;
}

EFI_STATUS
EFIAPI
QemuQuadStoreRunDxe_Fvb_EraseBlocks(
    IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
    ...
)
{
    EFI_STATUS  Status;
    VA_LIST     Args;
    EFI_LBA     StartingLba;
    UINTN       NumOfLba;
    UINT8 *     pTarget;

    if (This != &gpCardInst->Protocol2)
        return EFI_INVALID_PARAMETER;

    if (gpCardInst->Media.ReadOnly)
        return EFI_ACCESS_DENIED;

    VA_START(Args, This);
    do
    {
        // Get the Lba from which we start erasing
        StartingLba = VA_ARG(Args, EFI_LBA);

        // Have we reached the end of the list?
        if (StartingLba == EFI_LBA_LIST_TERMINATOR)
            break;

        // How many Lba blocks are we requested to erase?
        NumOfLba = VA_ARG(Args, UINTN);

        // All blocks must be within range
    } while ((NumOfLba != 0) && ((StartingLba + NumOfLba) <= VAR_LBA_COUNT32));
    VA_END(Args);

    if (StartingLba != EFI_LBA_LIST_TERMINATOR)
        return EFI_INVALID_PARAMETER;

    ZeroMem(gpTempBuf, SD_BLOCK_SIZE);

    Status = EFI_SUCCESS;

    VA_START(Args, This);
    do
    {
        // Get the Lba from which we start erasing
        StartingLba = VA_ARG(Args, EFI_LBA);

        // Have we reached the end of the list?
        if (StartingLba == EFI_LBA_LIST_TERMINATOR)
            break;

        pTarget = (UINT8 *)gShadowBase;
        pTarget += (StartingLba * SD_BLOCK_SIZE);

        // How many Lba blocks are we requested to erase?
        NumOfLba = VA_ARG(Args, UINTN);

        do
        {
            Status = IMX6_USDHC_DoTransfer(&gpCardInst->Usdhc, TRUE, VAR_FIRST_LBA64 + StartingLba, SD_BLOCK_SIZE, gpTempBuf);
            
            if (EFI_ERROR(Status))
                break;

            ZeroMem(pTarget, SD_BLOCK_SIZE);
            pTarget += SD_BLOCK_SIZE;

            StartingLba++;
        } while (--NumOfLba);

    } while (TRUE);
    VA_END(Args);

    return Status;
}

EFI_STATUS
QemuQuadStoreRunDxe_Init(
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE *   SystemTable
)
{
    EFI_STATUS          stat;
    EFI_HOB_GUID_TYPE * GuidHob;

    ZeroMem(gpCardInst, sizeof(CARDINSTANCE));

    // Block IO Protocol
    gpCardInst->Protocol.Revision = EFI_BLOCK_IO_INTERFACE_REVISION;
    gpCardInst->Protocol.Media = &gpCardInst->Media;
    gpCardInst->Protocol.Reset = QemuQuadStoreRunDxe_BlockIo_Reset;
    gpCardInst->Protocol.ReadBlocks = QemuQuadStoreRunDxe_BlockIo_ReadBlocks;
    gpCardInst->Protocol.WriteBlocks = QemuQuadStoreRunDxe_BlockIo_WriteBlocks;
    gpCardInst->Protocol.FlushBlocks = QemuQuadStoreRunDxe_BlockIo_Flush;

    // Volume Block Protocol
    gpCardInst->Protocol2.GetAttributes = QemuQuadStoreRunDxe_Fvb_GetAttributes;
    gpCardInst->Protocol2.SetAttributes = QemuQuadStoreRunDxe_Fvb_SetAttributes;
    gpCardInst->Protocol2.GetPhysicalAddress = QemuQuadStoreRunDxe_Fvb_GetPhysicalAddress;
    gpCardInst->Protocol2.GetBlockSize = QemuQuadStoreRunDxe_Fvb_GetBlockSize;
    gpCardInst->Protocol2.Read = QemuQuadStoreRunDxe_Fvb_Read;
    gpCardInst->Protocol2.Write = QemuQuadStoreRunDxe_Fvb_Write;
    gpCardInst->Protocol2.EraseBlocks = QemuQuadStoreRunDxe_Fvb_EraseBlocks;
    gpCardInst->Protocol2.ParentHandle = NULL;

    //
    // pei should have left a HOB describing the boot card media here
    //
    GuidHob = (EFI_HOB_GUID_TYPE *)GetFirstGuidHob(&gQemuQuadPeiBootCardMediaGuid);
    ASSERT(NULL != GuidHob);
    
    CopyMem(&gpCardInst->Usdhc, (IMX6_USDHC *)(GuidHob + 1), sizeof(IMX6_USDHC));
    IMX6_GPT_Init(&gpCardInst->Gpt, IMX6_PHYSADDR_CCM, IMX6_PHYSADDR_GPT);
    gpCardInst->Usdhc.mpGpt = &gpCardInst->Gpt;

    gpCardInst->Media.BlockSize = gpCardInst->Usdhc.Info.BlockSize;
    gpCardInst->Media.IoAlign = 8;
    gpCardInst->Media.LastBlock = gpCardInst->Usdhc.Info.BlockCount - 1;
    gpCardInst->Media.LogicalBlocksPerPhysicalBlock = 1;
    gpCardInst->Media.LogicalPartition = FALSE;
    gpCardInst->Media.LowestAlignedLba = 0;
    gpCardInst->Media.MediaId = gpCardInst->Usdhc.Info.MediaId;
    gpCardInst->Media.MediaPresent = TRUE;
    gpCardInst->Media.OptimalTransferLengthGranularity = SD_BLOCK_SIZE;
    gpCardInst->Media.ReadOnly = FALSE;
    gpCardInst->Media.RemovableMedia = FALSE;
    gpCardInst->Media.WriteCaching = FALSE;

    // device path protocols for this instance
    gpCardInst->DevicePath.Vendor.Header.Type = HARDWARE_DEVICE_PATH;
    gpCardInst->DevicePath.Vendor.Header.SubType = HW_VENDOR_DP;
    gpCardInst->DevicePath.Vendor.Header.Length[0] = (UINT8)(SIZEOF_CARD_DEVICE_PATH & 0xFF);
    gpCardInst->DevicePath.Vendor.Header.Length[1] = (UINT8)((SIZEOF_CARD_DEVICE_PATH >> 8) & 0xFF);

    CopyMem(&gpCardInst->DevicePath.Vendor.Guid, &gQemuQuadTokenSpaceGuid, sizeof(GUID));

    gpCardInst->DevicePath.Signature = SIGNATURE_32('s', 'd', 'c', '0');
    gpCardInst->DevicePath.End.Type = END_DEVICE_PATH_TYPE;
    gpCardInst->DevicePath.End.SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
    gpCardInst->DevicePath.End.Length[0] = sizeof(gpCardInst->DevicePath.End);
    gpCardInst->DevicePath.End.Length[1] = 0;

    // Install
    stat = gBS->InstallMultipleProtocolInterfaces(&ImageHandle,
        &gEfiBlockIoProtocolGuid, &gpCardInst->Protocol,
        &gEfiDevicePathProtocolGuid, &gpCardInst->DevicePath,
        &gEfiFirmwareVolumeBlock2ProtocolGuid, &gpCardInst->Protocol2,
        NULL);

    ASSERT_EFI_ERROR(stat);

    return stat;
}

void
QemuQuadStoreRunDxe_Update(
    void
)
{
    EFI_STATUS Status;

    AtRuntime = TRUE;

    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Usdhc.mpGpt->mRegs_GPT);
    ASSERT_EFI_ERROR(Status);
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Usdhc.mpGpt);
    ASSERT_EFI_ERROR(Status);
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Usdhc.mRegs_CCM);
    ASSERT_EFI_ERROR(Status);
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Usdhc.mRegs_USDHC);
    ASSERT_EFI_ERROR(Status);

    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol.Media);
    ASSERT_EFI_ERROR(Status);
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol.Reset);
    ASSERT_EFI_ERROR(Status);
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol.ReadBlocks);
    ASSERT_EFI_ERROR(Status);
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol.WriteBlocks);
    ASSERT_EFI_ERROR(Status);
    Status = gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol.FlushBlocks);
    ASSERT_EFI_ERROR(Status);

    //
    // variable store may have already converted these pointers
    // as it may have already done its runtime update logic.
    // so we do not bother checking the results from the convert attempts
    // because they will have failed with NOT_FOUND
    //
    gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol2.GetAttributes);
    gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol2.SetAttributes);
    gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol2.GetPhysicalAddress);
    gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol2.GetBlockSize);
    gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol2.Read);
    gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol2.Write);
    gRT->ConvertPointer(0, (void **)&gpCardInst->Protocol2.EraseBlocks);
}
