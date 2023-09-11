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

#include <Uefi.h>
#include <PiDxe.h>
#include <QemuQuad.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#define RATE_SECOND     32768ULL
#define RATE_MINUTE     (RATE_SECOND * 60ULL)
#define RATE_HOUR       (RATE_MINUTE * 60ULL)
#define RATE_DAY        (RATE_HOUR * 24ULL)
#define RATE_YEAR       (RATE_DAY * 365ULL)

static UINT16 const sgMonthDays[12] = 
{
    31,28,31,30,31,30,31,31,30,31,30,31
};

static UINT32   sgRegs_SNVS = IMX6_PHYSADDR_SNVS;

EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME                *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities
  )
{
    UINT32 timeLow1;
    UINT32 timeHigh1;
    UINT32 timeLow2;
    UINT32 timeHigh2;
    UINT64 timeVal;

    ASSERT(Time);

    timeLow1 = MmioRead32(sgRegs_SNVS + IMX6_SNVS_OFFSET_HPRTCLR);
    timeHigh1 = MmioRead32(sgRegs_SNVS + IMX6_SNVS_OFFSET_HPRTCMR);
    do
    {
        timeLow2 = MmioRead32(sgRegs_SNVS + IMX6_SNVS_OFFSET_HPRTCLR);
        timeHigh2 = MmioRead32(sgRegs_SNVS + IMX6_SNVS_OFFSET_HPRTCMR);
        if (timeHigh2 == timeHigh1)
        {
            if ((timeLow2 - timeLow1) < 32)
                break;
        }
        timeLow1 = timeLow2;
        timeHigh1 = timeHigh2;
    } while (TRUE);

    timeVal = (((UINT64)timeHigh2) << 32) | ((UINT64)timeLow2);

    timeLow1 = (UINT32)(timeVal / RATE_YEAR);
    timeVal -= (((UINT64)timeLow1) * RATE_YEAR);
    Time->Year = 2015 + timeLow1;          // 1900 – 9999

    ASSERT(Time->Year >= 2015);

    Time->Month = 1;            // 1 – 12
    timeLow1 = (UINT32)(timeVal / RATE_DAY);
    timeVal -= (((UINT64)timeLow1) * RATE_DAY);
    for (timeLow2 = 0;timeLow2 < 12;timeLow2++)
    {
        if (sgMonthDays[timeLow2] > timeLow1)
            break;
        timeLow1 -= sgMonthDays[timeLow2];
        Time->Month++;
    }

    ASSERT(Time->Month >= 1);
    ASSERT(Time->Month <= 12);

    Time->Day = timeLow2 + 1;

    ASSERT(Time->Day > 0);
    ASSERT(Time->Day <= sgMonthDays[Time->Month-1]);

    timeLow1 = (UINT32)(timeVal / RATE_HOUR);
    timeVal -= (((UINT64)timeLow1) * RATE_HOUR);
    Time->Hour = timeLow1;

    ASSERT(Time->Hour < 24);

    timeLow1 = (UINT32)(timeVal / RATE_MINUTE);
    timeVal -= (((UINT64)timeLow1) * RATE_MINUTE);
    Time->Minute = timeLow1;

    ASSERT(Time->Minute < 60);

    timeLow1 = (UINT32)(timeVal / RATE_SECOND);
    timeVal -= (((UINT64)timeLow1) * RATE_SECOND);
    Time->Second = timeLow1;

    ASSERT(Time->Second < 60);

    Time->Nanosecond = 0;       // 0 – 999,999,999
    Time->TimeZone = 480;       // PST; -1440 to 1440 or 2047
    Time->Daylight = 0;

    if (Capabilities != NULL)
    {
        Capabilities->Resolution = 32768;
        Capabilities->Accuracy = 1000000;
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

typedef struct {
    INT16           TimeZone;
    UINT8           Daylight;
} NON_VOLATILE_TIME_SETTINGS;
STATIC CONST CHAR16 sgTimeSettingsVariableName[] = L"RtcTimeSettings";

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
    EFI_STATUS                  Status;
    NON_VOLATILE_TIME_SETTINGS  TimeSettings;
    UINTN                       DataSize;

    // Declare the SNVS as EFI_MEMORY_RUNTIME, Mapped IO
    Status = gDS->AddMemorySpace(
        EfiGcdMemoryTypeMemoryMappedIo,
        QEMUQUAD_SNVS_PHYSADDR, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    Status = gDS->SetMemorySpaceAttributes(
        QEMUQUAD_SNVS_PHYSADDR, 0x1000,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
    ASSERT_EFI_ERROR(Status);

    //
    // if timezone persistent variable does not exist then make one up
    //
    DataSize = sizeof(TimeSettings);
    Status = EfiGetVariable((CHAR16*)sgTimeSettingsVariableName,
        &gEfiCallerIdGuid, NULL, &DataSize, (VOID*)&TimeSettings);
    if (EFI_ERROR(Status))
    {
        TimeSettings.TimeZone = EFI_UNSPECIFIED_TIMEZONE;
        TimeSettings.Daylight = 0;
        DataSize = sizeof(NON_VOLATILE_TIME_SETTINGS);
        Status = EfiSetVariable((CHAR16*)sgTimeSettingsVariableName,
            &gEfiCallerIdGuid, 
            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
            DataSize, (VOID*)&TimeSettings);
        if (EFI_ERROR(Status))
        {
            DEBUG((EFI_D_ERROR, "***BDS could not set missing %s variable\n", sgTimeSettingsVariableName));
        }
    }

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
    Status = gRT->ConvertPointer(0, (void **)&sgRegs_SNVS);
    ASSERT_EFI_ERROR(Status);
}



