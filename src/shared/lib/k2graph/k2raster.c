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

void K2PXFI_ConvertPixels(UINT32 aNumPix, UINT8 const *apSrc, K2PIXFORMAT aSrcPxf, int aSrcInc, UINT8 *apDst, K2PIXFORMAT aDstPxf);

void 
K2RASTER_Init(
    K2RASTER *          apRaster,
    K2RASTERPROP const *apProp,
    void *              apPixelBufferTopLeft
)
{
    UINT_PTR chkAddr;

    K2_ASSERT(NULL != apRaster);
    K2_ASSERT(NULL != apProp);
    K2_ASSERT(apProp->mHeight > 0);
    K2_ASSERT(apProp->mWidth > 0);
    K2_ASSERT(NULL != apPixelBufferTopLeft);
    K2_ASSERT(0 != K2PXF_ArgbToRasterColor(apProp->mPxf, 0xFFFFFFFF));
    K2_ASSERT(apProp->mLineBytes >= K2PXF_CalcLineBytes(apProp->mPxf, apProp->mWidth));
    K2MEM_Zero(apRaster, sizeof(K2RASTER));
    K2MEM_Copy(&apRaster->Prop, apProp, sizeof(K2RASTER));
    apRaster->mpPixelBufferTopLeft = apPixelBufferTopLeft;
    chkAddr = (UINT_PTR)apPixelBufferTopLeft;
    if (K2PXF_BitsPerPixel(apProp->mPxf) > 16)
    {
        // pixel buffer must be 32-bit aligned
        K2_ASSERT(0 == (chkAddr & 3));
    }
    else
    {
        // pixel buffer must be 16-bit aligned
        K2_ASSERT(0 == (chkAddr & 1));
    }
}

void 
K2RASTER_Fill(
    K2RASTER *      apRaster,
    K2RECT const *  apRect,
    UINT32          aRasterColor
)
{
    K2RECT      base;
    K2RECT      clip;
    UINT8 *     pPix;
    UINT32      bytesPerPixel;
    UINT32      yLeft;
    UINT32      xSet;

    K2_ASSERT(NULL != apRaster);
    if ((0 == apRaster->Prop.mWidth) ||
        (0 == apRaster->Prop.mHeight))
        return;
    K2_ASSERT(0 != K2PXF_ArgbToRasterColor(apRaster->Prop.mPxf, 0xFFFFFFFF));
    K2_ASSERT(apRaster->Prop.mLineBytes >= K2PXF_CalcLineBytes(apRaster->Prop.mPxf, apRaster->Prop.mWidth));

    base.TopLeft.mX = 0;
    base.TopLeft.mY = 0;
    base.BottomRight.mX = apRaster->Prop.mWidth;
    base.BottomRight.mY = apRaster->Prop.mHeight;

    if (NULL == apRect)
    {
        apRect = &base;
    }
    else
    {
        if (!K2RECT_HasArea(apRect))
            return;
    }
    K2MEM_Copy(&clip, apRect, sizeof(K2RECT));
    if (!K2RECT_Clip(&base, &clip))
        return;

    bytesPerPixel = (K2PXF_BitsPerPixel(apRaster->Prop.mPxf) + 7) >> 3;

    pPix = apRaster->mpPixelBufferTopLeft + (clip.TopLeft.mY * apRaster->Prop.mLineBytes) + (clip.TopLeft.mX * bytesPerPixel);

    yLeft = clip.BottomRight.mY - clip.TopLeft.mY;
    K2_ASSERT(0 != yLeft);
    xSet = (clip.BottomRight.mX - clip.TopLeft.mX) * 2;
    K2_ASSERT(0 != xSet);

    if (bytesPerPixel == 2)
    {
        do {
            K2MEM_Set16(pPix, (UINT16)aRasterColor, xSet);
            pPix += apRaster->Prop.mLineBytes;
        } while (0 != --yLeft);
    }
    else
    {
        xSet *= 2;
        do {
            K2MEM_Set32(pPix, (UINT32)aRasterColor, xSet);
            pPix += apRaster->Prop.mLineBytes;
        } while (0 != --yLeft);
    }
}

void
K2RASTERI_Blit(
    K2RASTER *          apDstRaster,
    int                 aDstLeft,
    int                 aDstTop,
    int                 aWidth,
    int                 aHeight,
    K2RASTER const *    apSrcRaster,
    int                 aSrcLeft,
    int                 aSrcTop
)
{
    UINT8 const * pSrc;
    UINT8 * pDst;
    UINT32 bytesPerPixel;

    //
    // src rect entirely intersects src raster
    // dst rect entirely intersects dst raster
    // src raster and dst raster are not the same raster
    //
    bytesPerPixel = (K2PXF_BitsPerPixel(apDstRaster->Prop.mPxf) + 7) >> 3;
    pDst = apDstRaster->mpPixelBufferTopLeft + (apDstRaster->Prop.mLineBytes * aDstTop) + (bytesPerPixel * aDstLeft);
    bytesPerPixel = (K2PXF_BitsPerPixel(apSrcRaster->Prop.mPxf) + 7) >> 3;
    pSrc = apSrcRaster->mpPixelBufferTopLeft + (apSrcRaster->Prop.mLineBytes * aSrcTop) + (bytesPerPixel * aSrcLeft);
    do {
        K2PXFI_ConvertPixels(aWidth, pSrc, apSrcRaster->Prop.mPxf, bytesPerPixel, pDst, apDstRaster->Prop.mPxf);
        pSrc += apSrcRaster->Prop.mLineBytes;
        pDst += apDstRaster->Prop.mLineBytes;
    } while (0 != --aHeight);
}

