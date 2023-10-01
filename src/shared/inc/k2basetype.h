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
#ifndef __K2BASETYPE_H
#define __K2BASETYPE_H

#include "k2basedefs.inc"

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

#if K2_TOOLCHAIN_IS_MS

#define K2_PACKED_PUSH              __pragma(pack(push,1))
#define K2_PACKED_POP               __pragma(pack(pop))
#define K2_PACKED_ATTRIB   
#define K2_ALIGN_ATTRIB(x)          __declspec(align(x))

#if K2_TARGET_ARCH_IS_32BIT
#define K2_CALLCONV_REGS            __fastcall
#else
#define K2_CALLCONV_REGS            
#endif
#define K2_CALLCONV_CALLERCLEANS    __cdecl
#define K2_CALLCONV_CALLEECLEANS    __stdcall

#define K2_CALLCONV_NAKED           __declspec(naked)
#define K2_CALLCONV_NORETURN        __declspec(noreturn)    
#define K2_INLINE                   __inline

#define K2_CpuFullBarrier           _ReadWriteBarrier
    void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)

#define K2_CpuReadBarrier           _ReadBarrier
    void _ReadBarrier(void);
#pragma intrinsic(K2_CpuReadBarrier)

#define K2_CpuWriteBarrier          _WriteBarrier
    void _WriteBarrier(void);
#pragma intrinsic(K2_CpuWriteBarrier)

#define K2_CpuYield                 YieldProcessor
    void YieldProcessor(void);

#define K2_RETURN_ADDRESS           _ReturnAddress()
    void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

#elif K2_TOOLCHAIN_IS_GCC

#define K2_PACKED_PUSH     
#define K2_PACKED_POP      
#define K2_PACKED_ATTRIB            __attribute__((packed))
#define K2_ALIGN_ATTRIB(x)          __attribute__((aligned(x)))
#define K2_INLINE                   __inline__
#define K2_CALLCONV_NORETURN        __attribute__((noreturn))
#define K2_CALLCONV_NAKED           __attribute__((naked))
#define K2_FUNC_MAYBE_UNUSED        __attribute__((unused))

void K2_CpuYield(void);

#if K2_TARGET_ARCH_IS_INTEL

#if K2_TARGET_ARCH_IS_32BIT
#define K2_CALLCONV_REGS            __attribute__((fastcall))
#else
#define K2_CALLCONV_REGS            
#endif
#define K2_CALLCONV_CALLERCLEANS    __attribute__((cdecl))
#define K2_CALLCONV_CALLEECLEANS    __attribute__((stdcall))

static __inline__
    __attribute__((always_inline))
    void K2_CpuFullBarrier(void)
{
    __asm__("mfence\n");
}

static __inline__
    __attribute__((always_inline))
    void K2_CpuReadBarrier(void)
{
    __asm__("lfence\n");
}

static __inline__
    __attribute__((always_inline))
    void K2_CpuWriteBarrier(void)
{
    __asm__("sfence\n");
}

#else

#define K2_CALLCONV_REGS            
#define K2_CALLCONV_CALLERCLEANS    
#define K2_CALLCONV_CALLEECLEANS    

static __inline__
    __attribute__((always_inline))
    void K2_CpuFullBarrier(void)
{
    __asm__("dsb\n");
    __asm__("isb\n");
}

static __inline__
    __attribute__((always_inline))
    void K2_CpuReadBarrier(void)
{
    __asm__("dmb\n");
}

static __inline__
    __attribute__((always_inline))
    void K2_CpuWriteBarrier(void)
{
    __asm__("dsb\n");
}

#endif

#define K2_RETURN_ADDRESS        __builtin_return_address(0)

#else

#error !!!Unknown Build Environment

#endif

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

//
// basic
//

typedef unsigned char       UINT8;
typedef signed char         INT8;

typedef unsigned short      UINT16;
typedef signed short        INT16;

typedef unsigned int        UINT32;
typedef signed int          INT32;

typedef unsigned long long  UINT64;
typedef signed long long    INT64;

#if K2_TARGET_ARCH_IS_32BIT
typedef UINT32              UINT_PTR;
typedef INT32               INT_PTR;
#else
typedef UINT64              UINT_PTR;
typedef INT64               INT_PTR;
#endif

//
// derived
//

typedef int         BOOL;

typedef UINT32      K2STAT;

typedef INT8 *      VALIST;

typedef UINT64      K2_TIME;

typedef void        (*K2_pf_VOID)(void);

