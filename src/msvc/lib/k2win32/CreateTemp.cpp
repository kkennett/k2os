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

char *
MakeTempFileName(
    char const *    apExt,
    UINT_PTR *      apRetLen
)
{
    char *      pResult;
    UINT_PTR    extLen;
    char *      pEnd;
    char        pathBuffer[_MAX_PATH + 1];
    size_t      sLen;

    if ((NULL != apExt) && (*apExt == '.'))
        extLen = strlen(apExt);
    else
    {
        apExt = ".tmp";
        extLen = 4;
    }

    if (!GetTempPath(_MAX_PATH, pathBuffer))
        return NULL;

    if (!GetTempFileName(pathBuffer, "tmp~", 0, pathBuffer))
        return NULL;

    pathBuffer[_MAX_PATH] = 0;

    sLen = strlen(pathBuffer);

    pEnd = &pathBuffer[sLen];
    pEnd -= 4;
    sLen -= 4;
    strcpy_s(pEnd, _MAX_PATH - sLen, apExt);
    while (*pEnd)
        pEnd++;
    sLen = (UINT_PTR)(pEnd - pathBuffer);

    pResult = new char[sLen + 1];
    if (NULL == pResult)
        return NULL;

    CopyMemory(pResult, pathBuffer, sLen * sizeof(char));
    pResult[sLen] = 0;

    if (NULL != apRetLen)
        *apRetLen = sLen;

    return pResult;
}

