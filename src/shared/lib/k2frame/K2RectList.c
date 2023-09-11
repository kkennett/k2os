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

#include <lib/k2frame.h>

K2RECTENT * 
K2RECTENTI_Alloc(
    K2RECTENT_CACHE *   apCache,
    K2RECT const *      apInit
)
{
    K2RECTENT *pRet;

    K2_ASSERT(NULL != apCache);

    pRet = apCache->Get(apCache);
    K2_ASSERT(NULL != pRet);

    if (NULL != apInit)
    {
        K2_ASSERT(K2RECT_IsValid(apInit));

        pRet->mpNext = pRet->mpPrev = NULL;
        K2MEM_Copy(&pRet->Rect, apInit, sizeof(K2RECT));
    }
    else
    {
        K2MEM_Zero(pRet, sizeof(K2RECTENT));
    }

    return pRet;
}

void 
K2RECTENTI_Free(
    K2RECTENT_CACHE *   apCache,
    K2RECTENT *         pEnt
)
{
    K2_ASSERT(NULL != apCache);
    K2_ASSERT(NULL != pEnt);
    apCache->Put(apCache, pEnt);
}

void 
K2RECTLIST_Init(
    K2RECTLIST *        apRectList,
    K2RECTENT_CACHE *   apCache
    )
{
    K2_ASSERT(NULL != apCache);
    K2_ASSERT(NULL != apRectList);
    K2MEM_Zero(apRectList, sizeof(K2RECTLIST));
    apRectList->mpCache = apCache;
}

void 
K2RECTENTI_AddEnt(
    K2RECTLIST * apRectList, 
    K2RECTENT *  apEnt
)
{
    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    if (0 == apRectList->mCount)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(NULL == apRectList->mpTail);
    }
    else
    {
        K2_ASSERT(NULL != apRectList->mpHead);
        K2_ASSERT(NULL != apRectList->mpTail);
    }

    K2_ASSERT(NULL != apEnt);

    apEnt->mpNext = NULL;

    if (NULL == apRectList->mpTail)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(0 == apRectList->mCount);
        apEnt->mpPrev = NULL;
        apRectList->mpHead = apEnt;
        apRectList->mpTail = apEnt;
        apRectList->mCount = 1;
    }
    else
    {
        apEnt->mpPrev = apRectList->mpTail;
        apRectList->mpTail->mpNext = apEnt;
        apRectList->mpTail = apEnt;
        apRectList->mCount++;
    }
}

void
K2RECTLISTI_RemoveEnt(
    K2RECTLIST * apRectList,
    K2RECTENT *  apEnt
)
{
    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    K2_ASSERT(0 != apRectList->mCount);
    K2_ASSERT(NULL != apRectList->mpHead);
    K2_ASSERT(NULL != apRectList->mpTail);

    K2_ASSERT(NULL != apEnt);

    if (apEnt->mpPrev != NULL)
    {
        apEnt->mpPrev->mpNext = apEnt->mpNext;
    }
    else
    {
        K2_ASSERT(apEnt == apRectList->mpHead);
        apRectList->mpHead = apEnt->mpNext;
        if (NULL != apRectList->mpHead)
            apRectList->mpHead->mpPrev = NULL;
    }

    if (apEnt->mpNext != NULL)
    {
        apEnt->mpNext->mpPrev = apEnt->mpPrev;
    }
    else
    {
        K2_ASSERT(apEnt == apRectList->mpTail);
        apRectList->mpTail = apEnt->mpPrev;
        if (NULL != apRectList->mpTail)
            apRectList->mpTail->mpNext = NULL;
    }

    --apRectList->mCount;
}

void
K2RECTLIST_Purge(
    K2RECTLIST *apRectList
)
{
    K2RECTENT * pPurge;
    K2RECTENT * pEnt;

    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    if (0 == apRectList->mCount)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(NULL == apRectList->mpTail);
    }
    else
    {
        K2_ASSERT(NULL != apRectList->mpHead);
        K2_ASSERT(NULL != apRectList->mpTail);

        pEnt = apRectList->mpHead;
        do {
            pPurge = pEnt;
            pEnt = pEnt->mpNext;
            K2RECTENTI_Free(apRectList->mpCache, pPurge);
        } while (NULL != pEnt);

        K2RECTLIST_Init(apRectList, apRectList->mpCache);
    }
}

