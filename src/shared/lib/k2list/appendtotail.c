//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
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
K2LIST_AppendToTail(
    K2LIST_ANCHOR * apAnchor,
    K2LIST_ANCHOR * apMerge
)
{
    K2_ASSERT(apAnchor != NULL);
    K2_ASSERT(apMerge != NULL);

    if (apAnchor->mpTail == NULL)
    {
        //
        // apAnchor was empty, so just copy apMerge
        //
        K2_ASSERT(apAnchor->mNodeCount == 0);
        apAnchor->mpHead = apMerge->mpHead;
        apAnchor->mpTail = apMerge->mpTail;
        apAnchor->mNodeCount = apMerge->mNodeCount;
    }
    else
    {
        K2_ASSERT(apAnchor->mNodeCount > 0);
        if (apMerge->mpHead != NULL)
        {
            K2_ASSERT(apMerge->mNodeCount > 0);
            apMerge->mpHead->mpPrev = apAnchor->mpTail;
            apAnchor->mpTail->mpNext = apMerge->mpHead;
            apAnchor->mpTail = apMerge->mpTail;
            apAnchor->mNodeCount += apMerge->mNodeCount;
        }
        else
        {
            K2_ASSERT(apMerge->mNodeCount == 0);
        }
    }

    apMerge->mpHead = NULL;
    apMerge->mpTail = NULL;
    apMerge->mNodeCount = 0;
}

