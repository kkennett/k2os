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
#include <lib/k2asc.h>

int          
K2ASC_CompLen(
    char const *    apStr1,
    char const *    apStr2,
    UINT_PTR        aMaxLen
    )
{
    int c;
    int ret;

    if ((aMaxLen == 0) || (apStr1==apStr2))
        return 0;

    do
    {
        c = (int)(*((unsigned char *)apStr1));

        ret = c - ((int)(*((unsigned char *)apStr2)));
        if (ret != 0)
            return ret;

        if ((c == 0) || (--aMaxLen == 0))
            return 0;

        apStr1++;
        apStr2++;

    } while (1);

    return ret;
}

int          
K2ASC_CompInsLen(
    char const *    apStr1,
    char const *    apStr2,
    UINT_PTR        aMaxLen
    )
{
    int c;
    int ret;

    if ((aMaxLen == 0) || (apStr1==apStr2))
        return 0;

    do
    {
        c = (int)((unsigned char)(K2ASC_ToUpper(*apStr1)));

        ret = c - ((int)((unsigned char)(K2ASC_ToUpper(*apStr2))));
        if (ret != 0)
            return ret;

        if ((c == 0) || (--aMaxLen == 0))
            return 0;

        apStr1++;
        apStr2++;

    } while (1);

    return ret;
}

