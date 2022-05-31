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
#include "idlx.h"

void
iK2DLXSUPP_Cleanup(
    DLX *apDlx
    )
{
    Elf32_Shdr *    pSecHdr;
    K2DLX_SECTOR *  pSector;
    UINT32          sectionCount;
    UINT32          secIx;
    UINT32          segIx;
    UINT32          usedCount;

    pSecHdr = apDlx->mpSecHdr;
    sectionCount = apDlx->mpElf->e_shnum;
    pSector = K2_GET_CONTAINER(K2DLX_SECTOR, apDlx, Module);
    K2MEM_Zero(&pSector->mSecAddr[3], (sectionCount - 3) * sizeof(UINT32));

    usedCount = 3;
    for (secIx = 3; secIx < sectionCount; secIx++)
    {
        segIx = pSecHdr[secIx].sh_addralign;
        if (segIx >= DlxSeg_Text)
        {
            if (apDlx->SegAlloc.Segment[segIx].mLinkAddr == 0)
            {
                pSector->mSecAddr[secIx] = 0;
                pSecHdr[secIx].sh_size = 0;
            }
            else
                usedCount = secIx+1;
        }
    }

    // trim reported number of sections to highest section actually containing data
    // at the link address, and update the linked sector address table
    for (secIx = 1; secIx < usedCount; secIx++)
        pSector->mSecAddr[secIx + usedCount] = pSector->mSecAddr[secIx + sectionCount];

    apDlx->mpElf->e_shnum = (Elf32_Half)usedCount;
}
