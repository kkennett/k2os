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
#include "ik2osrpc.h"

#define EMIT_TRACE  0
#if EMIT_TRACE
#define FUNC_ENTER      K2OSRPC_DebugPrintf("%d:ENTER %s\n", K2OS_Thread_GetId(), __FUNCTION__);
#define FUNC_EXIT       K2OSRPC_DebugPrintf("%d:EXIT  %s(%d)\n", K2OS_Thread_GetId(), __FUNCTION__, __LINE__);
#else
#define FUNC_ENTER 
#define FUNC_EXIT
#endif

#define TRAP_EXCEPTIONS 0

typedef enum _K2OSRpc_WorkerMethodType K2OSRpc_WorkerMethodType;
enum _K2OSRpc_WorkerMethodType
{
    K2OSRpc_WorkerMethod_Invalid = 0,
    K2OSRpc_WorkerMethod_Create,
    K2OSRpc_WorkerMethod_Call,
    K2OSRpc_WorkerMethod_Release,
    K2OSRpc_WorkerMethod_Delete,

    K2OSRpc_WorkerMethodType_Count
};

typedef struct _K2OSRPC_SERVER_CLASS    K2OSRPC_SERVER_CLASS;
typedef struct _K2OSRPC_SERVER_OBJECT   K2OSRPC_SERVER_OBJECT;
typedef struct _K2OSRPC_SERVER          K2OSRPC_SERVER;
typedef struct _K2OSRPC_WORK_ITEM       K2OSRPC_WORK_ITEM;
typedef struct _K2OSRPC_SERVER_CLIENT   K2OSRPC_SERVER_CLIENT;
typedef struct _K2OSRPC_WORKER_THREAD   K2OSRPC_WORKER_THREAD;

struct _K2OSRPC_SERVER_CLASS
{
    INT_PTR                 mRefCount;

    K2OSRPC_OBJECT_CLASS    ObjectClass;

    K2OS_XDL                mXdlRefs[3];
    BOOL                    mIsRegistered;          // a deregistered class may still be in use

    UINT_PTR                mPublishContext;

    K2TREE_NODE             GlobalServerClassTreeNode;

    K2LIST_ANCHOR           ObjectList;
};

struct _K2OSRPC_SERVER_OBJECT
{
    K2OSRPC_SERVER_CLASS *  mpServerClass;

    K2TREE_NODE             GlobalServerObjectTreeNode;   // mUserVal is object id

    K2LIST_LINK             ServerClassObjectListLink;

    K2LIST_ANCHOR           UsageList;

    UINT_PTR                mClientRef;

    K2OSRPC_WORKER_THREAD * mpWorkerThread;     // if not null this is a single-threaded object
};

struct _K2OSRPC_SERVER_OBJECT_USAGE
{
    K2OSRPC_OBJECT_USAGE_HDR    UsageHdr;

    K2OSRPC_SERVER_OBJECT *     mpServerObject;

    K2LIST_LINK                 ObjectUsageListLink;

    K2LIST_LINK                 ServerClientUsageListLink;
};

struct _K2OSRPC_WORK_ITEM
{
    K2OSRPC_SERVER_CLIENT *         mpServerClient;
    K2LIST_LINK                     ServerClientWorkListLink;

    BOOL                            mIsCancelled;

    K2OSRPC_WORK_ITEM * volatile    mpNextWorkItem;

    K2OSRPC_SERVER_OBJECT_USAGE *   mpUsage;

    K2OSRPC_MSG_RESPONSE_HDR *      mpResp;

    UINT8 const *                   mpInBuf;
    UINT8 *                         mpOutBuf;

    K2OS_NOTIFY_TOKEN               mDoneNotifyToken;

    K2OSRpc_WorkerMethodType        mWorkerMethod;

    // must be last thing in struct
    K2OSRPC_MSG_REQUEST_HDR         RequestHdr;
    // input data follows, then output buffer
};

struct _K2OSRPC_SERVER_CLIENT
{
    K2OSRPC_THREAD  Thread;
    BOOL            mIsConnected;
    UINT_PTR        mRequestId;

    K2TREE_NODE     GlobalClientTreeNode;       // mUserVal is process id

    K2OS_MAILBOX    mMailbox;

    K2OS_IPCEND     mIpcEnd;

    K2OS_GATE_TOKEN mDisconnectedGateToken;

    K2LIST_ANCHOR   UsageList;
    K2OS_CRITSEC    UsageListSec;

    K2LIST_ANCHOR   WorkItemList;
    K2OS_CRITSEC    WorkItemListSec;
};

struct _K2OSRPC_SERVER
{
    K2OSRPC_THREAD      Thread;
    UINT_PTR volatile   mShutDown;
    K2OS_NOTIFY_TOKEN   mStopNotifyToken;
    K2OS_GATE_TOKEN     mActiveGateToken;
};

struct _K2OSRPC_WORKER_THREAD
{
    K2OSRPC_THREAD                  Thread;

    K2LIST_LINK                     GlobalListLink;

    BOOL                            mIsActive;

    K2OSRPC_WORK_ITEM * volatile    mpWorkItemTransferList;

    K2OSRPC_SERVER_OBJECT *         mpOwnedByObject;  // if non-null this is a single-threaded object's thread
};

typedef struct _K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE;
struct _K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE
{
    K2OSRPC_MSG_RESPONSE_HDR                    ResponseHdr;
    K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA    Data;
};

static K2OS_CRITSEC     sgGraphSec;
static K2TREE_ANCHOR    sgGlobalServerClassTree;
static K2TREE_ANCHOR    sgGlobalServerObjectTree;
static INT_PTR          sgNextServerObjectId;
static INT_PTR          sgGlobalServerClassCount;
static K2OSRPC_SERVER * sgpServer;
static K2TREE_ANCHOR    sgGlobalServerClientTree;
static K2LIST_ANCHOR    sgIdleThreadList;
static K2LIST_ANCHOR    sgActiveThreadList;
static K2OS_CRITSEC     sgThreadListSec;
static K2OS_GATE_TOKEN  sgAlwaysShutGateToken;

void K2OSRPC_ServerClass_Release(K2OSRPC_SERVER_CLASS * apServerClass);
void K2OSRPC_ServerUsage_ReleaseInternal(K2OSRPC_SERVER_OBJECT_USAGE *apServerUsage, K2OSRPC_SERVER_CLIENT * apServerClient);

//
// ------------------------------------------------------------------------
//

K2STAT
K2OSRPC_ServerObject_CreateOnThread(
    K2_GUID128 const *              apClassId,
    UINT_PTR                        aCreatorProcessId,
    UINT_PTR                        aCreatorContext,
    K2OSRPC_SERVER_OBJECT_USAGE **  appRetUsage,
    K2OSRPC_SERVER_CLIENT *         apServerClient
)
{
    K2TREE_NODE *                   pTreeNode;
    K2OSRPC_SERVER_CLASS *          pServerClass;
    K2OSRPC_SERVER_OBJECT *         pServerObject;
    K2OSRPC_SERVER_OBJECT_USAGE *   pUsage;
    K2_EXCEPTION_TRAP               trap;
    K2STAT                          stat;
    K2OSRPC_OBJECT_CREATE           cret;

    FUNC_ENTER;

    *appRetUsage = NULL;

    K2OS_CritSec_Enter(&sgGraphSec);

    pTreeNode = K2TREE_Find(&sgGlobalServerClassTree, (UINT32)apClassId);
    if (NULL != pTreeNode)
    {
        pServerClass = K2_GET_CONTAINER(K2OSRPC_SERVER_CLASS, pTreeNode, GlobalServerClassTreeNode);
        K2_ASSERT(pServerClass->mIsRegistered);
        K2ATOMIC_Inc(&pServerClass->mRefCount);
    }
    else
    {
        pServerClass = NULL;
    }

    K2OS_CritSec_Leave(&sgGraphSec);

    if (NULL == pServerClass)
    {
        //        K2OSRPC_DebugPrintf("Create: Class not found\n");
        FUNC_EXIT;
        return K2STAT_ERROR_CLASS_NOT_FOUND;
    }

    pServerObject = (K2OSRPC_SERVER_OBJECT *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER_OBJECT));
    if (NULL == pServerObject)
    {
        //        K2OSRPC_DebugPrintf("Create: failed memory alloc\n");
        stat = K2OS_Thread_GetLastStatus();
    }
    else
    {
        do {
            pUsage = (K2OSRPC_SERVER_OBJECT_USAGE *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER_OBJECT_USAGE));
            if (NULL == pUsage)
            {
                //                K2OSRPC_DebugPrintf("Create: failed memory alloc(2)\n");
                stat = K2OS_Thread_GetLastStatus();
                break;
            }

            K2MEM_Zero(pServerObject, sizeof(K2OSRPC_SERVER_OBJECT));
            pServerObject->mpServerClass = pServerClass;
            K2LIST_Init(&pServerObject->UsageList);

            K2MEM_Zero(pUsage, sizeof(K2OSRPC_SERVER_OBJECT_USAGE));
            pUsage->UsageHdr.mIsServer = TRUE;
            pUsage->UsageHdr.mRefCount = 1;
            pUsage->mpServerObject = pServerObject;

            cret.mClassPublishContext = pServerClass->mPublishContext;
            cret.mCallerProcessId = aCreatorProcessId;
            cret.mCallerContext = aCreatorContext;
            cret.mRemoteDisconnectedGateToken = (NULL == apServerClient) ? sgAlwaysShutGateToken : apServerClient->mDisconnectedGateToken;
#if TRAP_EXCEPTIONS
            stat = K2_EXTRAP(&trap, pServerClass->ObjectClass.Create(&cret, &pServerObject->mClientRef));
#else
            stat = pServerClass->ObjectClass.Create(&cret, &pServerObject->mClientRef);
#endif
            if (!K2STAT_IS_ERROR(stat))
            {
                pServerObject->GlobalServerObjectTreeNode.mUserVal = K2ATOMIC_AddExchange(&sgNextServerObjectId, 1);

                pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal = K2ATOMIC_AddExchange(&gK2OSRPC_NextGlobalUsageId, 1);

                K2OS_CritSec_Enter(&gK2OSRPC_GlobalUsageTreeSec);
                K2OS_CritSec_Enter(&sgGraphSec);

                //
                // clientref is in object
                // object goes into object tree and onto class object list
                //
                K2TREE_Insert(&sgGlobalServerObjectTree, pServerObject->GlobalServerObjectTreeNode.mUserVal, &pServerObject->GlobalServerObjectTreeNode);
                K2LIST_AddAtTail(&pServerClass->ObjectList, &pServerObject->ServerClassObjectListLink);
                K2ATOMIC_Inc(&pServerClass->mRefCount);

                //
                // usage goes onto object usage list and into global usage tree
                //
                // server object ref count is already one
                K2LIST_AddAtTail(&pServerObject->UsageList, &pUsage->ObjectUsageListLink);
                K2TREE_Insert(&gK2OSRPC_GlobalUsageTree, pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal, &pUsage->UsageHdr.GlobalUsageTreeNode);

                K2OS_CritSec_Leave(&sgGraphSec);
                K2OS_CritSec_Leave(&gK2OSRPC_GlobalUsageTreeSec);
            }

            if (K2STAT_IS_ERROR(stat))
            {
                K2OSRPC_DebugPrintf("***RPCCreate: failed create call (stat=0x%08X)\n", stat);
                K2OS_Heap_Free(pUsage);
            }
            else
            {
                *appRetUsage = pUsage;
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Heap_Free(pServerObject);
        }
    }

    K2OSRPC_ServerClass_Release(pServerClass);

    FUNC_EXIT;
    return stat;
}

