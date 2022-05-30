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
#ifndef __K2ELF_H
#define __K2ELF_H

#include <k2systype.h>

//
//------------------------------------------------------------------------
//

#define K2ELF32_MIN_FILE_SIZE  (sizeof(Elf32_Ehdr) + (2 * sizeof(Elf32_Shdr)))
#define K2ELF64_MIN_FILE_SIZE  (sizeof(Elf64_Ehdr) + (2 * sizeof(Elf64_Shdr)))

#if K2_TARGET_ARCH_IS_INTEL
#if K2_TARGET_ARCH_IS_32BIT
#define K2ELF32_TARGET_MACHINE_TYPE EM_X32
#else
#define K2ELF32_TARGET_MACHINE_TYPE EM_X64
#endif
#else
#if K2_TARGET_ARCH_IS_32BIT
#define K2ELF32_TARGET_MACHINE_TYPE EM_A32
#else
#define K2ELF32_TARGET_MACHINE_TYPE EM_A64
#endif
#endif


#ifdef __cplusplus

class K2Elf32Section;
class K2Elf64Section;

extern "C" typedef struct _K2ELF32PARSE K2ELF32PARSE;
extern "C" typedef struct _K2ELF64PARSE K2ELF64PARSE;

class K2Elf32File
{
public:
    static K2STAT Create(UINT8 const *apBuf, UINT_PTR aBufBytes, K2Elf32File **appRetFile);
    static K2STAT Create(K2ELF32PARSE const *apParse, K2Elf32File **appRetFile);

    INT_PTR AddRef(void);
    INT_PTR Release(void);

    Elf32_Ehdr const & Header(void) const
    {
        return *((Elf32_Ehdr const *)mpBuf);
    }
    K2Elf32Section const & Section(UINT_PTR aIndex) const
    {
        if (aIndex >= Header().e_shnum)
            aIndex = 0;
        return *mppSection[aIndex];
    }
    UINT8 const * RawData(void) const
    {
        return mpBuf;
    }
    UINT_PTR const & SizeBytes(void) const
    {
        return mSizeBytes;
    }

private:
    K2Elf32File(
        UINT8 const * apBuf,
        UINT_PTR aSizeBytes
    ) : mpBuf(apBuf),
        mSizeBytes(aSizeBytes)
    {
        mppSection = NULL;
        mRefs = 1;
    }
    virtual ~K2Elf32File(void);

    UINT8 const * const mpBuf;
    UINT_PTR const      mSizeBytes;

    K2Elf32Section **   mppSection;
    INT_PTR volatile    mRefs;
};

class K2Elf32Section
{
public:
    K2Elf32File const & File(void) const
    {
        return mFile;
    }
    UINT_PTR IndexInFile(void) const
    {
        return mIndex;
    }
    Elf32_Shdr const & Header(void) const
    {
        return mHdr;
    }
    UINT8 const * RawData(void) const
    {
        return File().RawData() + mHdr.sh_offset;
    }

protected:
    K2Elf32Section(
        K2Elf32File const & aFile,
        UINT_PTR aIndex,
        Elf32_Shdr const * apHdr
    ) : mFile(aFile), 
        mIndex(aIndex),
        mHdr(*apHdr)
    {
    }
    virtual ~K2Elf32Section(void)
    {
    }

private:
    friend class K2Elf32File;
    K2Elf32File const & mFile;
	UINT_PTR const      mIndex;
	Elf32_Shdr const &  mHdr;
};

class K2Elf32SymbolSection : public K2Elf32Section
{
public:
    UINT_PTR const & EntryCount(void) const
    {
        return mSymCount;
    }
    Elf32_Sym const & Symbol(UINT_PTR aIndex) const
    {
        if (aIndex >= mSymCount)
            aIndex = 0;
        return *((Elf32_Sym const *)(RawData() + (aIndex * Header().sh_entsize)));
    }
    K2Elf32Section const & StringSection(void) const
    {
        return File().Section(Header().sh_link);
    }

private:
    friend class K2Elf32File;
    K2Elf32SymbolSection(
        K2Elf32File const & aFile,
        UINT_PTR aIndex,
        Elf32_Shdr const * apHdr
    ) : K2Elf32Section(aFile, aIndex, apHdr),
        mSymCount(apHdr->sh_size / apHdr->sh_entsize)
    {
    }
    ~K2Elf32SymbolSection(void)
    {
    }
    UINT_PTR const mSymCount;
};

