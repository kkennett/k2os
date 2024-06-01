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

BOOL
K2NET_StackBuild(
    K2NET_HOST *                    apHost,
    K2NET_STACK_PROTO_CHUNK_HDR *   pStackProtoChunk
)
{
    K2NET_REGPROTO_TRACK *  pTrack;
    K2LIST_LINK *           pListLink;
    K2NET_HOST_LAYER *      pHostLayer;
    K2NET_PROTO_INSTANCE *  pNew;

    //
    // add this protocol on top of the host
    //

    pListLink = gRegProtoList.mpHead;
    if (NULL == pListLink)
        return FALSE;
    do {
        pTrack = K2_GET_CONTAINER(K2NET_REGPROTO_TRACK, pListLink, ListLink);
        if (pTrack->mProtocolId == pStackProtoChunk->mProtocolId)
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    if (NULL == pListLink)
        return FALSE;

    pNew = (K2NET_PROTO_INSTANCE *)K2NET_MemAlloc(sizeof(K2NET_PROTO_INSTANCE));
    if (NULL == pNew)
    {
        return FALSE;
    }

    pHostLayer = pTrack->mfFactory(pTrack->mProtocolId, pStackProtoChunk);

    if (NULL == pHostLayer)
    {
        K2NET_MemFree(pNew);
        return FALSE;
    }

    if ((NULL == pHostLayer->Layer.OnStart) ||
        (NULL == pHostLayer->Layer.OnFinish) ||
        (NULL == pHostLayer->Layer.Api.OnUp) ||
        (NULL == pHostLayer->Layer.Api.OnDown))
    {
        // required callbacks not present
        K2_ASSERT(0);
        K2NET_MemFree(pNew);
        return FALSE;
    }

    K2MEM_Zero(pNew, sizeof(K2NET_PROTO_INSTANCE));
    pHostLayer->mInstanceId = pNew->Prop.mInstanceId = gNextProtoInstanceId++;
    pNew->Prop.mProtocolId = pTrack->mProtocolId;
    pNew->Prop.mpDetail = (UINT8 const *)&pStackProtoChunk->Hdr;
    pNew->Prop.mDetailBytes = pStackProtoChunk->Hdr.mLen;
    pNew->mpHostLayer = pHostLayer;
    K2LIST_AddAtTail(&gProtoList, &pNew->ListLink);

    if (!apHost->OnAttach(apHost, &pHostLayer->Layer))
    {
        K2_ASSERT(0);
        return FALSE;
    }

    return TRUE;
}

BOOL    
K2NET_StackDefine(
    K2NET_STACK_DEF *apStackDef
)
{
    K2LIST_LINK *                   pListLink;
    K2NET_REGPHYS_TRACK *           pTrack;
    K2NET_PHYS_LAYER *              pPhysLayer;
    UINT32                          offset;
    K2NET_STACK_PROTO_CHUNK_HDR *   pProtoChunk;
    BOOL                            ok;

    if ((NULL == apStackDef) ||
        (apStackDef->mPhysType == K2NET_PHYS_AdapterInvalid) ||
        (apStackDef->mPhysType >= K2NET_PHYS_AdapterType_Count) ||
        (apStackDef->ConfigChunk.mLen < (sizeof(K2_CHUNK_HDR) + sizeof(K2NET_STACK_PROTO_CHUNK_HDR))) ||
        (apStackDef->ConfigChunk.mSubOffset < sizeof(K2_CHUNK_HDR)) ||
        (apStackDef->ConfigChunk.mSubOffset > (apStackDef->ConfigChunk.mLen - sizeof(K2NET_STACK_PROTO_CHUNK_HDR))) ||
        (apStackDef->PhysAdapterAddr.mLen == 0) ||
        (apStackDef->PhysAdapterAddr.mLen >= K2NET_PHYS_ADAPTER_ADDR_VALUE_MAX_LEN))
    {
        return FALSE;
    }

    //
    // find the registered physical adapter matching
    // the type and address in the stack's physical layer spec
    //
    pPhysLayer = NULL;
    pListLink = gRegPhysList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTrack = K2_GET_CONTAINER(K2NET_REGPHYS_TRACK, pListLink, ListLink);
            pPhysLayer = pTrack->mpPhysLayer;
            if ((pPhysLayer->Adapter.mType = apStackDef->mPhysType) &&
                (pPhysLayer->Adapter.Addr.mLen == apStackDef->PhysAdapterAddr.mLen) &&
                (0 == K2MEM_Compare(pPhysLayer->Adapter.Addr.mValue, apStackDef->PhysAdapterAddr.mValue, apStackDef->PhysAdapterAddr.mLen)))
            {
                break;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
    if (NULL == pPhysLayer)
    {
        return FALSE;
    }

    //
    // attach all subchunks of the config chunk
    //
    offset = apStackDef->ConfigChunk.mSubOffset;
    while (offset < (apStackDef->ConfigChunk.mLen - sizeof(K2NET_STACK_PROTO_CHUNK_HDR)))
    {
        pProtoChunk = (K2NET_STACK_PROTO_CHUNK_HDR *)(((UINT8 *)&apStackDef->ConfigChunk) + offset);
        ok = K2NET_StackBuild(&pPhysLayer->Host, pProtoChunk);
        if (!ok)
            break;
        offset += pProtoChunk->Hdr.mLen;
    }

    if (!ok)
    {
        K2_ASSERT(0);
        // undo
    }

    return ok;
}
