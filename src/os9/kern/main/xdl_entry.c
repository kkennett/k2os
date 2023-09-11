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

//
// exported 
//
K2_CACHEINFO const * gpK2OSKERN_CacheInfo;

//
// globals
//
KERN_DATA gData;

K2STAT 
K2_CALLCONV_REGS 
xdl_entry(
    XDL *   apXdl,
    UINT32  aReason
    )
{
    K2MEM_Zero(&gData, sizeof(KERN_DATA));

    gData.mpShared = (K2OSKERN_SHARED *)aReason;
    gData.mCpuCoreCount = gData.mpShared->LoadInfo.mCpuCoreCount;

    K2LIST_Init(&gData.Thread.DeferredCsInitList);
    K2LIST_Init(&gData.Thread.CreateList);
    K2LIST_Init(&gData.Thread.ActiveList);
    K2LIST_Init(&gData.Thread.DeadList);
    K2OSKERN_SeqInit(&gData.Thread.ListSeqLock);
    K2LIST_Init(&gData.Token.FreeList);
    K2OSKERN_SeqInit(&gData.Token.SeqLock);
    K2TREE_Init(&gData.Iface.IdTree, NULL);
    K2LIST_Init(&gData.Iface.SubsList);
    K2OSKERN_SeqInit(&gData.Iface.SeqLock);
    K2TREE_Init(&gData.Iface.RequestTree, NULL);
    K2OSKERN_SeqInit(&gData.IpcEnd.SeqLock);
    K2LIST_Init(&gData.IpcEnd.KernList);
    K2OSKERN_SeqInit(&gData.VirtMap.SeqLock);
    K2TREE_Init(&gData.VirtMap.Tree, NULL);
    K2OSKERN_SeqInit(&gData.Intr.SeqLock);
    K2LIST_Init(&gData.Intr.Locked.IntrList);
    K2TREE_Init(&gData.Intr.Locked.IrqTree, NULL);

    gpK2OSKERN_CacheInfo = gData.mpShared->mpCacheInfo;

    gData.mpShared->FuncTab.Exec = Kern_Exec;

    gData.mpShared->FuncTab.Debug = K2OSKERN_Debug;
    gData.mpShared->FuncTab.Panic = K2OSKERN_Panic;
    gData.mpShared->FuncTab.MicroStall = K2OSKERN_MicroStall;
    gData.mpShared->FuncTab.SetIntr = K2OSKERN_SetIntr;
    gData.mpShared->FuncTab.GetIntr = K2OSKERN_GetIntr;
    gData.mpShared->FuncTab.SeqInit = K2OSKERN_SeqInit;
    gData.mpShared->FuncTab.SeqLock = K2OSKERN_SeqLock;
    gData.mpShared->FuncTab.SeqUnlock = K2OSKERN_SeqUnlock;
    gData.mpShared->FuncTab.GetCpuIndex = K2OSKERN_GetCpuIndex;
    gData.mpShared->FuncTab.HeapAlloc = KernHeap_Alloc;
    gData.mpShared->FuncTab.HeapFree = KernHeap_Free;

    gData.mpShared->FuncTab.CritSecInit = K2OS_CritSec_Init;
    gData.mpShared->FuncTab.CritSecEnter = K2OS_CritSec_Enter;
    gData.mpShared->FuncTab.CritSecLeave = K2OS_CritSec_Leave;

    gData.mpShared->FuncTab.Assert = KernEx_Assert;
    gData.mpShared->FuncTab.ExTrap_Mount = KernArch_ExTrapMount;
    gData.mpShared->FuncTab.ExTrap_Dismount = KernEx_TrapDismount;
    gData.mpShared->FuncTab.RaiseException = KernEx_RaiseException;

    //
    // first init is with no support functions.  reinit will not get called
    //
    KernXdl_AtXdlEntry();
    KernArch_AtXdlEntry();
    KernCpu_AtXdlEntry();

    return 0;
}