K2_PACKED_PUSH
struct _K2_GUID128
{
    UINT32  mData1;
    UINT16  mData2;
    UINT16  mData3;
    UINT8   mData4[8];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _K2_GUID128 K2_GUID128;

// {00000000-0000-0000-0000-000000000000}
#define K2_NULL_GUID \
{ 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }

K2_PACKED_PUSH
struct _K2PCI_BUS_ID
{
    UINT16  Segment;
    UINT16  Bus;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _K2PCI_BUS_ID K2PCI_BUS_ID;

K2_PACKED_PUSH
struct _K2PCI_DEVICE_LOC
{
    UINT16  Segment;
    UINT16  Bus;
    UINT16  Device;
    UINT16  Function;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _K2PCI_DEVICE_LOC K2PCI_DEVICE_LOC;

enum _K2_NUMTYPE
{
    K2_NUMTYPE_None = 0,
    K2_NUMTYPE_Card,  /* cardinal       ##         ex.  12    */
    K2_NUMTYPE_Int,   /* integer        -##        ex.  -3    */
    K2_NUMTYPE_Hex,   /* hexadecimal    0x##       ex.  0x3A  */
    K2_NUMTYPE_Bin,   /* B binary    -  0?1* b     ex.  1101b */
    K2_NUMTYPE_Kilo,  /* K kilobytes -  ##k or ##K ex.  64k   */
    K2_NUMTYPE_Mega,  /* M megabytes -  ##m or ##M ex.  16M   */
};
typedef enum _K2_NUMTYPE K2_NUMTYPE;

K2_PACKED_PUSH
struct _K2_EARTHTIME
{
    UINT16  mTimeZoneId;
    UINT16  mYear;
    UINT16  mMonth;
    UINT16  mDay;
    UINT16  mHour;
    UINT16  mMinute;
    UINT16  mSecond;
    UINT16  mMillisecond;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _K2_EARTHTIME K2_EARTHTIME;

K2_PACKED_PUSH
struct _K2POINT
{
    INT32 mX;
    INT32 mY;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _K2POINT K2POINT;

K2_PACKED_PUSH
struct _K2RECT
{
    K2POINT TopLeft;
    K2POINT BottomRight;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _K2RECT K2RECT;

enum _K2Rot
{
    K2Rot_None = 0,
    K2Rot_90Deg = 90,
    K2Rot_180Deg = 180,
    K2Rot_270Deg = 270
};
typedef enum _K2Rot K2Rot;

#if !defined(TRUE)
#define TRUE    ((BOOL)1)
#endif

#if !defined(FALSE)
#define FALSE   ((BOOL)0)
#endif

#if !defined(NULL)
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#define K2_STRINGIZE(x)         #x

#define K2_STATIC_ASSERT(e)   \
    typedef int static_assert__failed__ ## __COUNTER__ [!(e) ? -1 : 1]

#define K2_ROUNDDOWN(x,round)   (((x) / (round)) * (round))
#define K2_ROUNDUP(x,round)     K2_ROUNDDOWN(x + (round-1), round)

#ifdef __cplusplus
#define K2_FIELDOFFSET(s,m)     ((UINT_PTR)&reinterpret_cast<const volatile char&>((((s *)0)->m)))
#define K2_ADDROF(v)            ( &reinterpret_cast<const char &>(v) )
#else
#define K2_FIELDOFFSET(s,m)     ((UINT_PTR)&(((const s *)0)->m))
#define K2_ADDROF(v)            ( &(v) )
#endif    

#define K2_INTSIZE(n)           ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define K2_VASTART(ap,v)        ( (ap) = (VALIST)K2_ADDROF(v) + K2_INTSIZE(v) )
#define K2_VAARG(ap,t)          ( *(t *)(((ap) += K2_INTSIZE(t)) - K2_INTSIZE(t)) )
#define K2_VAEND(ap)            ( (ap) = (VALIST)0 )

#define K2_SWAP16(x)            (((((UINT16)(x))&0xFF)<<8) | ((((UINT16)(x))&0xFF00)>>8))
#define K2_SWAP32(x)            (((((UINT32)(x))&0xFFU)<<24) | ((((UINT32)(x))&0xFF00U)<<8) | ((((UINT32)(x))&0xFF0000U)>>8) | ((((UINT32)(x))&0xFF000000U)>>24))
#define K2_SWAP64(x)            ((K2_SWAP32((x)>>32)) | (((UINT64)(K2_SWAP32((x) & 0xFFFFFFFFULL)))<<32))

#ifndef K2_MAKEID4
#define K2_MAKEID4(a,b,c,d) \
        ((((UINT32)a)&0xFFU) | ((((UINT32)b)&0xFFU)<<8) | ((((UINT32)c)&0xFFU)<<16) | ((((UINT32)d)&0xFFU)<<24))
#endif  

#ifndef K2_GET_CONTAINER
#define K2_GET_CONTAINER(contType,pField,fieldName)  \
        ((contType *)(((UINT8 *)(pField)) - K2_FIELDOFFSET(contType,fieldName)))
#endif

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#define K2STAT_FACILITY_EXCEPTION       0x0FF00000U
#define K2STAT_FACILITY_SYSTEM          0x05000000U
#define K2STAT_FACILITY_ELF             0x05100000U
#define K2STAT_FACILITY_XDL             0x05200000U

#define K2STAT_TYPE_MASK                0xF0000000U
#define K2STAT_FACILITY_MASK            0x0FF00000U
#define K2STAT_NUMBER_MASK              0x000FFFFFU

#define K2STAT_TYPE_EXCEPTION           0xF0000000U
#define K2STAT_TYPE_ERROR               0x80000000U
#define K2STAT_TYPE_WARNING             0x40000000U

#define K2STAT_MAKE_ERROR(fac,num)      \
    (K2STAT_TYPE_ERROR |                \
     ((fac) & K2STAT_FACILITY_MASK) |   \
     ((num) & K2STAT_NUMBER_MASK))

#define K2STAT_MAKE_EXCEPTION(num)      \
    (K2STAT_TYPE_EXCEPTION |            \
     K2STAT_FACILITY_EXCEPTION |        \
     ((num) & K2STAT_NUMBER_MASK))

#define K2STAT_MAKE_WARNING(fac,num)    \
    (K2STAT_TYPE_WARNING |              \
     ((fac) & K2STAT_FACILITY_MASK) |   \
     ((num) & K2STAT_NUMBER_MASK))

#define K2STAT_GET_TYPE(stat)           ((stat) & K2STAT_TYPE_MASK)
#define K2STAT_GET_FACILITY(stat)       ((stat) & K2STAT_FACILITY_MASK)
#define K2STAT_GET_NUMBER(stat)         ((stat) & K2STAT_NUMBER_MASK)

#define K2STAT_IS_ERROR(x)              (K2STAT_GET_TYPE(x) & K2STAT_TYPE_ERROR)
#define K2STAT_IS_EXCEPTION(x)          (K2STAT_GET_TYPE(x) == K2STAT_TYPE_EXCEPTION)

//
//------------------------------------------------------------------------
//

#define K2STAT_OK                               0
#define K2STAT_NO_ERROR                         0

#define K2STAT_THREAD_WAITED                    1
#define K2STAT_OPENED                           2
#define K2STAT_CLOSED                           3
#define K2STAT_PENDING                          4
#define K2STAT_ALREADY_EXISTS                   5
#define K2STAT_COMPLETED                        6

#define K2STAT_EX_UNKNOWN                       K2STAT_MAKE_EXCEPTION(0)
#define K2STAT_EX_ACCESS                        K2STAT_MAKE_EXCEPTION(1)
#define K2STAT_EX_ZERODIVIDE                    K2STAT_MAKE_EXCEPTION(2)
#define K2STAT_EX_STACK                         K2STAT_MAKE_EXCEPTION(3)
#define K2STAT_EX_INSTRUCTION                   K2STAT_MAKE_EXCEPTION(4)
#define K2STAT_EX_PRIVILIGE                     K2STAT_MAKE_EXCEPTION(5)
#define K2STAT_EX_LOGIC                         K2STAT_MAKE_EXCEPTION(6)
#define K2STAT_EX_ALIGNMENT                     K2STAT_MAKE_EXCEPTION(7)

#define K2STAT_ERROR_UNKNOWN                    K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00000)
#define K2STAT_ERROR_TIMEOUT                    K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00001)
#define K2STAT_ERROR_NOT_IMPL                   K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00002)
#define K2STAT_ERROR_NOT_SUPPORTED              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00003)
#define K2STAT_ERROR_NOT_READY                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00004)
#define K2STAT_ERROR_HARDWARE                   K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00005)
#define K2STAT_ERROR_TOO_SMALL                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00006)
#define K2STAT_ERROR_TOO_BIG                    K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00007)
#define K2STAT_ERROR_OUT_OF_MEMORY              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00008)
#define K2STAT_ERROR_OUT_OF_RESOURCES           K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00009)
#define K2STAT_ERROR_INCOMPLETE                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0000A)
#define K2STAT_ERROR_COMPLETED                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0000B)
#define K2STAT_ERROR_NULL_POINTER               K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0000C)
#define K2STAT_ERROR_BAD_ARGUMENT               K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0000D)
#define K2STAT_ERROR_BAD_ALIGNMENT              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0000E)
#define K2STAT_ERROR_BAD_TOKEN                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0000F)
#define K2STAT_ERROR_BAD_FORMAT                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00010)
#define K2STAT_ERROR_VALUE_RESERVED             K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00011)
#define K2STAT_ERROR_CORRUPTED                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00012)
#define K2STAT_ERROR_NO_INTERFACE               K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00013)
#define K2STAT_ERROR_API_ORDER                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00014)
#define K2STAT_ERROR_NO_NULL_TERM               K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00015)
#define K2STAT_ERROR_OUT_OF_BOUNDS              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00016)
#define K2STAT_ERROR_ALREADY_EXISTS             K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00017)
#define K2STAT_ERROR_NOT_EXIST                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00018)
#define K2STAT_ERROR_NOT_FOUND                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00019)
#define K2STAT_ERROR_NOT_IN_USE                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0001A)
#define K2STAT_ERROR_IN_USE                     K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0001B)
#define K2STAT_ERROR_EMPTY                      K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0001C) 
#define K2STAT_ERROR_FULL                       K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0001D) 
#define K2STAT_ERROR_ABANDONED                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0001E) 
#define K2STAT_ERROR_ALREADY_OPEN               K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0001F)
#define K2STAT_ERROR_NOT_OPEN                   K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00020)
#define K2STAT_ERROR_CLOSED                     K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00021)
#define K2STAT_ERROR_NOT_CLOSED                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00022)
#define K2STAT_ERROR_RUNNING                    K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00023) 
#define K2STAT_ERROR_NOT_RUNNING                K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00024) 
#define K2STAT_ERROR_READ_ONLY                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00025) 
#define K2STAT_ERROR_NOT_READ_ONLY              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00026)
#define K2STAT_ERROR_OWNED                      K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00027)
#define K2STAT_ERROR_NOT_OWNED                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00028)
#define K2STAT_ERROR_END_OF_FILE                K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00029)
#define K2STAT_ERROR_NOT_END_OF_FILE            K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0002A)
#define K2STAT_ERROR_ADDR_NOT_ACCESSIBLE        K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0002B)
#define K2STAT_ERROR_ADDR_NOT_TEXT              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0002C)
#define K2STAT_ERROR_ADDR_NOT_WRITEABLE         K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0002D)
#define K2STAT_ERROR_ADDR_NOT_DATA              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0002E)
#define K2STAT_ERROR_ALREADY_RESERVED           K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0002F)
#define K2STAT_ERROR_NOT_RESERVED               K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00030)
#define K2STAT_ERROR_ALREADY_MAPPED             K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00031)
#define K2STAT_ERROR_NOT_MAPPED                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00032)
#define K2STAT_ERROR_WAIT_FAILED                K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00033)         
#define K2STAT_ERROR_WAIT_ABANDONED             K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00034)
#define K2STAT_ERROR_NOT_LOCKED                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00035)
#define K2STAT_ERROR_LOCKED                     K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00036)
#define K2STAT_ERROR_NOT_PAUSED                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00037)
#define K2STAT_ERROR_PAUSED                     K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00038)
#define K2STAT_ERROR_BAD_TYPE                   K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00039)
#define K2STAT_ERROR_UNSUPPORTED                K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0003A)
#define K2STAT_ERROR_INBUF_NULL                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0003B)
#define K2STAT_ERROR_INBUF_NOT_NULL             K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0003C)
#define K2STAT_ERROR_INBUF_TOO_SMALL            K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0003D)
#define K2STAT_ERROR_INBUF_TOO_BIG              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0003E)
#define K2STAT_ERROR_OUTBUF_NULL                K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0003F)
#define K2STAT_ERROR_OUTBUF_NOT_NULL            K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00040)
#define K2STAT_ERROR_OUTBUF_TOO_SMALL           K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00041)
#define K2STAT_ERROR_OUTBUF_TOO_BIG             K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00042)
#define K2STAT_ERROR_NOT_ALLOWED                K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00043)
#define K2STAT_ERROR_ABORTED                    K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00044)
#define K2STAT_ERROR_CHANGED                    K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00045)
#define K2STAT_ERROR_NOT_CONNECTED              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00046)
#define K2STAT_ERROR_CONNECTED                  K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00047)
#define K2STAT_ERROR_NO_MORE_ITEMS              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00048)
#define K2STAT_ERROR_PROCESS_EXITED             K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00049)
#define K2STAT_ERROR_REJECTED                   K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0004A)
#define K2STAT_ERROR_DISCONNECTED               K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0004B)
#define K2STAT_ERROR_BAD_SIZE                   K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0004C)
#define K2STAT_ERROR_CLASS_NOT_FOUND            K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0004D)
#define K2STAT_ERROR_OBJECT_NOT_FOUND           K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0004E)
#define K2STAT_ERROR_USAGE_NOT_FOUND            K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x0004F)
#define K2STAT_ERROR_ENABLED                    K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00050)
#define K2STAT_ERROR_NO_MEDIA                   K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00051)
#define K2STAT_ERROR_MEDIA_PRESENT              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00052)
#define K2STAT_ERROR_MEDIA_CHANGED              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00053)
#define K2STAT_ERROR_MEDIA_NOT_REMOVABLE        K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00054)
#define K2STAT_ERROR_INVALID_MEDIA              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00055)
#define K2STAT_ERROR_UNKNOWN_MEDIA_SIZE         K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00056)
#define K2STAT_ERROR_DUPLICATE_TOKEN            K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00057)
#define K2STAT_ERROR_NOT_NEEDED                 K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00058)
#define K2STAT_ERROR_NOT_SHAREABLE              K2STAT_MAKE_ERROR(K2STAT_FACILITY_SYSTEM, 0x00059)

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

K2_PACKED_PUSH
union _A32_REG_PSR
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mMODE            : 5;
        UINT32  mT               : 1;
        UINT32  mF               : 1;
        UINT32  mI               : 1;

