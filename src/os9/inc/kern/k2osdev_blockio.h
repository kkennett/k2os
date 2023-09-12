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
#ifndef __K2OSDRV_BLOCKIO_H
#define __K2OSDRV_BLOCKIO_H

#include <k2osddk.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

// {5D2DAF34-2B80-48DC-8ED5-DB4E1DDA4055}
#define K2OS_IFACE_BLOCKIO_DEVICE_CLASSID   { 0x5d2daf34, 0x2b80, 0x48dc, { 0x8e, 0xd5, 0xdb, 0x4e, 0x1d, 0xda, 0x40, 0x55 } }

//
//------------------------------------------------------------------------
//

#define K2OS_BLOCKIO_NOTIFY_MEDIA_CHANGE            1
// sent to handles with notify mailslots specified when media changes

#define K2OS_BLOCKIO_METHOD_GET_MEDIA               1
// input: nothing
// output: current K2OS_STORAGE_MEDIA structure



//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSDRV_BLOCKIO_H