void
K2OSRPC_ServerObject_DeleteOnThread(
    K2OSRPC_SERVER_OBJECT * apServerObject,
    K2OSRPC_SERVER_CLIENT * apServerClient
)
{
    K2OSRPC_SERVER_CLASS *  pServerClass;
    K2OSRPC_WORKER_THREAD * pWorkerThread;
    K2_EXCEPTION_TRAP       trap;
    K2STAT                  stat;
    K2OSRPC_OBJECT_DELETE   del;

    FUNC_ENTER;

    K2_ASSERT(0 == apServerObject->UsageList.mNodeCount);

    pServerClass = apServerObject->mpServerClass;

    K2OS_CritSec_Enter(&sgGraphSec);

    K2LIST_Remove(&pServerClass->ObjectList, &apServerObject->ServerClassObjectListLink);

    K2TREE_Remove(&sgGlobalServerObjectTree, &apServerObject->GlobalServerObjectTreeNode);

    K2OS_CritSec_Leave(&sgGraphSec);

    pWorkerThread = apServerObject->mpWorkerThread;
    if (NULL != pWorkerThread)
    {
        K2OS_CritSec_Enter(&sgThreadListSec);

        K2_ASSERT(pWorkerThread->mIsActive);

        pWorkerThread->mpOwnedByObject = NULL;
        apServerObject->mpWorkerThread = NULL;

        K2LIST_Remove(&sgActiveThreadList, &pWorkerThread->GlobalListLink);

        K2LIST_AddAtTail(&sgIdleThreadList, &pWorkerThread->GlobalListLink);

        pWorkerThread->mIsActive = FALSE;

        K2OS_CritSec_Leave(&sgThreadListSec);
    }

    //
    // run the delete method now
    //
    del.mRemoteDisconnectedGateToken = (NULL == apServerClient) ? sgAlwaysShutGateToken : apServerClient->mDisconnectedGateToken;
    del.mCreatorRef = apServerObject->mClientRef;
#if TRAP_EXCEPTIONS
    stat = K2_EXTRAP(&trap, pServerClass->ObjectClass.Delete(&del));
#else
    stat = pServerClass->ObjectClass.Delete(&del);
#endif
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSRPC_DebugPrintf("***Object %d delete failed with status %08X\n", apServerObject->mClientRef, stat);
    }

    K2OSRPC_ServerClass_Release(pServerClass);

    K2OS_Heap_Free(apServerObject);

    FUNC_EXIT;
}

void
K2OSRPC_WorkerThread_AtExit(
    K2OSRPC_THREAD *apThread
)
{
    K2OSRPC_WORKER_THREAD * pWorkerThread;

    FUNC_ENTER;
    pWorkerThread = K2_GET_CONTAINER(K2OSRPC_WORKER_THREAD, apThread, Thread);

    K2OS_Heap_Free(pWorkerThread);
    FUNC_EXIT;
}

void
K2OSRPC_WorkerThread_ExecWorkItem(
    K2OSRPC_WORKER_THREAD * apWorkerThread,
    K2OSRPC_WORK_ITEM *     apWorkItem
)
{
    K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA *  pData;
    K2OSRPC_SERVER_OBJECT_USAGE *               pUsage;
    K2STAT                                      stat;
    K2OSRPC_SERVER_OBJECT *                     pServerObject;
    K2OSRPC_SERVER_CLIENT *                     pServerClient;
    K2_EXCEPTION_TRAP                           trap;
    K2OSRPC_OBJECT_CALL                         call;

    FUNC_ENTER;

    pServerClient = apWorkItem->mpServerClient;
    if (NULL != pServerClient)
    {
        //
        // remove from serverclient work item list as we're going
        // to do it and don't want it to be cancelled in the middle
        // of us executing it
        //
        K2OS_CritSec_Enter(&pServerClient->WorkItemListSec);
        K2LIST_Remove(&pServerClient->WorkItemList, &apWorkItem->ServerClientWorkListLink);
        K2OS_CritSec_Leave(&pServerClient->WorkItemListSec);
    }

    if (!apWorkItem->mIsCancelled)
    {
        switch (apWorkItem->mWorkerMethod)
        {
        case K2OSRpc_WorkerMethod_Create:
            //K2OSRPC_DebugPrintf("Create on Thread %d\n", apWorkerThread->Thread.mThreadId);
            K2_ASSERT(NULL == apWorkItem->mpUsage);
            K2_ASSERT(apWorkItem->RequestHdr.mInByteCount == sizeof(K2OSRPC_MSG_CREATE_REQUEST_DATA));
            K2_ASSERT(apWorkItem->RequestHdr.mOutBufSizeProvided >= sizeof(K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA));
            pData = (K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA *)apWorkItem->mpOutBuf;

            pUsage = NULL;
            stat = K2OSRPC_ServerObject_CreateOnThread(
                &(((K2OSRPC_MSG_CREATE_REQUEST_DATA const *)apWorkItem->mpInBuf)->mClassId),
                (pServerClient != NULL) ? pServerClient->GlobalClientTreeNode.mUserVal : K2OS_Process_GetId(),
                ((K2OSRPC_MSG_CREATE_REQUEST_DATA const *)apWorkItem->mpInBuf)->mCreatorContext,
                &pUsage,
                pServerClient
            );

            apWorkItem->mpResp->mStatus = stat;

            if (!K2STAT_IS_ERROR(stat))
            {
                K2_ASSERT(NULL != pUsage);

                pData->mServerObjectId = pUsage->mpServerObject->GlobalServerObjectTreeNode.mUserVal;
                pData->mServerUsageId = pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal;
                apWorkItem->mpResp->mResultsByteCount = sizeof(K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA);

                if (pUsage->mpServerObject->mpServerClass->ObjectClass.mSingleThreadPerObject)
                {
                    pUsage->mpServerObject->mpWorkerThread = apWorkerThread;
                    apWorkerThread->mpOwnedByObject = pUsage->mpServerObject;
                }

                if (NULL != pServerClient)
                {
                    //
                    // add usage to the serverclient's usage list
                    //
                    K2OS_CritSec_Enter(&pServerClient->UsageListSec);
                    K2LIST_AddAtTail(&pServerClient->UsageList, &pUsage->ServerClientUsageListLink);
                    K2OS_CritSec_Leave(&pServerClient->UsageListSec);
                }
            }

            break;

        case K2OSRpc_WorkerMethod_Call:
            //K2OSRPC_DebugPrintf("Call on Thread %d\n", apWorkerThread->Thread.mThreadId);
            pUsage = apWorkItem->mpUsage;
            K2_ASSERT(NULL != pUsage);

            pServerObject = apWorkItem->mpUsage->mpServerObject;

            call.mCreatorRef = pServerObject->mClientRef;
            call.Args.mMethodId = apWorkItem->RequestHdr.mTargetMethodId;
            call.Args.mpInBuf = apWorkItem->mpInBuf;
            call.Args.mInBufByteCount = apWorkItem->RequestHdr.mInByteCount;
            call.Args.mpOutBuf = apWorkItem->mpOutBuf;
            call.Args.mOutBufByteCount = apWorkItem->RequestHdr.mOutBufSizeProvided;
            call.mRemoteDisconnectedGateToken = (NULL == pServerClient) ? NULL : pServerClient->mDisconnectedGateToken;
#if TRAP_EXCEPTIONS
            stat = K2_EXTRAP(&trap, pServerObject->mpServerClass->ObjectClass.Call(
                &call,
                &apWorkItem->mpResp->mResultsByteCount));
#else
            stat = pServerObject->mpServerClass->ObjectClass.Call(
                &call,
                &apWorkItem->mpResp->mResultsByteCount);
#endif

            if (NULL != pServerClient)
            {
                // 
                // this ref taken when usage found in OnRecv
                //
                K2OSRPC_ServerUsage_ReleaseInternal(pUsage, pServerClient);
            }

            apWorkItem->mpResp->mStatus = stat;

            break;

        case K2OSRpc_WorkerMethod_Release:
            //K2OSRPC_DebugPrintf("Release on Thread %d\n", apWorkerThread->Thread.mThreadId);
            pUsage = apWorkItem->mpUsage;
            K2_ASSERT(NULL != pUsage);
            //
            // usage already removed from serverclient usage list
            //
            K2OSRPC_ServerUsage_ReleaseInternal(pUsage, pServerClient);
            apWorkItem->mpResp->mStatus = K2STAT_NO_ERROR;
            break;

        case K2OSRpc_WorkerMethod_Delete:
            //K2OSRPC_DebugPrintf("Delete on Thread %d\n", apWorkerThread->Thread.mThreadId);
            K2_ASSERT(NULL == pServerClient);   // not possible
            K2_ASSERT(NULL != apWorkItem->mpUsage);
            K2_ASSERT(apWorkItem->RequestHdr.mRequestType == 0);
            K2_ASSERT(apWorkItem->RequestHdr.mInByteCount == 0);
            K2_ASSERT(apWorkItem->RequestHdr.mOutBufSizeProvided == 0);
            K2OSRPC_ServerObject_DeleteOnThread((K2OSRPC_SERVER_OBJECT *)apWorkItem->mpUsage, pServerClient);
            if (NULL != apWorkItem->mpResp)
            {
                apWorkItem->mpResp->mStatus = K2STAT_NO_ERROR;
            }
            break;

        case K2OSRpc_WorkerMethod_Invalid:
        default:
            K2_ASSERT(0);
        }

        //
        // send the result (even if failed) to a remote client if needed
        //
        if (NULL != pServerClient)
        {
            //
            // output buffer follows response header at end of work item
            //
            K2OS_Ipc_Send(
                pServerClient->mIpcEnd,
                apWorkItem->mpResp,
                sizeof(K2OSRPC_MSG_RESPONSE_HDR) + apWorkItem->mpResp->mResultsByteCount
            );
        }
    }

    if (NULL != apWorkItem->mDoneNotifyToken)
    {
        //
        // signal completion of work item
        //
        K2OS_Notify_Signal(apWorkItem->mDoneNotifyToken);
    }

    //
    // lastly free the memory the work item was using
    //
    K2OS_Heap_Free(apWorkItem);
    FUNC_EXIT;
}