        UINT32  mA               : 1;
        UINT32  mReserved5       : 1;
        UINT32  mReserved6       : 1;
        UINT32  mReserved7       : 1;

        UINT32  mReserved12_15   : 4;

        UINT32  mGE              : 4;

        UINT32  mReserved20_23   : 4;

        UINT32  mJ               : 1;
        UINT32  mReserved17      : 1;
        UINT32  mReserved18      : 1;
        UINT32  mQ               : 1;

        UINT32  mV               : 1;
        UINT32  mC               : 1;
        UINT32  mZ               : 1;
        UINT32  mN               : 1;
    } Bits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _A32_REG_PSR A32_REG_PSR;
K2_STATIC_ASSERT(sizeof(A32_REG_PSR)==sizeof(UINT32));

K2_PACKED_PUSH
union _A32_REG_SCTRL
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mM           : 1;    // M_MMUENABLE               
        UINT32  mA           : 1;    // A_ALIGNFAULTCHECKENABLE   
        UINT32  mC           : 1;    // C_DCACHEENABLE            
        UINT32  mReserved3   : 1;    
                                      
        UINT32  mReserved4   : 1;      
        UINT32  mReserved5   : 1;      
        UINT32  mReserved6   : 1;      
        UINT32  mB           : 1;    // B_BIGENDIAN                 
                                    
