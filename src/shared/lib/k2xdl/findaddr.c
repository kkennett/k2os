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

static char const * const sgpSegName[XDLProgDataType_Count + 1] =
{
    "HEADER",   // not used, but correct!
    "TEXT",
    "READ",
    "DATA"
};

#if K2_TARGET_ARCH_IS_32BIT
#define SYMTYPE Elf32_Sym
#else
#define SYMTYPE Elf64_Sym
#endif

K2STAT  
XDL_FindAddrName(
    XDL *       apXdl,
    UINT_PTR    aAddr, 
    char *      apRetNameBuffer, 
    UINT_PTR    aBufferLen
)
{
    UINT_PTR            segIx;
    BOOL                found;
    XDL_FILE_HEADER *   pHeader;
    char const *        pSymSeg;
    SYMTYPE const *     pSym;
    UINT_PTR            symLeft;
    UINT_PTR            v;

    K2_ASSERT(NULL != apXdl);
    pHeader = apXdl->mpHeader;
    K2_ASSERT(NULL != pHeader);
    K2_ASSERT(pHeader->mMarker == XDL_FILE_HEADER_MARKER);

    //
    // find the segment range that matches the address
    //
    for (segIx = XDLSegmentIx_Text; segIx <= XDLSegmentIx_Data; segIx++)
    {
        v = (UINT_PTR)pHeader->Segment[segIx].mLinkAddr;
        if ((v <= aAddr) &&
            ((aAddr - v) < (UINT_PTR)pHeader->Segment[segIx].mMemActualBytes))
        {
            found = FALSE;

            if ((0 != (apXdl->mFlags & XDL_FLAG_KEEP_SYMBOLS)) &&
                (0 != (apXdl->SegAddrs.mLink[XDLSegmentIx_Symbols])))
            {
                symLeft = pHeader->mSymCount[segIx - (XDLSegmentIx_Text - XDLProgData_Text)];
                if (0 != symLeft)
                {
                    pSymSeg = (char const *)(UINT_PTR)apXdl->SegAddrs.mLink[XDLSegmentIx_Symbols];
                    pSym = (SYMTYPE const *)pSymSeg;
                    pSym += pHeader->mSymCount_Abs;
                    if (segIx > XDLSegmentIx_Text)
                        pSym += pHeader->mSymCount[XDLSegmentIx_Text];
                    if (segIx > XDLSegmentIx_Read)
                        pSym += pHeader->mSymCount[XDLSegmentIx_Read];
                    //
                    // at first symbol for segment, symbols are sorted
                    //
                    if (pSym->st_value <= aAddr)
                    {
                        symLeft--;
                        do {
                            pSym++;
                            if (pSym->st_value > aAddr)
                            {
                                --pSym;
                                break;
                            }
                        } while (--symLeft);
                        found = TRUE;

                        if (0 == pSym->st_name)
                            pSymSeg = sgpSegName[segIx];
                        else
                            pSymSeg += pSym->st_name;

                        K2ASC_PrintfLen(apRetNameBuffer, aBufferLen, "%.*s|%s+0x%X",
                            pHeader->mNameLen, pHeader->mName, pSymSeg,
                            aAddr - pSym->st_value);

                        found = TRUE;
                    }
                }
            }

            if (!found)
            {
                K2ASC_PrintfLen(apRetNameBuffer, aBufferLen, "%.*s|%s+0x%X",
                    pHeader->mNameLen, pHeader->mName, sgpSegName[segIx],
                    aAddr - (UINT_PTR)apXdl->SegAddrs.mLink[segIx]);
            }

            return K2STAT_NO_ERROR;
        }
    }

    return K2STAT_ERROR_NOT_FOUND;
}

