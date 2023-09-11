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

#include "k2elf2xdl.h"

K2STAT
FindSetAnchor32(
    K2ELF32PARSE *  apParse
)
{
    UINT_PTR            secIx;
    Elf32_Shdr const *  pSymSecHdr;
    Elf32_Shdr const *  pStrSecHdr;
    Elf32_Sym const *   pSym;
    char const *        pStr;
    UINT32              symType;
    UINT32              symBinding;
    UINT32              entSize;
    UINT32              symLeft;

    for (secIx = 1; secIx < apParse->mpRawFileData->e_shnum; secIx++)
    {
        pSymSecHdr = K2ELF32_GetSectionHeader(apParse, secIx);
        if (pSymSecHdr->sh_type != SHT_SYMTAB)
            continue;

        if ((pSymSecHdr->sh_link == 0) ||
            (pSymSecHdr->sh_link >= apParse->mpRawFileData->e_shnum))
            continue;

        pStrSecHdr = K2ELF32_GetSectionHeader(apParse, pSymSecHdr->sh_link);

        pStr = ((char const *)apParse->mpRawFileData) + pStrSecHdr->sh_offset;

        pSym = (Elf32_Sym const *)(((UINT8 const *)apParse->mpRawFileData) + pSymSecHdr->sh_offset);
        entSize = pSymSecHdr->sh_entsize;
        if (entSize == 0)
            entSize = sizeof(Elf32_Sym);
        symLeft = pSymSecHdr->sh_size / entSize;
        do
        {
            symBinding = ELF32_ST_BIND(pSym->st_info);
            if (symBinding = STB_GLOBAL)
            {
                symType = ELF32_ST_TYPE(pSym->st_info);
                if (symType == STT_OBJECT)
                {
                    if ((pSym->st_size == sizeof(XDL_ELF_ANCHOR)) &&
                        (0 == K2ASC_Comp("gpXdlAnchor", pStr + pSym->st_name)))
                    {
                        gOut.mpElfAnchor = (XDL_ELF_ANCHOR *)LoadAddrToDataPtr32(apParse, pSym->st_value, NULL);
                        if (NULL == gOut.mpElfAnchor)
                        {
                            printf("*** Could not get address of elf anchor by symbol\n");
                            return K2STAT_ERROR_BAD_ARGUMENT;
                        }
                        return K2STAT_NO_ERROR;
                    }
                }
            }
            pSym = (Elf32_Sym *)(((UINT8 const *)pSym) + entSize);
        } while (--symLeft);
    }

    printf("*** No elf anchor symbol found\n");
    return K2STAT_ERROR_NOT_FOUND;
}

K2STAT
Convert32(
    void
)
{
    K2ELF32PARSE    parse;
    K2STAT          stat;

    //
    // parse
    //
    stat = K2ELF32_Parse((UINT8 const *)gOut.mpElfFile->DataPtr(), (UINT_PTR)gOut.mpElfFile->FileBytes(), &parse);
    if (K2STAT_IS_ERROR(stat))
    {
        printf("*** Input elf file did not parse correctly (0x%08X)\n", stat);
        return stat;
    }

    //
    // find the anchor symbol in the symbol table (gpXdlAnchor) --> gOut.mpElfAnchor in rodata section
    //
    stat = FindSetAnchor32(&parse);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    //
    // create import library or delete existing one if no exports
    //
    stat = CreateImportLibrary32(&parse);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    //
    // create XDL file
    //
    return CreateOutputFile32(&parse);
}