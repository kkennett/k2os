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

#define SIZEOF_ONE_STUB 12

extern void * const X32KernAsm_InterruptStubs;

void X32KernAsm_IDTFlush(void);

void 
X32Kern_IDTSetup(
    void
)
{
    UINT32 ix;
    UINT32 workAddr;

    /* init table */
    workAddr = (UINT32)&X32KernAsm_InterruptStubs;
    for(ix=0;ix<X32_NUM_IDT_ENTRIES;ix++)
    {
        gX32Kern_IDT[ix].mAddrLow16 = (UINT16)(workAddr & 0xFFFF);
        gX32Kern_IDT[ix].mAddrHigh16 = (UINT16)((workAddr >> 16) & 0xFFFF);
        gX32Kern_IDT[ix].mSelector = X32_SEGMENT_SELECTOR_KERNEL_CODE;
        gX32Kern_IDT[ix].mFlags = 0x8E;    /* 0x8E = Present, DPL 0, NonSystem, 32-bit gate */
        gX32Kern_IDT[ix].mAlwaysZero = 0;
        workAddr += SIZEOF_ONE_STUB;
    }

    gX32Kern_IDTPTR.mBaseAddr = (UINT32)&gX32Kern_IDT;
    gX32Kern_IDTPTR.mTableSize = (X32_NUM_IDT_ENTRIES * sizeof(X32_IDTENTRY)) - 1;
    
    X32KernAsm_IDTFlush();
}

