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

static char const * const sgpSecStr_SecStr = ".sechdrstr";
static char const * const sgpSecStr_SymStr = ".symstr";
static char const * const sgpSecStr_Sym = ".symtable";
static char const * const sgpSecStr_XDLAnchor = ".rodata.xdlanchor";
static char const * const sgpSecStr_XDLAnchorReloc = ".rel.rodata.xdlanchor";
static char const * const sgpSecStr_Exp = ".rodata.exp.?";
static char const * const sgpSecStr_Rel = ".rel.rodata.?";

static char const * const sgpSymExp = "xdl_?export";
static char const * const sgpSymInfo = "gpXdlAnchor";

static
void
sPrepSection(
    UINT_PTR aIx
    )
{
    UINT_PTR        work;
    UINT_PTR        startOffset;
    UINT_PTR        len;
    EXPORT_SPEC *   pSpec;

    if (gOut.mOutSeg[aIx].mCount == 0)
        return;

    gOut.mFileSizeBytes += gOut.mOutSeg[aIx].mCount * sizeof(Elf32_Sym);

    gOut.mSecStrTotalBytes += K2ASC_Len(sgpSecStr_Exp) + 1;
    gOut.mSecStrTotalBytes += K2ASC_Len(sgpSecStr_Rel) + 1;

    gOut.mOutSeg[aIx].mIx = gOut.mSectionCount;
    gOut.mFileSizeBytes += sizeof(Elf32_Shdr);
    gOut.mSectionCount++;

    // data in export section
    startOffset = gOut.mFileSizeBytes;
    gOut.mFileSizeBytes += sizeof(XDL_EXPORTS_SEGMENT_HEADER);
    gOut.mFileSizeBytes += gOut.mOutSeg[aIx].mCount * sizeof(XDL_EXPORT32_REF);
    startOffset = gOut.mFileSizeBytes - startOffset;
    work = 0;
    pSpec = gOut.mOutSeg[aIx].mpSpecList;
    while (pSpec != NULL)
    {
        pSpec->mExpNameOffset = work + startOffset;
        pSpec->mSymNameOffset = gOut.mSymStrTotalBytes;

        len = K2ASC_Len(pSpec->mpName) + 1;
        work += len;
        gOut.mSymStrTotalBytes += len;
        pSpec = pSpec->mpNext;
    }
    gOut.mSymStrTotalBytes += work;

    // section data symbol name
    gOut.mOutSeg[aIx].mExpSymNameOffset = gOut.mSymStrTotalBytes;
    gOut.mSymStrTotalBytes += K2ASC_Len(sgpSymExp) + 1;

    // align
    gOut.mOutSeg[aIx].mExpStrBytes = K2_ROUNDUP(work, 4);
    gOut.mFileSizeBytes += gOut.mOutSeg[aIx].mExpStrBytes;
        
    // reloc section
    gOut.mOutSeg[aIx].mRelocIx = gOut.mSectionCount;
    gOut.mFileSizeBytes += sizeof(Elf32_Shdr);
    gOut.mSectionCount++;

    // data in reloc section
    gOut.mFileSizeBytes += sizeof(Elf32_Rel) * gOut.mOutSeg[aIx].mCount;

    // symbol for export section
    gOut.mFileSizeBytes += sizeof(Elf32_Sym);

    // reloc for export section (for pointer in info section)
    gOut.mFileSizeBytes += sizeof(Elf32_Rel);
}

static
void
sPlaceSection(
    UINT_PTR aIx
    )
{
    UINT_PTR secBase;

    if (gOut.mOutSeg[aIx].mCount == 0)
        return;

    // exports
    secBase = gOut.mRawWork;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_offset = gOut.mRawWork - gOut.mRawBase;

    gOut.mOutSeg[aIx].mpExpBase = (XDL_EXPORTS_SEGMENT_HEADER *)gOut.mRawWork;
    gOut.mRawWork += sizeof(XDL_EXPORTS_SEGMENT_HEADER);
    gOut.mRawWork += gOut.mOutSeg[aIx].mCount * sizeof(XDL_EXPORT32_REF);

    gOut.mOutSeg[aIx].mpExpStrBase = (char *)gOut.mRawWork;
    gOut.mRawWork += gOut.mOutSeg[aIx].mExpStrBytes;

    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_size = gOut.mRawWork - secBase;

    // relocs
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_offset = gOut.mRawWork - gOut.mRawBase;
    gOut.mOutSeg[aIx].Bits32.mpRelocs = (Elf32_Rel *)gOut.mRawWork;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_size = sizeof(Elf32_Rel) * gOut.mOutSeg[aIx].mCount;
    gOut.mRawWork += gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_size;
}

