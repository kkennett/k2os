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

#include "a32kern.h"

static char sgSymDump[A32_SYM_NAME_MAX_LEN * K2OS_MAX_CPU_COUNT];

static void
sEmitSymbolName(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aAddr,
    char *                  pBuffer
)
{
    *pBuffer = 0;
    KernXdl_FindClosestSymbol(apProc, aAddr, pBuffer, A32_SYM_NAME_MAX_LEN);
    if (*pBuffer == 0)
    {
        K2OSKERN_Debug("?(%08X)", aAddr);
        return;
    }
    K2OSKERN_Debug("%s", pBuffer);
}

void A32Kern_DumpStackTrace(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_PROCESS *apProc, UINT32 aPC, UINT32 aSP, UINT32* apBackPtr, UINT32 aLR)
{
    char *pBuffer = &sgSymDump[apThisCore->mCoreIx * A32_SYM_NAME_MAX_LEN];

    K2OSKERN_Debug("StackTrace:\n");
    K2OSKERN_Debug("------------------\n");
    K2OSKERN_Debug("SP       %08X\n", aSP);

    K2OSKERN_Debug("PC       ");
    sEmitSymbolName(apProc, aPC, pBuffer);
    K2OSKERN_Debug("\n");

    K2OSKERN_Debug("LR       %08X ", aLR);
    sEmitSymbolName(apProc, aLR, pBuffer);
    K2OSKERN_Debug("\n");

#if 0

each frame:

pc  <-- fp points here : contains pointer to 8 bytes after the push instruction at the start of the function that is saving these 4 registers
lr  -4  (saved Link register which holds the return address this routine will return to)
sp  -8  (saved stack pointer in function that is returned at the point of the call)
fp  -12 (next frame pointer in stack)

#endif

    K2OSKERN_Debug("%08X ", apBackPtr);
    if (apBackPtr == NULL)
    {
        K2OSKERN_Debug("Stack Ends\n");
        return;
    }
    K2OSKERN_Debug("%08X ", apBackPtr[-1]);
    sEmitSymbolName(apProc, apBackPtr[-1], pBuffer);
    K2OSKERN_Debug("\n");
    // saved stack pointer is at -2;  don't really care what that is
    do {
        apBackPtr = (UINT32*)apBackPtr[-3];
        K2OSKERN_Debug("%08X ", apBackPtr);
        if (apBackPtr == NULL)
        {
            K2OSKERN_Debug("\n");
            return;
        }
        if (apBackPtr[0] == 0)
        {
            K2OSKERN_Debug("00000000 Stack Ends\n");
            return;
        }
        K2OSKERN_Debug("%08X ", apBackPtr[-1]);
        sEmitSymbolName(apProc, apBackPtr[-1], pBuffer);
        K2OSKERN_Debug("\n");
    } while (1);
}

void
A32Kern_DumpExecContext(
    K2OSKERN_ARCH_EXEC_CONTEXT * apExContext
)
{
    K2OSKERN_Debug("SPSR      %08X\n", apExContext->PSR);
    K2OSKERN_Debug("  r0=%08X   r8=%08X\n", apExContext->R[0], apExContext->R[8]);
    K2OSKERN_Debug("  r1=%08X   r9=%08X\n", apExContext->R[1], apExContext->R[9]);
    K2OSKERN_Debug("  r2=%08X  r10=%08X\n", apExContext->R[2], apExContext->R[10]);
    K2OSKERN_Debug("  r3=%08X  r11=%08X\n", apExContext->R[3], apExContext->R[11]);
    K2OSKERN_Debug("  r4=%08X  r12=%08X\n", apExContext->R[4], apExContext->R[12]);
    K2OSKERN_Debug("  r5=%08X   sp=%08X\n", apExContext->R[5], apExContext->R[13]);
    K2OSKERN_Debug("  r6=%08X   lr=%08X\n", apExContext->R[6], apExContext->R[14]);
    K2OSKERN_Debug("  r7=%08X   pc=%08X\n", apExContext->R[7], apExContext->R[15]);
}

