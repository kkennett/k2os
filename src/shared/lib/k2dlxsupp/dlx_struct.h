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
#ifndef __DLX_STRUCT_H
#define __DLX_STRUCT_H

/* --------------------------------------------------------------------------------- */

#include <k2systype.h>

#include <lib/k2list.h>
#include <lib/k2elf.h>
#include <lib/k2tree.h>
#include <lib/k2dlxsupp.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _DLX
{
    K2LIST_LINK                 ListLink;  // must be first thing in node

    UINT32                      mLinkAddr;

    UINT32                      mRefs;

    UINT32                      mFlags;
    UINT32                      mEntrypoint;

    char const *                mpIntName;
    UINT32                      mIntNameLen;
    UINT32                      mIntNameFieldLen;

    void *                      mHostFile;
    UINT32                      mCurSector;
    UINT32                      mSectorCount;

    UINT32                      mHdrBytes;
    UINT32                      mRelocSectionCount;
    K2DLXSUPP_SEGALLOC          SegAlloc;
    Elf32_Ehdr *                mpElf;
    Elf32_Shdr *                mpSecHdr;

    DLX_INFO32 *                mpInfo;         // the mpExpXXX inside this gets updated during link

    DLX_EXPORTS32_SECTION *     mpExpCodeDataAddr;      // data addr of exports (Not link addr)
    DLX_EXPORTS32_SECTION *     mpExpReadDataAddr;
    DLX_EXPORTS32_SECTION *     mpExpDataDataAddr;

    K2TREE_ANCHOR               SymTree[3];
};

typedef struct _K2DLX_SYMTREE_NODE K2DLX_SYMTREE_NODE;
struct _K2DLX_SYMTREE_NODE
{
    K2TREE_NODE TreeNode;
    char *      mpSymName;
};

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif  // __DLX_STRUCT_H

