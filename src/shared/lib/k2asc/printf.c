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
#include <lib/k2asc.h>

typedef enum _PRINTFSTATE
{
    PFS_Reset,
    PFS_Normal,
    PFS_Percent,
    PFS_Flags,
    PFS_Width_Init,
    PFS_Width,
    PFS_Precision_Init,
    PFS_Precision,
    PFS_Precision_Size,
    PFS_Type
} PRINTFSTATE;

UINT_PTR
K2ASC_Emitf(
    K2ASC_Emitter_FnPtr afEmitter,
    void *              apContext,
    UINT_PTR            aMaxLen,
    char const *        apFormat,
    VALIST              aVarList
    )
{
    UINT32   state;
    UINT32   numOut;
    INT32    i,j,k;
    UINT_PTR w;
    UINT32   ui, uk, ul;
    UINT32   c_minus;
    UINT32   c_plus;
    UINT32   c_zero;
    UINT32   c_blank;
    UINT32   c_width;
    UINT32   c_type;
    UINT32   c_hash;
    UINT32   c_precision;
    char    *pSrc;
    char     outBuf[32];
    UINT32   *pOut;

    if (afEmitter == NULL)
        return 0;

    state = PFS_Reset;
    numOut = 0;
    ui = uk = 0;
    c_minus = 0;
    c_plus = 0;
    c_zero = 0;
    c_blank = 0;
    c_width = 0;
    c_type = 0;
    c_hash = 0;
    c_precision = (UINT32)-1;

    do {
        i = (int)*apFormat;
        apFormat++;
        switch (state)
        {
        case PFS_Reset:
            c_minus = 0;
            c_plus = 0;
            c_zero = 0;
            c_blank = 0;
            c_width = 0;
            c_type = 0;
            c_hash = 0;
            c_precision = (UINT32)-1;
            state = PFS_Normal;
            /* fall thru to PFS_Normal */
        case PFS_Normal:
            if (i!='%')
            {
                afEmitter(apContext,(char)i);
                numOut++;
                aMaxLen--;
            }
            else
                state = PFS_Percent;
            break;
        case PFS_Percent:
            if (i=='%')
            {
                state = PFS_Reset;
                afEmitter(apContext,(char)i);
                numOut++;
                aMaxLen--;
                break;
            }
            state = PFS_Flags;
            /* fall thru to PFS_Flags */
        case PFS_Flags:
            if (i=='-')
            {
                if (c_minus)
                    state = PFS_Reset;
                else
                    c_minus = 1;
            }
            else if (i=='+')
            {
                if (c_plus)
                    state = PFS_Reset;
                else
                    c_plus = 1;
            }
            else if (i=='0')
            {
                if (c_zero)
                    state = PFS_Reset;
                else
                    c_zero = 1;
            }
            else if (i==' ')
            {
                if (c_blank)
                    state = PFS_Reset;
                else    
                    c_blank = 1;
            }
            else if (i=='#')
            {
                if (c_hash)
                    state = PFS_Reset;
                else
                    c_hash = 1;
            }
            else
                state = PFS_Width_Init;
            if (state==PFS_Flags)
                break;
            /* fall thru to width init */
        case PFS_Width_Init:
            if (i=='*')
            {
                c_width = K2_VAARG(aVarList,UINT32);
                state = PFS_Precision_Init;
                break;
            }
            /* fall thru to width */               
        case PFS_Width:
            j = i - (int)'0';
            if ((j>=0) && (j<10))
            {
                c_width = (c_width * 10) + j;
                break;
            }
            state = PFS_Precision_Init;
            /* fall thru to PFS_Precision_Init */
        case PFS_Precision_Init:
            if (i=='.')
            {
                state = PFS_Precision;
                break;
            }
            else
                state = PFS_Type;
        case PFS_Type:
            /* ready to go */
            state = PFS_Reset;
            c_type = i;
            if (c_type=='c')
            {
                /* single character */
                c_type = K2_VAARG(aVarList,int);  /* char promoted to int thru vararg */
                if (c_precision == 0)
                    break;
                j = (int)c_width - 1;
                if (!c_minus)
                {
                    /* right justified */
                    while (j-->0)
                    {
                        afEmitter(apContext,' ');
                        numOut++;
                        if ((--aMaxLen) == 0)
                            break;
                    }
                    if (aMaxLen == 0)
                        break;
                }
                afEmitter(apContext,(char)c_type);
                numOut++;
                if ((--aMaxLen) == 0)
                    break;
                /* check for left justified */
                while (j-->0)
                {
                    afEmitter(apContext,' ');
                    numOut++;
                    if ((--aMaxLen) == 0)
                        break;
                }
                if (aMaxLen == 0)
                    break;
            }
            else if ((c_type=='d') || 
                     (c_type=='i') ||
                     (c_type=='o') ||   /* signed octal integer */
                     (c_type=='u') ||   /* unsigned decimal integer */
                     (c_type=='x') ||   /* unsigned hexadecimal integer, using "abcdef" */
                     (c_type=='p') ||   /* pointer */
                     (c_type=='X'))     /* unsigned hexadecimal integer, using "ABCDEF" */
            {
                /* integer of some sort */
                j = i = K2_VAARG(aVarList,int);
                if ((c_type!='d') && (c_type!='i') && (c_type!='o'))
                    i = (((i)<0)?-i:i); /* for sign checks */
                ui = (UINT32)i;
                if (c_type=='p')
                {
                    /* force to unsigned minimum 8-character width hex with leading zeros and 0x */
                    c_type = 'x';
                    if (c_plus)
                    {
                        c_plus = 0;
                        c_blank = 1;
                    }
                    if (c_width<8)
                        c_width = 8;
                    c_zero = 1;
                    c_hash = 1;
                }
                /* calculate necessary width of source number, including (if necessary) + or - */
                if (((i<0) || (c_blank) || (c_plus)) && ((c_type!='x') && (c_type!='X')))
                    w = 1;  /* sign */
                else
                    w = 0;
                if (c_type=='o')
                {
                    if (c_hash)
                        w++;    /* leading zero */
                    k = i;
                    do {
                        k /= 8;
                        w++;
                    } while (k);
                }
                else if ((c_type=='x') || (c_type=='X'))
                {
                    if (j<0)
                        uk = (~ui)+1;
                    else
                        uk = ui;
                    do {
                        uk /= 16;
                        w++;
                    } while (uk);
                }
                else
                {
                    k = i; 
                    do {
                        k /= 10;
                        w++;
                    } while (k);
                }
                k = (int)c_width - (int)w;
                /* k is extra padding that is needed in spaces or zeros */
                if (!c_minus)
                {
                    /* right justified - check for space or zero padding, and for hash */
                    if (!c_zero)
                    {
                        while (k-->0)
                        {
                            afEmitter(apContext,' ');
                            numOut++;
                            if ((--aMaxLen) == 0)
                                break;
                        }
                        if (aMaxLen == 0)
                            break;
                    }
                }
                if (((i<0) || (c_blank) || (c_plus)) && ((c_type!='x') && (c_type!='X')))
                {
                    /* output sign character or blank */
                    if (i < 0)
                    {
                        afEmitter(apContext, '-');
                        ui = (UINT32)(-i);
                    }
                    else if (c_plus)
                        afEmitter(apContext,'+');
                    else
                        afEmitter(apContext,' ');
                    numOut++;
                    if ((--aMaxLen) == 0)
                        break;
                }
                if (c_type=='o')
                {
                    if (c_hash)
                    {
                        afEmitter(apContext,'0');
                        numOut++;
                        if ((--aMaxLen) == 0)
                            break;
                    }
                }
                if ((!c_minus) && (c_zero))
                {
                    while (k-->0)
                    {
                        afEmitter(apContext,'0');
                        numOut++;
                        if ((--aMaxLen) == 0)
                            break;
                    }
                    if (aMaxLen == 0)
                        break;
                }

                /* output characters for number from left to right */
                if (c_type=='o')
                {
                    /* octal */
                    ul = 8;
                    if (j<0)
                        ui = (~ui)+1;  /* use two's complement representation */
                }
                else if ((c_type=='x') || (c_type=='X'))
                {
                    /* hex */
                    ul = 16;
                    if (j<0)
                        ui = (~ui)+1;  /* use two's complement representation */
                }
                else
                {
                    /* decimal */
                    ul = 10;
                }

                /* load stack */
                pSrc = outBuf;
                while (c_precision--)
                {
                    uk = ui % ul;
                    ui = ui / ul;
                    if (uk<10)
                        *(pSrc++) = '0'+((char)uk);
                    else
                        *(pSrc++) = (((char)c_type) - ('x'-'a')) + ((char)(uk-10));
                    if (ui == 0)
                        break;
                }

                /* output digits */
                while ((pSrc--)!=outBuf)
                {
                    afEmitter(apContext,*pSrc);
                    numOut++;
                    if ((--aMaxLen) == 0)
                        break;
                }

                if (aMaxLen == 0)
                    break;

                if (c_minus)
                {
                    /* left justify with spaces - zero is ignored in this case */
                    while (k-->0)
                    {
                        afEmitter(apContext,' ');
                        numOut++;
                        if ((--aMaxLen) == 0)
                            break;
                    }
                    if (aMaxLen == 0)
                        break;
                }
            }
            else if (c_type=='s')
            {
                /* string */
                pSrc = K2_VAARG(aVarList,char *);
                if (c_precision == 0)
                    break;
                w = K2ASC_Len(pSrc);
                if (c_precision<w)
                    w = c_precision;
                if (w>aMaxLen)
                    w = aMaxLen;
                j = (int)c_width - (int)w;
                if (!c_minus)
                {
                    /* right justified */
                    while (j-->0)
                    {
                        afEmitter(apContext,' ');
                        numOut++;
                        if ((--aMaxLen) == 0)
                            break;
                    }
                    if (aMaxLen == 0)
                        break;
                }
                ui = 0;
                if (w)
                {
                    while ((*pSrc) && (w--))
                    {
                        afEmitter(apContext,*(pSrc++));
                        ui++;
                    }
                    if (aMaxLen == 0)
                        break;
                }
                numOut += ui;
                aMaxLen -= ui;
                if (aMaxLen == 0)
                    break;
                /* left justified */
                while (j-->0)
                {
                    afEmitter(apContext,' ');
                    numOut++;
                    if ((--aMaxLen) == 0)
                        break;
                }
                if (aMaxLen == 0)
                    break;
            }
            else if (c_type=='n')
            {
                /* return # of characters output to this point - ignore all formatting goo */
                pOut = K2_VAARG(aVarList,UINT32 *);
                *pOut = numOut;
            }
            /* else unrecognized (and therefore ignored) formatting character */
            break;
        case PFS_Precision:
            if (i=='*')
            {
                c_precision = K2_VAARG(aVarList,UINT32);
                state = PFS_Type;
                break;
            }
            else
                c_precision = 0;
            state = PFS_Precision_Size;
        case PFS_Precision_Size:
            j = i - (int)'0';
            if ((j>=0) && (j<10))
            {
                c_precision = (c_precision * 10) + j;
                break;
            }
            state = PFS_Type;
            apFormat--;  /* whoa. back up and go to type */
            break;
        default:
            state = PFS_Reset;
        }
    } while ((*apFormat) && (aMaxLen));

    /* put on null terminator, but do not include it in output count */
//    if (aMaxLen)
//        afEmitter(apContext,0);

    return numOut;
}