void
K2OSRPC_WorkerThread_DoWork(
    K2OSRPC_THREAD *apThread
)
{
    K2OSRPC_WORKER_THREAD * pWorkerThread;
    K2OSRPC_SERVER_OBJECT *  pServerObject;
    K2OSRPC_WORK_ITEM *     pWorkItem;
    K2OSRPC_WORK_ITEM *     pLast;
    K2OSRPC_WORK_ITEM *     pNext;

    FUNC_ENTER;

    pWorkerThread = K2_GET_CONTAINER(K2OSRPC_WORKER_THREAD, apThread, Thread);

    pWorkItem = (K2OSRPC_WORK_ITEM *)K2ATOMIC_Exchange((UINT_PTR volatile *)&pWorkerThread->mpWorkItemTransferList, 0);

    pServerObject = pWorkerThread->mpOwnedByObject;

    if (NULL != pServerObject)
    {
        //
        // this thread currently belongs to an object
        //
        if (NULL != pWorkItem)
        {
            //
            // reverse the list
            //
            pLast = NULL;
            do {
                pNext = pWorkItem->mpNextWorkItem;
                pWorkItem->mpNextWorkItem = pLast;
                pLast = pWorkItem;
                pWorkItem = pNext;
            } while (NULL != pWorkItem);
            pWorkItem = pLast;

            //
            // execute everything on the list
            //
            do {
                pNext = pWorkItem;
                pWorkItem = pNext->mpNextWorkItem;
                //
                // must not be a create (localobject is not NULL)
                //
                K2_ASSERT(pNext->mWorkerMethod != K2OSRpc_WorkerMethod_Create);
                K2OSRPC_WorkerThread_ExecWorkItem(pWorkerThread, pNext);

            } while (NULL != pWorkItem);
        }
    }
    else
    {
        //
        // this thread does not (yet) belong to an object
        //
        if (NULL != pWorkItem)
        {
            //
            // the work item should be a singleton
            //
            K2_ASSERT(NULL == pWorkItem->mpNextWorkItem);

            K2OSRPC_WorkerThread_ExecWorkItem(pWorkerThread, pWorkItem);
        }

        //
        // if this was a successful create the object will have been assigned to this thread
        // and vice versa
        //
        if (NULL == pWorkerThread->mpOwnedByObject)
        {
            //
            // this thread still doesn't belong to an object and so goes back onto the idle list
            //
            K2OS_CritSec_Enter(&sgThreadListSec);
            K2_ASSERT(pWorkerThread->mIsActive);
            K2LIST_Remove(&sgActiveThreadList, &pWorkerThread->GlobalListLink);
            K2LIST_AddAtTail(&sgIdleThreadList, &pWorkerThread->GlobalListLink);
            pWorkerThread->mIsActive = FALSE;
            K2OS_CritSec_Leave(&sgThreadListSec);
        }
    }

    FUNC_EXIT;
}

BOOL
K2OSRPC_AddWorkerThread(
    void
)
{
    K2OSRPC_WORKER_THREAD * pWorkerThread;
    K2STAT                  stat;

    FUNC_ENTER;

    pWorkerThread = (K2OSRPC_WORKER_THREAD *)K2OS_Heap_Alloc(sizeof(K2OSRPC_WORKER_THREAD));
    if (NULL == pWorkerThread)
    {
        FUNC_EXIT;
        return FALSE;
    }

    K2MEM_Zero(pWorkerThread, sizeof(K2OSRPC_WORKER_THREAD));

    stat = K2OSRPC_Thread_Create(&pWorkerThread->Thread, K2OSRPC_WorkerThread_AtExit, K2OSRPC_WorkerThread_DoWork);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pWorkerThread);
        FUNC_EXIT;
        return FALSE;
    }

    K2OS_CritSec_Enter(&sgThreadListSec);

    K2LIST_AddAtTail(&sgIdleThreadList, &pWorkerThread->GlobalListLink);

    K2_ASSERT(!pWorkerThread->mIsActive);

    K2OS_CritSec_Leave(&sgThreadListSec);

    FUNC_EXIT;
    return TRUE;
}

K2OSRPC_WORKER_THREAD  *
K2OSRPC_GetWorkerThread(
    void
)
{
    K2OSRPC_WORKER_THREAD * pWorkerThread;

    FUNC_ENTER;

    do {
        K2OS_CritSec_Enter(&sgThreadListSec);

        if (sgIdleThreadList.mNodeCount > 0)
        {
            pWorkerThread = K2_GET_CONTAINER(K2OSRPC_WORKER_THREAD, sgIdleThreadList.mpHead, GlobalListLink);
            K2_ASSERT(!pWorkerThread->mIsActive);
            K2LIST_Remove(&sgIdleThreadList, &pWorkerThread->GlobalListLink);
            K2LIST_AddAtTail(&sgActiveThreadList, &pWorkerThread->GlobalListLink);
            pWorkerThread->mIsActive = TRUE;
        }
        else
        {
            pWorkerThread = NULL;
        }

        K2OS_CritSec_Leave(&sgThreadListSec);

        if (NULL != pWorkerThread)
        {
            break;
        }

        if (!K2OSRPC_AddWorkerThread())
        {
            K2OS_Thread_Sleep(500);
        }

    } while (1);

    FUNC_EXIT;
    return pWorkerThread;
}

BOOL
K2OSRPC_ServerObject_AcquireUsage(
    UINT_PTR                        aServerObjectId,
    K2OSRPC_SERVER_OBJECT_USAGE **  apRetUsage
);

void
K2OSRPC_ServerClient_OnConnect(
    K2OS_IPCEND aEndpoint,
    void *      apContext,
    UINT_PTR    aRemoteMaxMsgBytes
)
{
    K2OSRPC_SERVER_CLIENT * pServerClient;

    FUNC_ENTER;

    pServerClient = (K2OSRPC_SERVER_CLIENT *)apContext;
    pServerClient->mIsConnected = TRUE;

    FUNC_EXIT;
}

void
K2OSRPC_ServerClient_RespondWithError(
    K2OS_IPCEND aEndpoint,
    UINT_PTR    aCallerRef,
    K2STAT      aErrorStatus
)
{
    K2OSRPC_MSG_RESPONSE_HDR  respHdr;

    FUNC_ENTER;

    K2_ASSERT(K2STAT_IS_ERROR(aErrorStatus));

    K2MEM_Zero(&respHdr, sizeof(respHdr));
    respHdr.mCallerRef = aCallerRef;
    respHdr.mStatus = aErrorStatus;
    K2OS_Ipc_Send(aEndpoint, &respHdr, sizeof(respHdr));
    FUNC_EXIT;
}

