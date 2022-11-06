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

#define DUMP_NODES 1

void
KernPlat_MapOne(
    K2OS_PLATINIT_MAP * apInit
)
{
    UINT32  physAddr;
    UINT32  virtAddr;
    UINT32  pageCount;
    UINT32  mapAttr;
   
    apInit->mVirtAddr = virtAddr = KernVirt_AllocPages(apInit->mPageCount);
    K2_ASSERT(0 != virtAddr);

    physAddr = apInit->mPhysAddr & K2_VA_PAGEFRAME_MASK;
    pageCount = apInit->mPageCount;
    mapAttr = apInit->mMapAttr &
        (K2OS_MEMPAGE_ATTR_READWRITE |
        K2OS_MEMPAGE_ATTR_UNCACHED |
        K2OS_MEMPAGE_ATTR_DEVICEIO |
        K2OS_MEMPAGE_ATTR_WRITE_THRU);

    do {
        KernPte_MakePageMap(NULL, virtAddr, physAddr, mapAttr);
        virtAddr += K2_VA_MEMPAGE_BYTES;
        physAddr += K2_VA_MEMPAGE_BYTES;
    } while (--pageCount);
}

void 
KernPlat_Init(
    void
)
{
    XDL_pf_ENTRYPOINT       platEntryPoint;
    K2STAT                  stat;
    K2OSPLAT_pf_Init        fInit;
    K2OSPLAT_pf_DebugOut    fDebugOut;
    K2OS_PLATINIT_MAP *     pMaps;
    K2OS_PLATINIT_MAP *     pIter;
    XDL_FILE_HEADER const * pXdlHeader;

    K2OSKERN_SeqInit(&gData.Plat.SeqLock);
    K2TREE_Init(&gData.Plat.Locked.Tree, NULL);

    //
    // loader should have checked all this in UEFI when PLAT XDL was loaded
    // but we check stuff again here
    //
    stat = XDL_FindExport(
        gData.Xdl.mpPlat,
        XDLProgData_Text,
        "K2OSPLAT_Init",
        (UINT32 *)&fInit);
    while (K2STAT_IS_ERROR(stat));

    stat = XDL_FindExport(
        gData.Xdl.mpPlat,
        XDLProgData_Text,
        "K2OSPLAT_DebugOut",
        (UINT32 *)&fDebugOut);
    while (K2STAT_IS_ERROR(stat));

    stat = XDL_FindExport(
        gData.Xdl.mpPlat,
        XDLProgData_Text,
        "K2OSPLAT_DebugIn",
        (UINT32 *)&gData.mpShared->FuncTab.DebugIn);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = XDL_FindExport(
        gData.Xdl.mpPlat,
        XDLProgData_Text,
        "K2OSPLAT_ForcedDriverQuery",
        (UINT32 *)&gData.Plat.mfForcedDriverQuery);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = XDL_FindExport(
        gData.Xdl.mpPlat,
        XDLProgData_Text,
        "K2OSPLAT_OnIrq",
        (UINT32 *)&gData.Intr.mfPlatOnIrq);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = XDL_FindExport(
        gData.Xdl.mpPlat,
        XDLProgData_Text,
        "K2OSPLAT_GetResTable",
        (UINT32 *)&gData.Plat.mfGetResTable);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = XDL_GetHeaderPtr(gData.Xdl.mpPlat, &pXdlHeader);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    platEntryPoint = (XDL_pf_ENTRYPOINT)(UINT_PTR)pXdlHeader->mEntryPoint;

    stat = platEntryPoint(gData.Xdl.mpPlat, XDL_ENTRY_REASON_LOAD);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    pMaps = fInit(NULL);
    if (NULL != pMaps)
    {
        //
        // map the stuff the plat wants
        //
        pIter = pMaps;
        while (pIter->mPageCount)
        {
            KernPlat_MapOne(pIter);
            pIter++;
        }

        //
        // now tell it the maps are filled in
        //
        fInit(pMaps);
    }

    //
    // only set this after init is done
    //
    gData.mpShared->FuncTab.DebugOut = fDebugOut;
    K2_CpuWriteBarrier();
}

