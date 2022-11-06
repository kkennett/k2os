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

#include "x32kern.h"

static UINT32   sgHPET = 0;
static UINT64   sgHPET_Period = 0;
static UINT64   sgHPET_Scaled_Period = 0;
static UINT64   sgHPET_Freq;
static UINT64   sgTimeBias = 0;
static UINT64   sgTimerDiv = 0;
static BOOL volatile sgArmed = FALSE;

static UINT32 K2_CALLCONV_REGS
sReadHPET32(
    void
)
{
    return MMREG_READ32(sgHPET, HPET_OFFSET_COUNT_LO32);
}

static void K2_CALLCONV_REGS 
sReadHPET64(
    UINT64 *apRetTick
)
{
    UINT32 hi1;
    UINT32 hi2;
    UINT32 low;

    hi1 = MMREG_READ32(sgHPET, HPET_OFFSET_COUNT_HI32);
    low = MMREG_READ32(sgHPET, HPET_OFFSET_COUNT_LO32);
    hi2 = MMREG_READ32(sgHPET, HPET_OFFSET_COUNT_HI32);
    
    if (hi1 != hi2)
        low = MMREG_READ32(sgHPET, HPET_OFFSET_COUNT_LO32);

    *apRetTick = ((((UINT64)hi2) << 32) | ((UINT64)low));
}

