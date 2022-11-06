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
#include <lib/k2mem.h>
#include <spec/k2rofs.h>
#include <lib/k2rofshelp.h>

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

class RofsFile;

UINT32 gStringFieldSize = 0;

class RofsDir
{
public:
    RofsDir(RofsDir *apParent) :
        mpParent(apParent)
    {
        mpName = NULL;
        mNameLen = 0;
        mSubCount = 0;
        mpSubs = NULL;
        mFileCount = 0;
        mpFiles = NULL;
        mpTarget = NULL;

        if (apParent != NULL)
        {
            mpParentNextSubLink = apParent->mpSubs;
            apParent->mpSubs = this;
            apParent->mSubCount++;
        }
        else
            mpParentNextSubLink = NULL;
    }

    int Init(char const *apName)
    {
        mNameLen = strlen(apName);
        gStringFieldSize += mNameLen + 1;
        mpName = new char[(mNameLen + 4) & ~3];
        if (mpName == NULL)
            return K2STAT_ERROR_OUT_OF_MEMORY;
        strcpy_s(mpName, mNameLen + 1, apName);
        return 0;
    }

    ~RofsDir(void);

    RofsDir * const mpParent;
    UINT32          mSubCount;
    RofsDir *       mpSubs;
    RofsDir *       mpParentNextSubLink;
    UINT32          mFileCount;
    RofsFile *      mpFiles;
    char *          mpName;
    UINT32          mNameLen;
    K2ROFS_DIR *    mpTarget;
};

class RofsFile
{
public:
    RofsFile(RofsDir *apParent) :
        mpParent(apParent)
    {
        mpName = NULL;
        mNameLen = 0;
        mpTarget = NULL;
        mpParentNextFileLink = apParent->mpFiles;
        apParent->mpFiles = this;
        apParent->mFileCount++;
        mpMap = NULL;
    }

    int Init(char const *apPath, char const *apNameOnly)
    {
        mNameLen = strlen(apNameOnly);
        gStringFieldSize += mNameLen + 1;
        mpName = new char[(mNameLen + 4) & ~3];
        if (mpName == NULL)
            return K2STAT_ERROR_OUT_OF_MEMORY;
        strcpy_s(mpName, mNameLen + 1, apNameOnly);

        mpMap = K2ReadOnlyMappedFile::Create(apPath);
        if (mpMap == NULL)
            return K2STAT_ERROR_OUT_OF_MEMORY;

        return 0;
    }

    ~RofsFile(void)
    {
        if (mpMap != NULL)
        {
            mpMap->Release();
            mpMap = NULL;
        }
        if (mpName != NULL)
            delete[] mpName;
    }

    RofsDir * const         mpParent;
    char *                  mpName;
    UINT32                  mNameLen;
    K2ReadOnlyMappedFile *  mpMap;
    RofsFile *              mpParentNextFileLink;
    K2ROFS_FILE *           mpTarget;
};

RofsDir::~RofsDir(void)
{
    RofsDir  *pSub;
    RofsFile *pFile;

    while (mpSubs)
    {
        pSub = mpSubs;
        mpSubs = pSub->mpParentNextSubLink;
        delete pSub;
    }

    while (mpFiles)
    {
        pFile = mpFiles;
        mpFiles = pFile->mpParentNextFileLink;
        delete pFile;
    }
}

RofsDir *   gpRoot = NULL;

int LoadOne(RofsDir *apTarget, char const *apPath, char *apEnd)
{
    HANDLE          hFind;
    WIN32_FIND_DATA findData;
    UINT32          nameLen;
    bool            retVal;
    RofsDir *       pSub;
    RofsFile *      pFile;

    retVal = false;

    strcpy_s(apEnd, _MAX_PATH - 1 - ((UINT32)(apEnd - apPath)), "*.*");
    apEnd[3] = 0;

    hFind = FindFirstFile(apPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return ERROR_FILE_NOT_FOUND;

    do {
        nameLen = strlen(findData.cFileName);

        strncpy_s(apEnd, _MAX_PATH - 1 - ((UINT32)(apEnd - apPath)),
            findData.cFileName, nameLen);
        apEnd[nameLen] = 0;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if ((0 != strcmp(findData.cFileName, ".")) &&
                (0 != strcmp(findData.cFileName, "..")))
            {
                apEnd[nameLen] = '\\';
                apEnd[nameLen + 1] = 0;

                pSub = new RofsDir(apTarget);
                if (pSub == NULL)
                    return K2STAT_ERROR_OUT_OF_MEMORY;
                retVal = pSub->Init(findData.cFileName);
                if (retVal != 0)
                    return retVal;
                retVal = LoadOne(pSub, apPath, apEnd + nameLen + 1);
                if (retVal != 0)
                    return retVal;
            }
        }
        else
        {
            pFile = new RofsFile(apTarget);
            if (pFile == NULL)
                return K2STAT_ERROR_OUT_OF_MEMORY;
            retVal = pFile->Init(apPath, findData.cFileName);
            if (retVal != 0)
                return retVal;
        }

    } while (FindNextFile(hFind, &findData));

    return retVal;
}

