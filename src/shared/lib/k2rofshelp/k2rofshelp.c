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

#include <lib/k2rofshelp.h>
#include <lib/k2asc.h>

static
UINT32 const *
sSubsOffsets(
    K2ROFS_DIR const *apDir
)
{
    return (UINT32 const *)
        (((UINT8 const *)apDir) + sizeof(K2ROFS_DIR) + (apDir->mFileCount * sizeof(K2ROFS_FILE)));
}

static
void const *
sSubEntry(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    char const *        apSubPath,
    BOOL                aMatchFile
)
{
    char const *        pParse;
    char                ch;
    UINT32              ix;
    UINT32              partLen;
    K2ROFS_FILE const * pFiles;
    UINT32 const *      pSubs;
    K2ROFS_DIR const *  pSubDir;
    char const *        pMatch;

    //
    // eat next component
    //
    pParse = apSubPath;
    do
    {
        ch = *pParse;
        if ((ch == 0) || (ch == '\\') || (ch == '/'))
            break;
        pParse++;
    } while (1);

    if (pParse == apSubPath)
    {
        // 
        // nothing here
        //
        return NULL;
    }

    // 
    // there is something there
    //
    if ((ch == 0) && (aMatchFile))
    {
        pFiles = K2ROFSHELP_SubFileIx(apBase, apDir, 0);
        for (ix = 0; ix < apDir->mFileCount; ix++)
        {
            if (0 == K2ASC_CompIns(apSubPath, K2ROFS_NAMESTR(apBase, pFiles[ix].mName)))
            {
                return &pFiles[ix];
            }
        }
        return NULL;
    }

    partLen = (UINT32)(pParse - apSubPath);
    pSubs = sSubsOffsets(apDir);
    for (ix = 0; ix < apDir->mSubCount; ix++)
    {
        pSubDir = (K2ROFS_DIR const *)(((UINT8 const *)apBase) + pSubs[ix]);
        pMatch = K2ROFS_NAMESTR(apBase, pSubDir->mName);
        if ((0 == K2ASC_CompInsLen(apSubPath, pMatch, partLen)) &&
            (pMatch[partLen] == 0))
        {
            //
            // we matched the name with a subdirectory name
            //
            if (ch == 0)
            {
                if (!aMatchFile)
                    return pSubDir;
                return NULL;
            }

            //
            // pParse poinst to a slash or backslash.
            // move past slashes to next component
            //
            pParse++;
            do
            {
                ch = *pParse;
                if ((ch != '\\') && (ch != '/'))    // includes null
                    break;
                pParse++;
            } while (1);

            return sSubEntry(apBase, pSubDir, pParse, aMatchFile);
        }
    }

    return NULL;
}

K2ROFS_DIR const *
K2ROFSHELP_SubDir(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    char const *        apSubPath
)
{
    char ch;

    K2_ASSERT(apBase != NULL);
    K2_ASSERT(apDir != NULL);
    K2_ASSERT(apSubPath != NULL);

    if (apDir == K2ROFS_ROOTDIR((K2ROFS const *)apBase))
    {
        //
        // strip any leading slash/backslash that may be superfluous
        //
        do
        {
            ch = *apSubPath;
            if ((ch != '\\') && (ch != '/'))    // includes null
                break;
            apSubPath++;
        } while (1);
    }
    else
    {
        //
        // if any leading slash or backslash, return NULL
        // since dir is not the root
        //
        ch = *apSubPath;
        if ((ch == '\\') || (ch == '/'))    // includes null
            return NULL;
    }

    return (K2ROFS_DIR const *)sSubEntry(apBase, apDir, apSubPath, FALSE);
}


K2ROFS_FILE const *
K2ROFSHELP_SubFile(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    char const *        apSubPath
)
{
    char ch;

    K2_ASSERT(apBase != NULL);
    K2_ASSERT(apDir != NULL);
    K2_ASSERT(apSubPath != NULL);

    if (apDir == K2ROFS_ROOTDIR((K2ROFS const *)apBase))
    {
        //
        // strip any leading slash/backslash that may be superfluous
        //
        do
        {
            ch = *apSubPath;
            if ((ch != '\\') && (ch != '/'))    // includes null
                break;
            apSubPath++;
        } while (1);
    }
    else
    {
        //
        // if any leading slash or backslash, return NULL
        // since dir is not the root
        //
        ch = *apSubPath;
        if ((ch == '\\') || (ch == '/'))    // includes null
            return NULL;
    }

    return (K2ROFS_FILE const *)sSubEntry(apBase, apDir, apSubPath, TRUE);
}

K2ROFS_DIR const *
K2ROFSHELP_SubDirIx(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    UINT_PTR            aSubDirIx
)
{
    UINT32 const *pSubs;

    K2_ASSERT(apBase != NULL);
    K2_ASSERT(apDir != NULL);
    K2_ASSERT(aSubDirIx < apDir->mSubCount);

    pSubs = sSubsOffsets(apDir);

    return (K2ROFS_DIR const *)(((UINT8 const *)apBase) + pSubs[aSubDirIx]);
}

K2ROFS_FILE const *
K2ROFSHELP_SubFileIx(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    UINT_PTR            aSubFileIx
)
{
    K2_ASSERT(apBase != NULL);
    K2_ASSERT(apDir != NULL);
    K2_ASSERT(aSubFileIx < apDir->mFileCount);
    return (K2ROFS_FILE const *)(((UINT8 const *)apDir) + sizeof(K2ROFS_DIR) + (aSubFileIx * sizeof(K2ROFS_FILE)));
}
