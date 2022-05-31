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
#include "idlx.h"

static
K2STAT
sHandoffOneDlxSector(
    pfK2DLXSUPP_ConvertLoadPtr  ConvertLoadPtr,
    K2DLX_SECTOR *              apSector
    )
{
    K2STAT          status;
    UINT32          sectionCount;
    UINT32          secIx;
    UINT32          segIx;
    UINT32          impIx;
    DLX_INFO *      pInfo;
    DLX_IMPORT *    pImport;

    sectionCount = apSector->Module.mpElf->e_shnum;

    for (secIx = 1; secIx < sectionCount; secIx++)
        apSector->mSecAddr[secIx] = apSector->mSecAddr[secIx + sectionCount];

    status = K2STAT_ERROR_UNKNOWN;

    do
    {
        // we can change these because we are the only code that
        // is traversing the list.  the converloadptr uses some other
        // mechanism to track conversions, not this list
        if (apSector->Module.ListLink.mpPrev != NULL)
        {
            if (!ConvertLoadPtr((UINT32 *)&apSector->Module.ListLink.mpPrev))
                break;
        }
        if (apSector->Module.ListLink.mpNext != NULL)
        {
            if (!ConvertLoadPtr((UINT32 *)&apSector->Module.ListLink.mpNext))
                break;
        }
        if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpIntName))
            break;
        if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpSecHdr[1].sh_addr))
            break;
        if (!ConvertLoadPtr((UINT32 *)&apSector->mSecAddr[1]))
            break;
        if (!ConvertLoadPtr((UINT32 *)&apSector->mSecAddr[1 + apSector->Module.mpElf->e_shnum]))
            break;
        if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpSecHdr[2].sh_addr))
            break;
        if (!ConvertLoadPtr((UINT32 *)&apSector->mSecAddr[2]))
            break;
        if (!ConvertLoadPtr((UINT32 *)&apSector->mSecAddr[2 + apSector->Module.mpElf->e_shnum]))
            break;
        if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpElf))
            break;
        if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpSecHdr))
            break;

        pInfo = apSector->Module.mpInfo;
        pImport = (DLX_IMPORT *)
            (((UINT8 *)pInfo) + (sizeof(DLX_INFO) - sizeof(UINT32)) + apSector->Module.mIntNameFieldLen);
        for (impIx = 0; impIx < pInfo->mImportCount; impIx++)
        {
            if (!ConvertLoadPtr((UINT32 *)&pImport->mReserved))
                break;
            pImport = (DLX_IMPORT *)(((UINT8 *)pImport) + pImport->mSizeBytes);
        }
        if (impIx < pInfo->mImportCount)
            break;

        if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpInfo))
            break;
       
        for (segIx = DlxSeg_Text; segIx < DlxSeg_Other; segIx++)
        {
            apSector->Module.SegAlloc.Segment[segIx].mDataAddr =
                apSector->Module.SegAlloc.Segment[segIx].mLinkAddr;
        }

        apSector->Module.mRefs = (UINT32)-1;
        apSector->Module.mFlags |= K2DLXSUPP_FLAG_PERMANENT;
        apSector->Module.mHostFile = 0;
        apSector->Module.mCurSector = 0;
        apSector->Module.mSectorCount = 0;
        apSector->Module.mRelocSectionCount = 0;

        if (apSector->Module.mpExpCodeDataAddr != NULL)
        {
            if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpExpCodeDataAddr))
                break;
        }
        if (apSector->Module.mpExpReadDataAddr != NULL)
        {
            if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpExpReadDataAddr))
                break;
        }
        if (apSector->Module.mpExpDataDataAddr != NULL)
        {
            if (!ConvertLoadPtr((UINT32 *)&apSector->Module.mpExpDataDataAddr))
                break;
        }

        status = K2STAT_OK;

    } while (0);

    return status;
}

K2STAT
K2DLXSUPP_Handoff(
    DLX **                      appEntryDlx,
    pfK2DLXSUPP_ConvertLoadPtr  ConvertLoadPtr,
    DLX_pf_ENTRYPOINT *         apRetEntrypoint
    )
{
    K2LIST_LINK *   pLink;
    K2DLX_PAGE *    pPage;
    BOOL            foundEntryDLX;
    K2STAT          status;

    if (gpK2DLXSUPP_Vars->mHandedOff)
        return K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_API_ORDER);

    if ((appEntryDlx == NULL) ||
        (ConvertLoadPtr == NULL) ||
        (apRetEntrypoint == NULL))
        return K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_BAD_ARGUMENT);

    *apRetEntrypoint = NULL;
    foundEntryDLX = FALSE;

    pLink = gpK2DLXSUPP_Vars->LoadedList.mpHead;

    if (pLink == NULL)
        return 0;

    if ((!ConvertLoadPtr((UINT32 *)&gpK2DLXSUPP_Vars->LoadedList.mpHead)) ||
        (!ConvertLoadPtr((UINT32 *)&gpK2DLXSUPP_Vars->LoadedList.mpTail)))
    {
        return K2STAT_DLX_ERROR_DURING_HANDOFF;
    }
    
    do
    {
        pPage = K2_GET_CONTAINER(K2DLX_PAGE, pLink, ModuleSector.Module.ListLink);
        K2_ASSERT((((UINT32)pPage) & K2_VA32_MEMPAGE_OFFSET_MASK) == 0);
        
        pLink = pPage->ModuleSector.Module.ListLink.mpNext;

        if (!foundEntryDLX)
        {
            if (*appEntryDlx == &pPage->ModuleSector.Module)
            {
                *apRetEntrypoint = (DLX_pf_ENTRYPOINT)pPage->ModuleSector.Module.mEntrypoint;
                if (!ConvertLoadPtr((UINT32 *)appEntryDlx))
                {
                    return K2STAT_DLX_ERROR_DURING_HANDOFF;
                }
            }
        }

        status = sHandoffOneDlxSector(ConvertLoadPtr, &pPage->ModuleSector);

        if (K2STAT_IS_ERROR(status))
        {
            return status;
        }

    } while (pLink != NULL);

    gpK2DLXSUPP_Vars->mHandedOff = TRUE;

    return 0;
}
