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
K2DLXSUPP_GetInfo(
    DLX *               apDlx,
    UINT32 *            apRetFlags,
    DLX_pf_ENTRYPOINT * apRetEntrypoint,
    DLX_SEGMENT_INFO *  apRetSegInfo,
    UINT32 *            apRetPageAddr,
    char const **       appRetFileName
    )
{
    if (apRetFlags != NULL)
        *apRetFlags = 0;
    
    if (apRetEntrypoint != NULL)
        *apRetEntrypoint = 0;
    
    if (apRetSegInfo != NULL)
        K2MEM_Zero(apRetSegInfo, sizeof(DLX_SEGMENT_INFO) * DlxSeg_Count);

    if (apDlx == NULL)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (!gpK2DLXSUPP_Vars->mHandedOff)
    {
        if (NULL == iK2DLXSUPP_FindAndAddRef(apDlx))
        {
            return K2STAT_ERROR_NOT_FOUND;
        }
    }

    if (apRetPageAddr != NULL)
        *apRetPageAddr = (UINT32)apDlx;

    if (apRetFlags != NULL)
        *apRetFlags = apDlx->mFlags;
    
    if (apRetEntrypoint != NULL)
        *apRetEntrypoint = (DLX_pf_ENTRYPOINT)apDlx->mEntrypoint;

    if (appRetFileName != NULL)
        *appRetFileName = apDlx->mpInfo->mFileName;

    if (!gpK2DLXSUPP_Vars->mHandedOff)
    {
        if (apRetSegInfo != NULL)
            K2MEM_Copy(apRetSegInfo, apDlx->mpInfo->SegInfo, sizeof(DLX_SEGMENT_INFO) * DlxSeg_Count);

        DLX_Release(apDlx);
    }

    return K2STAT_OK;
}
