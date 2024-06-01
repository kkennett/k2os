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
#ifndef __SPEC_ETHER_H
#define __SPEC_ETHER_H

//
// --------------------------------------------------------------------------------- 
//

#include <k2basetype.h>
#include <spec/etherdef.inc>

#ifdef __cplusplus
extern "C" {
#endif

K2_PACKED_PUSH
struct _ETHER_FRAME_HDR
{
    UINT8  mDestMAC[ETHER_FRAME_MAC_LENGTH];
    UINT8  mSrcMAC[ETHER_FRAME_MAC_LENGTH];
    UINT16 mTypeOrLength;   // >= 1536 means type
    // Data
    // 32-bit FCS (16-bit aligned)
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ETHER_FRAME_HDR ETHER_FRAME_HDR;
K2_STATIC_ASSERT(ETHER_FRAME_HDR_LENGTH == sizeof(ETHER_FRAME_HDR));

K2_PACKED_PUSH
struct _ETHER_FRAME_FULL
{
    ETHER_FRAME_HDR Hdr;
    UINT8           mData[ETHER_FRAME_MTU];
    UINT8           mFCS[ETHER_FRAME_FCS_LENGTH];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ETHER_FRAME_FULL ETHER_FRAME_FULL;
K2_STATIC_ASSERT(ETHER_FRAME_FULL_LENGTH == sizeof(ETHER_FRAME_FULL));

#ifdef __cplusplus
};  // extern "C"
#endif

/* ------------------------------------------------------------------------- */

#endif  // __SPEC_ETHER_H