void
X32Kern_InitTiming(
    void
)
{
    UINT32          reg;
    UINT32          v;
    UINT32          ix;
    K2OS_IRQ_CONFIG timerIrqConfig;
    UINT32          chk;
    UINT32          chk2;
    UINT32          delta;
    UINT32          red;
    X32_CPUID       cpuId;

    //
    // make sure legacy timer is off
    //
    K2_ASSERT(NULL != gpX32Kern_HPET);

    v = ((UINT32)gpX32Kern_HPET->Address.Address) & K2_VA_MEMPAGE_OFFSET_MASK;
    sgHPET = K2OS_KVA_X32_HPET | v;

    //
    // turn off the timer
    //
    reg = MMREG_READ32(sgHPET, HPET_OFFSET_CONFIG);
    reg &= ~HPET_CONFIG_ENABLE_CNF;
    MMREG_WRITE32(sgHPET, HPET_OFFSET_CONFIG, reg);
    reg = MMREG_READ32(sgHPET, HPET_OFFSET_CONFIG);

    //
    // clear it and ack any interrupts
    //
    MMREG_WRITE32(sgHPET, HPET_OFFSET_COUNT_LO32, 1);
    MMREG_WRITE32(sgHPET, HPET_OFFSET_COUNT_HI32, 0);
    do
    {
        reg = MMREG_READ32(sgHPET, HPET_OFFSET_GISR);
        if (0 == reg)
            break;
        MMREG_WRITE32(sgHPET, HPET_OFFSET_GISR, reg);
    } while (1);

    //
    // get # of timers
    //
    reg = MMREG_READ32(sgHPET, HPET_OFFSET_GCID);
    K2_ASSERT(0 != (reg & HPET_GCID_LO_COUNT_SIZE_CAP));
    v = (reg & HPET_GCID_LO_NUM_TIM_CAP) >> HPET_GCID_LO_NUM_TIM_CAP_SHL;

    //
    // store period of tick in femptoseconds
    //
    sgHPET_Period = MMREG_READ32(sgHPET, HPET_OFFSET_GCID_PERIOD32);    // femptoseconds
    sgHPET_Freq = (1000000000000000ull / sgHPET_Period);                // ticks per second
    gData.Timer.mFreq = (UINT32)(sgHPET_Freq & 0xFFFFFFFF);
    sgHPET_Scaled_Period = (sgHPET_Period * 1024 * 1024 * 1024) / 1000000000;

    //
    // disable interupts from all HPET timers, and set them all to one-shot level triggered 32-bit mode, no fsb delivery
    //
    ix = HPET_OFFSET_TIMER0_CONFIG;
    do {
        reg = MMREG_READ32(sgHPET, ix + HPET_TIMER_OFFSET_CONFIG);
        reg &= ~HPET_TIMER_CONFIG_LO_INT_TYPE;  // edge triggered
        reg &= ~HPET_TIMER_CONFIG_LO_INT_ENB;   // disable interrupts
        reg &= ~HPET_TIMER_CONFIG_LO_TYPE;      // non-periodic
        reg |= HPET_TIMER_CONFIG_LO_32MODE;     // operate in 32-bit mode
        reg &= ~HPET_TIMER_CONFIG_LO_FSB_EN;    // fsb delivery disabled
        MMREG_WRITE32(sgHPET, ix + HPET_TIMER_OFFSET_CONFIG, reg);
        MMREG_WRITE32(sgHPET, ix + HPET_TIMER_OFFSET_COMPARE, 0);
        ix += HPET_TIMER_SPACE;
    } while (--v);

    //
    // enable the main counter in legacy routing mode
    //
    reg = MMREG_READ32(sgHPET, HPET_OFFSET_CONFIG);
    reg |= HPET_CONFIG_ENABLE_CNF;
    reg |= HPET_CONFIG_LEG_RT_CNF;  // use legacy routing
    MMREG_WRITE32(sgHPET, HPET_OFFSET_CONFIG, reg);

    sgTimerDiv = sgHPET_Scaled_Period;

    //
    // configure the timer irq
    //
    K2MEM_Zero(&timerIrqConfig, sizeof(timerIrqConfig));
    timerIrqConfig.Line.mIsActiveLow = 0;
    timerIrqConfig.Line.mIsEdgeTriggered = 1;
    timerIrqConfig.Line.mShareConfig = 0;
    timerIrqConfig.Line.mWakeConfig = 1;
    timerIrqConfig.mDestCoreIx = 0;
    timerIrqConfig.mSourceIrq = 0;
    X32Kern_ConfigDevIrq(&timerIrqConfig);

    //
    // unmask at ioapic
    //
    X32Kern_UnmaskDevIrq(0);

    //
    // get bus clock timer divisor
    //
    red = MMREG_READ32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_TIMER_DIV);
    red = ((red >> 1) & 4) | (red & 3);
    if (red == 0x7)
    {
        red = 1;
    }
    else
    {
        red = 1 << (red + 1);
    }

    gX32Kern_CpuClockRate = 0;
    gX32Kern_BusClockRate = 0;

    //
    // try to get cpu core frequency
    //
    cpuId.EAX = 0x15;
    cpuId.EBX = 0xFFFFFFFF;
    cpuId.ECX = 0xFFFFFFFF;
    cpuId.EDX = 0xFFFFFFFF;
    X32_CallCPUID(&cpuId);
    if (cpuId.ECX == 0)
    {
        cpuId.EAX = 0x16;
        cpuId.EBX = 0xFFFFFFFF;
        cpuId.ECX = 0xFFFFFFFF;
        cpuId.EDX = 0xFFFFFFFF;
        X32_CallCPUID(&cpuId);
        if (cpuId.EAX != 0)
        {
            gX32Kern_CpuClockRate = cpuId.EAX;
            if (cpuId.ECX != 0)
            {
                gX32Kern_BusClockRate = cpuId.ECX;
            }
        }
    }
    else
    {
        gX32Kern_CpuClockRate = cpuId.ECX;
    }

    if (0 == gX32Kern_CpuClockRate)
    {
        //
        // need to estimate from HPET timing on LAPIC timer
        //
        delta = gData.Timer.mFreq / 10;
        chk = MMREG_READ32(sgHPET, HPET_OFFSET_COUNT_LO32);
        MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_TIMER_INIT, 0xFFFFFFFF);
        do
        {
            chk2 = MMREG_READ32(sgHPET, HPET_OFFSET_COUNT_LO32);
        } while ((chk2 - chk) < delta);
        delta = MMREG_READ32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_TIMER_CURR);
        MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_TIMER_INIT, 0);
        gX32Kern_CpuClockRate = (UINT64)(0xFFFFFFFF - delta);
        gX32Kern_CpuClockRate *= 10;

        //
        // round up rate to nearest Mhz
        //
        gX32Kern_CpuClockRate = ((gX32Kern_CpuClockRate + 999999ull) / 1000000) * 1000000;
        
        //
        // measured clock is already attenuated by divisor, so undo that
        //
        gX32Kern_CpuClockRate *= (UINT64)red;
    }
    if (0 == gX32Kern_BusClockRate)
    {
        gX32Kern_BusClockRate = gX32Kern_CpuClockRate / (UINT64)red;
    }
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

    last = sReadHPET32();

    pauseTicks = (UINT32)(fempto / sgTimerDiv);
    do {
        v = sReadHPET32();
        last = v - last;
        if (pauseTicks <= last)
            break;
        pauseTicks -= last;
        last = v;
    } while (1);
}