        UINT32  mReserved8   : 1;    
        UINT32  mReserved9   : 1;      
        UINT32  mSW          : 1;    // SW_SWPENABLE              
        UINT32  mZ           : 1;    // Z_BRANCHPREDICTENABLE   
                                      
        UINT32  mI           : 1;    // I_ICACHEENABLE            
        UINT32  mV           : 1;    // V_HIGHVECTORS             
        UINT32  mRR          : 1;    // RR_CACHEPREDICTABLE     
        UINT32  mReserved15  : 1;

        UINT32  mReserved16  : 1;
        UINT32  mHA          : 1;    // HA_HWACCESSFLAG           
        UINT32  mReserved18  : 1;
        UINT32  mReserved19  : 1;

        UINT32  mReserved20  : 1;
        UINT32  mFI          : 1;    // FI_FASTINTENABLE          
        UINT32  mReserved22  : 1;
        UINT32  mReserved23  : 1;

        UINT32  mVE          : 1;    // VE_IMPVECTORSENABLE     
        UINT32  mEE          : 1;    // EE_EXCEPTIONENDIAN      
        UINT32  mReserved26  : 1;
        UINT32  mNMFI        : 1;    // NMFI_NONMASKFASTINT       

        UINT32  mTRE         : 1;    // TRE_TEXREMAPENABLE      
        UINT32  mAFE         : 1;    // AFE_ACCESSFLAGENABLE    
        UINT32  mTE          : 1;    // TE_THUMBEXCEPTIONS      
        UINT32  mReserved31  : 1;
    } Bits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _A32_REG_SCTRL A32_REG_SCTRL;
K2_STATIC_ASSERT(sizeof(A32_REG_SCTRL)==sizeof(UINT32));

K2_PACKED_PUSH
union _A32_REG_TTBR
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mReserved0_13 : 14;
        UINT32  mTTPhysIx     : 18;  // 16kB aligned;
    } Bits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _A32_REG_TTBR A32_REG_TTBR;
K2_STATIC_ASSERT(sizeof(A32_REG_TTBR)==sizeof(UINT32));

K2_PACKED_PUSH
union _A32_TTBE
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mPresent        : 1;
        UINT32  mSBZ1           : 1;
        UINT32  mPXN            : 1;
        UINT32  mNS             : 1;

        UINT32  mSBZ4           : 1;
        UINT32  mDOMAIN         : 4;
        UINT32  mIMP            : 1;
        UINT32  mSBZ10_11       : 2;

        UINT32  mPTPhysIx       : 20;
    } PTBits;
    struct {
        UINT32  mPresent        : 1;
        UINT32  mSBO1           : 1;
        UINT32  mB              : 1;
        UINT32  mC              : 1;

        UINT32  mXN             : 1;
        UINT32  mDOMAIN         : 4;
        UINT32  mIMP            : 1;
        UINT32  mAP1_0          : 2;

        UINT32  mTEX            : 3;
        UINT32  mAP2            : 1;

        UINT32  mS              : 1;
        UINT32  mnG             : 1;
        UINT32  mSBZ18          : 1;
        UINT32  mNS             : 1;

        UINT32  mSecPhysIx      : 12;
    } SecBits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _A32_TTBE A32_TTBE;
K2_STATIC_ASSERT(sizeof(A32_TTBE)==sizeof(UINT32));

K2_PACKED_PUSH
struct _A32_TTBEQUAD
{
    A32_TTBE Quad[4];    /* 4 entries to map 1 4k PageTable */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _A32_TTBEQUAD A32_TTBEQUAD;
K2_STATIC_ASSERT(sizeof(A32_TTBEQUAD)==(sizeof(UINT32)*4));

K2_PACKED_PUSH
union _A32_TRANSTBL
{
    A32_TTBE        SingEntry[K2_VA32_PAGETABLES_FOR_4G * 4];   /* one entry per megabyte */
    A32_TTBEQUAD    QuadEntry[K2_VA32_PAGETABLES_FOR_4G];   /* one entry per 4 MB */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _A32_TRANSTBL A32_TRANSTBL;
K2_STATIC_ASSERT(sizeof(A32_TRANSTBL)==0x4000);

K2_PACKED_PUSH
union _A32_PROC_TRANSTBL
{
    A32_TTBE        SingEntry[K2_VA32_PAGETABLES_FOR_2G * 4];   /* one entry per megabyte */
    A32_TTBEQUAD    QuadEntry[K2_VA32_PAGETABLES_FOR_2G];       /* one entry per 4 MB */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _A32_PROC_TRANSTBL A32_PROC_TRANSTBL;
K2_STATIC_ASSERT(sizeof(A32_PROC_TRANSTBL)==0x2000);

K2_PACKED_PUSH
union _A32_PTE
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mXN          : 1;
        UINT32  mPresent     : 1;
        UINT32  mB           : 1;
        UINT32  mC           : 1;

