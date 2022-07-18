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

#include "k2elf2xdl.h"

UINT8 const *
LoadAddrToDataPtr32(
    K2ELF32PARSE *  apParse,
    UINT_PTR        aLoadAddr,
    UINT_PTR *      apRetSecIx
)
{
    UINT32              secIx;
    Elf32_Shdr const *  pSecHdr;

    if (apRetSecIx != NULL)
        *apRetSecIx = 0;

    for (secIx = 1; secIx < apParse->mpRawFileData->e_shnum; secIx++)
    {
        pSecHdr = K2ELF32_GetSectionHeader(apParse, secIx);
        if ((pSecHdr->sh_flags & SHF_ALLOC) &&
            (pSecHdr->sh_type != SHT_NOBITS))
        {
            if ((aLoadAddr >= pSecHdr->sh_addr) &&
                ((aLoadAddr - pSecHdr->sh_addr) < pSecHdr->sh_size))
            {
                if (apRetSecIx != NULL)
                    *apRetSecIx = secIx;
                return (((UINT8 *)apParse->mpRawFileData) + pSecHdr->sh_offset) + (aLoadAddr - pSecHdr->sh_addr);
            }
        }
    }

    return NULL;
}

UINT8 const *
LoadOffsetToDataPtr32(
    K2ELF32PARSE *  apParse,
    UINT_PTR        aLoadOffset,
    UINT_PTR *      apRetSecIx
)
{
    UINT32              secIx;
    Elf32_Shdr const *  pSecHdr;

    if (apRetSecIx != NULL)
        *apRetSecIx = 0;

    for (secIx = 1; secIx < apParse->mpRawFileData->e_shnum; secIx++)
    {
        pSecHdr = K2ELF32_GetSectionHeader(apParse, secIx);
        if (pSecHdr->sh_type != SHT_NOBITS)
        {
            if ((aLoadOffset >= pSecHdr->sh_offset) &&
                ((aLoadOffset - pSecHdr->sh_offset) < pSecHdr->sh_size))
            {
                if (apRetSecIx != NULL)
                    *apRetSecIx = secIx;
                return (((UINT8 *)apParse->mpRawFileData) + pSecHdr->sh_offset) + (aLoadOffset - pSecHdr->sh_offset);
            }
        }
    }

    return NULL;
}

