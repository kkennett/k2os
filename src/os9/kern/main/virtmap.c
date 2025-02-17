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
KernVirtMap_Purge(
    K2OSKERN_OBJ_VIRTMAP *apMap
)
{
    if (NULL != apMap->ProcRef.AsAny)
    {
        KernObj_ReleaseRef(&apMap->ProcRef);
    }

    KernObj_ReleaseRef(&apMap->PageArrayRef);

    KernObj_Free(&apMap->Hdr);
}

void
KernVirtMap_Cleanup_VirtNotLocked_FreeVirt(
    K2OSKERN_OBJ_VIRTMAP *  apMap
)
{
    if (NULL != apMap->ProcRef.AsAny)
    {
        KernProc_UserVirtHeapFree(apMap->ProcRef.AsProc, K2HEAP_NodeAddr(&apMap->User.mpProcHeapNode->HeapNode));
        apMap->User.mpProcHeapNode = NULL;
    }
    else
    {
        K2_ASSERT(0 != apMap->OwnerMapTreeNode.mUserVal);
        KernVirt_Release(apMap->OwnerMapTreeNode.mUserVal);
    }
}

BOOL
KernVirtMap_Cleanup_VirtLocked_Done(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_VIRTMAP *      apMap
)
{
    K2OSKERN_PROCHEAP_NODE *pProcHeapNode;

    K2_ASSERT(NULL != apMap->ProcRef.AsAny);

    //
    // return TRUE if virtual memory should be freed
    //
    pProcHeapNode = apMap->User.mpProcHeapNode;

    K2_ASSERT(0 != pProcHeapNode->mMapCount);

    --pProcHeapNode->mMapCount;

    if ((0 == pProcHeapNode->mMapCount) &&
        (0 == pProcHeapNode->mUserOwned))
    {
        //
        // non-user-controlled allocation hit zero mappings so
        // we are going to free it
        //
        pProcHeapNode->mMapCount = 0xFFFF;
        return TRUE;
    }

    return FALSE;
}

void
KernVirtMap_Cleanup_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_VIRTMAP *  pMap;
    K2OSKERN_OBJ_PROCESS *  pProc;
    BOOL                    disp;
    BOOL                    doFree;

    pMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 1, KTRACE_MAP_CLEAN_CHECK_DPC);

    if (0 == pMap->TlbShoot.mCoresRemaining)
    {
        pProc = pMap->ProcRef.AsProc;
        if (NULL != pProc)
        {
            disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);
            doFree = KernVirtMap_Cleanup_VirtLocked_Done(apThisCore, pMap);
            K2OSKERN_SeqUnlock(&pProc->Virt.SeqLock, disp);
        }
        else
        {
            doFree = TRUE;
        }
        if (doFree)
        {
            KernVirtMap_Cleanup_VirtNotLocked_FreeVirt(pMap);
        }
        KernVirtMap_Purge(pMap);
        KTRACE(apThisCore, 1, KTRACE_MAP_CLEAN_DONE);
    }
    else
    {
        pMap->Hdr.ObjDpc.Func = KernVirtMap_Cleanup_CheckComplete;
        KernCpu_QueueDpc(&pMap->Hdr.ObjDpc.Dpc, &pMap->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
    }
}

void
KernVirtMap_Cleanup_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_VIRTMAP *  pMap;
    UINT32                  sentMask;

    pMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pMap->mIciSendMask);
    
    KTRACE(apThisCore, 1, KTRACE_MAP_CLEAN_SENDICI_DPC);

    sentMask = KernArch_SendIci(
        apThisCore,
        pMap->mIciSendMask,
        KernIci_TlbInv,
        &pMap->TlbShoot
    );

    pMap->mIciSendMask &= ~sentMask;

    if (0 == pMap->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pMap->Hdr.ObjDpc.Func = KernVirtMap_Cleanup_CheckComplete;
    }
    else
    {
        pMap->Hdr.ObjDpc.Func = KernVirtMap_Cleanup_SendIci;
    }
    KernCpu_QueueDpc(&pMap->Hdr.ObjDpc.Dpc, &pMap->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
}

