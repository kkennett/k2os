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

K2STAT
iK2DLXSUPP_FindExport(
    DLX_EXPORTS_SECTION const * apSec,
    char const *                apName,
    UINT32 *                    apRetAddr
    )
{
    UINT32          b;
    UINT32          e;
    UINT32          m;
    int             c;
    char const *    pStrBase;

    pStrBase = (char *)apSec;
    b = 0;
    e = apSec->mCount;
    do
    {
        m = b + ((e - b) / 2);
        c = K2ASC_Comp(apName, pStrBase + apSec->Export[m].mNameOffset);
        if (c == 0)
        {
            *apRetAddr = apSec->Export[m].mAddr;
            return 0;
        }
        if (c < 0)
            e = m;
        else
            b = m + 1;
    } while (b < e);

    return K2STAT_ERROR_NOT_FOUND;
}

K2STAT
DLX_FindExport(
    DLX *           apDlx,
    UINT32          aDlxSegment,
    char const *    apExportName,
    UINT32 *        apRetAddr
    )
{
    DLX_EXPORTS_SECTION const **  ppExp;
    K2STAT                        status;

    if ((aDlxSegment < DlxSeg_Text) ||
        (aDlxSegment > DlxSeg_Data))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (NULL == iK2DLXSUPP_FindAndAddRef(apDlx))
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    do
    {
        //
        // try data address first, then link address
        //
        ppExp = (DLX_EXPORTS_SECTION const **)&apDlx->mpExpCodeDataAddr;
        ppExp += (aDlxSegment - DlxSeg_Text);
        if (*ppExp == NULL)
        {
            ppExp = (DLX_EXPORTS_SECTION const **)&apDlx->mpInfo->mpExpCode;
            ppExp += (aDlxSegment - DlxSeg_Text);
            if (*ppExp == NULL)
            {
                status = K2STAT_ERROR_NOT_FOUND;
                break;
            }
        }

        status = iK2DLXSUPP_FindExport(*ppExp, apExportName, apRetAddr);

    } while (0);

    iK2DLXSUPP_ReleaseModule(apDlx);

    if (K2STAT_IS_ERROR(status))
    {
        return status;
    }

    return K2STAT_OK;
}
