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
#include "crt32.h"

static K2OS_CRITSEC     sgSec;
static K2TREE_ANCHOR    sgTree;

void
CrtMailbox_Init(
    void
)
{
    BOOL ok;

    ok = K2OS_CritSec_Init(&sgSec);
    K2_ASSERT(ok);
    if (!ok)
    {
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }
    K2TREE_Init(&sgTree, NULL);
}

CRT_MBOXREF *           
CrtMailbox_CreateRef(
    CRT_MBOXREF *apMailboxRef
)
{
    CRT_MBOXREF *   pNewRef;
    K2TREE_NODE *   pTreeNode;

    pNewRef = (CRT_MBOXREF *)K2OS_Heap_Alloc(sizeof(CRT_MBOXREF));
    if (NULL == pNewRef)
        return NULL;

    K2OS_CritSec_Enter(&sgSec);

    if ((apMailboxRef->mSentinel1 != CRT_MBOXREF_SENTINEL_1) ||
        (apMailboxRef->mSentinel2 != CRT_MBOXREF_SENTINEL_2))
    {
        apMailboxRef = NULL;
    }
    else
    {
        pTreeNode = K2TREE_Find(&sgTree, (UINT32)apMailboxRef);
        if (NULL != pTreeNode)
        {
            K2_ASSERT(pTreeNode == &apMailboxRef->TreeNode);
            K2_ASSERT(apMailboxRef->mUseCount > 0);

            if (!apMailboxRef->mUserDeleted)
            {
                pNewRef->mUseCount = 1;
                pNewRef->mpMailboxOwner = apMailboxRef->mpMailboxOwner;
                pNewRef->mpMailboxOwner->mRefs++;
                pNewRef->mSentinel1 = CRT_MBOXREF_SENTINEL_1;
                pNewRef->mSentinel2 = CRT_MBOXREF_SENTINEL_2;
                pNewRef->TreeNode.mUserVal = (UINT32)pNewRef;
                K2TREE_Insert(&sgTree, pNewRef->TreeNode.mUserVal, &pNewRef->TreeNode);
            }
            else
                apMailboxRef = NULL;
        }
        else
            apMailboxRef = NULL;
    }

    K2OS_CritSec_Leave(&sgSec);

    if (NULL == apMailboxRef)
    {
        K2OS_Heap_Free(pNewRef);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return NULL;
    }

    return pNewRef;
}

K2OS_MAILBOX_OWNER *    
CrtMailboxRef_FindAddUse(
    K2OS_MAILBOX aMailbox
)
{
    UINT32          refAddr;
    CRT_MBOXREF *   pRef;
    K2TREE_NODE *   pTreeNode;

    refAddr = (UINT32)aMailbox;
    if ((refAddr > (K2OS_UVA_TIMER_IOPAGE_BASE - sizeof(CRT_MBOXREF))) ||
        (refAddr < K2OS_UVA_LOW_BASE))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    K2OS_CritSec_Enter(&sgSec);

    pRef = (CRT_MBOXREF *)aMailbox;
    if ((pRef->mSentinel1 != CRT_MBOXREF_SENTINEL_1) ||
        (pRef->mSentinel2 != CRT_MBOXREF_SENTINEL_2))
    {
        pRef = NULL;
    }
    else
    {
        pTreeNode = K2TREE_Find(&sgTree, (UINT32)aMailbox);
        if (NULL != pTreeNode)
        {
            K2_ASSERT(pTreeNode == &pRef->TreeNode);
            K2_ASSERT(pRef->mUseCount > 0);
            if (!pRef->mUserDeleted)
            {
                pRef->mUseCount++;
            }
            else
            {
                pRef = NULL;
            }
        }
        else
        {
            pRef = NULL;
        }
    }

    K2OS_CritSec_Leave(&sgSec);

    if (NULL == pRef)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return NULL;
    }

    return pRef->mpMailboxOwner;
}

