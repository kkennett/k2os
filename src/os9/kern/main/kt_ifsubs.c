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

K2OS_IFSUBS_TOKEN
K2OS_IfSubs_Create(
    K2OS_MAILBOX_TOKEN  aTokMailboxOwner, 
    UINT32              aClassCode, 
    K2_GUID128 const *  apSpecific, 
    UINT32              aBacklogCount, 
    BOOL                aProcSelfNotify, 
    void *              apContext
)
{
    K2OSKERN_OBJREF     refMailboxOwner;
    K2OS_IFSUBS_TOKEN   tokResult;
    K2STAT              stat;
    K2OSKERN_OBJREF     refSubs;
    K2_GUID128          guid;
    BOOL                disp;

    if (NULL == aTokMailboxOwner)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    if (NULL == apSpecific)
    {
        K2MEM_Zero(&guid, sizeof(guid));
        apSpecific = &guid;
    }

    tokResult = NULL;

    refMailboxOwner.AsAny = NULL;
    stat = KernToken_Translate(aTokMailboxOwner, &refMailboxOwner);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    do {
        if (refMailboxOwner.AsAny->mObjType != KernObj_MailboxOwner)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            refSubs.AsAny = NULL;
            stat = KernIfSubs_Create(
                refMailboxOwner.AsMailboxOwner->RefMailbox.AsMailbox,
                aClassCode,
                apSpecific,
                aBacklogCount,
                aProcSelfNotify,
                (UINT32)apContext,
                &refSubs
            );
            if (!K2STAT_IS_ERROR(stat))
            {
                stat = KernToken_Create(refSubs.AsAny, &tokResult);
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

                    KernObj_ReleaseRef(&refSubs);
                }
            }
        }

    } while (0);

    KernObj_ReleaseRef(&refMailboxOwner);

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL == tokResult);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    K2_ASSERT(NULL != tokResult);

    return tokResult;
}
