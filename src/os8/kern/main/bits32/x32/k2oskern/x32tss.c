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

void 
X32Kern_TSSSetup(
    UINT32          aThisCpuCoreIndex, 
    X32KERN_TSS *   apTSS, 
    UINT32          aESP0
)
{
    K2MEM_Zero(apTSS,sizeof(X32_TSS));
    K2MEM_Set(&apTSS->IoPermitBitmap, 0xFF, (65536 / 8) + 1);

    /* ring 0 exception stack segment */
    /* ring 0 exception stack pointer */
    apTSS->TSS.mSS0 = X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL;
    apTSS->TSS.mESP0 = aESP0;

    /* kernel mode code selector, can switch to from user mode */
    apTSS->TSS.mCS = X32_SEGMENT_SELECTOR_KERNEL_CODE | X32_SELECTOR_RPL_USER;

    /* stack and data selectors for kernel data, can switch to from user mode */
    apTSS->TSS.mSS = X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_USER;
    apTSS->TSS.mDS = X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_USER;
    apTSS->TSS.mES = X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_USER;
    apTSS->TSS.mFS = (aThisCpuCoreIndex * X32_SIZEOF_GDTENTRY) | X32_SELECTOR_TI_LDT | X32_SELECTOR_RPL_USER;
    apTSS->TSS.mGS = X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_USER;
    apTSS->TSS.mSS = X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_USER;

    apTSS->TSS.mIOBitmapOffset = sizeof(X32_TSS);
}

void
X32Kern_SetOneIoPermit(
    UINT32              aThisCoreIx,
    UINT32 *            apBitmap,
    K2OSKERN_IOPERMIT * apPermit,
    BOOL                aSet
)
{
    UINT32 pos;
    UINT32 left;
    UINT32 bits;

    left = apPermit->mCount;
    K2_ASSERT(0 != left);

    apBitmap += (apPermit->mStart / 0x20);

    pos = apPermit->mStart & 0x1F;

    do
    {
        bits = 32 - pos;

        if (bits > left)
            bits = left;

        left -= bits;

        if (bits == 32)
            bits = 0xFFFFFFFF;
        else
            bits = (1 << bits) - 1;
        bits <<= pos;
        pos = 0;

        if (aSet)
        {
            (*apBitmap) &= ~bits;
        }
        else
        {
            (*apBitmap) |= bits;
        }

        apBitmap++;

    } while (left > 0);
}

void 
X32Kern_SetIoPermit(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2LIST_ANCHOR *             apList,
    BOOL                        aSet
)
{
    K2LIST_LINK *       pListLink;
    K2OSKERN_IOPERMIT * pPermit;
    UINT32              thisCoreIx;
    X32_GDTENTRY *      pTSSEntry;

    thisCoreIx = apThisCore->mCoreIx;

    pListLink = apList->mpHead;
    if (NULL == pListLink)
    {
        return;
    }

    do
    {
        pPermit = K2_GET_CONTAINER(K2OSKERN_IOPERMIT, pListLink, ListLink);
        pListLink = pListLink->mpNext;
        X32Kern_SetOneIoPermit(thisCoreIx, &gX32Kern_TSS[thisCoreIx].IoPermitBitmap[0], pPermit, aSet);
    } while (NULL != pListLink);

    pTSSEntry = &gX32Kern_GDT[X32_SEGMENT_TSS0 + thisCoreIx];
    pTSSEntry->mAttrib &= ~2;   // clear busy bit in descriptor in GDT before reloading the task register

    K2_CpuWriteBarrier();
    X32_LoadTR(X32_SEGMENT_SELECTOR_TSS(thisCoreIx));
}