void
K2OSRPC_ServerClient_OnRecv(
    K2OS_IPCEND     aEndpoint,
    void *          apContext,
    UINT8 const *   apData,
    UINT_PTR        aByteCount
)
{
    K2OSRPC_SERVER_CLIENT *                 pServerClient;
    K2OSRPC_MSG_REQUEST_HDR const *         pReqHdr;
    BOOL                                    ok;
    UINT_PTR                                workItemBytes;
    K2OSRPC_WORK_ITEM *                     pWorkItem;
    K2LIST_LINK *                           pListLink;
    K2OSRPC_SERVER_OBJECT_USAGE *           pUsage;
    K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE  acquireResp;
    K2OSRPC_SERVER_OBJECT *                 pServerObject;
    K2OSRPC_WORKER_THREAD *                 pWorkerThread;
    UINT_PTR                                v;

    FUNC_ENTER;

    pServerClient = (K2OSRPC_SERVER_CLIENT *)apContext;
    pReqHdr = (K2OSRPC_MSG_REQUEST_HDR const *)apData;

#if 0
    K2OSRPC_DebugPrintf("ServerClient recv %d bytes\n", aByteCount);
    for (workItemBytes = 0; workItemBytes < aByteCount; workItemBytes++)
    {
        K2OSRPC_DebugPrintf("%02X ", apData[workItemBytes]);
    }
    K2OSRPC_DebugPrintf("\n");
#endif

    if ((aByteCount < sizeof(K2OSRPC_MSG_REQUEST_HDR)) ||
        (pReqHdr->mCallerRef == 0))
    {
        K2OSRPC_DebugPrintf("***RPC Recv data too small to be a request (%d) or with null caller ref\n", aByteCount);
        FUNC_EXIT;
        return;
    }

    ok = FALSE;

    if ((pReqHdr->mRequestType != K2OSRpc_ServerClientRequest_Invalid) &&
        (pReqHdr->mRequestType < K2OSRpc_ServerClientRequestType_Count) &&
        (aByteCount == (pReqHdr->mInByteCount + sizeof(K2OSRPC_MSG_REQUEST_HDR))))
    {
        switch (pReqHdr->mRequestType)
        {
        case K2OSRpc_ServerClientRequest_Create:
            if ((pReqHdr->mInByteCount == sizeof(K2OSRPC_MSG_CREATE_REQUEST_DATA)) &&
                (pReqHdr->mOutBufSizeProvided >= sizeof(K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA)))
                ok = TRUE;
            break;

        case K2OSRpc_ServerClientRequest_Acquire:
            if ((pReqHdr->mInByteCount == 0) &&
                (pReqHdr->mOutBufSizeProvided >= sizeof(K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA)))
                ok = TRUE;
            break;

        case K2OSRpc_ServerClientRequest_Call:
            ok = TRUE;
            break;

        case K2OSRpc_ServerClientRequest_Release:
            if (pReqHdr->mInByteCount == 0)
                ok = TRUE;
            break;
        }
    }

    if (!ok)
    {
        K2OSRPC_DebugPrintf("***RPC Recv malformed request hdr (%d)\n", aByteCount);
        K2OSRPC_DebugPrintf("mCallerRef              %08X\n", pReqHdr->mCallerRef);
        K2OSRPC_DebugPrintf("mInByteCount            %04X\n", pReqHdr->mInByteCount);
        K2OSRPC_DebugPrintf("mOutBufSizeProvided     %04X\n", pReqHdr->mOutBufSizeProvided);
        K2OSRPC_DebugPrintf("mRequestType            %04X\n", pReqHdr->mRequestType);
        K2OSRPC_DebugPrintf("mTargetId               %08X\n", pReqHdr->mTargetId);
        K2OSRPC_DebugPrintf("mTargetMethodId         %04X\n", pReqHdr->mTargetMethodId);
        K2OSRPC_ServerClient_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_BAD_FORMAT);
        FUNC_EXIT;
        return;
    }

    //
    // caller ref must not match a currently held caller ref
    //
    if (pServerClient->WorkItemList.mNodeCount != 0)
    {
        K2OS_CritSec_Enter(&pServerClient->WorkItemListSec);
        pListLink = pServerClient->WorkItemList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pWorkItem = K2_GET_CONTAINER(K2OSRPC_WORK_ITEM, pListLink, ServerClientWorkListLink);
                pListLink = pListLink->mpNext;
                if (pWorkItem->RequestHdr.mCallerRef == pReqHdr->mCallerRef)
                    break;
            } while (NULL != pListLink);
        }
        K2OS_CritSec_Leave(&pServerClient->WorkItemListSec);

        if (NULL != pListLink)
        {
            //
            // client sent duplicate caller ref to one that is in flight
            //
            K2OSRPC_DebugPrintf("***RPC Recv caller ref that already is in flight\n");
            K2OSRPC_ServerClient_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_IN_USE);
            FUNC_EXIT;
            return;
        }
    }

    if (pReqHdr->mRequestType == K2OSRpc_ServerClientRequest_Acquire)
    {
        if (!K2OSRPC_ServerObject_AcquireUsage(pReqHdr->mTargetId, &pUsage))
        {
            K2OSRPC_DebugPrintf("***RPC Recv call to acquire unknown object\n");
            K2OSRPC_ServerClient_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_OBJECT_NOT_FOUND);
            FUNC_EXIT;
            return;
        }

        K2_ASSERT(pUsage->mpServerObject->GlobalServerObjectTreeNode.mUserVal == pReqHdr->mTargetId);

        K2MEM_Zero(&acquireResp, sizeof(acquireResp));
        acquireResp.ResponseHdr.mCallerRef = pReqHdr->mCallerRef;
        acquireResp.ResponseHdr.mResultsByteCount = sizeof(acquireResp.Data);
        acquireResp.ResponseHdr.mStatus = K2STAT_NO_ERROR;
        acquireResp.Data.mServerObjectId = pUsage->mpServerObject->GlobalServerObjectTreeNode.mUserVal;
        acquireResp.Data.mServerUsageId = pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal;

        if (!K2OS_Ipc_Send(aEndpoint, &acquireResp, sizeof(acquireResp)))
        {
            K2OSRPC_ServerUsage_Release(pUsage);
        }
        else
        {
            K2OS_CritSec_Enter(&pServerClient->UsageListSec);
            K2LIST_AddAtTail(&pServerClient->UsageList, &pUsage->ServerClientUsageListLink);
            K2OS_CritSec_Leave(&pServerClient->UsageListSec);
        }

        FUNC_EXIT;
        return;
    }

    pUsage = NULL;

    if (pReqHdr->mRequestType != K2OSRpc_ServerClientRequest_Create)
    {
        //
        // this is a call or a release.  request header has a usage id that this 
        // serverclient connection should be holding
        //
        K2OS_CritSec_Enter(&pServerClient->UsageListSec);

        pListLink = pServerClient->UsageList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pUsage = K2_GET_CONTAINER(K2OSRPC_SERVER_OBJECT_USAGE, pListLink, ServerClientUsageListLink);
                if (pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal == pReqHdr->mTargetId)
                {
                    K2ATOMIC_Inc(&pUsage->UsageHdr.mRefCount);
                    break;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }

        K2OS_CritSec_Leave(&pServerClient->UsageListSec);

        if (NULL == pListLink)
        {
            //
            // usage not found
            //
            K2OSRPC_DebugPrintf("***RPC Recv call to unknown usage\n");
            K2OSRPC_ServerClient_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_USAGE_NOT_FOUND);
            FUNC_EXIT;
            return;
        }
        K2_ASSERT(NULL != pUsage);
        pServerObject = pUsage->mpServerObject;
    }
    else
    {
        pServerObject = NULL;
    }

    //
    // method action will proceed on a worker thread
    //

    //
    // copy out the request so we can queue it to a worker thread and let the process connection
    // thread resume for other calls that may come in
    //
    workItemBytes =
        sizeof(K2OSRPC_WORK_ITEM) - sizeof(K2OSRPC_MSG_REQUEST_HDR) +
        ((aByteCount + 3) & ~3) +
        sizeof(K2OSRPC_MSG_RESPONSE_HDR) +
        pReqHdr->mOutBufSizeProvided;
    pWorkItem = (K2OSRPC_WORK_ITEM *)K2OS_Heap_Alloc(workItemBytes);
    if (NULL == pWorkItem)
    {
        K2OSRPC_DebugPrintf("***RPC memory alloc failed\n", aByteCount);
        K2OSRPC_ServerClient_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_OUT_OF_MEMORY);
        if (NULL != pUsage)
        {
            K2OSRPC_ServerUsage_Release(pUsage);
        }
        FUNC_EXIT;
        return;
    }
    K2MEM_Zero(pWorkItem, sizeof(K2OSRPC_WORK_ITEM) - sizeof(K2OSRPC_MSG_REQUEST_HDR));
    K2MEM_Copy(&pWorkItem->RequestHdr, pReqHdr, aByteCount);
    pWorkItem->mpServerClient = pServerClient;
    pWorkItem->mpUsage = pUsage;
    pWorkItem->mpInBuf = ((UINT8 const *)pWorkItem) + sizeof(K2OSRPC_WORK_ITEM);
    pWorkItem->mpResp = (K2OSRPC_MSG_RESPONSE_HDR *)
        (((UINT8 *)pWorkItem) +
            sizeof(K2OSRPC_WORK_ITEM) - sizeof(K2OSRPC_MSG_REQUEST_HDR) +
            ((aByteCount + 3) & ~3)
            );
    pWorkItem->mpResp->mCallerRef = pReqHdr->mCallerRef;
    pWorkItem->mpResp->mStatus = K2STAT_ERROR_UNKNOWN;
    pWorkItem->mpResp->mResultsByteCount = 0;
    pWorkItem->mpOutBuf = ((UINT8 *)pWorkItem->mpResp) + sizeof(K2OSRPC_MSG_RESPONSE_HDR);
    switch (pReqHdr->mRequestType)
    {
    case K2OSRpc_ServerClientRequest_Create:
        pWorkItem->mWorkerMethod = K2OSRpc_WorkerMethod_Create;
        break;
    case K2OSRpc_ServerClientRequest_Call:
        pWorkItem->mWorkerMethod = K2OSRpc_WorkerMethod_Call;
        break;
    case K2OSRpc_ServerClientRequest_Release:
        pWorkItem->mWorkerMethod = K2OSRpc_WorkerMethod_Release;
        break;
    default:
        K2_ASSERT(0);
        break;
    }

    //
    // if this is a release then remove the usage from the list now so that
    // further attempts on that usage through the connection will fail
    //
    if (pWorkItem->mWorkerMethod == K2OSRpc_WorkerMethod_Release)
    {
        K2_ASSERT(NULL != pUsage);
        K2OS_CritSec_Enter(&pServerClient->UsageListSec);
        K2LIST_Remove(&pServerClient->UsageList, &pUsage->ServerClientUsageListLink);
        K2OS_CritSec_Leave(&pServerClient->UsageListSec);
        //
        // additional usage refernce already held by K2ATOMIC_Inc above
        //
        K2OSRPC_ServerUsage_Release(pUsage);
    }

    //
    // get ready to queue work item to a worker thread
    //
    K2OS_CritSec_Enter(&pServerClient->WorkItemListSec);

    K2LIST_AddAtTail(&pServerClient->WorkItemList, &pWorkItem->ServerClientWorkListLink);

    K2OS_CritSec_Leave(&pServerClient->WorkItemListSec);

    //
    // find or create a worker thread to do the work
    //
    if ((NULL != pServerObject) &&
        (NULL != pServerObject->mpWorkerThread))
    {
        //
        // work item needs to go to local object's thread
        //
        pWorkerThread = pServerObject->mpWorkerThread;
        K2_ASSERT(pWorkerThread->mpOwnedByObject == pServerObject);
    }
    else
    {
        //
        // work item can go to any worker thread
        //
        pWorkerThread = K2OSRPC_GetWorkerThread();
        K2_ASSERT(NULL == pWorkerThread->mpOwnedByObject);
    }

    //
    // queue the work item to the thread and wake it up
    //
    do {
        v = (UINT_PTR)pWorkerThread->mpWorkItemTransferList;
        pWorkItem->mpNextWorkItem = (K2OSRPC_WORK_ITEM *)v;
    } while (v != K2ATOMIC_CompareExchange((UINT_PTR volatile *)&pWorkerThread->mpWorkItemTransferList, (UINT_PTR)pWorkItem, v));

    K2OSRPC_Thread_WakeUp(&pWorkerThread->Thread);
    FUNC_EXIT;
}

