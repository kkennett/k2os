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
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/ArmLib.h>
#include <Library/PcdLib.h>
#include <Library/IMX6/IMX6EpitLib.h>

#include <Protocol/Timer.h>
#include <Protocol/HardwareInterrupt.h>

// timer period is in 100ns units

// The notification function to call on every timer interrupt.
volatile EFI_TIMER_NOTIFY      mTimerNotifyFunction   = (EFI_TIMER_NOTIFY)NULL;
volatile EFI_PERIODIC_CALLBACK mTimerPeriodicCallback = (EFI_PERIODIC_CALLBACK)NULL;

// Cached copy of the Hardware Interrupt protocol instance
EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterrupt = NULL;

// Cached interrupt vector
static UINT32 sgTimerRegs;
static UINT32 sgTimerIrq;
static UINT64 sgCurrentTimerPeriod;

EFI_STATUS
EFIAPI
TimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  )
{
  if ((NotifyFunction == NULL) && (mTimerNotifyFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NotifyFunction != NULL) && (mTimerNotifyFunction != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  mTimerNotifyFunction = NotifyFunction;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod   // 100nS units.  so 10 = 1 microsecond
  )
{
  EFI_STATUS    Status;
  UINT32        TimerCount;
  UINT32        timerRate;
  UINT32        RegVal;

  // disable the timer and its ability to produce an interrupt
  RegVal = MmioRead32(sgTimerRegs + IMX6_EPIT_OFFSET_CR);
  RegVal &= ~(IMX6_EPIT_CR_EN | IMX6_EPIT_CR_OCIEN);
  MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_CR, RegVal);

  // if turning off disable interrupt source and clear the compare event flag
  if (TimerPeriod == 0) {
    Status = gInterrupt->DisableInterruptSource(gInterrupt, sgTimerIrq);
    TimerCount = 0;
	sgCurrentTimerPeriod = 0;
    MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_SR, IMX6_EPIT_SR_OCIF);
	return Status;
  } 

  sgCurrentTimerPeriod = TimerPeriod;

  // configure the timer source
  MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_CR,
        IMX6_EPIT_CR_CLKSRC_HIGHFREQ      |
        IMX6_EPIT_CR_OM_DISCONNECT);

  // get the EPIT rate from reading the clock settings, then set for divisor to hit 1Mhz
  timerRate = IMX6_EPIT_GetRate(IMX6_PHYSADDR_CCM, sgTimerRegs);
  timerRate /= 1000000; 
  ASSERT(timerRate > 0);

  // configure timer and let it generate interrupts
  MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_CR,
        IMX6_EPIT_CR_CLKSRC_HIGHFREQ      |
        IMX6_EPIT_CR_OM_DISCONNECT        |
        IMX6_EPIT_CR_STOPEN               |
        IMX6_EPIT_CR_WAITEN               |
        IMX6_EPIT_CR_DBGEN                |
        IMX6_EPIT_CR_IOVW                 |
        ((timerRate-1) << IMX6_EPIT_CR_PRESCALER_SHL) |
        IMX6_EPIT_CR_RLD                  |
        IMX6_EPIT_CR_OCIEN                |
        IMX6_EPIT_CR_ENMOD);

  // clear the compare event flag
  MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_SR, IMX6_EPIT_SR_OCIF);

  // TimerPeriod is in set the new compare value
  TimerPeriod /= 10;    // 10 per microsecond, so get period is microseconds
  if (TimerPeriod & 0xFFFFFFFF00000000)
      TimerCount = 0xFFFFFFFF;
  else
      TimerCount = (UINT32)TimerPeriod;

  MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_CMPR, TimerCount);
  MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_LR, TimerCount);
  Status = gInterrupt->EnableInterruptSource(gInterrupt, sgTimerIrq);

  // turn the timer on
  RegVal = MmioRead32(sgTimerRegs + IMX6_EPIT_OFFSET_CR);
  RegVal |= IMX6_EPIT_CR_EN;
  MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_CR, RegVal);

  return Status;
}

EFI_STATUS
EFIAPI
TimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                   *TimerPeriod
  )
{
  *TimerPeriod = sgCurrentTimerPeriod;
  return EFI_SUCCESS;
}

VOID
EFIAPI
TimerInterruptHandler (
  IN  HARDWARE_INTERRUPT_SOURCE   Source,
  IN  EFI_SYSTEM_CONTEXT          SystemContext       
  )
{
  EFI_TPL OriginalTPL;

  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  if (mTimerNotifyFunction) {
    mTimerNotifyFunction(sgCurrentTimerPeriod);
  }

  // Ack the timer event
  MmioWrite32(sgTimerRegs + IMX6_EPIT_OFFSET_SR, IMX6_EPIT_SR_OCIF);

  // EOI the interrupt
  gInterrupt->EndOfInterrupt(gInterrupt, sgTimerIrq);

  gBS->RestoreTPL (OriginalTPL);
}


EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  )
{
  return EFI_UNSUPPORTED;
}


EFI_TIMER_ARCH_PROTOCOL   gTimer = {
  TimerDriverRegisterHandler,
  TimerDriverSetTimerPeriod,
  TimerDriverGetTimerPeriod,
  TimerDriverGenerateSoftInterrupt
};

EFI_STATUS
EFIAPI
TimerInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_HANDLE  Handle = NULL;
  EFI_STATUS  Status;

  if (FixedPcdGet32(PcdTimerDxeUseEpit) == 1)
  {
      sgTimerRegs = IMX6_PHYSADDR_EPIT1;
      sgTimerIrq = IMX6_IRQ_EPIT1;
  }
  else
  {
      ASSERT(FixedPcdGet32(PcdTimerDxeUseEpit) == 2);
      sgTimerRegs = IMX6_PHYSADDR_EPIT2;
      sgTimerIrq = IMX6_IRQ_EPIT2;
  }

  // Find the interrupt controller protocol.  ASSERT if not found.
  Status = gBS->LocateProtocol(&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  ASSERT_EFI_ERROR (Status);

  // Disable the timer
  Status = TimerDriverSetTimerPeriod(&gTimer, 0);
  ASSERT_EFI_ERROR (Status);

  // Install interrupt handler
  Status = gInterrupt->RegisterInterruptSource(gInterrupt, sgTimerIrq, TimerInterruptHandler);
  ASSERT_EFI_ERROR (Status);

  // Set up default timer rate at 1Mhz
  Status = TimerDriverSetTimerPeriod(&gTimer, 100000);    // 100ns units, so 100000 = 100Hz (1 jiffy)
  ASSERT_EFI_ERROR (Status);

  // Install the Timer Architectural Protocol onto a new handle
  Status = gBS->InstallMultipleProtocolInterfaces(&Handle,
                                                  &gEfiTimerArchProtocolGuid,      &gTimer,
                                                  NULL);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

