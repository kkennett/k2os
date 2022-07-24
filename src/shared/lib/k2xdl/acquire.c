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
IXDL_Acquire(
    UINT_PTR            aContext,
    char const *        apFilePath,
    char const *        apName,
    UINT_PTR            aNameLen,
    K2_GUID128 const *  apMatchId,
    XDL **              appRetXdl
)
{
    return K2STAT_ERROR_NOT_IMPL;
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
