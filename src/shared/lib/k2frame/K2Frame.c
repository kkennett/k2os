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

BOOL K2FRAMEI_IsKnown(K2FRAME *apFrame);
void K2FRAMEI_AddNewExposed(K2FRAME *apFrame, K2RECT const *apRect);
void K2FRAMEI_Remove(K2FRAME *apFrame);
void K2FRAMEI_Change(K2FRAME *apFrame, K2RECT const *apNewRect, K2FRAME *apUnder);
void K2FRAMEI_RegenerateDamaged(K2FRAME *apFrame);
void K2FRAMEI_RegenerateValid(K2FRAME *apFrame);

void        
K2FRAME_Init(
    K2FRAME_ANCHOR *    apAnchor,
    K2RECTENT_CACHE *   apCache,
    K2FRAME *           apRootFrame,
    int                 aRootWidth,
    int                 aRootHeight
)
{
    K2_ASSERT(NULL != apAnchor);
    K2_ASSERT(NULL != apCache);
    K2_ASSERT(NULL != apCache->Get);
    K2_ASSERT(NULL != apCache->Put);
    K2_ASSERT(aRootWidth >= 0);
    K2_ASSERT(aRootHeight >= 0);

    K2MEM_Zero(apAnchor, sizeof(K2FRAME_ANCHOR));
    apAnchor->mpCache = apCache;
    K2TREE_Init(&apAnchor->KnownFrameTree, NULL);
    K2LIST_Init(&apAnchor->FrameFixList);

    K2_ASSERT(NULL != apRootFrame);

    K2MEM_Zero(apRootFrame, sizeof(K2FRAME));
    apAnchor->mpRootFrame = apRootFrame;
    apRootFrame->mpAnchor = apAnchor;
    apRootFrame->Rect.BottomRight.mX = aRootWidth;
    apRootFrame->Rect.BottomRight.mY = aRootHeight;
    K2LIST_Init(&apRootFrame->ChildList);
    K2RECTLIST_Init(&apRootFrame->ExposedList, apCache);
    K2RECTLIST_Init(&apRootFrame->BackValidList, apCache);
    K2RECTLIST_Init(&apRootFrame->BackDamagedList, apCache);
    apRootFrame->KnownTreeNode.mUserVal = (UINT_PTR)apRootFrame;
    K2TREE_Insert(&apAnchor->KnownFrameTree, apRootFrame->KnownTreeNode.mUserVal, &apRootFrame->KnownTreeNode);
    if ((aRootWidth > 0) &&
        (aRootHeight > 0))
    {
        K2FRAMEI_AddNewExposed(apRootFrame, &apRootFrame->Rect);
    }

    apAnchor->mInitialized = TRUE;
}

void        
K2FRAME_Done(
    K2FRAME_ANCHOR * apAnchor
)
{
    K2_ASSERT(apAnchor->mInitialized);
    K2FRAMEI_Remove(apAnchor->mpRootFrame);
    K2_ASSERT(0 == apAnchor->KnownFrameTree.mNodeCount);
    K2_ASSERT(0 == apAnchor->FrameFixList.mNodeCount);
    K2MEM_Zero(apAnchor, sizeof(K2FRAME_ANCHOR));
}

void
K2FRAME_Remove(
    K2FRAME * apFrame
)
{
    K2RECT nullRect;

    K2_ASSERT(NULL != apFrame);
    K2_ASSERT(NULL != apFrame->mpAnchor);
    K2_ASSERT(apFrame->mpAnchor->mInitialized);
    K2_ASSERT(apFrame != apFrame->mpAnchor->mpRootFrame);
    K2_ASSERT(K2FRAMEI_IsKnown(apFrame));

    if (0 != apFrame->ExposedList.mCount)
    {
        K2MEM_Zero(&nullRect, sizeof(nullRect));
        K2FRAME_Change(apFrame, &nullRect, K2FRAME_ORDER_SAME);
    }

    K2FRAMEI_Remove(apFrame);
}

