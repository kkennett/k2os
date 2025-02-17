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
#include <lib/k2json.h>

static void Spaces(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, UINT_PTR aCount)
{
    if (0 == aCount)
        return;
    do {
        afEmit(apParse, ' ');
    } while (--aCount);
}

static void EmitObj(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_OBJ *apObj, UINT_PTR aLevel, BOOL aIsLast);

static void EmitString(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_STR *apStr)
{
    UINT_PTR        left;
    char const *    pOut;

    left = apStr->mStrLen;
    afEmit(apParse, '\"');
    if (0 != left)
    {
        pOut = apStr->mpStr;
        do {
            afEmit(apParse, *pOut);
            pOut++;
        } while (--left);
    }
    afEmit(apParse, '\"');
}

static void EmitBool(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_BOOL *apBool)
{
    if (apBool->mBool)
    {
        afEmit(apParse, 't');
        afEmit(apParse, 'r');
        afEmit(apParse, 'u');
        afEmit(apParse, 'e');
    }
    else
    {
        afEmit(apParse, 'f');
        afEmit(apParse, 'a');
        afEmit(apParse, 'l');
        afEmit(apParse, 's');
        afEmit(apParse, 'e');
    }
}

static void EmitNum(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_NUM *apNum)
{
    UINT_PTR        left;
    char const *    pOut;

    left = apNum->Hdr.mRawLen;
    if (0 != left)
    {
        pOut = apNum->Hdr.mpRawChars;
        do {
            afEmit(apParse, *pOut);
            pOut++;
        } while (--left);
    }
}

static void EmitValue(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_PTR aPtr, UINT_PTR aLevel, BOOL aIsLast);

static void EmitArray(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_ARRAY *apArray, UINT_PTR aLevel, BOOL aIsLast)
{
    UINT_PTR    ix;
    K2JSON_Type vType;
    K2JSON_PTR  ptr;
    
    afEmit(apParse, '[');
    if (apArray->mElementCount > 0)
    {
        afEmit(apParse, '\n');
        for (ix = 0; ix < apArray->mElementCount - 1; ix++)
        {
            Spaces(apParse, afEmit, aLevel + 2);
            ptr = apArray->mpElements[ix];
            if (NULL != ptr.mpAsAny)
                vType = ptr.mpAsAny->mType;
            else
                vType = K2JSON_InvalidType;
            EmitValue(apParse, afEmit, apArray->mpElements[ix], aLevel + 2, FALSE);
            if ((vType != K2JSON_Obj) && (vType != K2JSON_Array))
            {
                afEmit(apParse, '\n');
            }
        }
        Spaces(apParse, afEmit, aLevel + 2);
        ptr = apArray->mpElements[ix];
        if (NULL != ptr.mpAsAny)
            vType = ptr.mpAsAny->mType;
        else
            vType = K2JSON_InvalidType;
        EmitValue(apParse, afEmit, apArray->mpElements[ix], aLevel + 2, TRUE);
        if ((vType != K2JSON_Obj) && (vType != K2JSON_Array))
        {
            afEmit(apParse, '\n');
        }
        Spaces(apParse, afEmit, aLevel);
    }
    else
    {
        afEmit(apParse, ' ');
    }
    afEmit(apParse, ']');
    if (!aIsLast)
    {
        afEmit(apParse, ',');
    }
    afEmit(apParse, '\n');
}

static void EmitValue(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_PTR aPtr, UINT_PTR aLevel, BOOL aIsLast)
{
    K2JSON_Type vType;

    if (NULL == aPtr.mpAsAny)
    {
        afEmit(apParse, 'n');
        afEmit(apParse, 'u');
        afEmit(apParse, 'l');
        afEmit(apParse, 'l');
        if (!aIsLast)
        {
            afEmit(apParse, ',');
        }
    }
    else
    {
        vType = aPtr.mpAsAny->mType;
        if ((vType == K2JSON_Array) || (vType == K2JSON_Obj))
        {
            if (vType == K2JSON_Array)
            {
                EmitArray(apParse, afEmit, aPtr.mpAsArray, aLevel + 2, aIsLast);
            }
            else
            {
                EmitObj(apParse, afEmit, aPtr.mpAsObj, aLevel + 2, aIsLast);
            }
        }
        else
        {
            if (vType == K2JSON_Str)
            {
                EmitString(apParse, afEmit, aPtr.mpAsStr);
            }
            else if (vType == K2JSON_Num)
            {
                EmitNum(apParse, afEmit, aPtr.mpAsNum);
            }
            else
            {
                K2_ASSERT(vType == K2JSON_Bool);
                EmitBool(apParse, afEmit, aPtr.mpAsBool);
            }
            if (!aIsLast)
            {
                afEmit(apParse, ',');
            }
        }
    }
}

static void EmitPair(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_PAIR *apPair, UINT_PTR aLevel, BOOL aIsLast)
{
    K2JSON_Type vType;

    EmitString(apParse, afEmit, &apPair->Name);
    afEmit(apParse, ' ');
    afEmit(apParse, ':');
    afEmit(apParse, ' ');
    if (NULL == apPair->Value.mpAsAny)
    {
        afEmit(apParse, 'n');
        afEmit(apParse, 'u');
        afEmit(apParse, 'l');
        afEmit(apParse, 'l');
        if (!aIsLast)
        {
            afEmit(apParse, ',');
        }
        afEmit(apParse, '\n');
    }
    else
    {
        vType = apPair->Value.mpAsAny->mType;
        EmitValue(apParse, afEmit, apPair->Value, aLevel, aIsLast);
        if ((vType != K2JSON_Obj) && (vType != K2JSON_Array))
        {
            afEmit(apParse, '\n');
        }
    }
}

static void EmitObj(K2JSON_PARSE *apParse, K2JSON_pf_Emitter afEmit, K2JSON_OBJ *apObj, UINT_PTR aLevel, BOOL aIsLast)
{
    UINT_PTR ix;

    afEmit(apParse, '{');
    if (apObj->mPairCount > 0)
    {
        afEmit(apParse, '\n');
        for (ix = 0; ix < apObj->mPairCount - 1; ix++)
        {
            Spaces(apParse, afEmit, aLevel + 2);
            EmitPair(apParse, afEmit, &apObj->mpPairs[ix], aLevel + 2, FALSE);
        }
        Spaces(apParse, afEmit, aLevel + 2);
        EmitPair(apParse, afEmit, &apObj->mpPairs[ix], aLevel + 2, TRUE);
        Spaces(apParse, afEmit, aLevel);
    }
    else
    {
        afEmit(apParse, ' ');
    }
    afEmit(apParse, '}');
    if (!aIsLast)
    {
        afEmit(apParse, ',');
    }
    afEmit(apParse, '\n');
}

void
K2JSON_Emit(
    K2JSON_PARSE *      apParse,
    K2JSON_pf_Emitter   afEmitter
)
{
    EmitObj(apParse, afEmitter, &apParse->RootObj, 0, TRUE);
}

