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
#include <lib/k2win32.h>
#pragma warning(disable: 4477)
#include <lib/k2elf.h>
#include <lib/k2sort.h>
#include <lib/k2mem.h>
#include <lib/k2atomic.h>


class K2Elf32Section;
class K2Elf64Section;

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

K2Elf32File::~K2Elf32File(
    void
)
{
    UINT32 ix;

    if (mppSection != NULL)
    {
        for (ix = 0; ix < Header().e_shnum; ix++)
        {
            if (mppSection[ix] != NULL)
                delete mppSection[ix];
        }
        delete[] mppSection;
    }
}

K2STAT
K2Elf32File::Create(
    UINT8 const *   apBuf,
    UINT_PTR        aBufBytes,
    K2Elf32File **  appRetFile
)
{
    K2STAT          stat;
    K2ELF32PARSE    parse;

    stat = K2ELF32_Parse(apBuf, aBufBytes, &parse);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    return Create(&parse, appRetFile);
}

K2STAT
K2Elf32File::Create(
    K2ELF32PARSE const *    apParse,
    K2Elf32File **          appRetFile
)
{
    K2STAT              stat;
    K2Elf32File *       pRet;
    UINT32              hdrIx;
    K2Elf32Section **   ppSec;
    Elf32_Shdr const *  pSecHdr;

    if ((apParse == NULL) ||
        (appRetFile == NULL))
        return K2STAT_ERROR_BAD_ARGUMENT;

    pRet = new K2Elf32File((UINT8 const *)apParse->mpRawFileData, apParse->mRawFileByteCount);
    if (pRet == NULL)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    stat = K2STAT_OK;

    do
    {
        if (apParse->mpRawFileData->e_shnum == 0)
            break;

        ppSec = new K2Elf32Section * [apParse->mpRawFileData->e_shnum];
        if (ppSec == NULL)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }

        stat = K2STAT_OK;

        K2MEM_Zero(ppSec, sizeof(K2Elf32Section *) * apParse->mpRawFileData->e_shnum);

        for (hdrIx = 0; hdrIx < apParse->mpRawFileData->e_shnum; hdrIx++)
        {
            pSecHdr = K2ELF32_GetSectionHeader(apParse, hdrIx);
            if (pSecHdr->sh_type == SHT_SYMTAB)
                ppSec[hdrIx] = new K2Elf32SymbolSection(*pRet, hdrIx, pSecHdr);
            else if (pSecHdr->sh_type == SHT_REL)
                ppSec[hdrIx] = new K2Elf32RelocSection(*pRet, hdrIx, pSecHdr);
            else
                ppSec[hdrIx] = new K2Elf32Section(*pRet, hdrIx, pSecHdr);
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
K2Elf32File::AddRef(
    void
)
{
    return K2ATOMIC_Inc(&mRefs);
}

INT_PTR
K2Elf32File::Release(
    void
)
{
    INT_PTR ret;

    ret = K2ATOMIC_Dec(&mRefs);
    if (ret == 0)
        delete this;

    return ret;
}




static
void
myAssert(
    char const *    apFile,
    int             aLineNum,
    char const *    apCondition
)
{	
    DebugBreak();
}

extern "C" K2_pf_ASSERT K2_Assert = myAssert;

static int 
sSymCompare(void const *aPtr1, void const *aPtr2)
{
    UINT32 v1;
    UINT32 v2;

    v1 = ((Elf32_Sym const *)aPtr1)->st_value;
    v2 = ((Elf32_Sym const *)aPtr2)->st_value;
    if (v1 < v2)
        return -1;
    if (v1 == v2)
        return 0;
    return 1;
}

void
DumpElf32(
    K2ELF32PARSE const *apParse
)
{
    K2STAT  stat;
    UINT32 sizeText = 0;
    UINT32 sizeRead = 0;
    UINT32 sizeData = 0;
    UINT32 align32;
    UINT32 ix;
    char const *pNames;
    K2Elf32File *pElfFile;
    stat = K2Elf32File::Create(apParse, &pElfFile);
    if (!K2STAT_IS_ERROR(stat))
    {
        do {
            printf("  ENTRY: %08X\n", pElfFile->Header().e_entry);

            printf("  SECTIONS:\n");
            if ((pElfFile->Header().e_shstrndx > 0) && (pElfFile->Header().e_shstrndx < pElfFile->Header().e_shnum))
            {
                pNames = (const char *)pElfFile->Section(pElfFile->Header().e_shstrndx).RawData();
            }
            else
            {
                pNames = NULL;
            }
            for (ix = 0; ix < pElfFile->Header().e_shnum; ix++)
            {
                Elf32_Shdr const &secHdr = pElfFile->Section(ix).Header();
                printf("    %02d %5d addr %08X size %08X off %08X %s\n", ix, secHdr.sh_type,
                    secHdr.sh_addr, secHdr.sh_size, secHdr.sh_offset,
                    (pNames != NULL) ? (pNames + secHdr.sh_name) : "");

                if (secHdr.sh_type == SHT_SYMTAB)
                {
                    K2Elf32SymbolSection const &symSec = (K2Elf32SymbolSection const &)pElfFile->Section(ix);
                    printf("      %d SYMBOLS\n", symSec.EntryCount());

                    UINT8 * pSyms = (UINT8 *)malloc(symSec.Header().sh_entsize * symSec.EntryCount());

                    CopyMemory(pSyms, symSec.RawData(), symSec.EntryCount() * symSec.Header().sh_entsize);

                    K2SORT_Quick(pSyms, symSec.EntryCount(), symSec.Header().sh_entsize, sSymCompare);

                    UINT32 jx;
                    for (jx = 0; jx < symSec.EntryCount(); jx++)
                    {
                        Elf32_Sym *pSym = (Elf32_Sym *)(pSyms + (jx * symSec.Header().sh_entsize));
                        printf("        %08X %s\n",
                            pSym->st_value,
                            (char const *)(symSec.StringSection().RawData() + pSym->st_name)
                        );
                    }

                    free(pSyms);
                }

                if (!(secHdr.sh_flags & SHF_ALLOC))
                    continue;

                align32 = (((UINT32)1) << secHdr.sh_addralign);

                if (secHdr.sh_flags & SHF_EXECINSTR)
                {
                    sizeText = ((sizeText + align32 - 1) / align32) * align32;
                    sizeText += secHdr.sh_size;
                }
                else if (secHdr.sh_flags & SHF_WRITE)
                {
                    sizeData = ((sizeData + align32 - 1) / align32) * align32;
                    sizeData += secHdr.sh_size;
                }
                else
                {
                    sizeRead = ((sizeRead + align32 - 1) / align32) * align32;
                    sizeRead += secHdr.sh_size;
                }
            }
            printf("  SIZES:\n    x %08I32X r %08I32X d %08I32X\n", sizeText, sizeRead, sizeData);

        } while (false);
    }
}

void
DumpElf64(
    K2ELF64PARSE const *apParse
)
{
}

int main(int argc, char **argv)
{
    UINT8 const *pData;

    if (argc < 2)
    {
        printf("\nk2dumpelf: Need an Arg\n\n");
        return -1;
    }

    K2ReadOnlyMappedFile *pFile = K2ReadOnlyMappedFile::Create(argv[1]);
    if (pFile == NULL)
    {
        printf("\nk2dumpelf: could not map in file \"%s\"\n", argv[1]);
        return -2;
    }

    K2STAT stat;
    pData = (UINT8 const *)pFile->DataPtr();
    if (ELFCLASS32 == pData[EI_CLASS])
    {
        K2ELF32PARSE parse32;
        stat = K2ELF32_Parse((UINT8 const *)pFile->DataPtr(), (UINT_PTR)pFile->FileBytes(), &parse32);
        if (!K2STAT_IS_ERROR(stat))
        {
            DumpElf32(&parse32);
        }
        else
        {
            printf("\nk2dumpelf: error %08X parsing 64-bit elf file \"%s\"\n", stat, argv[1]);
        }
    }
    else if (ELFCLASS64 == pData[EI_CLASS])
    {
        K2ELF64PARSE parse64;
        stat = K2ELF64_Parse((UINT8 const *)pFile->DataPtr(), (UINT_PTR)pFile->FileBytes(), &parse64);
        if (!K2STAT_IS_ERROR(stat))
        {
            DumpElf64(&parse64);
        }
        else
        {
            printf("\nk2dumpelf: error %08X parsing 64-bit elf file \"%s\"\n", stat, argv[1]);
        }
    }
    else
    {
        printf("\nk2dumpelf: unknown file class \"%s\"\n", argv[1]);
    }

    pFile->Release();

    return stat;
}