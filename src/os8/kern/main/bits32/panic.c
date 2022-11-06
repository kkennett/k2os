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

#include "kern.h"

void
K2OSKERN_Panic(
    char const *apFormat,
    ...
)
{
    VALIST                      vList;
    UINT32                      mask;
    K2OSKERN_CPUCORE volatile * pThisCore;

    K2OSKERN_SetIntr(FALSE);

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

    if (0 != K2ATOMIC_CompareExchange((UINT32 volatile *)&gData.Debug.mCoresInPanicSpin, 1, 0))
    {
        K2OSKERN_Debug("--- CORE %d PANIC after CORE %d PANIC\n", pThisCore->mCoreIx, gData.Debug.mLeadPanicCore);
        KernCpu_PanicSpin(pThisCore);
        while (1);
    }

    K2OSKERN_Debug("\n\n\n--- CORE %d PANIC ---\n", pThisCore->mCoreIx);

    KernDbg_RawDumpLockStack(pThisCore);
    
    gData.Debug.mLeadPanicCore = pThisCore->mCoreIx;
    K2_CpuWriteBarrier();
    if ((gData.mCpuCoreCount > 1) && (gData.mCore0MonitorStarted))
    {
        mask = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << pThisCore->mCoreIx);
        do
        {
            mask &= ~KernArch_SendIci(pThisCore, mask, KernIci_Panic, NULL);
        } while (0 != mask);

        K2OSKERN_Debug("---- WAITING FOR OTHER CORES ----\n", pThisCore->mCoreIx);
        mask = 10000;
        do
        {
            if (gData.Debug.mCoresInPanicSpin == (gData.mCpuCoreCount - 1))
                break;
            K2OSKERN_MicroStall(1000);
        } while (--mask);
        if (0 == mask)
        {
            K2OSKERN_Debug("!!! Some cores not responding to panic ICI (%d)!!!\n", gData.mCpuCoreCount - gData.Debug.mCoresInPanicSpin);
        }
        else
        {
            K2OSKERN_Debug("---- OTHER CORES IN PANIC SPIN ----\n", pThisCore->mCoreIx);
        }
    }

    if (NULL != apFormat)
    {
        K2_VASTART(vList, apFormat);
        KernDbg_OutputWithArgs(apFormat, vList);
    }

    KernArch_Panic(pThisCore, TRUE);

    while (1);
}