int LoadItAll(char const *apPath, char *apEnd)
{
    int         result;

    gpRoot = new RofsDir(NULL);
    if (gpRoot == NULL)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    result = gpRoot->Init("\\");

    if (result == 0)
        result = LoadOne(gpRoot, apPath, apEnd);

    if (result != 0)
        delete gpRoot;

    return result;
}

static UINT32 sCount(RofsDir *apDir, UINT32 *apPayloadSectors)
{
    UINT32      ret;
    RofsDir *   pSub;
    RofsFile *  pFile;

    ret = sizeof(K2ROFS_DIR);
    ret += apDir->mFileCount * sizeof(K2ROFS_FILE);
    ret += apDir->mSubCount * sizeof(UINT32);

    pFile = apDir->mpFiles;
    while (pFile != NULL)
    {
        *apPayloadSectors += (UINT32)((pFile->mpMap->FileBytes() + (K2ROFS_SECTOR_BYTES - 1)) / K2ROFS_SECTOR_BYTES);
        pFile = pFile->mpParentNextFileLink;
    }

    pSub = apDir->mpSubs;
    while (pSub != NULL)
    {
        ret += sCount(pSub, apPayloadSectors);
        pSub = pSub->mpParentNextSubLink;
    }

    return ret;
}

static void sFill(UINT8 *apBase, UINT8 **apWork, RofsDir *apDir, char **apStrings, UINT8 **apPayload)
{
    K2ROFS_DIR *    pOutDir;
    RofsFile *      pFile;
    RofsDir *       pSub;
    UINT32          payloadSectors;
    K2ROFS_FILE *   pOutFile;
    FILETIME        localFileTime;
    UINT32 *        pOffsets;

    pOutDir = (K2ROFS_DIR *)(*apWork);
    apDir->mpTarget = pOutDir;
    (*apWork) += sizeof(K2ROFS_DIR);

    pOutDir->mName = (UINT32)((*apStrings) - (char *)apBase);
    K2MEM_Copy((*apStrings), apDir->mpName, apDir->mNameLen);
    (*apStrings) += apDir->mNameLen + 1;

    pOutDir->mFileCount = apDir->mFileCount;

    pFile = apDir->mpFiles;
    while (pFile)
    {
        pOutFile = (K2ROFS_FILE *)(*apWork);
        pFile->mpTarget = pOutFile;
        (*apWork) += sizeof(K2ROFS_FILE);

        pOutFile->mName = (UINT32)((*apStrings) - (char *)apBase);
        K2MEM_Copy((*apStrings), pFile->mpName, pFile->mNameLen);
        (*apStrings) += pFile->mNameLen + 1;

        pOutFile->mSizeBytes = (UINT32)pFile->mpMap->FileBytes();
        pOutFile->mStartSectorOffset = ((UINT32)((*apPayload) - apBase)) / K2ROFS_SECTOR_BYTES;

        payloadSectors = (UINT32)((pFile->mpMap->FileBytes() + (K2ROFS_SECTOR_BYTES - 1)) / K2ROFS_SECTOR_BYTES);
        K2MEM_Copy((*apPayload), pFile->mpMap->DataPtr(), (UINT_PTR)pFile->mpMap->FileBytes());
        (*apPayload) += payloadSectors * K2ROFS_SECTOR_BYTES;

        FileTimeToLocalFileTime(&pFile->mpMap->FileTime(), &localFileTime);
        FileTimeToDosDateTime(&localFileTime, ((WORD *)&pOutFile->mTime) + 1, ((WORD *)&pOutFile->mTime));

        pFile = pFile->mpParentNextFileLink;
    }

    pOutDir->mSubCount = apDir->mSubCount;
    pOffsets = (UINT32 *)(*apWork);
    (*apWork) += apDir->mSubCount * sizeof(UINT32);

    pSub = apDir->mpSubs;
    while (pSub)
    {
        *pOffsets = (UINT32)((*apWork) - apBase);
        pOffsets++;

        sFill(apBase, apWork, pSub, apStrings, apPayload);

        pSub = pSub->mpParentNextSubLink;
    }
}

static void sPrefix(int aLevel)
{
    while (aLevel)
    {
        --aLevel;
        printf(" ");
    }
}

