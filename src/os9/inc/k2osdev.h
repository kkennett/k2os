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
#ifndef __K2OSDEV_H
#define __K2OSDEV_H

#include <k2os.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

#define K2OS_STORAGE_MEDIA_FRIENDLY_BUFFER_CHARS    32

#define K2OS_STORAGE_MEDIA_ATTRIB_READ_ONLY         0x00000001

typedef struct _K2OS_STORAGE_MEDIA K2OS_STORAGE_MEDIA;
struct _K2OS_STORAGE_MEDIA
{
    UINT64  mUniqueId;
    UINT64  mBlockCount;
    UINT64  mTotalBytes;
    UINT32  mBlockSizeBytes;
    UINT32  mAttrib;
    char    mFriendly[K2OS_STORAGE_MEDIA_FRIENDLY_BUFFER_CHARS];
};

#define K2OS_STORAGE_VOLUME_ATTRIB_READ_ONLY    1
#define K2OS_STORAGE_VOLUME_ATTRIB_BOOT         2   // boot volume containing OS

typedef struct _K2OS_STORAGE_VOLUME K2OS_STORAGE_VOLUME;
struct _K2OS_STORAGE_VOLUME
{
    K2_GUID128  mUniqueId;
    UINT64      mBlockCount;
    UINT64      mTotalBytes;
    UINT64      mAttributes;
    UINT32      mBlockSizeBytes;
    UINT32      mPartitionCount;
};

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSDEV_H