void 
K2FRAME_Add(
    K2FRAME *                   apFrame,
    K2FRAME *                   apParent,
    K2RECT const *              apRect,
    K2FRAME *                   apOrderUnder,
    K2FRAME_pf_AddRemove_Notify aKey
)
{
    K2RECTENT_CACHE *   pCache;

    K2_ASSERT(NULL != apFrame);

    K2_ASSERT(NULL != apParent);
    K2_ASSERT(NULL != apParent->mpAnchor);
    K2_ASSERT(apParent->mpAnchor->mInitialized);
    K2_ASSERT(K2FRAMEI_IsKnown(apParent));
  
    K2MEM_Zero(apFrame, sizeof(K2FRAME));
    apFrame->mpAnchor = apParent->mpAnchor;
    K2_ASSERT(!K2FRAMEI_IsKnown(apFrame));
    apFrame->mpParent = apParent;

    if (NULL != apRect)
    {
        K2_ASSERT(K2RECT_IsValid(apRect));
    }

    if (apOrderUnder == K2FRAME_ORDER_TOP)
    {
        apOrderUnder = NULL;
    }
    else if (apOrderUnder == K2FRAME_ORDER_BOTTOM)
    {
        if (apParent->ChildList.mNodeCount > 0)
        {
            apOrderUnder = K2_GET_CONTAINER(K2FRAME, apParent->ChildList.mpTail, ParentChildListLink);
        }
        else
        {
            apOrderUnder = NULL;
        }
    }
    else
    {
        //
        // specific frame it is supposed to be below
        //
        K2_ASSERT(apOrderUnder->mpParent == apParent);
        K2_ASSERT(K2FRAMEI_IsKnown(apOrderUnder));
    }

    pCache = apParent->mpAnchor->mpCache;

    K2LIST_Init(&apFrame->ChildList);
    K2RECTLIST_Init(&apFrame->ExposedList, pCache);
    K2RECTLIST_Init(&apFrame->BackValidList, pCache);
    K2RECTLIST_Init(&apFrame->BackDamagedList, pCache);

    K2LIST_AddAtHead(&apParent->ChildList, &apFrame->ParentChildListLink);

    apFrame->KnownTreeNode.mUserVal = (UINT_PTR)apFrame;
    K2TREE_Insert(&apParent->mpAnchor->KnownFrameTree, apFrame->KnownTreeNode.mUserVal, &apFrame->KnownTreeNode);

    if ((NULL != apRect) ||
        (NULL != apOrderUnder))
    {
        K2FRAMEI_Change(apFrame, apRect, apOrderUnder);
    }
}

void 
K2FRAME_SetDamaged(
    K2FRAME *       apFrame,
    K2RECT const *  apDamagedRect
)
{
    K2RECT  r;

    K2_ASSERT(NULL != apFrame);
    K2_ASSERT(NULL != apFrame->mpAnchor);
    K2_ASSERT(apFrame->mpAnchor->mInitialized);
    K2_ASSERT(K2FRAMEI_IsKnown(apFrame));

    if (NULL != apDamagedRect)
    {
        K2_ASSERT(K2RECT_IsValid(apDamagedRect));
    }

    if ((0 == apFrame->ExposedList.mCount) ||
        (0 == apFrame->BackValidList.mCount))
    {
        //
        // nothing exposed, or nothing is valid. 
        //
        return;
    }

    //
    // something is exposed and something is valid
    //

    if (NULL == apDamagedRect)
    {
        r.TopLeft.mX = r.TopLeft.mY = 0;
        r.BottomRight.mX = apFrame->Rect.BottomRight.mX - apFrame->Rect.TopLeft.mX;
        r.BottomRight.mY = apFrame->Rect.BottomRight.mY - apFrame->Rect.TopLeft.mY;
        apDamagedRect = &r;
    }

    //
    // cut out fixed area from back damaged list
    //
    K2RECTLIST_CutOut(&apFrame->BackValidList, 0, 0, apDamagedRect);

    //
    // regenerate valid list from exposed, child list, and back damaged list
    //
    K2FRAMEI_RegenerateDamaged(apFrame);
}

void 
K2FRAME_SetFixed(
    K2FRAME *       apFrame,
    K2RECT const *  apFixedRect
)
{
    K2RECT  r;

    K2_ASSERT(NULL != apFrame);
    K2_ASSERT(NULL != apFrame->mpAnchor);
    K2_ASSERT(apFrame->mpAnchor->mInitialized);
    K2_ASSERT(K2FRAMEI_IsKnown(apFrame));

    if (NULL != apFixedRect)
    {
        K2_ASSERT(K2RECT_IsValid(apFixedRect));
    }

    if ((0 == apFrame->ExposedList.mCount) ||
        (0 == apFrame->BackDamagedList.mCount))
    {
        //
        // nothing exposed, or nothing is damaged. 
        //
        return;
    }

    //
    // something is exposed and something is damaged
    //

    if (NULL == apFixedRect)
    {
        r.TopLeft.mX = r.TopLeft.mY = 0;
        r.BottomRight.mX = apFrame->Rect.BottomRight.mX - apFrame->Rect.TopLeft.mX;
        r.BottomRight.mY = apFrame->Rect.BottomRight.mY - apFrame->Rect.TopLeft.mY;
        apFixedRect = &r;
    }

    //
    // cut out fixed area from back damaged list
    //
    K2RECTLIST_CutOut(&apFrame->BackDamagedList, 0, 0, apFixedRect);

    //
    // regenerate valid list from exposed, child list, and back damaged list
    //
    K2FRAMEI_RegenerateValid(apFrame);
}