void
K2RECTLIST_Copy(
    K2RECTLIST const *  apRectList,
    int                 aOffsetX,
    int                 aOffsetY,
    K2RECTLIST *        apRetCopy
)
{
    K2RECTENT * pCopy;
    K2RECTENT * pEnt;

    K2_ASSERT(NULL != apRetCopy);
    K2_ASSERT(apRetCopy->mCount == 0);
    K2_ASSERT(apRetCopy->mpCache != NULL);
    K2_ASSERT(apRetCopy->mpHead == NULL);
    K2_ASSERT(apRetCopy->mpTail == NULL);

    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    if (0 == apRectList->mCount)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(NULL == apRectList->mpTail);
    }
    else
    {
        K2_ASSERT(NULL != apRectList->mpHead);
        K2_ASSERT(NULL != apRectList->mpTail);

        pEnt = apRectList->mpHead;
        do {
            pCopy = K2RECTENTI_Alloc(apRetCopy->mpCache, &pEnt->Rect);
            K2_ASSERT(NULL != pCopy);
            K2RECT_Offset(&pCopy->Rect, aOffsetX, aOffsetY);
            pCopy->mpNext = NULL;

            if (NULL == apRetCopy->mpTail)
            {
                pCopy->mpPrev = NULL;
                apRetCopy->mpHead = apRetCopy->mpTail = pCopy;
            }
            else
            {
                pCopy->mpPrev = apRetCopy->mpTail;
                apRetCopy->mpTail->mpNext = pCopy;
                apRetCopy->mpTail = pCopy;
            }
            apRetCopy->mCount++;

            pEnt = pEnt->mpNext;

        } while (NULL != pEnt);
    }
}

void
K2RECTLIST_CopyAndIntersect(
    K2RECTLIST const *  apRectList,
    K2RECT const *      apIntersect,
    int                 aOffsetX,
    int                 aOffsetY,
    K2RECTLIST *        apRetNewListAfterIntersect
)
{
    K2RECTENT *  pEnt;
    K2RECT       intRect;

    K2_ASSERT(NULL != apRetNewListAfterIntersect);
    K2_ASSERT(NULL != apRetNewListAfterIntersect->mpCache);
    K2_ASSERT(apRetNewListAfterIntersect->mpHead == NULL);
    K2_ASSERT(apRetNewListAfterIntersect->mpTail == NULL);
    K2_ASSERT(apRetNewListAfterIntersect->mCount == 0);
   
    K2_ASSERT(K2RECT_IsValid(apIntersect));

    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    if (0 == apRectList->mCount)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(NULL == apRectList->mpTail);
    }
    else
    {
        K2_ASSERT(NULL != apRectList->mpHead);
        K2_ASSERT(NULL != apRectList->mpTail);

        if (K2RECT_HasArea(apIntersect))
        {
            pEnt = apRectList->mpHead;
            do {
                if (K2RECT_Intersect(&pEnt->Rect, apIntersect, aOffsetX, aOffsetY, &intRect))
                {
                    K2RECTLIST_MergeIn(apRetNewListAfterIntersect, &intRect);
                }
                pEnt = pEnt->mpNext;
            } while (NULL != pEnt);
        }
    }
}

