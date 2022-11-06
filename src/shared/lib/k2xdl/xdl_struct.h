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

#ifndef __XDL_STRUCT_H
#define __XDL_STRUCT_H

#include <lib/k2xdl.h>

#include <lib/k2list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XDL_FLAG_FULLY_LOADED     1
#define XDL_FLAG_PERMANENT        2
#define XDL_FLAG_ENTRY_CALLED     4
#define XDL_FLAG_KEEP_SYMBOLS     8

struct _XDL
{
    K2LIST_LINK                     ListLink;

    INT32                           mRefs;

    UINT32                          mFlags;

    XDL_FILE_HEADER *               mpHeader;
    
    XDL_IMPORT *                    mpImports;

    K2XDL_LOADCTX *                 mpLoadCtx;

    K2XDL_SEGMENT_ADDRS             SegAddrs;

    XDL_EXPORTS_SEGMENT_HEADER *    mpExpHdr[XDLProgDataType_Count];
};

#ifdef __cplusplus
};  // extern "C"
#endif

#endif // __XDL_STRUCT_H
