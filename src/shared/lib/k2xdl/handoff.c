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

#include "ixdl.h"

K2STAT
IXDL_HandoffOne(
    K2XDL_pf_ConvertLoadPtr ConvertLoadPtr,
    XDL_SECTOR *            apSector
)
{
    K2STAT              stat;
    XDL *               pXdl;
    XDL_FILE_HEADER *   pHeader;
    UINT_PTR            importCount;
    XDL_IMPORT *        pImport;

    pXdl = &apSector->Module;
    pHeader = pXdl->mpHeader;

    for (importCount = 0; importCount < XDLSegmentIx_Count; importCount++)
    {
        pXdl->SegAddrs.mData[importCount] = pXdl->SegAddrs.mLink[importCount];
    }

    importCount = ((UINT_PTR)(pHeader->Segment[XDLSegmentIx_Header].mMemActualBytes - pHeader->mImportsOffset)) / sizeof(XDL_IMPORT);

    pXdl->mRefs = (INT_PTR)-1;
    pXdl->mFlags |= XDL_FLAG_PERMANENT;

    stat = K2STAT_ERROR_UNKNOWN;

    do {
        if (!ConvertLoadPtr((UINT_PTR *)&pXdl->ListLink.mpPrev))
            break;

        if (!ConvertLoadPtr((UINT_PTR *)&pXdl->ListLink.mpNext))
            break;

        if (!ConvertLoadPtr((UINT_PTR *)&apSector->LoadCtx.OpenArgs.mpNamePart))
            apSector->LoadCtx.OpenArgs.mpNamePart = NULL;

        if (!ConvertLoadPtr((UINT_PTR *)&apSector->LoadCtx.OpenArgs.mpParentLoadCtx))
            break;

        if (!ConvertLoadPtr((UINT_PTR *)&apSector->LoadCtx.OpenArgs.mpPath))
            apSector->LoadCtx.OpenArgs.mpPath = NULL;

        if (!ConvertLoadPtr((UINT_PTR *)&pXdl->mpExpHdr[XDLProgData_Text]))
            break;

        if (!ConvertLoadPtr((UINT_PTR *)&pXdl->mpExpHdr[XDLProgData_Read]))
            break;

        if (!ConvertLoadPtr((UINT_PTR *)&pXdl->mpExpHdr[XDLProgData_Data]))
            break;

        if (!ConvertLoadPtr((UINT_PTR *)&pXdl->mpHeader))
            break;

        if (!ConvertLoadPtr((UINT_PTR *)&pXdl->mpLoadCtx))
            break;

        if (0 != importCount)
        {
            pImport = pXdl->mpImports;
            do {
                if (!ConvertLoadPtr((UINT_PTR *)&pImport->mReserved))
                    break;
                pImport++;
            } while (--importCount);
            if (0 != importCount)
                break;
        }

        if (!ConvertLoadPtr((UINT_PTR *)&pXdl->mpImports))
            break;

        //
        // entry point value in header is already at link address once module is loaded
        //
        
        stat = K2STAT_NO_ERROR;

    } while (0);

    return stat;
}

K2STAT 
K2XDL_Handoff(
    XDL **                  appEntryXdl, 
    K2XDL_pf_ConvertLoadPtr ConvertLoadPtr, 
    XDL_pf_ENTRYPOINT *     apRetEntrypoint
)
{
    K2LIST_LINK *   pListLink;
    XDL_PAGE *      pPage;
    BOOL            foundEntry;
    K2STAT          stat;

    if (gpXdlGlobal->mHandedOff)
        return K2STAT_ERROR_API_ORDER;

    if ((NULL == appEntryXdl) ||
        (NULL == ConvertLoadPtr) ||
        (NULL == apRetEntrypoint))
        return K2STAT_ERROR_BAD_ARGUMENT;

    *apRetEntrypoint = NULL;
    foundEntry = FALSE;

    pListLink = gpXdlGlobal->LoadedList.mpHead;
    if (NULL == pListLink)
        return K2STAT_NO_ERROR;

    if ((!ConvertLoadPtr((UINT_PTR *)&gpXdlGlobal->LoadedList.mpHead)) ||
        (!ConvertLoadPtr((UINT_PTR *)&gpXdlGlobal->LoadedList.mpTail)))
        return K2STAT_ERROR_ADDR_NOT_ACCESSIBLE;

    do {
        pPage = K2_GET_CONTAINER(XDL_PAGE, pListLink, ModuleSector.Module.ListLink);
        K2_ASSERT(0 == (((UINT_PTR)pPage) & K2_VA_MEMPAGE_OFFSET_MASK));

        pListLink = pPage->ModuleSector.Module.ListLink.mpNext;

        if (!foundEntry)
        {
            if (*appEntryXdl == &pPage->ModuleSector.Module)
            {
                *apRetEntrypoint = (XDL_pf_ENTRYPOINT)(UINT_PTR)pPage->ModuleSector.Module.mpHeader->mEntryPoint;
                if (!ConvertLoadPtr((UINT_PTR *)appEntryXdl))
                    return K2STAT_ERROR_ADDR_NOT_ACCESSIBLE;
            }
        }

        stat = IXDL_HandoffOne(ConvertLoadPtr, &pPage->ModuleSector);
        if (K2STAT_IS_ERROR(stat))
            return stat;

    } while (NULL != pListLink);

    gpXdlGlobal->mHandedOff = TRUE;

    return K2STAT_NO_ERROR;
}
