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

#include "ik2osacpi.h"

typedef struct _K2OSACPI_INTR K2OSACPI_INTR;
struct _K2OSACPI_INTR
{
    K2OS_INTERRUPT_TOKEN    mInterruptToken;
    K2OS_IRQ_CONFIG         IrqConfig;
    ACPI_OSD_HANDLER        mfServiceRoutine;
    void *                  mpHandlerContext;
    K2OSKERN_pf_Hook_Key    mIntrHookKey;
    UINT32                  mThreadId;
};

UINT32
AcpiInterruptHandlerThread(
    void *apArgument
)
{
    K2OSACPI_INTR * pIntr;
    K2OS_WaitResult waitResult;

    pIntr = (K2OSACPI_INTR *)apArgument;

    K2OSKERN_IntrVoteIrqEnable(pIntr->mInterruptToken, TRUE);

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, pIntr->mInterruptToken, K2OS_TIMEOUT_INFINITE))
            break;

        pIntr->mfServiceRoutine(pIntr->mpHandlerContext);

        K2OSKERN_IntrDone(pIntr->mInterruptToken);
    } while (1);

    K2OSKERN_IntrVoteIrqEnable(pIntr->mInterruptToken, FALSE);

    K2OS_Token_Destroy(pIntr->mInterruptToken);

    K2OS_Heap_Free(pIntr);

    return 0;
}

KernIntrDispType
AcpiIntrHandler(
    void *              apKey,
    KernIntrActionType  aAction
)
{
    if (aAction == KernIntrAction_AtIrq)
    {
        //
        // pulse the interrupt gate and leave the interrupt disabled
        // as the IST above will handle this threaded
        //
        return KernIntrDisp_Fire_Disable;
    }
    return KernIntrDisp_None;
}

ACPI_STATUS
AcpiOsInstallInterruptHandler(
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context
)
{
    K2OSACPI_INTR *     pIntr;
    BOOL                ok;
    K2OS_THREAD_TOKEN   tokThread;

    pIntr = (K2OSACPI_INTR *)K2OS_Heap_Alloc(sizeof(K2OSACPI_INTR));
    if (NULL == pIntr)
        return AE_ERROR;

    K2MEM_Zero(pIntr, sizeof(K2OSACPI_INTR));

    pIntr->IrqConfig.mSourceIrq = InterruptNumber;
    pIntr->IrqConfig.Line.mIsActiveLow = TRUE;
    pIntr->mfServiceRoutine = ServiceRoutine;
    pIntr->mpHandlerContext = Context;
    pIntr->mIntrHookKey = AcpiIntrHandler;

    ok = K2OSKERN_IrqDefine(&pIntr->IrqConfig);
    K2_ASSERT(ok);

    pIntr->mInterruptToken = K2OSKERN_IrqHook(InterruptNumber, &pIntr->mIntrHookKey);
    if (NULL == pIntr->mInterruptToken)
    {
        K2OS_Heap_Free(pIntr);
        return AE_ERROR;
    }

    tokThread = K2OS_Thread_Create(
        "ACPI Interrupt Handler",
        AcpiInterruptHandlerThread,
        pIntr,
        NULL,
        &pIntr->mThreadId
    );
    if (NULL == tokThread)
    {
        K2OS_Token_Destroy(pIntr->mInterruptToken);
        K2OS_Heap_Free(pIntr);
        return AE_ERROR;
    }

    K2OS_Token_Destroy(tokThread);

    return AE_OK;
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler(
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
    K2_ASSERT(0);   // TBD if necessary
    return AE_ERROR;
}

