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
K2OSKERN_IrqDefine(
    K2OS_IRQ_CONFIG const * apConfig
)
{
    K2OSKERN_IRQ *  pIrq;
    K2TREE_NODE *   pTreeNode;
    K2OSKERN_IRQ *  pOtherIrq;
    BOOL            disp;
    BOOL            result;

    pIrq = (K2OSKERN_IRQ *)KernHeap_Alloc(sizeof(K2OSKERN_IRQ));
    if (NULL == pIrq)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_MEMORY);
        return FALSE;
    }

    K2MEM_Copy(&pIrq->Config, apConfig, sizeof(K2OS_IRQ_CONFIG));
    K2LIST_Init(&pIrq->InterruptList);

    result = FALSE;

    disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);

    pTreeNode = K2TREE_Find(&gData.Intr.Locked.IrqTree, apConfig->mSourceIrq);
    if (NULL == pTreeNode)
    {
//        K2OSKERN_Debug("Create IRQ %d\n", pIrq->Config.mSourceIrq);
        pIrq->IrqTreeNode.mUserVal = pIrq->Config.mSourceIrq;
        K2TREE_Insert(&gData.Intr.Locked.IrqTree, pIrq->IrqTreeNode.mUserVal, &pIrq->IrqTreeNode);
        result = TRUE;
        KernArch_InstallDevIrqHandler(pIrq);
    }
    else
    {
        pOtherIrq = K2_GET_CONTAINER(K2OSKERN_IRQ, pTreeNode, IrqTreeNode);
        if (pOtherIrq->Config.mDestCoreIx != pIrq->Config.mDestCoreIx)
        {
            K2OSKERN_Debug("***Attempt to define irq %d with two different destCore settings. Failed.\n");
        }
        else if (pOtherIrq->Config.Line.mIsActiveLow != pIrq->Config.Line.mIsActiveLow)
        {
            K2OSKERN_Debug("***Attempt to define irq %d with two different levels. Failed.\n");
        }
        else if (pOtherIrq->Config.Line.mIsEdgeTriggered != pIrq->Config.Line.mIsEdgeTriggered)
        {
            K2OSKERN_Debug("***Attempt to define irq %d with two different trigger types\n");
        }
        else
        {
//            K2OSKERN_Debug("Using Existing IRQ %d\n", pIrq->Config.mSourceIrq);
            result = TRUE;
        }
    }

    K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);

    if (!result)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_IN_USE);
        KernHeap_Free(pIrq);
    }

    return result;
}

K2OS_INTERRUPT_TOKEN 
K2OSKERN_IrqHook(
    UINT32                  aSourceIrq,
    K2OSKERN_pf_Hook_Key *  apKey
)
{
    K2OSKERN_OBJ_INTERRUPT *    pIntr;
    K2STAT                      stat;
    BOOL                        disp;
    K2TREE_NODE *               pTreeNode;
    K2OSKERN_IRQ *              pIrq;
    K2OS_INTERRUPT_TOKEN        tokResult;

    if (NULL == apKey)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    pIntr = (K2OSKERN_OBJ_INTERRUPT *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_INTERRUPT));
    if (NULL == pIntr)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    K2MEM_Zero(pIntr, sizeof(K2OSKERN_OBJ_INTERRUPT));
    pIntr->Hdr.mObjType = KernObj_Interrupt;
    K2LIST_Init(&pIntr->Hdr.RefObjList);

    tokResult = NULL;

    pIntr->mpHookKey = apKey;

    stat = KernGate_Create(FALSE, &pIntr->GateRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);
        pTreeNode = K2TREE_Find(&gData.Intr.Locked.IrqTree, aSourceIrq);
        if (NULL == pTreeNode)
        {
            stat = K2STAT_ERROR_NOT_FOUND;
        }
        else
        {
            tokResult = NULL;
            stat = KernToken_Create(&pIntr->Hdr, &tokResult);
            if (!K2STAT_IS_ERROR(stat))
            {
                pIrq = K2_GET_CONTAINER(K2OSKERN_IRQ, pTreeNode, IrqTreeNode);
                K2LIST_AddAtTail(&pIrq->InterruptList, &pIntr->IrqInterruptListLink);
                pIntr->mpParentIrq = pIrq;
            }
        }

        K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        KernHeap_Free(pIntr);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    K2_ASSERT(NULL != tokResult);

    return tokResult;
}

