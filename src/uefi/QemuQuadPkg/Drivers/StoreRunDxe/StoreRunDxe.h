#ifndef __STORERUNDXE_H
#define __STORERUNDXE_H

#include <Uefi.h>

#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiHob.h>

#include <QemuQuad.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/IMX6/IMX6GptLib.h>
#include <Library/IMX6/IMX6UsdhcLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/DevicePath.h>

#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>


typedef struct {
    VENDOR_DEVICE_PATH  Vendor;
    UINT32              Signature;
    EFI_DEVICE_PATH     End;
} CARD_DEVICE_PATH;

#define SIZEOF_CARD_DEVICE_PATH     (sizeof(CARD_DEVICE_PATH) - sizeof(EFI_DEVICE_PATH))

typedef struct _CARDINSTANCE
{
    EFI_BLOCK_IO_PROTOCOL               Protocol;       // must be first thing in structure (&Protocol == &CARDINSTANCE)
    EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL Protocol2;
    EFI_BLOCK_IO_MEDIA                  Media;
    CARD_DEVICE_PATH                    DevicePath;
    IMX6_GPT                            Gpt;
    IMX6_USDHC                          Usdhc;
} CARDINSTANCE;

extern UINTN            gShadowBase;
extern UINT8 *          gpTempBuf;
extern CARDINSTANCE *   gpCardInst;

EFI_STATUS  QemuQuadStoreRunDxe_Init(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void        QemuQuadStoreRunDxe_Update(void);

#endif // __STORERUNDXE_H