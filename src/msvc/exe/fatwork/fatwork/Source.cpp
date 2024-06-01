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

#include <lib/k2win32.h>
#include <lib/k2dbus.h>

static
void
myAssert(
    char const *    apFile,
    int             aLineNum,
    char const *    apCondition
)
{
    DebugBreak();
}

extern "C" K2_pf_ASSERT K2_Assert = myAssert;

UINT8 const sTest[] =
{
0x6c, 0x01, 0x00, 0x01, 0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
0x01, 0x01, 0x6f, 0x00, 0x15, 0x00, 0x00, 0x00, 0x2f, 0x72, 0x65, 0x2f, 0x66, 0x72, 0x69, 0x64,
0x61, 0x2f, 0x48, 0x6f, 0x73, 0x74, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00,
0x02, 0x01, 0x73, 0x00, 0x16, 0x00, 0x00, 0x00, 0x72, 0x65, 0x2e, 0x66, 0x72, 0x69, 0x64, 0x61,
0x2e, 0x48, 0x6f, 0x73, 0x74, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x31, 0x35, 0x00, 0x00,
0x08, 0x01, 0x67, 0x00, 0x05, 0x61, 0x7b, 0x73, 0x76, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x03, 0x01, 0x73, 0x00, 0x17, 0x00, 0x00, 0x00, 0x47, 0x65, 0x74, 0x46, 0x72, 0x6f, 0x6e, 0x74,
0x6d, 0x6f, 0x73, 0x74, 0x41, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x00,

0x14, 0x00, 0x00, 0x00, // array of {sv} length

0,0,0,0,                 // align for dictent

0x03, 0x00, 0x00, 0x00, // string
'b','o','o', 0,

0x01,   // variant signature length
'u',    // variant is a UINT32 
0,      // variant signature null terminator
0,      // padding to hit 4 byte alignment for uint32
0x01, 0x02, 0x03, 0x04  // unit32 corresponding to dict ent "boo"


};

int main(int argc, char **argv)
{
    K2STAT stat;
    K2DBUS_PARSED_HDR parsed;
    UINT8 const *pData = sTest;
    UINT32 len = sizeof(sTest);
    K2DBUS_PARSE bodyParse;

    UINT32 sigLen;
    stat = K2DBUS_SigIsWellFormed("yyyyuua(((yv))a{us})", 120, &sigLen);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    stat = K2DBUS_EatHeader(&pData, &len, &parsed);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (len < parsed.mpRaw->mBodyByteCount)
        return K2STAT_ERROR_BAD_FORMAT;

    bodyParse.mpSig = parsed.mpSignature;
    bodyParse.mSigLen = parsed.mSignatureLen;
    bodyParse.mMemAddr = (UINT32)pData;
    bodyParse.mMemLeft = parsed.mpRaw->mBodyByteCount;

    stat = K2DBUS_Validate(&bodyParse);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    return stat;
}