class K2Elf32RelocSection : public K2Elf32Section
{
public:
    UINT_PTR const & EntryCount(void) const
    {
        return mRelCount;
    }

    Elf32_Rel const & Reloc(UINT_PTR aIndex) const
    {
        if (aIndex >= mRelCount)
            aIndex = 0;
        return *((Elf32_Rel const *)(RawData() + (aIndex * Header().sh_entsize)));
    }
    K2Elf32Section const & TargetSection(void) const
    {
        return File().Section(Header().sh_info);
    }
    K2Elf32SymbolSection const & SymbolSection(void) const
    {
        return (K2Elf32SymbolSection const &)File().Section(Header().sh_link);
    }

private:
    friend class K2Elf32File;
    K2Elf32RelocSection(
        K2Elf32File const & aFile,
        UINT_PTR aIndex,
        Elf32_Shdr const * apHdr
    ) : K2Elf32Section(aFile, aIndex, apHdr),
        mRelCount(apHdr->sh_size / apHdr->sh_entsize)
    {
    }
    ~K2Elf32RelocSection(void)
    {
    }
    UINT_PTR const mRelCount;
};

class K2Elf64File
{
public:
    static K2STAT Create(UINT8 const *apBuf, UINT_PTR aBufBytes, K2Elf64File **appRetFile);
    static K2STAT Create(K2ELF64PARSE const *apParse, K2Elf64File **appRetFile);

    INT_PTR AddRef(void);
    INT_PTR Release(void);

    Elf64_Ehdr const & Header(void) const
    {
        return *((Elf64_Ehdr const *)mpBuf);
    }
    K2Elf64Section const & Section(UINT_PTR aIndex) const
    {
        if (aIndex >= Header().e_shnum)
            aIndex = 0;
        return *mppSection[aIndex];
    }
    UINT8 const * RawData(void) const
    {
        return mpBuf;
    }
    UINT_PTR const & SizeBytes(void) const
    {
        return mSizeBytes;
    }

private:
    K2Elf64File(
        UINT8 const * apBuf,
        UINT_PTR aSizeBytes
    ) : mpBuf(apBuf),
        mSizeBytes(aSizeBytes)
    {
        mppSection = NULL;
        mRefs = 1;
    }
    virtual ~K2Elf64File(void);

    UINT8 const * const mpBuf;
    UINT_PTR const      mSizeBytes;

    K2Elf64Section **   mppSection;
    INT_PTR volatile    mRefs;
};

class K2Elf64Section
{
public:
    K2Elf64File const & File(void) const
    {
        return mFile;
    }
    UINT_PTR IndexInFile(void) const
    {
        return mIndex;
    }
    Elf64_Shdr const & Header(void) const
    {
        return mHdr;
    }
    UINT8 const * RawData(void) const
    {
        return File().RawData() + mHdr.sh_offset;
    }

protected:
    K2Elf64Section(
        K2Elf64File const & aFile,
        UINT_PTR aIndex,
        Elf64_Shdr const * apHdr
    ) : mFile(aFile),
        mIndex(aIndex),
        mHdr(*apHdr)
    {
    }
    virtual ~K2Elf64Section(void)
    {
    }

private:
    friend class K2Elf64File;
    K2Elf64File const & mFile;
    UINT_PTR const      mIndex;
    Elf64_Shdr const &  mHdr;
};

