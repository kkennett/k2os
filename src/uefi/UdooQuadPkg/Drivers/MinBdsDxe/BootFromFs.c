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
#include "MinBdsDxe.h"

static BOOLEAN sgFirstTime = TRUE;

void
RunFsFile(
    EFI_HANDLE                          FsProtHandle,
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *   FsProt,
    CHAR16 const *                      FilePath
)
{
    EFI_STATUS                  Status;
    EFI_FILE_PROTOCOL *         pRootDir;
    EFI_FILE_PROTOCOL *         pApp;
    UINTN                       Size;
    EFI_FILE_INFO *             FileInfo;
    EFI_DEVICE_PATH_PROTOCOL *  FsDevicePath;
    CHAR16 *                    pPath;
    EFI_HANDLE                  ImageHandle;
    UINTN                       ExitDataSize;
    CHAR16 *                    ExitData;

    Status = FsProt->OpenVolume(FsProt, &pRootDir);
    if (Status != EFI_SUCCESS)
        return;

    if ((FilePath[0] == L'\\') || (FilePath[0] == '/'))
        FilePath++;

    Size = 0;
 
    do
    {
        Status = pRootDir->Open(pRootDir, &pApp, (CHAR16 *)FilePath, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status))
            break;

        do
        {
            Status = pApp->GetInfo(pApp, &gEfiFileInfoGuid, &Size, NULL);
            if (Status == EFI_BUFFER_TOO_SMALL)
                Status = EFI_SUCCESS;
            if (EFI_ERROR(Status) || (Size == 0))
                break;

            FileInfo = AllocatePool(Size);
            if (FileInfo == NULL)
            {
                Size = 0;
                break;
            }

            Status = pApp->GetInfo(pApp, &gEfiFileInfoGuid, &Size, FileInfo);
            if (!EFI_ERROR(Status))
                Size = (UINTN)FileInfo->FileSize;

            FreePool(FileInfo);

        } while (0);

        pApp->Close(pApp);

    } while (0);

    pRootDir->Close(pRootDir);

    if (Size == 0)
        return;

    //
    // if we get here, the file should be loadable
    // so get the device path of the file 
    // then let the image loader load it
    //
    FsDevicePath = FileDevicePath(FsProtHandle, FilePath);
    if (FsDevicePath == NULL)
    {
        DebugPrint(0xFFFFFFFF, "Could not create device path for target file result = %r\n", Status);
        return;
    }

    pPath = ConvertDevicePathToText(FsDevicePath, TRUE, FALSE);
    if (pPath != NULL)
    {
        DebugPrint(0xFFFFFFFF, "PATH = %s\n", pPath);
        FreePool(pPath);
    }
    
    ImageHandle = NULL;
        
    Status = gBS->LoadImage(TRUE, MinBdsImageHandle, FsDevicePath, NULL, 0, &ImageHandle);

    if (!EFI_ERROR(Status))
    {
        DebugPrint(0xFFFFFFFF, "Image loaded ok\n");
        ExitDataSize = 0;
        ExitData = NULL;

        if (sgFirstTime)
        {
            sgFirstTime = FALSE;
            EfiSignalEventReadyToBoot();
        }

        Status = gBS->StartImage(ImageHandle, &ExitDataSize, &ExitData);

        if ((ExitDataSize > 0) && (ExitData != NULL))
            FreePool(ExitData);

        gBS->UnloadImage(ImageHandle);

        DebugPrint(0xFFFFFFFF, "Image returned with result %d (%08X) %r\n", Status, Status, Status);
    }
    else
    {
        DebugPrint(0xFFFFFFFF, "Image load error result %r\n", Status);
    }

    FreePool(FsDevicePath);
}

