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

#define DUMP_EXPORTS     0

typedef union _WORKPTR WORKPTR;
union _WORKPTR
{
    UINT_PTR    mAsVal;
    void *      mAsPtr;
};

static char const * const sgpSecStr_SecStr = ".sechdrstr";
static char const * const sgpSecStr_SymStr = ".symstr";
static char const * const sgpSecStr_Sym = ".symtable";
static char const * const sgpSecStr_Imp = ".?import_";

K2STAT
CreateImportLibrary32(
    K2ELF32PARSE *  apParse
)
{
    UINT64 const *                      pSrcPtr;
    UINT_PTR                            ixExpSec;
    XDL_EXPORTS_SEGMENT_HEADER const *  pExpHdr;
    XDL_EXPORT32_REF *                  pExpRef;
    UINT_PTR                            ixExp;
    char const *                        pExpStr;
    UINT_PTR                            totalExportCount;
    UINT_PTR                            exportsStrSize[XDLProgDataType_Count];
    K2TREE_ANCHOR                       symTree;
    K2TREE_NODE *                       pTreeNode;
    WORKPTR                             workFile;
    WORKPTR                             work;
    WORKPTR                             workElfBase;
    UINT_PTR                            symStrBytes;
    UINT_PTR                            secStrBytes;
    UINT_PTR                            fileSizeBytes;
    UINT_PTR                            sectionCount;
    UINT_PTR                            symStrSecSize;
    UINT_PTR                            totalExportsStrSize;
    UINT_PTR                            expSegBytes;
    UINT_PTR                            outSecIx[XDLProgDataType_Count];
    UINT_PTR                            indexBytes;
    Elf_LibRec *                        pLibIndexRecord;
    UINT8 *                             pLibIndex;
    UINT32                              rev;
    Elf_LibRec *                        pOutLibRecord;
    Elf32_Ehdr *                        pOutHdr;
    Elf32_Shdr *                        pOutSecHdr;
    char *                              pOutSecHdrStr;
    Elf32_Sym *                         pOutSymTab;
    UINT_PTR                            ixSym;
    WORKPTR                             outSymStrBase;
    WORKPTR                             outSymStrWork;
    XDL_EXPORTS_SEGMENT_HEADER *        pOutExpSeg[XDLProgDataType_Count];
    char *                              pOutStr;
    UINT8                               symType;
    HANDLE                              hFile;
    DWORD                               wrot;
    BOOL                                ok;
    UINT_PTR                            roSecIx;

    K2TREE_Init(&symTree, TreeStrCompare);

//    printf("--- Create import library\n");

    pSrcPtr = (UINT64 *)&gOut.mpElfAnchor->mAnchor[0];
    totalExportCount = 0;
    totalExportsStrSize = 0;
    for (ixExpSec = 0; ixExpSec < XDLProgDataType_Count; ixExpSec++)
    {
        exportsStrSize[ixExpSec] = 0;
        if (pSrcPtr[ixExpSec] != 0)
        {
            pExpHdr = gOut.mpElfExpSegHdr[ixExpSec] =
                (XDL_EXPORTS_SEGMENT_HEADER const *)LoadAddrToDataPtr32(apParse, (UINT_PTR)pSrcPtr[ixExpSec], &roSecIx);
            if (pExpHdr == NULL)
            {
                printf("*** Exports type %d could not be found in ELF file\n", ixExpSec);
                return K2STAT_ERROR_NOT_FOUND;
            }
            if (0 == gOut.mElfRoSecIx)
            {
                pOutSecHdr = (Elf32_Shdr *)(apParse->mpSectionHeaderTable + (roSecIx * apParse->mSectionHeaderTableEntryBytes));
                if ((SHT_PROGBITS != pOutSecHdr->sh_type) ||
                    (0 == (SHF_ALLOC & pOutSecHdr->sh_flags)) ||
                    (0 != (SHF_WRITE & pOutSecHdr->sh_flags)) ||
                    (0 != (SHF_EXECINSTR & pOutSecHdr->sh_flags)) ||
                    (0 != (XDL_ELF_SHF_TYPE_IMPORTS & pOutSecHdr->sh_flags)))
                {
                    printf("*** Program exports are in the wront type of section\n");
                    return K2STAT_ERROR_BAD_ARGUMENT;
                }
                gOut.mElfRoSecIx = roSecIx;
            }
            else if (gOut.mElfRoSecIx != roSecIx)
            {
                printf("*** Exports type %d is not in same section as other exports (%d)\n", ixExpSec, gOut.mElfRoSecIx);
                return K2STAT_ERROR_BAD_ARGUMENT;
            }
#if DUMP_EXPORTS
            printf("Found Exports type %d at %08X from %08X\n", ixExpSec, (UINT_PTR)pExpHdr, (UINT_PTR)pSrcPtr[ixExpSec]);
            printf("  Count %d\n", pExpHdr->mCount);
            printf("  Crc32 %08X\n", pExpHdr->mCRC32);
#endif
            pExpRef = (XDL_EXPORT32_REF *)(((UINT8 const *)pExpHdr) + sizeof(XDL_EXPORTS_SEGMENT_HEADER));
            totalExportCount += pExpHdr->mCount;
            for (ixExp = 0; ixExp < pExpHdr->mCount; ixExp++)
            {
                pExpStr = ((char const *)pExpHdr) + pExpRef->mNameOffset;
#if DUMP_EXPORTS
                printf("    %08X %08X(%d) %s\n", pExpRef->mAddr, pExpRef->mNameOffset, pExpRef->mNameOffset, pExpStr);
#endif
                pTreeNode = K2TREE_Find(&symTree, (UINT_PTR)pExpStr);
                if (NULL != pTreeNode)
                {
                    printf("*** EXPORTED symbol \"%s\" is not uniquely named\n", pExpStr);
                    return K2STAT_ERROR_BAD_ARGUMENT;
                }
                pTreeNode = new K2TREE_NODE;
                if (NULL == pTreeNode)
                {
                    printf("*** out of memory\n");
                    return K2STAT_ERROR_OUT_OF_MEMORY;
                }
                pTreeNode->mUserVal = (UINT_PTR)pExpStr;
                K2TREE_Insert(&symTree, pTreeNode->mUserVal, pTreeNode);
                exportsStrSize[ixExpSec] += K2ASC_Len(pExpStr) + 1;
                pExpRef++;
            }
#if DUMP_EXPORTS
            printf("    %d bytes strings\n", exportsStrSize[ixExpSec]);
#endif
            totalExportsStrSize += exportsStrSize[ixExpSec];
        }
    }
    if (0 == totalExportCount)
    {
        return K2STAT_NO_ERROR;
    }
//    printf("%d Total Exports\n", totalExportCount);
    K2_ASSERT(totalExportCount == symTree.mNodeCount);

    // start with empty symbol string table
    symStrBytes = 1;

    // start with 'empty' section header string table
    secStrBytes = 1;

    // start with library header and the single object file record
    fileSizeBytes = 8 + ELF_LIBREC_LENGTH;

    // object file header
    fileSizeBytes += sizeof(Elf32_Ehdr);

    // null section header
    sectionCount = 1;

    // section header string table header
    secStrBytes += K2ASC_Len(sgpSecStr_SecStr) + 1;
    sectionCount++;

    // symbol table section 
    fileSizeBytes += sizeof(Elf32_Sym) * (totalExportCount + 1);
    secStrBytes += K2ASC_Len(sgpSecStr_Sym) + 1;
    sectionCount++;

    // symbol string table section header
    symStrSecSize = totalExportsStrSize + 1;
    symStrSecSize = K2_ROUNDUP(symStrSecSize, 4);
    fileSizeBytes += symStrSecSize;
    secStrBytes += K2ASC_Len(sgpSecStr_SymStr) + 1;
    sectionCount++;

    // import sections and symbol index entry for library (stupidly necessary)
    for (ixExpSec = 0; ixExpSec < XDLProgDataType_Count; ixExpSec++)
    {
        pExpHdr = gOut.mpElfExpSegHdr[ixExpSec];
        if (NULL != pExpHdr)
        {
            secStrBytes += K2ASC_Len(sgpSecStr_Imp) + gOut.mTargetNameLen + 1;

            // import data (strings are going into string table)
            expSegBytes = sizeof(XDL_EXPORTS_SEGMENT_HEADER);
            expSegBytes += pExpHdr->mCount * sizeof(XDL_EXPORT32_REF);
            expSegBytes += sizeof(K2_GUID128);
            expSegBytes += gOut.mTargetNameLen + 1;
            expSegBytes = K2_ROUNDUP(expSegBytes, 4);
            fileSizeBytes += expSegBytes;
            outSecIx[ixExpSec] = sectionCount++;
        }
        else
            outSecIx[ixExpSec] = 0;
    }
    indexBytes = (sizeof(UINT32) * (totalExportCount + 1)) + totalExportsStrSize;
    indexBytes = K2_ROUNDUP(indexBytes, 4);
    fileSizeBytes += ELF_LIBREC_LENGTH + indexBytes;

    secStrBytes = K2_ROUNDUP(secStrBytes, 4);

    fileSizeBytes += sizeof(Elf32_Shdr) * sectionCount;

    fileSizeBytes += secStrBytes;

    workFile.mAsPtr = new UINT8[(fileSizeBytes + 3) & ~3];
    if (workFile.mAsPtr == NULL)
    {
        printf("*** Memory allocation failed\n");
        return -100;
    }
    K2MEM_Zero(workFile.mAsPtr, fileSizeBytes);

    work.mAsPtr = workFile.mAsPtr;

    K2ASC_Copy((char *)work.mAsPtr, "!<arch>\n");
    work.mAsVal += 8;

    pLibIndexRecord = (Elf_LibRec *)work.mAsPtr;
    K2ASC_Copy(pLibIndexRecord->mName, "/               ");
    K2ASC_Copy(pLibIndexRecord->mTime, "1367265298  ");
    K2ASC_Copy(pLibIndexRecord->mJunk, "0     0     100666  ");
    K2ASC_PrintfLen(pLibIndexRecord->mFileBytes, 11, "%-10d", indexBytes);
    pLibIndexRecord->mMagic[0] = 0x60;
    pLibIndexRecord->mMagic[1] = 0x0A;
    work.mAsVal += ELF_LIBREC_LENGTH;

    pLibIndex = (UINT8 *)work.mAsPtr;
    work.mAsVal += indexBytes;
    *((UINT32 *)pLibIndex) = K2_SWAP32(totalExportCount);
    pLibIndex += sizeof(UINT32);
    rev = work.mAsVal - workFile.mAsVal;
    rev = K2_SWAP32(rev);
    for (ixExp = 0; ixExp < totalExportCount; ixExp++)
    {
        *((UINT32 *)pLibIndex) = rev;
        pLibIndex += sizeof(UINT32);
    }
    pTreeNode = K2TREE_FirstNode(&symTree);
    do
    {
        K2TREE_Remove(&symTree, pTreeNode);
        K2ASC_Copy((char *)pLibIndex, (char *)pTreeNode->mUserVal);
        pLibIndex += K2ASC_Len((char *)pLibIndex) + 1;
        pTreeNode = K2TREE_FirstNode(&symTree);
    } while (pTreeNode != NULL);

    pOutLibRecord = (Elf_LibRec *)work.mAsPtr;
    K2ASC_PrintfLen(pOutLibRecord->mName, 16, "%d.o/", K2CRC_Calc32(0, gOut.mTargetName, gOut.mTargetNameLen));
    K2ASC_Copy(pOutLibRecord->mTime, "1367265298  ");
    K2ASC_Copy(pOutLibRecord->mJunk, "0     0     100666  ");
    K2ASC_PrintfLen(pOutLibRecord->mFileBytes, 11, "%-10d", fileSizeBytes - (8 + ELF_LIBREC_LENGTH));
    pOutLibRecord->mMagic[0] = 0x60;
    pOutLibRecord->mMagic[1] = 0x0A;
    work.mAsVal += ELF_LIBREC_LENGTH;

    workElfBase.mAsPtr = work.mAsPtr;
    pOutHdr = (Elf32_Ehdr *)work.mAsPtr;
    work.mAsVal += sizeof(Elf32_Ehdr);
    pOutHdr->e_ident[EI_MAG0] = ELFMAG0;
    pOutHdr->e_ident[EI_MAG1] = ELFMAG1;
    pOutHdr->e_ident[EI_MAG2] = ELFMAG2;
    pOutHdr->e_ident[EI_MAG3] = ELFMAG3;
    pOutHdr->e_ident[EI_CLASS] = ELFCLASS32;
    pOutHdr->e_ident[EI_DATA] = ELFDATA2LSB;
    pOutHdr->e_ident[EI_VERSION] = EV_CURRENT;
    pOutHdr->e_type = ET_REL;
    pOutHdr->e_machine = apParse->mpRawFileData->e_machine;
    pOutHdr->e_version = EV_CURRENT;
    if (pOutHdr->e_machine == EM_A32)
        pOutHdr->e_flags = EF_A32_ABIVER;
    pOutHdr->e_ehsize = sizeof(Elf32_Ehdr);
    pOutHdr->e_shentsize = sizeof(Elf32_Shdr);
    pOutHdr->e_shnum = sectionCount;
    pOutHdr->e_shstrndx = 1;
    pOutHdr->e_shoff = work.mAsVal - workElfBase.mAsVal;

    pOutSecHdr = (Elf32_Shdr *)work.mAsPtr;
    work.mAsVal += sizeof(Elf32_Shdr) * sectionCount;

    pOutSecHdrStr = (char *)work.mAsPtr;
    pOutSecHdr[1].sh_name = 1;
    pOutSecHdr[1].sh_type = SHT_STRTAB;
    pOutSecHdr[1].sh_size = secStrBytes;
    pOutSecHdr[1].sh_offset = work.mAsVal - workElfBase.mAsVal;
    pOutSecHdr[1].sh_addralign = 1;
    K2ASC_Copy(pOutSecHdrStr + pOutSecHdr[1].sh_name, sgpSecStr_SecStr);
    secStrBytes = 1 + K2ASC_Len(sgpSecStr_SecStr) + 1;
    work.mAsVal += pOutSecHdr[1].sh_size;

    pOutSymTab = (Elf32_Sym *)work.mAsPtr;
    pOutSecHdr[2].sh_name = secStrBytes;
    pOutSecHdr[2].sh_type = SHT_SYMTAB;
    pOutSecHdr[2].sh_size = (totalExportCount + 1) * sizeof(Elf32_Sym);
    pOutSecHdr[2].sh_offset = work.mAsVal - workElfBase.mAsVal;
    pOutSecHdr[2].sh_link = 3;
    pOutSecHdr[2].sh_info = 1;
    pOutSecHdr[2].sh_addralign = 4;
    pOutSecHdr[2].sh_entsize = sizeof(Elf32_Sym);
    K2ASC_Copy(pOutSecHdrStr + pOutSecHdr[2].sh_name, sgpSecStr_Sym);
    secStrBytes += K2ASC_Len(sgpSecStr_Sym) + 1;
    work.mAsVal += pOutSecHdr[2].sh_size;
    ixSym = 1;

    outSymStrBase = outSymStrWork = work;
    outSymStrWork.mAsVal++;
    pOutSecHdr[3].sh_name = secStrBytes;
    pOutSecHdr[3].sh_type = SHT_STRTAB;
    pOutSecHdr[3].sh_size = symStrSecSize;
    pOutSecHdr[3].sh_offset = work.mAsVal - workElfBase.mAsVal;
    pOutSecHdr[3].sh_addralign = 1;
    K2ASC_Copy(pOutSecHdrStr + pOutSecHdr[3].sh_name, sgpSecStr_SymStr);
    secStrBytes += K2ASC_Len(sgpSecStr_SymStr) + 1;
    work.mAsVal += pOutSecHdr[3].sh_size;

    // imports
    for (ixExpSec = 0; ixExpSec < XDLProgDataType_Count; ixExpSec++)
    {
        pExpHdr = gOut.mpElfExpSegHdr[ixExpSec];
        if (NULL != pExpHdr)
        {
            pOutExpSeg[ixExpSec] = (XDL_EXPORTS_SEGMENT_HEADER *)work.mAsPtr;
            pOutExpSeg[ixExpSec]->mCount = pExpHdr->mCount;
            pOutExpSeg[ixExpSec]->mCRC32 = pExpHdr->mCRC32;
            pOutSecHdr[outSecIx[ixExpSec]].sh_name = secStrBytes;
            pOutStr = pOutSecHdrStr + pOutSecHdr[outSecIx[ixExpSec]].sh_name;
            K2ASC_Printf(pOutStr, "%s%.*s", sgpSecStr_Imp, gOut.mTargetNameLen, gOut.mTargetName);
            if (ixExpSec == 0)
                pOutStr[1] = 'c';
            else if (ixExpSec == 1)
                pOutStr[1] = 'r';
            else
                pOutStr[1] = 'd';
            secStrBytes += K2ASC_Len(sgpSecStr_Imp) + gOut.mTargetNameLen + 1;

            pOutSecHdr[outSecIx[ixExpSec]].sh_type = SHT_PROGBITS;
            pOutSecHdr[outSecIx[ixExpSec]].sh_flags = SHF_ALLOC | XDL_ELF_SHF_TYPE_IMPORTS;
            if (ixExpSec == 0)
                pOutSecHdr[outSecIx[ixExpSec]].sh_flags |= SHF_EXECINSTR;
            else if (ixExpSec == 2)
                pOutSecHdr[outSecIx[ixExpSec]].sh_flags |= SHF_WRITE;

            expSegBytes = sizeof(XDL_EXPORTS_SEGMENT_HEADER);
            expSegBytes += pExpHdr->mCount * sizeof(XDL_EXPORT32_REF);
            K2MEM_Copy(((UINT8 *)work.mAsPtr) + expSegBytes, &gOut.mpElfAnchor->Id, sizeof(K2_GUID128));
            expSegBytes += sizeof(K2_GUID128);
            K2ASC_CopyLen(((char *)pOutExpSeg[ixExpSec]) + expSegBytes, gOut.mTargetName, gOut.mTargetNameLen);
            expSegBytes += gOut.mTargetNameLen + 1;
            expSegBytes = K2_ROUNDUP(expSegBytes, 4);

            if (ixExpSec == 0)
                symType = ELF32_MAKE_SYMBOL_INFO(STB_GLOBAL, STT_FUNC);
            else
                symType = ELF32_MAKE_SYMBOL_INFO(STB_GLOBAL, STT_OBJECT);
            pExpRef = (XDL_EXPORT32_REF *)(((UINT8 const *)pExpHdr) + sizeof(XDL_EXPORTS_SEGMENT_HEADER));
            for (ixExp = 0; ixExp < pExpHdr->mCount; ixExp++)
            {
                pOutSymTab[ixSym].st_name = (outSymStrWork.mAsVal - outSymStrBase.mAsVal);
                pOutSymTab[ixSym].st_value = sizeof(XDL_EXPORTS_SEGMENT_HEADER) +
                    (ixExp * sizeof(XDL_EXPORT32_REF));
                pOutSymTab[ixSym].st_size = sizeof(XDL_EXPORT32_REF);
                pOutSymTab[ixSym].st_info = symType;
                pOutSymTab[ixSym].st_shndx = outSecIx[ixExpSec];
                ixSym++;

                pExpStr = ((char const *)pExpHdr) + pExpRef->mNameOffset;
                K2ASC_Copy((char *)outSymStrWork.mAsPtr, pExpStr);
                outSymStrWork.mAsVal += K2ASC_Len((char *)outSymStrWork.mAsPtr) + 1;
                pExpRef++;
            }

            pOutSecHdr[outSecIx[ixExpSec]].sh_size = expSegBytes;
            pOutSecHdr[outSecIx[ixExpSec]].sh_offset = work.mAsVal - workElfBase.mAsVal;
            pOutSecHdr[outSecIx[ixExpSec]].sh_addr = 0x10000000 | pOutSecHdr[outSecIx[ixExpSec]].sh_offset;
            pOutSecHdr[outSecIx[ixExpSec]].sh_addralign = 4;
            work.mAsVal += expSegBytes;
        }
    }
    if ((ixSym != totalExportCount + 1) ||
        ((work.mAsVal - workFile.mAsVal) != fileSizeBytes))
    {
        printf("*** Internal error\n");
        return K2STAT_ERROR_UNKNOWN;
    }

#if 0
    {
        K2ELF32PARSE file32;
        K2STAT status;

        status = K2ELF32_Parse((UINT8 const *)workElfBase.mAsPtr, fileSizeBytes - (8 + ELF_LIBREC_LENGTH), &file32);
        if (K2STAT_IS_ERROR(status))
        {
            printf("*** File LoadBack failed\n");
            return K2STAT_ERROR_UNKNOWN;
        }

        printf("import elf parse ok\n");
    }
#endif

    hFile = CreateFile(gOut.mpImportLibFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        printf("*** Error %d: Could not create import library at \"%s\"\n", GetLastError(), gOut.mpImportLibFilePath);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    ok = WriteFile(hFile, workFile.mAsPtr, fileSizeBytes, &wrot, NULL);
    CloseHandle(hFile);
    if (!ok)
    {
        printf("*** Import library file write error %d\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

//    printf("Wrote out import library \"%s\"\n", gOut.mpImportLibFilePath);

    delete[]((UINT8 *)workFile.mAsPtr);

    return K2STAT_NO_ERROR;
}