void 
K2FRAME_Change(
    K2FRAME *       apFrame,
    K2RECT const *  apNewRect,
    K2FRAME *       apOrderUnder
)
{
    K2FRAME *   pParent;

    K2_ASSERT(NULL != apFrame);
    K2_ASSERT(NULL != apFrame->mpAnchor);
    K2_ASSERT(apFrame->mpAnchor->mInitialized);
    K2_ASSERT(K2FRAMEI_IsKnown(apFrame));

    if (NULL != apNewRect)
    {
        K2_ASSERT(K2RECT_IsValid(apNewRect));
    }

    if (apFrame == apFrame->mpAnchor->mpRootFrame)
    {
        if (NULL != apNewRect)
        {
            K2_ASSERT(0 == apNewRect->TopLeft.mX);
            K2_ASSERT(0 == apNewRect->TopLeft.mY);
        }
        apOrderUnder = NULL;
    }
    else
    {
        pParent = apFrame->mpParent;
        K2_ASSERT(NULL != pParent);

        if (apOrderUnder == K2FRAME_ORDER_TOP)
        {
            apOrderUnder = NULL;
        }
        else if (apOrderUnder == K2FRAME_ORDER_BOTTOM)
        {
            if (pParent->ChildList.mNodeCount > 0)
            {
                apOrderUnder = K2_GET_CONTAINER(K2FRAME, pParent->ChildList.mpTail, ParentChildListLink);
            }
            else
            {
                apOrderUnder = NULL;
            }
        }
        else if (apOrderUnder == K2FRAME_ORDER_SAME)
        {
            if (apFrame->ParentChildListLink.mpPrev != NULL)
            {
                apOrderUnder = K2_GET_CONTAINER(K2FRAME, apFrame->ParentChildListLink.mpPrev, ParentChildListLink);
            }
            else
            {
                apOrderUnder = NULL;
            }
        }
        else
        {
            //
            // specific frame it is supposed to be below
            //
            K2_ASSERT(apOrderUnder->mpParent == pParent);
            K2_ASSERT(K2FRAMEI_IsKnown(apOrderUnder));
        }
    }

    K2FRAMEI_Change(apFrame, apNewRect, apOrderUnder);
}

void 
K2FRAME_GetRootOffset(
    K2FRAME *   apFrame,
    int *       apRetOffsetX,
    int *       apRetOffsetY
)
{
    K2FRAME * pRoot;

    K2_ASSERT(NULL != apFrame);
    K2_ASSERT(NULL != apFrame->mpAnchor);
    K2_ASSERT(apFrame->mpAnchor->mInitialized);
    K2_ASSERT(K2FRAMEI_IsKnown(apFrame));

    pRoot = apFrame->mpAnchor->mpRootFrame;

    if (NULL != apRetOffsetX)
    {
        *apRetOffsetX = 0;
    }

    if (NULL != apRetOffsetX)
    {
        *apRetOffsetY = 0;
    }

    if (apFrame == pRoot)
    {
        return;
    }

    do {
        if (NULL != apRetOffsetX)
        {
            (*apRetOffsetX) += apFrame->Rect.TopLeft.mX;
        }
        if (NULL != apRetOffsetY)
        {
            (*apRetOffsetY) += apFrame->Rect.TopLeft.mY;
        }
        apFrame = apFrame->mpParent;

    } while (apFrame != pRoot);
}

