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
#include "MinBdsDxe.h"

#define RUNBOOT 1

EFI_HANDLE MinBdsImageHandle = NULL;

static 
void
RunShell(
    void
)
{
    EFI_STATUS                          Status;
    EFI_LOADED_IMAGE_PROTOCOL *         pLoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *   pFileProt;

    DebugPrint(0xFFFFFFFF, "Run Shell\n");

    // find simple file systems on handle of device we were loaded from
    // this should be the same FV as the one we are in

    Status = gBS->OpenProtocol(MinBdsImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&pLoadedImage,
        MinBdsImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
    {
        DebugPrint(0xFFFFFFFF, "*** Get MinBdsDxe LoadedImage Protocol failed - %r\n", Status);
        return;
    }

    do
    {
        Status = gBS->OpenProtocol(pLoadedImage->DeviceHandle,
            &gEfiSimpleFileSystemProtocolGuid,
            (void **)&pFileProt,
            MinBdsImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if (EFI_ERROR(Status))
        {
            DebugPrint(0xFFFFFFFF, "*** Open File System on MinBds-hosting device handle failed with %r\n", Status);
            break;
        }

        RunFsFile(pLoadedImage->DeviceHandle, pFileProt, L"Shell.efi");

        gBS->CloseProtocol(pLoadedImage->DeviceHandle,
            &gEfiSimpleFileSystemProtocolGuid,
            MinBdsImageHandle,
            NULL);
        pFileProt = NULL;

    } while (0);

    gBS->CloseProtocol(MinBdsImageHandle,
        &gEfiLoadedImageProtocolGuid,
        MinBdsImageHandle,
        NULL);
    pLoadedImage = NULL;
}

#if RUNBOOT

static
void
BootFirstAvailable(void)
{
    EFI_STATUS                          Status;
    UINTN                               numHandles;
    UINTN                               ixHandle;
    EFI_HANDLE *                        pHandles;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *   Protocol;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &numHandles, &pHandles);
    if (EFI_ERROR(Status))
        return;
    if (numHandles == 0)
        return;
    DebugPrint(0xFFFFFFFF, "%d Simple File System Handles found\n", numHandles);
    ixHandle = 0;
    do
    {
        Status = gBS->HandleProtocol(pHandles[ixHandle], &gEfiSimpleFileSystemProtocolGuid, (void **)&Protocol);
        ASSERT_EFI_ERROR(Status);
        RunFsFile(pHandles[ixHandle], Protocol, EFI_REMOVABLE_MEDIA_FILE_NAME);
        ixHandle++;
    } while (--numHandles);
    FreePool(pHandles);

    DebugPrint(0xFFFFFFFF, "Nothing Bootable. Run Shell\n");
}

#endif

static
EFI_STATUS
ConnectConsole(
    void
)
{
    EFI_STATUS      Status;
    UINTN           numHandles;
    EFI_HANDLE *    pHandles;
    VOID *          Interface;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleTextOutProtocolGuid, NULL, &numHandles, &pHandles);
    if (EFI_ERROR(Status))
        return Status;
    if (numHandles == 0)
        return EFI_NOT_FOUND;
    Status = gBS->HandleProtocol(pHandles[0], &gEfiSimpleTextOutProtocolGuid, &Interface);
    ASSERT_EFI_ERROR(Status);

    gST->ConsoleOutHandle = pHandles[0];
    gST->StandardErrorHandle = pHandles[0];
    gST->ConOut = Interface;
    gST->StdErr = Interface;

    FreePool(pHandles);

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleTextInProtocolGuid, NULL, &numHandles, &pHandles);
    if (EFI_ERROR(Status))
        return Status;
    if (numHandles == 0)
        return EFI_NOT_FOUND;
    Status = gBS->HandleProtocol(pHandles[0], &gEfiSimpleTextInProtocolGuid, &Interface);
    ASSERT_EFI_ERROR(Status);

    gST->ConsoleInHandle = pHandles[0];
    gST->ConIn = Interface;

    FreePool(pHandles);

    gST->Hdr.CRC32 = 0;
    Status = gBS->CalculateCrc32((VOID*)gST, gST->Hdr.HeaderSize, &gST->Hdr.CRC32);
    ASSERT_EFI_ERROR(Status);

    return EFI_SUCCESS;
}

static
EFI_STATUS
BruteForceConnectAllDrivers(
    VOID
)
{
    UINTN           HandleCount, Index;
    EFI_HANDLE  *   HandleBuffer;
    EFI_STATUS      Status;
    EFI_EVENT       TimerEvent;
    UINTN           SignalledIndex;

    do
    {
        // Locate all the driver handles
        Status = gBS->LocateHandleBuffer(
            AllHandles,
            NULL,
            NULL,
            &HandleCount,
            &HandleBuffer
        );
        if (EFI_ERROR(Status))
            return Status;

        // Connect every handle
        for (Index = 0; Index < HandleCount; Index++)
        {
            gBS->ConnectController(HandleBuffer[Index], NULL, NULL, TRUE);
        }

        if (HandleBuffer != NULL)
        {
            FreePool(HandleBuffer);
        }

        // Check if new handles have been created after the start of the previous handles
        Status = gDS->Dispatch();

    } while (!EFI_ERROR(Status));

    //
    // give 2 seconds for USB stuff to enumerate asynchronously
    //
    DebugPrint(0xFFFFFFFF, "..Waiting 3 seconds to let USB enumerate..\n");
    Status = gBS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    ASSERT_EFI_ERROR(Status);
    Status = gBS->SetTimer(TimerEvent, TimerRelative, 10 * 1000 * 1000 * 3);
    ASSERT_EFI_ERROR(Status);
    Status = gBS->WaitForEvent(1, &TimerEvent, &SignalledIndex);
    ASSERT_EFI_ERROR(Status);
    gBS->CloseEvent(TimerEvent);

    return EFI_SUCCESS;
}

VOID
EFIAPI
EmptyCallbackFunction(
    IN EFI_EVENT                Event,
    IN VOID                     *Context
)
{
    return;
}

VOID
EFIAPI
BdsEntry(
    IN EFI_BDS_ARCH_PROTOCOL  *This
)
{
    EFI_STATUS  Status;
    EFI_EVENT   EndOfDxeEvent;

    Status = gBS->CreateEventEx(
        EVT_NOTIFY_SIGNAL,
        TPL_NOTIFY,
        EmptyCallbackFunction,
        NULL,
        &gEfiEndOfDxeEventGroupGuid,
        &EndOfDxeEvent
    );
    ASSERT_EFI_ERROR(Status);
    gBS->SignalEvent(EndOfDxeEvent);

    Status = BruteForceConnectAllDrivers();
    ASSERT_EFI_ERROR(Status);

    Status = ConnectConsole();
    ASSERT_EFI_ERROR(Status);

#if RUNBOOT
    BootFirstAvailable();
#endif

    do {
        RunShell();
        DebugPrint(0xFFFFFFFF, "Shell Exited. Starting it again.\n");
    } while (1);

    DebugPrint(0xFFFFFFFF, "CpuDeadLoop()\n");
    CpuDeadLoop();
}

static
EFI_BDS_ARCH_PROTOCOL  
gBds = {
  BdsEntry
};

EFI_STATUS
EFIAPI
BdsInitialize(
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS  Status;

    MinBdsImageHandle = ImageHandle;

    //
    // Install protocol interface
    //
    Status = gBS->InstallMultipleProtocolInterfaces(
        &MinBdsImageHandle,
        &gEfiBdsArchProtocolGuid, &gBds,
        NULL
    );
    ASSERT_EFI_ERROR(Status);

    return Status;
}

