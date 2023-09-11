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

#include "kern.h"

K2STAT  
KernToken_Create(
    K2OSKERN_OBJ_HEADER *   apObjHdr,
    K2OS_TOKEN *            apRetToken
)
{
    BOOL                    disp;
    K2LIST_LINK *           pListLink;
    K2OSKERN_KERN_TOKEN *   pKernToken;

    K2_ASSERT(NULL != apObjHdr);
    K2_ASSERT(NULL != apRetToken);

    // these guys aren't allowed to have tokens.  use the 'user' variant
    K2_ASSERT(KernObj_Sem != apObjHdr->mObjType);

    disp = K2OSKERN_SeqLock(&gData.Token.SeqLock);

    if (0 != gData.Token.FreeList.mNodeCount)
    {
        pListLink = gData.Token.FreeList.mpHead;
        K2LIST_Remove(&gData.Token.FreeList, pListLink);
    }
    else
    {
        pListLink = NULL;
    }

    K2OSKERN_SeqUnlock(&gData.Token.SeqLock, disp);

    if (NULL != pListLink)
    {
        pKernToken = (K2OSKERN_KERN_TOKEN *)pListLink;
    }
    else
    {
        pKernToken = (K2OSKERN_KERN_TOKEN *)KernHeap_Alloc(sizeof(K2OSKERN_KERN_TOKEN));
        if (NULL == pKernToken)
            return K2STAT_ERROR_OUT_OF_MEMORY;
        K2MEM_Zero(pKernToken, sizeof(K2OSKERN_KERN_TOKEN));
    }

    pKernToken->mSentinel1 = KERN_TOKEN_SENTINEL1;
    pKernToken->Ref.AsAny = NULL;
    pKernToken->mSentinel2 = KERN_TOKEN_SENTINEL2;
    K2ATOMIC_Inc((INT32 volatile *)&apObjHdr->mTokenCount);
    KernObj_CreateRef(&pKernToken->Ref, apObjHdr);

    *apRetToken = (K2OS_TOKEN)pKernToken;

    return K2STAT_NO_ERROR;
}

K2STAT  
KernToken_Translate(
    K2OS_TOKEN          aToken,
    K2OSKERN_OBJREF *   apRef
)
{
    K2OSKERN_KERN_TOKEN * pKernToken;

    pKernToken = (K2OSKERN_KERN_TOKEN *)aToken;
    K2_ASSERT(KERN_TOKEN_SENTINEL1 == pKernToken->mSentinel1);
    K2_ASSERT(KERN_TOKEN_SENTINEL2 == pKernToken->mSentinel2);

    KernObj_CreateRef(apRef, pKernToken->Ref.AsAny);

    return K2STAT_NO_ERROR;
}

