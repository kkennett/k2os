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
#ifndef __K2OSKERN_H
#define __K2OSKERN_H

#include <k2os.h>
#include <spec/acpi.h>

#if K2_TARGET_ARCH_IS_INTEL
#include <lib/k2archx32.h>
#endif
#if K2_TARGET_ARCH_IS_ARM
#include <lib/k2archa32.h>
#endif

#if __cplusplus
extern "C" {
#endif

//
//------------------------------s------------------------------------------
//

typedef UINT32 (K2_CALLCONV_REGS *K2OSKERN_pf_GetCpuIndex)(void);
UINT32 K2_CALLCONV_REGS K2OSKERN_GetCpuIndex(void);

//
//------------------------------------------------------------------------
//

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_SetIntr)(BOOL aEnable);
BOOL K2_CALLCONV_REGS K2OSKERN_SetIntr(BOOL aEnable);

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_GetIntr)(void);
BOOL K2_CALLCONV_REGS K2OSKERN_GetIntr(void);

//
//------------------------------------------------------------------------
//

struct _K2OSKERN_SEQLOCK
{
    UINT8 mQpaque[3 * K2OS_CACHELINE_BYTES];
};
typedef struct _K2OSKERN_SEQLOCK K2OSKERN_SEQLOCK;

typedef void (K2_CALLCONV_REGS *K2OSKERN_pf_SeqInit)(K2OSKERN_SEQLOCK * apLock);
void K2_CALLCONV_REGS K2OSKERN_SeqInit(K2OSKERN_SEQLOCK * apLock);

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_SeqLock)(K2OSKERN_SEQLOCK * apLock);
BOOL K2_CALLCONV_REGS K2OSKERN_SeqLock(K2OSKERN_SEQLOCK * apLock);

typedef void  (K2_CALLCONV_REGS *K2OSKERN_pf_SeqUnlock)(K2OSKERN_SEQLOCK * apLock, BOOL aDisp);
void K2_CALLCONV_REGS K2OSKERN_SeqUnlock(K2OSKERN_SEQLOCK * apLock, BOOL aLockDisp);

//
//------------------------------------------------------------------------
//

typedef UINT32 (*K2OSKERN_pf_Debug)(char const *apFormat, ...);
UINT32 K2OSKERN_Debug(char const *apFormat, ...);

typedef void (*K2OSKERN_pf_Panic)(char const *apFormat, ...);
void K2OSKERN_Panic(char const *apFormat, ...);

//
//------------------------------------------------------------------------
//

typedef void (K2_CALLCONV_REGS *K2OSKERN_pf_MicroStall)(UINT32 aMicroseconds);
void K2_CALLCONV_REGS K2OSKERN_MicroStall(UINT32 aMicroseconds);

//
//------------------------------------------------------------------------
//

typedef K2STAT (*K2OSKERN_pf_Sysproc_Notify)(K2OS_MSG const *apMsg);
K2STAT K2OSKERN_SysProc_Notify(K2OS_MSG const *apMsg);

//
//------------------------------------------------------------------------
//

//
// these are what the hooks can be told to do
//
typedef enum _KernIntrActionType KernIntrActionType;
enum _KernIntrActionType
{
    KernIntrAction_Invalid = 0,

    KernIntrAction_AtIrq,       // go see if the device is interrupting. return KernIntrDispType
    KernIntrAction_Enable,      // enable the interrupt at the device.  return value ignored
    KernIntrAction_Disable,     // disable the interrupt at the device. return value ignored
    KernIntrAction_Done,        // driver indicating interrupt is finished. return value ignored

    KernIntrActionType_Count
};

//
// these are what the hooks can return when they are given the action KernIntrAction_Eval
//
typedef enum _KernIntrDispType KernIntrDispType;
enum _KernIntrDispType
{
    KernIntrDisp_None = 0,      // this isn't for me (i am not interrupting)
    KernIntrDisp_Handled,       // handled in the hook during irq. do not pulse the interrupt gate
    KernIntrDisp_Fire,          // pulse the interrupt gate, but do not need interrupt to be disabled
    KernIntrDisp_Fire_Disable,  // pulse the interrupt gate and leave the irq disabled

    KernIntrDispType_Count
};

//
// this is a hook function that returns what to do for the interrupt when it is called
//
typedef KernIntrDispType (*K2OSKERN_pf_Hook_Key)(void *apKey, KernIntrActionType aAction);

//
// create K2OSKERN_IRQ and install it via Arch intr call
//
typedef BOOL (*K2OSKERN_pf_IrqDefine)(K2OS_IRQ_CONFIG *apConfig);
BOOL K2OSKERN_IrqDefine(K2OS_IRQ_CONFIG const *apConfig);

//
// create K2OSKERN_OBJ_INTERRUPT and add it to the IRQ interrupt list
// setting the hook key in the object
//
typedef K2OS_INTERRUPT_TOKEN (*K2OSKERN_pf_IrqHook)(UINT32 aSourceIrq, K2OSKERN_pf_Hook_Key *apKey);
K2OS_INTERRUPT_TOKEN K2OSKERN_IrqHook(UINT32 aSourceIrq, K2OSKERN_pf_Hook_Key *apKey);

//
// raw enable or disable the specified irq using the arch API
//
typedef BOOL (*K2OSKERN_pf_IrqSetEnable)(UINT32 aSourceIrq, BOOL aSetEnable);
BOOL K2OSKERN_IrqSetEnable(UINT32 aSourceIrq, BOOL aSetEnable);

//
// used by drivers to act on the interrupt tokens they receive
//
typedef BOOL (*K2OSKERN_pf_IntrVoteIrqEnable)(K2OS_INTERRUPT_TOKEN aTokIntr, BOOL aSetEnable);
BOOL K2OSKERN_IntrVoteIrqEnable(K2OS_INTERRUPT_TOKEN aTokIntr, BOOL aSetEnable);

typedef BOOL (*K2OSKERN_pf_IntrDone)(K2OS_INTERRUPT_TOKEN aTokIntr);
BOOL K2OSKERN_IntrDone(K2OS_INTERRUPT_TOKEN aTokIntr);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif


#endif // __K2OSKERN_H