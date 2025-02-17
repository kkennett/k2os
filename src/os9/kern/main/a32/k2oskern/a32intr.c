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

UINT32  
KernArch_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore, 
    UINT32                      aMask, 
    KernIciType                 aIciType, 
    void *                      apArg
)
{
    UINT32                              coreIx;
    UINT32                              sentMask;
    K2OSKERN_CPUCORE volatile *         pOtherCore;
    K2OSKERN_CPUCORE_ICI volatile *     pIciTarget;
    UINT32                              sendBit;

    K2_ASSERT(1 < gData.mCpuCoreCount); // dont call this on a single-core system
    K2_ASSERT(aMask != 0);
    K2_ASSERT(0 == (aMask & (1 << apThisCore->mCoreIx)));

    sendBit = 1;
    sentMask = 0;

    for (coreIx = 0; coreIx < gData.mCpuCoreCount; coreIx++)
    {
        if ((coreIx != apThisCore->mCoreIx) &&
            (0 != (sendBit & aMask)))
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

            aMask &= ~sendBit;
            if (0 == aMask)
                break;
        }
        sendBit <<= 1;
    }

    if (0 == sentMask)
        return 0;

    MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDSGIR, (sentMask << 16) | apThisCore->mCoreIx);

    return sentMask;
}

void 
A32Kern_IntrInitGicDist(
    void
)
{
    UINT32  ix;
    UINT32  numRegs32;
    UINT32  mask;

    gA32Kern_IntrCount = 32 * ((MMREG_READ32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDICTR) & 0x1F) + 1);
    if (gA32Kern_IntrCount > A32KERN_MAX_IRQ)
        gA32Kern_IntrCount = A32KERN_MAX_IRQ;

    //
    // disable distributor and per-cpu interface
    //
    MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDDCR, 0);
    MMREG_WRITE32(A32KERN_MP_GICC_VIRT, A32_PERIF_GICC_OFFSET_ICCICR, 0);

    numRegs32 = gA32Kern_IntrCount / 32;

    //
    // clear all enables
    //
    for (ix = 0; ix < numRegs32; ix++)
    {
        MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDICER0 + (ix * sizeof(UINT32)), 0xFFFFFFFF);
    }

    //
    // clear all pendings
    //
    for (ix = 0; ix < numRegs32; ix++)
    {
        MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDICPR0 + (ix * sizeof(UINT32)), 0xFFFFFFFF);
    }

    //
    // set all interrupts to top priority. 8 bits per interrupt
    //
    for (ix = 0; ix < gA32Kern_IntrCount / (32 / 8); ix++)
    {
        MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDIPR0 + (ix * sizeof(UINT32)), 0);
    }

    //
    // set configuration as level sensitive N:N, 2 bits per interrupt
    //
    for (ix = 0; ix < gA32Kern_IntrCount / (32 / 2); ix++)
    {
        MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDICFR0 + (ix * sizeof(UINT32)), 0);
    }

    // 
    // target all IRQs >= 16 at CPU 0. 8 bits per interrupt
    //
    for (ix = 4; ix < gA32Kern_IntrCount / (32 / 8); ix++)
    {
        MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDIPTR0 + (ix * sizeof(UINT32)), 0x01010101);
    }

    //
    // SGI 0-15 go to all processors. 8 bits per interrupt
    //
    mask = (1 << gData.mCpuCoreCount) - 1;
    mask = mask | (mask << 8) | (mask << 16) | (mask << 24);
    for (ix = 0; ix < 16 / (32 / 8); ix++)
    {
        MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDIPTR0 + (ix * sizeof(UINT32)), mask);
    }

    //
    // enable interrupts at distributor
    //
    MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDDCR,
        A32_PERIF_GICD_ICDDCR_NONSECURE_ENABLE_NONSEC);

}

void
A32Kern_IntrInitGicPerCore(
    void
)
{
    //
    // disable interrupts on this CPU interface
    //
    MMREG_WRITE32(A32KERN_MP_GICC_VIRT, A32_PERIF_GICC_OFFSET_ICCICR, 0);

    //
    // set priority mask for this cpu
    //
    MMREG_WRITE32(A32KERN_MP_GICC_VIRT, A32_PERIF_GICC_OFFSET_ICCPMR, 0xFF);

    //
    // enable interrupts on this CPU interface
    //
    MMREG_WRITE32(A32KERN_MP_GICC_VIRT, A32_PERIF_GICC_OFFSET_ICCICR,
        A32_PERIF_GICC_ICCICR_ENABLE_SECURE |
        A32_PERIF_GICC_ICCICR_ENABLE_NS);
}

