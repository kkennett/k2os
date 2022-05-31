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

void
iK2DLXSUPP_ReleaseModule(
    DLX *apDlx
);

void
iK2DLXSUPP_ReleaseImports(
    DLX *   apDlx,
    UINT32  aAcqCount
    )
{
    UINT8 *         pWork;
    UINT8 *         pStartImports;
    UINT32          relIx;
    DLX_IMPORT *    pImport;
    DLX *           pImportDlx;

    if (aAcqCount == 0)
        return;

    pWork = (UINT8 *)apDlx->mpInfo;
    pWork += sizeof(DLX_INFO) - 4 + apDlx->mIntNameFieldLen;

    pStartImports = pWork;

    do
    {
        aAcqCount--;

        pWork = pStartImports;
        pImport = (DLX_IMPORT *)pWork;
        for (relIx = 0;relIx < aAcqCount; relIx++)
        {
            pWork += pImport->mSizeBytes;
            pImport = (DLX_IMPORT *)pWork;
        }

        pImportDlx = (DLX *)pImport->mReserved;

        iK2DLXSUPP_ReleaseModule(pImportDlx);

        pImport->mReserved = 0;

    } while (aAcqCount > 0);
}

void
iK2DLXSUPP_ReleaseModule(
    DLX *apDlx
    )
{
    if (apDlx->mFlags & K2DLXSUPP_FLAG_PERMANENT)
        return;

    if (gpK2DLXSUPP_Vars->Host.RefChange != NULL)
        gpK2DLXSUPP_Vars->Host.RefChange(apDlx->mHostFile, apDlx, -1);
    --apDlx->mRefs;

    if (apDlx->mRefs > 0)
        return;

    if (apDlx->mFlags & K2DLXSUPP_FLAG_FULLY_LOADED)
    {
        iK2DLXSUPP_DoCallback(NULL, apDlx, FALSE);
        K2LIST_Remove(&gpK2DLXSUPP_Vars->LoadedList, &apDlx->ListLink);
    }
    else
        K2LIST_Remove(&gpK2DLXSUPP_Vars->AcqList, &apDlx->ListLink);

    iK2DLXSUPP_ReleaseImports(apDlx, apDlx->mpInfo->mImportCount);

    gpK2DLXSUPP_Vars->Host.Purge(apDlx->mHostFile);
}

K2STAT
DLX_Release(
    DLX * apDlx
)
{
    K2LIST_LINK *   pLink;
    K2STAT          status;

    if (gpK2DLXSUPP_Vars->Host.CritSec != NULL)
    {
        status = gpK2DLXSUPP_Vars->Host.CritSec(TRUE);
        if (K2STAT_IS_ERROR(status))
        {
            return status;
        }
    }

    pLink = gpK2DLXSUPP_Vars->LoadedList.mpHead;
    while (pLink != NULL)
    {
        if (pLink == (K2LIST_LINK *)apDlx)
            break;
        pLink = pLink->mpNext;
    }
    if (pLink != NULL)
    {
        iK2DLXSUPP_ReleaseModule(apDlx);
    }

    if (gpK2DLXSUPP_Vars->Host.CritSec != NULL)
        gpK2DLXSUPP_Vars->Host.CritSec(FALSE);

    if (pLink == NULL)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    return K2STAT_OK;
}