BOOL 
K2FRAME_GetNextDamaged(
    K2FRAME_ANCHOR * apAnchor,
    K2FRAME **       appRetFrame,
    K2RECT *         apRetFrameRect,
    K2RECT *         apRetRootRect
)
{
    K2FRAME *   pFrame;
    K2RECT      dummy;

    K2_ASSERT(NULL != apAnchor);
    K2_ASSERT(apAnchor->mInitialized);

    if (0 == apAnchor->FrameFixList.mNodeCount)
    {
        return FALSE;
    }

    if (0 == apAnchor->FrameFixList.mNodeCount)
    {
        if (NULL != appRetFrame)
        {
            *appRetFrame = NULL;
        }

        if (NULL != apRetFrameRect)
        {
            K2MEM_Zero(apRetFrameRect, sizeof(K2RECT));
        }

        if (NULL != apRetRootRect)
        {
            K2MEM_Zero(apRetRootRect, sizeof(K2RECT));
        }

        return FALSE;
    }

    pFrame = K2_GET_CONTAINER(K2FRAME, apAnchor->FrameFixList.mpHead, FrameFixListLink);

    K2_ASSERT(0 != pFrame->BackDamagedList.mCount);
    K2_ASSERT(NULL != pFrame->BackDamagedList.mpHead);

    if (NULL != appRetFrame)
    {
        *appRetFrame = pFrame;
    }

    if (NULL != apRetRootRect)
    {
        if (NULL == apRetFrameRect)
            apRetFrameRect = &dummy;
    }

    if (NULL != apRetFrameRect)
    {
        K2MEM_Copy(apRetFrameRect, &pFrame->BackDamagedList.mpHead->Rect, sizeof(K2RECT));
    }

    if (NULL != apRetRootRect)
    {
        if (NULL != pFrame->mpParent)
        {
            K2RECT_CopyAndOffset(apRetFrameRect, pFrame->Rect.TopLeft.mX, pFrame->Rect.TopLeft.mY, apRetRootRect);
            do {
                pFrame = pFrame->mpParent;
                if (NULL == pFrame)
                    break;
                K2RECT_Offset(apRetRootRect, pFrame->Rect.TopLeft.mX, pFrame->Rect.TopLeft.mY);
            } while (1);
        }
        else
        {
            K2MEM_Copy(apRetRootRect, apRetFrameRect, sizeof(K2RECT));
        }
    }

    return TRUE;
}

BOOL
K2FRAMEI_IsKnown(
    K2FRAME *apFrame
)
{
    return (NULL == K2TREE_Find(&apFrame->mpAnchor->KnownFrameTree, (UINT_PTR)apFrame)) ? FALSE : TRUE;
}

