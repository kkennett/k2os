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

static UINT64           sgTimeBias = 0;
static UINT64           sgTimerDiv = 0;
static K2OSKERN_SEQLOCK sgSeqLock;
static volatile UINT32  sgTimerCore = (UINT32)-1;

static
UINT32
sReadGlobTimer32(
    void
)
{
    return MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COUNTLOW);
}

static 
void 
sReadGlobTimer64(
    UINT64 *apRetTick
)
{
    UINT32 hi1;
    UINT32 hi2;
    UINT32 low;

    hi1 = MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COUNTHIGH);
    low = MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COUNTLOW);
    hi2 = MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COUNTHIGH);

    if (hi1 != hi2)
        low = MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COUNTLOW);

    *apRetTick = ((((UINT64)hi2) << 32) | ((UINT64)low));
}

void
A32Kern_InitTiming(
    void
)
{
    UINT32 reg;

    K2OSKERN_SeqInit(&sgSeqLock);

    //
    // turn off the timer
    //
    MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_CONTROL, 0);

    //
    // clear it and ack any interrupt
    //
    MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COUNTHIGH, 0);
    MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COUNTLOW, 1);
    do
    {
        reg = MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_INTSTATUS);
        if (0 == reg)
            break;
        MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_INTSTATUS, reg);
    } while (1);

    //
    // bootloader figured out global timer rate
    //
    gData.Timer.mFreq = gData.mpShared->LoadInfo.mArchTimerRate;
    K2_ASSERT(0 != gData.Timer.mFreq);

    // convert frequency to period in femptoseconds
    // then to a divisor pre-multiplied to a power of two.
    // this lets us use a shift << 30 instead of a 64-bit multiply in the stall below
    sgTimerDiv = gData.Timer.mFreq;
    sgTimerDiv = (1000000000000000ull / sgTimerDiv);
    sgTimerDiv = (sgTimerDiv * 1024 * 1024 * 1024) / 1000000000;

    //
    // enable the timer
    //
    MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_CONTROL,
        A32_PERIF_GTIMER_CONTROL_ENABLE);

    //
    // irq is a ppi, so no configuration is necessary or available
    //

    //
    // make sure private timer is off and clear interrupt status from it
    //
    MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_CONTROL, 0);
    do
    {
        reg = MMREG_READ32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS);
        if (0 == reg)
            break;
        MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS, reg);
    } while (1);
}

void
K2_CALLCONV_REGS
K2OSKERN_MicroStall(
    UINT32 aMicroseconds
)
{
    UINT32 pauseTicks;
    UINT32 v;
    UINT32 last;
    UINT64 fempto;

    // this doesn't work until the stall counter is initialized
    if (0 == gData.Timer.mFreq)
        return;

    while (aMicroseconds > 1000000)
    {
        K2OSKERN_MicroStall(1000000);
        aMicroseconds -= 1000000;
    }

    if (0 == aMicroseconds)
        return;

    fempto = ((UINT64)aMicroseconds) << 30;

    last = sReadGlobTimer32();

    pauseTicks = (UINT32)(fempto / sgTimerDiv);
    do {
        v = sReadGlobTimer32();
        last = v - last;
        if (pauseTicks <= last)
            break;
        pauseTicks -= last;
        last = v;
    } while (1);
}

void
K2_CALLCONV_REGS
KernArch_GetHfTimerTick(
    UINT64 *apRetCount
)
{
    UINT64 counterVal;
    sReadGlobTimer64(&counterVal);
    *apRetCount = counterVal - sgTimeBias;
}

void
A32Kern_SetCoreTimer(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT32                      aCpuTickDelay
)
{
    UINT32 reg;

    K2_ASSERT(!apThisCore->mIsTimerRunning);

    KTRACE(apThisCore, 2, KTRACE_CORE_TIMER_SET, aCpuTickDelay);

    if (0 == aCpuTickDelay)
    {
        //
        // 0 means do not set a per-cpu timer interrupt
        //
        MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_CONTROL, 0);
        do
        {
            reg = MMREG_READ32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS);
            if (0 == reg)
                break;
            MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS, reg);
        } while (1);
        return;
    }

    //
    // load and enable the private timer for the delay
    //
    MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_LOAD, aCpuTickDelay);
    do
    {
        reg = MMREG_READ32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS);
        if (0 == reg)
            break;
        MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS, reg);
    } while (1);

    //
    // enable the timer
    //
    MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_CONTROL,
        A32_PERIF_PTIMERS_CONTROL_ENABLE | 
        A32_PERIF_PTIMERS_CONTROL_IRQ_ENABLE);

    apThisCore->mIsTimerRunning = TRUE;

    A32Kern_IntrSetEnable(A32_MP_PTIMERS_IRQ, TRUE);
}

BOOL
A32Kern_CoreTimerInterrupt(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    UINT32 reg;

    //
    // return true if we should enter the monitor
    //
    if (!apThisCore->mIsTimerRunning)
        return FALSE;

    apThisCore->mIsTimerRunning = FALSE;

    MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_CONTROL, 0);
    do
    {
        reg = MMREG_READ32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS);
        if (0 == reg)
            break;
        MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS, reg);
    } while (1);

    return TRUE;
}

