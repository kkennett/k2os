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
#ifndef __K2XML_H
#define __K2XML_H

#include <k2systype.h>

#ifdef __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

typedef struct _K2XML_STR K2XML_STR;
struct _K2XML_STR 
{
    char const *    mpChars;
    UINT_PTR        mLen;
};

typedef struct _K2XML_ATTRIB K2XML_ATTRIB;
struct _K2XML_ATTRIB
{
    K2XML_STR   Name;
    K2XML_STR   Value;
};

typedef struct _K2XML_NODE K2XML_NODE;
struct _K2XML_NODE
{
    K2XML_STR       Name;
    UINT_PTR        mNameTagLen;

    K2XML_STR       Content;

    UINT_PTR        mNumAttrib;
    K2XML_ATTRIB ** mppAttrib;

    UINT_PTR        mNumSubNodes;
    K2XML_NODE **   mppSubNode;

    K2XML_STR       RawXml;
};

typedef void *  (*K2XML_pf_Allocator)(UINT_PTR aSizeBytes);
typedef void    (*K2XML_pf_Deallocator)(void *apPtr);

typedef struct _K2XML_PARSE K2XML_PARSE;
struct _K2XML_PARSE
{
    K2XML_pf_Deallocator    mfDeallocator;
    K2XML_NODE *            mpRootNode;
};

K2STAT
K2XML_Parse(
    K2XML_pf_Allocator      aAllocator,
    K2XML_pf_Deallocator    aDeallocator,
    char const *            apXml,
    UINT_PTR                aXmlLen,
    K2XML_PARSE *           apRetParse
);

K2STAT
K2XML_Purge(
    K2XML_PARSE *   apParse
);

BOOL
K2XML_FindAttrib(
    K2XML_NODE const *  apNode,
    char const *        apName,
    UINT_PTR *          apRetIx
);

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif // __K2XML_H
