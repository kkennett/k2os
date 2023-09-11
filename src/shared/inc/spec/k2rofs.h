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
#ifndef __SPEC_K2ROFS_H
#define __SPEC_K2ROFS_H

#include <k2basetype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------- */

#define K2ROFS_SECTOR_BYTES 512

typedef struct _K2ROFS_FILE K2ROFS_FILE;
typedef struct _K2ROFS_DIR  K2ROFS_DIR;
typedef struct _K2ROFS      K2ROFS;

K2_PACKED_PUSH
struct _K2ROFS
{
    UINT32  mSectorCount;       // in entire ROFS
    UINT32  mRootDir;           // byte offset from K2ROFS to K2ROFS_DIR of root
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

K2_PACKED_PUSH
struct _K2ROFS_DIR
{
    UINT32  mName;              // offset in stringfield
    UINT32  mSubCount;          // number of subdirectories
    UINT32  mFileCount;         // number of files
    // K2ROFS_FILE * mFileCount
    // K2ROFS_DIR offsets from K2ROFS * mSubCount
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

K2_PACKED_PUSH
struct _K2ROFS_FILE
{
    UINT32  mName;              // offset in stringfield
    UINT32  mTime;              // dos datetime
    UINT32  mStartSectorOffset; // sector offset from ROFS
    UINT32  mSizeBytes;         // round up to 512 and divide to get sector count
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

/* ------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // __SPEC_K2ROFS_H