void A32Kern_DumpExceptionContext(K2OSKERN_CPUCORE volatile* apCore, UINT32 aReason, K2OSKERN_ARCH_EXEC_CONTEXT* pEx)
{
    K2OSKERN_Debug("Exception 0x%08X (", aReason);
    switch (aReason)
    {
    case A32KERN_EXCEPTION_REASON_UNDEFINED_INSTRUCTION:
        K2OSKERN_Debug("Undefined Instruction");
        break;
    case A32KERN_EXCEPTION_REASON_SYSTEM_CALL:
        K2OSKERN_Debug("System Call");
        break;
    case A32KERN_EXCEPTION_REASON_PREFETCH_ABORT:
        K2OSKERN_Debug("Prefetch Abort");
        break;
    case A32KERN_EXCEPTION_REASON_DATA_ABORT:
        K2OSKERN_Debug("Data Abort");
        break;
    case A32KERN_EXCEPTION_REASON_IRQ:
        K2OSKERN_Debug("IRQ");
        break;
    case A32KERN_EXCEPTION_REASON_RAISE_EXCEPTION:
        K2OSKERN_Debug("RaiseException");
        break;
    default:
        K2OSKERN_Debug("Unknown");
        break;
    }
    K2OSKERN_Debug(") in Arm32.");
    switch (pEx->PSR & A32_PSR_MODE_MASK)
    {
    case A32_PSR_MODE_USR:
        K2OSKERN_Debug("USR");
        break;
    case A32_PSR_MODE_SYS:
        K2OSKERN_Debug("SYS");
        break;
    case A32_PSR_MODE_SVC:
        K2OSKERN_Debug("SVC");
        break;
    case A32_PSR_MODE_UNDEF:
        K2OSKERN_Debug("UND");
        break;
    case A32_PSR_MODE_ABT:
        K2OSKERN_Debug("ABT");
        break;
    case A32_PSR_MODE_IRQ:
        K2OSKERN_Debug("IRQ");
        break;
    case A32_PSR_MODE_FIQ:
        K2OSKERN_Debug("FIQ");
        break;
    default:
        K2OSKERN_Debug("???");
        break;
    }
    K2OSKERN_Debug(" mode. ExContext @ 0x%08X\n", pEx);
    K2OSKERN_Debug("------------------\n");
    K2OSKERN_Debug("Core      %d\n", apCore->mCoreIx);
    K2OSKERN_Debug("DFAR      %08X\n", A32_ReadDFAR());
    K2OSKERN_Debug("DFSR      %08X\n", A32_ReadDFSR());
    K2OSKERN_Debug("IFAR      %08X\n", A32_ReadIFAR());
    K2OSKERN_Debug("IFSR      %08X\n", A32_ReadIFSR());
    K2OSKERN_Debug("STACK     %08X\n", A32_ReadStackPointer());
    K2OSKERN_Debug("SCTLR     %08X\n", A32_ReadSCTRL());
    K2OSKERN_Debug("ACTLR     %08X\n", A32_ReadAUXCTRL());
    K2OSKERN_Debug("SCU       %08X\n", *((UINT32*)K2OS_KVA_A32_PERIPHBASE));
    //    K2OSKERN_Debug("SCR       %08X\n", A32_ReadSCR());    // will fault in NS mode
    K2OSKERN_Debug("DACR      %08X\n", A32_ReadDACR());
    K2OSKERN_Debug("TTBCR     %08X\n", A32_ReadTTBCR());
    K2OSKERN_Debug("TTBR0     %08X\n", A32_ReadTTBR0());
    K2OSKERN_Debug("TTBR1     %08X\n", A32_ReadTTBR1());
    K2OSKERN_Debug("CPSR      %08X\n", A32_ReadCPSR());
    K2OSKERN_Debug("------------------\n");
    A32Kern_DumpExecContext(pEx);
}

#if 0
.text.MmioRead32
                0x00002fac       0x40 c:/repo/k2os/src/uefi/Build/QemuQuad/DEBUG_GCC48/ARM/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic/OUTPUT/BaseIoLibIntrinsic.lib(IoLibArm.obj)
                0x00002fac                MmioRead32
 
2FD4

isassembly of section .text.MmioRead32:

