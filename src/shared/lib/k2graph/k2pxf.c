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

UINT32
K2PXF_CalcLineBytes(
    K2PIXFORMAT aPxf,
    UINT32      aWidth
    )
{
    return ((((K2PXF_BitsPerPixel(aPxf) + 7) >> 3) * aWidth) + 3) & ~3;
}

UINT32
K2PXF_CalcBytes(
    UINT32      aWidth,
    UINT32      aHeight,
    K2PIXFORMAT aPxf
)
{
    return K2PXF_CalcLineBytes(aPxf, aWidth) * aHeight;
}

UINT32 
K2PXF_RasterColorToArgb(
    K2PIXFORMAT aPxf, 
    UINT32      aRasterColor
)
{
    unsigned int    bpp;
    UINT32          ret;

    bpp = aPxf & 0xFF;

    if (bpp == 15)
    {
        /* red */
        if ((aPxf & 0xFF00) < 0x0200)
            ret = ((aRasterColor & 0x7C00) << 9) | ((aRasterColor & 0x7000) << 4);
        else
            ret = ((aRasterColor & 0x7C00) >> 7) | ((aRasterColor & 0x7000) >> 12);
        /* green */
        ret |= ((aRasterColor & 0x03E0) << 6) | ((aRasterColor & 0x0380) << 1);
        /* blue */
        if ((aPxf & 0xFF00) < 0x0200)
            ret |= ((aRasterColor & 0x001F) << 3) | ((aRasterColor & 0x001C) >> 2);
        else
            ret |= ((aRasterColor & 0x001F) << 19) | ((aRasterColor & 0x001C) << 14);
        /* alpha */
        if (aPxf & 0x0100)
        {
            /* source format has alpha */
            if (aRasterColor & 0x8000)
                ret |= 0xFF000000;
        }
        else
        {
            /* source format didn't have alpha - set alpha to full opaque */
            ret |= 0xFF000000;
        }
        return ret;
    }

    if (bpp == 16)
    {
        if (aPxf & 0x0100)
        {
            /* 4444 format */
            if (aPxf & 0x0200)
            {
                /* reversed */
                /* alpha */
                ret = ((aRasterColor & 0x000F) << 28) | ((aRasterColor & 0x000F) << 24);
                /* red */
                ret |= ((aRasterColor & 0x00F0) << 16) | ((aRasterColor & 0x00F0) << 12);
                /* green */
                ret |= ((aRasterColor & 0x0F00) << 4) | (aRasterColor & 0x0F00);
                /* blue */
                return ret | ((aRasterColor & 0xF000) >> 8) | ((aRasterColor & 0xF000) >> 12);
            }
            /* alpha */
            ret = ((aRasterColor & 0xF000) << 16) | ((aRasterColor & 0xF000) << 12);
            /* red */
            ret |= ((aRasterColor & 0x0F00) << 12) | ((aRasterColor & 0x0F00) << 8);
            /* green */
            ret |= ((aRasterColor & 0x00F0) << 8) | ((aRasterColor & 0x00F0) << 4);
            /* blue */
            return ret | ((aRasterColor & 0x000F) << 4) | (aRasterColor & 0x000F);
        }
        /* 565 format */
        /* red */
        if (aPxf & 0x0200)
            ret = ((aRasterColor & 0x001F) << 19) | ((aRasterColor & 0x001C) << 14);
        else
            ret = ((aRasterColor & 0xF800) << 8) | ((aRasterColor & 0xE000) << 3);
        /* green */
        ret |= ((aRasterColor & 0x07E0) << 5) | ((aRasterColor & 0x0600) >> 1);
        /* blue */
        if (aPxf & 0x0200)
            ret |= ((aRasterColor & 0xF800) >> 8) | ((aRasterColor & 0xE000) >> 13);
        else
            ret |= ((aRasterColor & 0x001F) << 3) | ((aRasterColor & 0x001C) >> 2);
        /* no alpha in source format - set to full opaque */
        return ret | 0xFF000000;
    }

    if (bpp != 32)
        return 0;

    if ((aPxf & 0xFF00) >= 0x0200)
    {
        /* reverse r and b components */
        aRasterColor = (aRasterColor & 0xFF00FF00) |
            ((aRasterColor & 0x00FF0000) >> 16) |
            ((aRasterColor & 0x000000FF) << 16);
    }

    if (!(aPxf & 0x0100))
    {
        /* no alpha in source - set to full opaque */
        aRasterColor |= 0xFF000000;
    }

    return aRasterColor;
}

