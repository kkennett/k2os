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
#include <lib/k2list.h>

void 
K2LIST_Remove(
    K2LIST_ANCHOR * apAnchor, 
    K2LIST_LINK *   apNode
    )
{
    K2LIST_LINK *pOther;

    K2_ASSERT(apAnchor != NULL);
    K2_ASSERT(apNode != NULL);
    K2_ASSERT(apAnchor->mNodeCount > 0);

    K2_ASSERT(((apAnchor->mpHead == NULL) && (apAnchor->mpTail == NULL)) || ((apAnchor->mpHead != NULL) && (apAnchor->mpTail != NULL)));

    pOther = apNode->mpPrev;
    if (pOther != NULL)
    {
        K2_ASSERT(apNode != apAnchor->mpHead);
        pOther->mpNext = apNode->mpNext;
    }
    else
    {
        K2_ASSERT(apNode == apAnchor->mpHead);
        apAnchor->mpHead = apNode->mpNext;
    }

    pOther = apNode->mpNext;
    if (pOther != NULL)
    {
        K2_ASSERT(apNode != apAnchor->mpTail);
        pOther->mpPrev = apNode->mpPrev;
    }
    else
    {
        K2_ASSERT(apNode == apAnchor->mpTail);
        apAnchor->mpTail = apNode->mpPrev;
    }

    K2_ASSERT(((apAnchor->mpHead == NULL) && (apAnchor->mpTail == NULL)) || ((apAnchor->mpHead != NULL) && (apAnchor->mpTail != NULL)));

    apNode->mpNext = NULL;
    apNode->mpPrev = NULL;

    apAnchor->mNodeCount--;
}