static
void
sEmitSection(
    UINT_PTR   aIx
    )
{
    Elf32_Rel *         pReloc;
    XDL_EXPORT32_REF *  pRef;
    EXPORT_SPEC *       pSpec;
    UINT_PTR            symIx;
    UINT8               binding;
    char *              pExpSecSymName;

    if (gOut.mOutSeg[aIx].mCount == 0)
        return;

    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_name = (Elf32_Word)(gOut.mpSecStrWork - gOut.mpSecStrBase);
    K2ASC_Copy(gOut.mpSecStrWork, sgpSecStr_Exp);
    if (aIx == XDLProgData_Text)
    {
        binding = ELF32_MAKE_SYMBOL_INFO(STB_GLOBAL, STT_FUNC);
        *(gOut.mpSecStrWork + 12) = 'c';
    }
    else
    {
        binding = ELF32_MAKE_SYMBOL_INFO(STB_GLOBAL, STT_OBJECT);
        if (aIx == XDLProgData_Read)
            *(gOut.mpSecStrWork + 12) = 'r';
        else
            *(gOut.mpSecStrWork + 12) = 'd';
    }
    gOut.mpSecStrWork += K2ASC_Len(sgpSecStr_Exp) + 1;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_type = SHT_PROGBITS;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_flags = SHF_ALLOC;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_addr = (Elf32_Addr)gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_offset;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_addralign = 4;

    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_name = (Elf32_Word)(gOut.mpSecStrWork - gOut.mpSecStrBase);
    K2ASC_Copy(gOut.mpSecStrWork, sgpSecStr_Rel);
    if (aIx == XDLProgData_Text)
    {
        *(gOut.mpSecStrWork + 12) = 'c';
    }
    else if (aIx == XDLProgData_Read)
    {
        *(gOut.mpSecStrWork + 12) = 'r';
    }
    else
    {
        *(gOut.mpSecStrWork + 12) = 'd';
    }
    gOut.mpSecStrWork += K2ASC_Len(sgpSecStr_Rel) + 1;

    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_type = SHT_REL;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_addralign = 4;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_link = SECIX_SYM;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_info = gOut.mOutSeg[aIx].mIx;
    gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mRelocIx].sh_entsize = sizeof(Elf32_Rel);

    pReloc = gOut.mOutSeg[aIx].Bits32.mpRelocs;
    pRef = (XDL_EXPORT32_REF *)(((UINT8 *)gOut.mOutSeg[aIx].mpExpBase) + sizeof(XDL_EXPORTS_SEGMENT_HEADER));
    pSpec = gOut.mOutSeg[aIx].mpSpecList;
    while (pSpec != NULL)
    {
        K2ASC_Copy(gOut.mpSymStrBase + pSpec->mSymNameOffset, pSpec->mpName);
        K2ASC_Copy(((char *)gOut.mOutSeg[aIx].mpExpBase) + pSpec->mExpNameOffset, pSpec->mpName);

        pRef->mNameOffset = pSpec->mExpNameOffset;

        symIx = (UINT_PTR)(gOut.Bits32.mpSymWork - gOut.Bits32.mpSymBase);
        gOut.Bits32.mpSymWork->st_name = pSpec->mSymNameOffset;
        gOut.Bits32.mpSymWork->st_shndx = SHN_UNDEF;
        gOut.Bits32.mpSymWork->st_size = sizeof(void *);
        gOut.Bits32.mpSymWork->st_value = 0;
        gOut.Bits32.mpSymWork->st_info = binding;
                
        pReloc->r_info = ELF32_MAKE_RELOC_INFO(symIx, gOut.mRelocType);
        pReloc->r_offset = ((UINT_PTR)&pRef->mAddr) - ((UINT_PTR)gOut.mOutSeg[aIx].mpExpBase);

        pRef++;
        gOut.Bits32.mpSymWork++;
        pReloc++;
        pSpec = pSpec->mpNext;
    }
    gOut.mOutSeg[aIx].mpExpBase->mCount = gOut.mOutSeg[aIx].mCount;
    gOut.mOutSeg[aIx].mpExpBase->mCRC32 = K2CRC_Calc32(0, gOut.mOutSeg[aIx].mpExpStrBase, gOut.mOutSeg[aIx].mExpStrBytes);

    // name of symbol for export section
    pExpSecSymName = gOut.mpSymStrBase + gOut.mOutSeg[aIx].mExpSymNameOffset;
    K2ASC_Copy(pExpSecSymName, sgpSymExp);
    if (aIx == XDLProgData_Text)
        *(pExpSecSymName + 4) = 'c';
    else if (aIx == XDLProgData_Read)
        *(pExpSecSymName + 4) = 'r';
    else
        *(pExpSecSymName + 4) = 'd';

    // symbol for export section
    symIx = (UINT_PTR)(gOut.Bits32.mpSymWork - gOut.Bits32.mpSymBase);
    gOut.Bits32.mpSymWork->st_name = gOut.mOutSeg[aIx].mExpSymNameOffset;
    gOut.Bits32.mpSymWork->st_shndx = gOut.mOutSeg[aIx].mIx;
    gOut.Bits32.mpSymWork->st_size = gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_size;
    // object file (REL, not EXEC), so symbol values are section relative, not absolue
    gOut.Bits32.mpSymWork->st_value = 0; // gOut.Bits32.mpSecHdrs[gOut.mOutSeg[aIx].mIx].sh_addr;
    gOut.Bits32.mpSymWork->st_info = ELF32_MAKE_SYMBOL_INFO(STB_GLOBAL, STT_OBJECT);
    gOut.Bits32.mpSymWork++;

    // reloc for export section to pointer in xdlanchor
    gOut.Bits32.mpSecRelocWork->r_info = ELF32_MAKE_RELOC_INFO(symIx, gOut.mRelocType);
    gOut.Bits32.mpSecRelocWork->r_offset = (aIx * sizeof(UINT64));
    gOut.Bits32.mpSecRelocWork++;
}