UINT_PTR
KernPlat_Locked_ConfigureNode(
    K2OSKERN_PLATNODE * apNode,
    UINT8 *             apIoBuf,
    UINT_PTR            aInBytes
)
{
    UINT_PTR result;

    result = gData.Plat.mfForcedDriverQuery(apNode, (char *)apIoBuf, K2OS_THREAD_PAGE_BUFFER_BYTES - 1, aInBytes);
    K2_ASSERT(result < K2OS_THREAD_PAGE_BUFFER_BYTES);
    apIoBuf[result] = 0;

    return result;
}

#if DUMP_NODES

void
KernPlat_DumpName(
    K2OSKERN_PLATNODE * apNode
)
{
    if (NULL != apNode->mpParent)
    {
        KernPlat_DumpName(apNode->mpParent);
    }
    K2OSKERN_Debug("\\%s", apNode->mName);
}

void
KernPlat_DumpNode(
    K2OSKERN_PLATNODE * apNode,
    UINT8 const *       apExtra
)
{
    K2OSKERN_PLATIO *   pIo;
    K2OSKERN_PLATPHYS * pPhys;
    K2OSKERN_PLATINTR * pIntr;

    K2OSKERN_Debug("PLATNODE-%08X------------\n", apNode);
    KernPlat_DumpName(apNode);
    K2OSKERN_Debug(" @ROOT_%08X\n", apNode->mRootContext);
    if (NULL != apExtra)
    {
        K2OSKERN_Debug("  XTRA \"%s\"\n", (char const *)apExtra);
    }
    pIo = apNode->mpIoList;
    if (NULL != pIo)
    {
        do {
            K2OSKERN_Debug("  IO %04x %04X\n", pIo->Range.mBasePort, pIo->Range.mSizeBytes);
            pIo = pIo->mpNext;
        } while (NULL != pIo);
    }
    pPhys = apNode->mpPhysList;
    if (NULL != pPhys)
    {
        do {
            K2OSKERN_Debug("  PHYS %08X %08X v%08X\n", pPhys->Range.mBaseAddr, pPhys->Range.mSizeBytes, pPhys->mKernVirtAddr);
            pPhys = pPhys->mpNext;
        } while (NULL != pPhys);
    }
    pIntr = apNode->mpIntrList;
    if (NULL != pIntr)
    {
        do {
            K2OSKERN_Debug("  INTR IRQ %d\n", pIntr->mpIrq->Config.mSourceIrq);
            pIntr = pIntr->mpNext;
        } while (NULL != pIntr);
    }
    K2OSKERN_Debug("-----------------------------\n");
}

#endif

