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

#include <kern/lib/k2vmap32.h>

void
K2VMAP32_Dump(
    K2VMAP32_CONTEXT *  apContext,
    pfK2VMAP32_Dump     afDump,
    UINT32              aStartVirt,
    UINT32              aEndVirt
    )
{
    UINT32      pdeIx;
    UINT32      pde;
    UINT32 *    pPT;
    UINT32      pte;
    UINT32      pteIx;
    UINT32      pdeFlagPresent;
    UINT32      pteFlagPresent;

    if (apContext->mFlags & K2VMAP32_FLAG_REALIZED)
    {
#if K2_TARGET_ARCH_IS_ARM
        pdeFlagPresent = A32_TTBE_PT_PRESENT;
        pteFlagPresent = A32_PTE_PRESENT;
#else
        pdeFlagPresent = X32_PDE_PRESENT;
        pteFlagPresent = X32_PTE_PRESENT;
#endif
    }
    else
    {
        pdeFlagPresent = K2VMAP32_FLAG_PRESENT;
        pteFlagPresent = K2VMAP32_FLAG_PRESENT;
    }

    pdeIx = aStartVirt / K2_VA32_PAGETABLE_MAP_BYTES;

    pteIx = (aStartVirt - (pdeIx * K2_VA32_PAGETABLE_MAP_BYTES)) / K2_VA_MEMPAGE_BYTES;

    do
    {
        pde = K2VMAP32_ReadPDE(apContext, pdeIx);

        if ((pde & pdeFlagPresent) != 0)
        {
            afDump(TRUE, pdeIx * K2_VA32_PAGETABLE_MAP_BYTES, pde);

            pPT = (UINT32 *)(pde & K2VMAP32_PAGEPHYS_MASK);

            do
            {
                pte = pPT[pteIx];
                if ((pte & pteFlagPresent) != 0)
                    afDump(FALSE, aStartVirt, pte);

                aStartVirt += K2_VA_MEMPAGE_BYTES;

                pteIx++;

            } while ((pteIx < K2_VA32_ENTRIES_PER_PAGETABLE) && (aStartVirt < aEndVirt));

            pteIx = 0;
        }
        else
            aStartVirt += K2_VA32_PAGETABLE_MAP_BYTES;

        pdeIx++;

    } while ((pdeIx < K2_VA32_ENTRIES_PER_PAGETABLE) && (aStartVirt < aEndVirt));
}

