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

#if K2_TARGET_ARCH_IS_32BIT
#define EXPORT_REF  XDL_EXPORT32_REF
#else
#define EXPORT_REF  XDL_EXPORT64_REF
#endif

K2STAT
IXDL_FindExport(
    XDL_EXPORTS_SEGMENT_HEADER const *  apSeg,
    char const *                        apName,
    UINT_PTR *                          apRetAddr
)
{
    UINT_PTR        b;
    UINT_PTR        e;
    UINT_PTR        m;
    int             c;
    EXPORT_REF *    pExpRef;
    char const *    pStrBase;

    pStrBase = (char *)apSeg;
    pExpRef = (EXPORT_REF *)(pStrBase + sizeof(XDL_EXPORTS_SEGMENT_HEADER));
    b = 0;
    e = apSeg->mCount;
    do
    {
        m = b + ((e - b) / 2);
        c = K2ASC_Comp(apName, pStrBase + pExpRef[m].mNameOffset);
        if (c == 0)
        {
            *apRetAddr = pExpRef[m].mAddr;
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
XDL_FindExport(
    XDL *           apXdl, 
    XDLProgDataType aType, 
    char const *    apName, 
    UINT_PTR *      apRetAddr
)
{
    XDL_EXPORTS_SEGMENT_HEADER const *  pSeg;
    K2STAT                              status;

    if (aType >= XDLProgDataType_Count)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (!IXDL_FindAndAddRef(apXdl))
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    K2_ASSERT(aType >= XDLProgData_Text);
    K2_ASSERT(aType < XDLProgDataType_Count);

    pSeg = apXdl->mpExpHdr[aType];
    if (NULL == pSeg)
    {
        status = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        status = IXDL_FindExport(pSeg, apName, apRetAddr);
    }

    IXDL_ReleaseModule(apXdl);

    return status;
}
