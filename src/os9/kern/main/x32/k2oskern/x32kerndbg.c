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

#include "x32kern.h"
#include <lib/k2dis.h>

static void
sEmitSymbolName(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aAddr,
    char *                  pBuffer
)
{
    *pBuffer = 0;
    KernXdl_FindClosestSymbol(apProc, aAddr, pBuffer, X32_SYM_NAME_MAX_LEN);
    if (*pBuffer == 0)
    {
        K2OSKERN_Debug("?(%08X)", aAddr);
        return;
    }
    K2OSKERN_Debug("%s", pBuffer);
}

void
X32Kern_DumpStackTrace(
    K2OSKERN_OBJ_PROCESS *apProc,
    UINT32 aEIP,
    UINT32 aEBP,
    UINT32 aESP,
    char *  apBuffer
)
{
    UINT32 *pBackPtr;

    K2OSKERN_Debug("StackTrace:\n");
    K2OSKERN_Debug("------------------\n");
    K2OSKERN_Debug("Proc     %08X (%d)\n", apProc, (NULL == apProc) ? 0 : apProc->mId);
    K2OSKERN_Debug("ESP      %08X\n", aESP);
    K2OSKERN_Debug("EIP      %08X ", aEIP);
    sEmitSymbolName(apProc, aEIP, apBuffer);
    K2OSKERN_Debug("\n");

    pBackPtr = &aEBP;
    do {
        pBackPtr = (UINT32 *)pBackPtr[0];
        K2OSKERN_Debug("%08X ", pBackPtr);
        if (pBackPtr == NULL)
        {
            K2OSKERN_Debug("--X\n");
            return;
        }
        K2OSKERN_Debug("%08X ", pBackPtr[1]);
        if (pBackPtr[1] == 0)
        {
            K2OSKERN_Debug("--X\n");
            break;
        }
        sEmitSymbolName(apProc, pBackPtr[1], apBuffer);
        K2OSKERN_Debug("\n");
    } while (1);
}

static void
sDumpCommonExContext(
    K2OSKERN_ARCH_EXEC_CONTEXT *apContext
)
{
    K2OSKERN_Debug("Vector  0x%08X\n", apContext->Exception_Vector);
    K2OSKERN_Debug("ErrCode 0x%08X", apContext->Exception_ErrorCode);
    if (apContext->Exception_Vector == X32_EX_PAGE_FAULT)
    {
        K2OSKERN_Debug(" PAGE FAULT %sMode %sPresent %s\n",
            (apContext->Exception_ErrorCode & X32_EX_PAGE_FAULT_FROM_USER) ? "User" : "Kernel",
            (apContext->Exception_ErrorCode & X32_EX_PAGE_FAULT_PRESENT) ? "" : "Not",
            (apContext->Exception_ErrorCode & X32_EX_PAGE_FAULT_ON_WRITE) ? "Write" : "Read");
    }
    else
    {
        K2OSKERN_Debug("\n");
    }
    K2OSKERN_Debug("EAX     0x%08X\n", apContext->REGS.EAX);
    K2OSKERN_Debug("ECX     0x%08X\n", apContext->REGS.ECX);
    K2OSKERN_Debug("EDX     0x%08X\n", apContext->REGS.EDX);
    K2OSKERN_Debug("EBX     0x%08X\n", apContext->REGS.EBX);
    K2OSKERN_Debug("ESP@psh 0x%08X\n", apContext->REGS.ESP_Before_PushA);
    K2OSKERN_Debug("EBP     0x%08X\n", apContext->REGS.EBP);
    K2OSKERN_Debug("ESI     0x%08X\n", apContext->REGS.ESI);
    K2OSKERN_Debug("EDI     0x%08X\n", apContext->REGS.EDI);
    K2OSKERN_Debug("DS      0x%08X\n", apContext->DS);
}

void
X32Kern_DumpUserModeExceptionContext(
    K2OSKERN_ARCH_EXEC_CONTEXT *apContext
)
{
    K2OSKERN_Debug("------------------\n");
    K2OSKERN_Debug("UserMode Exception Context @ 0x%08X\n", apContext);
    sDumpCommonExContext(apContext);
    K2OSKERN_Debug("EIP     0x%08X\n", apContext->EIP);
    K2OSKERN_Debug("CS      0x%08X\n", apContext->CS);
    K2OSKERN_Debug("EFLAGS  0x%08X\n", apContext->EFLAGS);
    K2OSKERN_Debug("U-ESP   0x%08X\n", apContext->ESP);
    K2OSKERN_Debug("SS      0x%08X\n", apContext->SS);
}

void
X32Kern_DumpKernelModeExceptionContext(
    K2OSKERN_ARCH_EXEC_CONTEXT * apContext
)
{
    K2OSKERN_Debug("------------------\n");
    sDumpCommonExContext(apContext);
    K2OSKERN_Debug("EIP     0x%08X\n", apContext->EIP);
    K2OSKERN_Debug("CS      0x%08X\n", apContext->CS);
    K2OSKERN_Debug("EFLAGS  0x%08X\n", apContext->EFLAGS);
    K2OSKERN_Debug("------------------\n");
}

void
DumpOneThread(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    K2OSKERN_Debug("    Process %d Thread %d\n", (apThread->RefProc.AsProc == NULL) ? 0 : apThread->RefProc.AsProc->mId, apThread->mGlobalIx);
    K2OSKERN_Debug("        Name [%s]\n", apThread->mName);
    KernArch_DumpThreadContext(apThisCore, apThread);
    K2OSKERN_Debug("        -------------\n");
}