void 
K2RASTER_Blit(
    K2RASTER *      apSrcRaster,
    K2RECT const *  apSrcRect,
    K2RASTER *      apDstRaster,
    int             aDstLeft,
    int             aDstTop
)
{
    K2RECT  base;
    K2RECT  clip;
    int     width;
    int     height;
    K2RECT  dstRect;
    K2RECT  dstBase;

    K2_ASSERT(NULL != apSrcRaster);
    if ((0 == apSrcRaster->Prop.mWidth) ||
        (0 == apSrcRaster->Prop.mHeight))
        return;
    K2_ASSERT(0 != K2PXF_ArgbToRasterColor(apSrcRaster->Prop.mPxf, 0xFFFFFFFF));
    K2_ASSERT(apSrcRaster->Prop.mLineBytes >= K2PXF_CalcLineBytes(apSrcRaster->Prop.mPxf, apSrcRaster->Prop.mWidth));

    K2_ASSERT(NULL != apDstRaster);
    if ((0 == apDstRaster->Prop.mWidth) ||
        (0 == apDstRaster->Prop.mHeight))
        return;
    K2_ASSERT(0 != K2PXF_ArgbToRasterColor(apDstRaster->Prop.mPxf, 0xFFFFFFFF));
    K2_ASSERT(apDstRaster->Prop.mLineBytes >= K2PXF_CalcLineBytes(apDstRaster->Prop.mPxf, apDstRaster->Prop.mWidth));

    if (apSrcRaster == apDstRaster)
    {
        K2RASTER_MoveRect(apSrcRaster, apSrcRect, aDstLeft - apSrcRect->TopLeft.mX, aDstTop - apSrcRect->TopLeft.mY);
        return;
    }

    //
    // copying pixels from one raster to another
    //
    base.TopLeft.mX = 0;
    base.TopLeft.mY = 0;
    base.BottomRight.mX = apSrcRaster->Prop.mWidth;
    base.BottomRight.mY = apSrcRaster->Prop.mHeight;

    if (NULL == apSrcRect)
    {
        apSrcRect = &base;
    }
    else
    {
        if (!K2RECT_HasArea(apSrcRect))
            return;
    }
    K2MEM_Copy(&clip, apSrcRect, sizeof(K2RECT));
    if (!K2RECT_Clip(&base, &clip))
    {
        // source rect does not intersect at all with raster
        return;
    }
    // test whole source rect must fit inside raster
    K2_ASSERT(0 == K2MEM_Compare(&clip, apSrcRect, sizeof(K2RECT)));

    width = apSrcRect->BottomRight.mX - apSrcRect->TopLeft.mX;
    height = apSrcRect->BottomRight.mY - apSrcRect->TopLeft.mY;

    dstRect.TopLeft.mX = aDstLeft;
    dstRect.TopLeft.mY = aDstTop;
    dstRect.BottomRight.mX = aDstLeft + width;
    dstRect.BottomRight.mY = aDstTop + height;

    dstBase.TopLeft.mX = 0;
    dstBase.TopLeft.mY = 0;
    dstBase.BottomRight.mX = apDstRaster->Prop.mWidth;
    dstBase.BottomRight.mY = apDstRaster->Prop.mHeight;
    if (!K2RECT_Clip(&dstBase, &dstRect))
    {
        // destination rect does not intersect target raster area
        return;
    }

    //
    // dstRect is clipped to base range
    // if top left moved we need to move the source rect
    // top left as well
    //
    K2RASTERI_Blit(
        apDstRaster,
        dstRect.TopLeft.mX,
        dstRect.TopLeft.mY,
        dstRect.BottomRight.mX - dstRect.TopLeft.mX,
        dstRect.BottomRight.mY - dstRect.TopLeft.mY,
        apSrcRaster,
        apSrcRect->TopLeft.mX + (dstRect.TopLeft.mX - aDstLeft),
        apSrcRect->TopLeft.mY + (dstRect.TopLeft.mY - aDstTop)
        );
}

