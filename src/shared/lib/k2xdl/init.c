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

XDL_GLOBAL * gpXdlGlobal = NULL;

K2STAT 
K2XDL_Init(
    void *      apXdlControlPage, 
    K2XDL_HOST *apHost, 
    BOOL        aKeepSym, 
    BOOL        aReInit
)
{
    K2LIST_LINK *   pListLink;
    XDL *           pXdl;

    if (apXdlControlPage == NULL)
        return K2STAT_ERROR_BAD_ARGUMENT;

    gpXdlGlobal = (XDL_GLOBAL *)apXdlControlPage;
    
    if (!aReInit)
        K2MEM_Zero(apXdlControlPage, K2_VA_MEMPAGE_BYTES);

    if (apHost != NULL)
    {
        K2MEM_Copy(&gpXdlGlobal->Host, apHost, sizeof(K2XDL_HOST));
    }
    else
    {
        K2MEM_Zero(&gpXdlGlobal->Host, sizeof(K2XDL_HOST));
    }

    gpXdlGlobal->mAcqDisabled = FALSE;

    gpXdlGlobal->mKeepSym = aKeepSym;

    if (!aReInit)
    {
        K2LIST_Init(&gpXdlGlobal->LoadedList);
        K2LIST_Init(&gpXdlGlobal->AcqList);
    }
    else
    {
        gpXdlGlobal->mHandedOff = FALSE;

        if (gpXdlGlobal->Host.AtReInit != NULL)
        {
            pListLink = gpXdlGlobal->LoadedList.mpHead;
            while (pListLink != NULL)
            {
                pXdl = K2_GET_CONTAINER(XDL, pListLink, ListLink);
                gpXdlGlobal->Host.AtReInit(pXdl, pXdl->mpLoadCtx->mModulePageLinkAddr, &pXdl->mpLoadCtx->mHostFile);
                pListLink = pListLink->mpNext;
            }
        }
    }

    return K2STAT_OK;
}

K2STAT 
K2XDL_InitSelf(
    void *                  apXdlControlPage, 
    void *                  apSelfPage, 
    XDL_FILE_HEADER const * apHeader, 
    K2XDL_HOST *            apHost, 
    BOOL                    aKeepSym
)
{
    XDL *           pXdl;
    UINT_PTR        ix;

    if ((apXdlControlPage == NULL) ||
        (apSelfPage == NULL) ||
        (apHeader == NULL))
        return K2STAT_ERROR_BAD_ARGUMENT;

    gpXdlGlobal = (XDL_GLOBAL *)apXdlControlPage;

    K2MEM_Zero(apXdlControlPage, K2_VA_MEMPAGE_BYTES);

    if (apHost != NULL)
    {
        K2MEM_Copy(&gpXdlGlobal->Host, apHost, sizeof(K2XDL_HOST));
    }
    else
    {
        K2MEM_Zero(&gpXdlGlobal->Host, sizeof(K2XDL_HOST));
    }

    gpXdlGlobal->mAcqDisabled = FALSE;

    gpXdlGlobal->mKeepSym = aKeepSym;

    K2LIST_Init(&gpXdlGlobal->LoadedList);
    K2LIST_Init(&gpXdlGlobal->AcqList);

    pXdl = (XDL *)apSelfPage;
    pXdl->mRefs = (INT32)-1;
    pXdl->mpHeader = (XDL_FILE_HEADER *)apHeader;
    pXdl->mFlags = XDL_FLAG_PERMANENT;
    pXdl->mFlags |= XDL_FLAG_ENTRY_CALLED;
    pXdl->mFlags |= XDL_FLAG_FULLY_LOADED;
    if (apHeader->Segment[XDLSegmentIx_Symbols].mMemActualBytes > 0)
        pXdl->mFlags |= XDL_FLAG_KEEP_SYMBOLS;
    pXdl->mpImports = NULL;
    pXdl->mpLoadCtx = (K2XDL_LOADCTX *)(((UINT_PTR)pXdl) + sizeof(XDL));
    pXdl->mpLoadCtx->mModulePageDataAddr = (UINT_PTR)apSelfPage;
    pXdl->mpLoadCtx->mModulePageLinkAddr = (UINT_PTR)apSelfPage;
    pXdl->mpLoadCtx->OpenArgs.mpPath = apHeader->mName;
    pXdl->mpLoadCtx->OpenArgs.mpNamePart = apHeader->mName;
    pXdl->mpLoadCtx->OpenArgs.mNameLen = apHeader->mNameLen;
    for (ix = 0; ix < XDLSegmentIx_Count; ix++)
    {
        pXdl->SegAddrs.mData[ix] = pXdl->SegAddrs.mLink[ix] = apHeader->Segment[ix].mLinkAddr;
    }
    for (ix = 0; ix < XDLProgDataType_Count; ix++)
    {
        if (0 != apHeader->mReadExpOffset[ix])
        {
            pXdl->mpExpHdr[ix] = (XDL_EXPORTS_SEGMENT_HEADER *)
                ((UINT_PTR)(pXdl->SegAddrs.mData[XDLSegmentIx_Read] +
                    apHeader->mReadExpOffset[ix]));
        }
    }
    K2LIST_AddAtTail(&gpXdlGlobal->LoadedList, &pXdl->ListLink);

    return K2STAT_NO_ERROR;
}
