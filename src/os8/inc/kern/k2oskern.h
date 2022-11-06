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
#ifndef __K2OSKERN_H
#define __K2OSKERN_H

#include <k2os.h>
#include <spec/acpi.h>

#if K2_TARGET_ARCH_IS_INTEL
#if K2_TARGET_ARCH_IS_32BIT
#include <lib/k2archx32.h>
#else
#include <lib/k2archx64.h>
#endif
#endif
#if K2_TARGET_ARCH_IS_ARM
#if K2_TARGET_ARCH_IS_32BIT
#include <lib/k2archa32.h>
#else
#include <lib/k2archa64.h>
#endif
#endif

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

typedef UINT32 (K2_CALLCONV_REGS *K2OSKERN_pf_GetCpuIndex)(void);
UINT_PTR K2_CALLCONV_REGS K2OSKERN_GetCpuIndex(void);

//
//------------------------------------------------------------------------
//

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_SetIntr)(BOOL aEnable);
BOOL    K2_CALLCONV_REGS K2OSKERN_SetIntr(BOOL aEnable);

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_GetIntr)(void);
BOOL    K2_CALLCONV_REGS K2OSKERN_GetIntr(void);

//
//------------------------------------------------------------------------
//

struct _K2OSKERN_SEQLOCK
{
    UINT8 mQpaque[3 * K2OS_CACHELINE_BYTES];
};
typedef struct _K2OSKERN_SEQLOCK K2OSKERN_SEQLOCK;

typedef void (K2_CALLCONV_REGS *K2OSKERN_pf_SeqInit)(K2OSKERN_SEQLOCK * apLock);
void    K2_CALLCONV_REGS K2OSKERN_SeqInit(K2OSKERN_SEQLOCK * apLock);

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_SeqLock)(K2OSKERN_SEQLOCK * apLock);
BOOL    K2_CALLCONV_REGS K2OSKERN_SeqLock(K2OSKERN_SEQLOCK * apLock);

typedef void  (K2_CALLCONV_REGS *K2OSKERN_pf_SeqUnlock)(K2OSKERN_SEQLOCK * apLock, BOOL aDisp);
void    K2_CALLCONV_REGS K2OSKERN_SeqUnlock(K2OSKERN_SEQLOCK * apLock, BOOL aLockDisp);

//
//------------------------------------------------------------------------
//

typedef UINT_PTR (*K2OSKERN_pf_Debug)(char const *apFormat, ...);
UINT_PTR K2OSKERN_Debug(char const *apFormat, ...);

typedef void (*K2OSKERN_pf_Panic)(char const *apFormat, ...);
void     K2OSKERN_Panic(char const *apFormat, ...);

//
//------------------------------------------------------------------------
//

typedef void (K2_CALLCONV_REGS *K2OSKERN_pf_MicroStall)(UINT_PTR aMicroseconds);
void K2_CALLCONV_REGS K2OSKERN_MicroStall(UINT_PTR aMicroseconds);

//
//------------------------------------------------------------------------
//

typedef struct _K2OSKERN_PLATNODE   K2OSKERN_PLATNODE;
typedef struct _K2OSKERN_PLATIRQ    K2OSKERN_PLATIRQ;
typedef struct _K2OSKERN_PLATINTR   K2OSKERN_PLATINTR;
typedef struct _K2OSKERN_PLATIO     K2OSKERN_PLATIO;
typedef struct _K2OSKERN_PLATPHYS   K2OSKERN_PLATPHYS;

struct _K2OSKERN_PLATIRQ
{
    K2OS_IRQ_CONFIG     Config;
    K2LIST_ANCHOR       IntrList;
};

struct _K2OSKERN_PLATINTR
{
    K2OSKERN_PLATIRQ *  mpIrq;
    K2LIST_LINK         IrqIntrListLink;

    K2OSKERN_PLATNODE * mpNode;

    K2OSKERN_PLATINTR * mpNext;
};

struct _K2OSKERN_PLATIO
{
    K2OS_IOPORT_RANGE   Range;
    K2OSKERN_PLATIO *   mpNext;
};

struct _K2OSKERN_PLATPHYS
{
    K2OS_PHYSADDR_RANGE Range;
    UINT_PTR            mKernVirtAddr;
    K2OSKERN_PLATPHYS * mpNext;
};

#define K2OSKERN_PLATNODE_NAME_BUFFER_CHARS    32

struct _K2OSKERN_PLATNODE
{
    char                mName[K2OSKERN_PLATNODE_NAME_BUFFER_CHARS];
    UINT_PTR            mRootContext;
    UINT_PTR            mRefs;

    K2OSKERN_PLATIO *   mpIoList;
    K2OSKERN_PLATPHYS * mpPhysList;
    K2OSKERN_PLATINTR * mpIntrList;

    K2TREE_NODE         TreeNode;

    K2OSKERN_PLATNODE * mpParent;
    K2LIST_ANCHOR       ChildList;
    K2LIST_LINK         ParentChildListLink;
};

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif


#endif // __K2OSKERN_H