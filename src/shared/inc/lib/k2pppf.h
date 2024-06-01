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

#ifndef __K2PPPF_H
#define __K2PPPF_H

/* --------------------------------------------------------------------------------- */

#include <k2basetype.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PPPF_ADDR    0xFF
#define PPPF_CONTROL 3
#define PPPF_FLAG    0x7E
#define PPPF_ESCAPE  0x7D

typedef struct _K2PPPF K2PPPF;

//
// called when a full frame is received (callback from the call to KPPPF.mfDataIn() )
// you can leave the *appFrame the same to re-use the buffer, or change it to put a new
// buffer for receiving in
//
typedef void (*K2PPPF_pf_FrameIn)(K2PPPF *apStream, UINT8 **appFrame, UINT32 aByteCount);

//
// called when a byte is to be sent (callback from the call to KPPPF_FrameOut() )
//
typedef void (*K2PPPF_pf_DataOut)(K2PPPF *apStream, UINT8 aByteOut);

//
// user calls this when data comes in, byte for byte
// when a full valid frame arrives, the FrameIn() is called
//
typedef void (*K2PPPF_pf_DataIn)(K2PPPF *apStream, UINT8 aInByte);

struct _K2PPPF
{
    BOOL                mInEsc;
    UINT8               mBack2;
    UINT8               mBack1;
    UINT16              mProtocol;
    UINT16              mRunCrc;
    UINT8 *             mpFrame;
    UINT32              mMaxFrameBytes;
    UINT8 *             mpOut;
    UINT32              mLen;
    K2PPPF_pf_DataIn    mfDataIn;
    K2PPPF_pf_FrameIn   mfFrameIn;
    K2PPPF_pf_DataOut   mfDataOut;
    UINT32              mStat_GarbageCharCount;
    UINT32              mStat_NoFcsOnEmpty;
    UINT32              mStat_FirstEscInvalid;
    UINT32              mStat_OneByteFcs;
    UINT32              mStat_SecondEscInvalid;
    UINT32              mStat_BadEsc;
    UINT32              mStat_OverflowFrames;
    UINT32              mStat_FcsBad;
};

K2STAT
K2PPPF_Init(
    K2PPPF *            apStream,
    UINT8 *             apFrame,        // Initial pointer for memory to receive a frame
    K2PPPF_pf_FrameIn   afFrameIn,      // K2PPPF will call this when a full valud PPP frame has been received
    K2PPPF_pf_DataOut   afDataOut       // K2PPPF will call this to output bytes when a K2PPPF_FrameOut() is called
);

K2STAT
K2PPPF_FrameOut(
    K2PPPF *        apStream,
    UINT8 const *   apData,         // first two bytes are network order of 16-bit PPP protocol id
    UINT32          aDataLen
);

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif // __K2PPPF_H
