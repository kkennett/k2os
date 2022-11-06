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
IXDL_AcqContain(
    UINT_PTR    aAddr,
    XDL **      appRetXdl,
    UINT_PTR *  apRetSegment
)
{
    K2LIST_LINK *       pListLink;
    XDL *               pXdl;
    UINT_PTR            ixSeg;
    XDL_FILE_HEADER *   pHeader;
    UINT_PTR            v;

    pListLink = gpXdlGlobal->LoadedList.mpHead;
    if (NULL == pListLink)
        return K2STAT_ERROR_NOT_FOUND;

    do {
        pXdl = K2_GET_CONTAINER(XDL, pListLink, ListLink);
        pHeader = pXdl->mpHeader;
        for (ixSeg = XDLSegmentIx_Text; ixSeg <= XDLSegmentIx_Data; ixSeg++)
        {
            v = (UINT_PTR)pHeader->Segment[ixSeg].mLinkAddr;
            if ((v <= aAddr) && ((aAddr - v) < pHeader->Segment[ixSeg].mMemActualBytes))
            {
                ++pXdl->mRefs;
                *appRetXdl = pXdl;
                *apRetSegment = ixSeg;
                return K2STAT_NO_ERROR;
            }
        }
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    return K2STAT_ERROR_NOT_FOUND;
}

K2STAT  
XDL_AcquireContaining(
    UINT_PTR    aAddr,
    XDL **      appRetXdl,
    UINT_PTR *  apRetSegment
)
{
    K2STAT          stat;

    *appRetXdl = NULL;
    *apRetSegment = XDLSegmentIx_Count;
    stat = K2STAT_ERROR_NOT_FOUND;

    if (NULL != gpXdlGlobal->Host.CritSec)
    {
        stat = gpXdlGlobal->Host.CritSec(TRUE);
        if (K2STAT_IS_ERROR(stat))
            return stat;
    }

    stat = IXDL_AcqContain(aAddr, appRetXdl, apRetSegment);

    if (NULL != gpXdlGlobal->Host.CritSec)
    {
        gpXdlGlobal->Host.CritSec(FALSE);
    }

    return stat;
}