static
bool
sCreateOutputFile(
    void
    )
{
    bool            ret;
    UINT_PTR        ix;
    K2ELF32PARSE    elfFile;
    HANDLE          hOutFile;

    if (gOut.mElfMachine == EM_A32)
        gOut.mRelocType = R_ARM_ABS32;
    else if (gOut.mElfMachine == EM_X32)
        gOut.mRelocType = R_386_32;
    else
    {
        printf("*** Output is for an unknown machine type\n");
    }

    // file header
    gOut.mFileSizeBytes = sizeof(Elf32_Ehdr);

    // null section header 0
    gOut.mFileSizeBytes += sizeof(Elf32_Shdr);
    gOut.mSectionCount = 1;

    // section strings header 1
    gOut.mFileSizeBytes += sizeof(Elf32_Shdr);
    gOut.mSectionCount++;
   
    gOut.mSecStrTotalBytes =
        1 +
        K2ASC_Len(sgpSecStr_SecStr) + 1 +
        K2ASC_Len(sgpSecStr_SymStr) + 1 +
        K2ASC_Len(sgpSecStr_Sym) + 1 +
        K2ASC_Len(sgpSecStr_XDLAnchor) + 1;

    // symbol strings section 2
    gOut.mFileSizeBytes += sizeof(Elf32_Shdr);
    gOut.mSectionCount++;
    gOut.mSymStrTotalBytes = 1; // 0th byte must be null

    // symbol table section 3
    gOut.mFileSizeBytes += sizeof(Elf32_Shdr);
    gOut.mSectionCount++;
    gOut.mFileSizeBytes += sizeof(Elf32_Sym); // 0th symbol is always blank

    // info section 4
    gOut.mFileSizeBytes += sizeof(Elf32_Shdr);
    gOut.mSectionCount++;
    gOut.mFileSizeBytes += sizeof(XDL_ELF_ANCHOR);
    gOut.mFileSizeBytes += sizeof(Elf32_Sym); // section symbol
    gOut.mSymStrTotalBytes += K2ASC_Len(sgpSymInfo) + 1; 

    if (gOut.mTotalExports > 0)
    {
        // info relocs section 5
        gOut.mFileSizeBytes += sizeof(Elf32_Shdr);
        gOut.mSectionCount++;
        gOut.mSecStrTotalBytes += K2ASC_Len(sgpSecStr_XDLAnchorReloc) + 1;

        // prep for export sections now
        for (ix = 0;ix < XDLProgDataType_Count;ix++)
            sPrepSection(ix);
    }

    gOut.mSymStrTotalBytes = K2_ROUNDUP(gOut.mSymStrTotalBytes, 4);
    gOut.mFileSizeBytes += gOut.mSymStrTotalBytes;

    gOut.mSecStrTotalBytes = K2_ROUNDUP(gOut.mSecStrTotalBytes, 4);
    gOut.mFileSizeBytes += gOut.mSecStrTotalBytes;

    gOut.mRawBase = (UINT_PTR)new UINT8[gOut.mFileSizeBytes];
    if (gOut.mRawBase == 0)
    {
        printf("*** Memory allocation failed\n");
        return false;
    }
    
    ret = false;
    do
    {
        K2MEM_Zero((UINT8 *)gOut.mRawBase, gOut.mFileSizeBytes);

        gOut.mRawWork = gOut.mRawBase;

        gOut.Bits32.mpFileHdr = (Elf32_Ehdr *)gOut.mRawWork;
        gOut.mRawWork += sizeof(Elf32_Ehdr);

        gOut.Bits32.mpSecHdrs = (Elf32_Shdr *)gOut.mRawWork;
        gOut.Bits32.mpFileHdr->e_shoff = gOut.mRawWork - gOut.mRawBase;
        gOut.mRawWork += sizeof(Elf32_Shdr) * gOut.mSectionCount;

        gOut.Bits32.mpSecHdrs[SECIX_SEC_STR].sh_offset = gOut.mRawWork - gOut.mRawBase;
        gOut.Bits32.mpSecHdrs[SECIX_SEC_STR].sh_size = gOut.mSecStrTotalBytes;
        gOut.mpSecStrBase = (char *)gOut.mRawWork;
        gOut.mpSecStrWork = gOut.mpSecStrBase + 1;
        gOut.mRawWork += gOut.Bits32.mpSecHdrs[SECIX_SEC_STR].sh_size;

        gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_offset = gOut.mRawWork - gOut.mRawBase;
        gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_size = gOut.mSymStrTotalBytes;
        gOut.mpSymStrBase = (char *)gOut.mRawWork;
        gOut.mRawWork += gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_size;

        gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_offset = gOut.mRawWork - gOut.mRawBase;
        gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_size = sizeof(Elf32_Sym) * 2;  // null symbol and xdl anchor symbol
        gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_info = 1;
        for (ix = 0;ix < XDLProgDataType_Count;ix++)
        {
            if (gOut.mOutSeg[ix].mCount != 0)
            {
                gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_size += (1 + gOut.mOutSeg[ix].mCount) * sizeof(Elf32_Sym);
            }
        }
        gOut.Bits32.mpSymBase = (Elf32_Sym *)gOut.mRawWork;
        gOut.Bits32.mpSymWork = gOut.Bits32.mpSymBase + 2;    // null symbol and xdl anchor symbol
        gOut.mRawWork += gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_size;

        gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_offset = gOut.mRawWork - gOut.mRawBase;
        gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_addr = (Elf32_Addr)gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_offset;
        gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_size = sizeof(XDL_ELF_ANCHOR);
        gOut.mpAnchor = (XDL_ELF_ANCHOR *)gOut.mRawWork;
        gOut.mRawWork += gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_size;

        if (gOut.mTotalExports > 0)
        {
            gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_offset = gOut.mRawWork - gOut.mRawBase;
            gOut.Bits32.mpSecRelocBase = (Elf32_Rel *)gOut.mRawWork;
            gOut.Bits32.mpSecRelocWork = gOut.Bits32.mpSecRelocBase;
            for (ix = 0;ix < XDLProgDataType_Count;ix++)
            {
                if (gOut.mOutSeg[ix].mCount > 0)
                    gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_size += sizeof(Elf32_Rel);
            }
            gOut.mRawWork += gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_size;

            for (ix = 0;ix < XDLProgDataType_Count;ix++)
                sPlaceSection(ix);
        }

        if ((gOut.mRawWork - gOut.mRawBase) != gOut.mFileSizeBytes)
        {
            printf("*** Internal error creating export\n");
            return false;
        }

        gOut.Bits32.mpFileHdr->e_ident[EI_MAG0] = ELFMAG0;
        gOut.Bits32.mpFileHdr->e_ident[EI_MAG1] = ELFMAG1;
        gOut.Bits32.mpFileHdr->e_ident[EI_MAG2] = ELFMAG2;
        gOut.Bits32.mpFileHdr->e_ident[EI_MAG3] = ELFMAG3;
        gOut.Bits32.mpFileHdr->e_ident[EI_CLASS] = ELFCLASS32;
        gOut.Bits32.mpFileHdr->e_ident[EI_DATA] = ELFDATA2LSB;
        gOut.Bits32.mpFileHdr->e_ident[EI_VERSION] = EV_CURRENT;
        gOut.Bits32.mpFileHdr->e_type = ET_REL;
        gOut.Bits32.mpFileHdr->e_machine = gOut.mElfMachine;
        gOut.Bits32.mpFileHdr->e_version = EV_CURRENT;
        if (gOut.mElfMachine == EM_A32)
            gOut.Bits32.mpFileHdr->e_flags = EF_A32_ABIVER;
        gOut.Bits32.mpFileHdr->e_ehsize = sizeof(Elf32_Ehdr);
        gOut.Bits32.mpFileHdr->e_shentsize = sizeof(Elf32_Shdr);
        gOut.Bits32.mpFileHdr->e_shnum = gOut.mSectionCount;
        gOut.Bits32.mpFileHdr->e_shstrndx = SECIX_SEC_STR;

        // section header strings
        gOut.Bits32.mpSecHdrs[SECIX_SEC_STR].sh_name = ((UINT_PTR)gOut.mpSecStrWork) - ((UINT_PTR)gOut.mpSecStrBase);
        K2ASC_Copy(gOut.mpSecStrWork, sgpSecStr_SecStr);
        gOut.mpSecStrWork += K2ASC_Len(sgpSecStr_SecStr) + 1;
        gOut.Bits32.mpSecHdrs[SECIX_SEC_STR].sh_type = SHT_STRTAB;
        gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_flags = SHF_STRINGS;
        gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_addralign = 1;

        // symbol strings
        gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_name = ((UINT_PTR)gOut.mpSecStrWork) - ((UINT_PTR)gOut.mpSecStrBase);
        K2ASC_Copy(gOut.mpSecStrWork, sgpSecStr_SymStr);
        gOut.mpSecStrWork += K2ASC_Len(sgpSecStr_SymStr) + 1;
        gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_type = SHT_STRTAB;
        gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_flags = SHF_STRINGS;
        gOut.Bits32.mpSecHdrs[SECIX_SYM_STR].sh_addralign = 1;

        // symbol table
        gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_name = ((UINT_PTR)gOut.mpSecStrWork) - ((UINT_PTR)gOut.mpSecStrBase);
        K2ASC_Copy(gOut.mpSecStrWork,sgpSecStr_Sym);
        gOut.mpSecStrWork += K2ASC_Len(sgpSecStr_Sym) + 1;
        gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_type = SHT_SYMTAB;
        gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_addralign = 4;
        gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_link = SECIX_SYM_STR;
        gOut.Bits32.mpSecHdrs[SECIX_SYM].sh_entsize = sizeof(Elf32_Sym);

        // xdl anchor
        gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_name = ((UINT_PTR)gOut.mpSecStrWork) - ((UINT_PTR)gOut.mpSecStrBase);
        K2ASC_Copy(gOut.mpSecStrWork, sgpSecStr_XDLAnchor);
        gOut.mpSecStrWork += K2ASC_Len(sgpSecStr_XDLAnchor) + 1;
        gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_flags = SHF_ALLOC;
        gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_type = SHT_PROGBITS;
        gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_addralign = 4;

        // xdl anchor symbol is always symbol 1, and its string is always at offset 1
        gOut.Bits32.mpSymBase[1].st_name = 1;
        K2ASC_Copy(gOut.mpSymStrBase + gOut.Bits32.mpSymBase[1].st_name, sgpSymInfo);
        gOut.Bits32.mpSymBase[1].st_shndx = SECIX_ANCHOR;
        gOut.Bits32.mpSymBase[1].st_size = sizeof(XDL_ELF_ANCHOR);
        // object file (REL, not EXEC), so symbol values are section relative, not absolute
        gOut.Bits32.mpSymBase[1].st_value = 0; // gOut.Bits32.mpSecHdrs[SECIX_ANCHOR].sh_addr;
        gOut.Bits32.mpSymBase[1].st_info = ELF32_MAKE_SYMBOL_INFO(STB_GLOBAL, STT_OBJECT);
        K2MEM_Copy(gOut.mpAnchor, &gOut.Anchor, sizeof(XDL_ELF_ANCHOR));
        K2MEM_Copy(&gOut.mpAnchor->Id, &gOut.Id, sizeof(K2_GUID128));

        if (gOut.mTotalExports > 0)
        {
            gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_name = ((UINT_PTR)gOut.mpSecStrWork) - ((UINT_PTR)gOut.mpSecStrBase);
            K2ASC_Copy(gOut.mpSecStrWork, sgpSecStr_XDLAnchorReloc);
            gOut.mpSecStrWork += K2ASC_Len(sgpSecStr_XDLAnchorReloc) + 1;
            gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_type = SHT_REL;
            gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_addralign = 4;
            gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_link = SECIX_SYM;
            gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_info = SECIX_ANCHOR;
            gOut.Bits32.mpSecHdrs[SECIX_ANCHOR_RELOC].sh_entsize = sizeof(Elf32_Rel);

            for (ix = 0;ix < XDLProgDataType_Count;ix++)
                sEmitSection(ix);
        }

        if (0 != K2ELF32_Parse((UINT8 const *)gOut.Bits32.mpFileHdr, gOut.mFileSizeBytes, &elfFile))
        {
            printf("*** Emitted file did not verify correctly\n");
            return false;
        }

        hOutFile = CreateFile(gOut.mpOutputFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hOutFile == INVALID_HANDLE_VALUE)
        {
            printf("*** Error creating output file\n");
            return false;
        }

        ret = (TRUE == WriteFile(hOutFile, gOut.Bits32.mpFileHdr, gOut.mFileSizeBytes, (LPDWORD)&ix, NULL));

        CloseHandle(hOutFile);

        if (!ret)
            DeleteFile(gOut.mpOutputFilePath);

    } while (false);

    delete[] ((UINT8 *)gOut.mRawBase);

    return ret;
}