UINT32
A32Kern_IntrAck(
    void
)
{
    K2_ASSERT(A32KERN_MP_GICC_VIRT != 0);
    return MMREG_READ32(A32KERN_MP_GICC_VIRT, A32_PERIF_GICC_OFFSET_ICCIAR);
}

void
A32Kern_IntrEoi(
    UINT32 aAckVal
)
{
    K2_ASSERT(A32KERN_MP_GICC_VIRT != 0);
    K2_ASSERT((((aAckVal & 0x1C00) >> 10) < gData.mCpuCoreCount) && ((aAckVal & 0x3FF) < gA32Kern_IntrCount));
    MMREG_WRITE32(A32KERN_MP_GICC_VIRT, A32_PERIF_GICC_OFFSET_ICCEOIR, aAckVal);
}

void
A32Kern_IntrSetEnable(
    UINT32  aIntrId,
    BOOL    aSetEnable
)
{
    UINT32 regOffset;

    K2_ASSERT(A32KERN_MP_GICC_VIRT != 0);
    K2_ASSERT(aIntrId < gA32Kern_IntrCount);

    regOffset = (aIntrId / 32) * sizeof(UINT32);
    aIntrId = (1 << (aIntrId & 31));

    if (aSetEnable)
    {
        regOffset += A32_PERIF_GICD_OFFSET_ICDISER0;
    }
    else
    {
        regOffset += A32_PERIF_GICD_OFFSET_ICDICER0;
    }

    MMREG_WRITE32(A32KERN_MP_GICD_VIRT, regOffset, aIntrId);
}

BOOL
A32Kern_IntrGetEnable(
    UINT32 aIntrId
)
{
    UINT32 regOffset;

    K2_ASSERT(A32KERN_MP_GICC_VIRT != 0);
    K2_ASSERT(aIntrId < gA32Kern_IntrCount);

    regOffset = (aIntrId / 32) * sizeof(UINT32);
    regOffset += A32_PERIF_GICD_OFFSET_ICDISER0;
    aIntrId = (1 << (aIntrId & 31));

    regOffset = MMREG_READ32(A32KERN_MP_GICD_VIRT, regOffset);

    return (((regOffset & aIntrId) != 0) ? TRUE : FALSE);
}

void
A32Kern_IntrClearPending(
    UINT32 aIntrId,
    BOOL   aAlsoDisable
)
{
    UINT32 regOffset;

    K2_ASSERT(A32KERN_MP_GICC_VIRT != 0);
    K2_ASSERT(aIntrId < gA32Kern_IntrCount);

    regOffset = (aIntrId / 32) * sizeof(UINT32);
    aIntrId = (1 << (aIntrId & 31));

    MMREG_WRITE32(A32KERN_MP_GICC_VIRT, regOffset + A32_PERIF_GICD_OFFSET_ICDICPR0, aIntrId);

    //
    // disable after clear or you may miss an interrupt that happens in between
    // the clear pending and the disable
    //
    if (aAlsoDisable)
    {
        MMREG_WRITE32(A32KERN_MP_GICD_VIRT, regOffset + A32_PERIF_GICD_OFFSET_ICDICER0, aIntrId);
    }
}

void
A32Kern_IntrConfig(
    UINT32                  aIntrId,
    K2OS_IRQ_CONFIG const * apConfig
)
{
    UINT32  regOffset;
    UINT32  shl;
    UINT32  regVal;

    // 16 interrupts per register
    regOffset = aIntrId >> 4;
    shl = (aIntrId & 0xF) << 1;

    // high bit is level=0, edge=1
    // low bit is N:N=0, 1:N=1

    regVal = MMREG_READ32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDICFR0 + (regOffset * sizeof(UINT32)));

    regVal &= ~(3 << shl);

    if (apConfig->Line.mIsEdgeTriggered)
        regVal |= (2 << shl);

    MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDICFR0 + (regOffset * sizeof(UINT32)), regVal);
}
