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
#include "crtuser.h"

UINT32
K2OS_Debug_OutputString(
    char const *apStr
)
{
    K2OS_THREAD_PAGE *  pThreadPage;
    char *              pOut;
    char                ch;
    UINT32              left;

    if (NULL == apStr)
        return 0;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    left = K2OS_THREAD_PAGE_BUFFER_BYTES - 1;
    pOut = (char *)pThreadPage->mMiscBuffer;
    --apStr;
    do
    {
        ++apStr;
        ch = *apStr;
        if (ch != 0)
        {
            if (((ch < ' ') && (ch != '\n') && (ch != '\r')) ||
                (ch > 127))
                ch = '.';
        }
        *pOut = ch;
        if (0 == ch)
            break;
        pOut++;
    } while (--left);
    if (0 == left)
        *pOut = 0;

    return CrtKern_SysCall1(K2OS_SYSCALL_ID_OUTPUT_DEBUG, 0);
}

void
K2OS_Debug_Break(
    void
)
{
    CrtKern_SysCall1(K2OS_SYSCALL_ID_DEBUG_BREAK, 0);
}

UINT32 
CrtDbg_Printf(
    char const *apFormat, 
    ...
)
{
    char    outBuf[128];
    VALIST  vaList;
    UINT32  result;
    
    K2_VASTART(vaList, apFormat);
    result = K2ASC_PrintfVarLen(outBuf, 127, apFormat, vaList);
    K2_VAEND(vaList);
    outBuf[95] = 0;
    K2OS_Debug_OutputString(outBuf);
    return result;
}
