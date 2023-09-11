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

#include "kern.h"

BOOL
K2OS_Xdl_AddRef(
    K2OS_XDL aXdl
)
{
    K2STAT  stat;

    stat = XDL_AddRef((XDL *)aXdl);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

K2OS_XDL
K2OS_Xdl_AddRefContaining(
    UINT32 aAddr
)
{
    K2STAT  stat;
    UINT32  segIx;
    XDL *   pXdl;

    stat = XDL_AcquireContaining(aAddr, &pXdl, &segIx);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("AcqContaining(%08X) error %08x\n", aAddr, stat);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    return (K2OS_XDL)pXdl;
}

BOOL
K2OS_Xdl_Release(
    K2OS_XDL aXdl
)
{
    K2STAT stat;

    stat = XDL_Release((XDL *)aXdl);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

UINT32
K2OS_Xdl_FindExport(
    K2OS_XDL    aXdl,
    BOOL        aIsText,
    char const *apExportName
)
{
    K2STAT stat;
    UINT32 addr;

    addr = 0;

    stat = XDL_FindExport((XDL *)aXdl, aIsText ? XDLProgData_Text : XDLProgData_Data, apExportName, &addr);
    if (!K2STAT_IS_ERROR(stat))
        return addr;

    if (!aIsText)
    {
        stat = XDL_FindExport((XDL *)aXdl, XDLProgData_Read, apExportName, &addr);
        if (!K2STAT_IS_ERROR(stat))
            return addr;
    }

    K2OS_Thread_SetLastStatus(stat);

    return 0;
}

BOOL
K2OS_Xdl_GetIdent(
    K2OS_XDL        aXdl,
    char *          apNameBuf,
    UINT32          aNameBufLen,
    K2_GUID128  *   apRetID
)
{
    K2STAT                  stat;
    XDL_FILE_HEADER const * pHeader;

    if (!K2OS_Xdl_AddRef(aXdl))
        return FALSE;

    stat = XDL_GetHeaderPtr((XDL *)aXdl, &pHeader);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        K2OS_Xdl_Release(aXdl);
        return FALSE;
    }

    if (NULL != apNameBuf)
    {
        K2_ASSERT(0 != aNameBufLen);
        if (aNameBufLen > pHeader->mNameLen)
            aNameBufLen = pHeader->mNameLen;
        K2MEM_Copy(apNameBuf, pHeader->mName, aNameBufLen);
    }

    if (NULL != apRetID)
    {
        K2MEM_Copy(apRetID, &pHeader->Id, sizeof(K2_GUID128));
    }

    return TRUE;
}
