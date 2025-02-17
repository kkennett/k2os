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
   
    apInit->mVirtAddr = virtAddr = KernVirt_Reserve(apInit->mPageCount);
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
        "K2OSPLAT_DeviceCreate",
        (UINT32 *)&gData.Plat.mfDeviceCreate);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = XDL_FindExport(
        gData.Xdl.mpPlat,
        XDLProgData_Text,
        "K2OSPLAT_DeviceRemove",
        (UINT32 *)&gData.Plat.mfDeviceRemove);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = XDL_FindExport(
        gData.Xdl.mpPlat,
        XDLProgData_Text,
        "K2OSPLAT_DeviceAddRes",
        (UINT32 *)&gData.Plat.mfDeviceAddRes);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = XDL_GetHeaderPtr(gData.Xdl.mpPlat, &pXdlHeader);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    platEntryPoint = (XDL_pf_ENTRYPOINT)(UINT32)pXdlHeader->mEntryPoint;

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

//    K2OSKERN_Debug("\nK2OSKERN - Platform Initialized\n");
}

void
KernPlat_IntrLocked_Queue(
    UINT32                  aCoreIx,
    UINT64 const *          apHfTick,
    K2OSKERN_OBJ_INTERRUPT *apIntr
)
{
    K2OSKERN_CPUCORE_EVENT volatile *pEvent;

    //
    // self-reference is held to the interrupt until the it is handled
    //
    K2_ASSERT(apIntr->SchedItem.ObjRef.AsAny == NULL);
    K2_ASSERT(!apIntr->mInService);
    KernObj_CreateRef(&apIntr->SchedItem.ObjRef, &apIntr->Hdr);

    //
    // this CPU event will route to a call to:
    //
    // KernIntr_CpuEvent_Fired
    // 
    // which will queue a scheduler event to pulse the 
    // interrupt gate corresponding to the interrupt
    //
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
    K2LIST_LINK *               pListLink;
    K2OSKERN_OBJ_INTERRUPT *    pIntr;
    KernIntrDispType            intrDisp;
    UINT64                      hfTick;
    K2OSKERN_pf_Hook_Key        irqHook;

//    K2OSKERN_Debug(">>>%d: IRQ(%d)<<<\n", apThisCore->mCoreIx, apIrq->Config.mSourceIrq);
    pListLink = apIrq->InterruptList.mpHead;
    if (1 == apIrq->InterruptList.mNodeCount)
    {
        //
        // non-shared irq
        //
        pIntr = K2_GET_CONTAINER(K2OSKERN_OBJ_INTERRUPT, pListLink, IrqInterruptListLink);
        K2_ASSERT(pIntr->mVoteForEnabled);
        if (NULL == pIntr->mpHookKey)
        {
            K2OSKERN_Debug("*** Irq %d fired but interrupt not hooked. disabling IRQ\n");
            pIntr->mVoteForEnabled = FALSE;
            KernArch_SetDevIrqMask(apIrq, TRUE);
        }
        else
        {
            irqHook = *(pIntr->mpHookKey);
            if (NULL != irqHook)
            {
                intrDisp = irqHook(pIntr->mpHookKey, KernIntrAction_AtIrq);
            }
            else
            {
                intrDisp = KernIntrDisp_None;
            }
            switch (intrDisp)
            {
            case KernIntrDisp_Handled:
                // handled in the hook during irq. do not pulse the interrupt gate
                return;

            case KernIntrDisp_None:
                K2OSKERN_Debug("*** IRQ(%d) has only one user and they ignored it.  Disabling\n", apIrq->Config.mSourceIrq);
            case KernIntrDisp_Handled_Disable:
                pIntr->mVoteForEnabled = FALSE;
                KernArch_SetDevIrqMask(apIrq, TRUE);
                return;

            case KernIntrDisp_Fire:
                // pulse the interrupt gate, but do not need irq to be disabled
                KernArch_GetHfTimerTick(&hfTick);
                KernPlat_IntrLocked_Queue(apThisCore->mCoreIx, &hfTick, pIntr);
                return;

            case KernIntrDisp_Fire_Disable:
                // pulse the interrupt gate and leave the irq disabled
                KernArch_GetHfTimerTick(&hfTick);
                KernPlat_IntrLocked_Queue(apThisCore->mCoreIx, &hfTick, pIntr);
                pIntr->mVoteForEnabled = FALSE;
                KernArch_SetDevIrqMask(apIrq, TRUE);
                return;

            default:
                K2_ASSERT(0);
                break;
            };
        }
    }
    else
    {
        //
        // shared irq
        //
        K2_ASSERT(0);
    }
}

