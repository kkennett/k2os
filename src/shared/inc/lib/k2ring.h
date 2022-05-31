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
#ifndef __K2RING_H
#define __K2RING_H

#include <k2systype.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _K2RING_STATE_FIELDS K2RING_STATE_FIELDS;
struct _K2RING_STATE_FIELDS
{
    UINT32  mReadIx     : 15;
    UINT32  mSpare      : 1;
    UINT32  mWriteIx    : 15;
    UINT32  mHasGap     : 1;
};

typedef union _K2RING_STATE K2RING_STATE;
union _K2RING_STATE
{
    K2RING_STATE_FIELDS Fields;
    UINT32 volatile     mAsUINT32;
};

typedef struct _K2RING K2RING;
struct _K2RING
{
    K2RING_STATE    State;
    UINT32          mWriterGap;
    UINT32          mSize;
};

void   K2RING_Init(K2RING *apRing, UINT32 aSize);
UINT32 K2RING_Reader_GetAvail(K2RING *apRing, UINT32 *apRetOffset, BOOL aSetSpareOnEmpty);
K2STAT K2RING_Reader_Consumed(K2RING *apRing, UINT32 aCount);
BOOL   K2RING_Writer_GetOffset(K2RING *apRing, UINT32 aCount, UINT32 *apRetOffset);
K2STAT K2RING_Writer_Wrote(K2RING *apRing, UINT32 aWroteAtOffset, UINT32 aCount);
void   K2RING_ClearSpareBit(K2RING *apRing);

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __K2RING_H