static void sDumpDir(K2ROFS const *apBase, K2ROFS_DIR const *apDir, int aLevel)
{
    UINT32              ix;
    K2ROFS_FILE const * pFile;

    sPrefix(aLevel);

    printf("[%s]\n", K2ROFS_NAMESTR(apBase, apDir->mName));

    for (ix = 0; ix < apDir->mFileCount; ix++)
    {
        sPrefix(aLevel + 2);
        pFile = K2ROFSHELP_SubFileIx(apBase, apDir, ix);
        printf(" %8d %08X %s\n", pFile->mSizeBytes, (UINT32)K2ROFS_FILEDATA(apBase, pFile), K2ROFS_NAMESTR(apBase, pFile->mName));
    }

    for (ix = 0; ix < apDir->mSubCount; ix++)
    {
        sDumpDir(apBase, K2ROFSHELP_SubDirIx(apBase, apDir, ix), aLevel + 2);
    }
}

static void sDump(K2ROFS const *apRofs)
{
    sDumpDir(apRofs, K2ROFS_ROOTDIR(apRofs), 0);
}

int CreateOutputFile(char const *apOutFileName)
{
    UINT32      totalOut;
    UINT32      payloadSectors;
    UINT8 *     pAlloc;
    UINT8 *     pOutRaw;
    UINT8 *     pOutWork;
    K2ROFS *    pROFS;
    UINT32      stringsOffset;
    UINT32      payloadsOffset;
    char *      pStrings;
    UINT8 *     pPayloads;
    HANDLE      hFile;
    DWORD       wrote;
    BOOL        ok;

    totalOut = sizeof(K2ROFS);
    payloadSectors = 0;

    totalOut += sCount(gpRoot, &payloadSectors);

    // add strings at the end of the dir structure
    stringsOffset = totalOut;
    totalOut += gStringFieldSize;

    // round up, convert to sectors
    totalOut = ((totalOut + (K2ROFS_SECTOR_BYTES - 1)) / K2ROFS_SECTOR_BYTES);

    // now add all the payloads
    payloadsOffset = totalOut * K2ROFS_SECTOR_BYTES;
    totalOut += payloadSectors;

    // alloc aligned
    pAlloc = (UINT8 *)malloc((totalOut * K2ROFS_SECTOR_BYTES) + (K2ROFS_SECTOR_BYTES - 1));
    pOutRaw = (UINT8 *)((((UINT32)pAlloc) + (K2ROFS_SECTOR_BYTES - 1)) & XDL_SECTORINDEX_MASK);

    K2MEM_Zero(pOutRaw, totalOut * K2ROFS_SECTOR_BYTES);

    pOutWork = pOutRaw;
    pStrings = (char *)pOutRaw + stringsOffset;
    pPayloads = pOutRaw + payloadsOffset;

    pROFS = (K2ROFS *)pOutWork;
    pROFS->mRootDir = sizeof(K2ROFS);
    pROFS->mSectorCount = totalOut;

    pOutWork += sizeof(K2ROFS);

    sFill(pOutRaw, &pOutWork, gpRoot, &pStrings, &pPayloads);

    sDump(pROFS);

    hFile = CreateFile(apOutFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("\n***k2rofs - could not create target file \"%s\"\n", apOutFileName);
        return GetLastError();
    }
    ok = WriteFile(hFile, pOutRaw, totalOut * K2ROFS_SECTOR_BYTES, &wrote, NULL);
    if (!ok)
    {
        printf("\n***k2rofs - failed to write output file\n");
    }

    CloseHandle(hFile);

    free(pAlloc);

    return ok ? 0 : -1;
}

int main(int argc, char **argv)
{
    DWORD   fAttr;
    char    dirPath[_MAX_PATH];
    char *  pEnd;
    int     result;

    if (argc < 3)
    {
        printf("\nk2rofs: need source dir and target filename\n\n");
        return -1;
    }

    fAttr = GetFileAttributes(argv[1]);
    if ((fAttr == INVALID_FILE_ATTRIBUTES) || ((fAttr & FILE_ATTRIBUTE_DIRECTORY) == 0))
    {
        printf("\nk2rofs: first argument is not a directory\n\n");
        return -2;
    }
    strcpy_s(dirPath, _MAX_PATH, argv[1]);
    dirPath[_MAX_PATH - 1] = 0;
    dirPath[_MAX_PATH - 2] = 0;
    pEnd = dirPath;
    while (*pEnd)
        pEnd++;
    pEnd--;
    if ((*pEnd != '/') && (*pEnd != '\\'))
    {
        pEnd++;
        *pEnd = '\\';
        pEnd++;
        *pEnd = 0;
    }
    else
        pEnd++;

    result = LoadItAll(dirPath, pEnd);
    if (result != 0)
        return result;

    result = CreateOutputFile(argv[2]);

    delete gpRoot;

    return result;
}