class K2Elf64SymbolSection : public K2Elf64Section
{
public:
    UINT_PTR const & EntryCount(void) const
    {
        return mSymCount;
    }
    Elf64_Sym const & Symbol(UINT_PTR aIndex) const
    {
        if (aIndex >= mSymCount)
            aIndex = 0;
        return *((Elf64_Sym const *)(RawData() + (aIndex * Header().sh_entsize)));
    }
    K2Elf64Section const & StringSection(void) const
    {
        return File().Section(Header().sh_link);
    }

private:
    friend class K2Elf64File;
    K2Elf64SymbolSection(
        K2Elf64File const & aFile,
        UINT_PTR aIndex,
        Elf64_Shdr const * apHdr
    ) : K2Elf64Section(aFile, aIndex, apHdr),
        mSymCount((UINT_PTR)(apHdr->sh_size / apHdr->sh_entsize))
    {
    }
    ~K2Elf64SymbolSection(void)
    {
    }
    UINT_PTR const mSymCount;
};

class K2Elf64RelocSection : public K2Elf64Section
{
public:
    UINT_PTR const & EntryCount(void) const
    {
        return mRelCount;
    }

    Elf64_Rel const & Reloc(UINT_PTR aIndex) const
    {
        if (aIndex >= mRelCount)
            aIndex = 0;
        return *((Elf64_Rel const *)(RawData() + (aIndex * Header().sh_entsize)));
    }
    K2Elf64Section const & TargetSection(void) const
    {
        return File().Section(Header().sh_info);
    }
    K2Elf64SymbolSection const & SymbolSection(void) const
    {
        return (K2Elf64SymbolSection const &)File().Section(Header().sh_link);
    }

private:
    friend class K2Elf64File;
    K2Elf64RelocSection(
        K2Elf64File const & aFile,
        UINT_PTR aIndex,
        Elf64_Shdr const * apHdr
    ) : K2Elf64Section(aFile, aIndex, apHdr),
        mRelCount((UINT_PTR)(apHdr->sh_size / apHdr->sh_entsize))
    {
    }
    ~K2Elf64RelocSection(void)
    {
    }
    UINT_PTR const mRelCount;
};

