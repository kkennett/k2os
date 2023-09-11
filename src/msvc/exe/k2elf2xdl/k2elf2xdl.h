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
#ifndef __K2ELF2XDL_H
#define __K2ELF2XDL_H

#include <lib/k2win32.h>
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <lib/k2parse.h>
#include <lib/k2elf.h>
#include <lib/k2tree.h>
#include <lib/k2crc.h>

typedef struct _OUTCTX OUTCTX;
struct _OUTCTX
{
    bool                                mSpecKernel;
    char const *                        mpOutputFilePath;
    INT_PTR                             mSpecStack;
    char const *                        mpImportLibFilePath;

    K2ReadOnlyMappedFile *              mpElfFile;
    UINT_PTR                            mElfAnchorSectionIx;
    XDL_ELF_ANCHOR const *              mpElfAnchor;
    XDL_EXPORTS_SEGMENT_HEADER const *  mpElfExpSegHdr[XDLProgDataType_Count];
    UINT_PTR                            mElfRoSecIx;

    UINT_PTR                            mTargetNameLen;
    char                                mTargetName[XDL_NAME_MAX_LEN + 1];

    bool                                mUsePlacement;
    UINT64                              mPlacement;
};

extern OUTCTX gOut;

int TreeStrCompare(UINT_PTR aKey, K2TREE_NODE * apNode);

K2STAT Convert64(void);

K2STAT          Convert32(void);

K2STAT          CreateOutputFile32(K2ELF32PARSE *apParse);

UINT8 const *   LoadAddrToDataPtr32(K2ELF32PARSE *apParse, UINT_PTR aLoadAddr, UINT_PTR *apRetSecIx);
UINT8 const *   LoadOffsetToDataPtr32(K2ELF32PARSE *apParse, UINT_PTR aLoadOffset, UINT_PTR *apRetSecIx);

K2STAT          CreateImportLibrary32(K2ELF32PARSE *apParse);

#endif // #ifndef __K2ELF2XDL_H

