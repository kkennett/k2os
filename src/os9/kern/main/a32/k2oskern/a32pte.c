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
KernArch_MakePTE(
    UINT32 aPhysAddr, 
    UINT32 aPageMapAttr
)
{
    UINT32                  pte;
    K2OSKERN_PHYSTRACK *    pTrack;

    if (0 == (aPageMapAttr & K2OS_MEMPAGE_ATTR_DEVICEIO))
    {
        //
        // not a deviceio mapping.  must have a known physical range
        //
        pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(aPhysAddr);
        K2_ASSERT(0 != pTrack->Flags.Field.Exists);
        if (0 == pTrack->Flags.Field.XP_CAP)
        {
            // set as executable to defeat XN
            aPageMapAttr |= K2OS_MEMPAGE_ATTR_EXEC;
        }
    }
    else
    {
        pTrack = NULL;
        K2_ASSERT(0 == (aPageMapAttr & K2OS_MEMPAGE_ATTR_EXEC));
    }

    aPhysAddr &= A32_PTE_PAGEPHYS_MASK;
    aPageMapAttr &= K2OS_MEMPAGE_ATTR_MASK;
    
    pte = A32_PTE_PRESENT | aPhysAddr | A32_PTE_SHARED;

    //
    // set EXEC_NEVER if appropriate and possible
    //
    if (0 == (aPageMapAttr & K2OS_MEMPAGE_ATTR_EXEC))
    {
        // appropriate
        if ((pTrack == NULL) || (pTrack->Flags.Field.XP_CAP))
        {
            // possible
            pte |= A32_PTE_EXEC_NEVER;
        }
    }

    //
    // set region type
    //
    if (aPageMapAttr & K2OS_MEMPAGE_ATTR_DEVICEIO)
    {
        pte |= A32_MMU_PTE_REGIONTYPE_SHAREABLE_DEVICE;
    }
    else
    {
        K2_ASSERT(pTrack != NULL);

        if (aPageMapAttr & K2OS_MEMPAGE_ATTR_UNCACHED)
        {
            K2_ASSERT(0 != pTrack->Flags.Field.UC_CAP);
            pte |= A32_MMU_PTE_REGIONTYPE_UNCACHED;
        }
        else if (aPageMapAttr & K2OS_MEMPAGE_ATTR_WRITE_THRU)
        {
            K2_ASSERT(0 != pTrack->Flags.Field.WT_CAP);
            pte |= A32_MMU_PTE_REGIONTYPE_CACHED_WRITETHRU;
        }
        else
        {
            K2_ASSERT(0 != pTrack->Flags.Field.WB_CAP);
            pte |= A32_MMU_PTE_REGIONTYPE_CACHED_WRITEBACK;
        }
    }

    //
    // finally set RW
    //
    if (0 != (aPageMapAttr & K2OS_MEMPAGE_ATTR_USER))
    {
        //
        // user space PTE
        //
        pte |= A32_PTE_NOT_GLOBAL;
        if (aPageMapAttr & K2OS_MEMPAGE_ATTR_WRITEABLE)
            pte |= A32_MMU_PTE_PERMIT_KERN_RW_USER_RW;
        else
            pte |= A32_MMU_PTE_PERMIT_KERN_RW_USER_RO;
    }
    else
    {
        //
        // kernel space PTE
        //
        if (aPageMapAttr & K2OS_MEMPAGE_ATTR_WRITEABLE)
            pte |= A32_MMU_PTE_PERMIT_KERN_RW_USER_NONE;
        else
            pte |= A32_MMU_PTE_PERMIT_KERN_RO_USER_NONE;
    }

    return pte;
}

BOOL    
KernArch_PteMapsUserWriteable(
    UINT32 aPTE
)
{
    if (0 == (aPTE & A32_PTE_NOT_GLOBAL))
        return FALSE;
    // PTE maps user space
    return ((aPTE & A32_PTE_AP_MASK) == A32_MMU_PTE_PERMIT_KERN_RW_USER_RW) ? TRUE : FALSE;
}

