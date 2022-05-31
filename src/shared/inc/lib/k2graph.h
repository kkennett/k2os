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
#ifndef __K2GRAPH_H
#define __K2GRAPH_H

#include <k2systype.h>
#include <lib/k2mem.h>
#include <lib/k2frame.h>

//
//------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _K2PIXFORMAT K2PIXFORMAT;
enum _K2PIXFORMAT
{
    // byte addresses are right to left
    K2PXF_Unknown     = 0,
    K2PXF_15_RGB      = 0x000F,  /* (15) .rrrrrgggggbbbbb - 555 rgb */
    K2PXF_15_ARGB     = 0x010F,  /*      arrrrrgggggbbbbb - 1555 argb */
    K2PXF_15_BGR      = 0x020F,  /*      .bbbbbgggggrrrrr - 555 bgr */
    K2PXF_15_ABGR     = 0x030F,  /*      abbbbbgggggrrrrr - 1555 abgr */
    K2PXF_16_RGB      = 0x0010,  /* (16) rrrrrggggggbbbbb - 565 rgb */
    K2PXF_16_ARGB     = 0x0110,  /*      aaaarrrrggggbbbb - 4444 argb */
    K2PXF_16_BGR      = 0x0210,  /*      bbbbbggggggrrrrr - 565 bgr */
    K2PXF_16_BGRA     = 0x0310,  /*      bbbbggggrrrraaaa - 4444 bgra */
    K2PXF_16_I        = 0x2010,  /*      16-bit integer */
    K2PXF_32_RGB      = 0x0020,  /* (32) ........rrrrrrrrggggggggbbbbbbbb - .888 rgb */
    K2PXF_32_ARGB     = 0x0120,  /*      aaaaaaaarrrrrrrrggggggggbbbbbbbb - 8888 argb */
    K2PXF_32_BGR      = 0x0220,  /*      ........bbbbbbbbggggggggrrrrrrrr - .888 bgr */
    K2PXF_32_BGRA     = 0x0320,  /*      bbbbbbbbggggggggrrrrrrrraaaaaaaa - 8888 bgra */
};

#define K2PXF_MakeRGB32(r,g,b)        (0xFF000000|((((UINT32)r)&0xFF)<<16)|((((UINT32)g)&0xFF)<<8)|(((UINT32)b)&0xFF))
#define K2PXF_MakeARGB32(a,r,g,b)     (((((UINT32)a)&0xFF)<<16)|((((UINT32)r)&0xFF)<<16)|((((UINT32)g)&0xFF)<<8)|(((UINT32)b)&0xFF))
#define K2PXF_BitsPerPixel(pxf)     ((pxf)&0xFF)

UINT32 K2PXF_CalcLineBytes(K2PIXFORMAT aPxf, UINT32 aWidth);
UINT32 K2PXF_CalcBytes(UINT32 aWidth, UINT32 aHeight, K2PIXFORMAT aPxf);
UINT32 K2PXF_RasterColorToArgb(K2PIXFORMAT aPxf, UINT32 aRasterColor);
UINT32 K2PXF_ArgbToRasterColor(K2PIXFORMAT aPxf, UINT32 aPackedArgb);
UINT32 K2PXF_Convert(K2PIXFORMAT aSrcPxf, UINT32 aSrcPixel, K2PIXFORMAT aDstPxf);

typedef struct _K2RASTERPROP K2RASTERPROP;
struct _K2RASTERPROP
{
    UINT32  mWidth;
    UINT32  mHeight;
    UINT32  mPxf;
    UINT32  mLineBytes;
};

typedef struct _K2RASTER K2RASTER;
struct _K2RASTER
{
    K2RASTERPROP    Prop;
    UINT8 *         mpPixelBufferTopLeft;
};

void K2RASTER_Init(K2RASTER *apRaster, K2RASTERPROP const *apProp, void *apPixelBufferTopLeft);
void K2RASTER_Fill(K2RASTER *apRaster, K2RECT const *apRect, UINT32 aRasterColor);
void K2RASTER_MoveRect(K2RASTER * apRaster, K2RECT const *apRect, int aMoveX, int aMoveY);
void K2RASTER_Blit(K2RASTER *apSrcRaster, K2RECT const *apSrcRect, K2RASTER *apDstRaster, int aDstLeft, int aDstTop);

#define K2RASTER_Clear(pr, rc)      K2RASTER_Fill(pr, NULL, rc)
#define K2RASTER_ClearRGB(pr, rgb)  K2RASTER_Fill(pr, NULL, K2PXF_Convert(K2XPF_32_RGB, rgb), pr->Prop.mPxf)

typedef struct _K2BITMAP K2BITMAP;
struct _K2BITMAP
{
    K2RASTER *  mpRaster;
    K2RECT      Rect;
};

void K2BITMAP_Init(K2BITMAP *apBitmap, K2RASTER *apRaster, K2RECT const *apRect);
void K2BITMAP_BlitToRaster(K2BITMAP const *apBitmap, K2RASTER *apDstRaster, int aDstLeft, int aDstTop);
void K2BITMAP_FillRect(K2BITMAP *apBitmap, K2RECT const *apRect, UINT32 aRasterColor);
void K2BITMAP_Fill(K2BITMAP *apBitmap, int aLeft, int aTop, unsigned int aWidth, unsigned int aHeight, UINT32 aRasterColor);

//
//------------------------------------------------------
//

#ifdef __cplusplus
}
#endif

#endif