extern "C" {
#endif

//
//------------------------------------------------------------------------
//

struct _K2ELF32PARSE
{
    Elf32_Ehdr const *  mpRawFileData;
    UINT_PTR            mRawFileByteCount;

    UINT_PTR            mFileHeaderBytes;

    UINT8 const *       mpSectionHeaderTable;
    UINT_PTR            mSectionHeaderTableEntryBytes;
    UINT_PTR            mSectionHeaderTableBytes;

    UINT8 const *       mpProgramHeaderTable;
    UINT_PTR            mProgramHeaderTableEntryBytes;
    UINT_PTR            mProgramHeaderTableBytes;
};
typedef struct _K2ELF32PARSE K2ELF32PARSE;

K2STAT
K2ELF32_ValidateHeader(
    UINT8 const *   apHdrBuffer,
    UINT_PTR        aHdrBytes,
    UINT_PTR        aFileSizeBytes
);

K2STAT
K2ELF32_Parse(
    UINT8 const *   apFileData,
    UINT_PTR        aFileSizeBytes,
    K2ELF32PARSE *  apRetParse
    );

Elf32_Shdr const *
K2ELF32_GetSectionHeader(
    K2ELF32PARSE const *    apParse,
    UINT_PTR                aIndex
    );

void const *
K2ELF32_GetSectionData(
    K2ELF32PARSE const *    apParse,
    UINT_PTR                aIndex
    );

Elf32_Phdr const *
K2ELF32_GetProgramHeader(
    K2ELF32PARSE const *    apParse,
    UINT_PTR                aIndex
    );

//
//------------------------------------------------------------------------
//

struct _K2ELF64PARSE
{
    Elf64_Ehdr const *  mpRawFileData;
    UINT_PTR            mRawFileByteCount;

    UINT_PTR            mFileHeaderBytes;

    UINT8 const *       mpSectionHeaderTable;
    UINT_PTR            mSectionHeaderTableEntryBytes;
    UINT_PTR            mSectionHeaderTableBytes;

    UINT8 const *       mpProgramHeaderTable;
    UINT_PTR            mProgramHeaderTableEntryBytes;
    UINT_PTR            mProgramHeaderTableBytes;
};
typedef struct _K2ELF64PARSE K2ELF64PARSE;

K2STAT
K2ELF64_ValidateHeader(
    UINT8 const *   apHdrBuffer,
    UINT_PTR        aHdrBytes,
    UINT_PTR        aFileSizeBytes
);

K2STAT
K2ELF64_Parse(
    UINT8 const *   apFileData,
    UINT_PTR        aFileSizeBytes,
    K2ELF64PARSE *  apRetParse
);

Elf64_Shdr const *
K2ELF64_GetSectionHeader(
    K2ELF64PARSE const *    apParse,
    UINT_PTR                aIndex
);

void const *
K2ELF64_GetSectionData(
    K2ELF64PARSE const *    apParse,
    UINT_PTR                aIndex
);

Elf64_Phdr const *
K2ELF64_GetProgramHeader(
    K2ELF64PARSE const *    apParse,
    UINT_PTR                aIndex
);

//
//------------------------------------------------------------------------
//

#define K2STAT_ERROR_ELF_INVALID_FILE_TYPE              K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00000)
#define K2STAT_ERROR_ELF_INVALID_PHENTSIZE              K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00001)
#define K2STAT_ERROR_ELF_INVALID_PHNUM                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00002)
#define K2STAT_ERROR_ELF_INVALID_PHOFF                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00003)
#define K2STAT_ERROR_ELF_INVALID_SHENTSIZE              K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00004)
#define K2STAT_ERROR_ELF_INVALID_SHNUM                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00005)
#define K2STAT_ERROR_ELF_INVALID_SHOFF                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00006)
#define K2STAT_ERROR_ELF_INVALID_SHSTRNDX               K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00007)
#define K2STAT_ERROR_ELF_TABLES_OVERLAP                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00008)
#define K2STAT_ERROR_ELF_NOT_ELF_FILE                   K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00009)
#define K2STAT_ERROR_ELF_NOT_SUPPORTED_VERSION          K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x0000A)
#define K2STAT_ERROR_ELF_UNKNOWN_FILE_CLASS             K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x0000B)
#define K2STAT_ERROR_ELF_SECHDR_0_INVALID               K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x0000C)
#define K2STAT_ERROR_ELF_SHSTRNDX_BAD_TYPE              K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x0000D)
#define K2STAT_ERROR_ELF_SECTION_OVERLAPS_TABLES        K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x0000E)
#define K2STAT_ERROR_ELF_INVALID_SYMTAB_ENTSIZE         K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x0000F)
#define K2STAT_ERROR_ELF_INVALID_SYMTAB_SHLINK_IX       K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00010)
#define K2STAT_ERROR_ELF_INVALID_SYMTAB_SHLINK_TARGET   K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00011)
#define K2STAT_ERROR_ELF_INVALID_SYMBOL_NAME            K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00012)
#define K2STAT_ERROR_ELF_INVALID_SOURCEFILE_SYMBOL      K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00013)
#define K2STAT_ERROR_ELF_INVALID_SYMBOL_SHNDX           K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00014)
#define K2STAT_ERROR_ELF_INVALID_SYMBOL_ADDR            K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00015)
#define K2STAT_ERROR_ELF_INVALID_SYMBOL_SIZE            K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00016)
#define K2STAT_ERROR_ELF_INVALID_SYMBOL_OFFSET          K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00017)
#define K2STAT_ERROR_ELF_INVALID_SYMBOL_SECTION         K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00018)
#define K2STAT_ERROR_ELF_INVALID_RELOCS_SHLINK_IX       K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x00019)
#define K2STAT_ERROR_ELF_INVALID_RELOCS_SHINFO_IX       K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x0001A)
#define K2STAT_ERROR_ELF_INVALID_RELOC_ENTSIZE          K2STAT_MAKE_ERROR(K2STAT_FACILITY_ELF, 0x0001B)

#ifdef __cplusplus
};   // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __K2ELF_H

