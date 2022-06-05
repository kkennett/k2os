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

#include <Uefi.h>
#include <PiDxe.h>
#include <virtarm.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/VARealTimeAdapterLib.h>

#define PHYS_REGS_ADDR VIRTARM_PHYSADDR_ADAPTER_REGS(FixedPcdGet32(PcdRealTimeAdapterSlotNumber))

static UINT32 sgRegs = PHYS_REGS_ADDR;

EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME                *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities
  )
{
    VASYSTEMTIME    time;

    ASSERT(Time);

    VIRTARMTIME_GetRealTime((volatile VIRTARM_REALTIMEADAPTER_REGS *)sgRegs, &time);

    Time->Year = time.wYear;
    Time->Month = (UINT8)time.wMonth;
    Time->Day = (UINT8)time.wDay;
    Time->Hour = (UINT8)time.wHour;
    Time->Minute = (UINT8)time.wMinute;
    Time->Second = (UINT8)time.wSecond;
    Time->Nanosecond = ((UINT32)time.wMilliseconds) * 1000000;
    Time->TimeZone = 480;       // PST; -1440 to 1440 or 2047
    Time->Daylight = 0;

    if (Capabilities != NULL)
    {
        Capabilities->Resolution = 1000;
        Capabilities->Accuracy = 1000;
        Capabilities->SetsToZero = TRUE;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LibSetTime (
  IN EFI_TIME                *Time
  )
{
    // not supported
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LibGetWakeupTime (
  OUT BOOLEAN     *Enabled,
  OUT BOOLEAN     *Pending,
  OUT EFI_TIME    *Time
  )
{
    // not supported
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LibSetWakeupTime (
  IN BOOLEAN      Enabled,
  OUT EFI_TIME    *Time
  )
{
    // not supported
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LibRtcInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
    //
    // Init done in platsec. clock is running.
    //
    EFI_STATUS Status;

    // Declare the SNVS as EFI_MEMORY_RUNTIME, Mapped IO
    Status = gDS->AddMemorySpace(
        EfiGcdMemoryTypeMemoryMappedIo,
        PHYS_REGS_ADDR, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
        PHYS_REGS_ADDR, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    return EFI_SUCCESS;
}

VOID
EFIAPI
LibRtcVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
    //
    // Only needed if you are going to support the OS calling RTC functions in virtual mode.
    // You will need to call EfiConvertPointer (). To convert any stored physical addresses
    // to virtual address. After the OS transistions to calling in virtual mode, all future
    // runtime calls will be made in virtual mode.
    //
    EFI_STATUS Status;
    Status = gRT->ConvertPointer(0, (void **)&sgRegs);
    ASSERT_EFI_ERROR(Status);
}



