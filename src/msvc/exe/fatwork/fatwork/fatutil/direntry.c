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
#include <lib/fatutil/cfatutil.h>
#include <lib/ascii/cascii.h>
#include <lib/memory/cmemory.h>

UINT8 
FATUTIL_LongFileNameChecksum(
    UINT8 const *   apDirEntry83Name
)
{
    UINT  i;
    UINT8 sum=0;
    for (i=11; i>0; i--)
        sum = ((sum & 1) << 7) + (sum >> 1) + *(apDirEntry83Name++);
    return sum;
}

errCode
FATUTIL_FindDirEntry(
    UINT8 const *           apDirEntryBuffer
,   UINT                    aNumDirEntries
,   char const *            apName
,   UINT                    aNameLen
,   FAT_DIRENTRY *     apRetHeadEntry
)
{
    FAT_DIRENTRY const *pEnt;
    FAT_LONGENTRY const *pFirst;
    FAT_LONGENTRY const *pLong;
    UINT inLongName;
    UINT compare83;
    UINT chkIx;
    UINT entryLen;
    UINT8 chkSum;
    char constructedName[256*sizeof(UINT16)];
    char *pConstruct;
    UINT16 *pConstruct16;

    if ((!apDirEntryBuffer) || (!apName) || (!(*apName)) || (!apRetHeadEntry))
        return ERRCODE_BADARG;

    MEM_Zero(apRetHeadEntry,sizeof(FAT_DIRENTRY));

    pEnt = (FAT_DIRENTRY const *)apDirEntryBuffer;
    pFirst = NULL;
    inLongName = 0;
    compare83 = 0;
    do {
        if (inLongName)
        {
            pLong = (FAT_LONGENTRY const *)pEnt;
            if (FAT_DIRENTRY_IS_LONGNAME(pLong->mAttribute & 0x0F))
            {
                compare83 = 0;
                if (pLong->mMustBeZero!=0)
                {
                    /* error in chain */
                    pFirst = NULL;
                    inLongName = 0;
                }
            }
            else
            {
                /* should be head entry for the current long name */
                pLong--;
                /* pLong should have longend bit set */

                if (!(pLong->mCaseSpec & FAT_DIRENTRY_CASE_NO83NAME))
                {
                    compare83 = 1;
                    if ((pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_AVAIL) &&
                        (pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_DOT) &&
                        (pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_ERASED))
                    {
                        /* calc the checksum and check the long name stuffs */
                        chkSum = FATUTIL_LongFileNameChecksum(pEnt->mFileName);
                        do {
                            pLong--;
                            if (pLong->m83NameChecksum!=chkSum)
                            {
                                pLong = NULL;
                                break;
                            }
                        } while (pLong!=pFirst);
                    }
                    else
                        pLong = NULL;
                }
                else 
                {
                    compare83 = 0;
                    if (!(pLong->mAttribute & FAT_DIRENTRY_ATTR_LONGEND))
                        pLong = NULL;
                }

                if (pLong)
                {
                    entryLen = 0;
                    /* all entries in long name have the right checksum */
                    /* reconstruct the long name */
                    pConstruct = &constructedName[255];
                    *pConstruct = 0;
                    pConstruct--;
                    *pConstruct = 0;
                    pConstruct--;
                    do {
                        if (entryLen>aNameLen)
                            break;
                        pConstruct -= (2*sizeof(UINT16));
                        MEM_Copy(pConstruct,pFirst->mNameChars3,2*sizeof(UINT16));
                        pConstruct -= (6*sizeof(UINT16));
                        MEM_Copy(pConstruct,pFirst->mNameChars2,6*sizeof(UINT16));
                        pConstruct -= (5*sizeof(UINT16));
                        MEM_Copy(pConstruct,pFirst->mNameChars1,5*sizeof(UINT16));
                        entryLen += 13;
                        pFirst++;
                    } while (pFirst!=((FAT_LONGENTRY const *)pEnt));

                    /* convert the unicode to ascii */
                    pConstruct16 = (UINT16 *)pConstruct;
                    pConstruct = constructedName;
                    entryLen = 0;
                    do {
                        *pConstruct = (UINT8)(*pConstruct16);
                        pConstruct++;
                        pConstruct16++;
                        entryLen++;
                    } while (*pConstruct16);
                    *pConstruct = 0;

                    /* now compare to the input name */
                    if (entryLen==aNameLen)
                    {
                        if (!ASC_CompInsLen(constructedName, apName, entryLen))
                            break;
                    }
                }
                else
                {
                    /* error in chain */
                    pFirst = NULL;
                    inLongName = 0;
                    compare83 = 0;
                }
            }
        }
        else
        {
            if (FAT_DIRENTRY_IS_LONGNAME(pEnt->mAttribute))
            {
                pLong = (FAT_LONGENTRY const *)pEnt;
                if (pLong->mMustBeZero==0)
                {
                    pFirst = (FAT_LONGENTRY const *)pEnt;
                    inLongName = 1;
                }
                compare83 = 0;
            }
            else
            {
                /* this is a normal 8.3 filename (not a long name) */
                compare83 = 1;
            }
        }

        if (compare83)
        {
            compare83 = 0;
            if ((pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_AVAIL) &&
                (pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_DOT) &&
                (pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_ERASED) &&
                (!(pEnt->mAttribute & (FAT_DIRENTRY_ATTR_LABEL | FAT_DIRENTRY_ATTR_DIR))))
            {
                /* remove any internal spaces in the 8.3 name before comparison */
                pConstruct = constructedName;
                entryLen = 0;
                for(chkIx=0;chkIx<8;chkIx++)
                {
                    if ((!pEnt->mFileName[chkIx]) || (pEnt->mFileName[chkIx]==' '))
                        break;
                    *(pConstruct++) = pEnt->mFileName[chkIx];
                    entryLen++;
                }
                if ((pEnt->mExtension[0]) && (pEnt->mExtension[0]!=' '))
                {
                    *pConstruct = '.';
                    entryLen++;
                    pConstruct++;
                }
                *pConstruct = 0;
                for(chkIx=0;chkIx<3;chkIx++)
                {
                    if ((!pEnt->mExtension[chkIx]) || (pEnt->mExtension[chkIx]==' '))
                        break;
                    *(pConstruct++) = pEnt->mExtension[chkIx];
                    entryLen++;
                }
                *pConstruct = 0;

                if (entryLen==aNameLen)
                {
                    if (!ASC_CompInsLen(constructedName,apName,entryLen))
                        break;
                }
            }
        }

        pEnt++;

    } while (--aNumDirEntries);

    if (!aNumDirEntries)
        return ERRCODE_NODATA;

    MEM_Copy(apRetHeadEntry, pEnt, sizeof(FAT_DIRENTRY));

    return 0;
}
