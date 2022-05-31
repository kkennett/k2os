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

#include "kern.h"

void    
KernProc_SysCall_Root_Launch(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pCallerProc;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT32                  ix;
    UINT32                  lockMapCount;
    K2OSKERN_OBJREF         lockedMapRefs[2];
    UINT32                  map0FirstPageIx;
    CRT_LAUNCH_INFO *       pLaunchInfo;
    K2OSKERN_OBJREF         newProcRef;
    K2OS_PROCESS_TOKEN      tokProc;
    K2OSKERN_OBJ_THREAD *   pThread;
    BOOL                    disp;

    pCallerProc = apCurThread->ProcRef.Ptr.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    if (pCallerProc->mId != 1)
    {
        pThreadPage->mLastStatus = K2STAT_ERROR_NOT_ALLOWED;
        apCurThread->mSysCall_Result = 0;
        return;
    }

    K2MEM_Zero(lockedMapRefs, sizeof(K2OSKERN_OBJREF) * 2);

    stat = KernProc_AcqMaps(
        pCallerProc, 
        apCurThread->mSysCall_Arg0, 
        sizeof(CRT_LAUNCH_INFO), 
        FALSE, 
        &lockMapCount, 
        lockedMapRefs, 
        &map0FirstPageIx
    );

    if (!K2STAT_IS_ERROR(stat))
    {
        pLaunchInfo = (CRT_LAUNCH_INFO *)KernHeap_Alloc(sizeof(CRT_LAUNCH_INFO));
        if (NULL == pLaunchInfo)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            K2MEM_Copy(pLaunchInfo, (void const *)apCurThread->mSysCall_Arg0, sizeof(CRT_LAUNCH_INFO));
            newProcRef.Ptr.AsHdr = NULL;
            stat = KernProc_Create(apThisCore, pLaunchInfo, &newProcRef);
            if (!K2STAT_IS_ERROR(stat))
            {
                //
                // launch info owned by new process now.
                // new process is in launching state
                //
                stat = KernProc_TokenCreate(pCallerProc, newProcRef.Ptr.AsHdr, &tokProc);
                if (!K2STAT_IS_ERROR(stat))
                {
                    apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
                    KernTimer_GetHfTick(&apCurThread->SchedItem.mHfTick);
                    apCurThread->SchedItem.Args.Process_Create.mProcessToken = (UINT32)tokProc;
                    KernObj_CreateRef(&apCurThread->SchedItem.ObjRef, newProcRef.Ptr.AsHdr);
                    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                    KernSched_QueueItem(&apCurThread->SchedItem);
                }
                else
                {
                    K2_ASSERT(newProcRef.Ptr.AsProc->Thread.Locked.CreateList.mpHead != NULL);
                    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, newProcRef.Ptr.AsProc->Thread.Locked.CreateList.mpHead, ProcThreadListLink);
                    pThread->mState = KernThreadState_Exited;
                    pThread->mExitCode = stat;
                    disp = K2OSKERN_SeqLock(&newProcRef.Ptr.AsProc->Thread.SeqLock);
                    K2LIST_Remove(&newProcRef.Ptr.AsProc->Thread.Locked.CreateList, &pThread->ProcThreadListLink);
                    K2LIST_AddAtTail(&newProcRef.Ptr.AsProc->Thread.Locked.DeadList, &pThread->ProcThreadListLink);
                    K2OSKERN_SeqUnlock(&newProcRef.Ptr.AsProc->Thread.SeqLock, disp);
                    newProcRef.Ptr.AsProc->mState = KernProcState_Stopped;
                    KernThread_Cleanup(apThisCore, pThread);
                }

                KernObj_ReleaseRef(&newProcRef);
            }
            else
            {
                KernHeap_Free(pLaunchInfo);
            }
        }

        for (ix = 0; ix < lockMapCount; ix++)
        {
            KernObj_ReleaseRef(&lockedMapRefs[ix]);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        apCurThread->mSysCall_Result = 0;
    }
}
