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
#ifndef __K2EXPORT_H
#define __K2EXPORT_H

#include <k2systype.h>
#include <lib/k2win32.h>
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <lib/k2parse.h>
#include <lib/k2elf.h>
#include <lib/k2tree.h>

typedef struct _EXPORT_SPEC EXPORT_SPEC;
struct _EXPORT_SPEC
{
    char const *    mpName;
    EXPORT_SPEC *   mpNext;
    UINT_PTR        mExpNameOffset; // offset to copy of symbol name in export section
    UINT_PTR        mSymNameOffset; // offset to copy of symbol name in string table
};

typedef struct _EXPSECT EXPSECT;
struct _EXPSECT
{
    UINT_PTR        mCount;
    EXPORT_SPEC *   mpSpecList;
};

#define SECIX_SEC_STR       1
#define SECIX_SYM_STR       2
#define SECIX_SYM           3
#define SECIX_DLXINFO       4
#define SECIX_DLXINFO_RELOC 5

#define OUTSEC_CODE         0
#define OUTSEC_READ         1
#define OUTSEC_DATA         2
#define OUTSEC_COUNT        3

typedef struct _OUTCTX OUTCTX;
struct _OUTCTX
{
    char const *            mpOutputFilePath;

    K2ReadWriteMappedFile * mpMappedDlxInf;

    K2_GUID128              Id;

    EXPSECT                 mOutSec[OUTSEC_COUNT];

    UINT_PTR                mTotalExports;          // for all mOutSec
};

extern OUTCTX gOut;

K2STAT LoadDlxInfFile(char const *apArgument);

#endif // __K2EXPORT_H
