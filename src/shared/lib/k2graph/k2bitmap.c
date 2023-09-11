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
#include <lib/k2graph.h>

void 
K2BITMAP_Init(
    K2BITMAP *      apBitmap,
    K2RASTER *      apRaster,
    K2RECT const *  apRect
)
{
    K2RECT clip;

    K2_ASSERT(NULL != apBitmap);
    K2_ASSERT(NULL != apRaster);
    if ((0 == apRaster->Prop.mWidth) ||
        (0 == apRaster->Prop.mHeight))
        return;
    K2_ASSERT(0 != K2PXF_ArgbToRasterColor(apRaster->Prop.mPxf, 0xFFFFFFFF));
    K2_ASSERT(apRaster->Prop.mLineBytes >= K2PXF_CalcLineBytes(apRaster->Prop.mPxf, apRaster->Prop.mWidth));
    K2MEM_Zero(apBitmap, sizeof(K2BITMAP));
    apBitmap->mpRaster = apRaster;
    apBitmap->Rect.TopLeft.mX = 0;
    apBitmap->Rect.TopLeft.mY = 0;
    apBitmap->Rect.BottomRight.mX = apRaster->Prop.mWidth;
    apBitmap->Rect.BottomRight.mY = apRaster->Prop.mHeight;
    if (NULL != apRect)
    {
        //
        // rect must completely intersect on area of raster
        //
        K2_ASSERT(K2RECT_IsValidAndHasArea(apRect));
        K2MEM_Copy(&clip, apRect, sizeof(K2RECT));
        K2_ASSERT(K2RECT_Clip(&apBitmap->Rect, &clip));
        K2_ASSERT(0 == K2MEM_Compare(&clip, apRect, sizeof(K2RECT)));
        K2MEM_Copy(&apBitmap->Rect, apRect, sizeof(K2RECT));
    }
}

void 
K2BITMAP_BlitToRaster(
    K2BITMAP const *apBitmap,
    K2RASTER *      apDstRaster,
    int             aDstLeft,
    int             aDstTop
)
{
    K2RASTER_Blit(apBitmap->mpRaster, &apBitmap->Rect, apDstRaster, aDstLeft, aDstTop);
}

void 
K2BITMAP_FillRect(
    K2BITMAP *      apBitmap,
    K2RECT const *  apRect,
    UINT32          aRasterColor
)
{
    K2RECT base;
    K2RECT clip;

    base.TopLeft.mX = base.TopLeft.mY = 0;
    base.BottomRight.mX = (apBitmap->Rect.BottomRight.mX - apBitmap->Rect.TopLeft.mX);
    base.BottomRight.mY = (apBitmap->Rect.BottomRight.mY = apBitmap->Rect.TopLeft.mY);
    if (NULL == apRect)
    {
        K2MEM_Copy(&clip, &base, sizeof(K2RECT));
    }
    else 
    {
        if (!K2RECT_IsValidAndHasArea(apRect))
            return;
        K2MEM_Copy(&clip, apRect, sizeof(K2RECT));
        if (!K2RECT_Clip(&base, &clip))
            return;
    }
    K2RECT_Offset(&clip, apBitmap->Rect.TopLeft.mX, apBitmap->Rect.TopLeft.mY);
    K2RASTER_Fill(apBitmap->mpRaster, &clip, aRasterColor);
}

void 
K2BITMAP_Fill(
    K2BITMAP *      apBitmap,
    int             aLeft,
    int             aTop,
    unsigned int    aWidth,
    unsigned int    aHeight,
    UINT32          aRasterColor
)
{
    K2RECT fillRect;
    K2RECT_FromWH(&fillRect, aLeft, aTop, aWidth, aHeight);
    K2BITMAP_FillRect(apBitmap, &fillRect, aRasterColor);
}