void
A32Kern_StopCoreTimer(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    UINT32 reg;

    apThisCore->mIsTimerRunning = FALSE;

    KTRACE(apThisCore, 1, KTRACE_CORE_TIMER_STOP);

    //
    // disable the private timer interrupt at the distributor
    //
    A32Kern_IntrSetEnable(A32_MP_PTIMERS_IRQ, FALSE);

    //
    // clear the timer enable and interrupt status
    //
    MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_CONTROL, 0);
    do
    {
        reg = MMREG_READ32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS);
        if (0 == reg)
            break;
        MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS, reg);
    } while (1);
}

void
A32Kern_StartTime(
    void
)
{
    sReadGlobTimer64(&sgTimeBias);
}

static
void
sDisableCoreGlobalTimer(
    void
)
{
    UINT32 reg;
    //
    // this only accesses banked registers on the global timer
    // so locking is not necessary
    //

    //
    // disable timer interrupt for this core (banked reg)
    //
    reg = MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_CONTROL);
    reg &= ~(A32_PERIF_GTIMER_CONTROL_IRQ_ENABLE | A32_PERIF_GTIMER_CONTROL_COMP_ENABLE);
    MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_CONTROL, reg);

    //
    // clear timer interrupt status for this core (banked reg)
    //
    do
    {
        reg = MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_INTSTATUS);
        if (0 == reg)
            break;
        MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_INTSTATUS, reg);
    } while (1);
}

static
void
sLocked_DisableGlobalTimer(
    void
)
{
    sgTimerCore = (UINT32)-1;
    A32Kern_IntrSetEnable(A32_MP_GTIMER_IRQ, FALSE);
    sDisableCoreGlobalTimer();
}

void
KernArch_SchedTimer_Arm(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT64 const *              apDeltaHfTicks
)
{
    BOOL    disp;
//    UINT32  timerCore;
    UINT32  reg;
    UINT32  deltaHfTicks;
    UINT64  compTarget;

    disp = K2OSKERN_SeqLock(&sgSeqLock);

//    timerCore = sgTimerCore;

    sLocked_DisableGlobalTimer();

    if (NULL == apDeltaHfTicks)
    {
//        K2OSKERN_Debug("Core %d DISARM timer set by core %d\n", apThisCore->mCoreIx, timerCore);
        KTRACE(apThisCore, 1, KTRACE_SCHED_TIMER_DISARM);
    }
    else
    {
        deltaHfTicks = (UINT32)((*apDeltaHfTicks) & 0x00000000FFFFFFFFull);

        reg = (UINT32)(gData.Timer.mFreq / 500);
        if (deltaHfTicks < reg)
            deltaHfTicks = reg;

        sReadGlobTimer64(&compTarget);
        compTarget += deltaHfTicks;

        KTRACE(apThisCore, 2, KTRACE_SCHED_TIMER_ARM, (UINT32)(deltaHfTicks & 0xFFFFFFFFull));

        MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COMPVALHIGH, (UINT32)((compTarget >> 32) & 0xFFFFFFFF));
        MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_COMPVALLOW, (UINT32)(compTarget & 0xFFFFFFFF));

        reg = MMREG_READ32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_CONTROL);
        reg |= (A32_PERIF_GTIMER_CONTROL_IRQ_ENABLE | A32_PERIF_GTIMER_CONTROL_COMP_ENABLE);
        MMREG_WRITE32(A32KERN_MP_GLOBAL_TIMER_VIRT, A32_PERIF_GTIMER_OFFSET_CONTROL, reg);

#if 0
        if (((UINT32)-1) != timerCore)
        {
            K2OSKERN_Debug("Core %d RE-ARM timer after taking it from core %d\n", apThisCore->mCoreIx, timerCore);
        }
        else
        {
            K2OSKERN_Debug("Core %d ARM timer\n", apThisCore->mCoreIx);
        }
#endif

        sgTimerCore = apThisCore->mCoreIx;
        A32Kern_IntrSetEnable(A32_MP_GTIMER_IRQ, TRUE);
    }

    K2OSKERN_SeqUnlock(&sgSeqLock, disp);
}

void
A32Kern_TimerInterrupt(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    K2OSKERN_CPUCORE_EVENT volatile *   pEvent;
    BOOL                                disp;

    if (apThisCore->mCoreIx != sgTimerCore)
    {
        sDisableCoreGlobalTimer();   // stop interrupting
//        K2OSKERN_Debug("--Core %d ignored timer interrupt\n", apThisCore->mCoreIx);
        return;
    }

    disp = K2OSKERN_SeqLock(&sgSeqLock);

//    K2OSKERN_Debug("!!Core %d real timer interrupt\n", apThisCore->mCoreIx);

    sLocked_DisableGlobalTimer();

    K2OSKERN_SeqUnlock(&sgSeqLock, disp);

    pEvent = &gData.Timer.SchedCpuEvent;
    K2_ASSERT(KernCpuCoreEventType_Invalid == pEvent->mEventType);
    pEvent->mEventType = KernCpuCoreEvent_SchedTimer_Fired;
    pEvent->mSrcCoreIx = apThisCore->mCoreIx;
    KernArch_GetHfTimerTick((UINT64 *)&pEvent->mEventHfTick);
    KernCpu_QueueEvent(pEvent);
}