        UINT32  mAP1_0       : 2;
        UINT32  mTEX         : 3;
        UINT32  mAP2         : 1;
        UINT32  mS           : 1;
        UINT32  mnG          : 1;

        UINT32  mPFPhysIx    : 20;
    } Bits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _A32_PTE A32_PTE;
K2_STATIC_ASSERT(sizeof(A32_PTE)==sizeof(UINT32));

K2_PACKED_PUSH
struct _A32_PAGETABLE
{
    A32_PTE  Entry[K2_VA32_ENTRIES_PER_PAGETABLE];    /* 4 contiguous hardware pagetables */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _A32_PAGETABLE A32_PAGETABLE;
K2_STATIC_ASSERT(sizeof(A32_PAGETABLE)==0x1000);

struct _A32_CACHECONFIG
{
    UINT32  mIIDR;
    UINT32  mDIDR;
    UINT32  mISizeBytes;
    UINT32  mDSizeBytes;
    UINT32  mILineBytes;
    UINT32  mDLineBytes;
    UINT32  mINumSets;
    UINT32  mINumWays;
    UINT32  mDNumSets;
    UINT32  mDNumWays;
    UINT32  mDSetBit;
    UINT32  mDWayBit;
};
typedef struct _A32_CACHECONFIG A32_CACHECONFIG;

struct _A32_CACHEINFO
{
    UINT32           mNumCacheLevels;
    UINT32           mDMinLineBytes;
    UINT32           mIMinLineBytes;
    UINT32           mDCacheLargest;
    UINT32           mICacheLargest;
    A32_CACHECONFIG  mCacheConfig[A32_MAX_CACHE_LEVELS];
};
typedef struct _A32_CACHEINFO A32_CACHEINFO;

struct _A32_CONTEXT
{
    UINT32 PSR;
    UINT32 R[16];
};
typedef struct _A32_CONTEXT A32_CONTEXT;

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

struct _A64_CONTEXT
{
    UINT64 R[32];
    UINT64 PSR;
};
typedef struct _A64_CONTEXT A64_CONTEXT;

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

K2_PACKED_PUSH
union _X32_REG_EFLAGS
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mCarry      : 1;
        UINT32  mReserved1  : 1;
        UINT32  mParity     : 1;
        UINT32  mReserved3  : 1;

        UINT32  mAuxCarry   : 1;
        UINT32  mReserved5  : 1;
        UINT32  mZero       : 1;
        UINT32  mSign       : 1;

        UINT32  mTrap       : 1;
        UINT32  mIntEnable  : 1;
        UINT32  mDir        : 1;
        UINT32  mOverflow   : 1;

        UINT32  mIOPL       : 2;
        UINT32  mNestedTask : 1;
        UINT32  mReserved15 : 1;
        
        UINT32  mResume     : 1;
        UINT32  mReserved17 : 1;
        UINT32  mAlign      : 1;
        UINT32  mReserved19 : 1;

        UINT32  mReserved20 : 1;
        UINT32  mIdent      : 1;
        UINT32  mReserved22 : 1;
        UINT32  mReserved23 : 1;

        UINT32  mReserved   : 8;
    } Bits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _X32_REG_EFLAGS X32_REG_EFLAGS;
K2_STATIC_ASSERT(sizeof(X32_REG_EFLAGS)==sizeof(UINT32));

K2_PACKED_PUSH
union _X32_REG_CR0
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mPM_ENABLE       : 1;
        UINT32  mCOPROC_PRESENT  : 1;
        UINT32  mFPEMUL          : 1;
        UINT32  mTASK_SWITCHED   : 1;

        UINT32  mEXTENSION       : 1;
        UINT32  mFPERR_ENABLE    : 1;
        UINT32  mReserved6       : 1;
        UINT32  mReserved7       : 1;

        UINT32  mReserved8_15   : 8;

        UINT32  mWRITE_PROTECT   : 1;
        UINT32  mReserved17      : 1;
        UINT32  mALIGN_MASK      : 1;
        UINT32  mReserved19      : 1;

        UINT32  mReserved20_27  : 8;

        UINT32  mReserved28     : 1;
        UINT32  mPnWT           : 1;
        UINT32  mPCD            : 1;
        UINT32  mPAGING_ENABLE  : 1;
    } Bits;
} K2_PACKED_ATTRIB; 
K2_PACKED_POP
typedef union _X32_REG_CR0 X32_REG_CR0;
K2_STATIC_ASSERT(sizeof(X32_REG_CR0)==sizeof(UINT32));

K2_PACKED_PUSH
union _X32_REG_CR3
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mReserved0_2  : 3;
        UINT32  mPWT          : 1;

        UINT32  mPCD          : 1;
        UINT32  mReserved5_11 : 7;
        
        UINT32  mPDPhysIx     : 20;
    } Bits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _X32_REG_CR3 X32_REG_CR3;
K2_STATIC_ASSERT(sizeof(X32_REG_CR3)==sizeof(UINT32));

K2_PACKED_PUSH
union _X32_PDE
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mPresent         : 1;
        UINT32  mWriteable       : 1;
        UINT32  mUser            : 1;
        UINT32  mWriteThrough    : 1;

        UINT32  mCacheDisable    : 1;
        UINT32  mAccessed        : 1;
        UINT32  mIgnored6        : 1;
        UINT32  mSBZ7            : 1;

        UINT32  mIgnored8_11     : 4;

        UINT32  mPTPhysIx        : 20;
    } Bits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _X32_PDE X32_PDE;
K2_STATIC_ASSERT(sizeof(X32_PDE)==sizeof(UINT32));

