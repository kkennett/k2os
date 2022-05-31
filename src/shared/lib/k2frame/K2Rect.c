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
#include <lib/k2frame.h>

BOOL
K2RECT_IsValid(
    K2RECT const *apRect
)
{
    K2_ASSERT(NULL != apRect);

    if (apRect->TopLeft.mX > apRect->BottomRight.mX)
    {
        return FALSE;
    }

    if (apRect->TopLeft.mY > apRect->BottomRight.mY)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL 
K2RECT_HasArea(
    K2RECT const *apRect
)
{
    K2_ASSERT(NULL != apRect);

    if (apRect->TopLeft.mX >= apRect->BottomRight.mX)
    {
        return FALSE;
    }

    if (apRect->TopLeft.mY >= apRect->BottomRight.mY)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL 
K2RECT_IsValidAndHasArea(
    K2RECT const *apRect
)
{
    if (!K2RECT_IsValid(apRect))
    {
        return FALSE;
    }

    return K2RECT_HasArea(apRect);
}

void 
K2RECT_Offset(
    K2RECT *    apRect,
    int         aOffsetX,
    int         aOffsetY
)
{
    K2_ASSERT(K2RECT_IsValid(apRect));

    apRect->TopLeft.mX += aOffsetX;
    apRect->BottomRight.mX += aOffsetX;
    apRect->TopLeft.mY += aOffsetY;
    apRect->BottomRight.mY += aOffsetY;
}

void 
K2RECT_CopyAndOffset(
    K2RECT const *  apRect,
    int             aOffsetX,
    int             aOffsetY,
    K2RECT *        apRetCopy
)
{
    K2_ASSERT(K2RECT_IsValid(apRect));
    K2_ASSERT(NULL != apRetCopy);

    apRetCopy->TopLeft.mX = apRect->TopLeft.mX + aOffsetX;
    apRetCopy->BottomRight.mX = apRect->BottomRight.mX + aOffsetX;
    apRetCopy->TopLeft.mY = apRect->TopLeft.mY + aOffsetY;
    apRetCopy->BottomRight.mY = apRect->BottomRight.mY + aOffsetY;
}

BOOL
K2RECT_IntersectBits(
    K2RECT const *  apRect1,
    K2RECT const *  apRect2,
    UINT_PTR *      apRetBits
)
{
    K2_ASSERT(K2RECT_IsValid(apRect1));
    K2_ASSERT(K2RECT_IsValid(apRect2));
    K2_ASSERT(NULL != apRetBits);

    *apRetBits = 0;

    if (apRect1->TopLeft.mX <= apRect2->TopLeft.mX)
    {
        if (apRect1->BottomRight.mX <= apRect2->TopLeft.mX)
        {
            return FALSE;
        }
        (*apRetBits) |= K2RECT_INTERSECT_BIT_LEFT;
    }

    if (apRect1->BottomRight.mX >= apRect2->BottomRight.mX)
    {
        if (apRect1->TopLeft.mX >= apRect2->BottomRight.mX)
        {
            return FALSE;
        }
        (*apRetBits) |= K2RECT_INTERSECT_BIT_RIGHT;
    }

    if (apRect1->TopLeft.mY <= apRect2->TopLeft.mY)
    {
        if (apRect1->BottomRight.mY <= apRect2->TopLeft.mY)
        {
            return FALSE;
        }
        (*apRetBits) |= K2RECT_INTERSECT_BIT_TOP;
    }

    if (apRect1->BottomRight.mY >= apRect2->BottomRight.mY)
    {
        if (apRect1->TopLeft.mY >= apRect2->BottomRight.mY)
        {
            return FALSE;
        }
        (*apRetBits) |= K2RECT_INTERSECT_BIT_BOTTOM;
    }

    return TRUE;
}

BOOL
K2RECT_Intersect(
    K2RECT const *  apRect1,
    K2RECT const *  apRect2,
    int             aOffsetX,
    int             aOffsetY,
    K2RECT *        apRetIntersect
)
{
    UINT_PTR bits;

    K2_ASSERT(K2RECT_IsValid(apRect1));
    K2_ASSERT(K2RECT_IsValid(apRect2));

    if (!K2RECT_IntersectBits(apRect1, apRect2, &bits))
    {
        return FALSE;
    }

    if (NULL == apRetIntersect)
    {
        return TRUE;
    }

    if (bits & K2RECT_INTERSECT_BIT_LEFT)
        apRetIntersect->TopLeft.mX = apRect2->TopLeft.mX;
    else
        apRetIntersect->TopLeft.mX = apRect1->TopLeft.mX;
    apRetIntersect->TopLeft.mX += aOffsetX;

    if (bits & K2RECT_INTERSECT_BIT_RIGHT)
        apRetIntersect->BottomRight.mX = apRect2->BottomRight.mX;
    else
        apRetIntersect->BottomRight.mX = apRect1->BottomRight.mX;
    apRetIntersect->BottomRight.mX += aOffsetX;

    if (bits & K2RECT_INTERSECT_BIT_TOP)
        apRetIntersect->TopLeft.mY = apRect2->TopLeft.mY;
    else
        apRetIntersect->TopLeft.mY = apRect1->TopLeft.mY;
    apRetIntersect->TopLeft.mY += aOffsetY;

    if (bits & K2RECT_INTERSECT_BIT_BOTTOM)
        apRetIntersect->BottomRight.mY = apRect2->BottomRight.mY;
    else
        apRetIntersect->BottomRight.mY = apRect1->BottomRight.mY;
    apRetIntersect->BottomRight.mY += aOffsetY;

    return TRUE;
}

BOOL
K2RECT_Clip(
    K2RECT const *  apBase,
    K2RECT *        apClip
)
{
    if ((!K2RECT_IsValidAndHasArea(apBase)) ||
        (!K2RECT_IsValidAndHasArea(apClip)))
    {
        return FALSE;
    }

    if (apClip->TopLeft.mX < apBase->TopLeft.mX)
    {
        if (apClip->BottomRight.mX <= apBase->TopLeft.mX)
            return FALSE;
        apClip->TopLeft.mX = apBase->TopLeft.mX;
    }
    else if (apClip->TopLeft.mX >= apBase->BottomRight.mX)
    {
        return FALSE;
    }

    if (apClip->TopLeft.mY < apBase->TopLeft.mY)
    {
        if (apClip->BottomRight.mY <= apBase->TopLeft.mY)
            return FALSE;
        apClip->TopLeft.mY = apBase->TopLeft.mY;
    }
    else if (apClip->TopLeft.mY >= apBase->BottomRight.mY)
    {
        return FALSE;
    }

    if (apClip->BottomRight.mX > apBase->BottomRight.mX)
    {
        apClip->BottomRight.mX = apBase->BottomRight.mX;
    }

    if (apClip->BottomRight.mY > apBase->BottomRight.mY)
    {
        apClip->BottomRight.mY = apBase->BottomRight.mY;
    }

    return TRUE;
}

void 
K2RECT_FromWH(
    K2RECT *apRect,
    int aLeft,
    int aTop,
    int aWidth,
    int aHeight
)
{
    K2_ASSERT(aWidth >= 0);
    K2_ASSERT(aHeight >= 0);
    K2_ASSERT(NULL != apRect);
    apRect->TopLeft.mX = aLeft;
    apRect->BottomRight.mX = aTop;
    apRect->BottomRight.mX = aLeft + aWidth;
    apRect->BottomRight.mY = aTop + aHeight;
}

void 
K2RECT_GetRotate(
    K2RECT const *  apRect,
    K2Rot           aRot,
    K2POINT *       apRetNewTopLeft,
    K2POINT *       apRetIncLeftToRight,
    K2POINT *       apRetIncTopToBottom
)
{
    int width;
    int height;

    K2_ASSERT(K2RECT_IsValid(apRect));
    K2_ASSERT(NULL != apRetNewTopLeft);
    K2_ASSERT(NULL != apRetIncLeftToRight);
    K2_ASSERT(NULL != apRetIncTopToBottom);

    aRot %= 360;
    if (aRot < 90)
    {
        K2MEM_Copy(apRetNewTopLeft, &apRect->TopLeft, sizeof(K2POINT));
        apRetIncLeftToRight->mX = 1;
        apRetIncLeftToRight->mY = 0;
        apRetIncTopToBottom->mX = 0;
        apRetIncTopToBottom->mY = 1;
        return;
    }

    width = apRect->BottomRight.mX - apRect->TopLeft.mX;
    height = apRect->BottomRight.mY - apRect->TopLeft.mY;

    if (aRot < 180)
    {
        apRetNewTopLeft->mX = apRect->TopLeft.mX;
        apRetNewTopLeft->mY = apRect->TopLeft.mY + height - 1;
        apRetIncLeftToRight->mX = 0;
        apRetIncLeftToRight->mY = -1;
        apRetIncTopToBottom->mX = 1;
        apRetIncTopToBottom->mY = 0;
        return;
    }

    if (aRot < 270)
    {
        apRetNewTopLeft->mX = apRect->TopLeft.mX + width - 1;
        apRetNewTopLeft->mY = apRect->TopLeft.mY + height - 1;
        apRetIncLeftToRight->mX = -1;
        apRetIncLeftToRight->mY = 0;
        apRetIncTopToBottom->mX = 0;
        apRetIncTopToBottom->mY = -1;
        return;
    }

    apRetNewTopLeft->mX = apRect->TopLeft.mX + height - 1;
    apRetNewTopLeft->mY = apRect->TopLeft.mY;
    apRetIncLeftToRight->mX = 0;
    apRetIncLeftToRight->mY = 1;
    apRetIncTopToBottom->mX = -1;
    apRetIncTopToBottom->mY = 0;
}