void
K2_CALLCONV_REGS
KernTimer_GetHfTick(
    UINT64 *apRetCount
)
{
    UINT64 counterVal;
    sReadHPET64(&counterVal);
    *apRetCount = counterVal - sgTimeBias;
}

static void
sDisableTimerInterrupts(
    void
)
{
    UINT32 reg;

    sgArmed = FALSE;

    //
    // disable timer interrupt
    //
    reg = MMREG_READ32(sgHPET, HPET_OFFSET_TIMER0_CONFIG);
    reg &= ~HPET_TIMER_CONFIG_LO_INT_ENB;
    MMREG_WRITE32(sgHPET, HPET_OFFSET_TIMER0_CONFIG, reg);

    //
    // clear timer interrupt status
    //
    MMREG_WRITE32(sgHPET, HPET_OFFSET_GISR, HPET_GISR_T0_INT_STS);
}

void
X32Kern_StartTime(
    void
)
{
//    K2OSKERN_Debug("CpuClockRate = %d\n", (UINT32)gX32Kern_CpuClockRate);
//    K2OSKERN_Debug("BusClockRate = %d\n", (UINT32)gX32Kern_BusClockRate);
    sReadHPET64(&sgTimeBias);
    sDisableTimerInterrupts();
}

void
KernArch_SchedTimer_Arm(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT64 const *              apDeltaHfTicks
)
{
    UINT32 reg;
    UINT32 deltaHfTicks;

    sDisableTimerInterrupts();

    if (NULL == apDeltaHfTicks)
    {
//        K2OSKERN_Debug("SchedTimer(Disabled)\n");
        return;
    }

    deltaHfTicks = (UINT32)((*apDeltaHfTicks) & 0x00000000FFFFFFFFull);

//    K2OSKERN_Debug("SchedTimer(%d)\n", aDeltaHfTicks);

    //
    // arm the timer
    //
    reg = (UINT32)(sgHPET_Freq / 500);
    if (deltaHfTicks < reg)
        deltaHfTicks = reg;

    sgArmed = TRUE;

    reg = sReadHPET32() + deltaHfTicks;
    MMREG_WRITE32(sgHPET, HPET_OFFSET_TIMER0_COMPARE, reg);

    reg = MMREG_READ32(sgHPET, HPET_OFFSET_TIMER0_CONFIG);
    reg |= HPET_TIMER_CONFIG_LO_INT_ENB;
    MMREG_WRITE32(sgHPET, HPET_OFFSET_TIMER0_CONFIG, reg);
}

void 
X32Kern_TimerInterrupt(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    K2OSKERN_CPUCORE_EVENT * pEvent;

    if (!sgArmed)
    {
//        K2OSKERN_Debug("--Unexpected timer interrupt\n");
        return;
    }

    sgArmed = FALSE;

//    K2OSKERN_Debug("SchedTimer(Fires)\n");

    pEvent = &gData.Timer.SchedEvent;
    K2_ASSERT(KernCpuCoreEvent_None == pEvent->mEventType);
    pEvent->mEventType = KernCpuCoreEvent_SchedTimer_Fired;
    pEvent->mSrcCoreIx = apThisCore->mCoreIx;
    KernTimer_GetHfTick((UINT64 *)&pEvent->mEventHfTick);
    KernCpu_QueueEvent(pEvent);
}

void 
X32Kern_StopCoreTimer(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    apThisCore->mIsTimerRunning = FALSE;

    //
    // disable the LOCAPIC timer interrupt
    //
    X32Kern_MaskDevIrq(X32_DEVIRQ_LVT_TIMER);

    //
    // clear the timer init register which should disable the timer
    //
    MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_TIMER_INIT, 0);
}

void 
X32Kern_SetCoreTimer(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT32                      aCpuTickDelay
)
{
    K2_ASSERT(!apThisCore->mIsTimerRunning);
    
    if (0 == aCpuTickDelay)
    {
        //
        // 0 means do not set a per-cpu timer interrupt
        //
        return;
    }

    //
    // load and enable the LOCAPIC timer for the delay
    //
    MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_TIMER_INIT, aCpuTickDelay);

    X32Kern_UnmaskDevIrq(X32_DEVIRQ_LVT_TIMER);

    apThisCore->mIsTimerRunning = TRUE;
}

BOOL 
X32Kern_CoreTimerInterrupt(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    //
    // return true if we should enter the monitor
    //
    if (!apThisCore->mIsTimerRunning)
        return FALSE;

    apThisCore->mIsTimerRunning = FALSE;

    return TRUE;
}