void    
KernPlat_SysCall_Root_CreateNode(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                      stat;
    K2OSKERN_PLATNODE *         pNode;
    K2OSKERN_PLATNODE *         pParent;
    K2TREE_NODE *               pTreeNode;
    K2OS_USER_THREAD_PAGE *     pThreadPage;
    BOOL                        disp;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    if (apCurThread->ProcRef.Ptr.AsProc->mId != 1)
    {
        stat = K2STAT_ERROR_NOT_ALLOWED;
    }
    else
    {
        pNode = (K2OSKERN_PLATNODE *)KernHeap_Alloc(sizeof(K2OSKERN_PLATNODE));
        if (NULL == pNode)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            K2MEM_Zero(pNode, sizeof(K2OSKERN_PLATNODE));
            K2LIST_Init(&pNode->ChildList);
            K2MEM_Copy(&pNode->mName, &pThreadPage->mSysCall_Arg1, sizeof(UINT32));
            pNode->mName[4] = 0;
            pNode->mRefs = 1;
            pNode->mRootContext = pThreadPage->mSysCall_Arg2;

            disp = K2OSKERN_SeqLock(&gData.Plat.SeqLock);

            if (apCurThread->mSysCall_Arg0 == 0)
            {
                if (gData.Plat.Locked.Tree.mNodeCount > 0)
                {
                    stat = K2STAT_ERROR_ALREADY_EXISTS;
                }
                else
                {
                    stat = K2STAT_NO_ERROR;

                    pNode->TreeNode.mUserVal = (UINT32)pNode;
                    K2TREE_Insert(&gData.Plat.Locked.Tree, pNode->TreeNode.mUserVal, &pNode->TreeNode);
                    apCurThread->mSysCall_Result = (UINT_PTR)pNode;
                    pNode = NULL;
                }
            }
            else
            {
                pTreeNode = K2TREE_Find(&gData.Plat.Locked.Tree, (UINT32)apCurThread->mSysCall_Arg0);
                if (NULL == pTreeNode)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                }
                else
                {
                    stat = K2STAT_NO_ERROR;

                    pParent = K2_GET_CONTAINER(K2OSKERN_PLATNODE, pTreeNode, TreeNode);

                    K2LIST_AddAtTail(&pParent->ChildList, &pNode->ParentChildListLink);
                    pNode->mpParent = pParent;
                    pNode->TreeNode.mUserVal = (UINT32)pNode;
                    K2TREE_Insert(&gData.Plat.Locked.Tree, pNode->TreeNode.mUserVal, &pNode->TreeNode);
                    apCurThread->mSysCall_Result = (UINT_PTR)pNode;
#if DUMP_NODES
                    KernPlat_DumpNode(pNode, pThreadPage->mMiscBuffer);
#endif
                    pThreadPage->mSysCall_Arg7_Result0 = KernPlat_Locked_ConfigureNode(pNode, pThreadPage->mMiscBuffer, pThreadPage->mSysCall_Arg3);
                    pNode = NULL;
                    pParent->mRefs++;
                }
            }

            K2OSKERN_SeqUnlock(&gData.Plat.SeqLock, disp);

            if (NULL != pNode)
            {
                KernHeap_Free(pNode);
            }
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

K2OSKERN_PLATNODE *
KernPlat_FindAddRefNode(
    UINT_PTR    aNodeKey
)
{
    BOOL                disp;
    K2TREE_NODE *       pTreeNode;
    K2OSKERN_PLATNODE * pPlatNode;

    disp = K2OSKERN_SeqLock(&gData.Plat.SeqLock);

    pTreeNode = K2TREE_Find(&gData.Plat.Locked.Tree, aNodeKey);
    if (NULL != pTreeNode)
    {
        pPlatNode = K2_GET_CONTAINER(K2OSKERN_PLATNODE, pTreeNode, TreeNode);
        pPlatNode->mRefs++;
    }
    else
        pPlatNode = NULL;

    K2OSKERN_SeqUnlock(&gData.Plat.SeqLock, disp);

    return pPlatNode;
}

void
KernPlat_ReleaseNode(
    K2OSKERN_PLATNODE * apNode
)
{
    K2LIST_ANCHOR       relList;
    BOOL                disp;
    K2OSKERN_PLATNODE * pParent;
    K2LIST_LINK *       pListLink;

    K2LIST_Init(&relList);

    disp = K2OSKERN_SeqLock(&gData.Plat.SeqLock);

    do {
        pParent = apNode->mpParent;

        K2_ASSERT(0 < apNode->mRefs);

        if (0 != --apNode->mRefs)
            break;

        if (pParent != NULL)
        {
            K2LIST_Remove(&pParent->ChildList, &apNode->ParentChildListLink);
        }

        K2TREE_Remove(&gData.Plat.Locked.Tree, &apNode->TreeNode);

        K2LIST_AddAtTail(&relList, &apNode->ParentChildListLink);

        apNode = pParent;

    } while (NULL != apNode);

    K2OSKERN_SeqUnlock(&gData.Plat.SeqLock, disp);

    if (relList.mNodeCount > 0)
    {
        pListLink = relList.mpHead;
        do {
            apNode = K2_GET_CONTAINER(K2OSKERN_PLATNODE, pListLink, ParentChildListLink);
            pListLink = pListLink->mpNext;

            //
            // purge resources from apNode then purge the node
            //
            K2_ASSERT(0);

            KernHeap_Free(apNode);

        } while (NULL != pListLink);
    }
}

void    
KernPlat_SysCall_Root_AddRes(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                      stat;
    K2OSKERN_IRQ *              pNewIrq;
    K2OSKERN_IRQ *              pExistingIrq;
    K2OSKERN_OBJ_INTERRUPT *    pIntr;
    K2OSKERN_PLATMAP *          pPlatMap;
    K2OSKERN_PLATIO *           pPlatIo;
    K2OSKERN_PLATNODE *         pPlatNode;
    K2LIST_LINK *               pListLink;
    K2OSKERN_OBJ_PROCESS *      pProc;
    K2OS_USER_THREAD_PAGE *     pThreadPage;
    UINT_PTR                    resType;
    UINT_PTR                    endAddr;
    BOOL                        disp;

    pProc = apCurThread->ProcRef.Ptr.AsProc;
    if (pProc->mId != 1)
    {
        stat = K2STAT_ERROR_NOT_ALLOWED;
    }
    else
    {
        pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

        resType = pThreadPage->mSysCall_Arg1;
        if ((resType < K2OS_PLATRES_MIN) ||
            (resType > K2OS_PLATRES_MAX))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = K2STAT_NO_ERROR;
            switch (resType)
            {
            case K2OS_PLATRES_IRQ:
                if (((K2OS_IRQ_CONFIG *)&pThreadPage->mSysCall_Arg2)->mSourceIrq > 255)
                {
                    stat = K2STAT_ERROR_BAD_ARGUMENT;
                }
                break;
            case K2OS_PLATRES_IO:
#if K2_TARGET_ARCH_IS_INTEL
                if ((pThreadPage->mSysCall_Arg3 == 0) ||
                    (pThreadPage->mSysCall_Arg2 > 0xFFFF) ||
                    (pThreadPage->mSysCall_Arg3 > 0xFFFF) ||
                    (0x10000 - pThreadPage->mSysCall_Arg2) < pThreadPage->mSysCall_Arg3)
                {
                    K2OSKERN_Debug("PlatresIo - %08X,%08X fail\n", pThreadPage->mSysCall_Arg2, pThreadPage->mSysCall_Arg3);
                    stat = K2STAT_ERROR_BAD_ARGUMENT;
                }
#else 
                stat = K2STAT_ERROR_NOT_SUPPORTED;
#endif 
                break;
            case K2OS_PLATRES_PHYS:
                if ((pThreadPage->mSysCall_Arg3 == 0) ||
                    (pThreadPage->mSysCall_Arg2 > 0xFFFFFFF0) ||
                    (pThreadPage->mSysCall_Arg3 > 0xFFFFFFF0) ||
                    (((0xFFFFFFFF - pThreadPage->mSysCall_Arg2) + 1) < pThreadPage->mSysCall_Arg3))
                {
                    stat = K2STAT_ERROR_BAD_ARGUMENT;
                }
                break;
            default:
                stat = K2STAT_ERROR_UNKNOWN;
                break;
            }
        }

        if (!K2STAT_IS_ERROR(stat))
        {
            pPlatNode = KernPlat_FindAddRefNode(apCurThread->mSysCall_Arg0);
            if (NULL == pPlatNode)
            {
                stat = K2STAT_ERROR_NOT_FOUND;
            }
            else
            {
                switch (resType)
                {
                case K2OS_PLATRES_IRQ:
                    pNewIrq = (K2OSKERN_IRQ *)KernHeap_Alloc(sizeof(K2OSKERN_IRQ));
                    if (NULL == pNewIrq)
                    {
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }
                    K2MEM_Zero(pNewIrq, sizeof(K2OSKERN_IRQ));
                    K2MEM_Copy(&pNewIrq->PlatIrq.Config, &pThreadPage->mSysCall_Arg2, sizeof(K2OS_IRQ_CONFIG));
                    K2LIST_Init(&pNewIrq->PlatIrq.IntrList);

                    pIntr = (K2OSKERN_OBJ_INTERRUPT *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_INTERRUPT));
                    if (NULL == pIntr)
                    {
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        K2MEM_Zero(pIntr, sizeof(K2OSKERN_OBJ_INTERRUPT));
                        pIntr->Hdr.mObjType = KernObj_Interrupt;
                        K2LIST_Init(&pIntr->Hdr.RefObjList);
                        stat = KernGate_Create(FALSE, &pIntr->GateRef);
                        if (K2STAT_IS_ERROR(stat))
                        {
                            KernHeap_Free(pIntr);
                        }
                        else
                        {
                            disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);
                            pListLink = gData.Intr.Locked.List.mpHead;
                            if (NULL != pListLink)
                            {
                                do {
                                    pExistingIrq = K2_GET_CONTAINER(K2OSKERN_IRQ, pListLink, GlobalIrqListLink);
                                    if (pExistingIrq->PlatIrq.Config.mSourceIrq == pNewIrq->PlatIrq.Config.mSourceIrq)
                                        break;
                                    pListLink = pListLink->mpNext;
                                } while (NULL != pListLink);
                            }
                            if (NULL == pListLink)
                            {
                                K2LIST_AddAtTail(&gData.Intr.Locked.List, &pNewIrq->GlobalIrqListLink);
                                pExistingIrq = pNewIrq;
                                KernArch_InstallDevIrqHandler(pNewIrq);
                                pNewIrq = NULL;
                            }

                            pIntr->PlatIntr.mpIrq = &pExistingIrq->PlatIrq;
                            pIntr->PlatIntr.mpNode = pPlatNode;
                            K2LIST_AddAtTail(&pExistingIrq->PlatIrq.IntrList, &pIntr->PlatIntr.IrqIntrListLink);

                            K2OSKERN_SeqLock(&gData.Plat.SeqLock);
                            pIntr->PlatIntr.mpNext = pPlatNode->mpIntrList;
                            pPlatNode->mpIntrList = &pIntr->PlatIntr;
                            pPlatNode->mRefs++;  // interrupt object holds reference to node it points to

                            K2OSKERN_SeqUnlock(&gData.Plat.SeqLock, FALSE);

                            apCurThread->mSysCall_Result = (UINT_PTR)&pIntr->PlatIntr;

                            K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);
                        }
                    }
                    if (NULL != pNewIrq)
                    {
                        KernHeap_Free(pNewIrq);
                    }
                    break;

                case K2OS_PLATRES_PHYS:
                    pPlatMap = (K2OSKERN_PLATMAP *)KernHeap_Alloc(sizeof(K2OSKERN_PLATMAP));
                    if (NULL == pPlatMap)
                    {
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        K2MEM_Zero(pPlatMap, sizeof(K2OSKERN_PLATMAP));

                        // get end byte of range and round that up to the next page boundary
                        endAddr = pThreadPage->mSysCall_Arg2 + pThreadPage->mSysCall_Arg3 - 1;
                        endAddr = ((endAddr + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;

                        stat = KernPageArray_CreateSpec(
                            pThreadPage->mSysCall_Arg2 & K2_VA_PAGEFRAME_MASK,
                            (endAddr - (pThreadPage->mSysCall_Arg2 & K2_VA_PAGEFRAME_MASK)) / K2_VA_MEMPAGE_BYTES,
                            K2OS_MEMPAGE_ATTR_READWRITE | K2OS_MEMPAGE_ATTR_DEVICEIO | K2OS_MEMPAGE_ATTR_WRITE_THRU | K2OS_MEMPAGE_ATTR_UNCACHED,
                            &pPlatMap->PageArrayRef);
                        if (K2STAT_IS_ERROR(stat))
                        {
                            KernHeap_Free(pPlatMap);
                        }
                        else
                        {
                            pPlatMap->PlatPhys.Range.mBaseAddr = pThreadPage->mSysCall_Arg2;
                            pPlatMap->PlatPhys.Range.mSizeBytes = pThreadPage->mSysCall_Arg3;
                            //
                            // map anything smaller than a meg into the kernel
                            //
                            if (pPlatMap->PageArrayRef.Ptr.AsPageArray->mPageCount < 256)
                            {
                                pPlatMap->PlatPhys.mKernVirtAddr = KernVirt_AllocPages(pPlatMap->PageArrayRef.Ptr.AsPageArray->mPageCount);
                                if (0 == pPlatMap->PlatPhys.mKernVirtAddr)
                                {
                                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                                    KernObj_ReleaseRef(&pPlatMap->PageArrayRef);
                                    KernHeap_Free(pPlatMap);
                                }
                                else
                                {
                                    pPlatMap->PlatPhys.mKernVirtAddr =
                                        pThreadPage->mSysCall_Arg2
                                        - pPlatMap->PageArrayRef.Ptr.AsPageArray->Data.Spec.mBasePhys
                                        + pPlatMap->PlatPhys.mKernVirtAddr;
                                }
                            }
                            if (!K2STAT_IS_ERROR(stat))
                            {
                                disp = K2OSKERN_SeqLock(&gData.Plat.SeqLock);
                                pPlatMap->PlatPhys.mpNext = pPlatNode->mpPhysList;
                                pPlatNode->mpPhysList = &pPlatMap->PlatPhys;
                                K2OSKERN_SeqUnlock(&gData.Plat.SeqLock, disp);

                                apCurThread->mSysCall_Result = (UINT_PTR)&pPlatMap->PlatPhys;
                            }
                        }
                    }
                    break;

                case K2OS_PLATRES_IO:
                    pPlatIo = (K2OSKERN_PLATIO *)KernHeap_Alloc(sizeof(K2OSKERN_PLATIO));
                    if (NULL == pPlatIo)
                    {
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        K2MEM_Zero(pPlatIo, sizeof(K2OSKERN_PLATIO));
                        pPlatIo->Range.mBasePort = (UINT16)pThreadPage->mSysCall_Arg2;
                        pPlatIo->Range.mSizeBytes = (UINT16)pThreadPage->mSysCall_Arg3;

                        disp = K2OSKERN_SeqLock(&gData.Plat.SeqLock);
                        pPlatIo->mpNext = pPlatNode->mpIoList;
                        pPlatNode->mpIoList = pPlatIo;
                        K2OSKERN_SeqUnlock(&gData.Plat.SeqLock, disp);

                        apCurThread->mSysCall_Result = (UINT_PTR)pPlatIo;
                    }
                    break;

                default:
                    K2_ASSERT(0);
                }
                KernPlat_ReleaseNode(pPlatNode);
            }
        }
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
    else
    {
#if DUMP_NODES
        KernPlat_DumpNode(pPlatNode, NULL);
#endif
    }
}

