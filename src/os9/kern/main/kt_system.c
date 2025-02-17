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

void
KernSystem_CreateProc_Dpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2STAT                  stat;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(NULL != pThread->Kern.mpArg);

    pThread->Kern.ResultRef.AsAny = NULL;

    stat = KernProc_Create(apThisCore, 
            (K2OS_PROC_START_INFO *)pThread->Kern.mpArg, 
            &pThread->Kern.ResultRef
        );

    if (!K2STAT_IS_ERROR(stat))
    {
        pThread->Kern.mSchedCall_Result = TRUE;
    }
    else
    {
        pThread->Kern.mSchedCall_Result = FALSE;
        pThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}

K2OS_PROCESS_TOKEN
K2OS_System_CreateProcess(
    char const *    apFilePath,
    char const *    apArgs,
    UINT32 *        apRetId
)
{
    K2OS_PROC_START_INFO *      pLaunchInfo;
    K2OS_PROCESS_TOKEN          tokProc;
    UINT32                      count;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    K2STAT                      stat;

    if ((NULL == apFilePath) ||
        (0 == *apFilePath))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    pLaunchInfo = (K2OS_PROC_START_INFO *)KernHeap_Alloc(sizeof(K2OS_PROC_START_INFO));
    if (NULL == pLaunchInfo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    if (NULL != apArgs)
    {
        count = K2ASC_CopyLen(pLaunchInfo->mArgStr, apArgs, 1023);
        pLaunchInfo->mArgStr[count] = 0;
    }
    else
    {
        pLaunchInfo->mArgStr[0] = 0;
    }

    /*-------------------------------------*/
    disp = K2OSKERN_SetIntr(FALSE);
    K2_ASSERT(disp);
    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    pThisThread = pThisCore->mpActiveThread;
    K2_ASSERT(pThisThread->mIsKernelThread);
    pThisThread->Kern.mpArg = pLaunchInfo;
    pThisThread->Kern.ResultRef.AsAny = NULL;
    pThisThread->Hdr.ObjDpc.Func = KernSystem_CreateProc_Dpc;
    KernCpu_QueueDpc(&pThisThread->Hdr.ObjDpc.Dpc, &pThisThread->Hdr.ObjDpc.Func, KernDpcPrio_Med);

    KernArch_IntsOff_SaveKernelThreadStateAndEnterMonitor(pThisCore, pThisThread);
    //
    // this is return point from entering the monitor to do the process create
    // interrupts will be on
    K2_ASSERT(K2OSKERN_GetIntr());
    /*-------------------------------------*/

    if (0 == pThisThread->Kern.mSchedCall_Result)
    {
        KernHeap_Free(pLaunchInfo);
        return NULL;
    }

    // new process reference is in pThisThread->ResultRef
    K2_ASSERT(NULL != pThisThread->Kern.ResultRef.AsAny);
    K2_ASSERT(KernObj_Process == pThisThread->Kern.ResultRef.AsAny->mObjType);
    if (NULL != apRetId)
    {
        *apRetId = pThisThread->Kern.ResultRef.AsProc->mId;
    }

    stat = KernToken_Create(pThisThread->Kern.ResultRef.AsAny, &tokProc);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    // actually start the process now
    // this will release the ResultRef reference

    pSchedItem = &pThisThread->SchedItem;
    pSchedItem->mSchedItemType = KernSchedItem_KernThread_StartProc;

    disp = K2OSKERN_SetIntr(FALSE);
    K2_ASSERT(disp);
    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    KernThread_CallScheduler(pThisCore);
    // interrupts will be back on again here

    K2_ASSERT(NULL == pThisThread->Kern.ResultRef.AsAny);
    
    return tokProc;
}

