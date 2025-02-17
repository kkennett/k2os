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

#ifndef __A32KERN_H
#define __A32KERN_H

#include "..\..\kern.h"
#include "a32kerndef.inc"
#include <lib/k2dis.h>

/* --------------------------------------------------------------------------------- */

void A32KernAsm_LaunchEntryPoint(UINT32 aCoreIx);
void A32KernAsm_ResumeInMonitor(UINT32 aStackPtr);
void A32KernAsm_ResumeThread(UINT32 aKernModeStackPtr, UINT32 aSvcScratch);
void A32Kern_InitTiming(void);
void A32Kern_DumpExecContext(K2OSKERN_ARCH_EXEC_CONTEXT * apExContext);
void A32Kern_DumpExceptionContext(K2OSKERN_CPUCORE volatile* apCore, UINT32 aReason, K2OSKERN_ARCH_EXEC_CONTEXT* pEx);
void A32Kern_DumpStackTrace(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_PROCESS *apProc, UINT32 aPC, UINT32 aSP, UINT32* apBackPtr, UINT32 aLR);

void A32Kern_StartTime(void);
void A32Kern_TimerInterrupt(K2OSKERN_CPUCORE volatile * apThisCore);
void A32Kern_SetCoreTimer(K2OSKERN_CPUCORE volatile * apThisCore, UINT32 aCpuTickDelay);
BOOL A32Kern_CoreTimerInterrupt(K2OSKERN_CPUCORE volatile *apThisCore);
void A32Kern_StopCoreTimer(K2OSKERN_CPUCORE volatile *apThisCore);

void    A32Kern_IntrInitGicDist(void);
void    A32Kern_IntrInitGicPerCore(void);
UINT32  A32Kern_IntrAck(void);
void    A32Kern_IntrEoi(UINT32 aAckVal);
void    A32Kern_IntrSetEnable(UINT32 aIntrId, BOOL aSetEnable);
BOOL    A32Kern_IntrGetEnable(UINT32 aIntrId);
void    A32Kern_IntrClearPending(UINT32 aIntrId, BOOL aAlsoDisable);
void    A32Kern_IntrConfig(UINT32 aIntrId, K2OS_IRQ_CONFIG const * apConfig);

void A32KernAsm_SaveKernelThreadStateAndEnterMonitor(UINT32 aNewStackPtr, K2OSKERN_ARCH_EXEC_CONTEXT *apSaveContext);

#define A32_SYM_NAME_MAX_LEN            80

extern K2OSKERN_SEQLOCK                 gA32Kern_IntrSeqLock;
extern UINT64 *                         gpA32Kern_AcpiTablePtrs;
extern UINT32                           gA32Kern_AcpiTableCount;
extern ACPI_MADT *                      gpA32Kern_MADT;
extern ACPI_MADT_SUB_GIC *              gpA32Kern_MADT_GICC[K2OS_MAX_CPU_COUNT];
extern ACPI_MADT_SUB_GIC_DISTRIBUTOR *  gpA32Kern_MADT_GICD;
extern UINT32                           gA32Kern_IntrCount;
extern UINT32                           gA32Kern_VectorPagePhys;
extern K2OSKERN_PHYSTRACK *             gpA32Kern_StartTtbPhysTrack;
extern K2LIST_ANCHOR                    gA32Kern_StartAreaPhysList;

/* --------------------------------------------------------------------------------- */

#endif // __A32KERN_H