void
KernPlat_SysCall_Root_HookIntr(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                      stat;
    K2OSKERN_PLATNODE *         pPlatNode;
    K2OSKERN_OBJ_INTERRUPT *    pInterrupt;
    K2OSKERN_PLATINTR *         pPlatIntr;
    K2OS_USER_THREAD_PAGE *     pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    pPlatNode = KernPlat_FindAddRefNode(apCurThread->mSysCall_Arg0);
    if (NULL == pPlatNode)
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        pPlatIntr = pPlatNode->mpIntrList;
        if (NULL != pPlatIntr)
        {
            do {
                if (pPlatIntr->mpIrq->Config.mSourceIrq == pThreadPage->mSysCall_Arg1)
                    break;
                pPlatIntr = pPlatIntr->mpNext;
            } while (NULL != pPlatIntr);
        }
        if (NULL == pPlatIntr)
        {
            stat = K2STAT_ERROR_NOT_FOUND;
        }
        else
        {
            pInterrupt = K2_GET_CONTAINER(K2OSKERN_OBJ_INTERRUPT, pPlatIntr, PlatIntr);

            stat = KernProc_TokenCreate(apCurThread->ProcRef.Ptr.AsProc, &pInterrupt->Hdr, (K2OS_TOKEN *)&apCurThread->mSysCall_Result);
        }
        KernPlat_ReleaseNode(pPlatNode);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void
KernPlat_IntrLocked_IrqEnableIntr(
    K2OSKERN_IRQ *              apIrq,
    K2OSKERN_OBJ_INTERRUPT *    apIntr,
    BOOL                        aSetEnable,
    BOOL                        aIsIntrEnd
)
{
    BOOL changeIrq;
    BOOL setIrqEnable;

    changeIrq = FALSE;

    if (!aSetEnable)
    {
        //
        // interrupt is being manually disabled by the user
        //
        K2_ASSERT(!aIsIntrEnd);

        if (apIrq->PlatIrq.IntrList.mNodeCount == 1)
        {
            K2_ASSERT(apIrq->PlatIrq.IntrList.mpHead == &apIntr->PlatIntr.IrqIntrListLink);
            apIntr->mEnabled = FALSE;
            changeIrq = TRUE;
            setIrqEnable = FALSE;
        }
        else
        {
            K2_ASSERT(0);
        }
    }
    else
    {
        if (aIsIntrEnd)
        {
            //
            // intrend system call
            // 
            // only place in the system an interrupt can go out of service
            //
            K2_ASSERT(apIntr->mInService);
            apIntr->mInService = FALSE;

            if (apIrq->PlatIrq.IntrList.mNodeCount == 1)
            {
                K2_ASSERT(apIrq->PlatIrq.IntrList.mpHead == &apIntr->PlatIntr.IrqIntrListLink);
                apIntr->mEnabled = TRUE;
                apIrq->mpOneMask_Holder = NULL;
                apIntr->mOneMaskingIrq = FALSE;
                changeIrq = TRUE;
                setIrqEnable = TRUE;
            }
            else
            {
                K2_ASSERT(0);
            }
        }
        else
        {
            //
            // interrupt is being manually enabled by the user
            //
            K2_ASSERT(!apIntr->mInService);

            if (apIrq->PlatIrq.IntrList.mNodeCount == 1)
            {
                K2_ASSERT(apIrq->PlatIrq.IntrList.mpHead == &apIntr->PlatIntr.IrqIntrListLink);
                apIntr->mEnabled = TRUE;
                changeIrq = TRUE;
                setIrqEnable = TRUE;
            }
            else
            {
                K2_ASSERT(0);
            }
        }
    }

    if (changeIrq)
    {
        KernArch_SetDevIrqMask(apIrq, !setIrqEnable);
    }
}

BOOL 
KernPlat_IntrLocked_SetEnable(
    K2OSKERN_OBJ_INTERRUPT *apIntr,
    BOOL                    aSetEnable
)
{
    K2OSKERN_IRQ *  pIrq;

    if (apIntr->mEnabled == aSetEnable)
        return TRUE;

    //
    // if interrupt is in service then attempting to enable it is an error
    // use interrupt end to enable it
    //
    if (apIntr->mInService)
    {
        K2_ASSERT(!apIntr->mEnabled);
        if (aSetEnable)
            return FALSE;
        return TRUE;
    }

    //
    // interrupt not in service
    //
    pIrq = K2_GET_CONTAINER(K2OSKERN_IRQ, apIntr->PlatIntr.mpIrq, PlatIrq);
    
    KernPlat_IntrLocked_IrqEnableIntr(pIrq, apIntr, aSetEnable, FALSE);

    return TRUE;
}

void    
KernPlat_IntrLocked_End(
    K2OSKERN_OBJ_INTERRUPT *apIntr
)
{
    K2OSKERN_IRQ *  pIrq;

    pIrq = K2_GET_CONTAINER(K2OSKERN_IRQ, apIntr->PlatIntr.mpIrq, PlatIrq);

    KernPlat_IntrLocked_IrqEnableIntr(pIrq, apIntr, TRUE, TRUE);
}

void
KernPlat_IntrLocked_Queue(
    UINT_PTR                aCoreIx,
    UINT64 const *          apHfTick,
    K2OSKERN_OBJ_INTERRUPT *apIntr
)
{
    K2OSKERN_CPUCORE_EVENT *pEvent;

    //
    // self-reference is held to the interrupt until the it is handled
    //
    K2_ASSERT(apIntr->SchedItem.ObjRef.Ptr.AsHdr == NULL);
    K2_ASSERT(!apIntr->mInService);
    K2_ASSERT(!apIntr->mEnabled);
    KernObj_CreateRef(&apIntr->SchedItem.ObjRef, &apIntr->Hdr);

    pEvent = &apIntr->Event;
    pEvent->mEventType = KernCpuCoreEvent_Interrupt;
    pEvent->mSrcCoreIx = aCoreIx;
    pEvent->mEventHfTick = *apHfTick;

    KernCpu_QueueEvent(pEvent);
}

void
KernPlat_IntrLocked_OnIrq(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT64 const *              apTick,
    K2OSKERN_IRQ *              apIrq
)
{
    UINT_PTR                    nodeCount;
    K2OSKERN_OBJ_INTERRUPT *    pIntr;

    nodeCount = apIrq->PlatIrq.IntrList.mNodeCount;

    if (nodeCount == 0)
    {
        KernArch_SetDevIrqMask(apIrq, TRUE);
        return;
    }

    if (nodeCount == 1)
    {
        K2_ASSERT(apIrq->PlatIrq.IntrList.mpHead != NULL);
        pIntr = K2_GET_CONTAINER(K2OSKERN_OBJ_INTERRUPT, apIrq->PlatIrq.IntrList.mpHead, PlatIntr.IrqIntrListLink);

        apIrq->mState = KernIrq_Active_OneMasked;
        apIrq->mpOneMask_Holder = pIntr;

        KernArch_SetDevIrqMask(apIrq, TRUE);

        pIntr->mEnabled = FALSE;
        pIntr->mOneMaskingIrq = TRUE;

        KernPlat_IntrLocked_Queue(apThisCore->mCoreIx, apTick, pIntr);

        return;
    }


    K2_ASSERT(0);
}

void
KernPlat_GetPhysTable(
    UINT32 *                    apRetCount,
    K2OS_PHYSADDR_INFO const ** appRetTable
)
{
    *apRetCount = 0;
    gData.Plat.mfGetResTable(0, apRetCount, (void const **)appRetTable);
}

void
KernPlat_GetIoTable(
    UINT32 *                    apRetCount,
    K2OS_IOADDR_INFO const **   appRetTable
)
{
    *apRetCount = 0;
#if K2_TARGET_ARCH_IS_INTEL
    gData.Plat.mfGetResTable(1, apRetCount, (void const **)appRetTable);
#endif
}
