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
#ifndef __K2JSON_H
#define __K2JSON_H

#include <k2systype.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _K2JSON_PARSE    K2JSON_PARSE;
typedef struct _K2JSON_PAIR     K2JSON_PAIR;
typedef struct _K2JSON_OBJ      K2JSON_OBJ;
typedef struct _K2JSON_STR      K2JSON_STR;
typedef struct _K2JSON_NUM      K2JSON_NUM;
typedef struct _K2JSON_ARRAY    K2JSON_ARRAY;
typedef struct _K2JSON_BOOL     K2JSON_BOOL;

typedef void * (*K2JSON_pf_MemAlloc)(UINT_PTR aBytes);
typedef void   (*K2JSON_pf_MemFree)(void *aPtr);
typedef void   (*K2JSON_pf_Emitter)(K2JSON_PARSE *apParse, char ch);

typedef enum _K2JSON_Type K2JSON_Type;
enum _K2JSON_Type
{
    K2JSON_InvalidType = 0,
    K2JSON_Obj,
    K2JSON_Str,
    K2JSON_Num,
    K2JSON_Array,
    K2JSON_Bool
};

typedef struct _K2JSON_HDR K2JSON_HDR;
struct _K2JSON_HDR
{
    K2JSON_Type     mType;
    UINT32          mUser;
    UINT_PTR        mRawLen;
    UINT8 const *   mpRawChars;
};

typedef union _K2JSON_PTR K2JSON_PTR;
union _K2JSON_PTR
{
    K2JSON_HDR *    mpAsAny;
    K2JSON_OBJ *    mpAsObj;
    K2JSON_STR *    mpAsStr;
    K2JSON_NUM *    mpAsNum;
    K2JSON_ARRAY *  mpAsArray;
    K2JSON_BOOL *   mpAsBool;
};

struct _K2JSON_BOOL
{
    K2JSON_HDR  Hdr;
    BOOL        mBool;
};

struct _K2JSON_STR
{
    K2JSON_HDR      Hdr;
    UINT_PTR        mStrLen;
    char const *    mpStr;
};

struct _K2JSON_PAIR
{
    K2JSON_STR  Name;
    K2JSON_PTR  Value;
};

struct _K2JSON_OBJ
{
    K2JSON_HDR      Hdr;
    UINT_PTR        mPairCount;
    K2JSON_PAIR *   mpPairs;
};

struct _K2JSON_NUM
{
    K2JSON_HDR  Hdr;
    BOOL        mIsNeg;
    BOOL        mIsFrac;
    BOOL        mHasExp;
    BOOL        mExpIsNeg;
};

struct _K2JSON_ARRAY
{
    K2JSON_HDR      Hdr;
    UINT_PTR        mElementCount;
    K2JSON_PTR *    mpElements;
};

struct _K2JSON_PARSE
{
    K2JSON_pf_MemAlloc  mfAlloc;
    K2JSON_pf_MemFree   mfFree;

    char const *        mpErrorPoint;
    K2JSON_OBJ          RootObj;
};

K2STAT
K2JSON_Parse(
    K2JSON_pf_MemAlloc  afAlloc,
    K2JSON_pf_MemFree   afFree,
    char const *        apData,
    UINT_PTR            aMaxDataLen,
    K2JSON_PARSE *      apRetParse
);

void
K2JSON_Purge(
    K2JSON_pf_MemFree   afFree,
    K2JSON_PTR          aPtr
);

void
K2JSON_Emit(
    K2JSON_PARSE *      apParse,
    K2JSON_pf_Emitter   afEmitter
);

void
K2JSON_EatWhitespace(
    char const **   appPars,
    UINT_PTR *      apLeft
);

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __K2LIST_H

