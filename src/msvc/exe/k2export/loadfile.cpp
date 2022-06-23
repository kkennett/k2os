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

struct FileTrack
{
    K2ReadOnlyMappedFile *  mpFile;
    FileTrack *             mpNext;
};
static FileTrack * sgpFileTrack = NULL;
static FileTrack * sgpFileTrackEnd = NULL;

K2STAT
AddObjectLibrary(
    K2ReadOnlyMappedFile * apSrcFile
)
{
    Elf_LibRec      record;
    char *          pEnd;
    char *          pName;
    char            chk;
    UINT_PTR        nameLen;
    UINT_PTR        fileBytes;
    bool            ignoreThisFile;
    UINT_PTR        validChars;
    char *          pLongNames;
    UINT_PTR        longNamesLen;
    UINT_PTR        nameOffset;
    UINT_PTR        extLen;
    UINT8 const *   pParse;
    UINT_PTR        parseLeft;
    K2STAT          stat;

    pLongNames = NULL;

    pParse = (UINT8 const *)apSrcFile->DataPtr();
    parseLeft = (UINT_PTR)apSrcFile->FileBytes();

    // bypass header
    pParse += 8;
    parseLeft -= 8;

    do
    {
        K2MEM_Copy(&record, pParse, ELF_LIBREC_LENGTH);
        if ((record.mMagic[0] != 0x60) ||
            (record.mMagic[1] != 0x0A))
        {
            printf("*** File \"%s\" has invalid library file format.\n", apSrcFile->FileName());
            return K2STAT_ERROR_BAD_ARGUMENT;
        }

        pParse += ELF_LIBREC_LENGTH;
        parseLeft -= ELF_LIBREC_LENGTH;

        record.mTime[0] = 0;
        validChars = 0;
        pEnd = record.mName + 15;
        do {
            chk = *pEnd;
            if (chk == ' ')
            {
                if (validChars == 0)
                    *pEnd = 0;
            }
            else if (chk == '/')
                break;
            else
                validChars++;
        } while (--pEnd != record.mName);
        nameLen = (int)(pEnd - record.mName);

        record.mMagic[0] = 0;
        for (validChars = 0; validChars < 10; validChars++)
        {
            if ((record.mFileBytes[validChars] < '0') ||
                (record.mFileBytes[validChars] > '9'))
                break;
        }
        // this is fine if we overwrite the magic field
        record.mFileBytes[validChars] = 0;

        fileBytes = K2ASC_NumValue32(record.mFileBytes);
        if (fileBytes == 0)
        {
            printf("*** Invalid file size for file \"%s\" inside \"%s\"\n", record.mName, apSrcFile->FileName());
            return K2STAT_ERROR_BAD_ARGUMENT;
        }

        ignoreThisFile = false;
        pName = NULL;

        if (nameLen == 0)
        {
            if (record.mName[1] == 0)
            {
                // this is file '/' which is the symbol lookup table
                ignoreThisFile = true;
            }
            else
            {
                // this is a file with a long name
                if (pLongNames != NULL)
                {
                    nameOffset = K2ASC_NumValue32(&record.mName[1]);
                    if (nameOffset > longNamesLen)
                    {
                        ignoreThisFile = 1;
                        pName = (char *)"<error>";
                        nameLen = (int)strlen(pName);
                    }
                    else
                    {
                        pName = pLongNames + nameOffset;
                        nameLen = longNamesLen - nameOffset;
                        pEnd = pName;
                        do {
                            if (*pEnd == '/')
                                break;
                            pEnd++;
                        } while (--nameLen);
                        nameLen = (int)(pEnd - pName);
                        if (!nameLen)
                            ignoreThisFile = true;
                    }
                }
                else
                    ignoreThisFile = true;
            }
        }
        else if ((nameLen == 1) && (record.mName[1] == '/'))
        {
            // this is file '//' which is the long filenames table
            pLongNames = (char *)pParse;
            longNamesLen = fileBytes;
            ignoreThisFile = true;
        }
        else
        {
            pName = record.mName;
        }

        if (!ignoreThisFile)
        {
            if (nameLen)
            {
                pEnd = pName + nameLen - 1;
                extLen = 0;
                do {
                    chk = *pEnd;
                    if (chk == '.')
                        break;
                    extLen++;
                } while (--pEnd != pName);
                if (pEnd != pName)
                {
                    pEnd++;
                    chk = *pEnd;
                    if ((chk != 'o') && (chk != 'O'))
                        ignoreThisFile = true;
                }
                else
                {
                    /* no extension */
                    ignoreThisFile = true;
                }
            }
        }

        if (!ignoreThisFile)
        {
            stat = AddOneObject(apSrcFile, pName, nameLen, pParse, fileBytes);
            if (K2STAT_IS_ERROR(stat))
                return stat;
        }

        if (fileBytes & 1)
            fileBytes++;
        pParse += fileBytes;
        if (parseLeft > fileBytes)
            parseLeft -= fileBytes;
        else
            parseLeft = 0;

    } while (parseLeft >= ELF_LIBREC_LENGTH);

    return K2STAT_NO_ERROR;
}

