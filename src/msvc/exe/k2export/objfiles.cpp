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

#include "k2export.h"

K2STAT
AddOneGlobalSymbol32(
    OBJ_FILE *          apObjFile,
    char const *        apSymName,
    Elf32_Sym const *   apSymEnt
)
{
    Elf32_Shdr const *  pFoundSecHdr;
    Elf32_Shdr const *  pSymSecHdr;
    bool                isWeak;
    bool                isCode;
    bool                isRead;
    GLOBAL_SYMBOL *     pGlob;
    K2TREE_NODE *       pTreeNode;
    GLOBAL_SYMBOL *     pFound;
    UINT8 const *       pFoundData;
    UINT8 const *       pSymData;

    pSymSecHdr = (Elf32_Shdr const *)(apObjFile->Bits32.Parse.mpSectionHeaderTable + (apObjFile->Bits32.Parse.mSectionHeaderTableEntryBytes * apSymEnt->st_shndx));

    isWeak = (ELF32_ST_BIND(apSymEnt->st_info) == STB_WEAK);
    isCode = (pSymSecHdr->sh_flags & SHF_EXECINSTR) ? true : false;
    if (isCode)
        isRead = true;
    else
        isRead = (pSymSecHdr->sh_flags & SHF_WRITE) ? false : true;

    pTreeNode = K2TREE_Find(&gOut.SymbolTree, (UINT_PTR)apSymName);
    if (pTreeNode != NULL)
    {
        pFound = K2_GET_CONTAINER(GLOBAL_SYMBOL, pTreeNode, TreeNode);

        if ((pFound->mIsCode != isCode) ||
            (pFound->mIsRead != isRead))
        {
            printf("Symbol \"%s\" found with two different types:\n", apSymName);
            printf("%c%c in %s(%.*s)\n",
                pFound->mIsCode ? 'x' : '-',
                pFound->mIsRead ? 'r' : '-',
                pFound->mpObjFile->mpParentFile->FileName(), pFound->mpObjFile->mObjNameLen, pFound->mpObjFile->mpObjName);
            printf("%c%c in %s(%.*s)\n",
                isCode ? 'x' : '-',
                isRead ? 'r' : '-',
                apObjFile->mpParentFile->FileName(), apObjFile->mObjNameLen, apObjFile->mpObjName);
            return K2STAT_ERROR_ALREADY_EXISTS;
        }

        if (pFound->mIsWeak)
        {
            if (!isWeak)
            {
                /* found weak symbol strong name. alter tree node */
                pFound->mIsWeak = false;
                pFound->mpObjFile = apObjFile;
                pFound->Bits32.mpSymEnt = apSymEnt;
            }
            return K2STAT_NO_ERROR;
        }
        else if (!isWeak)
        {
            //
            // duplicate symbol
            //
            if ((pFound->mIsCode == isCode) &&
                (pFound->Bits32.mpSymEnt->st_size == apSymEnt->st_size))
            {
                if (apSymEnt->st_size == 0)
                    pFound = NULL;
                else
                {
                    pFoundSecHdr = (Elf32_Shdr const *)(pFound->mpObjFile->Bits32.Parse.mpSectionHeaderTable + (pFound->Bits32.mpSymEnt->st_shndx * pFound->mpObjFile->Bits32.Parse.mSectionHeaderTableEntryBytes));
                    pFoundData = ((UINT8 const *)pFound->mpObjFile->Bits32.Parse.mpRawFileData) + pFoundSecHdr->sh_offset;
                    pFoundData += pFound->Bits32.mpSymEnt->st_value;

                    pSymData = ((UINT8 const *)apObjFile->Bits32.Parse.mpRawFileData) + pSymSecHdr->sh_offset;
                    pSymData += apSymEnt->st_value;

                    if (!K2MEM_Compare(pFoundData, pSymData, apSymEnt->st_size))
                    {
                        //
                        // symbol contents are identical
                        //
                        pFound = NULL;
                    }
                }
            }
            if (pFound)
            {
                printf("Duplicate symbol \"%s\" found:\n", apSymName);
                printf("in  %s(%.*s)\n", pFound->mpObjFile->mpParentFile->FileName(), pFound->mpObjFile->mObjNameLen, pFound->mpObjFile->mpObjName);
                printf("and %s(%.*s)\n", apObjFile->mpParentFile->FileName(), apObjFile->mObjNameLen, apObjFile->mpObjName);
                return K2STAT_ERROR_BAD_ARGUMENT;
            }
            return K2STAT_NO_ERROR;
        }
    }

    pGlob = new GLOBAL_SYMBOL;
    if (pGlob == NULL)
    {
        printf("*** Memory allocation error\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pGlob->mpObjFile = apObjFile;
    pGlob->Bits32.mpSymEnt = apSymEnt;
    pGlob->mpSymName = apSymName;
    pGlob->mIsWeak = isWeak;
    pGlob->mIsCode = isCode;
    pGlob->mIsRead = isRead;

    pGlob->TreeNode.mUserVal = (UINT_PTR)apSymName;
    K2TREE_Insert(&gOut.SymbolTree, (UINT_PTR)apSymName, &pGlob->TreeNode);

    return K2STAT_NO_ERROR;
}

K2STAT
AddOneObject32(
    K2ReadOnlyMappedFile *  apSrcFile,
    char const *            apObjectFileName,
    UINT_PTR                aObjectFileNameLen,
    UINT8 const *           apFileData,
    UINT_PTR                aFileDataBytes
)
{
    OBJ_FILE *      pObj;
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

//    printf("k2export:AddOneObject32(\"%.*s\")\n", aObjectFileNameLen, apObjectFileName);

    pObj = new OBJ_FILE;
    if (NULL == pObj)
    {
        printf("*** Out of memory\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    stat = K2STAT_NO_ERROR;

    do {
        K2MEM_Zero(pObj, sizeof(OBJ_FILE));
        stat = K2ELF32_Parse(apFileData, aFileDataBytes, &pObj->Bits32.Parse);
        if (K2STAT_IS_ERROR(stat))
        {
            printf("*** Could not parse input file as ELF (%.*s)\n", aObjectFileNameLen, apObjectFileName);
            break;
        }

        pObj->mpParentFile = apSrcFile;
        pObj->mpObjName = apObjectFileName;
        pObj->mObjNameLen = aObjectFileNameLen;

        for (secIx = 1; secIx < pObj->Bits32.Parse.mpRawFileData->e_shnum; secIx++)
        {
            pSecHdr = (Elf32_Shdr *)(pObj->Bits32.Parse.mpSectionHeaderTable + (pObj->Bits32.Parse.mSectionHeaderTableEntryBytes * secIx));
            if (pSecHdr->sh_type != SHT_SYMTAB)
                continue;

            symBytes = pSecHdr->sh_entsize;
            if (0 == symBytes)
                symBytes = sizeof(Elf32_Sym);
            symCount = pSecHdr->sh_size / symBytes;
            if (1 >= symCount)
                continue;

            pStrSecHdr = (Elf32_Shdr *)(pObj->Bits32.Parse.mpSectionHeaderTable + (pObj->Bits32.Parse.mSectionHeaderTableEntryBytes * pSecHdr->sh_link));
            pStr = ((char const *)pObj->Bits32.Parse.mpRawFileData) + pStrSecHdr->sh_offset;

            pScan = ((UINT8 const *)pObj->Bits32.Parse.mpRawFileData) + pSecHdr->sh_offset;
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
                    if ((symNameLen != 0) && (pSym->st_shndx < pObj->Bits32.Parse.mpRawFileData->e_shnum))
                    {
                        stat = AddOneGlobalSymbol32(pObj, pSymName, pSym);
                        if (K2STAT_IS_ERROR(stat))
                            break;
                    }
                }
            } while (--symCount);

            if (K2STAT_IS_ERROR(stat))
                break;
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        delete pObj;
    }

    return stat;
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

//    printf("k2export:AddOneObject64(\"%.*s\")\n", aObjectFileNameLen, apObjectFileName);
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
        if (gOut.mFileClass == ELFCLASS32)
        {
            if (((Elf32_Ehdr *)apFileData)->e_machine == EM_X32)
            {
                gOut.mElfMachine = EM_X32;
            }
            else
            {
                gOut.mElfMachine = EM_A32;
            }
        }
        else
        {
            if (((Elf32_Ehdr *)apFileData)->e_machine == EM_X64)
            {
                gOut.mElfMachine = EM_X64;
            }
            else
            {
                gOut.mElfMachine = EM_A64;
            }
        }
    }

    if (gOut.mFileClass == ELFCLASS32)
    {
        return AddOneObject32(apSrcFile, apObjectFileName, aObjectFileNameLen, apFileData, aFileDataBytes);
    }

    return AddOneObject64(apSrcFile, apObjectFileName, aObjectFileNameLen, apFileData, aFileDataBytes);
}
