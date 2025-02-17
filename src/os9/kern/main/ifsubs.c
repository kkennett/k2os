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
KernIfSubs_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_IFSUBS *       apIfSubs
)
{
    BOOL disp;

    K2_ASSERT(0 != (apIfSubs->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    //
    // remove from global list is the first step
    //
    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
    K2LIST_Remove(&gData.Iface.SubsList, &apIfSubs->ListLink);
    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    // sanity check - this must be zero or there are outstanding references
    K2_ASSERT(apIfSubs->mBacklogRemaining == apIfSubs->mBacklogInit);

    KernMailbox_UndoReserve(apIfSubs->MailboxRef.AsMailbox, apIfSubs->mBacklogInit);

    KernObj_ReleaseRef(&apIfSubs->MailboxRef);

    KernObj_Free(&apIfSubs->Hdr);
}

K2STAT
KernIfSubs_Create(
    K2OSKERN_OBJ_MAILBOX *  apMailbox,
    UINT32                  aClassCode,
    K2_GUID128 const *      apSpecific,
    UINT32                  aBacklogCount,
    BOOL                    aProcSelfNotify,
    UINT32                  aContext,
    K2OSKERN_OBJREF *       apRetRef
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_IFSUBS *   pSubs;

    K2_ASSERT(NULL != apSpecific);

    pSubs = (K2OSKERN_OBJ_IFSUBS *)KernObj_Alloc(KernObj_IfSubs);
    if (pSubs == NULL)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    if (!KernMailbox_Reserve(apMailbox, aBacklogCount))
    {
        stat = K2STAT_ERROR_OUT_OF_RESOURCES;
        KernObj_Free(&pSubs->Hdr);
    }
    else
    {
        stat = K2STAT_NO_ERROR;

        KernObj_CreateRef(&pSubs->MailboxRef, &apMailbox->Hdr);

        pSubs->mBacklogInit = pSubs->mBacklogRemaining = aBacklogCount;

        pSubs->mProcSelfNotify = aProcSelfNotify;

        pSubs->mClassCode = aClassCode;

        if (!K2MEM_VerifyZero(apSpecific, sizeof(K2_GUID128)))
        {
            pSubs->mHasSpecific = TRUE;
            K2MEM_Copy(&pSubs->Specific, apSpecific, sizeof(K2_GUID128));
        }

        pSubs->mUserContext = aContext;

        KernObj_CreateRef(apRetRef, &pSubs->Hdr);
    }

    return stat;
}

void    
KernIfSubs_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_THREAD_PAGE *      pThreadPage;
    BOOL                    disp;
    K2OSKERN_OBJREF         refMailboxOwner;
    K2OSKERN_OBJREF         refSubs;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    pProc = apCurThread->RefProc.AsProc;

    refMailboxOwner.AsAny = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refMailboxOwner);

    if (!K2STAT_IS_ERROR(stat))
    {
        if (refMailboxOwner.AsAny->mObjType != KernObj_MailboxOwner)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            refSubs.AsAny = NULL;
            stat = KernIfSubs_Create(
                refMailboxOwner.AsMailboxOwner->RefMailbox.AsMailbox,
                pThreadPage->mSysCall_Arg1,
                (K2_GUID128 const *)&pThreadPage->mSysCall_Arg2,
                pThreadPage->mSysCall_Arg6_Result1 & 0x0000FFFF,
                (pThreadPage->mSysCall_Arg6_Result1 & 0xFFFF0000) ? TRUE : FALSE,
                (UINT32)pThreadPage->mSysCall_Arg7_Result0,
                &refSubs
            );

            if (!K2STAT_IS_ERROR(stat))
            {
                stat = KernProc_TokenCreate(pProc, refSubs.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);
                if (K2STAT_IS_ERROR(stat))
                {
                    //
                    // messy manual destruction of mailbox before add to global list
                    //
                    KernMailbox_UndoReserve(
                        refMailboxOwner.AsMailboxOwner->RefMailbox.AsMailbox, 
                        refSubs.AsIfSubs->mBacklogInit
                    );
                    KernObj_ReleaseRef(&refSubs.AsIfSubs->MailboxRef);
                    KernObj_Free(refSubs.AsAny);
                    refSubs.AsAny = NULL;
                }
                else
                {
                    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                    K2LIST_AddAtTail(&gData.Iface.SubsList, &refSubs.AsIfSubs->ListLink);
                    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
                }
            }
        }
        KernObj_ReleaseRef(&refMailboxOwner);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}
