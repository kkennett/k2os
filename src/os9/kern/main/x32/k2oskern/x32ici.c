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

#include "x32kern.h"

UINT32
KernArch_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT32                      aTargetCoreMask,
    KernIciType                 aIciType,
    void *                      apArg
)
{
    UINT32                              coreIx;
    UINT32                              sentMask;
    K2OSKERN_CPUCORE volatile *         pOtherCore;
    K2OSKERN_CPUCORE_ICI volatile *     pIciTarget;
    UINT32                              reg;
    UINT32                              sendBit;

    K2_ASSERT(1 < gData.mCpuCoreCount); // do not call this in single-core system
    K2_ASSERT(aTargetCoreMask != 0);
    K2_ASSERT(0 == (aTargetCoreMask & (1 << apThisCore->mCoreIx)));
    
    sendBit = 1;
    sentMask = 0;

    for (coreIx = 0; coreIx < gData.mCpuCoreCount; coreIx++)
    {
        if ((coreIx != apThisCore->mCoreIx) &&
            (0 != (sendBit & aTargetCoreMask)))
        {
            pOtherCore = K2OSKERN_COREIX_TO_CPUCORE(coreIx);
    
            pIciTarget = &pOtherCore->IciFromOtherCore[apThisCore->mCoreIx];

            if (pIciTarget->mIciType == KernIciType_Invalid)
            {
                //
                // no pending ici from this core to the other core
                //
                pIciTarget->mpArg = apArg;
                pIciTarget->mIciType = aIciType;
                K2_CpuWriteBarrier();
                sentMask |= sendBit;
            }
            else
            {
                //
                // ici pending from this core to the other core it has not serviced
                // yet.
                //
            }

            aTargetCoreMask &= ~sendBit;
            if (0 == aTargetCoreMask)
                break;
        }
        sendBit <<= 1;
    }

    if (0 == sentMask)
        return 0;

    /* check pending send bit clear - if it is not then we can't send */
    reg = MMREG_READ32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_LOW32);
    if (0 != (reg & (1 << 12)))
        return 0;

    /* set target cpus mask */
    reg = sentMask << 24;

    /* send to logical CPU interrupt X32KERN_VECTOR_ICI_BASE + my Index */
    MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_HIGH32, reg);
    MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_LOW32, X32_LOCAPIC_ICR_LOW_LEVEL_ASSERT | X32_LOCAPIC_ICR_LOW_LOGICAL | X32_LOCAPIC_ICR_LOW_MODE_FIXED | (X32KERN_VECTOR_ICI_BASE + apThisCore->mCoreIx));

    return sentMask;
}