void
K2OSRPC_ServerClient_OnDisconnect(
    K2OS_IPCEND aEndpoint,
    void *      apContext
)
{
    K2OSRPC_SERVER_CLIENT * pServerClient;

    FUNC_ENTER;

    pServerClient = (K2OSRPC_SERVER_CLIENT *)apContext;

    K2OS_CritSec_Enter(&sgGraphSec);
    K2TREE_Remove(&sgGlobalServerClientTree, &pServerClient->GlobalClientTreeNode);
    K2OS_CritSec_Leave(&sgGraphSec);

    pServerClient->mIsConnected = FALSE;

    FUNC_EXIT;
}

void
K2OSRPC_ServerClient_AtExit(
    K2OSRPC_THREAD *apThread
)
{
    K2OSRPC_SERVER_CLIENT * pServerClient;

    FUNC_ENTER;

    pServerClient = K2_GET_CONTAINER(K2OSRPC_SERVER_CLIENT, apThread, Thread);

    if (NULL != pServerClient->mIpcEnd)
    {
        K2OS_Ipc_DeleteEndpoint(pServerClient->mIpcEnd);
    }

    if (NULL != pServerClient->mMailbox)
    {
        K2OS_Mailbox_Delete(pServerClient->mMailbox);
    }

    K2OS_CritSec_Done(&pServerClient->WorkItemListSec);
    K2OS_CritSec_Done(&pServerClient->UsageListSec);

    K2OS_Heap_Free(pServerClient);
    FUNC_EXIT;
}

void
K2OSRPC_ServerClient_DoWork(
    K2OSRPC_THREAD *apThread
)
{
    static const K2OS_IPCEND_FUNCTAB sServerClientFuncTab = {
        K2OSRPC_ServerClient_OnConnect,
        K2OSRPC_ServerClient_OnRecv,
        K2OSRPC_ServerClient_OnDisconnect,
        NULL
    };

    UINT_PTR                        waitResult;
    K2OS_MSG                        msg;
    BOOL                            ok;
    K2OSRPC_SERVER_CLIENT *         pServerClient;
    K2LIST_LINK *                   pListLink;
    K2OSRPC_SERVER_OBJECT_USAGE *   pServerUsage;
    K2OSRPC_WORK_ITEM *             pWorkItem;

    FUNC_ENTER;

    pServerClient = K2_GET_CONTAINER(K2OSRPC_SERVER_CLIENT, apThread, Thread);

    ok = FALSE;

    do {
        pServerClient->mMailbox = K2OS_Mailbox_Create(NULL);
        if (NULL == pServerClient->mMailbox)
        {
            break;
        }

        pServerClient->mIpcEnd = K2OS_Ipc_CreateEndpoint(pServerClient->mMailbox, 32, 1024, pServerClient, &sServerClientFuncTab);
        if (NULL == pServerClient->mIpcEnd)
        {
            break;
        }

        ok = K2OS_Ipc_AcceptRequest(pServerClient->mIpcEnd, pServerClient->mRequestId);

    } while (0);

    if (!ok)
    {
        K2OS_CritSec_Enter(&sgGraphSec);
        K2TREE_Remove(&sgGlobalServerClientTree, &pServerClient->GlobalClientTreeNode);
        K2OS_CritSec_Leave(&sgGraphSec);

        K2OS_Ipc_RejectRequest(pServerClient->mRequestId, K2OS_Thread_GetLastStatus());
    }
    else
    {
        //
        // connected now. connection message should be sitting in mailbox now
        //
        do {
            waitResult = K2OS_Wait_OnMailbox(pServerClient->mMailbox, &msg, 0);
            if (waitResult != K2OS_THREAD_WAIT_MAILBOX_SIGNALLED)
            {
                break;
            }
            if (!K2OS_System_ProcessMsg(&msg))
            {
                if (msg.mControl == K2OS_SYSTEM_MSG_IPC_REQUEST)
                {
                    K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                }
            }
        } while (1);

        if (pServerClient->mIsConnected)
        {
            do {
                waitResult = K2OS_Wait_OnMailbox(pServerClient->mMailbox, &msg, K2OS_TIMEOUT_INFINITE);
                K2_ASSERT(waitResult == K2OS_THREAD_WAIT_MAILBOX_SIGNALLED);
                if (!K2OS_System_ProcessMsg(&msg))
                {
                    if (msg.mControl == K2OS_SYSTEM_MSG_IPC_REQUEST)
                    {
                        K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                    }
                }
            } while (pServerClient->mIsConnected);
        }

        //
        // disconnected.  cancel any inflight calls we can
        // 
        K2OS_CritSec_Enter(&pServerClient->WorkItemListSec);
        pListLink = pServerClient->WorkItemList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pWorkItem = K2_GET_CONTAINER(K2OSRPC_WORK_ITEM, pListLink, ServerClientWorkListLink);
                pListLink = pListLink->mpNext;
                pWorkItem->mIsCancelled = TRUE;
            } while (NULL != pListLink);
        }
        K2OS_CritSec_Leave(&pServerClient->WorkItemListSec);

        //
        // this will signal anybody who is blocking inside a class callback
        // that the caller has disconnected.  The inflight calls have all already
        // been cancelled.
        //
        K2OS_Gate_Open(pServerClient->mDisconnectedGateToken);

        //
        // wait for in-process work to be purged by worker threads by snapshotting the
        // work item list that the worker threads are servicing
        //
        do {
            waitResult = *((UINT_PTR volatile *)&pServerClient->WorkItemList.mNodeCount);
            if (0 == waitResult)
                break;
            K2OS_Thread_Sleep(200);
        } while (1);

        //
        // clean up usages that this client held
        //
        K2OS_CritSec_Enter(&pServerClient->UsageListSec);

        do {
            pListLink = pServerClient->UsageList.mpHead;
            if (NULL == pListLink)
                break;

            pServerUsage = K2_GET_CONTAINER(K2OSRPC_SERVER_OBJECT_USAGE, pListLink, ServerClientUsageListLink);
            pListLink = pListLink->mpNext;

            K2LIST_Remove(&pServerClient->UsageList, &pServerUsage->ServerClientUsageListLink);

            K2OS_CritSec_Leave(&pServerClient->UsageListSec);

            K2OSRPC_ServerUsage_Release(pServerUsage);

            K2OS_CritSec_Enter(&pServerClient->UsageListSec);

        } while (1);

        K2OS_CritSec_Leave(&pServerClient->UsageListSec);

    }

    K2OSRPC_Thread_Release(apThread);

    FUNC_EXIT;
}

void
K2OSRPC_Server_OnConnectRequest(
    UINT_PTR    aRequestorProcessId,
    UINT_PTR    aRequestId
)
{
    K2OSRPC_SERVER_CLIENT * pServerClient;
    K2TREE_NODE *           pTreeNode;
    K2STAT                  stat;

    FUNC_ENTER;

    pServerClient = (K2OSRPC_SERVER_CLIENT *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER_CLIENT));
    if (NULL == pServerClient)
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        K2MEM_Zero(pServerClient, sizeof(K2OSRPC_SERVER_CLIENT));

        pServerClient->mDisconnectedGateToken = K2OS_Gate_Create(FALSE);
        if (NULL == pServerClient->mDisconnectedGateToken)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
        else
        {
            K2LIST_Init(&pServerClient->UsageList);
            K2LIST_Init(&pServerClient->WorkItemList);

            stat = K2STAT_NO_ERROR;

            do {
                K2OS_CritSec_Enter(&sgGraphSec);

                pTreeNode = K2TREE_Find(&sgGlobalServerClientTree, aRequestorProcessId);
                if (NULL == pTreeNode)
                {
                    pServerClient->GlobalClientTreeNode.mUserVal = aRequestorProcessId;
                    K2TREE_Insert(&sgGlobalServerClientTree, aRequestorProcessId, &pServerClient->GlobalClientTreeNode);
                }

                K2OS_CritSec_Leave(&sgGraphSec);

                if (NULL != pTreeNode)
                {
                    K2OSRPC_DebugPrintf("*** Process %d already connected\n", aRequestorProcessId);
                    stat = K2STAT_ERROR_ALREADY_EXISTS;
                    break;
                }

                pServerClient->mRequestId = aRequestId;
                if (!K2OS_CritSec_Init(&pServerClient->WorkItemListSec))
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                }
                else
                {
                    if (!K2OS_CritSec_Init(&pServerClient->UsageListSec))
                    {
                        K2OS_CritSec_Done(&pServerClient->WorkItemListSec);
                        stat = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(stat));
                    }
                    else
                    {
                        stat = K2OSRPC_Thread_Create(&pServerClient->Thread,
                            K2OSRPC_ServerClient_AtExit,
                            K2OSRPC_ServerClient_DoWork
                        );
                        if (K2STAT_IS_ERROR(stat))
                        {
                            K2OS_CritSec_Done(&pServerClient->UsageListSec);
                            K2OS_CritSec_Done(&pServerClient->WorkItemListSec);
                        }
                    }
                }

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSRPC_DebugPrintf("*** Could not create thread for process %d as RPC client\n", aRequestorProcessId);

                    K2OS_CritSec_Enter(&sgGraphSec);
                    K2TREE_Remove(&sgGlobalServerClientTree, &pServerClient->GlobalClientTreeNode);
                    K2OS_CritSec_Leave(&sgGraphSec);

                    break;
                }

                K2OSRPC_Thread_WakeUp(&pServerClient->Thread);

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Token_Destroy(pServerClient->mDisconnectedGateToken);
            }
        }

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Heap_Free(pServerClient);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Ipc_RejectRequest(aRequestId, K2STAT_ERROR_CONNECTED);
    }

    FUNC_EXIT;
}

//
// ------------------------------------------------------------------------
//

void
K2OSRPC_Server_ShutDown(
    K2OSRPC_SERVER * apServer
)
{
    FUNC_ENTER;
    if (0 == K2ATOMIC_CompareExchange((UINT_PTR volatile *)&apServer->mShutDown, 1, 0))
    {
        K2OS_Notify_Signal(apServer->mStopNotifyToken);
        K2OSRPC_Thread_Release(&apServer->Thread);
    }
    FUNC_EXIT;
}

