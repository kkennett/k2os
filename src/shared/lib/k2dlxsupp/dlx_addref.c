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

DLX *
iK2DLXSUPP_FindAndAddRef(
    DLX * apDlx
    )
{
    K2LIST_LINK *   pLink;
    DLX *           pDlx;
    K2STAT          status;

    if (gpK2DLXSUPP_Vars->Host.CritSec != NULL)
    {
        status = gpK2DLXSUPP_Vars->Host.CritSec(TRUE);
        if (K2STAT_IS_ERROR(status))
        {
            status = K2DLXSUPP_ERRORPOINT(status);
            return NULL;
        }
    }

    pDlx = NULL;
    pLink = gpK2DLXSUPP_Vars->LoadedList.mpHead;
    while (pLink != NULL)
    {
        if (pLink == (K2LIST_LINK *)apDlx)
        {
            pDlx = K2_GET_CONTAINER(DLX, pLink, ListLink);
            if (0 == (pDlx->mFlags & K2DLXSUPP_FLAG_PERMANENT))
            {
                if (gpK2DLXSUPP_Vars->Host.RefChange != NULL)
                    gpK2DLXSUPP_Vars->Host.RefChange(pDlx->mHostFile, pDlx, 1);
                pDlx->mRefs++;
            }
            break;
        }
        pLink = pLink->mpNext;
    }

    if (gpK2DLXSUPP_Vars->Host.CritSec != NULL)
        gpK2DLXSUPP_Vars->Host.CritSec(FALSE);

    if (pDlx == NULL)
    {
        status = K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_NOT_FOUND);
    }

    return pDlx;
}

K2STAT
DLX_AddRef(
    DLX *   apDlx
)
{
    DLX * pDlx;

    pDlx = iK2DLXSUPP_FindAndAddRef(apDlx);
    if (pDlx == NULL)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    return K2STAT_OK;
}

