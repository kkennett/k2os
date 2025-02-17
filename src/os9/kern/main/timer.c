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

#include "kern.h"

#define WIN32_EPOCH_64                  116444736000000000LL
#define WIN32_FILETIME_TICKS_PER_SEC    10000000

void
KernTimer_Init(
    void
)
{
    //
    // arch will have filled these in
    //
    K2_ASSERT(0 != gData.Timer.mFreq);
    K2_ASSERT(0 != gData.Timer.mIoPhys);
}

void                
KernTimer_HfTickFromMsTick(
    UINT64 *apRetHfTicks,
    UINT64 const *apMsTick
)
{
    *apRetHfTicks = ((*apMsTick) * ((UINT64)gData.Timer.mFreq)) / 1000ull;
}

void
KernTimer_MsTickFromHfTick(
    UINT64 *        apRetMs,
    UINT64 const *  apHfTicks
)
{
    *apRetMs = (((*apHfTicks) * 1000ull) / ((UINT64)gData.Timer.mFreq));
}

void    
KernTimer_GetTime(
    K2OS_TIME *apRetTime
)
{
    //
    // convert current millisecond to calendar date
    //

    //
    // if we don't have one or its been too long since we
    // did a dead reckoning, do a dead reckoning
    // but make sure time doesn't go backwards.
    //

    KernFirm_GetTime(apRetTime);
}

void    
KernTimer_SysCall_GetTime(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OS_THREAD_PAGE *pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    KernTimer_GetTime((K2OS_TIME *)&pThreadPage->mMiscBuffer);

    apCurThread->User.mSysCall_Result = 1;
}