BOOL
KernVirtMap_Cleanup_VirtLocked_StartShootDown(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_VIRTMAP *      apMap
)
{
    UINT32 ix;
    UINT32 workAddr;

    if (gData.mCpuCoreCount > 1)
    {
        apMap->TlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        apMap->TlbShoot.mpProc = apMap->ProcRef.AsProc;
        apMap->TlbShoot.mVirtBase = apMap->OwnerMapTreeNode.mUserVal;
        apMap->TlbShoot.mPageCount = apMap->mPageCount;
        apMap->mIciSendMask = apMap->TlbShoot.mCoresRemaining;
        KernVirtMap_Cleanup_SendIci(apThisCore, &apMap->Hdr.ObjDpc.Func);
    }

    if ((NULL == apMap->ProcRef.AsAny) ||
        (apThisCore->MappedProcRef.AsProc == apMap->ProcRef.AsProc))
    {
        workAddr = apMap->OwnerMapTreeNode.mUserVal;
        for (ix = 0; ix < apMap->mPageCount; ix++)
        {
            KernArch_InvalidateTlbPageOnCurrentCore(workAddr);
            workAddr += K2_VA_MEMPAGE_BYTES;
        }
    }

    if (gData.mCpuCoreCount == 1)
    {
        if (NULL != apMap->ProcRef.AsAny)
        {
            return KernVirtMap_Cleanup_VirtLocked_Done(apThisCore, apMap);
        }
    }

    return FALSE;
}

void    
KernVirtMap_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_VIRTMAP *      apMap
)
{
    K2OSKERN_OBJ_PROCESS *      pProc;
    K2OSKERN_OBJ_PAGEARRAY *    pPageArray;
    BOOL                        disp;
    BOOL                        procStopped;
    UINT32                      ixPage;
    UINT32                      pagePhys;
    UINT32                      virtAddr;
    BOOL                        doFree;
    UINT32                      pagesLeft;
    UINT32 *                    pPTE;

    K2_ASSERT(0 != (apMap->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    pPageArray = apMap->PageArrayRef.AsPageArray;
 
    pProc = apMap->ProcRef.AsProc;
    if (NULL != pProc)
    {
        procStopped = (pProc->mState == KernProcState_Stopped) ? TRUE : FALSE;

        disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);
        if (!procStopped)
        {
            //
            // remove the pte mappings from the process pagetables
            //
            virtAddr = apMap->OwnerMapTreeNode.mUserVal;
            for (ixPage = 0; ixPage < apMap->mPageCount; ixPage++)
            {
                pagePhys = KernPte_BreakPageMap(pProc, virtAddr, 0);
                K2_ASSERT(pagePhys == KernPageArray_PagePhys(pPageArray, apMap->mPageArrayStartPageIx + ixPage));
                virtAddr += K2_VA_MEMPAGE_BYTES;
            }

            //
            // begin tlb shootdown dpc chain
            //
            doFree = KernVirtMap_Cleanup_VirtLocked_StartShootDown(apThisCore, apMap);
        }
        else
        {
            doFree = FALSE;
        }

        //
        // always remove from the mapping tree
        //
        K2TREE_Remove(&pProc->Virt.Locked.MapTree, &apMap->OwnerMapTreeNode);
        K2OSKERN_SeqUnlock(&pProc->Virt.SeqLock, disp);

        if (procStopped)
        {
            //
            // process exited case.  no tlbshootdown or virtual free done
            // but we still purge
            //
            KernVirtMap_Purge(apMap);
            return;
        }
    }
    else
    {
        //        K2OSKERN_Debug("Kern   : Virtmap %08X(%08X) cleanup\n", apMap->OwnerMapTreeNode.mUserVal, apMap->mPageCount * K2_VA_MEMPAGE_BYTES);
        disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);

        virtAddr = apMap->OwnerMapTreeNode.mUserVal;
        pagesLeft = apMap->mPageCount;

        if (apMap->Kern.mSegType == KernSeg_Thread_Stack)
        {
            // clear the guard mappings
            pPTE = apMap->mpPte;
            K2_ASSERT(K2OSKERN_PTE_STACK_GUARD == *pPTE);
            *pPTE = 0;
            pagesLeft--;
            pPTE += pagesLeft;
            K2_ASSERT(K2OSKERN_PTE_STACK_GUARD == *pPTE);
            *pPTE = 0;
            pagesLeft--;

            virtAddr += K2_VA_MEMPAGE_BYTES;
        }
        for (ixPage = 0; ixPage < pagesLeft; ixPage++)
        {
            pagePhys = KernPte_BreakPageMap(pProc, virtAddr, 0);
            K2_ASSERT(pagePhys == KernPageArray_PagePhys(pPageArray, apMap->mPageArrayStartPageIx + ixPage));
            virtAddr += K2_VA_MEMPAGE_BYTES;
        }

        //
        // begin tlb shootdown dpc chain
        //
        doFree = KernVirtMap_Cleanup_VirtLocked_StartShootDown(apThisCore, apMap);

        //
        // always remove from the mapping tree
        //
        K2TREE_Remove(&gData.VirtMap.Tree, &apMap->OwnerMapTreeNode);
        K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);
    }

    if (doFree)
    {
        KernVirtMap_Cleanup_VirtNotLocked_FreeVirt(apMap);
    }

    if (gData.mCpuCoreCount == 1)
    {
        KernVirtMap_Purge(apMap);
    }
    //
    // else DPC queued (for sending icis or for checking if they are all complete)
    //
}