00000000 <MmioRead32>:
   0:   b500            push    {lr}
   2:   b085            sub     sp, #20
   4:   9001            str     r0, [sp, #4]
   6:   f7ff fffe       bl      0 <DebugAssertEnabled>
   a:   4603            mov     r3, r0
   c:   2b00            cmp     r3, #0
   e:   d00a            beq.n   26 <MmioRead32+0x26>
  10:   9b01            ldr     r3, [sp, #4]
  12:   f003 0303       and.w   r3, r3, #3
  16:   2b00            cmp     r3, #0
  18:   d005            beq.n   26 <MmioRead32+0x26>
  1a:   4807            ldr     r0, [pc, #28]   ; (38 <MmioRead32+0x38>)
  1c:   f240 2106       movw    r1, #518        ; 0x206
  20:   4a06            ldr     r2, [pc, #24]   ; (3c <MmioRead32+0x3c>)
  22:   f7ff fffe       bl      0 <DebugAssert>
  26:   9b01            ldr     r3, [sp, #4]
  28:   681b            ldr     r3, [r3, #0]
  2a:   9303            str     r3, [sp, #12]
  2c:   9b03            ldr     r3, [sp, #12]
  2e:   4618            mov     r0, r3
  30:   b005            add     sp, #20
  32:   f85d fb04       ldr.w   pc, [sp], #4
  36:   bf00            nop
  38:   00000000        .word   0x00000000
  3c:   0000006c        .word   0x0000006c

#endif

void
KernArch_DumpThreadContext(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    K2OSKERN_OBJREF refMap;
    UINT32          pageIx;
    UINT32          offset;
    UINT32          iter;
    UINT32          dataAddr;

    if (apThread->mIsKernelThread)
    {
        K2OSKERN_Debug("ArmReason %d, fsr = %08X\n", apThread->LastEx.mArchSpec[0], apThread->LastEx.mArchSpec[1]);
        K2OSKERN_Debug("ExCode %08X\n", apThread->LastEx.mExCode);
        K2OSKERN_Debug("WasWrite = %d\n", apThread->LastEx.mWasWrite);
        A32Kern_DumpExceptionContext(apThisCore, apThread->LastEx.mArchSpec[1], &apThread->Kern.ArchExecContext);

        offset = apThread->Kern.ArchExecContext.R[15];

        A32Kern_DumpStackTrace(
            apThisCore,
            NULL,
            offset,
            apThread->Kern.ArchExecContext.R[13],
            (UINT32 *)apThread->Kern.ArchExecContext.R[11],
            apThread->Kern.ArchExecContext.R[14]
        );

        // try to find text segment that the EIP is inside of
        refMap.AsAny = NULL;
        if (KernVirtMap_FindMapAndCreateRef(offset, &refMap, &pageIx))
        {
            K2OSKERN_Debug("Code at PC:\n");
            pageIx = refMap.AsVirtMap->OwnerMapTreeNode.mUserVal;
            K2_ASSERT(pageIx < offset);
            offset -= pageIx;

            for (iter = 0; iter < 8; iter++)
            {
                dataAddr = (UINT32)-1;
                K2DIS_a32(&sgSymDump[apThisCore->mCoreIx * A32_SYM_NAME_MAX_LEN],
                    A32_SYM_NAME_MAX_LEN - 1, (UINT8 const *)pageIx, &offset, &dataAddr);
                K2OSKERN_Debug("  %s\n", &sgSymDump[apThisCore->mCoreIx * A32_SYM_NAME_MAX_LEN]);
            }

            KernObj_ReleaseRef(&refMap);
        }
        K2OSKERN_Debug("\n");
    }
    else
    {
//        K2_ASSERT(0);
    }
}

void
KernArch_Panic(
    K2OSKERN_CPUCORE volatile * apThisCore,
    BOOL                        aDumpStack
)
{
    KernEx_PanicDump(apThisCore);

    if (aDumpStack)
    {
        A32Kern_DumpStackTrace(
            apThisCore,
            NULL,
            (UINT32)KernArch_Panic,
            A32_ReadStackPointer(),
            (UINT32 *)A32_ReadFramePointer(),
            (UINT32)K2_RETURN_ADDRESS
        );
    }

    while (1);

}

