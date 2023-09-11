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
#include <lib/k2elf.h>
#include <lib/k2mem.h>

K2STAT
K2ELF32_ValidateHeader(
    UINT8 const *   apHdrBuffer,
    UINT_PTR        aHdrBytes,
    UINT_PTR        aFileSizeBytes
)
{
    Elf32_Ehdr const *  pFileHdr;
    UINT32              fileHdrBytes;
    UINT32              chk;
    UINT32              chkEnd;

    if ((apHdrBuffer == NULL) ||
        (aHdrBytes > aFileSizeBytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((aHdrBytes < sizeof(Elf32_Ehdr)) ||
        (aFileSizeBytes < K2ELF32_MIN_FILE_SIZE))
        return K2STAT_ERROR_TOO_SMALL;

    pFileHdr = (Elf32_Ehdr const *)apHdrBuffer;

    if ((pFileHdr->e_ident[EI_MAG0] != ELFMAG0) ||
        (pFileHdr->e_ident[EI_MAG1] != ELFMAG1) ||
        (pFileHdr->e_ident[EI_MAG2] != ELFMAG2) ||
        (pFileHdr->e_ident[EI_MAG3] != ELFMAG3))
        return K2STAT_ERROR_ELF_NOT_ELF_FILE;

    if (pFileHdr->e_ident[EI_VERSION] != EV_CURRENT)
        return K2STAT_ERROR_ELF_NOT_SUPPORTED_VERSION;

    if (pFileHdr->e_ident[EI_CLASS] != ELFCLASS32)
        return K2STAT_ERROR_ELF_UNKNOWN_FILE_CLASS;

    if (pFileHdr->e_ident[EI_DATA] != ELFDATA2LSB)
        return K2STAT_ERROR_NOT_SUPPORTED;

    // e_ehsize
    fileHdrBytes = pFileHdr->e_ehsize;
    if (fileHdrBytes == 0)
        fileHdrBytes = sizeof(Elf32_Ehdr);
    else if (fileHdrBytes < sizeof(Elf32_Ehdr))
        return K2STAT_ERROR_CORRUPTED;

    if (aHdrBytes < fileHdrBytes)
        return K2STAT_ERROR_TOO_SMALL;

    // e_type
    if ((pFileHdr->e_type < ET_REL) ||
        (pFileHdr->e_type > ET_CORE))
    {
        if (pFileHdr->e_type < ET_OSSPEC_LO)
            return K2STAT_ERROR_ELF_INVALID_FILE_TYPE;
    }

    // e_version
    if (pFileHdr->e_version > EV_CURRENT)
        return K2STAT_ERROR_NOT_SUPPORTED;

    // e_machine
    if ((pFileHdr->e_machine != EM_X32) &&
        (pFileHdr->e_machine != EM_A32))
        return K2STAT_ERROR_NOT_SUPPORTED;

    // validate program header table location if present
    chk = pFileHdr->e_phnum;
    if (chk != 0)
    {
        if (pFileHdr->e_phentsize == 0)
            chkEnd = (UINT32)(aFileSizeBytes / sizeof(Elf32_Phdr));
        else
        {
            if (pFileHdr->e_phentsize < sizeof(Elf32_Phdr))
                return K2STAT_ERROR_ELF_INVALID_PHENTSIZE;
            chkEnd = (UINT32)(aFileSizeBytes / pFileHdr->e_phentsize);
        }
        if (chk > chkEnd)
            return K2STAT_ERROR_ELF_INVALID_PHNUM;

        if (pFileHdr->e_phentsize == 0)
            chkEnd = chk * sizeof(Elf32_Phdr);
        else
            chkEnd = chk * pFileHdr->e_phentsize;
        if (chkEnd > aFileSizeBytes)
            return K2STAT_ERROR_ELF_INVALID_PHENTSIZE;
        chkEnd = (UINT32)(aFileSizeBytes - chkEnd);

        if ((pFileHdr->e_phoff < fileHdrBytes) ||
            (pFileHdr->e_phoff > chkEnd))
            return K2STAT_ERROR_ELF_INVALID_PHOFF;

        // ph table lies within file bounds 
    }
    else if (pFileHdr->e_phoff != 0)
        return K2STAT_ERROR_CORRUPTED;

    // validate section header table location if present
    chk = pFileHdr->e_shnum;
    if (chk != 0)
    {
        if (pFileHdr->e_shentsize == 0)
            chkEnd = (UINT32)(aFileSizeBytes / sizeof(Elf32_Shdr));
        else
        {
            if (pFileHdr->e_shentsize < sizeof(Elf32_Shdr))
                return K2STAT_ERROR_ELF_INVALID_SHENTSIZE;
            chkEnd = (UINT32)(aFileSizeBytes / pFileHdr->e_shentsize);
        }
        if (chk > chkEnd)
            return K2STAT_ERROR_ELF_INVALID_SHNUM;

        if (pFileHdr->e_shentsize == 0)
            chkEnd = chk * sizeof(Elf32_Shdr);
        else
            chkEnd = chk * pFileHdr->e_shentsize;
        if (chkEnd > aFileSizeBytes)
            return K2STAT_ERROR_ELF_INVALID_SHENTSIZE;
        chkEnd = (UINT32)(aFileSizeBytes - chkEnd);

        if ((pFileHdr->e_shoff < fileHdrBytes) ||
            (pFileHdr->e_shoff > chkEnd))
            return K2STAT_ERROR_ELF_INVALID_SHOFF;

        // sh table lies within file bounds 
    }
    else if ((pFileHdr->e_shoff != 0) || (pFileHdr->e_shstrndx != 0))
        return K2STAT_ERROR_CORRUPTED;

    // validate section header string table index if it is nonzero
    chk = pFileHdr->e_shstrndx;
    if (chk != 0)
    {
        if (chk >= pFileHdr->e_shnum)
            return K2STAT_ERROR_ELF_INVALID_SHSTRNDX;
    }

    // validate tables do not overlap 
    chk = pFileHdr->e_phnum;
    chkEnd = pFileHdr->e_shnum;
    if ((chk) && (chkEnd))
    {
        if (pFileHdr->e_phoff < pFileHdr->e_shoff)
        {
            if (pFileHdr->e_phentsize)
                chk *= pFileHdr->e_phentsize;
            else
                chk *= sizeof(Elf32_Phdr);
            if ((pFileHdr->e_phoff + chk) > pFileHdr->e_shoff)
                return K2STAT_ERROR_ELF_TABLES_OVERLAP;
        }
        else
        {
            if (pFileHdr->e_shentsize)
                chkEnd *= pFileHdr->e_shentsize;
            else
                chkEnd *= sizeof(Elf32_Shdr);
            if ((pFileHdr->e_shoff + chkEnd) > pFileHdr->e_phoff)
                return K2STAT_ERROR_ELF_TABLES_OVERLAP;
        }
    }

    //
    // file header is valid at this point, except for e_entry 
    // being confirmed to live in a code section
    //

    return K2STAT_NO_ERROR;
}

static
BOOL
sIsValidSectionArea32(
    K2ELF32PARSE const *  apFile,
    UINT_PTR              aOffset,
    UINT_PTR              aSize
    )
{
    UINT32 chk;

    if (((aOffset + aSize) < aOffset) ||
        ((aOffset + aSize) > apFile->mRawFileByteCount) ||
        (aOffset < apFile->mFileHeaderBytes))
        return FALSE;

    if (apFile->mpSectionHeaderTable != NULL)
    {
        chk = apFile->mpRawFileData->e_shoff;
        if (aOffset < chk)
        {
            if (aOffset + aSize > chk)
                return FALSE;
        }
        else
        {
            if (aOffset < (chk + apFile->mSectionHeaderTableBytes))
                return FALSE;
        }
    }

    if (apFile->mpProgramHeaderTable != NULL)
    {
        chk = apFile->mpRawFileData->e_phoff;
        if (aOffset < chk)
        {
            if (aOffset + aSize > chk)
                return FALSE;
        }
        else
        {
            if (aOffset < (chk + apFile->mProgramHeaderTableBytes))
                return FALSE;
        }
    }

    return TRUE;
}

static
BOOL
sIsValidProgramArea32(
    K2ELF32PARSE const *  apFile,
    UINT_PTR              aOffset,
    UINT_PTR              aSize
    )
{
    if (((aOffset + aSize) < aOffset) ||
        ((aOffset + aSize) > apFile->mRawFileByteCount) ||
        (aOffset < apFile->mFileHeaderBytes))
        return FALSE;

    return TRUE;
}

static
K2STAT
sValidateSymbolTable(
    K2ELF32PARSE *      apFile,
    UINT_PTR            aSecIx,
    Elf32_Shdr const *  apSecHdr
    )
{
    UINT32              sectionCount;
    UINT8 const *       pSymTable;
    Elf32_Shdr const *  pTargetHdr;
    Elf32_Shdr const *  pStrHdr;
    Elf32_Sym           ent;
    Elf32_Word          symEntSize;
    UINT32              left;
    UINT32              symType;

    if (apSecHdr->sh_entsize == 0)
        symEntSize = sizeof(Elf32_Sym);
    else
    {
        symEntSize = apSecHdr->sh_entsize;
        if (symEntSize < sizeof(Elf32_Sym))
            return K2STAT_ERROR_ELF_INVALID_SYMTAB_ENTSIZE;
    }

    pSymTable = ((UINT8 const *)apFile->mpRawFileData) + apSecHdr->sh_offset;
    left = apSecHdr->sh_size / symEntSize;

    sectionCount = apFile->mpRawFileData->e_shnum;

    if (apSecHdr->sh_link != 0)
    {
        if (apSecHdr->sh_link >= sectionCount)
            return K2STAT_ERROR_ELF_INVALID_SYMTAB_SHLINK_IX;

        pStrHdr = K2ELF32_GetSectionHeader(apFile, apSecHdr->sh_link);
        if (pStrHdr->sh_type != SHT_STRTAB)
            return K2STAT_ERROR_ELF_INVALID_SYMTAB_SHLINK_TARGET;
    }
    else
        pStrHdr = NULL;

       
    do {
        K2MEM_Copy(&ent, pSymTable, sizeof(Elf32_Sym));
        pSymTable += symEntSize;

        if ((pStrHdr != NULL) && (ent.st_name >= pStrHdr->sh_size))
            return K2STAT_ERROR_ELF_INVALID_SYMBOL_NAME;

        symType = ELF32_ST_TYPE(ent.st_info);
        if (symType == STT_FILE)
        {
            if ((ELF32_ST_BIND(ent.st_info) != STB_LOCAL) ||
                (ent.st_shndx != SHN_ABS))
                return K2STAT_ERROR_ELF_INVALID_SOURCEFILE_SYMBOL;
        }

        if ((ent.st_shndx != 0) && (symType != STT_NONE))
        {
            if ((ent.st_shndx >= sectionCount) &&
                (ent.st_shndx < SHN_LORESERVE))
                return K2STAT_ERROR_ELF_INVALID_SYMBOL_SHNDX;

            if (ent.st_shndx < sectionCount)
            {
                // symbol refers to section in the file */
                pTargetHdr = K2ELF32_GetSectionHeader(apFile, ent.st_shndx);
                if (pTargetHdr->sh_size == 0)
                {
                    // 0-sized section.  symbol must be same as section address */
                    if (ent.st_value != pTargetHdr->sh_addr)
                        return K2STAT_ERROR_ELF_INVALID_SYMBOL_OFFSET;
                }
                else
                {
                    if (apFile->mpRawFileData->e_type == ET_REL)
                    {
                        if (ent.st_value >= pTargetHdr->sh_size)
                            return K2STAT_ERROR_ELF_INVALID_SYMBOL_ADDR;
                        // functions and other types may lie about size
                        if (ELF32_ST_TYPE(ent.st_info) == STT_OBJECT)
                        {
                            if ((pTargetHdr->sh_size - ent.st_value) < ent.st_size)
                                return K2STAT_ERROR_ELF_INVALID_SYMBOL_SIZE;
                        }
                    }
                    else if (apFile->mpRawFileData->e_type == ET_EXEC)
                    {
                        if ((ent.st_value < pTargetHdr->sh_addr) ||
                            (ent.st_value - pTargetHdr->sh_addr >= pTargetHdr->sh_size))
                            return K2STAT_ERROR_ELF_INVALID_SYMBOL_OFFSET;
                        if ((pTargetHdr->sh_size - (ent.st_value - pTargetHdr->sh_addr)) < ent.st_size)
                            return K2STAT_ERROR_ELF_INVALID_SYMBOL_SIZE;
                    }
                    else
                        return K2STAT_ERROR_ELF_INVALID_FILE_TYPE;
                }

                if (symType == STT_FUNC)
                {
                    if (!(pTargetHdr->sh_flags & SHF_EXECINSTR))
                        return K2STAT_ERROR_ELF_INVALID_SYMBOL_SECTION;
                }
                else if (symType == STT_OBJECT)
                {
                    if (pTargetHdr->sh_flags & SHF_EXECINSTR)
                        return K2STAT_ERROR_ELF_INVALID_SYMBOL_SECTION;
                }
            }
        }

    } while (--left);

    return K2STAT_NO_ERROR;
}

static
K2STAT
sValidateRelocTable(
    K2ELF32PARSE *      apFile,
    UINT_PTR            aSecIx,
    Elf32_Shdr const *  apSecHdr
    )
{
    UINT32              sectionCount;
    Elf32_Shdr const *  pTargetHdr;
    Elf32_Shdr const *  pSymHdr;

    sectionCount = apFile->mpRawFileData->e_shnum;

    if ((apSecHdr->sh_link == 0) || 
        (apSecHdr->sh_link >= sectionCount))
        return K2STAT_ERROR_ELF_INVALID_RELOCS_SHLINK_IX;

    pSymHdr = K2ELF32_GetSectionHeader(apFile, apSecHdr->sh_link);
    if (pSymHdr->sh_type != SHT_SYMTAB)
        return K2STAT_ERROR_ELF_INVALID_RELOCS_SHLINK_IX;
    
    if ((apSecHdr->sh_info == 0) || 
        (apSecHdr->sh_info >= sectionCount))
        return K2STAT_ERROR_ELF_INVALID_RELOCS_SHINFO_IX;

    pTargetHdr = K2ELF32_GetSectionHeader(apFile, apSecHdr->sh_info);
    if (pTargetHdr->sh_type == 0)
        return K2STAT_ERROR_ELF_INVALID_RELOCS_SHINFO_IX;

    if (apSecHdr->sh_entsize != 0)
    {
        if (apSecHdr->sh_entsize < sizeof(Elf32_Rel))
            return K2STAT_ERROR_ELF_INVALID_RELOC_ENTSIZE;
    }

    if (pSymHdr->sh_entsize != 0)
    {
        if (pSymHdr->sh_entsize < sizeof(Elf32_Sym))
            return K2STAT_ERROR_ELF_INVALID_SYMTAB_ENTSIZE;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2ELF32_Parse(
    UINT8 const *   apFileData,
    UINT_PTR        aFileSizeBytes,
    K2ELF32PARSE *  apRetParse
)
{
    K2STAT              stat;
    Elf32_Ehdr const *  pFileHdr;
    UINT32              fileHdrBytes;
    Elf32_Shdr const *  pSecHdrStr;
    Elf32_Shdr const *  pSecHdr;
    UINT32              hdrIx;
    Elf32_Phdr const *  pProgHdr;

    if ((apFileData == NULL) ||
        (aFileSizeBytes < K2ELF32_MIN_FILE_SIZE) ||
        (apRetParse == NULL))
        return K2STAT_ERROR_BAD_ARGUMENT;

    stat = K2ELF32_ValidateHeader(apFileData, aFileSizeBytes, aFileSizeBytes);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    // 
    // file header is usable
    //
    pFileHdr = (Elf32_Ehdr const *)apFileData;

    //
    // fill in target prior to verification
    //
    apRetParse->mpRawFileData = pFileHdr;
    apRetParse->mRawFileByteCount = aFileSizeBytes;

    fileHdrBytes = pFileHdr->e_ehsize;
    if (fileHdrBytes == 0)
        fileHdrBytes = sizeof(Elf32_Ehdr);
    apRetParse->mFileHeaderBytes = fileHdrBytes;

    pSecHdrStr = NULL;

    if (pFileHdr->e_shnum > 0)
    {
        apRetParse->mpSectionHeaderTable = apFileData + pFileHdr->e_shoff;

        if (pFileHdr->e_shentsize == 0)
            apRetParse->mSectionHeaderTableEntryBytes = sizeof(Elf32_Shdr);
        else
            apRetParse->mSectionHeaderTableEntryBytes = pFileHdr->e_shentsize;

        apRetParse->mSectionHeaderTableBytes = pFileHdr->e_shnum * apRetParse->mSectionHeaderTableEntryBytes;

        if (!K2MEM_VerifyZero(apRetParse->mpSectionHeaderTable, apRetParse->mSectionHeaderTableEntryBytes))
            return K2STAT_ERROR_ELF_SECHDR_0_INVALID;

        if (pFileHdr->e_shstrndx != 0)
        {
            pSecHdrStr = K2ELF32_GetSectionHeader(apRetParse, pFileHdr->e_shstrndx);
            if (pSecHdrStr->sh_type != SHT_STRTAB)
                return K2STAT_ERROR_ELF_SHSTRNDX_BAD_TYPE;
        }
    }
    else
    {
        apRetParse->mpSectionHeaderTable = NULL;
        apRetParse->mSectionHeaderTableEntryBytes = 0;
        apRetParse->mSectionHeaderTableBytes = 0;
    }
    
    if (pFileHdr->e_phnum != 0)
    {
        apRetParse->mpProgramHeaderTable = apFileData + pFileHdr->e_phoff;

        if (pFileHdr->e_phentsize == 0)
            apRetParse->mProgramHeaderTableEntryBytes = sizeof(Elf32_Phdr);
        else
            apRetParse->mProgramHeaderTableEntryBytes = pFileHdr->e_phentsize;

        apRetParse->mProgramHeaderTableBytes = pFileHdr->e_phnum * apRetParse->mProgramHeaderTableEntryBytes;
    }
    else
    {
        apRetParse->mpProgramHeaderTable = NULL;
        apRetParse->mProgramHeaderTableEntryBytes = 0;
        apRetParse->mProgramHeaderTableBytes = 0;
    }

    //
    // now can validate section locations because we know where everything is
    // supposed to be
    //
    if (pFileHdr->e_shnum > 0)
    {
        if (pSecHdrStr != NULL)
        {
            if (!sIsValidSectionArea32(apRetParse, pSecHdrStr->sh_offset, pSecHdrStr->sh_size))
                return K2STAT_ERROR_ELF_SECTION_OVERLAPS_TABLES;
        }

        for (hdrIx = 1; hdrIx < pFileHdr->e_shnum; hdrIx++)
        {
            pSecHdr = K2ELF32_GetSectionHeader(apRetParse, hdrIx);

            if (pSecHdr->sh_type == 0)
                continue;

            if ((pSecHdr->sh_name != 0) &&
                (pSecHdrStr != NULL))
            {
                if (pSecHdr->sh_name >= pSecHdrStr->sh_size)
                    return K2STAT_ERROR_CORRUPTED;
            }

            if (pSecHdr->sh_type != SHT_NOBITS)
            {
                if (!sIsValidSectionArea32(apRetParse, pSecHdr->sh_offset, pSecHdr->sh_size))
                    return K2STAT_ERROR_ELF_SECTION_OVERLAPS_TABLES;
            }

            if (pSecHdr->sh_type >= SHT_LOOS)
                continue;

            if (pSecHdr->sh_type == SHT_SYMTAB)
            {
                stat = sValidateSymbolTable(apRetParse, hdrIx, pSecHdr);
                if (K2STAT_IS_ERROR(stat))
                    return stat;
            }
            else if (pSecHdr->sh_type == SHT_REL)
            {
                stat = sValidateRelocTable(apRetParse, hdrIx, pSecHdr);
                if (K2STAT_IS_ERROR(stat))
                    return stat;
            }
        }
    }

    if (pFileHdr->e_phnum > 0)
    {
        for (hdrIx = 1; hdrIx < pFileHdr->e_phnum; hdrIx++)
        {
            pProgHdr = K2ELF32_GetProgramHeader(apRetParse, hdrIx);

            if (pProgHdr->p_type == 0)
                continue;

            if (!sIsValidProgramArea32(apRetParse, pProgHdr->p_offset, pProgHdr->p_filesz))
                return K2STAT_ERROR_ELF_SECTION_OVERLAPS_TABLES;
        }
    }

    return K2STAT_OK;
}

Elf32_Shdr const *
K2ELF32_GetSectionHeader(
    K2ELF32PARSE const *    apParse,
    UINT_PTR                aIndex
)
{
    UINT8 const *   pEntry;
    UINT32          entSize;

    K2_ASSERT(apParse != NULL);
    K2_ASSERT(apParse->mpRawFileData != NULL);
    K2_ASSERT(apParse->mpRawFileData->e_shnum > 0);

    pEntry = apParse->mpSectionHeaderTable;

    entSize = apParse->mpRawFileData->e_shentsize;
    if (entSize == 0)
        entSize = sizeof(Elf32_Shdr);

    if (aIndex < apParse->mpRawFileData->e_shnum)
        pEntry += (aIndex * entSize);

    return (Elf32_Shdr const *)pEntry;
}

void const *
K2ELF32_GetSectionData(
    K2ELF32PARSE const *    apParse,
    UINT_PTR                aIndex
)
{
    Elf32_Shdr const * pSecHdr;

    pSecHdr = K2ELF32_GetSectionHeader(apParse, aIndex);
    if (pSecHdr == NULL)
        return NULL;

    return ((UINT8 const *)apParse->mpRawFileData) + pSecHdr->sh_offset;
}

Elf32_Phdr const *
K2ELF32_GetProgramHeader(
    K2ELF32PARSE const *    apParse,
    UINT_PTR                aIndex
)
{
    UINT8 const *   pEntry;
    UINT32          entSize;

    K2_ASSERT(apParse != NULL);
    K2_ASSERT(apParse->mpRawFileData != NULL);
    K2_ASSERT(apParse->mpRawFileData->e_phnum > 0);

    pEntry = apParse->mpProgramHeaderTable;

    entSize = apParse->mpRawFileData->e_phentsize;
    if (entSize == 0)
        entSize = sizeof(Elf32_Phdr);

    if (aIndex < apParse->mpRawFileData->e_phnum)
        pEntry += (aIndex * entSize);

    return (Elf32_Phdr const *)pEntry;
}