void 
K2FRAMEI_AddNewExposed(
    K2FRAME *       apFrame,
    K2RECT const *  apRect
)
{
    K2RECTLIST      antiExposed;
    K2RECT          r;
    BOOL            ok;
    K2RECTENT *     pEnt;
    K2LIST_LINK *   pListLink;
    K2FRAME *       pChild;
    K2RECT          frameRect;
    K2RECT          isectRect;
    K2RECTLIST      childNewExposed;

    K2_ASSERT(apFrame->Rect.BottomRight.mX > apFrame->Rect.TopLeft.mX);
    K2_ASSERT(apFrame->Rect.BottomRight.mY > apFrame->Rect.TopLeft.mY);
    K2_ASSERT(K2RECT_IsValidAndHasArea(apRect));
    K2_ASSERT(apRect->TopLeft.mX >= 0);
    K2_ASSERT(apRect->TopLeft.mY >= 0);

    //
    // sanity
    //
    K2RECT_CopyAndOffset(&apFrame->Rect, -apFrame->Rect.TopLeft.mX, -apFrame->Rect.TopLeft.mY, &frameRect);
    ok = K2RECT_Intersect(&frameRect, apRect, 0, 0, &isectRect);
    K2_ASSERT(ok);
    K2_ASSERT(isectRect.TopLeft.mX == apRect->TopLeft.mX);
    K2_ASSERT(isectRect.TopLeft.mY == apRect->TopLeft.mY);
    K2_ASSERT(isectRect.BottomRight.mX == apRect->BottomRight.mX);
    K2_ASSERT(isectRect.BottomRight.mY == apRect->BottomRight.mY);

    //
    // remove current exposed and the new exposed to a new
    // anti-exposed list
    //
    K2RECTLIST_Init(&antiExposed, apFrame->mpAnchor->mpCache);
    r.TopLeft.mX = r.TopLeft.mY = 0;
    r.BottomRight.mX = apFrame->Rect.BottomRight.mX - apFrame->Rect.TopLeft.mX;
    r.BottomRight.mY = apFrame->Rect.BottomRight.mY - apFrame->Rect.TopLeft.mY;
    K2RECTLIST_MergeIn(&antiExposed, &r);
    if (0 != apFrame->ExposedList.mCount)
    {
        pEnt = apFrame->ExposedList.mpHead;
        do {
            K2RECTLIST_CutOut(&antiExposed, 0, 0, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
    }
    K2RECTLIST_CutOut(&antiExposed, 0, 0, apRect);

    //
    // now make the new exposed list by removing all the anti stuff
    //
    K2RECTLIST_Purge(&apFrame->ExposedList);
    r.TopLeft.mX = r.TopLeft.mY = 0;
    r.BottomRight.mX = apFrame->Rect.BottomRight.mX - apFrame->Rect.TopLeft.mX;
    r.BottomRight.mY = apFrame->Rect.BottomRight.mY - apFrame->Rect.TopLeft.mY;
    K2RECTLIST_MergeIn(&apFrame->ExposedList, &r);
    if (0 != antiExposed.mCount)
    {
        pEnt = antiExposed.mpHead;
        do {
            K2RECTLIST_CutOut(&apFrame->ExposedList, 0, 0, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
        K2RECTLIST_Purge(&antiExposed);
    }

    //
    // now we can remove children from the exposed list to get to the background list
    // where by default everything is damaged until we remove the valid parts
    //
    K2RECTLIST_Purge(&apFrame->BackDamagedList);
    K2RECTLIST_Copy(&apFrame->ExposedList, 0, 0, &apFrame->BackDamagedList);
    if (0 != apFrame->ChildList.mNodeCount)
    {
        pListLink = apFrame->ChildList.mpHead;
        do {
            pChild = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);

            K2RECTLIST_Init(&childNewExposed, apFrame->mpAnchor->mpCache);

            K2RECTLIST_CopyAndIntersect(&apFrame->BackDamagedList, &pChild->Rect, -pChild->Rect.TopLeft.mX, -pChild->Rect.TopLeft.mY, &childNewExposed);

            if (0 != pChild->ExposedList.mCount)
            {
                pEnt = pChild->ExposedList.mpHead;
                do {
                    K2RECTLIST_CutOut(&childNewExposed, 0, 0, &pEnt->Rect);
                    pEnt = pEnt->mpNext;
                } while (NULL != pEnt);
            }

            if (0 != childNewExposed.mCount)
            {
                pEnt = childNewExposed.mpHead;
                do {
                    K2FRAMEI_AddNewExposed(pChild, &pEnt->Rect);
                    pEnt = pEnt->mpNext;
                } while (NULL != pEnt);
                K2RECTLIST_Purge(&childNewExposed);
            }

            K2RECTLIST_CutOut(&apFrame->BackDamagedList, 0, 0, &pChild->Rect);

            pListLink = pListLink->mpNext;

        } while (NULL != pListLink);
    }

    //
    // remove old valid stuff from the new damaged list to get the
    // new damaged list
    //
    if (0 != apFrame->BackValidList.mCount)
    {
        pEnt = apFrame->BackValidList.mpHead;
        do {
            K2RECTLIST_CutOut(&apFrame->BackDamagedList, 0, 0, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
    }

    //
    // probably have new damaged stuff
    //
    if (0 != apFrame->BackDamagedList.mCount)
    {
        if (!apFrame->mOnFrameFixList)
        {
            K2LIST_AddAtTail(&apFrame->mpAnchor->FrameFixList, &apFrame->FrameFixListLink);
            apFrame->mOnFrameFixList = TRUE;
        }
    }

    //
    // now finally regenerate the new valid list from the new damaged list
    //
    K2FRAMEI_RegenerateValid(apFrame);
}

void
K2FRAMEI_Remove(
    K2FRAME *apFrame
)
{
    K2LIST_LINK *               pListLink;
    K2FRAME  *                  pChild;
    K2FRAME_pf_AddRemove_Notify key;

    K2_ASSERT(!apFrame->mOnFrameFixList);

    pListLink = apFrame->ChildList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pChild = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);
            pListLink = pListLink->mpNext;
            K2FRAMEI_Remove(pChild);
        } while (NULL != pListLink);
    }

    if (NULL != apFrame->mpParent)
    {
        K2LIST_Remove(&apFrame->mpParent->ChildList, &apFrame->ParentChildListLink);
    }

    K2RECTLIST_Purge(&apFrame->ExposedList);
    K2RECTLIST_Purge(&apFrame->BackValidList);
    K2RECTLIST_Purge(&apFrame->BackDamagedList);

    K2TREE_Remove(&apFrame->mpAnchor->KnownFrameTree, &apFrame->KnownTreeNode);

    key = apFrame->mKey;
    K2MEM_Zero(apFrame, sizeof(K2FRAME));
    if (NULL != key)
    {
        key((UINT_PTR)key, FALSE);
    }
}

void
K2FRAMEI_RegenerateDamaged(
    K2FRAME *   apFrame
)
{
    K2LIST_LINK *   pListLink;
    K2FRAME *       pChild;
    K2RECTENT *     pEnt;

    K2RECTLIST_Purge(&apFrame->BackDamagedList);

    K2RECTLIST_Copy(&apFrame->ExposedList, 0, 0, &apFrame->BackDamagedList);

    if (0 != apFrame->ChildList.mNodeCount)
    {
        pListLink = apFrame->ChildList.mpHead;
        do {
            pChild = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);
            pListLink = pListLink->mpNext;
            K2RECTLIST_CutOut(&apFrame->BackDamagedList, 0, 0, &pChild->Rect);
        } while (NULL != pListLink);
    }

    if (0 != apFrame->BackValidList.mCount)
    {
        pEnt = apFrame->BackValidList.mpHead;
        do {
            K2RECTLIST_CutOut(&apFrame->BackDamagedList, 0, 0, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
    }

    if (0 != apFrame->BackDamagedList.mCount)
    {
        if (!apFrame->mOnFrameFixList)
        {
            K2LIST_AddAtTail(&apFrame->mpAnchor->FrameFixList, &apFrame->FrameFixListLink);
            apFrame->mOnFrameFixList = TRUE;
        }
    }
    else
    {
        K2_ASSERT(!apFrame->mOnFrameFixList);
    }
}

void
K2FRAMEI_RegenerateValid(
    K2FRAME *   apFrame
)
{
    K2LIST_LINK *   pListLink;
    K2FRAME *       pChild;
    K2RECTENT *     pEnt;

    K2RECTLIST_Purge(&apFrame->BackValidList);

    K2RECTLIST_Copy(&apFrame->ExposedList, 0, 0, &apFrame->BackValidList);

    if (0 != apFrame->ChildList.mNodeCount)
    {
        pListLink = apFrame->ChildList.mpHead;
        do {
            pChild = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);
            pListLink = pListLink->mpNext;
            K2RECTLIST_CutOut(&apFrame->BackValidList, 0, 0, &pChild->Rect);
        } while (NULL != pListLink);
    }

    if (0 != apFrame->BackDamagedList.mCount)
    {
        K2_ASSERT(apFrame->mOnFrameFixList);
        pEnt = apFrame->BackDamagedList.mpHead;
        do {
            K2RECTLIST_CutOut(&apFrame->BackValidList, 0, 0, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
    }
    else
    {
        if (apFrame->mOnFrameFixList)
        {
            K2LIST_Remove(&apFrame->mpAnchor->FrameFixList, &apFrame->FrameFixListLink);
            apFrame->mOnFrameFixList = FALSE;
        }
    }
}

void
K2FRAMEI_RemoveFromExposed(
    K2FRAME *        apFrame,
    K2RECT const *   apRect
)
{
    K2RECT          isectRect;
    BOOL            ok;
    K2LIST_LINK *   pListLink;
    K2RECT          frameRect;
    K2FRAME  *      pChild;

    K2_ASSERT(apFrame->Rect.BottomRight.mX > apFrame->Rect.TopLeft.mX);
    K2_ASSERT(apFrame->Rect.BottomRight.mY > apFrame->Rect.TopLeft.mY);
    K2_ASSERT(K2RECT_IsValidAndHasArea(apRect));
    K2_ASSERT(apRect->TopLeft.mX >= 0);
    K2_ASSERT(apRect->TopLeft.mY >= 0);

    //
    // sanity test for clip which should never happen
    //
    K2RECT_CopyAndOffset(&apFrame->Rect, -apFrame->Rect.TopLeft.mX, -apFrame->Rect.TopLeft.mY, &frameRect);
    ok = K2RECT_Intersect(&frameRect, apRect, 0, 0, &isectRect);
    K2_ASSERT(ok);
    K2_ASSERT(isectRect.TopLeft.mX == apRect->TopLeft.mX);
    K2_ASSERT(isectRect.TopLeft.mY == apRect->TopLeft.mY);
    K2_ASSERT(isectRect.BottomRight.mX == apRect->BottomRight.mX);
    K2_ASSERT(isectRect.BottomRight.mY == apRect->BottomRight.mY);

    K2RECTLIST_CutOut(&apFrame->ExposedList, 0, 0, apRect);
    K2RECTLIST_CutOut(&apFrame->BackValidList, 0, 0, apRect);
    K2RECTLIST_CutOut(&apFrame->BackDamagedList, 0, 0, apRect);

    if (0 != apFrame->ChildList.mNodeCount)
    {
        pListLink = apFrame->ChildList.mpHead;
        do {
            pChild = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);
            if (K2RECT_Intersect(apRect, &pChild->Rect, -pChild->Rect.TopLeft.mX, -pChild->Rect.TopLeft.mY, &isectRect))
            {
                K2FRAMEI_RemoveFromExposed(pChild, &isectRect);
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
}

void
K2FRAMEI_Vanish(
    K2FRAME * apFrame
)
{
    K2LIST_LINK *   pListLink;
    K2FRAME *       pChild;

    if (0 == apFrame->ExposedList.mCount)
        return;

    K2RECTLIST_Purge(&apFrame->ExposedList);
    K2RECTLIST_Purge(&apFrame->BackDamagedList);
    K2RECTLIST_Purge(&apFrame->BackValidList);

    if (0 != apFrame->ChildList.mNodeCount)
    {
        pListLink = apFrame->ChildList.mpHead;
        do {
            pChild = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);
            K2FRAMEI_Vanish(pChild);
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
}

void
K2FRAMEI_ExposureChanged(
    K2FRAME *       apFrame,
    K2RECTLIST *    apNewExp,
    K2RECT const *  apNewRect
)
{
    K2RECTLIST   minus;
    K2RECTLIST   plus;
    K2RECTENT *  pEnt;

    //
    // subtract new exposed list from old exposed list
    // to get stuff to remove
    //
    K2RECTLIST_Init(&minus, apFrame->mpAnchor->mpCache);
    K2RECTLIST_Copy(&apFrame->ExposedList, 0, 0, &minus);
    if ((0 != apNewExp->mCount) && (0 != minus.mCount))
    {
        pEnt = apNewExp->mpHead;
        do {
            K2RECTLIST_CutOut(&minus, 0, 0, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
    }

    //
    // subtract old exposed list from new exposed list
    // to get stuff to add
    //
    K2RECTLIST_Init(&plus, apFrame->mpAnchor->mpCache);
    K2RECTLIST_Copy(apNewExp, 0, 0, &plus);
    if ((0 != apFrame->ExposedList.mCount) && (0 != plus.mCount))
    {
        pEnt = apFrame->ExposedList.mpHead;
        do {
            K2RECTLIST_CutOut(&plus, 0, 0, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
    }

    if (0 != minus.mCount)
    {
        pEnt = minus.mpHead;
        do {
            K2FRAMEI_RemoveFromExposed(apFrame, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
        K2RECTLIST_Purge(&minus);
    }

    K2MEM_Copy(&apFrame->Rect, apNewRect, sizeof(K2RECT));

    if (0 != plus.mCount)
    {
        pEnt = plus.mpHead;
        do {
            K2FRAMEI_AddNewExposed(apFrame, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
        K2RECTLIST_Purge(&plus);
    }
}

void
K2FRAMEI_Change(
    K2FRAME *       apFrame,
    K2RECT const *   apNewRect,
    K2FRAME *       apNewBelow
)
{
    K2FRAME *       pParent;
    K2FRAME *       pOldBelow;
    K2LIST_LINK *   pListLink;
    BOOL            posChange;
    BOOL            orderChange;
    BOOL            dimChange;
    BOOL            movingDown;
    K2RECT          newRect;
    K2RECTLIST      workList;
    K2RECTLIST      newFrameExposed;
    K2FRAME *       pChild;
    K2FRAME *       pMowTo;
    K2RECTENT *     pEnt;
    K2RECT const *  pUseRect;

    K2_ASSERT((NULL == apNewRect) || K2RECT_IsValid(apNewRect));

    //
    // get frame it is currently below before the change
    //
    pListLink = apFrame->ParentChildListLink.mpPrev;
    if (NULL == pListLink)
    {
        pOldBelow = NULL;
    }
    else
    {
        pOldBelow = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);
    }
    orderChange = (apNewBelow == pOldBelow) ? FALSE : TRUE;

    //
    // determine if position within parent is changing.  if it is the whole
    // frame contents get nuked
    //
    posChange = FALSE;
    if (NULL == apNewRect)
    {
        if (!orderChange)
        {
            //
            // no change
            //
            return;
        }
        dimChange = FALSE;
        K2MEM_Copy(&newRect, &apFrame->Rect, sizeof(K2RECT));
        apNewRect = &newRect;
    }
    else
    {
        dimChange = ((apNewRect->BottomRight.mY - apNewRect->TopLeft.mY) != (apFrame->Rect.BottomRight.mY - apFrame->Rect.TopLeft.mY)) ? TRUE : FALSE;
        if (!dimChange)
            dimChange = ((apNewRect->BottomRight.mX - apNewRect->TopLeft.mX) != (apFrame->Rect.BottomRight.mX - apFrame->Rect.TopLeft.mX)) ? TRUE : FALSE;
        if ((apNewRect->TopLeft.mX == apFrame->Rect.TopLeft.mX) &&
            (apNewRect->TopLeft.mY == apFrame->Rect.TopLeft.mY))
        {
            if ((!dimChange) && (!orderChange))
            {
                //
                // no change
                //
                return;
            }
        }
        else
        {
            K2_ASSERT(apFrame != apFrame->mpAnchor->mpRootFrame);
            posChange = TRUE;
        }
    }

    //
    // if we get here apNewRect points to new rect location
    // which may be the same as the old location
    //
    K2_ASSERT(NULL != apNewRect);

    movingDown = FALSE;
    if (orderChange)
    {
        if (NULL == pOldBelow)
        {
            movingDown = TRUE;
        }
        else if (NULL != apNewBelow)
        {
            //
            // both old and new below are not NULL
            // but are different. determine if frame is moving up
            // or down in the order
            //
            pListLink = pOldBelow->ParentChildListLink.mpPrev;
            do {
                if (pListLink == &apNewBelow->ParentChildListLink)
                    break;
                pListLink = pListLink->mpPrev;
            } while (NULL != pListLink);
            if (NULL == pListLink)
            {
                movingDown = TRUE;
            }
        }
    }

    if (apFrame == apFrame->mpAnchor->mpRootFrame)
    {
        K2_ASSERT(apNewRect->TopLeft.mX == 0);
        K2_ASSERT(apNewRect->TopLeft.mY == 0);
        K2_ASSERT(!orderChange);
        K2_ASSERT(!posChange);
        K2RECTLIST_Init(&newFrameExposed, apFrame->mpAnchor->mpCache);
        K2RECTLIST_MergeIn(&newFrameExposed, apNewRect);
        K2FRAMEI_ExposureChanged(apFrame, &newFrameExposed, apNewRect);
        K2RECTLIST_Purge(&newFrameExposed);
        return;
    }

    pParent = apFrame->mpParent;
    K2_ASSERT(NULL != pParent);

    if (posChange)
    {
        K2FRAMEI_Vanish(apFrame);
    }

    K2RECTLIST_Init(&workList, apFrame->mpAnchor->mpCache);
    K2RECTLIST_Copy(&pParent->ExposedList, 0, 0, &workList);

    pMowTo = pOldBelow;
    if (orderChange)
    {
        if (!movingDown)
        {
            pMowTo = apNewBelow;
        }

        K2LIST_Remove(&pParent->ChildList, &apFrame->ParentChildListLink);

        if (NULL == apNewBelow)
        {
            K2LIST_AddAtHead(&pParent->ChildList, &apFrame->ParentChildListLink);
        }
        else
        {
            K2LIST_AddAfter(&pParent->ChildList, &apFrame->ParentChildListLink, &apNewBelow->ParentChildListLink);
        }
    }

    pListLink = pParent->ChildList.mpHead;
    if (NULL != pMowTo)
    {
        do {
            pChild = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);
            K2_ASSERT(pChild != apFrame);
            K2RECTLIST_CutOut(&workList, 0, 0, &pChild->Rect);
            pListLink = pListLink->mpNext;
            K2_ASSERT(NULL != pListLink);
        } while (pChild != pMowTo);
    }

    //
    // from this point down in the parent, the exposure lists(s) may be changing
    //
    K2_ASSERT(NULL != pListLink);
    do {
        pChild = K2_GET_CONTAINER(K2FRAME, pListLink, ParentChildListLink);
        K2RECTLIST_Init(&newFrameExposed, apFrame->mpAnchor->mpCache);
        if (pChild == apFrame)
            pUseRect = apNewRect;
        else
            pUseRect = &pChild->Rect;
        K2RECTLIST_CopyAndIntersect(&workList, pUseRect, -pUseRect->TopLeft.mX, -pUseRect->TopLeft.mY, &newFrameExposed);
        K2FRAMEI_ExposureChanged(pChild, &newFrameExposed, pUseRect);
        K2RECTLIST_Purge(&newFrameExposed);
        K2RECTLIST_CutOut(&workList, 0, 0, &pChild->Rect);
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    //
    // remove parents previously valid bits
    //
    if (0 != pParent->BackValidList.mCount)
    {
        pEnt = pParent->BackValidList.mpHead;
        do {
            K2RECTLIST_CutOut(&workList, 0, 0, &pEnt->Rect);
            pEnt = pEnt->mpNext;
        } while (NULL != pEnt);
    }

    //
    // whatever is left is the parent's new damaged list
    // which we use to generate the valid list
    //
    K2RECTLIST_Purge(&pParent->BackDamagedList);
    K2MEM_Copy(&pParent->BackDamagedList, &workList, sizeof(K2RECTLIST));
    if (0 != pParent->BackDamagedList.mCount)
    {
        if (!pParent->mOnFrameFixList)
        {
            K2LIST_AddAtTail(&pParent->mpAnchor->FrameFixList, &pParent->FrameFixListLink);
            pParent->mOnFrameFixList = TRUE;
        }
    }
    else
    {
        if (pParent->mOnFrameFixList)
        {
            K2LIST_Remove(&pParent->mpAnchor->FrameFixList, &pParent->FrameFixListLink);
            pParent->mOnFrameFixList = FALSE;
        }
    }
    K2FRAMEI_RegenerateValid(pParent);
}