void
K2RECTLISTI_MergeAdd(
    K2RECTLIST *     apRectList,
    K2RECTENT *      apNewEnt
)
{
    K2RECTENT * pEnt;
#if K2_DEBUG
    UINT_PTR    dummy;
#endif
    BOOL        merged;

    K2_ASSERT(NULL != apNewEnt);
    K2_ASSERT(K2RECT_IsValidAndHasArea(&apNewEnt->Rect));

    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    if (apRectList->mCount == 0)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(NULL == apRectList->mpTail);
        K2RECTENTI_AddEnt(apRectList, apNewEnt);
        return;
    }
    K2_ASSERT(NULL != apRectList->mpHead);
    K2_ASSERT(NULL != apRectList->mpTail);

    pEnt = apRectList->mpHead;

    do {
        K2_ASSERT(!K2RECT_IntersectBits(&pEnt->Rect, &apNewEnt->Rect, &dummy));

        merged = FALSE;

        if ((pEnt->Rect.TopLeft.mY == apNewEnt->Rect.TopLeft.mY) &&
            (pEnt->Rect.BottomRight.mY == apNewEnt->Rect.BottomRight.mY))
        {
            BOOL newToTheRight = (apNewEnt->Rect.TopLeft.mX == pEnt->Rect.BottomRight.mX) ? TRUE : FALSE;
            if ((newToTheRight) ||
                (apNewEnt->Rect.BottomRight.mX == pEnt->Rect.TopLeft.mX))
            {
                //
                // share a left or right edge, match vertically
                //
                merged = TRUE;

                K2RECTLISTI_RemoveEnt(apRectList, pEnt);

                if (newToTheRight)
                {
                    pEnt->Rect.BottomRight.mX = apNewEnt->Rect.BottomRight.mX;
                }
                else
                {
                    pEnt->Rect.TopLeft.mX = apNewEnt->Rect.TopLeft.mX;
                }

                K2RECTENTI_Free(apRectList->mpCache, apNewEnt);
                apNewEnt = NULL;
            }
        }
        else if ((pEnt->Rect.TopLeft.mX == apNewEnt->Rect.TopLeft.mX) &&
            (pEnt->Rect.BottomRight.mX == apNewEnt->Rect.BottomRight.mX))
        {
            BOOL newIsBelow = (apNewEnt->Rect.TopLeft.mY == pEnt->Rect.BottomRight.mY) ? TRUE : FALSE;
            if ((newIsBelow) ||
                (apNewEnt->Rect.BottomRight.mY == pEnt->Rect.TopLeft.mY))
            {
                //
                // share a top of bottom edge, match horizontally
                //
                merged = TRUE;

                K2RECTLISTI_RemoveEnt(apRectList, pEnt);

                if (newIsBelow)
                {
                    pEnt->Rect.BottomRight.mY = apNewEnt->Rect.BottomRight.mY;
                }
                else
                {
                    pEnt->Rect.TopLeft.mY = apNewEnt->Rect.TopLeft.mY;
                }

                K2RECTENTI_Free(apRectList->mpCache, apNewEnt);
                apNewEnt = NULL;
            }
        }

        if (merged)
        {
            apNewEnt = pEnt;
            pEnt = apRectList->mpHead;
        }
        else
        {
            K2_ASSERT(NULL != apNewEnt);
            pEnt = pEnt->mpNext;
        }

    } while (NULL != pEnt);

    if (NULL != apNewEnt)
    {
        K2RECTENTI_AddEnt(apRectList, apNewEnt);
    }
}

void
K2RECTLIST_MergeIn(
    K2RECTLIST *     apRectList,
    K2RECT const *   apMergeIn
)
{
    K2RECTENT * pEnt;

    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    if (apRectList->mCount == 0)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(NULL == apRectList->mpTail);
    }
    else
    {
        K2_ASSERT(NULL != apRectList->mpHead);
        K2_ASSERT(NULL != apRectList->mpTail);
    }

    K2_ASSERT(K2RECT_IsValid(apMergeIn));

    if (K2RECT_HasArea(apMergeIn))
    {
        pEnt = K2RECTENTI_Alloc(apRectList->mpCache, apMergeIn);
        K2_ASSERT(NULL != pEnt);

        K2RECTLISTI_MergeAdd(apRectList, pEnt);
    }
}

