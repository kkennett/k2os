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
KernGate_Create(
    BOOL                aInitOpen, 
    K2OSKERN_OBJREF *   apRetGateRef
)
{
    K2OSKERN_OBJ_GATE *pGate;

    pGate = (K2OSKERN_OBJ_GATE *)KernObj_Alloc(KernObj_Gate);
    if (NULL == pGate)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pGate->SchedLocked.mOpen = aInitOpen;
    K2LIST_Init(&pGate->SchedLocked.MacroWaitEntryList);

    KernObj_CreateRef(apRetGateRef, &pGate->Hdr);

    return K2STAT_NO_ERROR;
}

void    
KernGate_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_GATE *         apGate
)
{
    K2_ASSERT(0 == apGate->SchedLocked.MacroWaitEntryList.mNodeCount);
    KernObj_Free(&apGate->Hdr);
}

void    
KernGate_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         gateRef;
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    gateRef.AsAny = NULL;
    stat = KernGate_Create((0 != apCurThread->User.mSysCall_Arg0) ? TRUE : FALSE, &gateRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        stat = KernProc_TokenCreate(apCurThread->RefProc.AsProc, gateRef.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);
        KernObj_ReleaseRef(&gateRef);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}