UINT32 
K2PXF_ArgbToRasterColor(
    K2PIXFORMAT aPxf, 
    UINT32      aPackedArgb
)
{
    unsigned int bpp;
    
    bpp = aPxf & 0xFF;

    if (bpp == 15)
    {
        return ((aPxf & 0x0100)?((aPackedArgb & 0x80000000) >> 16):0) | /* alpha */
            ((aPackedArgb & 0x00F80000) >> ((aPxf & 0x0200)?19:9)) | /* red */
            ((aPackedArgb & 0x0000F800) >> 6) |                      /* green */
            ((aPxf & 0x0200)?((aPackedArgb & 0x000000F8) << 7):((aPackedArgb & 0x000000F8) >> 3)); /* blue */
    }
    if (bpp == 16)
    {
        if (aPxf & 0x0100)
        {
            /* 4444 format */
            if (aPxf & 0x0200)
                return ((aPackedArgb & 0xF0000000) >> 28) |
                ((aPackedArgb & 0x00F00000) >> 16) |
                ((aPackedArgb & 0x0000F000) >> 4) |
                ((aPackedArgb & 0x000000F0) << 8);
            return ((aPackedArgb & 0xF0000000) >> 16) |
                ((aPackedArgb & 0x00F00000) >> 12) |
                ((aPackedArgb & 0x0000F000) >> 8) |
                ((aPackedArgb & 0x000000F0) >> 4);
        }
        /* 565 format */
        if (aPxf & 0x0200)
            return ((aPackedArgb & 0x00F80000) >> 19) |
            ((aPackedArgb & 0x0000FC00) >> 5) |
            ((aPackedArgb & 0x000000F8) << 7);
        return ((aPackedArgb & 0x00F80000) >> 8) |
            ((aPackedArgb & 0x0000FC00) >> 5) |
            ((aPackedArgb & 0x000000F8) >> 3);
    }

    if (bpp != 32)
        return 0;
    if (!(aPxf & 0x0100))           /* no alpha component */
        aPackedArgb &= 0x00FFFFFF;
    if (aPxf & 0x0200)              /* bgr instead of rgb */
        aPackedArgb = (aPackedArgb & 0xFF00FF00) |
        ((aPackedArgb & 0x00FF0000) >> 16) |
        ((aPackedArgb & 0x000000FF) << 16);
    return aPackedArgb;
}

UINT32 
K2PXF_Convert(
    K2PIXFORMAT aSrcPxf,
    UINT32      aSrcPixel,
    K2PIXFORMAT aDstPxf
)
{
    if (aSrcPxf == aDstPxf)
        return aSrcPixel;
    return K2PXF_ArgbToRasterColor(aDstPxf, K2PXF_RasterColorToArgb(aSrcPxf, aSrcPixel));
}

void 
K2PXF_MixApplyArgb(
    UINT32 *apTarget,
    UINT32  aSrc
)
{
    UINT32 dst;
    UINT32 t;
    UINT32 t2;
    UINT32 sa;
    UINT32 oneminussa;

    sa = ((aSrc >> 24) + (aSrc >> 30)) & 0x100;
    if (0 == sa)
    {
        // source completely transparent, no-op
        return;
    }
    if (0xFF == sa)
    {
        *apTarget = aSrc;
        return;
    }

    dst = (*apTarget);
    t = (dst >> 8) & 0xFF;
    dst &= 0x00FF00FF;
    oneminussa = 0xFF - (aSrc >> 24);
    oneminussa = (oneminussa + (oneminussa >> 6)) & 0x100;
    dst = ((dst * oneminussa) >> 8) & 0x00FF00FF;

    t2 = (aSrc >> 8) & 0xFF;    // t2 = 0000gg
    aSrc &= 0x00FF00FF;
    aSrc = ((aSrc * sa) >> 8) & 0x00FF00FF;

    *apTarget = ((*apTarget) & 0xFF000000) |
        dst |
        ((t * oneminussa) & 0x0000FF00) |
        aSrc |
        ((t2 * sa) & 0x0000FF00);
}

