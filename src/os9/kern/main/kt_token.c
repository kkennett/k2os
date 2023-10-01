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

BOOL
K2OS_Token_Destroy(
    K2OS_TOKEN aToken
)
{
    BOOL                        disp;
    K2OSKERN_KERN_TOKEN *       pKernToken;
    K2OSKERN_OBJREF             objRef;
    BOOL                        decToZero;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    BOOL                        doSchedCall;

    pKernToken = (K2OSKERN_KERN_TOKEN *)aToken;
    K2_ASSERT(KERN_TOKEN_SENTINEL1 == pKernToken->mSentinel1);
    K2_ASSERT(KERN_TOKEN_SENTINEL2 == pKernToken->mSentinel2);

    pKernToken->mSentinel1 = 0;
    pKernToken->mSentinel2 = 0;

    objRef.AsAny = NULL;

    disp = K2OSKERN_SeqLock(&gData.Token.SeqLock);

    KernObj_CreateRef(&objRef, pKernToken->Ref.AsAny);

    if (0 == K2ATOMIC_Dec((INT32 volatile *)&pKernToken->Ref.AsAny->mTokenCount))
    {
        decToZero = TRUE;
    }
    else
    {
        decToZero = FALSE;
    }

    KernObj_ReleaseRef(&pKernToken->Ref);

    K2LIST_AddAtTail(&gData.Token.FreeList, (K2LIST_LINK *)pKernToken);

    K2OSKERN_SeqUnlock(&gData.Token.SeqLock, disp);

    if (decToZero)
    {
        //
        // objRef just destroyed a token and that made the token count
        // go to zero.  If this is for a waitable object then anything
        // waiting on it needs to have the waits aborted. also need to
        // deal with case of interface instance leaving
        //
        doSchedCall = FALSE;

        switch (objRef.AsAny->mObjType)
        {
        case KernObj_Gate:
        case KernObj_Notify:
        case KernObj_MailboxOwner:
        case KernObj_IfInst:
            doSchedCall = TRUE;
            break;

        case KernObj_IpcEnd:
            // must already be disconnected 
            K2_ASSERT(objRef.AsIpcEnd->Connection.Locked.mState == KernIpcEndState_Disconnected);
            break;

        default:
            break;
        }

        if (doSchedCall)
        {
            if (objRef.AsAny->mObjType == KernObj_IfInst)
            {
                disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                K2_ASSERT(disp);

                if (objRef.AsIfInst->mIsPublic)
                {
                    objRef.AsIfInst->mIsDeparting = TRUE;
                }
                else
                {
                    doSchedCall = FALSE;
                }

                K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, !doSchedCall);
            }
            else
            {
                disp = K2OSKERN_SetIntr(FALSE);
            }

            if (doSchedCall)
            {
                K2_ASSERT(disp);

                pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
                pThisThread = pThisCore->mpActiveThread;
                K2_ASSERT(pThisThread->mIsKernelThread);

                pSchedItem = &pThisThread->SchedItem;
                pSchedItem->mType = KernSchedItem_KernThread_LastTokenDestroyed;
                KernObj_CreateRef(&pSchedItem->ObjRef, objRef.AsAny);

                KernThread_CallScheduler(pThisCore);

                // interrupts will be back on again here

                K2_ASSERT(NULL == pSchedItem->ObjRef.AsAny);
            }
        }
    }

    KernObj_ReleaseRef(&objRef);

    return TRUE;
}

BOOL
K2OS_Token_Clone(
    K2OS_TOKEN aToken,
    K2OS_TOKEN *apRetNewToken
)
{
    K2OSKERN_OBJREF objRef;
    K2STAT          stat;

    K2_ASSERT(NULL != apRetNewToken);

    objRef.AsAny = NULL;
    stat = KernToken_Translate(aToken, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_VirtMap == objRef.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_NOT_ALLOWED;
        }
        else
        {
            stat = KernToken_Create(objRef.AsAny, apRetNewToken);
        }

        KernObj_ReleaseRef(&objRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

UINT32
K2OS_Token_Share(
    K2OS_TOKEN  aToken,
    UINT32      aProcessId
)
{
    K2STAT          stat;
    K2OSKERN_OBJREF procRef;
    K2OSKERN_OBJREF objRef;
    UINT32          tokenValue;

    // cannot share with yourself
    if ((NULL == aToken) ||
        (0 == aProcessId))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return 0;
    }

    procRef.AsAny = NULL;
    if (!KernProc_FindAddRefById(aProcessId, &procRef))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return 0;
    }

    tokenValue = 0;
    objRef.AsAny = NULL;
    stat = KernToken_Translate(aToken, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        stat = KernObj_Share(NULL, objRef.AsAny, procRef.AsProc, &tokenValue);

        KernObj_ReleaseRef(&objRef);
    }

    KernObj_ReleaseRef(&procRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(0 == tokenValue);
        K2OS_Thread_SetLastStatus(stat);
        return 0;
    }

    K2_ASSERT(0 != tokenValue);

    return tokenValue;
}
