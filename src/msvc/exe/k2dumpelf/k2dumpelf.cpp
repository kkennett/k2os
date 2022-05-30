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
    UINT_PTR alignptr;
    UINT32 ix;
    char const *pNames;
    DLX_INFO32 const *pDlxInfo;
    DLX_IMPORT const *pImport;
    K2Elf32File *pElfFile;
    stat = K2Elf32File::Create(apParse, &pElfFile);
    if (!K2STAT_IS_ERROR(stat))
    {
        do {
            if (pElfFile->Header().e_type == DLX_ET_DLX)
            {
                printf("DLX FILE\n");

                if (pElfFile->Header().e_ident[EI_OSABI] != DLX_ELFOSABI_K2)
                {
                    printf("  *** Incorrect OSABI in header\n");
                    break;
                }

                if (pElfFile->Header().e_ident[EI_ABIVERSION] != DLX_ELFOSABIVER_DLX)
                {
                    printf("  *** Incorrect OSABIVERSION in header\n");
                    break;
                }

                if (pElfFile->Header().e_shnum < 3)
                {
                    printf("  *** Number of section headers is too small.\n");
                    break;
                }

                if (pElfFile->Header().e_shstrndx != DLX_SHN_SHSTR)
                {
                    printf("  *** section header strings are not in the right place.\n");
                    break;
                }

                Elf32_Shdr const &dlxInfoSecHdr = pElfFile->Section(DLX_SHN_DLXINFO).Header();

                if (dlxInfoSecHdr.sh_flags != (DLX_SHF_TYPE_DLXINFO | SHF_ALLOC))
                {
                    printf("  *** DLXINFO section flags are incorrect.\n");
                    break;
                }

                if (pElfFile->Header().e_flags & DLX_EF_KERNEL_ONLY)
                {
                    printf("  KERNEL MODE\n");
                }
                else
                {
                    printf("  USER MODE\n");
                }

                pDlxInfo = (DLX_INFO32 const *)pElfFile->Section(DLX_SHN_DLXINFO).RawData();
                printf("  ID       %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                    pDlxInfo->ID.mData1, pDlxInfo->ID.mData2, pDlxInfo->ID.mData3,
                    pDlxInfo->ID.mData4[0], pDlxInfo->ID.mData4[1],
                    pDlxInfo->ID.mData4[2], pDlxInfo->ID.mData4[3],
                    pDlxInfo->ID.mData4[4], pDlxInfo->ID.mData4[5],
                    pDlxInfo->ID.mData4[6], pDlxInfo->ID.mData4[7]);

                printf("  FILENAME \"%s\"\n", pDlxInfo->mFileName);

                printf("  STACKREQ %d\n", pDlxInfo->mEntryStackReq);

                printf("  %d IMPORTS\n", pDlxInfo->mImportCount);

                if (pDlxInfo->mImportCount)
                {
                    alignptr = ((UINT_PTR)pDlxInfo) + sizeof(DLX_INFO32) + strlen(pDlxInfo->mFileName) - 4;
                    alignptr = (alignptr + 4) & ~3;
                    pImport = (DLX_IMPORT const *)alignptr;
                    for (ix = 0; ix < pDlxInfo->mImportCount; ix++)
                    {
                        printf("    %02d: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X %s\n", ix,
                            pImport->ID.mData1, pImport->ID.mData2, pImport->ID.mData3,
                            pImport->ID.mData4[0], pImport->ID.mData4[1],
                            pImport->ID.mData4[2], pImport->ID.mData4[3],
                            pImport->ID.mData4[4], pImport->ID.mData4[5],
                            pImport->ID.mData4[6], pImport->ID.mData4[7],
                            pImport->mFileName);
                        alignptr += pImport->mSizeBytes;
                        K2_ASSERT((alignptr & 3) == 0);
                        pImport = (DLX_IMPORT const *)alignptr;
                    }
                }

                printf("  SEGMENTS:\n");
                for (ix = 0; ix < DlxSeg_Count; ix++)
                {
                    printf("    %d: crc %08X file %08X offs %08X link %08X mem %08X\n",
                        ix,
                        pDlxInfo->SegInfo[ix].mCRC32,
                        pDlxInfo->SegInfo[ix].mFileBytes,
                        pDlxInfo->SegInfo[ix].mFileOffset,
                        pDlxInfo->SegInfo[ix].mLinkAddr,
                        pDlxInfo->SegInfo[ix].mMemActualBytes);
                }
            }

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