BOOL 
K2OSKERN_IrqSetEnable(
    UINT32  aSourceIrq,
    BOOL    aSetEnable
)
{
    K2_ASSERT(0);
    return FALSE;
}

static 
void
IntrLocked_IntrEnabled_CheckIrqEnable(
    K2OSKERN_IRQ *  apIrq
)
{
    K2LIST_LINK *               pListLink;
    K2OSKERN_OBJ_INTERRUPT *    pIntr;

    K2_ASSERT(0 != apIrq->InterruptList.mNodeCount);

    if ((!apIrq->Config.Line.mIsEdgeTriggered) && (1 < apIrq->InterruptList.mNodeCount))
    {
        //
        // level triggered irq with more than one thing on it
        //
        pListLink = apIrq->InterruptList.mpHead;
        K2_ASSERT(NULL != pListLink);
        do {
            pIntr = K2_GET_CONTAINER(K2OSKERN_OBJ_INTERRUPT, pListLink, IrqInterruptListLink);

            if (!pIntr->mVoteForEnabled)
            {
                // this intr still voting the irq not be enabled
                return;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    //
    // everybody on this irq is voting for it to be enabled
    //
    KernArch_SetDevIrqMask(apIrq, FALSE);
}

BOOL
K2OSKERN_IntrVoteIrqEnable(
    K2OS_INTERRUPT_TOKEN    aTokIntr,
    BOOL                    aSetEnable
)
{
    K2OSKERN_OBJREF             refIntr;
    K2STAT                      stat;
    BOOL                        disp;
    K2OSKERN_IRQ *              pIrq;
    BOOL                        result;

    refIntr.AsAny = NULL;
    stat = KernToken_Translate(aTokIntr, &refIntr);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    result = FALSE;
    
    do {
        if (refIntr.AsAny->mObjType != KernObj_Interrupt)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
            break;
        }

        result = TRUE;

        if (refIntr.AsInterrupt->mVoteForEnabled == aSetEnable)
        {
            break;
        }

        //
        // changing interrupt enable state. check again inside lock
        //

        disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);

        if (refIntr.AsInterrupt->mVoteForEnabled != aSetEnable)
        {
            refIntr.AsInterrupt->mVoteForEnabled = aSetEnable;

            pIrq = refIntr.AsInterrupt->mpParentIrq;

            if (!aSetEnable)
            {
                KernArch_SetDevIrqMask(pIrq, TRUE);
            }
            else
            {
                IntrLocked_IntrEnabled_CheckIrqEnable(pIrq);
            }
        }

        K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);

    } while (0);

    KernObj_ReleaseRef(&refIntr);

    return result;
}

BOOL
K2OSKERN_IntrDone(
    K2OS_INTERRUPT_TOKEN    aTokIntr
)
{
    K2OSKERN_OBJREF refIntr;
    K2STAT          stat;
    BOOL            disp;
    BOOL            result;

    refIntr.AsAny = NULL;
    stat = KernToken_Translate(aTokIntr, &refIntr);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    result = FALSE;

    do {
        if (refIntr.AsAny->mObjType != KernObj_Interrupt)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
            break;
        }

        disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);

        if (!refIntr.AsInterrupt->mInService)
        {
            K2OSKERN_Debug("*** Attempt to call Done on interrupt not in service!\n");
        }
        else
        {
            refIntr.AsInterrupt->mInService = FALSE;

            result = TRUE;

            if (!refIntr.AsInterrupt->mVoteForEnabled)
            {
                refIntr.AsInterrupt->mVoteForEnabled = TRUE;

                IntrLocked_IntrEnabled_CheckIrqEnable(refIntr.AsInterrupt->mpParentIrq);
            }
        }

        K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);

    } while (0);

    KernObj_ReleaseRef(&refIntr);

    return result;
}

