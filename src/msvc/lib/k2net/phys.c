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

#include "ik2net.h"

K2LIST_ANCHOR gRegPhysList;

K2STAT  
K2NET_RegisterPhysLayerInstance(
    K2NET_PHYS_LAYER *apPhysLayer
)
{
    K2NET_REGPHYS_TRACK *   pTrack;
    K2LIST_LINK *           pListLink;

    if ((NULL == apPhysLayer) || 
        (apPhysLayer->Adapter.mType == K2NET_PHYS_AdapterInvalid) ||
        (apPhysLayer->Adapter.mType >= K2NET_PHYS_AdapterType_Count) ||
        (0 == apPhysLayer->Adapter.Addr.mLen) ||
        (K2NET_PHYS_ADAPTER_ADDR_VALUE_MAX_LEN <= apPhysLayer->Adapter.Addr.mLen) ||
        (NULL == apPhysLayer->Host.HostApi.Acquire) ||
        (NULL == apPhysLayer->Host.HostApi.Release))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pListLink = gRegPhysList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTrack = K2_GET_CONTAINER(K2NET_REGPHYS_TRACK, pListLink, ListLink);
            if (pTrack->mpPhysLayer == apPhysLayer)
            {
                return K2STAT_ERROR_ALREADY_EXISTS;
            }
            if ((pTrack->mpPhysLayer->Adapter.mType == apPhysLayer->Adapter.mType) &&
                (pTrack->mpPhysLayer->Adapter.Addr.mLen == apPhysLayer->Adapter.Addr.mLen) &&
                (0 == K2MEM_Compare(pTrack->mpPhysLayer->Adapter.Addr.mValue, apPhysLayer->Adapter.Addr.mValue, apPhysLayer->Adapter.Addr.mLen)))
            {
                return K2STAT_ERROR_ALREADY_MAPPED;
            }
        } while (NULL != pListLink);
    }

    pTrack = (K2NET_REGPHYS_TRACK *)K2NET_MemAlloc(sizeof(K2NET_REGPHYS_TRACK));
    if (NULL == pTrack)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    K2MEM_Zero(pTrack, sizeof(K2NET_REGPHYS_TRACK));

    pTrack->mpPhysLayer = apPhysLayer;

    K2LIST_AddAtTail(&gRegPhysList, &pTrack->ListLink);

    return K2STAT_NO_ERROR;
}

void    
K2NET_UnregisterPhysLayerInstance(
    K2NET_PHYS_LAYER *apPhysLayer
)
{
    K2NET_REGPHYS_TRACK *   pTrack;
    K2LIST_LINK *           pListLink;

    pListLink = gRegPhysList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTrack = K2_GET_CONTAINER(K2NET_REGPHYS_TRACK, pListLink, ListLink);
            if (pTrack->mpPhysLayer == apPhysLayer)
            {
                K2LIST_Remove(&gRegPhysList, &pTrack->ListLink);
                K2NET_MemFree(pTrack);
                return;
            }
        } while (NULL != pListLink);
    }
}