void
K2OSRPC_ServerClass_Release(
    K2OSRPC_SERVER_CLASS *  apServerClass
)
{
    K2OSRPC_SERVER * pShutServer;

    FUNC_ENTER;
    if (0 != K2ATOMIC_Dec(&apServerClass->mRefCount))
    {
        FUNC_EXIT;
        return;
    }

    K2_ASSERT(FALSE == apServerClass->mIsRegistered);
    K2_ASSERT(0 == apServerClass->ObjectList.mNodeCount);

    K2OS_Xdl_Release(apServerClass->mXdlRefs[2]);
    K2OS_Xdl_Release(apServerClass->mXdlRefs[1]);
    K2OS_Xdl_Release(apServerClass->mXdlRefs[0]);

    K2OS_Heap_Free(apServerClass);

    if (0 == K2ATOMIC_Dec(&sgGlobalServerClassCount))
    {
        //
        // no classes left - shut down the current server
        //
        pShutServer = sgpServer;
        K2ATOMIC_Exchange((UINT_PTR volatile *)&sgpServer, 0);

        //
        // shut down pShutServer
        //
        K2OSRPC_Server_ShutDown(pShutServer);
    }
    FUNC_EXIT;
}

void 
K2OSRPC_Server_AtExit(
    K2OSRPC_THREAD *apThread
)
{
    K2OSRPC_SERVER * pServer;
    
    FUNC_ENTER;
    
    pServer = K2_GET_CONTAINER(K2OSRPC_SERVER, apThread, Thread);

    K2OS_Token_Destroy(pServer->mActiveGateToken);
    K2OS_Token_Destroy(pServer->mStopNotifyToken);
    K2OS_Heap_Free(pServer);

    FUNC_EXIT;
}

void 
K2OSRPC_Server_DoWork(
    K2OSRPC_THREAD *apThread
)
{
    static K2_GUID128   sServerGuid = K2OS_IFACE_ID_RPC_SERVER;
    K2OSRPC_SERVER *    pServer;
    K2OS_MAILBOX        mailbox;
    UINT_PTR            ifInstanceId;
    UINT_PTR            waitResult;
    K2OS_MSG            msg;

    FUNC_ENTER;
    
    pServer = K2_GET_CONTAINER(K2OSRPC_SERVER, apThread, Thread);

    mailbox = K2OS_Mailbox_Create(NULL);
    if (NULL == mailbox)
    {
        K2OSRPC_DebugPrintf("***Server mailbox create failed\n");
        FUNC_EXIT;
        return;
    }

    do {
        ifInstanceId = K2OS_IfInstance_Create(mailbox, 0, NULL);
        if (0 == ifInstanceId)
        {
            K2OSRPC_DebugPrintf("***Server interface instance create failed\n");
            break;
        }
        if (!K2OS_IfInstance_Publish(ifInstanceId, K2OS_IFCLASS_RPC, &sServerGuid))
        {
            K2OSRPC_DebugPrintf("***Server interface instance publish failed\n");
            break;
        }

        //
        // server is up now
        //
        K2OS_Gate_Open(pServer->mActiveGateToken);

        do {
            waitResult = K2OS_Wait_OnMailboxAndOne(mailbox, &msg, pServer->mStopNotifyToken, K2OS_TIMEOUT_INFINITE);
            if (waitResult != K2OS_THREAD_WAIT_MAILBOX_SIGNALLED)
            {
                // stop notify token signalled.
                K2OSRPC_DebugPrintf("Server stopping\n");
                break;
            }

            //
            // mailbox received message
            //
            if (msg.mControl == K2OS_SYSTEM_MSG_IPC_REQUEST)
            {
                //
                // somebody is requesting a connection
                // 
                // msg.mPayload[0] is interface context (NULL in this case, from K2OS_IfInstance_Create above)
                // msg.mPayload[1] is requestor process id
                // msg.mPayload[2] is global request id
                K2OSRPC_Server_OnConnectRequest(msg.mPayload[1], msg.mPayload[2]);
            }
            else
            {
                K2OSRPC_DebugPrintf("***Server received non-connect message on rpc interface\n");
            }

        } while (1);

        K2OS_Gate_Close(pServer->mActiveGateToken);
        
        K2OS_IfInstance_Remove(ifInstanceId);

    } while (0);

    K2OS_Mailbox_Delete(mailbox);

    FUNC_EXIT;
}

K2OSRPC_SERVER *
K2OSRPC_Server_Start(
    void
)
{
    K2OSRPC_SERVER *    pServer;
    K2STAT              stat;

    FUNC_ENTER;
    
    pServer = (K2OSRPC_SERVER *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER));
    if (NULL == pServer)
    {
        FUNC_EXIT;
        return NULL;
    }

    K2MEM_Zero(pServer, sizeof(K2OSRPC_SERVER));

    pServer->mStopNotifyToken = K2OS_Notify_Create(FALSE);
    if (NULL == pServer->mStopNotifyToken)
    {
        K2OS_Heap_Free(pServer);
        FUNC_EXIT;
        return NULL;
    }

    pServer->mActiveGateToken = K2OS_Gate_Create(FALSE);
    if (NULL == pServer->mActiveGateToken)
    {
        K2OS_Token_Destroy(pServer->mStopNotifyToken);
        K2OS_Heap_Free(pServer);
        FUNC_EXIT;
        return NULL;
    }

    stat = K2OSRPC_Thread_Create(&pServer->Thread, K2OSRPC_Server_AtExit, K2OSRPC_Server_DoWork);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Token_Destroy(pServer->mActiveGateToken);
        K2OS_Token_Destroy(pServer->mStopNotifyToken);
        K2OS_Heap_Free(pServer);
        FUNC_EXIT;
        return NULL;
    }

    K2OSRPC_Thread_WakeUp(&pServer->Thread);

    K2OS_Wait_One(pServer->mActiveGateToken, K2OS_TIMEOUT_INFINITE);

    FUNC_EXIT;
    return pServer;
}

BOOL
K2OSRPC_ServerObject_CreateInternal(
    K2_GUID128 const *      apClassId,
    UINT_PTR                aCreatorContext,
    UINT_PTR *              apRetServerObjectId,
    UINT_PTR *              apRetUsageId,
    K2OSRPC_SERVER_CLIENT * apServerClient
)
{
    K2STAT                                  stat;
    K2OSRPC_SERVER_CLASS *                  pServerClass;
    K2TREE_NODE *                           pTreeNode;
    K2OSRPC_SERVER_OBJECT_USAGE *           pUsage;
    K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE  resp;
    K2OSRPC_WORK_ITEM *                     pWorkItem;
    K2OSRPC_WORKER_THREAD *                 pWorkerThread;
    K2OS_NOTIFY_TOKEN                       doneNotifyToken;
    UINT_PTR                                v;
    K2OSRPC_MSG_CREATE_REQUEST_DATA         request;

    FUNC_ENTER;
    
    if ((NULL == apClassId) ||
        (NULL == apRetUsageId))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    *apRetUsageId = 0;

    if (NULL != apRetServerObjectId)
    {
        *apRetServerObjectId = 0;
    }

    K2OS_CritSec_Enter(&sgGraphSec);

    pTreeNode = K2TREE_Find(&sgGlobalServerClassTree, (UINT32)apClassId);
    if (NULL != pTreeNode)
    {
        pServerClass = K2_GET_CONTAINER(K2OSRPC_SERVER_CLASS, pTreeNode, GlobalServerClassTreeNode);
        K2_ASSERT(pServerClass->mIsRegistered);
        K2ATOMIC_Inc(&pServerClass->mRefCount);
    }
    else
    {
        pServerClass = NULL;
    }

    K2OS_CritSec_Leave(&sgGraphSec);

    if (NULL == pServerClass)
    {
        FUNC_EXIT;
        return K2STAT_ERROR_CLASS_NOT_FOUND;
    }

    K2MEM_Copy(&request.mClassId, apClassId, sizeof(K2_GUID128));
    request.mCreatorContext = aCreatorContext;

    if (pServerClass->ObjectClass.mSingleThreadPerObject)
    {
        pWorkItem = (K2OSRPC_WORK_ITEM *)K2OS_Heap_Alloc(sizeof(K2OSRPC_WORK_ITEM));
        if (NULL == pWorkItem)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            K2MEM_Zero(pWorkItem, sizeof(K2OSRPC_WORK_ITEM));
            pWorkItem->mpInBuf = (UINT8 const *)&request;
            K2MEM_Zero(&resp, sizeof(resp));
            resp.ResponseHdr.mStatus = K2STAT_ERROR_UNKNOWN;
            pWorkItem->mpResp = &resp.ResponseHdr;
            pWorkItem->mpOutBuf = (UINT8 *)&resp.Data;
            pWorkItem->RequestHdr.mInByteCount = sizeof(request);
            pWorkItem->RequestHdr.mOutBufSizeProvided = sizeof(resp.Data);
            pWorkItem->RequestHdr.mRequestType = K2OSRpc_ServerClientRequest_Create;
            pWorkItem->mWorkerMethod = K2OSRpc_WorkerMethod_Create;
            doneNotifyToken = pWorkItem->mDoneNotifyToken = K2OS_Notify_Create(FALSE);
            if (NULL == doneNotifyToken)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OS_Heap_Free(pWorkItem);
            }
            else
            {
                pWorkerThread = K2OSRPC_GetWorkerThread();
                if (NULL == pWorkerThread)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    K2OS_Heap_Free(pWorkItem);
                }
                else
                {
                    do {
                        v = (UINT_PTR)pWorkerThread->mpWorkItemTransferList;
                        pWorkItem->mpNextWorkItem = (K2OSRPC_WORK_ITEM *)v;
                    } while (v != K2ATOMIC_CompareExchange((UINT_PTR volatile *)&pWorkerThread->mpWorkItemTransferList, (UINT_PTR)pWorkItem, v));

                    K2OSRPC_Thread_WakeUp(&pWorkerThread->Thread);

                    K2OS_Wait_One(doneNotifyToken, K2OS_TIMEOUT_INFINITE);

                    stat = resp.ResponseHdr.mStatus;
                }

                K2OS_Token_Destroy(doneNotifyToken);

                if (!K2STAT_IS_ERROR(stat))
                {
                    *apRetUsageId = resp.Data.mServerUsageId;
                    if (NULL != apRetServerObjectId)
                    {
                        *apRetServerObjectId = resp.Data.mServerObjectId;
                    }
                }
            }
        }
    }
    else
    {
        stat = K2OSRPC_ServerObject_CreateOnThread(
            apClassId,
            K2OS_Process_GetId(),
            aCreatorContext,
            &pUsage,
            apServerClient
        );
        if (!K2STAT_IS_ERROR(stat))
        {
            *apRetUsageId = pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal;
            if (NULL != apRetServerObjectId)
            {
                *apRetServerObjectId = pUsage->mpServerObject->GlobalServerObjectTreeNode.mUserVal;
            }
        }
    }

    K2OSRPC_ServerClass_Release(pServerClass);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        FUNC_EXIT;
        return FALSE;
    }

    FUNC_EXIT;
    return TRUE;
}