void
K2RECTLIST_CopyAndCutOut(
    K2RECTLIST const *  apRectList,
    K2RECT const *      apCutOut,
    int                 aOffsetX,
    int                 aOffsetY,
    K2RECTLIST *        apRetNewListAfterCutOut
)
{
    static int sNewNeeded[16] = { 3, 2, 2, 1,
        2, 1, 1, 0,
        2, 1, 1, 0,
        1, 0, 0, 0 };

    K2RECTENT * pNew[3];
    K2RECTENT * pWork;
    K2RECTENT * pCopy;
    UINT_PTR    bits;
    int         needed, left;

    K2_ASSERT(NULL != apRetNewListAfterCutOut);
    K2_ASSERT(NULL != apRetNewListAfterCutOut->mpCache);
    K2_ASSERT(0 == apRetNewListAfterCutOut->mCount);
    K2_ASSERT(NULL == apRetNewListAfterCutOut->mpHead);
    K2_ASSERT(NULL == apRetNewListAfterCutOut->mpTail);

    K2_ASSERT(K2RECT_IsValid(apCutOut));
    if (!K2RECT_HasArea(apCutOut))
    {
        K2RECTLIST_Copy(apRectList, aOffsetX, aOffsetY, apRetNewListAfterCutOut);
        return;
    }

    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    if (apRectList->mCount == 0)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(NULL == apRectList->mpTail);
        return;
    }
    K2_ASSERT(NULL != apRectList->mpHead);
    K2_ASSERT(NULL != apRectList->mpTail);

    pNew[0] = NULL;
    pNew[1] = NULL;
    pNew[2] = NULL;

    pWork = apRectList->mpHead;
    do {
        pCopy = K2RECTENTI_Alloc(apRetNewListAfterCutOut->mpCache, &pWork->Rect);
        K2_ASSERT(NULL != pCopy);

        if (K2RECT_IntersectBits(apCutOut, &pCopy->Rect, &bits))
        {
            needed = sNewNeeded[bits];
            left = 0;
            if (needed)
            {
                pNew[0] = K2RECTENTI_Alloc(apRetNewListAfterCutOut->mpCache, &pCopy->Rect);
                K2_ASSERT(NULL != pNew[0]);
                needed--;
                left++;
            }
            if (needed)
            {
                pNew[1] = K2RECTENTI_Alloc(apRetNewListAfterCutOut->mpCache, &pCopy->Rect);
                K2_ASSERT(NULL != pNew[1]);
                needed--;
                left++;
            }
            if (needed)
            {
                pNew[2] = K2RECTENTI_Alloc(apRetNewListAfterCutOut->mpCache, &pCopy->Rect);
                K2_ASSERT(NULL != pNew[2]);
                needed--;
                left++;
            }

            switch (bits)
            {
            case 0:
                K2_ASSERT(left == 3);
                pCopy->Rect.BottomRight.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.BottomRight.mX = apCutOut->TopLeft.mX;
                pNew[0]->Rect.BottomRight.mY = apCutOut->BottomRight.mY;
                pNew[1]->Rect.TopLeft.mY = apCutOut->TopLeft.mY;
                pNew[1]->Rect.TopLeft.mX = apCutOut->BottomRight.mX;
                pNew[1]->Rect.BottomRight.mY = apCutOut->BottomRight.mY;
                pNew[2]->Rect.TopLeft.mY = apCutOut->BottomRight.mY;
                break;

            case 1:
                K2_ASSERT(left == 2);
                pCopy->Rect.BottomRight.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mX = apCutOut->BottomRight.mX;
                pNew[0]->Rect.BottomRight.mY = apCutOut->BottomRight.mY;
                pNew[1]->Rect.TopLeft.mY = apCutOut->BottomRight.mY;
                break;

            case 2:
                K2_ASSERT(left == 2);
                pCopy->Rect.BottomRight.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.BottomRight.mX = apCutOut->TopLeft.mX;
                pNew[0]->Rect.BottomRight.mY = apCutOut->BottomRight.mY;
                pNew[1]->Rect.TopLeft.mY = apCutOut->BottomRight.mY;
                break;

            case 3:
                K2_ASSERT(left == 1);
                pCopy->Rect.BottomRight.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mY = apCutOut->BottomRight.mY;
                break;

            case 4:
                K2_ASSERT(left == 2);
                pCopy->Rect.BottomRight.mY = apCutOut->BottomRight.mY;
                pCopy->Rect.BottomRight.mX = apCutOut->TopLeft.mX;
                pNew[0]->Rect.BottomRight.mY = apCutOut->BottomRight.mY;
                pNew[0]->Rect.TopLeft.mX = apCutOut->BottomRight.mX;
                pNew[1]->Rect.TopLeft.mY = apCutOut->BottomRight.mY;
                break;

            case 5:
                K2_ASSERT(left == 1);
                pCopy->Rect.BottomRight.mY = apCutOut->BottomRight.mY;
                pCopy->Rect.TopLeft.mX = apCutOut->BottomRight.mX;
                pNew[0]->Rect.TopLeft.mY = apCutOut->BottomRight.mY;
                break;

            case 6:
                K2_ASSERT(left == 1);
                pCopy->Rect.BottomRight.mY = apCutOut->BottomRight.mY;
                pCopy->Rect.BottomRight.mX = apCutOut->TopLeft.mX;
                pNew[0]->Rect.TopLeft.mY = apCutOut->BottomRight.mY;
                break;

            case 7:
                K2_ASSERT(left == 0);
                pCopy->Rect.TopLeft.mY = apCutOut->BottomRight.mY;
                break;

            case 8:
                K2_ASSERT(left == 2);
                pCopy->Rect.BottomRight.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.BottomRight.mX = apCutOut->TopLeft.mX;
                pNew[1]->Rect.TopLeft.mY = apCutOut->TopLeft.mY;
                pNew[1]->Rect.TopLeft.mX = apCutOut->BottomRight.mX;
                break;

            case 9:
                K2_ASSERT(left == 1);
                pCopy->Rect.BottomRight.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mX = apCutOut->BottomRight.mX;
                break;

            case 10:
                K2_ASSERT(left == 1);
                pCopy->Rect.BottomRight.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.TopLeft.mY = apCutOut->TopLeft.mY;
                pNew[0]->Rect.BottomRight.mX = apCutOut->TopLeft.mX;
                break;

            case 11:
                K2_ASSERT(left == 0);
                pCopy->Rect.BottomRight.mY = apCutOut->TopLeft.mY;
                break;

            case 12:
                K2_ASSERT(left == 1);
                pCopy->Rect.BottomRight.mX = apCutOut->TopLeft.mX;
                pNew[0]->Rect.TopLeft.mX = apCutOut->BottomRight.mX;
                break;

            case 13:
                K2_ASSERT(left == 0);
                pCopy->Rect.TopLeft.mX = apCutOut->BottomRight.mX;
                break;

            case 14:
                K2_ASSERT(left == 0);
                pCopy->Rect.BottomRight.mX = apCutOut->TopLeft.mX;
                break;

            case 15:
                K2RECTENTI_Free(apRetNewListAfterCutOut->mpCache, pCopy);
                pCopy = NULL;
                break;
            }

            if (bits != 15)
            {
                K2RECT_Offset(&pCopy->Rect, aOffsetX, aOffsetY);
                K2RECTLISTI_MergeAdd(apRetNewListAfterCutOut, pCopy);
                pCopy = NULL;
            }

            if (left)
            {
                K2RECT_Offset(&pNew[0]->Rect, aOffsetX, aOffsetY);
                K2RECTLISTI_MergeAdd(apRetNewListAfterCutOut, pNew[0]);
                pNew[0] = NULL;
                left--;
            }

            if (left)
            {
                K2RECT_Offset(&pNew[1]->Rect, aOffsetX, aOffsetY);
                K2RECTLISTI_MergeAdd(apRetNewListAfterCutOut, pNew[1]);
                pNew[1] = NULL;
                left--;
            }

            if (left)
            {
                K2RECT_Offset(&pNew[2]->Rect, aOffsetX, aOffsetY);
                K2RECTLISTI_MergeAdd(apRetNewListAfterCutOut, pNew[2]);
                pNew[2] = NULL;
            }
        }
        else
        {
            K2RECT_Offset(&pCopy->Rect, aOffsetX, aOffsetY);
            K2RECTLISTI_MergeAdd(apRetNewListAfterCutOut, pCopy);
            pCopy = NULL;
        }

        pWork = pWork->mpNext;

    } while (NULL != pWork);
}