K2_PACKED_PUSH
struct _X32_PAGEDIR
{
    X32_PDE Entry[K2_VA32_PAGETABLES_FOR_4G];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _X32_PAGEDIR X32_PAGEDIR;
K2_STATIC_ASSERT(sizeof(X32_PAGEDIR)==K2_VA_MEMPAGE_BYTES);

K2_PACKED_PUSH
struct _X32_PROC_PAGEDIR
{
    X32_PDE Entry[K2_VA32_PAGETABLES_FOR_2G];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _X32_PROC_PAGEDIR X32_PROC_PAGEDIR;
K2_STATIC_ASSERT(sizeof(X32_PROC_PAGEDIR)==(K2_VA_MEMPAGE_BYTES/2));

K2_PACKED_PUSH
union _X32_PTE
{
    UINT32  mAsUINT32;
    struct {
        UINT32  mPresent         : 1;
        UINT32  mWriteable       : 1;
        UINT32  mUser            : 1;
        UINT32  mWriteThrough    : 1;

        UINT32  mCacheDisable    : 1;
        UINT32  mAccessed        : 1;
        UINT32  mDirty           : 1;
        UINT32  mPAT             : 1;

        UINT32  mGlobal          : 1;
        UINT32  mIgnored9_11     : 3;

        UINT32  mPFPhysIx        : 20;
    } Bits;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef union _X32_PTE X32_PTE;
K2_STATIC_ASSERT(sizeof(X32_PTE)==sizeof(UINT32));

K2_PACKED_PUSH
struct _X32_PAGETABLE
{
    X32_PTE Entry[K2_VA32_ENTRIES_PER_PAGETABLE];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _X32_PAGETABLE X32_PAGETABLE;
K2_STATIC_ASSERT(sizeof(X32_PAGETABLE)==K2_VA_MEMPAGE_BYTES);

K2_PACKED_PUSH
struct _X32_GDTENTRY
{
    UINT16  mLimitLow16;
    UINT16  mBaseLow16;
    UINT8   mBaseMid8;
    UINT8   mAttrib;
    UINT8   mLimitHigh4Attrib4;
    UINT8   mBaseHigh8;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _X32_GDTENTRY X32_GDTENTRY;
typedef X32_GDTENTRY X32_LDTENTRY;
K2_STATIC_ASSERT(sizeof(X32_GDTENTRY)==X32_SIZEOF_GDTENTRY);

K2_PACKED_PUSH
struct _X32_IDTENTRY
{
    UINT16  mAddrLow16;
    UINT16  mSelector;
    UINT8   mAlwaysZero;
    UINT8   mFlags;
    UINT16  mAddrHigh16;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _X32_IDTENTRY X32_IDTENTRY;
K2_STATIC_ASSERT(sizeof(X32_IDTENTRY)==X32_SIZEOF_IDTENTRY);

K2_PACKED_PUSH
struct _X32_xDTPTR
{
    UINT16  mTableSize;
    UINT32  mBaseAddr;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _X32_xDTPTR X32_xDTPTR;
K2_STATIC_ASSERT(sizeof(X32_xDTPTR)==6);

K2_PACKED_PUSH
struct _X32_TSS
{
    UINT16  mLinkTSSSelector;
    UINT16  mZeros0;

    UINT32  mESP0;
    UINT16  mSS0;
    UINT16  mZeros1;

    UINT32  mESP1;
    UINT16  mSS1;
    UINT16  mZeros2;

    UINT32  mESP2;
    UINT16  mSS2;
    UINT16  mZeros3;

    UINT32  mCR3;
    UINT32  mEIP;
    UINT32  mEFLAGS;
    UINT32  mEAX;
    UINT32  mECX;
    UINT32  mEDX;
    UINT32  mEBX;
    UINT32  mESP;
    UINT32  mEBP;
    UINT32  mESI;
    UINT32  mEDI;

    UINT16  mES;
    UINT16  mZeros4;
    UINT16  mCS;
    UINT16  mZeros5;
    UINT16  mSS;
    UINT16  mZeros6;
    UINT16  mDS;
    UINT16  mZeros7;
    UINT16  mFS;
    UINT16  mZeros8;
    UINT16  mGS;
    UINT16  mZeros9;

    UINT16  mLDTSelector;
    UINT16  mZerosA;