BOOL
K2OSRPC_ServerObject_Create(
    K2_GUID128 const *  apClassId,
    UINT_PTR            aCreatorContext,
    UINT_PTR *          apRetServerObjectId,
    UINT_PTR *          apRetUsageId
)
{
    BOOL result;

    FUNC_ENTER;
    result = K2OSRPC_ServerObject_CreateInternal(apClassId, aCreatorContext, apRetServerObjectId, apRetUsageId, NULL);
    FUNC_EXIT;

    return result;
}

BOOL
K2OSRPC_ServerObject_AcquireUsage(
    UINT_PTR                        aServerObjectId,
    K2OSRPC_SERVER_OBJECT_USAGE **  apRetUsage
)
{
    K2TREE_NODE *                   pTreeNode;
    K2OSRPC_SERVER_OBJECT *         pServerObject;
    K2OSRPC_SERVER_OBJECT_USAGE *   pUsage;

    FUNC_ENTER;

    if ((0 == aServerObjectId) ||
        (NULL == apRetUsage))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    *apRetUsage = NULL;

    pUsage = (K2OSRPC_SERVER_OBJECT_USAGE *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER_OBJECT_USAGE));
    if (NULL == pUsage)
    {
        FUNC_EXIT;
        return K2OS_Thread_GetLastStatus();
    }

    K2MEM_Zero(pUsage, sizeof(K2OSRPC_SERVER_OBJECT_USAGE));
    pUsage->UsageHdr.mIsServer = TRUE;
    pUsage->UsageHdr.mRefCount = 1;

    K2OS_CritSec_Enter(&gK2OSRPC_GlobalUsageTreeSec);
    K2OS_CritSec_Enter(&sgGraphSec);

    pTreeNode = K2TREE_Find(&sgGlobalServerObjectTree, aServerObjectId);
    if (NULL != pTreeNode)
    {
        pServerObject = K2_GET_CONTAINER(K2OSRPC_SERVER_OBJECT, pTreeNode, GlobalServerObjectTreeNode);
        if (pServerObject->UsageList.mNodeCount > 0)
        {
            pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal = K2ATOMIC_AddExchange(&gK2OSRPC_NextGlobalUsageId, 1);

            K2LIST_AddAtTail(&pServerObject->UsageList, &pUsage->ObjectUsageListLink);

            K2TREE_Insert(&gK2OSRPC_GlobalUsageTree, pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal, &pUsage->UsageHdr.GlobalUsageTreeNode);
        }
        else
        {
            pServerObject = NULL;
        }
    }
    else
    {
        pServerObject = NULL;
    }

    K2OS_CritSec_Leave(&sgGraphSec);
    K2OS_CritSec_Leave(&gK2OSRPC_GlobalUsageTreeSec);

    if (NULL == pServerObject)
    {
        K2OS_Heap_Free(pUsage);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OBJECT_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    *apRetUsage = pUsage;

    FUNC_EXIT;
    return TRUE;
}

BOOL
K2OSRPC_ServerObject_Acquire(
    UINT_PTR    aServerObjectId,
    UINT_PTR *  apRetUsageId
)
{
    K2OSRPC_SERVER_OBJECT_USAGE * pUsage;

    FUNC_ENTER;

    if (!K2OSRPC_ServerObject_AcquireUsage(aServerObjectId, &pUsage))
    {
        FUNC_EXIT;
        return FALSE;
    }

    *apRetUsageId = pUsage->UsageHdr.GlobalUsageTreeNode.mUserVal;

    FUNC_EXIT;
    return TRUE;
}

K2STAT
K2OSRPC_ServerUsage_CallInternal(
    K2OSRPC_SERVER_OBJECT_USAGE *   apServerUsage,
    K2OSRPC_CALLARGS const *        apArgs,
    UINT_PTR *                      apRetActualOut,
    K2OSRPC_SERVER_CLIENT *         apServerClient
)
{
    K2OSRPC_SERVER_OBJECT *     pServerObject;
    K2OSRPC_SERVER_CLASS *      pServerClass;
    K2STAT                      stat;
    K2_EXCEPTION_TRAP           trap;
    K2OSRPC_MSG_RESPONSE_HDR    respHdr;
    K2OSRPC_WORK_ITEM *         pWorkItem;
    K2OS_NOTIFY_TOKEN           doneNotifyToken;
    UINT_PTR                    v;
    K2OSRPC_WORKER_THREAD *     pWorkerThread;
    K2OSRPC_OBJECT_CALL         call;

    FUNC_ENTER;

    pServerObject = apServerUsage->mpServerObject;
    pServerClass = pServerObject->mpServerClass;

    pWorkerThread = pServerObject->mpWorkerThread;
    if ((NULL != pWorkerThread) &&
        (pWorkerThread->Thread.mThreadId != K2OS_Thread_GetId()))
    {
        K2_ASSERT(NULL != pServerObject->mpWorkerThread);

        pWorkItem = K2OS_Heap_Alloc(sizeof(K2OSRPC_WORK_ITEM));
        if (NULL == pWorkItem)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            K2MEM_Zero(pWorkItem, sizeof(K2OSRPC_WORK_ITEM));
            K2MEM_Zero(&respHdr, sizeof(respHdr));
            doneNotifyToken = pWorkItem->mDoneNotifyToken = K2OS_Notify_Create(FALSE);
            if (NULL == doneNotifyToken)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OS_Heap_Free(pWorkItem);
            }
            else
            {
                pWorkItem->mpUsage = apServerUsage;
                pWorkItem->RequestHdr.mTargetMethodId = apArgs->mMethodId;
                pWorkItem->mpInBuf = apArgs->mpInBuf;
                pWorkItem->RequestHdr.mInByteCount = apArgs->mInBufByteCount;
                pWorkItem->mpOutBuf = apArgs->mpOutBuf;
                pWorkItem->RequestHdr.mOutBufSizeProvided = apArgs->mOutBufByteCount;
                pWorkItem->mpResp = &respHdr;
                pWorkItem->mWorkerMethod = K2OSRpc_WorkerMethod_Call;

                do {
                    v = (UINT_PTR)pWorkerThread->mpWorkItemTransferList;
                    pWorkItem->mpNextWorkItem = (K2OSRPC_WORK_ITEM *)v;
                } while (v != K2ATOMIC_CompareExchange((UINT_PTR volatile *)&pWorkerThread->mpWorkItemTransferList, (UINT_PTR)pWorkItem, v));

                K2OSRPC_Thread_WakeUp(&pWorkerThread->Thread);

                K2OS_Wait_One(doneNotifyToken, K2OS_TIMEOUT_INFINITE);

                K2OS_Token_Destroy(doneNotifyToken);

                stat = respHdr.mStatus;
                if (!K2STAT_IS_ERROR(stat))
                {
                    *apRetActualOut = respHdr.mResultsByteCount;
                }
                else
                {
                    *apRetActualOut = 0;
                }
            }
        }
    }
    else
    {
        //
        // call proceeds on this thread
        //
        call.mCreatorRef = pServerObject->mClientRef;
        K2MEM_Copy(&call, apArgs, sizeof(K2OSRPC_CALLARGS));
        call.mRemoteDisconnectedGateToken = (NULL == apServerClient) ? sgAlwaysShutGateToken : apServerClient->mDisconnectedGateToken;

#if TRAP_EXCEPTIONS
        stat = K2_EXTRAP(&trap, pServerClass->ObjectClass.Call(&call, apRetActualOut));
#else
        stat = pServerClass->ObjectClass.Call(&call, apRetActualOut);
#endif
    }

    FUNC_EXIT;

    return stat;
}

K2STAT
K2OSRPC_ServerUsage_Call(
    K2OSRPC_SERVER_OBJECT_USAGE *   apServerUsage,
    K2OSRPC_CALLARGS const *        apArgs,
    UINT_PTR *                      apRetActualOut
)
{
    K2STAT stat;
    FUNC_ENTER;
    stat = K2OSRPC_ServerUsage_CallInternal(apServerUsage, apArgs, apRetActualOut, NULL);
    FUNC_EXIT;
    return stat;
}