static 
void 
sgEmit(
    void *  pContext, 
    char    ch
    )
{
    **((char **)pContext) = ch;
    (*((char **)pContext))++;
}

UINT_PTR
K2ASC_Printf(
    char *          apOutput,
    char const *    apFormat,
    ...
    )
{
    VALIST      vlist;
    UINT_PTR    ret;

    K2_VASTART(vlist,apFormat);
    ret = K2ASC_Emitf(sgEmit, &apOutput, (UINT_PTR)-1, apFormat, vlist);
    K2_VAEND(vlist);
    // null terminate
    *apOutput = 0;
    return ret;
}

UINT_PTR
K2ASC_PrintfLen(
    char *          apOutput,
    UINT_PTR        aMaxLen,
    char const *    apFormat,
    ...
    )
{
    VALIST      vlist;
    UINT_PTR    ret;

    K2_VASTART(vlist,apFormat);
    ret = K2ASC_Emitf(sgEmit, &apOutput, aMaxLen, apFormat, vlist);
    K2_VAEND(vlist);
    if ((aMaxLen - ret) > 0)
    {
        // null terminate
        *apOutput = 0;
    }
    return ret;
}

UINT_PTR
K2ASC_PrintfVarLen(
    char *          apOutput,
    UINT_PTR        aMaxLen,
    char const *    apFormat,
    VALIST          aVarList
    )
{
    UINT_PTR result;
    
    result = K2ASC_Emitf(sgEmit, &apOutput, aMaxLen, apFormat, aVarList);
    if ((aMaxLen - result) > 0)
    {
        // null terminate
        *apOutput = 0;
    }
    return result;
}