K2STAT
DoExport(
    void
    )
{
    K2TREE_NODE *   pTreeNode;
    GLOBAL_SYMBOL * pGlob;
    EXPORT_SPEC *   pSpec;
    EXPORT_SPEC *   pPrev;
    EXPORT_SPEC *   pHold;
    EXPORT_SPEC *   pSpecEnd;
    int             result;
    Elf32_Shdr *    pSecHdr;

    if (gOut.mTotalExports > 0)
    {
        result = 0;
        pSpec = gOut.mOutSeg[XDLProgData_Text].mpSpecList;
        while (pSpec != NULL)
        {
            pTreeNode = K2TREE_Find(&gOut.SymbolTree, (UINT_PTR)pSpec->mpName);
            if (pTreeNode == NULL)
            {
                printf("*** Exported global code symbol \"%s\" was not found in any input file\n", pSpec->mpName);
                return K2STAT_ERROR_NOT_FOUND;
                result = -100;
            }
            else
            {
                pGlob = K2_GET_CONTAINER(GLOBAL_SYMBOL, pTreeNode, TreeNode);
                if (!pGlob->mIsCode)
                {
                    printf("*** Exported global code symbol \"%s\" found, but is as data\n", pSpec->mpName);
                    result = -101;
                }
                else if (pGlob->mIsWeak)
                {
                    printf("*** Exported global code symbol \"%s\" cannot be weak\n", pSpec->mpName);
                    result = -102;
                }
                else
                {
                    pSecHdr = (Elf32_Shdr *)(pGlob->mpObjFile->Bits32.Parse.mpSectionHeaderTable + (pGlob->mpObjFile->Bits32.Parse.mSectionHeaderTableEntryBytes * pGlob->Bits32.mpSymEnt->st_shndx));
                    if ((pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK) == XDL_ELF_SHF_TYPE_IMPORTS)
                    {
                        printf("** Exported global code symbol \"%s\" cannot be an import\n", pSpec->mpName);
                        result = -107;
                    }
                }
            }
            pSpec = pSpec->mpNext;
        }
        if (result != 0)
            return result;
        //    printf("Code symbols all found ok\n");

        result = 0;
        pSpec = gOut.mOutSeg[XDLProgData_Data].mpSpecList;
        pSpecEnd = NULL;
        pPrev = NULL;
        while (pSpec != NULL)
        {
            pTreeNode = K2TREE_Find(&gOut.SymbolTree, (UINT_PTR)pSpec->mpName);
            if (pTreeNode == NULL)
            {
                printf("*** Exported global data symbol \"%s\" was not found in any input file\n", pSpec->mpName);
                result = -103;
                pSpec = pSpec->mpNext;
            }
            else
            {
                pGlob = K2_GET_CONTAINER(GLOBAL_SYMBOL, pTreeNode, TreeNode);
                if (pGlob->mIsCode)
                {
                    printf("*** Exported global data symbol \"%s\" found, but is as code\n", pSpec->mpName);
                    result = -104;
                }
                else if (pGlob->mIsWeak)
                {
                    printf("*** Exported global data symbol \"%s\" cannot be weak\n", pSpec->mpName);
                    result = -105;
                }
                else
                {
                    pSecHdr = (Elf32_Shdr *)(pGlob->mpObjFile->Bits32.Parse.mpSectionHeaderTable + (pGlob->mpObjFile->Bits32.Parse.mSectionHeaderTableEntryBytes * pGlob->Bits32.mpSymEnt->st_shndx));
                    if ((pSecHdr->sh_flags & XDL_ELF_SHF_TYPE_MASK) == XDL_ELF_SHF_TYPE_IMPORTS)
                    {
                        printf("** Exported global data or read symbol \"%s\" cannot be an import\n", pSpec->mpName);
                        result = -107;
                    }
                }
                
                if (pGlob->mIsRead)
                {
                    pHold = pSpec->mpNext;

                    if (pPrev == NULL)
                        gOut.mOutSeg[XDLProgData_Data].mpSpecList = pSpec->mpNext;
                    else
                        pPrev->mpNext = pSpec->mpNext;
                    pSpec->mpNext = NULL;
                    gOut.mOutSeg[XDLProgData_Data].mCount--;

                    if (pSpecEnd != NULL)
                        pSpecEnd->mpNext = pSpec;
                    else
                        gOut.mOutSeg[XDLProgData_Read].mpSpecList = pSpec;
                    pSpecEnd = pSpec;
                    gOut.mOutSeg[XDLProgData_Read].mCount++;

                    pSpec = pHold;
                }
                else
                {
                    pPrev = pSpec;
                    pSpec = pSpec->mpNext;
                }
            }
        }
        if (result != 0)
            return result;
        //    printf("Data symbols all found ok.  %d are read, %d are data\n", gOut.mOutSeg[XDLProgData_Read].mCount, gOut.mOutSeg[XDLProgData_Data].mCount);
    }

    return sCreateOutputFile() ? 0 : -106;
}