void
DumpThreads(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJ_THREAD *   pThread;
    K2LIST_LINK *           pListProc;
    K2LIST_LINK *           pListThread;

    pListThread = gData.Thread.CreateList.mpHead;
    if (NULL != pListThread)
    {
        K2OSKERN_Debug("KERNEL THREADS IN CREATE LIST:\n");
        do
        {
            pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListThread, OwnerThreadListLink);
            DumpOneThread(apThisCore, pThread);
            pListThread = pListThread->mpNext;
        } while (NULL != pListThread);
    }
    pListThread = gData.Thread.ActiveList.mpHead;
    if (NULL != pListThread)
    {
        K2OSKERN_Debug("KERNEL THREADS IN ACTIVE LIST:\n");
        do
        {
            pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListThread, OwnerThreadListLink);
            DumpOneThread(apThisCore, pThread);
            pListThread = pListThread->mpNext;
        } while (NULL != pListThread);
    }
    pListThread = gData.Thread.DeadList.mpHead;
    if (NULL != pListThread)
    {
        K2OSKERN_Debug("KERNEL THREADS IN DEAD LIST:\n");
        do
        {
            pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListThread, OwnerThreadListLink);
            DumpOneThread(apThisCore, pThread);
            pListThread = pListThread->mpNext;
        } while (NULL != pListThread);
    }

    pListProc = gData.Proc.List.mpHead;
    if (NULL != pListProc)
    {
        do
        {
            pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, pListProc, GlobalProcListLink);
            pListThread = pProc->Thread.Locked.CreateList.mpHead;
            if (NULL != pListThread)
            {
                K2OSKERN_Debug("PROCESS %d THREADS IN CREATE LIST:\n", pProc->mId);
                do
                {
                    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListThread, OwnerThreadListLink);
                    DumpOneThread(apThisCore, pThread);
                    pListThread = pListThread->mpNext;
                } while (NULL != pListThread);
            }
            pListThread = pProc->Thread.SchedLocked.ActiveList.mpHead;
            if (NULL != pListThread)
            {
                K2OSKERN_Debug("PROCESS %d THREADS IN ACTIVE LIST:\n", pProc->mId);
                do
                {
                    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListThread, OwnerThreadListLink);
                    DumpOneThread(apThisCore, pThread);
                    pListThread = pListThread->mpNext;
                } while (NULL != pListThread);
            }
            pListThread = pProc->Thread.Locked.DeadList.mpHead;
            if (NULL != pListThread)
            {
                K2OSKERN_Debug("PROCESS %d THREADS IN DEAD LIST:\n", pProc->mId);
                do
                {
                    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListThread, OwnerThreadListLink);
                    DumpOneThread(apThisCore, pThread);
                    pListThread = pListThread->mpNext;
                } while (NULL != pListThread);
            }
            pListProc = pListProc->mpNext;
        } while (NULL != pListProc);
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
        X32Kern_DumpStackTrace(
            NULL,
            (UINT32)K2_RETURN_ADDRESS,
            X32_ReadEBP(),
            X32_ReadESP(),
            &gX32Kern_SymDump[apThisCore->mCoreIx * X32_SYM_NAME_MAX_LEN]
        );
    }

//    DumpThreads(apThisCore);

    while (1);
}

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
        X32Kern_DumpKernelModeExceptionContext((K2OSKERN_ARCH_EXEC_CONTEXT *)apThread->Kern.mStackPtr);

        offset = ((K2OSKERN_ARCH_EXEC_CONTEXT *)apThread->Kern.mStackPtr)->EIP;

        X32Kern_DumpStackTrace(
            NULL,
            offset,
            ((K2OSKERN_ARCH_EXEC_CONTEXT *)apThread->Kern.mStackPtr)->REGS.EBP,
            ((K2OSKERN_ARCH_EXEC_CONTEXT *)apThread->Kern.mStackPtr)->REGS.ESP_Before_PushA,
            &gX32Kern_SymDump[apThisCore->mCoreIx * X32_SYM_NAME_MAX_LEN]
        );

        // try to find text segment that the EIP is inside of
        refMap.AsAny = NULL;
        if (KernVirtMap_FindMapAndCreateRef(offset, &refMap, &pageIx))
        {
            K2OSKERN_Debug("Code at EIP:\n");
            pageIx = refMap.AsVirtMap->OwnerMapTreeNode.mUserVal;
            K2_ASSERT(pageIx < offset);
            offset -= pageIx;

            for (iter = 0; iter < 8; iter++)
            {
                dataAddr = (UINT32)-1;
                K2DIS_x32(&gX32Kern_SymDump[apThisCore->mCoreIx * X32_SYM_NAME_MAX_LEN],
                    X32_SYM_NAME_MAX_LEN - 1, (UINT8 const *)pageIx, &offset, &dataAddr);
                K2OSKERN_Debug("  %s\n", &gX32Kern_SymDump[apThisCore->mCoreIx * X32_SYM_NAME_MAX_LEN]);
            }

            KernObj_ReleaseRef(&refMap);
        }
        K2OSKERN_Debug("\n");
    }
    else
    {
//        X32Kern_DumpUserModeExceptionContext(&apThread->User.ArchExecContext);
        KernArch_SetCoreToProcess(apThisCore, apThread->RefProc.AsProc);
        X32Kern_DumpStackTrace(apThread->RefProc.AsProc,
            apThread->User.ArchExecContext.EIP,
            apThread->User.ArchExecContext.REGS.EBP,
            apThread->User.ArchExecContext.ESP,
            &gX32Kern_SymDump[apThisCore->mCoreIx * X32_SYM_NAME_MAX_LEN]
        );
    }
}