void
K2OSRPC_ServerUsage_ReleaseInternal(
    K2OSRPC_SERVER_OBJECT_USAGE *   apServerUsage,
    K2OSRPC_SERVER_CLIENT *         apServerClient
    )
{
    K2OSRPC_SERVER_OBJECT * pServerObject;
    K2OSRPC_WORK_ITEM *     pWorkItem;
    K2OSRPC_WORKER_THREAD * pWorkerThread;
    K2OS_NOTIFY_TOKEN       doneNotifyToken;
    UINT_PTR                v;

    FUNC_ENTER;

    //
    // cs ordering - reference tree before object tree
    //
    K2OS_CritSec_Enter(&gK2OSRPC_GlobalUsageTreeSec);

    if (0 != K2ATOMIC_Dec(&apServerUsage->UsageHdr.mRefCount))
    {
        apServerUsage = NULL;
    }
    else
    {
        K2OS_CritSec_Enter(&sgGraphSec);

        pServerObject = apServerUsage->mpServerObject;

        K2TREE_Remove(&gK2OSRPC_GlobalUsageTree, &apServerUsage->UsageHdr.GlobalUsageTreeNode);

        K2LIST_Remove(&pServerObject->UsageList, &apServerUsage->ObjectUsageListLink);

        if (pServerObject->UsageList.mNodeCount != 0)
        {
            //
            // object is not going to be deleted as other usage still exist for it
            //
            pServerObject = NULL;
        }

        K2OS_CritSec_Leave(&sgGraphSec);
    }

    K2OS_CritSec_Leave(&gK2OSRPC_GlobalUsageTreeSec);

    if (NULL == apServerUsage)
    {
        FUNC_EXIT;
        return;
    }

    //
    // usage is being deleted
    //

    if (NULL != pServerObject)
    {
        //
        // object is being deleted (last usage was just removed)
        //
        pWorkerThread = pServerObject->mpWorkerThread;
        if ((NULL != pWorkerThread) &&
            (pWorkerThread->Thread.mThreadId != K2OS_Thread_GetId()))
        {
            //
            // must do delete on object's thread that is not this thread
            //
            pWorkItem = (K2OSRPC_WORK_ITEM *)K2OS_Heap_Alloc(sizeof(K2OSRPC_WORK_ITEM));
            if (NULL != pWorkItem)
            {
                K2MEM_Zero(pWorkItem, sizeof(K2OSRPC_WORK_ITEM));
                pWorkItem->mWorkerMethod = K2OSRpc_WorkerMethod_Delete;
                pWorkItem->mpUsage = (K2OSRPC_SERVER_OBJECT_USAGE *)pServerObject;
                doneNotifyToken = pWorkItem->mDoneNotifyToken = K2OS_Notify_Create(FALSE);
                if (NULL == doneNotifyToken)
                {
                    K2OS_Heap_Free(pWorkItem);
                }
                else
                {
                    do {
                        v = (UINT_PTR)pWorkerThread->mpWorkItemTransferList;
                        pWorkItem->mpNextWorkItem = (K2OSRPC_WORK_ITEM *)v;
                    } while (v != K2ATOMIC_CompareExchange((UINT_PTR volatile *)&pWorkerThread->mpWorkItemTransferList, (UINT_PTR)pWorkItem, v));

                    K2OSRPC_Thread_WakeUp(&pWorkerThread->Thread);

                    K2OS_Wait_One(doneNotifyToken, K2OS_TIMEOUT_INFINITE);

                    K2OS_Token_Destroy(doneNotifyToken);
                }
            }
        }
        else
        {
            //
            // run delete on this thread
            //
            K2OSRPC_ServerObject_DeleteOnThread(pServerObject, apServerClient);
        }
    }

    K2OS_Heap_Free(apServerUsage);

    FUNC_EXIT;
}

void
K2OSRPC_ServerUsage_Release(
    K2OSRPC_SERVER_OBJECT_USAGE * apServerUsage
)
{
    FUNC_ENTER;
    K2OSRPC_ServerUsage_ReleaseInternal(apServerUsage, NULL);
    FUNC_EXIT;
}

BOOL
K2OSRPC_ObjectClass_Publish(
    K2OSRPC_OBJECT_CLASS const *    apClass,
    UINT_PTR                        aPublishContext
)
{
    K2STAT                  stat;
    K2_GUID128 const *      pClassId;
    K2OSRPC_SERVER_CLASS *  pServerClass;
    K2TREE_NODE *           pTreeNode;
    K2OSRPC_SERVER *        pNewServer;
    UINT_PTR                chk;

    FUNC_ENTER;
    
    if (NULL == apClass)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    pServerClass = (K2OSRPC_SERVER_CLASS *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER_CLASS));
    if (NULL == pServerClass)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_MEMORY);
        FUNC_EXIT;
        return FALSE;
    }

    stat = K2STAT_NO_ERROR;

    do {
        K2MEM_Zero(pServerClass, sizeof(K2OSRPC_SERVER_CLASS));

        K2MEM_Copy(&pServerClass->ObjectClass, apClass, sizeof(K2OSRPC_OBJECT_CLASS));

        pClassId = &pServerClass->ObjectClass.ClassId;
        if ((0 == *(((UINT32 const *)pClassId) + 0)) &&
            (0 == *(((UINT32 const *)pClassId) + 1)) &&
            (0 == *(((UINT32 const *)pClassId) + 2)) &&
            (0 == *(((UINT32 const *)pClassId) + 3)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        pServerClass->mXdlRefs[0] = K2OS_Xdl_AddRefContaining((UINT_PTR)pServerClass->ObjectClass.Create);
        if (NULL == pServerClass->mXdlRefs[0])
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        do {
            pServerClass->mXdlRefs[1] = K2OS_Xdl_AddRefContaining((UINT_PTR)pServerClass->ObjectClass.Call);
            if (NULL == pServerClass->mXdlRefs[1])
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                break;
            }

            do {
                pServerClass->mXdlRefs[2] = K2OS_Xdl_AddRefContaining((UINT_PTR)pServerClass->ObjectClass.Delete);
                if (NULL == pServerClass->mXdlRefs[2])
                {
                    stat = K2STAT_ERROR_BAD_ARGUMENT;
                    break;
                }

                K2LIST_Init(&pServerClass->ObjectList);
                pServerClass->mIsRegistered = TRUE;
                pServerClass->mPublishContext = aPublishContext;
                pServerClass->mRefCount = 1;

                K2OS_CritSec_Enter(&sgGraphSec);

                pTreeNode = K2TREE_Find(&sgGlobalServerClassTree, (UINT32)&pServerClass->ObjectClass.ClassId);
                if (NULL != pTreeNode)
                {
                    stat = K2STAT_ERROR_ALREADY_EXISTS;
                }
                else
                {
                    K2TREE_Insert(&sgGlobalServerClassTree, (UINT32)&pServerClass->ObjectClass.ClassId, &pServerClass->GlobalServerClassTreeNode);
                }

                K2OS_CritSec_Leave(&sgGraphSec);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_Xdl_Release(pServerClass->mXdlRefs[2]);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Xdl_Release(pServerClass->mXdlRefs[1]);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Xdl_Release(pServerClass->mXdlRefs[0]);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pServerClass);

        K2OS_Thread_SetLastStatus(stat);

        FUNC_EXIT;

        return FALSE;
    }

    if (0 == K2ATOMIC_AddExchange(&sgGlobalServerClassCount, 1))
    {
        //
        // first registered class.  if there is an old server it should be
        // exiting or have already exited.  wait for sgpServer to be NULL
        //
        while (0 != K2ATOMIC_CompareExchange((UINT_PTR *)&sgpServer, (UINT_PTR)-1, 0))
        {
            K2OS_Thread_Sleep(100);
        }

        //
        // we can start the ipc server now that the old one is gone
        //
        pNewServer = K2OSRPC_Server_Start();
        K2_ASSERT(NULL != pNewServer);      // unrecoverable

        //
        // update the pointer to the server now that it is actually running
        //
        K2ATOMIC_Exchange((UINT_PTR *)&sgpServer, (UINT_PTR)pNewServer);
    }
    else
    {
        //
        // server may be coming up. wait for server pointer to not be 0 and not be -1 
        //
        do {
            chk = *((volatile UINT_PTR *)&sgpServer);
            K2_CpuReadBarrier();
        } while ((chk == 0) || (chk == (UINT_PTR)-1));
    }

    FUNC_EXIT;
    return TRUE;
}

BOOL
K2OSRPC_ObjectClass_Unpublish(
    K2_GUID128 const *  apClassId
)
{
    K2STAT                  stat;
    K2OSRPC_SERVER_CLASS *   pServerClass;
    K2TREE_NODE *           pTreeNode;

    FUNC_ENTER;
    
    if (NULL == apClassId)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    stat = K2STAT_NO_ERROR;

    K2OS_CritSec_Enter(&sgGraphSec);

    pTreeNode = K2TREE_Find(&sgGlobalServerClassTree, (UINT32)apClassId);
    if (NULL != pTreeNode)
    {
        pServerClass = K2_GET_CONTAINER(K2OSRPC_SERVER_CLASS, pTreeNode, GlobalServerClassTreeNode);
        if (!pServerClass->mIsRegistered)
        {
            stat = K2STAT_ERROR_CLASS_NOT_FOUND;
        }
        else
        {
            K2TREE_Remove(&sgGlobalServerClassTree, &pServerClass->GlobalServerClassTreeNode);
            pServerClass->mIsRegistered = FALSE;
        }
    }

    K2OS_CritSec_Leave(&sgGraphSec);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        FUNC_EXIT;
        return FALSE;
    }

    K2OSRPC_ServerClass_Release(pServerClass);

    FUNC_EXIT;
    return TRUE;
}

int
K2OSRPC_ServerClass_Compare(
    UINT32          aKey,
    K2TREE_NODE *   apNode
)
{
    K2OSRPC_SERVER_CLASS *pServerClass;
    pServerClass = K2_GET_CONTAINER(K2OSRPC_SERVER_CLASS, apNode, GlobalServerClassTreeNode);
    return K2MEM_Compare((K2_GUID128 const *)aKey, &pServerClass->ObjectClass.ClassId, sizeof(K2_GUID128));
}

K2STAT 
K2OSRPC_Server_AtXdlEntry(
    UINT32 aReason
)
{
    if (XDL_ENTRY_REASON_LOAD == aReason)
    {
        sgAlwaysShutGateToken = K2OS_Gate_Create(FALSE);
        if (NULL == sgAlwaysShutGateToken)
        {
            return K2OS_Thread_GetLastStatus();
        }

        if (!K2OS_CritSec_Init(&sgGraphSec))
        {
            K2OS_Token_Destroy(sgAlwaysShutGateToken);
            return K2OS_Thread_GetLastStatus();
        }
        if (!K2OS_CritSec_Init(&sgThreadListSec))
        {
            K2OS_CritSec_Done(&sgGraphSec);
            K2OS_Token_Destroy(sgAlwaysShutGateToken);
            return K2OS_Thread_GetLastStatus();
        }

        K2TREE_Init(&sgGlobalServerClassTree, K2OSRPC_ServerClass_Compare);
        K2TREE_Init(&sgGlobalServerObjectTree, NULL);
        K2TREE_Init(&sgGlobalServerClientTree, NULL);

        sgpServer = NULL;
        sgNextServerObjectId = 1;
        sgGlobalServerClassCount = 0;
        K2LIST_Init(&sgIdleThreadList);
        K2LIST_Init(&sgActiveThreadList);
        K2_CpuWriteBarrier();
    }
    else if (XDL_ENTRY_REASON_UNLOAD == aReason)
    {
        K2_ASSERT(0 == sgActiveThreadList.mNodeCount);
        K2_ASSERT(0 == sgIdleThreadList.mNodeCount);
        K2OS_CritSec_Done(&sgThreadListSec);
        K2OS_CritSec_Done(&sgGraphSec);
        K2OS_Token_Destroy(sgAlwaysShutGateToken);
    }

    return K2STAT_NO_ERROR;
}
