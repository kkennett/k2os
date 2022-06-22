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

#include "k2export.h"

K2STAT
AddOneObject32(
    K2ReadOnlyMappedFile *  apSrcFile,
    char const *            apObjectFileName,
    UINT_PTR                aObjectFileNameLen,
    UINT8 const *           apFileData,
    UINT_PTR                aFileDataBytes
)
{
    K2ELF32PARSE    parse;
    K2STAT          stat;
    UINT_PTR        secIx;
    Elf32_Shdr *    pSecHdr;
    UINT_PTR        symBytes;
    UINT_PTR        symCount;
    Elf32_Shdr *    pStrSecHdr;
    char const *    pStr;
    UINT8 const *   pScan;
    UINT_PTR        symBind;
    char const *    pSymName;
    UINT_PTR        symNameLen;
    Elf32_Sym *     pSym;

    printf("k2export:AddOneObject32(\"%.*s\")\n", aObjectFileNameLen, apObjectFileName);
    stat = K2ELF32_Parse(apFileData, aFileDataBytes, &parse);
    if (K2STAT_IS_ERROR(stat))
    {
        printf("*** Could not parse input file as ELF (%.*s)\n", aObjectFileNameLen, apObjectFileName);
        return stat;
    }

    for (secIx = 1; secIx < parse.mpRawFileData->e_shnum; secIx++)
    {
        pSecHdr = (Elf32_Shdr *)(parse.mpSectionHeaderTable + (parse.mSectionHeaderTableEntryBytes * secIx));
        if (pSecHdr->sh_type != SHT_SYMTAB)
            continue;

        symBytes = pSecHdr->sh_entsize;
        if (0 == symBytes)
            symBytes = sizeof(Elf32_Sym);
        symCount = pSecHdr->sh_size / symBytes;
        if (1 >= symCount)
            continue;

        pStrSecHdr = (Elf32_Shdr *)(parse.mpSectionHeaderTable + (parse.mSectionHeaderTableEntryBytes * pSecHdr->sh_link));
        pStr = ((char const *)parse.mpRawFileData) + pStrSecHdr->sh_offset;

        pScan = ((UINT8 const *)parse.mpRawFileData) + pSecHdr->sh_offset;
        do {
            pSym = (Elf32_Sym *)pScan;
            pScan += symBytes;

            pSymName = pStr + pSym->st_name;

            symBind = ELF32_ST_BIND(pSym->st_info);

            if (((symBind == STB_GLOBAL) ||
                (symBind == STB_WEAK)) &&
                (pSym->st_shndx != 0))
            {
                /* this is a global symbol in this object file */
                symNameLen = K2ASC_Len(pSymName);
                if ((symNameLen != 0) && (pSym->st_shndx < parse.mpRawFileData->e_shnum))
                {
                    printf("  AddSymbol(%s)\n", pSymName);
                }
            }
        } while (--symCount);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
AddOneObject64(
    K2ReadOnlyMappedFile *  apSrcFile,
    char const *            apObjectFileName,
    UINT_PTR                aObjectFileNameLen,
    UINT8 const *           apFileData,
    UINT_PTR                aFileDataBytes
)
{
    K2ELF64PARSE    parse;
    K2STAT          stat;

    printf("k2export:AddOneObject64(\"%.*s\")\n", aObjectFileNameLen, apObjectFileName);
    stat = K2ELF64_Parse(apFileData, aFileDataBytes, &parse);
    if (K2STAT_IS_ERROR(stat))
    {
        printf("*** Could not parse input file as ELF (%.*s)\n", aObjectFileNameLen, apObjectFileName);
        return stat;
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
AddOneObject(
    K2ReadOnlyMappedFile *  apSrcFile,
    char const *            apObjectFileName,
    UINT_PTR                aObjectFileNameLen,
    UINT8 const *           apFileData,
    UINT_PTR                aFileDataBytes
)
{
    UINT_PTR fileClass;

    fileClass = apFileData[EI_CLASS];
    if (fileClass != gOut.mFileClass) 
    {
        if (0 != gOut.mFileClass)
        {
            printf("*** mixed file classes (32-bit and 64-bit) - (%.*s)\n", aObjectFileNameLen, apObjectFileName);
            return K2STAT_ERROR_BAD_ARGUMENT;
        }
        if ((fileClass != ELFCLASS32) &&
            (fileClass != ELFCLASS64))
        {
            printf("*** invalid file class for ELF file (%.*s)\n", aObjectFileNameLen, apObjectFileName);
            return K2STAT_ERROR_BAD_ARGUMENT;
        }
        gOut.mFileClass = fileClass;
    }

    if (gOut.mFileClass == ELFCLASS32)
    {
        return AddOneObject32(apSrcFile, apObjectFileName, aObjectFileNameLen, apFileData, aFileDataBytes);
    }

    return AddOneObject64(apSrcFile, apObjectFileName, aObjectFileNameLen, apFileData, aFileDataBytes);
}