void
K2RECTLIST_CutOut(
    K2RECTLIST *    apRectList,
    int             aOffsetX,
    int             aOffsetY,
    K2RECT const *  apCutOut
)
{
    K2RECTLIST   newList;

    K2RECTLIST_Init(&newList, apRectList->mpCache);
    K2RECTLIST_CopyAndCutOut(apRectList, apCutOut, aOffsetX, aOffsetY, &newList);
    K2RECTLIST_Purge(apRectList);
    K2MEM_Copy(apRectList, &newList, sizeof(K2RECTLIST));
}

void
K2RECTLIST_GetBounded(
    K2RECTLIST const *   apRectList,
    K2RECT *             apBounded
)
{
    K2RECTENT *  pEnt;

    K2_ASSERT(NULL != apBounded);
    K2MEM_Zero(apBounded, sizeof(K2RECT));
    
    K2_ASSERT(NULL != apRectList);
    K2_ASSERT(NULL != apRectList->mpCache);
    if (0 == apRectList->mCount)
    {
        K2_ASSERT(NULL == apRectList->mpHead);
        K2_ASSERT(NULL == apRectList->mpTail);
        return;
    }
    K2_ASSERT(NULL != apRectList->mpHead);
    K2_ASSERT(NULL != apRectList->mpTail);

    apBounded->TopLeft.mY = (int)(((unsigned int)-1) >> 1);
    apBounded->TopLeft.mX = apBounded->TopLeft.mY;
    apBounded->BottomRight.mX = (int)((((unsigned int)-1) >> 1) ^ ((unsigned int)-1));
    apBounded->BottomRight.mY = apBounded->BottomRight.mX;

    pEnt = apRectList->mpHead;
    do {
        if (pEnt->Rect.TopLeft.mX < apBounded->TopLeft.mX)
            apBounded->TopLeft.mX = pEnt->Rect.TopLeft.mX;
        if (pEnt->Rect.BottomRight.mX > apBounded->BottomRight.mX)
            apBounded->BottomRight.mX = pEnt->Rect.BottomRight.mX;
        if (pEnt->Rect.TopLeft.mY < apBounded->TopLeft.mY)
            apBounded->TopLeft.mY = pEnt->Rect.TopLeft.mY;
        if (pEnt->Rect.BottomRight.mY > apBounded->BottomRight.mY)
            apBounded->BottomRight.mY = pEnt->Rect.BottomRight.mY;
        pEnt = pEnt->mpNext;
    } while (NULL != pEnt);
}

