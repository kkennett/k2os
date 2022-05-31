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
#ifndef __K2FRAME_H
#define __K2FRAME_H

#include <k2systype.h>
#include <lib/k2mem.h>
#include <lib/k2list.h>
#include <lib/k2tree.h>

//
//------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

#define K2RECT_INTERSECT_BIT_LEFT        1
#define K2RECT_INTERSECT_BIT_RIGHT       2
#define K2RECT_INTERSECT_BIT_TOP         4
#define K2RECT_INTERSECT_BIT_BOTTOM      8

BOOL K2RECT_IsValid(K2RECT const *apRect);
BOOL K2RECT_HasArea(K2RECT const *apRect);
BOOL K2RECT_IsValidAndHasArea(K2RECT const *apRect);
void K2RECT_Offset(K2RECT *apRect, int aOffsetX, int aOffsetY);
void K2RECT_CopyAndOffset(K2RECT const *apRect, int aOffsetX, int aOffsetY, K2RECT *apRetCopy);
BOOL K2RECT_IntersectBits(K2RECT const *apRect1, K2RECT const *apRect2, UINT_PTR *apRetBits);
BOOL K2RECT_Intersect(K2RECT const *apRect1, K2RECT const *apRect2, int aOffsetX, int aOffsetY, K2RECT *apRetIntersect);
BOOL K2RECT_Clip(K2RECT const *apBase, K2RECT *apClip);
void K2RECT_FromWH(K2RECT *apRect, int aLeft, int aTop, int aWidth, int aHeight);
void K2RECT_GetRotate(K2RECT const *apRect, K2Rot aRot, K2POINT *apRetNewTopLeft, K2POINT *apRetIncLeftToRight, K2POINT *apRetIncTopToBottom);

//
//------------------------------------------------------
//

typedef struct _K2RECTENT K2RECTENT;
struct _K2RECTENT
{
    K2RECT      Rect;
    K2RECTENT * mpPrev;
    K2RECTENT * mpNext;
};

typedef struct _K2RECTENT_CACHE K2RECTENT_CACHE;
typedef K2RECTENT * (*K2RECTENT_CACHE_pf_Get)(K2RECTENT_CACHE *apCache);
typedef void        (*K2RECTENT_CACHE_pf_Put)(K2RECTENT_CACHE *apCache, K2RECTENT *apEnt);
struct _K2RECTENT_CACHE
{
    K2RECTENT_CACHE_pf_Get  Get;
    K2RECTENT_CACHE_pf_Put  Put;
};

typedef struct _K2RECTLIST   K2RECTLIST;
struct _K2RECTLIST
{
    K2RECTENT_CACHE *   mpCache;
    K2RECTENT *         mpHead;
    K2RECTENT *         mpTail;
    UINT_PTR            mCount;
};

void K2RECTLIST_Init(K2RECTLIST *apRectList, K2RECTENT_CACHE * apCache);

void K2RECTLIST_Purge(K2RECTLIST *apRectList);

void K2RECTLIST_MergeIn(K2RECTLIST *apRectList, K2RECT const *apMergeIn);

void K2RECTLIST_Copy(K2RECTLIST const *apRectList, int aOffsetX, int aOffsetY, K2RECTLIST *apRetCopy);
void K2RECTLIST_CopyAndIntersect(K2RECTLIST const *apRectList, K2RECT const *apIntersect, int aOffsetX, int aOffsetY, K2RECTLIST *apRetNewListAfterIntersect);

void K2RECTLIST_CutOut(K2RECTLIST *apRectList, int aOffsetX, int aOffsetY, K2RECT const *apCutOut);
void K2RECTLIST_CopyAndCutOut(K2RECTLIST const *apRectList, K2RECT const *apCutOut, int aOffsetX, int aOffsetY, K2RECTLIST *apRetNewListAfterCutOut);

void K2RECTLIST_GetBounded(K2RECTLIST const *apRectList, K2RECT *apBounded);

//
//------------------------------------------------------
//

typedef struct _K2FRAME_ANCHOR K2FRAME_ANCHOR;

typedef void (*K2FRAME_pf_AddRemove_Notify)(UINT_PTR aKeyAddr, BOOL aIsAdd);

typedef struct _K2FRAME K2FRAME;
struct _K2FRAME
{
    K2FRAME_ANCHOR *            mpAnchor;

    K2FRAME *                   mpParent;

    K2RECT                      Rect;

    K2TREE_NODE                 KnownTreeNode;

    K2FRAME_pf_AddRemove_Notify mKey;

    K2RECTLIST                  ExposedList;
    K2RECTLIST                  BackValidList;
    K2RECTLIST                  BackDamagedList;

    BOOL                        mOnFrameFixList;
    K2LIST_LINK                 FrameFixListLink;

    K2LIST_ANCHOR               ChildList;
    K2LIST_LINK                 ParentChildListLink;
};

struct _K2FRAME_ANCHOR
{
    BOOL                mInitialized;
    K2RECTENT_CACHE *   mpCache;
    K2TREE_ANCHOR       KnownFrameTree;
    K2FRAME *           mpRootFrame;
    K2LIST_ANCHOR       FrameFixList;
};

#define K2FRAME_ORDER_SAME   ((K2FRAME *)0)
#define K2FRAME_ORDER_TOP    ((K2FRAME *)((UINT_PTR)-1))
#define K2FRAME_ORDER_BOTTOM ((K2FRAME *)((UINT_PTR)-2))

void K2FRAME_Init(K2FRAME_ANCHOR * apAnchor, K2RECTENT_CACHE *apCache, K2FRAME *apRootFrame, int aRootWidth, int aRootHeight);
void K2FRAME_Done(K2FRAME_ANCHOR * apAnchor);

void K2FRAME_Add(K2FRAME *apFrame, K2FRAME *apParent, K2RECT const *apRect, K2FRAME *apOrderUnder, K2FRAME_pf_AddRemove_Notify aKey);
void K2FRAME_Remove(K2FRAME *apFrame);

void K2FRAME_SetDamaged(K2FRAME *apFrame, K2RECT const *apDamagedRect);
void K2FRAME_SetFixed(K2FRAME *apFrame, K2RECT const *apFixedRect);

void K2FRAME_Change(K2FRAME *apFrame, K2RECT const *apNewRect, K2FRAME *apOrderUnder);

void K2FRAME_GetRootOffset(K2FRAME *apFrame, int *apRetOffsetX, int *apRetOffsetY);

BOOL K2FRAME_GetNextDamaged(K2FRAME_ANCHOR *apAnchor, K2FRAME **appRetFrame, K2RECT *apRetFrameRect, K2RECT *apRootRect);

//
//------------------------------------------------------
//

#ifdef __cplusplus
}
#endif

#endif