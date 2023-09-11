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

void
IXDL_ReleaseModule(
    XDL *   apXdl
)
{
    K2XDL_LOADCTX       temp;
    XDL_IMPORT *        pImport;
    UINT_PTR            importCount;
    K2XDL_SEGMENT_ADDRS segAddrs;

    if (apXdl->mFlags & XDL_FLAG_PERMANENT)
        return;

    --apXdl->mRefs;
    if (apXdl->mRefs > 0)
        return;

    if (apXdl->mFlags & XDL_FLAG_FULLY_LOADED)
    {
        IXDL_DoCallback(apXdl, FALSE);
        K2LIST_Remove(&gpXdlGlobal->LoadedList, &apXdl->ListLink);
    }
    else
    {
        K2LIST_Remove(&gpXdlGlobal->AcqList, &apXdl->ListLink);
    }

    importCount = ((UINT_PTR)apXdl->mpHeader->Segment[XDLSegmentIx_Header].mMemActualBytes) - apXdl->mpHeader->mImportsOffset;
    importCount /= sizeof(XDL_IMPORT);
    pImport = apXdl->mpImports;
    if (0 != importCount)
    {
        //
        // release imports
        //
        do {
            if (0 != pImport->mReserved)
            {
                IXDL_ReleaseModule((XDL *)(UINT_PTR)pImport->mReserved);
                pImport->mReserved = 0;
            }
            pImport++;
        } while (0 != --importCount);
    }

    K2MEM_Copy(&temp, apXdl->mpLoadCtx, sizeof(temp));
    K2MEM_Copy(&segAddrs, &apXdl->SegAddrs, sizeof(K2XDL_SEGMENT_ADDRS));
    segAddrs.mData[XDLSegmentIx_Header] = segAddrs.mLink[XDLSegmentIx_Header] = 0;

    gpXdlGlobal->Host.Purge(&temp, &segAddrs);
}

K2STAT  
XDL_Release(
    XDL *   apXdl
)
{
    K2LIST_LINK *   pLink;
    K2STAT          status;

    if (gpXdlGlobal->Host.CritSec != NULL)
    {
        status = gpXdlGlobal->Host.CritSec(TRUE);
        if (K2STAT_IS_ERROR(status))
        {
            return status;
        }
    }

    pLink = gpXdlGlobal->LoadedList.mpHead;
    while (pLink != NULL)
    {
        if (pLink == (K2LIST_LINK *)apXdl)
            break;
        pLink = pLink->mpNext;
    }
    if (pLink != NULL)
    {
        IXDL_ReleaseModule(apXdl);
    }

    if (gpXdlGlobal->Host.CritSec != NULL)
    {
        gpXdlGlobal->Host.CritSec(FALSE);
    }

    if (pLink == NULL)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    return K2STAT_OK;
}