    UINT16  mTFlag;
    UINT16  mIOBitmapOffset;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _X32_TSS X32_TSS;

struct _X32_PUSHA
{
    UINT32 EDI;
    UINT32 ESI;
    UINT32 EBP;
    UINT32 ESP_Before_PushA;
    UINT32 EBX;
    UINT32 EDX;
    UINT32 ECX;
    UINT32 EAX;
};
typedef struct _X32_PUSHA X32_PUSHA;

struct _X32_CPUID
{
    UINT32 EAX;
    UINT32 EBX;
    UINT32 ECX;
    UINT32 EDX;
};
typedef struct _X32_CPUID X32_CPUID;

#define X32_CPUID1_EAX_SteppingId               0x0000000F
#define X32_CPUID1_EAX_Model                    0x000000F0
#define X32_CPUID1_EAX_FamilyId                 0x00000F00
#define X32_CPUID1_EAX_ProcType                 0x00003000
#define X32_CPUID1_EAX_Reserved14               0x0000C000
#define X32_CPUID1_EAX_ExtendedModelId          0x000F0000
#define X32_CPUID1_EAX_ExtendedFamilyId         0x0FF00000
#define X32_CPUID1_EAX_Reserved28               0xF0000000

#define X32_CPUID1_EBX_BrandIx                  0x000000FF
#define X32_CPUID1_EBX_ClFlushSize8ByteChunks   0x0000FF00
#define X32_CPUID1_EBX_Reserved16               0x00FF0000
#define X32_CPUID1_EBX_LocalApicId              0xFF000000

#define X32_CPUID1_ECX_SSE3                     0x00000001
#define X32_CPUID1_ECX_PCLMULQDQ                0x00000002
#define X32_CPUID1_ECX_DTES64                   0x00000004
#define X32_CPUID1_ECX_MONITOR                  0x00000008
#define X32_CPUID1_ECX_DS_CPL                   0x00000010
#define X32_CPUID1_ECX_VMX                      0x00000020
#define X32_CPUID1_ECX_SMX                      0x00000040
#define X32_CPUID1_ECX_EIST                     0x00000080
#define X32_CPUID1_ECX_TM2                      0x00000100
#define X32_CPUID1_ECX_SSSE3                    0x00000200
#define X32_CPUID1_ECX_CNXT_ID                  0x00000400
#define X32_CPUID1_ECX_SDBG                     0x00000800
#define X32_CPUID1_ECX_FMA                      0x00001000
#define X32_CPUID1_ECX_CMPXCHG16B               0x00002000
#define X32_CPUID1_ECX_xTPR_UPDATE_CONTROL      0x00004000
#define X32_CPUID1_ECX_PDCM                     0x00008000
#define X32_CPUID1_ECX_Reserved16               0x00010000
#define X32_CPUID1_ECX_PCID                     0x00020000
#define X32_CPUID1_ECX_DCA                      0x00040000
#define X32_CPUID1_ECX_SSE4_1                   0x00080000
#define X32_CPUID1_ECX_SSE4_2                   0x00100000
#define X32_CPUID1_ECX_x2APIC                   0x00200000
#define X32_CPUID1_ECX_MOVBE                    0x00400000
#define X32_CPUID1_ECX_POPCNT                   0x00800000
#define X32_CPUID1_ECX_TSC_DEADLINE             0x01000000
#define X32_CPUID1_ECX_AESNI                    0x02000000
#define X32_CPUID1_ECX_XSAVE                    0x04000000
#define X32_CPUID1_ECX_OSXSAVE                  0x08000000
#define X32_CPUID1_ECX_AVX                      0x10000000
#define X32_CPUID1_ECX_F16C                     0x20000000
#define X32_CPUID1_ECX_RDRAND                   0x40000000
#define X32_CPUID1_ECX_Unused0                  0x80000000

#define X32_CPUID1_EDX_FPU                      0x00000001
#define X32_CPUID1_EDX_VME                      0x00000002
#define X32_CPUID1_EDX_DE                       0x00000004
#define X32_CPUID1_EDX_PSE                      0x00000008
#define X32_CPUID1_EDX_TSC                      0x00000010
#define X32_CPUID1_EDX_MSR                      0x00000020
#define X32_CPUID1_EDX_PAE                      0x00000040
#define X32_CPUID1_EDX_MCE                      0x00000080
#define X32_CPUID1_EDX_CXB                      0x00000100
#define X32_CPUID1_EDX_APIC                     0x00000200
#define X32_CPUID1_EDX_Reserved10               0x00000400
#define X32_CPUID1_EDX_SEP                      0x00000800
#define X32_CPUID1_EDX_MTRR                     0x00001000
#define X32_CPUID1_EDX_PGE                      0x00002000
#define X32_CPUID1_EDX_MCA                      0x00004000
#define X32_CPUID1_EDX_CMOV                     0x00008000
#define X32_CPUID1_EDX_PAT                      0x00010000
#define X32_CPUID1_EDX_PSE_36                   0x00020000
#define X32_CPUID1_EDX_PSN                      0x00040000
#define X32_CPUID1_EDX_CLFSH                    0x00080000
#define X32_CPUID1_EDX_Reserved20               0x00100000
#define X32_CPUID1_EDX_DS                       0x00200000
#define X32_CPUID1_EDX_ACPI                     0x00400000
#define X32_CPUID1_EDX_MMX                      0x00800000
#define X32_CPUID1_EDX_FXS                      0x01000000
#define X32_CPUID1_EDX_SSE                      0x02000000
#define X32_CPUID1_EDX_SSE2                     0x04000000
#define X32_CPUID1_EDX_SS                       0x08000000
#define X32_CPUID1_EDX_HTT                      0x10000000
#define X32_CPUID1_EDX_TM                       0x20000000
#define X32_CPUID1_EDX_Reserved30               0x40000000
#define X32_CPUID1_EDX_PBE                      0x80000000

struct _X32_CACHEINFO
{
    UINT32 mCacheLineBytes;
    UINT32 mL1CacheSizeBytesI;
    UINT32 mL1CacheSizeBytesD;
    UINT32 mL2CacheSizeBytes;
    UINT32 mL3CacheSizeBytes;
};
typedef struct _X32_CACHEINFO X32_CACHEINFO;

struct _X32_CONTEXT
{
    UINT32  EFLAGS;     /* pushed last (tos) */
    UINT32  EDI;        /* pusha */
    UINT32  ESI;        /* pusha */
    UINT32  EBP;        /* pusha */
    UINT32  ESP;        /* pusha */
    UINT32  EBX;        /* pusha */
    UINT32  EDX;        /* pusha */
    UINT32  ECX;        /* pusha */
    UINT32  EAX;        /* pusha */
    UINT32  EIP;        /* pushed first (bos). ESP points here */
};
typedef struct _X32_CONTEXT X32_CONTEXT;

#ifdef __cplusplus
};  // extern "C"
#endif
                                                
//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

struct _X64_CONTEXT
{
    UINT64 R[32];
    UINT64 PSR;
};
typedef struct _X64_CONTEXT X64_CONTEXT;

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#if K2_TARGET_ARCH_IS_ARM
#if K2_TARGET_ARCH_IS_32BIT
#define K2_PAGETABLE    A32_PAGETABLE
#define K2_CACHEINFO    A32_CACHEINFO
#define K2_TRAP_CONTEXT A32_CONTEXT
#else
// tbd
#define K2_TRAP_CONTEXT A64_CONTEXT
#endif
#elif K2_TARGET_ARCH_IS_INTEL
#if K2_TARGET_ARCH_IS_32BIT
#define K2_PAGETABLE    X32_PAGETABLE
#define K2_CACHEINFO    X32_CACHEINFO
#define K2_TRAP_CONTEXT X32_CONTEXT
#else
// tbd
#define K2_TRAP_CONTEXT X64_CONTEXT
#endif
#else
#error !!! Unknown target architecture
#endif

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

K2_PACKED_PUSH
struct _K2_DEVICE_IDENT
{
    UINT16  mVendorId;
    UINT16  mDeviceId;
    UINT8   mRevision;
    UINT8   mProgIF;
    UINT8   mSubClassCode;
    UINT8   mClassCode;
    UINT16  mSubsystemVendorId;
    UINT16  mSubsystemId;
    UINT32  mUserVal;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _K2_DEVICE_IDENT K2_DEVICE_IDENT;

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

static K2_INLINE UINT64
K2MMIO_Read64(UINT_PTR aAddr)
{
    UINT64 ret;
    ret = *((volatile UINT64 *)aAddr);
#if K2_TARGET_ARCH_IS_ARM
    K2_CpuReadBarrier();
#endif
    return ret;
}

static K2_INLINE UINT32
K2MMIO_Read32(UINT_PTR aAddr)
{
    UINT32 ret;
    ret = *((volatile UINT32 *)aAddr);
#if K2_TARGET_ARCH_IS_ARM
    K2_CpuReadBarrier();
#endif
    return ret;
}

static K2_INLINE UINT16
K2MMIO_Read16(UINT_PTR aAddr)
{
    UINT16 ret;
    ret = *((volatile UINT16 *)aAddr);
#if K2_TARGET_ARCH_IS_ARM
    K2_CpuReadBarrier();
#endif
    return ret;
}

static K2_INLINE UINT8
K2MMIO_Read8(UINT_PTR aAddr)
{
    UINT8 ret;
    ret = *((volatile UINT8 *)aAddr);
#if K2_TARGET_ARCH_IS_ARM
    K2_CpuReadBarrier();
#endif
    return ret;
}

static K2_INLINE void
K2MMIO_Write64(UINT_PTR aAddr, UINT64 val)
{
    *((volatile UINT64 *)aAddr) = val;
#if K2_TARGET_ARCH_IS_ARM
    K2_CpuWriteBarrier();
#endif
}

static K2_INLINE void
K2MMIO_Write32(UINT_PTR aAddr, UINT32 val)
{
    *((volatile UINT32 *)aAddr) = val;
#if K2_TARGET_ARCH_IS_ARM
    K2_CpuWriteBarrier();
#endif
}

static K2_INLINE void
K2MMIO_Write16(UINT_PTR aAddr, UINT16 val)
{
    *((volatile UINT16 *)aAddr) = val;
#if K2_TARGET_ARCH_IS_ARM
    K2_CpuWriteBarrier();
#endif
}

static K2_INLINE void
K2MMIO_Write8(UINT_PTR aAddr, UINT8 val)
{
    *((volatile UINT8 *)aAddr) = val;
#if K2_TARGET_ARCH_IS_ARM
    K2_CpuWriteBarrier();
#endif
}

static K2_INLINE void
K2MMIO_SetBits64(UINT_PTR aAddr, UINT64 val)
{
    UINT64 v;
    v = K2MMIO_Read64(aAddr);
    v |= val;
    K2MMIO_Write64(aAddr, v);
}

static K2_INLINE void
K2MMIO_SetBits32(UINT_PTR aAddr, UINT32 val)
{
    UINT32 v;
    v = K2MMIO_Read32(aAddr);
    v |= val;
    K2MMIO_Write32(aAddr, v);
}

static K2_INLINE void
K2MMIO_SetBits16(UINT_PTR aAddr, UINT16 val)
{
    UINT16 v;
    v = K2MMIO_Read16(aAddr);
    v |= val;
    K2MMIO_Write16(aAddr, v);
}

static K2_INLINE void
K2MMIO_SetBits8(UINT_PTR aAddr, UINT8 val)
{
    UINT8 v;
    v = K2MMIO_Read8(aAddr);
    v |= val;
    K2MMIO_Write8(aAddr, v);
}

static K2_INLINE void
K2MMIO_ClrBits64(UINT_PTR aAddr, UINT64 val)
{
    UINT64 v;
    v = K2MMIO_Read64(aAddr);
    v &= ~val;
    K2MMIO_Write64(aAddr, v);
}

static K2_INLINE void
K2MMIO_ClrBits32(UINT_PTR aAddr, UINT32 val)
{
    UINT32 v;
    v = K2MMIO_Read32(aAddr);
    v &= ~val;
    K2MMIO_Write32(aAddr, v);
}

static K2_INLINE void
K2MMIO_ClrBits16(UINT_PTR aAddr, UINT16 val)
{
    UINT16 v;
    v = K2MMIO_Read16(aAddr);
    v &= ~val;
    K2MMIO_Write16(aAddr, v);
}

static K2_INLINE void
K2MMIO_ClrBits8(UINT_PTR aAddr, UINT8 val)
{
    UINT8 v;
    v = K2MMIO_Read8(aAddr);
    v &= ~val;
    K2MMIO_Write8(aAddr, v);
}

#define MMREG_WRITE64(base, offset, value)    K2MMIO_Write64((base)+(offset),value)
#define MMREG_READ64(base, offset)            K2MMIO_Read64((base)+(offset))
#define MMREG_SETBITS64(base, offset, value)  K2MMIO_SetBits64((base)+(offset), value)
#define MMREG_CLRBITS64(base, offset, value)  K2MMIO_ClrBits64((base)+(offset), value)

#define MMREG_WRITE32(base, offset, value)    K2MMIO_Write32((base)+(offset),value)
#define MMREG_READ32(base, offset)            K2MMIO_Read32((base)+(offset))
#define MMREG_SETBITS32(base, offset, value)  K2MMIO_SetBits32((base)+(offset), value)
#define MMREG_CLRBITS32(base, offset, value)  K2MMIO_ClrBits32((base)+(offset), value)

#define MMREG_WRITE16(base, offset, value)    K2MMIO_Write16((base)+(offset),value)
#define MMREG_READ16(base, offset)            K2MMIO_Read16((base)+(offset))
#define MMREG_SETBITS16(base, offset, value)  K2MMIO_SetBits16((base)+(offset), value)
#define MMREG_CLRBITS16(base, offset, value)  K2MMIO_ClrBits16((base)+(offset), value)

#define MMREG_WRITE8(base, offset, value)     K2MMIO_Write8((base)+(offset),value)
#define MMREG_READ8(base, offset)             K2MMIO_Read8((base)+(offset))
#define MMREG_SETBITS8(base, offset, value)   K2MMIO_SetBits8((base)+(offset), value)
#define MMREG_CLRBITS8(base, offset, value)   K2MMIO_ClrBits8((base)+(offset), value)

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//


#endif  // __K2BASETYPE_H
