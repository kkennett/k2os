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
#include "root.h"

ROOTDEV_NODE    gRootDevNode;
K2OS_CRITSEC    gRootDevNode_RefSec;
ROOTDEV_NODE *  gpRootDevNode_CleanupList;

void
RootDev_Init(
    void
)
{
    static ROOTDEV_NODEREF sBaseRef;

    UINT_PTR    nodeName;
    BOOL        ok;

    ok = K2OS_CritSec_Init(&gRootDevNode_RefSec);
    K2_ASSERT(ok);

    gpRootDevNode_CleanupList = NULL;

    ok = RootDevNode_Init(&gRootDevNode, &sBaseRef);
    K2_ASSERT(ok);

    K2MEM_Copy(&nodeName, "SYS:", 4);
    gRootDevNode.InSec.mPlatContext = K2OS_Root_CreatePlatNode(0, nodeName, (UINT_PTR)&gRootDevNode, NULL, 0, NULL);
    K2_ASSERT(0 != gRootDevNode.InSec.mPlatContext);
}

void
#if REF_DEBUG
RootDevNode_CreateRef_Debug(
#else
RootDevNode_CreateRef(
#endif
    ROOTDEV_NODEREF *apRef,
    ROOTDEV_NODE *apDevNode
#if REF_DEBUG
    , char const *apFile,
    int aLine
#endif
)
{
    K2OS_CritSec_Enter(&gRootDevNode_RefSec);
    K2_ASSERT(NULL == apRef->mpRefNode);
    K2_ASSERT(0 != apDevNode->RefList.mNodeCount);
    K2LIST_AddAtTail(&apDevNode->RefList, &apRef->ListLink);
    apRef->mpRefNode = apDevNode;
#if REF_DEBUG
    apRef->mpFile = apFile;
    apRef->mLine = aLine;
    RootDebug_Printf("%08X ++ (%08X) %d %s:%d\n", apDevNode, apRef, apDevNode->RefList.mNodeCount, apFile, aLine);
#endif
    K2OS_CritSec_Leave(&gRootDevNode_RefSec);
}

void
#if REF_DEBUG
RootDevNode_ReleaseRef_Debug(
#else
RootDevNode_ReleaseRef(
#endif
    ROOTDEV_NODEREF *apRef
#if REF_DEBUG
    , char const *apFile,
    int aLine
#endif
)
{
    ROOTDEV_NODE *  pDevNode;

    K2OS_CritSec_Enter(&gRootDevNode_RefSec);
    pDevNode = apRef->mpRefNode;
    K2_ASSERT(NULL != pDevNode);
    K2LIST_Remove(&pDevNode->RefList, &apRef->ListLink);
    apRef->mpRefNode = NULL;
#if REF_DEBUG
    RootDebug_Printf("%08X -- (%08X) %d %s:%d (from %s:%d)\n", pDevNode, apRef, pDevNode->RefList.mNodeCount, apFile, aLine, apRef->mpFile, apRef->mLine);
    apRef->mpFile = NULL;
    apRef->mLine = 0;
#endif
    if (0 == pDevNode->RefList.mNodeCount)
    {
        *((ROOTDEV_NODE **)pDevNode) = gpRootDevNode_CleanupList;
        gpRootDevNode_CleanupList = pDevNode;
    }
    K2OS_CritSec_Leave(&gRootDevNode_RefSec);
}

BOOL 
RootDevNode_Init(
    ROOTDEV_NODE *      apDevNode,
    ROOTDEV_NODEREF *   apRef
)
{
    BOOL ok;

    K2MEM_Zero(apDevNode, sizeof(ROOTDEV_NODE));
    
    K2LIST_Init(&apDevNode->RefList);

    ok = K2OS_CritSec_Init(&apDevNode->Sec);
    K2_ASSERT(ok);

    apDevNode->mObjectCallWaitTokens[0] = K2OS_Gate_Create(FALSE);
    if (NULL == apDevNode->mObjectCallWaitTokens)
    {
        K2OS_CritSec_Done(&apDevNode->Sec);
        return FALSE;
    }

    apDevNode->InSec.mState = RootDevNodeState_NoDriver;

    K2LIST_Init(&apDevNode->InSec.IrqResList);
    K2LIST_Init(&apDevNode->InSec.IoResList);
    K2LIST_Init(&apDevNode->InSec.PhysResList);

    apDevNode->mfOnEvent = RootDevNode_OnEvent;

    apRef->mpRefNode = apDevNode;
    K2LIST_AddAtTail(&apDevNode->RefList, &apRef->ListLink);
#if REF_DEBUG
    RootDebug_Printf("%08X ++*(%08X) %d\n", apDevNode, apRef, apDevNode->RefList.mNodeCount);
    apRef->mpFile = __FILE__;
    apRef->mLine = __LINE__;
#endif

    return TRUE;
}

void
RootDevNode_Cleanup(
    ROOTDEV_NODE *apDevNode
)
{
    K2_ASSERT(0);
}

void
RootDevNode_OnEvent(
    ROOTDEV_NODE *  apDevNode,
    ROOTDEV_EVENT * apEvent
)
{
    K2_ASSERT(apEvent->RefDevNode.mpRefNode == apDevNode);

    switch (apEvent->mType)
    {
    case RootEvent_DriverEvent:
        RootDevNode_OnDriverEvent(apDevNode, apEvent);
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}