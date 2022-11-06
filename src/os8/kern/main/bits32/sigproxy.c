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

K2STAT  
KernSignalProxy_Create(
    K2OSKERN_OBJREF *   apRetRef
)
{
    K2OSKERN_OBJ_SIGNALPROXY *pProx;

    pProx = (K2OSKERN_OBJ_SIGNALPROXY *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_SIGNALPROXY));
    if (NULL == pProx)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    K2MEM_Zero(pProx, sizeof(K2OSKERN_OBJ_SIGNALPROXY));
    
    pProx->Hdr.mObjType = KernObj_SignalProxy;
    K2LIST_Init(&pProx->Hdr.RefObjList);

    pProx->SchedItem.mType = KernSchedItem_SignalProxy;

    KernObj_CreateRef(apRetRef, &pProx->Hdr);

    return K2STAT_NO_ERROR;
}

void    
KernSignalProxy_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_SIGNALPROXY *  apSignalProxy
)
{
    if (apSignalProxy->SchedItem.ObjRef.Ptr.AsHdr != NULL)
    {
        KernObj_ReleaseRef(&apSignalProxy->SchedItem.ObjRef);
    }

    K2MEM_Zero(apSignalProxy, sizeof(K2OSKERN_OBJ_SIGNALPROXY));

    KernHeap_Free(apSignalProxy);
}

void    
KernSignalProxy_Fire(
    K2OSKERN_OBJ_SIGNALPROXY *  apProxy,
    K2OSKERN_OBJ_NOTIFY *       apTargetNotify
)
{
    K2_ASSERT(apProxy->InFlightSelfRef.Ptr.AsHdr == NULL);
    apProxy->SchedItem.mType = KernSchedItem_SignalProxy;
    KernObj_CreateRef(&apProxy->InFlightSelfRef, &apProxy->Hdr);
    KernObj_CreateRef(&apProxy->SchedItem.ObjRef, &apTargetNotify->Hdr);
    KernTimer_GetHfTick(&apProxy->SchedItem.mHfTick);
    KernSched_QueueItem(&apProxy->SchedItem);
}
