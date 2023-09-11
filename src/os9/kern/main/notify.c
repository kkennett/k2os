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

K2STAT  
KernNotify_Create(
    BOOL                aInitSignalled, 
    K2OSKERN_OBJREF *   apRetNotifyRef
)
{
    K2OSKERN_OBJ_NOTIFY *pNotify;

    pNotify = (K2OSKERN_OBJ_NOTIFY *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_NOTIFY));
    if (NULL == pNotify)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pNotify, sizeof(K2OSKERN_OBJ_NOTIFY));

    pNotify->Hdr.mObjType = KernObj_Notify;
    K2LIST_Init(&pNotify->Hdr.RefObjList);

    pNotify->SchedLocked.mSignalled = aInitSignalled;
    K2LIST_Init(&pNotify->SchedLocked.MacroWaitEntryList);

    KernObj_CreateRef(apRetNotifyRef, &pNotify->Hdr);

    return K2STAT_NO_ERROR;
}

void    
KernNotify_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_NOTIFY *       apNotify
)
{
    K2_ASSERT(0 == apNotify->Hdr.RefObjList.mNodeCount);
    K2_ASSERT(0 == apNotify->SchedLocked.MacroWaitEntryList.mNodeCount);
    K2MEM_Zero(apNotify, sizeof(K2OSKERN_OBJ_NOTIFY));
    KernHeap_Free(apNotify);
}

void    
KernNotify_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         notifyRef;
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    notifyRef.AsAny = NULL;
    stat = KernNotify_Create((0 != apCurThread->User.mSysCall_Arg0) ? TRUE : FALSE, &notifyRef);
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
        return;
    }

    stat = KernProc_TokenCreate(apCurThread->User.ProcRef.AsProc, notifyRef.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);

    KernObj_ReleaseRef(&notifyRef);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernNotify_SysCall_Signal(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         notifyRef;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    notifyRef.AsAny = NULL;
    stat = KernProc_TokenTranslate(
        apCurThread->User.ProcRef.AsProc,
        (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, 
        &notifyRef
    );
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Notify == notifyRef.AsAny->mObjType)
        {
            //
            // this is done through scheduler call because
            // unblocking a thread may cause this thread to
            // never execute another instruction.  so this
            // thread has to not exec again until after the notify is processed
            //
            apCurThread->User.mSysCall_Result = TRUE;
            pSchedItem = &apCurThread->SchedItem;
            pSchedItem->mType = KernSchedItem_Thread_SysCall;
            KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
            KernObj_CreateRef(&pSchedItem->ObjRef, notifyRef.AsAny);
            KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
            KernSched_QueueItem(pSchedItem);
        }
        else
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        KernObj_ReleaseRef(&notifyRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}
