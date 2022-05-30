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
#include <lib/k2elf.h>
#include <lib/k2mem.h>
#include <lib/k2atomic.h>

K2Elf64File::~K2Elf64File(
    void
)
{
    UINT32 ix;

    if (mppSection != NULL)
    {
        for (ix = 0;ix < Header().e_shnum;ix++)
        {
            if (mppSection[ix] != NULL)
                delete mppSection[ix];
        }
        delete[] mppSection;
    }
}

K2STAT 
K2Elf64File::Create(
    UINT8 const *   apBuf, 
    UINT_PTR        aBufBytes, 
    K2Elf64File **  appRetFile
)
{
    K2STAT          stat;
    K2ELF64PARSE    parse;

    stat = K2ELF64_Parse(apBuf, aBufBytes, &parse);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    return Create(&parse, appRetFile);
}

K2STAT 
K2Elf64File::Create(
    K2ELF64PARSE const *    apParse, 
    K2Elf64File **          appRetFile
)
{
    K2STAT              stat;
    K2Elf64File *       pRet;
    UINT32              hdrIx;
    K2Elf64Section **   ppSec;
    Elf64_Shdr const *  pSecHdr;

    if ((apParse == NULL) ||
        (appRetFile == NULL))
        return K2STAT_ERROR_BAD_ARGUMENT;

    pRet = new K2Elf64File((UINT8 const *)apParse->mpRawFileData, apParse->mRawFileByteCount);
    if (pRet == NULL)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    stat = K2STAT_OK;

    do
    {
        if (apParse->mpRawFileData->e_shnum == 0)
            break;

        ppSec = new K2Elf64Section *[apParse->mpRawFileData->e_shnum];
        if (ppSec == NULL)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }

        stat = K2STAT_OK;

        K2MEM_Zero(ppSec, sizeof(K2Elf64Section *)*apParse->mpRawFileData->e_shnum);

        for (hdrIx = 0; hdrIx < apParse->mpRawFileData->e_shnum; hdrIx++)
        {
            pSecHdr = K2ELF64_GetSectionHeader(apParse, hdrIx);
            if (pSecHdr->sh_type == SHT_SYMTAB)
                ppSec[hdrIx] = new K2Elf64SymbolSection(*pRet, hdrIx, pSecHdr);
            else if (pSecHdr->sh_type == SHT_REL)
                ppSec[hdrIx] = new K2Elf64RelocSection(*pRet, hdrIx, pSecHdr);
            else
                ppSec[hdrIx] = new K2Elf64Section(*pRet, hdrIx, pSecHdr);
            if (ppSec[hdrIx] == NULL)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }
        }

        if (K2STAT_IS_ERROR(stat))
        {
            for (hdrIx = 0; hdrIx < apParse->mpRawFileData->e_shnum; hdrIx++)
            {
                if (ppSec[hdrIx] == NULL)
                    break;
                delete ppSec[hdrIx];
            }

            delete[] ppSec;
        }
        else
            pRet->mppSection = ppSec;

    } while (0);

    if (K2STAT_IS_ERROR(stat))
        delete pRet;
    else
        *appRetFile = pRet;

    return stat;
}


INT_PTR
K2Elf64File::AddRef(
    void
)
{
    return K2ATOMIC_Inc(&mRefs);
}

INT_PTR
K2Elf64File::Release(
    void
)
{
    INT_PTR ret;

    ret = K2ATOMIC_Dec(&mRefs);
    if (ret == 0)
        delete this;

    return ret;
}
