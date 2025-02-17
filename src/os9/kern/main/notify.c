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

    pNotify = (K2OSKERN_OBJ_NOTIFY *)KernObj_Alloc(KernObj_Notify);
    if (NULL == pNotify)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

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
    K2_ASSERT(0 == apNotify->SchedLocked.MacroWaitEntryList.mNodeCount);
    KernObj_Free(&apNotify->Hdr);
}

void    
KernNotify_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT              stat;
    K2OSKERN_OBJREF     notifyRef;
    K2OS_THREAD_PAGE *  pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    notifyRef.AsAny = NULL;
    stat = KernNotify_Create((0 != apCurThread->User.mSysCall_Arg0) ? TRUE : FALSE, &notifyRef);
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
        return;
    }

    stat = KernProc_TokenCreate(apCurThread->RefProc.AsProc, notifyRef.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);

    KernObj_ReleaseRef(&notifyRef);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