void
K2PXFI_ConvertPixels(
    UINT_PTR        aNumPix,
    UINT8 const *   apSrc,
    K2PIXFORMAT     aSrcPxf,
    int             aSrcInc,
    UINT8 *         apDst,
    K2PIXFORMAT     aDstPxf
)
{
    UINT32 sargb;
    UINT32 dargb;

    /* special case of no raster color conversion only allowed if no alpha in pix format */
    if ((aSrcPxf == aDstPxf) && 
        (0 == (aSrcPxf & 0x0100)))
    {
        if ((aSrcPxf & 0xFF) < 32)
        {
            /* do 16-bit line copy from possibly staggered source */
            do {
                *((UINT16 *)apDst) = *((UINT16 const *)apSrc);
                apDst += sizeof(UINT16);
                apSrc += aSrcInc;
            } while (--aNumPix);
        }
        else
        {
            /* do 32-bit line copy from possibly staggered source */
            do {
                *((UINT32 *)apDst) = *((UINT32 const *)apSrc);
                apDst += sizeof(UINT32);
                apSrc += aSrcInc;
            } while (--aNumPix);
        }
        return;
    }

    /* do pixel-by-pixel format conversion due to differing pixel formats
       or due to alpha being in a pixel format. (slow) */
    if ((aSrcPxf & 0xFF) < 32)
    {
        /* 15/16-bit source */
        if ((aDstPxf & 0xFF) < 32)
        {
            /* 15/16-bit target */
            if (aSrcPxf & 0x0100)
            {
                /* 15->15/16 */
                if ((aSrcPxf & 0xFF) == 15)
                {
                    /* single-bit alpha in 1.15 source */
                    do {
                        sargb = *((UINT16 const *)apSrc);
                        if (sargb & 0x8000)
                            *((UINT16 *)apDst) = (UINT16)K2PXF_Convert(aSrcPxf, sargb, aDstPxf);
                        apDst += sizeof(UINT16);
                        apSrc += aSrcInc;
                    } while (--aNumPix);
                }
                else
                {
                    /* 16->32 - must do full argb mix per pixel */
                    do {
                        sargb = K2PXF_RasterColorToArgb(aSrcPxf, *((UINT16 const *)apSrc));
                        dargb = K2PXF_RasterColorToArgb(aDstPxf, *((UINT16 *)apDst));
                        K2PXF_MixApplyArgb(&dargb, sargb);
                        *((UINT16 *)apDst) = (UINT16)K2PXF_ArgbToRasterColor(aDstPxf, dargb);
                        apDst += sizeof(UINT16);
                        apSrc += aSrcInc;
                    } while (--aNumPix);
                }
            }
            else
            {
                /* 15/16(noalpha)->16 */
                do {
                    *((UINT16 *)apDst) = (UINT16)K2PXF_Convert(aSrcPxf, *((UINT16 const *)apSrc), aDstPxf);
                    apDst += sizeof(UINT16);
                    apSrc += aSrcInc;
                } while (--aNumPix);
            }
        }
        else
        {
            /* 32-bit target */
            if (aSrcPxf & 0x0100)
            {
                /* 15/16->32 */
                if ((aSrcPxf & 0xFF) == 15)
                {
                    /* 15->32, single-bit alpha in 1.15 source */
                    do {
                        sargb = *((UINT16 const *)apSrc);
                        if (sargb & 0x8000)
                            *((UINT32 *)apDst) = (UINT32)K2PXF_Convert(aSrcPxf, sargb, aDstPxf);
                        apDst += sizeof(UINT32);
                        apSrc += aSrcInc;
                    } while (--aNumPix);
                }
                else
                {
                    /* 16->32, must do full argb mix per pixel */
                    do {
                        sargb = K2PXF_RasterColorToArgb(aSrcPxf, *((UINT16 const *)apSrc));
                        dargb = K2PXF_RasterColorToArgb(aDstPxf, *((UINT32 *)apDst));
                        K2PXF_MixApplyArgb(&dargb, sargb);
                        *((UINT32 *)apDst) = (UINT32)K2PXF_ArgbToRasterColor(aDstPxf, dargb);
                        apDst += sizeof(UINT32);
                        apSrc += aSrcInc;
                    } while (--aNumPix);
                }
            }
            else
            {
                /* 15/16(noalpha)->32 */
                do {
                    *((UINT32 *)apDst) = (UINT32)K2PXF_Convert(aSrcPxf, *((UINT16 const *)apSrc), aDstPxf);
                    apDst += sizeof(UINT32);
                    apSrc += aSrcInc;
                } while (--aNumPix);
            }
        }
    }
    else
    {
        /* 32-bit source */
        if ((aDstPxf & 0xFF) < 32)
        {
            /* 32->15/16 */
            if (aSrcPxf & 0x0100)
            {
                /* 32(alpha)->16 */
                do {
                    sargb = K2PXF_RasterColorToArgb(aSrcPxf, *((UINT32 const *)apSrc));
                    dargb = K2PXF_RasterColorToArgb(aDstPxf, *((UINT16 *)apDst));
                    K2PXF_MixApplyArgb(&dargb, sargb);
                    *((UINT16 *)apDst) = K2PXF_ArgbToRasterColor(aDstPxf, dargb);
                    apDst += sizeof(UINT16);
                    apSrc += aSrcInc;
                } while (--aNumPix);
            }
            else
            {
                /* 32(noalpha)->16 */
                do {
                    *((UINT16 *)apDst) = (UINT32)K2PXF_Convert(aSrcPxf, *((UINT32 const *)apSrc), aDstPxf);
                    apDst += sizeof(UINT16);
                    apSrc += aSrcInc;
                } while (--aNumPix);
            }
        }
        else
        {
            /* 32->32 */
            if (aSrcPxf & 0x0100)
            {
                /* 32(alpha)->32 */
                do {
                    sargb = K2PXF_RasterColorToArgb(aSrcPxf, *((UINT32 const *)apSrc));
                    dargb = K2PXF_RasterColorToArgb(aDstPxf, *((UINT32 *)apDst));
                    K2PXF_MixApplyArgb(&dargb, sargb);
                    *((UINT32 *)apDst) = K2PXF_ArgbToRasterColor(aDstPxf, dargb);
                    apDst += sizeof(UINT32);
                    apSrc += aSrcInc;
                } while (--aNumPix);
            }
            else
            {
                /* 32(noalpha)->32 */
                do {
                    *((UINT32 *)apDst) = (UINT32)K2PXF_Convert(aSrcPxf, *((UINT32 const *)apSrc), aDstPxf);
                    apDst += sizeof(UINT32);
                    apSrc += aSrcInc;
                } while (--aNumPix);
            }
        }
    }
}