void 
K2RASTER_MoveRect(
    K2RASTER *      apRaster,
    K2RECT const *  apRect,
    int             aMoveX,
    int             aMoveY
)
{
    K2RECT base;
    K2RECT clip;
    K2RECT src;
#if K2_DEBUG
    K2RECT chk;
#endif
    UINT8 * pDst;
    UINT8 const * pSrc;
    UINT32 bytesPerPixel;
    UINT32 width;
    UINT32 height;
    UINT32 pixLineBytes;
    UINT32 xLeft;

    K2_ASSERT(NULL != apRaster);
    if ((0 == apRaster->Prop.mWidth) ||
        (0 == apRaster->Prop.mHeight))
        return;
    K2_ASSERT(0 != K2PXF_ArgbToRasterColor(apRaster->Prop.mPxf, 0xFFFFFFFF));
    K2_ASSERT(apRaster->Prop.mLineBytes >= K2PXF_CalcLineBytes(apRaster->Prop.mPxf, apRaster->Prop.mWidth));

    if ((0 == aMoveX) &&
        (0 == aMoveY))
        return;

    base.TopLeft.mX = 0;
    base.TopLeft.mY = 0;
    base.BottomRight.mX = apRaster->Prop.mWidth;
    base.BottomRight.mY = apRaster->Prop.mHeight;

    if (NULL == apRect)
    {
        apRect = &base;
    }
    else
    {
        if (!K2RECT_HasArea(apRect))
            return;
    }
    K2MEM_Copy(&clip, apRect, sizeof(K2RECT));
    if (!K2RECT_Clip(&base, &clip))
    {
        // source rect does not intersect at all with raster
        return;
    }
    // test whole source rect must fit inside raster
    K2_ASSERT(0 == K2MEM_Compare(&clip, apRect, sizeof(K2RECT)));

    K2RECT_CopyAndOffset(apRect, aMoveX, aMoveY, &clip);
    if (!K2RECT_Clip(&base, &clip))
    {
        // destination rect does not intersect with raster
        return;
    }
    K2RECT_CopyAndOffset(&clip, -aMoveX, -aMoveY, &src);

    //
    // now src is source rect, clip is dest rect
    // same sized rects, for areas entirely within raster area
    //
#if K2_DEBUG
    // sanity
    K2MEM_Copy(&chk, &clip, sizeof(K2RECT));
    K2_ASSERT(K2RECT_Clip(&base, &chk));
    K2_ASSERT(0 == K2MEM_Compare(&chk, &clip, sizeof(K2RECT)));
    K2MEM_Copy(&chk, &src, sizeof(K2RECT));
    K2_ASSERT(K2RECT_Clip(&base, &src));
    K2_ASSERT(0 == K2MEM_Compare(&chk, &src, sizeof(K2RECT)));
#endif

    width = src.BottomRight.mX - src.TopLeft.mX;
    height = src.BottomRight.mY - src.TopLeft.mY;
    bytesPerPixel = (K2PXF_BitsPerPixel(apRaster->Prop.mPxf) + 7) >> 3;
    pSrc = apRaster->mpPixelBufferTopLeft + (src.TopLeft.mY * apRaster->Prop.mLineBytes) + (src.TopLeft.mX * bytesPerPixel);
    pDst = apRaster->mpPixelBufferTopLeft + (clip.TopLeft.mY * apRaster->Prop.mLineBytes) + (clip.TopLeft.mX * bytesPerPixel);

    if (0 == aMoveY)
    {
        if (aMoveX < 0)
        {
            // move left only - copy top down left to right
            pixLineBytes = width * bytesPerPixel;
            do {
                K2MEM_Copy(pDst, pSrc, pixLineBytes);
                pSrc += apRaster->Prop.mLineBytes;
                pDst += apRaster->Prop.mLineBytes;
            } while (0 != --height);
        }
        else
        {
            K2_ASSERT(0 != aMoveX);
            // move right only - copy top down right to left
            pixLineBytes = apRaster->Prop.mLineBytes + (width * bytesPerPixel);
            pSrc += (width - 1) * bytesPerPixel;
            pDst += (width - 1) * bytesPerPixel;
            xLeft = width;
            if (bytesPerPixel == 2)
            {
                do {
                    do {
                        *((UINT16 *)pDst) = *((UINT16 *)pSrc);
                        pDst -= 2;
                        pSrc -= 2;
                    } while (0 != --xLeft);
                    pSrc += pixLineBytes;
                    pDst += pixLineBytes;
                } while (0 != --height);
            }
            else
            {
                do {
                    do {
                        *((UINT32 *)pDst) = *((UINT32 *)pSrc);
                        pDst -= 4;
                        pSrc -= 4;
                    } while (0 != --xLeft);
                    pSrc += pixLineBytes;
                    pDst += pixLineBytes;
                } while (0 != --height);
            }
        }
    }
    else
    {
        pixLineBytes = width * bytesPerPixel;
        if (0 > aMoveY)
        {
            // moving up - copy top down left to right
            do {
                K2MEM_Copy(pDst, pSrc, pixLineBytes);
                pSrc += apRaster->Prop.mLineBytes;
                pDst += apRaster->Prop.mLineBytes;
            } while (0 != --height);
        }
        else 
        {
            // moving down - copy bottom up left to right
            pSrc += (apRaster->Prop.mLineBytes * (height - 1));
            pDst += (apRaster->Prop.mLineBytes * (height - 1));
            do {
                K2MEM_Copy(pDst, pSrc, pixLineBytes);
                pSrc -= apRaster->Prop.mLineBytes;
                pDst -= apRaster->Prop.mLineBytes;
            } while (0 != --height);
        }
    }
}