BOOL                    
CrtMailboxRef_DecUse(
    K2OS_MAILBOX        aMailbox,
    K2OS_MAILBOX_OWNER *apMailboxOwner,
    BOOL                aIsUserDelete
)
{
    UINT32          refAddr;
    CRT_MBOXREF *   pRef;
    K2TREE_NODE *   pTreeNode;
    BOOL            result;

    refAddr = (UINT32)aMailbox;
    if ((refAddr > (K2OS_UVA_TIMER_IOPAGE_BASE - sizeof(CRT_MBOXREF))) ||
        (refAddr < K2OS_UVA_LOW_BASE))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2OS_CritSec_Enter(&sgSec);

    pRef = (CRT_MBOXREF *)aMailbox;
    if ((pRef->mSentinel1 != CRT_MBOXREF_SENTINEL_1) ||
        (pRef->mSentinel2 != CRT_MBOXREF_SENTINEL_2))
    {
        pRef = NULL;
    }
    else
    {
        pTreeNode = K2TREE_Find(&sgTree, (UINT32)aMailbox);
        if (NULL != pTreeNode)
        {
            K2_ASSERT(pTreeNode == &pRef->TreeNode);
            K2_ASSERT(pRef->mUseCount > 0);
            if (apMailboxOwner == NULL)
            {
                apMailboxOwner = pRef->mpMailboxOwner;
            }
            else
            {
                K2_ASSERT(pRef->mpMailboxOwner == apMailboxOwner);
            }
            if (aIsUserDelete)
            {
                K2_ASSERT(!pRef->mUserDeleted);
                pRef->mUserDeleted = TRUE;
            }
            if (0 != --pRef->mUseCount)
            {
                pRef = NULL;
            }
            else
            {
                K2TREE_Remove(&sgTree, &pRef->TreeNode);
                pRef->mSentinel1 = 0;
                pRef->mSentinel2 = 0;
                pRef->mpMailboxOwner = NULL;
                if (0 != --apMailboxOwner->mRefs)
                    apMailboxOwner = NULL;
            }
        }
        else
        {
            pRef = NULL;
        }
    }
    K2OS_CritSec_Leave(&sgSec);

    if (NULL == pTreeNode)
        return FALSE;       // was not found

    if (NULL == pRef)
        return TRUE;        // was found but ref was not destroyed

    K2OS_Heap_Free(pRef);

    if ((NULL != apMailboxOwner) &&
        (0 == apMailboxOwner->mRefs))
    {
        result = CrtKern_SysCall1(K2OS_SYSCALL_ID_MAILBOX_CLOSE, (UINT_PTR)apMailboxOwner->mMailboxToken);
        K2_ASSERT(result);
        K2_ASSERT(0 == apMailboxOwner->mSentinel1);
        K2_ASSERT(0 == apMailboxOwner->mSentinel2);
        K2_ASSERT(NULL == apMailboxOwner->mMailboxToken);
        K2OS_Heap_Free(apMailboxOwner);
    }

    return TRUE;
}

K2OS_MAILBOX    
K2_CALLCONV_REGS 
K2OS_Mailbox_Create(
    K2OS_MAILSLOT_TOKEN *apRetTokMailslot
)
{
    CRT_MBOXREF *           pRef;
    K2OS_MAILBOX_OWNER *    pMailboxOwner;
    K2OS_MAILBOX_TOKEN      tokMailbox;
    K2OS_MAILSLOT_TOKEN     tokMailslot;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pRef = (CRT_MBOXREF *)K2OS_Heap_Alloc(sizeof(CRT_MBOXREF));
    if (NULL == pRef)
        return NULL;

    do
    {
        K2MEM_Zero(pRef, sizeof(CRT_MBOXREF));

        tokMailbox = NULL;

        pMailboxOwner = (K2OS_MAILBOX_OWNER *)K2OS_Heap_Alloc(sizeof(K2OS_MAILBOX_OWNER));
        if (NULL == pMailboxOwner)
            break;

        K2MEM_Zero(pMailboxOwner, sizeof(K2OS_MAILBOX_OWNER));

        do
        {
            tokMailbox = (K2OS_MAILBOX_TOKEN)CrtKern_SysCall1(K2OS_SYSCALL_ID_MAILBOX_CREATE, (UINT_PTR)pMailboxOwner);
            if (NULL == tokMailbox)
                break;

            pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
            tokMailslot = (K2OS_MAILSLOT_TOKEN)pThreadPage->mSysCall_Arg7_Result0;
            K2_ASSERT(NULL != tokMailslot);

            K2_ASSERT(tokMailbox == pMailboxOwner->mMailboxToken);
            pRef->mUseCount = 1;
            pMailboxOwner->mRefs = 1;
            pRef->mpMailboxOwner = pMailboxOwner;
            pRef->TreeNode.mUserVal = (UINT32)pRef;
            K2OS_CritSec_Enter(&sgSec);
            pRef->mSentinel1 = CRT_MBOXREF_SENTINEL_1;
            pRef->mSentinel2 = CRT_MBOXREF_SENTINEL_2;
            K2TREE_Insert(&sgTree, pRef->TreeNode.mUserVal, &pRef->TreeNode);
            K2OS_CritSec_Leave(&sgSec);
        } while (0);

        if (NULL == tokMailbox)
        {
            K2OS_Heap_Free(pMailboxOwner);
        }

    } while (0);

    if (NULL == tokMailbox)
    {
        K2OS_Heap_Free(pRef);
        pRef = NULL;
    }
    else
    {
        if (NULL != apRetTokMailslot)
        {
            *apRetTokMailslot = tokMailslot;
        }
        else
        {
            K2OS_Token_Destroy(tokMailslot);
        }
    }

    return (K2OS_MAILBOX)pRef;
}

BOOL            
K2_CALLCONV_REGS 
K2OS_Mailbox_Delete(
    K2OS_MAILBOX aMailbox
)
{
    return CrtMailboxRef_DecUse(aMailbox, NULL, TRUE);
}

BOOL            
K2_CALLCONV_REGS 
K2OS_Mailslot_Send(
    K2OS_MAILSLOT_TOKEN aSlot,
    K2OS_MSG const *    apMsg
)
{
    if ((NULL == aSlot) ||
        (NULL == apMsg))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    return (BOOL)CrtKern_SysCall5(K2OS_SYSCALL_ID_MAILSLOT_SEND,
        (UINT_PTR)aSlot,
        apMsg->mControl,
        apMsg->mPayload[0],
        apMsg->mPayload[1],
        apMsg->mPayload[2]);
}
