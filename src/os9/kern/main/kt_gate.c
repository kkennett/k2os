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

#include "kern.h"

K2OS_GATE_TOKEN
K2OS_Gate_Create(
    BOOL aInitOpen
)
{
    K2OSKERN_OBJREF ref;
    K2STAT          stat;
    K2OS_TOKEN      result;

    ref.AsAny = NULL;
    stat = KernGate_Create(aInitOpen, &ref);
    if (K2STAT_IS_ERROR(stat))
    {
        return NULL;
    }

    result = NULL;
    stat = KernToken_Create(ref.AsAny, &result);

    KernObj_ReleaseRef(&ref);

    if (K2STAT_IS_ERROR(stat))
    {
        return NULL;
    }

    return result;
}

BOOL
KernGate_Threaded_KernelChange(
    K2OS_GATE_TOKEN aTokGate, 
    BOOL            aNowOpen
)
{

    K2STAT                      stat;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_OBJREF             gateRef;
    BOOL                        disp;

    if (NULL == aTokGate)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_TLSAREA_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    stat = KernToken_Translate(aTokGate, &gateRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Gate == gateRef.AsAny->mObjType)
        {
            //
            // this is done through scheduler call because
            // unblocking a thread may cause this thread to
            // never execute another instruction.  so this
            // thread has to not exec again until after the Gate is processed
            //
            pSchedItem = &pThisThread->SchedItem;
            pSchedItem->mType = KernSchedItem_KernThread_ChangeGate;
            pSchedItem->Args.Gate_Change.mSetTo = aNowOpen;
            KernObj_CreateRef(&pSchedItem->ObjRef, gateRef.AsAny);

            disp = K2OSKERN_SetIntr(FALSE);
            K2_ASSERT(disp);

            pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

            KernThread_CallScheduler(pThisCore);

            // interrupts will be back on again here

            KernObj_ReleaseRef(&pSchedItem->ObjRef);

            if (pThisThread->Kern.mSchedCall_Result)
            {
                stat = K2STAT_NO_ERROR;
            }
            else
            {
                stat = pThreadPage->mLastStatus;
                K2_ASSERT(K2STAT_IS_ERROR(stat));
            }
        }
        else
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }

        KernObj_ReleaseRef(&gateRef);
    }

    return (K2STAT_IS_ERROR(stat) ? FALSE : TRUE);
}

BOOL
K2OS_Gate_Open(
    K2OS_GATE_TOKEN aTokGate
)
{
    return KernGate_Threaded_KernelChange(aTokGate, TRUE);
}

BOOL
K2OS_Gate_Close(
    K2OS_GATE_TOKEN aTokGate
)
{
    return KernGate_Threaded_KernelChange(aTokGate, FALSE);
}
