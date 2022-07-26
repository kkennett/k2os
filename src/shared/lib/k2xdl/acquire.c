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

#include "ixdl.h"

K2STAT
IXDL_Prep(
    UINT_PTR            aContext,
    char const *        apFilePath,
    char const *        apName,
    UINT_PTR            aNameLen,
    K2_GUID128 const *  apMatchId,
    XDL **              appRetXdl
)
{
    K2STAT              stat;
    XDL_PAGE *          pPage;
    XDL *               pXdl;
    K2XDL_LOADCTX       temp;
    UINT_PTR            segIx;
    UINT32              crc32;
    XDL_FILE_HEADER *   pHeader;
    UINT64              secLeft;
    UINT_PTR            importCount;

    if ((NULL == gpXdlGlobal->Host.Open) ||
        (NULL == gpXdlGlobal->Host.ReadSectors))
        return K2STAT_ERROR_NOT_IMPL;

    temp.OpenArgs.mAcqContext = aContext;
    temp.OpenArgs.mpFilePath = apFilePath;
    temp.OpenArgs.mpNamePart = apName;
    temp.OpenArgs.mNameLen = aNameLen;
    temp.mHostFile = 0;
    temp.mModulePageDataAddr = 0;
    temp.mModulePageLinkAddr = 0;

    stat = gpXdlGlobal->Host.Open(&temp.OpenArgs, &temp.mHostFile, &temp.mModulePageDataAddr, &temp.mModulePageLinkAddr);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    do {
        if ((0 == temp.mModulePageDataAddr) ||
            (0 == temp.mModulePageLinkAddr))
        {
            stat = K2STAT_ERROR_INCOMPLETE;
            break;
        }
        if ((0 != (K2_VA_MEMPAGE_OFFSET_MASK & temp.mModulePageDataAddr)) ||
            (0 != (K2_VA_MEMPAGE_OFFSET_MASK & temp.mModulePageLinkAddr)))
        {
            stat = K2STAT_ERROR_BAD_ALIGNMENT;
            break;
        }

        pPage = (XDL_PAGE *)temp.mModulePageDataAddr;

        pXdl = &pPage->ModuleSector.Module;

        K2MEM_Zero(pXdl, sizeof(XDL));
        pXdl->mFlags = gpXdlGlobal->mKeepSym ? XDL_FLAG_KEEP_SYMBOLS : 0;
        pXdl->mpLoadCtx = &pPage->ModuleSector.LoadCtx;
        pXdl->mRefs = 1;
        K2MEM_Copy(pXdl->mpLoadCtx, &temp, sizeof(K2XDL_LOADCTX));
        for (segIx = 0; segIx < XDLProgDataType_Count; segIx++)
        {
            K2TREE_Init(&pXdl->SymTree[segIx], NULL);
        }

        secLeft = 1;
        stat = gpXdlGlobal->Host.ReadSectors(pXdl->mpLoadCtx, pPage->mHdrSectorsBuffer, &secLeft);
        if (K2STAT_IS_ERROR(stat))
            break;

        pHeader = pXdl->mpHeader = (XDL_FILE_HEADER *)pPage->mHdrSectorsBuffer;
        if (pHeader->mMarker != XDL_FILE_HEADER_MARKER)
        {
            stat = K2STAT_ERROR_BAD_FORMAT;
            break;
        }

        crc32 = K2CRC_Calc32(0, &pHeader->mMarker, sizeof(XDL_FILE_HEADER) - sizeof(UINT32));
        if (crc32 != pHeader->mHeaderCrc32)
        {
            stat = K2STAT_ERROR_CORRUPTED;
            break;
        }
        pHeader->mHeaderCrc32 = 0;

        if (pHeader->mHeaderBytes < sizeof(XDL_FILE_HEADER))
        {
            stat = K2STAT_ERROR_TOO_SMALL;
            break;
        }

        if (pHeader->mImportsOffset < sizeof(XDL_FILE_HEADER))
        {
            stat = K2STAT_ERROR_BAD_FORMAT;
            break;
        }

        if (pHeader->mNameLen > XDL_NAME_MAX_LEN)
        {
            stat = K2STAT_ERROR_TOO_BIG;
            break;
        }

        secLeft = pHeader->Segment[XDLSegmentIx_Header].mSectorCount;
        if (0 == secLeft)
        {
            stat = K2STAT_ERROR_TOO_SMALL;
            break;
        }
        secLeft--;  // we read a sector already
        if (0 != secLeft)
        {
            //
            // need more sectors to complete header segment
            //
            if (secLeft > 2)
            {
                //
                // need to resize xdl header segment in memory to hold the extra sectors
                // +1 for one we already loaded
                // +1 for tracking at start of pages
                //
                segIx = (UINT_PTR)((((secLeft + 2) * XDL_SECTOR_BYTES) + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES);
                stat = gpXdlGlobal->Host.ResizeCopyModulePage(
                    &temp,
                    segIx,
                    &temp.mModulePageDataAddr,
                    &temp.mModulePageLinkAddr
                );
                if (K2STAT_IS_ERROR(stat))
                {
                    break;
                }

                pPage = (XDL_PAGE *)temp.mModulePageDataAddr;
                pXdl = &pPage->ModuleSector.Module;
                pXdl->mpLoadCtx = &pPage->ModuleSector.LoadCtx;
                K2MEM_Copy(pXdl->mpLoadCtx, &temp, sizeof(K2XDL_LOADCTX));
                pHeader = pXdl->mpHeader = (XDL_FILE_HEADER *)pPage->mHdrSectorsBuffer;
            }

            stat = gpXdlGlobal->Host.ReadSectors(pXdl->mpLoadCtx, ((UINT8 *)pHeader) + XDL_SECTOR_BYTES, &secLeft);
            if (K2STAT_IS_ERROR(stat))
                break;
        }

        pXdl->mpImports = (XDL_IMPORT *)(((UINT8 *)pHeader) + pHeader->mImportsOffset);
        importCount = (UINT_PTR)(pHeader->Segment[XDLSegmentIx_Header].mMemActualBytes - pHeader->mImportsOffset);
        importCount /= sizeof(XDL_IMPORT);

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        if (NULL != gpXdlGlobal->Host.Purge)
            gpXdlGlobal->Host.Purge(&temp);
    }

    K2MEM_Zero(&temp, sizeof(temp));

    return stat;
}

K2STAT
IXDL_ExecLoads(
    void
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

XDL *
IXDL_FindModuleOnList(
    K2LIST_ANCHOR *     apAnchor,
    char const *        apName,
    UINT_PTR            aNameLen,
    K2_GUID128 const *  apMatchId
)
{
    XDL *               pXdl;
    K2LIST_LINK *       pLink;
    XDL_FILE_HEADER *   pHeader;

    pXdl = NULL;
    pLink = apAnchor->mpHead;
    while (pLink != NULL)
    {
        pXdl = K2_GET_CONTAINER(XDL, pLink, ListLink);
        pHeader = pXdl->mpHeader;
        if ((apMatchId == NULL) ||
            (0 == K2MEM_Compare(apMatchId, &pHeader->Id, sizeof(K2_GUID128))))
        {
            if (pHeader->mNameLen == aNameLen)
            {
                if (!K2ASC_CompInsLen(apName, pHeader->mName, aNameLen))
                    break;
            }
        }
        pLink = pLink->mpNext;
    }
    if (pLink == NULL)
        return NULL;
    return pXdl;
}

K2STAT
IXDL_Acquire(
    UINT_PTR            aContext,
    char const *        apFilePath,
    char const *        apName,
    UINT_PTR            aNameLen,
    K2_GUID128 const *  apMatchId,
    XDL **              appRetXdl
)
{
    XDL *           pXdl;
    K2STAT          status;

    //
    // apMatchId will only be NULL for top-level load
    // 
    pXdl = IXDL_FindModuleOnList(&gpXdlGlobal->LoadedList, apName, aNameLen, apMatchId);
    if (pXdl != NULL)
    {
        //
        // already fully loaded dlx
        //
        pXdl->mRefs++;
        *appRetXdl = pXdl;
        return K2STAT_OK;
    }

    if (apMatchId != NULL)
    {
        //
        // check for partially loaded dlx
        //
        pXdl = IXDL_FindModuleOnList(&gpXdlGlobal->AcqList, apName, aNameLen, apMatchId);
        if (pXdl != NULL)
        {
            if (0 == (pXdl->mFlags & XDL_FLAG_PERMANENT))
            {
                pXdl->mRefs++;
            }
            *appRetXdl = pXdl;
            return K2STAT_OK;
        }
    }

    status = IXDL_Prep(aContext, apFilePath, apFilePath, aNameLen, apMatchId, appRetXdl);
    if (K2STAT_IS_ERROR(status))
    {
        *appRetXdl = NULL;
        return status;
    }

    if (apMatchId == NULL)
    {
        //
        // top level load
        //
        status = IXDL_ExecLoads();

        // if we failed we only need to release the instigating node
        // and it will transitively release all the other ones that
        // have been acquired or are pending load
        if (K2STAT_IS_ERROR(status))
        {
            pXdl = *appRetXdl;
            IXDL_ReleaseModule(pXdl);
            *appRetXdl = NULL;
        }
    }

    return status;
}

K2STAT  
XDL_Acquire(
    char const *apFilePath, 
    UINT_PTR    aContext, 
    XDL **      appRetXdl
)
{
    K2STAT          status;
    XDL *           pXdl;
    UINT32          nameLen;
    char            ch;
    char const *    pScan;

    if ((apFilePath == NULL) || ((*apFilePath) == 0) || (appRetXdl == NULL))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    *appRetXdl = NULL;

    nameLen = 0;
    pScan = apFilePath;
    while (*pScan)
        pScan++;
    do
    {
        pScan--;
        ch = *pScan;
        if (ch == '.')
            nameLen = 0;
        else
        {
            if ((ch == '/') || (ch == '\\'))
            {
                pScan++;
                break;
            }
            nameLen++;
        }
    } while (pScan != apFilePath);

    if (nameLen == 0)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (gpXdlGlobal->Host.CritSec != NULL)
    {
        status = gpXdlGlobal->Host.CritSec(TRUE);
        if (K2STAT_IS_ERROR(status))
        {
            return status;
        }
    }

    if (gpXdlGlobal->mAcqDisabled)
    {
        if (gpXdlGlobal->Host.CritSec != NULL)
            gpXdlGlobal->Host.CritSec(FALSE);
        return K2STAT_ERROR_API_ORDER;
    }

    K2LIST_Init(&gpXdlGlobal->AcqList);

    status = IXDL_Acquire(aContext, apFilePath, pScan, nameLen, NULL, &pXdl);

    if (gpXdlGlobal->Host.CritSec != NULL)
        gpXdlGlobal->Host.CritSec(FALSE);

    if (!K2STAT_IS_ERROR(status))
    {
        *appRetXdl = pXdl;
    }

    return status;
}
