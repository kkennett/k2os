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

#include <lib/k2win32.h>
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <lib/k2parse.h>
#include <lib/k2elf.h>
#include <lib/k2tree.h>
#include <lib/k2crc.h>

typedef struct _OBJ_FILE OBJ_FILE;
struct _OBJ_FILE
{
    K2ReadOnlyMappedFile *      mpParentFile;
    char const *                mpObjName;
    UINT_PTR                    mObjNameLen;
    UINT_PTR                    mFileBytes;
    union {
        struct {
            Elf32_Ehdr const *          mpRawHdr;
            K2ELF32PARSE                Parse;
        } Bits32;
        struct {
            Elf64_Ehdr const *          mpRawHdr;
            K2ELF64PARSE                Parse;
        } Bits64;
    };
};

typedef struct _GLOBAL_SYMBOL GLOBAL_SYMBOL;
struct _GLOBAL_SYMBOL
{
    OBJ_FILE *          mpObjFile;
    char const *        mpSymName;
    bool                mIsCode;
    bool                mIsRead;
    bool                mIsWeak;
    K2TREE_NODE         TreeNode;
    union {
        struct {
            Elf32_Sym const *   mpSymEnt;
        } Bits32;
        struct {
            Elf64_Sym const *   mpSymEnt;
        } Bits64;
    };
};

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

    UINT_PTR        mIx;
    UINT_PTR        mExpSymNameOffset;
    UINT_PTR        mExpStrBytes;
    UINT_PTR        mRelocIx;
    char *          mpExpStrBase;

    XDL_EXPORTS_SEGMENT_HEADER * mpExpBase;

    union {
        struct {
            Elf32_Rel * mpRelocs;
        } Bits32;
        struct {
            Elf64_Rel * mpRelocs;
        } Bits64;
    };
};

#define SECIX_SEC_STR       1
#define SECIX_SYM_STR       2
#define SECIX_SYM           3
#define SECIX_ANCHOR        4
#define SECIX_ANCHOR_RELOC  5

typedef struct _OUTCTX OUTCTX;
struct _OUTCTX
{
    char const *            mpOutputFilePath;

    K2ReadWriteMappedFile * mpMappedXdlInf;

    K2_GUID128              Id;

    EXPSECT                 mOutSeg[XDLProgDataType_Count];

    UINT_PTR                mTotalExports;          // for all mOutSeg

    UINT_PTR                mFileClass;
    UINT_PTR                mElfMachine;

    K2TREE_ANCHOR           SymbolTree;

    UINT_PTR                mFileSizeBytes;
    UINT_PTR                mSectionCount;
    UINT_PTR                mSecStrTotalBytes;
    UINT_PTR                mSymStrTotalBytes;
    UINT_PTR                mIx;
    UINT_PTR                mExpSymNameOffset;
    UINT_PTR                mExpStrBytes;
    UINT_PTR                mRelocIx;
    UINT_PTR                mRelocType;
    UINT_PTR                mRawWork;
    UINT_PTR                mRawBase;
    char *                  mpSecStrBase;
    char *                  mpSecStrWork;
    char *                  mpSymStrBase;
    char *                  mpSymStrWork;

    XDL_ELF_ANCHOR          Anchor;
    XDL_ELF_ANCHOR *        mpAnchor;

    union {
        struct {
            Elf32_Ehdr *    mpFileHdr;
            Elf32_Rel *     mpSecRelocBase;
            Elf32_Rel *     mpSecRelocWork;
            Elf32_Shdr *    mpSecHdrs;
            Elf32_Sym *     mpSymWork;
            Elf32_Sym *     mpSymBase;
        } Bits32;
        struct {
            Elf64_Ehdr *    mpFileHdr;
            Elf64_Rel *     mpSecRelocBase;
            Elf64_Rel *     pSecRelocWork;
            Elf64_Shdr *    mpSecHdrs;
            Elf64_Sym *     mpSymWork;
            Elf64_Sym *     mpSymBase;
        } Bits64;
    };
};

extern OUTCTX gOut;

K2STAT LoadXdlInfFile(char const *apArgument);

K2STAT LoadInputFile(char const *apFilePath);

K2STAT AddOneObject(K2ReadOnlyMappedFile *apSrcFile, char const *apObjectFileName, UINT_PTR aObjectFileNameLen, UINT8 const *apFileData, UINT_PTR aFileDataBytes);

K2STAT DoExport(void);

#endif // __K2EXPORT_H
