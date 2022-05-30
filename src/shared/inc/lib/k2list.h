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
#ifndef __K2LIST_H
#define __K2LIST_H

#include <k2systype.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _K2LIST_LINK K2LIST_LINK;
struct _K2LIST_LINK
{
    K2LIST_LINK *   mpPrev;
    K2LIST_LINK *   mpNext;
};

typedef struct _K2LIST_ANCHOR K2LIST_ANCHOR;
struct _K2LIST_ANCHOR
{
    K2LIST_LINK *   mpHead;
    K2LIST_LINK *   mpTail;
    UINT32          mNodeCount;
};

void 
K2LIST_Init(
    K2LIST_ANCHOR * apAnchor
);

void 
K2LIST_AddAtHead(
    K2LIST_ANCHOR * apAnchor,
    K2LIST_LINK *   apNode
);

void 
K2LIST_AddAtTail(
    K2LIST_ANCHOR * apAnchor,
    K2LIST_LINK *   apNode
);

void 
K2LIST_AddBefore(
    K2LIST_ANCHOR * apAnchor,
    K2LIST_LINK *   apNode,
    K2LIST_LINK *   apAddBefore
);

void 
K2LIST_AddAfter(
    K2LIST_ANCHOR * apAnchor,
    K2LIST_LINK *   apNode,
    K2LIST_LINK *   apAddAfter
);

void 
K2LIST_Remove(
    K2LIST_ANCHOR * apAnchor,
    K2LIST_LINK *   apNode
);

void
K2LIST_AppendToTail(
    K2LIST_ANCHOR * apAnchor,
    K2LIST_ANCHOR * apMerge
);

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __K2LIST_H