K2STAT
AddObjectFiles(
    K2ReadOnlyMappedFile * apSrcFile
)
{
    static UINT8 const sgElfSig[4] = {
        ELFMAG0,
        ELFMAG1,
        ELFMAG2,
        ELFMAG3 };
    UINT8 const *pChk;

    if (apSrcFile->FileBytes() < K2ELF32_MIN_FILE_SIZE)
    {
        printf("*** Input file \"%s\" is too small\n", apSrcFile->FileName());
        return K2STAT_ERROR_TOO_SMALL;
    }

    pChk = (UINT8 const *)apSrcFile->DataPtr();

    if (!K2MEM_Compare(pChk, "!<arch>\n", 8))
        return AddObjectLibrary(apSrcFile);

    if (!K2MEM_Compare(pChk, sgElfSig, 4))
        return AddOneObject(apSrcFile, apSrcFile->FileName(), K2ASC_Len(apSrcFile->FileName()), pChk, (UINT_PTR)apSrcFile->FileBytes());

    printf("*** Input file \"%s\" is not recognized as an object file or object library.\n", apSrcFile->FileName());

    return K2STAT_ERROR_BAD_ARGUMENT;
}

K2STAT
LoadInputFile(
    char const *apFilePath
)
{
    K2STAT          stat;
    FileTrack *     pTrack;
    DWORD           fpLen;
    char            fullPath[_MAX_PATH];

//    printf("k2export:LoadInputFile(\"%s\")\n", apFilePath);

    fpLen = GetFullPathName(apFilePath, _MAX_PATH - 1, fullPath, NULL);
    if ((0 == fpLen) ||
        (fpLen >= _MAX_PATH))
    {
        printf("*** full file path invalid or too long:\n  \"%s\"\n", apFilePath);
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pTrack = sgpFileTrack;
    while (pTrack != NULL)
    {
        if (!_stricmp(pTrack->mpFile->FileName(), fullPath))
            break;
        pTrack = pTrack->mpNext;
    }
    if (pTrack != NULL)
    {
        printf("!!! Argument \"%s\" ignored because \"%s\" already found.\n",
            fullPath, pTrack->mpFile->FileName());
        return K2STAT_NO_ERROR;
    }

    pTrack = new FileTrack;
    if (pTrack == NULL)
    {
        printf("*** Memory allocation error\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pTrack->mpFile = K2ReadOnlyMappedFile::Create(fullPath);
    if (pTrack->mpFile == NULL)
    {
        printf("*** Could not load file \"%s\"\n", fullPath);
        return K2STAT_ERROR_NOT_FOUND;
    }

    stat = AddObjectFiles(pTrack->mpFile);
    if (K2STAT_IS_ERROR(stat))
    {
        pTrack->mpFile->Release();
        delete pTrack;
        return stat;
    }

    pTrack->mpNext = NULL;
    if (sgpFileTrackEnd == NULL)
        sgpFileTrack = pTrack;
    else
        sgpFileTrackEnd->mpNext = pTrack;
    sgpFileTrackEnd = pTrack;

    return K2STAT_NO_ERROR;
}
