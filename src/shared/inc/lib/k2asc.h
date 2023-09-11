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
#ifndef __K2ASC_H
#define __K2ASC_H

#include <k2systype.h>
#include <spec/ascii.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*K2ASC_Emitter_FnPtr)(void * apContext, char aCh);

char *          K2ASC_FindCharLen(char aCh, char * apStr, UINT_PTR aMaxLen);
char *          K2ASC_FindCharInsLen(char aCh, char * apStr, UINT_PTR aMaxLen);
char *          K2ASC_FindAnyCharLen(char const * apChk, char * apStr, UINT_PTR aMaxLen);
char *          K2ASC_FindAnyCharInsLen(char const * apChk, char * apStr, UINT_PTR aMaxLen);
UINT_PTR        K2ASC_Len(char const * apStr);
UINT_PTR        K2ASC_CopyLen(char * apDest, char const * apSrc, UINT_PTR aMaxLen);
UINT_PTR        K2ASC_ConcatLen(char * apStr, char const * apConcat, UINT_PTR aMaxLen);
int             K2ASC_CompLen(char const * apStr1, char const * apStr2, UINT_PTR aMaxLen);
int             K2ASC_CompInsLen(char const * apStr1, char const * apStr2, UINT_PTR aMaxLen);
char            K2ASC_ToUpper(char aCh);
char            K2ASC_ToLower(char aCh);
K2_NUMTYPE      K2ASC_NumTypeLen(char const * apStr, UINT_PTR aStrLen);
INT32           K2ASC_NumValue32Len(char const * apStr, UINT_PTR aStrLen);
UINT_PTR        K2ASC_Emitf(K2ASC_Emitter_FnPtr afEmitter, void * apContext, UINT_PTR aMaxLen, char const * apFormat, VALIST aVarList);
UINT_PTR        K2ASC_Printf(char * apOutput, char const * apFormat, ...);
UINT_PTR        K2ASC_PrintfLen(char * apOutput, UINT_PTR aMaxLen, char const * apFormat, ...);
UINT_PTR        K2ASC_PrintfVarLen(char * apOutput, UINT_PTR aMaxLen, char const * apFormat, VALIST aVarList);
char const *    K2ASC_FindCharConstLen(char aCh, char const * apStr, UINT_PTR aMaxLen);
char const *    K2ASC_FindCharConstInsLen(char aCh, char const * apStr, UINT_PTR aMaxLen);
char const *    K2ASC_FindAnyCharConstLen(char const * apChk, char const * apStr, UINT_PTR aMaxLen);
char const *    K2ASC_FindAnyCharConstInsLen(char const * apChk, char const * apStr, UINT_PTR aMaxLen);


/* --------------------------------------------------------------------------------- */

static K2_INLINE
char * 
K2ASC_FindChar(
    char    aCh,
    char *  apStr
    )
{
    return K2ASC_FindCharLen(aCh, apStr, (UINT_PTR)-1);
}

static K2_INLINE
char * 
K2ASC_FindCharIns(
    char    aCh,
    char *  apStr
    )
{
    return K2ASC_FindCharInsLen(aCh, apStr, (UINT_PTR)-1);
}

static K2_INLINE
char *
K2ASC_FindAnyChar(
    char const *    apChk,
    char *          apStr
    )
{
    return K2ASC_FindAnyCharLen(apChk, apStr, (UINT_PTR)-1);
}

static K2_INLINE
char *
K2ASC_FindAnyCharIns(
    char const *    apChk,
    char *          apStr
    )
{
    return K2ASC_FindAnyCharInsLen(apChk, apStr, (UINT_PTR)-1);
}

static K2_INLINE
UINT_PTR        
K2ASC_Copy(
    char *          apDest,
    char const *    apSrc
    )
{
    return K2ASC_CopyLen(apDest, apSrc, (UINT_PTR)-1);
}

static K2_INLINE
UINT_PTR        
K2ASC_Concat(
    char *          apStr,
    char const *    apConcat
    )
{
    return K2ASC_ConcatLen(apStr, apConcat, (UINT_PTR)-1);
}

static K2_INLINE
int          
K2ASC_Comp(
    char const *    apStr1,
    char const *    apStr2
    )
{
    return K2ASC_CompLen(apStr1, apStr2, (UINT_PTR)-1);
}

static K2_INLINE
int          
K2ASC_CompIns(
    char const *    apStr1,
    char const *    apStr2
    )
{
    return K2ASC_CompInsLen(apStr1, apStr2, (UINT_PTR)-1);
}

static K2_INLINE
K2_NUMTYPE
K2ASC_NumType(
    char const *    apStr
    )
{
    return K2ASC_NumTypeLen(apStr, K2ASC_Len(apStr));
}

static K2_INLINE
INT32        
K2ASC_NumValue32(
    char const *    apStr
    )
{
    return K2ASC_NumValue32Len(apStr, K2ASC_Len(apStr));
}

static K2_INLINE
UINT_PTR
K2ASC_PrintfVar(
    char *          apOutput,
    char const *    apFormat,
    VALIST          aVarList
    )
{
    return K2ASC_PrintfVarLen(apOutput, (UINT_PTR)-1, apFormat, aVarList);
}

static K2_INLINE
char const * 
K2ASC_FindCharConst(
    char            aCh,
    char const *    apStr
    )
{
    return K2ASC_FindCharConstLen(aCh, apStr, (UINT_PTR)-1);
}

static K2_INLINE
char const * 
K2ASC_FindCharConstIns(
    char            aCh,
    char const *    apStr
    )
{
    return K2ASC_FindCharConstInsLen(aCh, apStr, (UINT_PTR)-1);
}

static K2_INLINE
char const *
K2ASC_FindAnyCharConst(
    char const *    apChk,
    char const *    apStr
    )
{
    return K2ASC_FindAnyCharConstLen(apChk, apStr, (UINT_PTR)-1);
}

static K2_INLINE
char const *
K2ASC_FindAnyCharConstIns(
    char const *    apChk,
    char const *    apStr
    )
{
    return K2ASC_FindAnyCharConstInsLen(apChk, apStr, (UINT_PTR)-1);
}

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif // __K2ASC_H
