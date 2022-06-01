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
KernPte_MakePageMap(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aVirtAddr, 
    UINT32                  aPhysAddr, 
    UINT32                  aPageMapAttr
)
{
    UINT32 * pPTE;
#if defined(K2_DEBUG)
    UINT32   pteOld;
#endif

    if (NULL == apProc)
    {
        K2_ASSERT(aVirtAddr >= K2OS_KVA_KERN_BASE);
        K2_ASSERT(0 == (aPageMapAttr & K2OS_MEMPAGE_ATTR_USER));
        pPTE = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(aVirtAddr);
    }
    else
    {
        K2_ASSERT(aVirtAddr < K2OS_KVA_KERN_BASE);
        K2_ASSERT(0 != (aPageMapAttr & K2OS_MEMPAGE_ATTR_USER));
        pPTE = (UINT32*)K2_VA32_TO_PTE_ADDR(apProc->mVirtMapKVA, aVirtAddr);
    }

#if defined(K2_DEBUG)
    pteOld = *pPTE;
    K2_ASSERT((pteOld & K2OSKERN_PTE_PRESENT_BIT) == 0);
#endif

    *pPTE = KernArch_MakePTE(aPhysAddr, aPageMapAttr);

    K2_CpuWriteBarrier();
}

UINT32  
KernPte_BreakPageMap(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aVirtAddr,
    UINT32                  aNpFlags
)
{
    UINT32 * pPTE;
    UINT32   pteOld;

    if (NULL == apProc)
    {
        K2_ASSERT(aVirtAddr >= K2OS_KVA_KERN_BASE);
        pPTE = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(aVirtAddr);
    }
    else
    {
        K2_ASSERT(aVirtAddr < K2OS_KVA_KERN_BASE);
        pPTE = (UINT32*)K2_VA32_TO_PTE_ADDR(apProc->mVirtMapKVA, aVirtAddr);
    }

    pteOld = *pPTE;
    K2_ASSERT((pteOld & (K2OSKERN_PTE_PRESENT_BIT | K2OSKERN_PTE_NP_BIT)) != 0);

    if (aNpFlags & K2OSKERN_PTE_NP_BIT)
    {
        K2_ASSERT(0 == (aNpFlags & K2OSKERN_PTE_PRESENT_BIT));
        *pPTE = aNpFlags;
    }
    else
    {
        *pPTE = 0;
    }

    K2_CpuWriteBarrier();

    return pteOld & K2_VA_PAGEFRAME_MASK;
}

