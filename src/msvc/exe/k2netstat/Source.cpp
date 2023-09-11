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
#include <stdlib.h>
#include <lib/k2win32.h>
#include <lib/k2parse.h>

int main(int argc, char **argv)
{
    int result;
    char const *pOutput;
    UINT_PTR outLen;
    char const *pWord;
    UINT_PTR wordLen;
    bool found;
    bool checkLines;
    char const *pCheck;
    UINT_PTR checkLen;
    UINT_PTR ipLen;
    char *pConv;
    int portNum;
    char const *pSrc;
    UINT_PTR srcLen;
    
    result = RunProgram(NULL, "netstat -n", NULL, (char **)&pOutput, &outLen);
    if (0 != result)
        return result;

    found = false;
    checkLines = false;

    do {
        do {
            K2PARSE_EatWhitespace(&pOutput, &outLen);
            
            K2PARSE_EatWord(&pOutput, &outLen, &pWord, &wordLen);
            
            if (0 == wordLen)
                break;

            if (!checkLines)
            {
                if ((wordLen != 5) ||
                    (0 != (_strnicmp(pWord, "Proto", 5))))
                    break;
                // next line is where the list starts
                checkLines = true;
                break;
            }
//            printf("%.*s", wordLen, pWord);

            K2PARSE_EatWhitespace(&pOutput, &outLen);
            if (0 == outLen)
                break;
            K2PARSE_EatWord(&pOutput, &outLen, &pWord, &wordLen);
            if (0 == wordLen)
                break;
//            printf(" %.*s", wordLen, pWord);
            pCheck = pWord;
            checkLen = wordLen;

            K2PARSE_EatWhitespace(&pOutput, &outLen);
            if (0 == outLen)
                break;
            K2PARSE_EatWord(&pOutput, &outLen, &pWord, &wordLen);
            if (0 == wordLen)
                break;
//            printf(" %.*s", wordLen, pWord);
            pSrc = pWord;
            srcLen = wordLen;

            K2PARSE_EatWhitespace(&pOutput, &outLen);
            if (0 == outLen)
                break;
            K2PARSE_EatWord(&pOutput, &outLen, &pWord, &wordLen);

            if ((11 == wordLen) && (0 == _strnicmp(pWord, "ESTABLISHED", 11)))
            {
                K2PARSE_EatToChar(&pCheck, &checkLen, ':');

                if (1 < checkLen)
                {
                    pCheck++;
                    checkLen--;
                    pConv = new char[(checkLen + 4) & ~3];
                    CopyMemory(pConv, pCheck, checkLen);
                    pConv[checkLen = 0];
                    portNum = atoi(pConv);
                    delete[] pConv;
                    if (portNum == 3389)
                    {
                        found = true;

                        pWord = pSrc;
                        wordLen = srcLen;
                        K2PARSE_EatToChar(&pWord, &wordLen, ':');
                        ipLen = (UINT_PTR)(pWord - pSrc);
                        printf("%.*s\n", ipLen, pSrc);
                    }
                }
            }

        } while (false);

        if (found)
            break;

        K2PARSE_EatToEOL(&pOutput, &outLen);

        if (outLen > 0)
        {
            K2PARSE_EatEOL(&pOutput, &outLen);
        }

    } while ((0 != outLen) && (!found));

    if (!found)
        printf("%s\n", "NO");

    return 0;
}