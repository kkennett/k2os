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
#ifndef __ROOT_H
#define __ROOT_H

#include "../crt/crtroot.h"
#include <k2osdrv.h>
#include <k2osrpc.h>
#include "../k2osacpi/k2osacpi.h"

#if __cplusplus
extern "C" {
#endif

#define REF_DEBUG 0

//
// -------------------------------------------------------------------------
// 

typedef struct _ROOTDEV_NODE            ROOTDEV_NODE;
typedef struct _ROOTDEV_NODEREF         ROOTDEV_NODEREF;
typedef struct _ROOTDEV_EVENT           ROOTDEV_EVENT;
typedef struct _ROOTACPI_NODEINFO       ROOTACPI_NODEINFO;
typedef enum   _RootDevNodeStateType    RootDevNodeStateType;
typedef struct _ROOTDEV_RES             ROOTDEV_RES;
typedef struct _ROOTDEV_IO              ROOTDEV_IO;
typedef struct _ROOTDEV_IRQ             ROOTDEV_IRQ;
typedef struct _ROOTDEV_PHYS            ROOTDEV_PHYS;
typedef enum   _RootEventType           RootEventType;
typedef struct _ROOTDEV_TIMER           ROOTDEV_TIMER;
typedef enum   _RootDriverEventType     RootDriverEventType;

typedef void (*RootDevNode_pf_OnEvent)(ROOTDEV_NODE *apNode, ROOTDEV_EVENT *apEvent);
typedef void (*RootEvent_pf_Release)(ROOTDEV_EVENT *apEvent);
typedef void (*RootTimer_pf_ExpiryCallback)(ROOTDEV_TIMER *apTimer, ROOTDEV_NODE *apDevNode, UINT_PTR aContextVal);

extern UINT_PTR            gMainThreadId;
extern K2OS_MAILBOX        gMainMailbox;
extern K2OS_MAILSLOT_TOKEN gMainMailslotToken;

//
// -------------------------------------------------------------------------
// 

UINT_PTR RootDebug_Printf(char const *apFormat, ...);

//
// -------------------------------------------------------------------------
// 

void RootRes_Init(void);
BOOL RootRes_IoHeap_Reserve(K2OS_IOADDR_INFO const *apIoInfo, UINT_PTR aContext, BOOL aExclusive);
BOOL RootRes_PhysHeap_Reserve(K2OS_PHYSADDR_INFO const *apPhysInfo, UINT_PTR aContext, BOOL aExclusive);

//
// -------------------------------------------------------------------------
// 

struct _ROOTACPI_NODEINFO
{
    char            mName[8];       // 4 chars. [4] is zero
    char const *    mpHardwareId;
};

void RootAcpi_Init(void);
void RootAcpi_Enable(void);

//
// -------------------------------------------------------------------------
// 
#define ROOTPLAT_MOUNT_MAX_BYTES    1024

enum _RootDevNodeStateType
{
    RootDevNodeState_Invalid = 0,

    RootDevNodeState_NoDriver,
    RootDevNodeState_DriverSpec,
    RootDevNodeState_DriverStarted,
    RootDevNodeState_ObjectCreated,
    RootDevNodeState_Running,
    RootDevNodeState_DriverStopping,
    RootDevNodeState_DriverStopped,

    RootDevNodeStateType_Count
};

struct _ROOTDEV_RES
{
    UINT_PTR        mAcpiContext;
    K2LIST_LINK     ListLink;
};

struct _ROOTDEV_IO
{
    ROOTDEV_RES         Res;
    K2OSDRV_RES_IO      DrvResIo;
};

struct _ROOTDEV_IRQ
{
    ROOTDEV_RES         Res;
    K2OSDRV_RES_IRQ     DrvResIrq;
};

struct _ROOTDEV_PHYS
{
    ROOTDEV_RES         Res;
    K2OSDRV_RES_PHYS    DrvResPhys;
};

struct _ROOTDEV_NODEREF
{
    ROOTDEV_NODE *  mpRefNode;
    K2LIST_LINK     ListLink;
#if REF_DEBUG
    char const *    mpFile;
    int             mLine;
#endif
};

struct _ROOTDEV_TIMER
{
    RootTimer_pf_ExpiryCallback mfCallback;
    UINT_PTR                    mMsTimeoutRemaining;
    K2LIST_LINK                 ListLink;
    BOOL                        mIsActive;
    struct {
        ROOTDEV_NODEREF         RefDevNode;
        UINT_PTR                mUserVal;
    } Context;
};

struct _ROOTDEV_NODE
{
    K2LIST_ANCHOR           RefList;                // leave as first thing in struct

    ROOTDEV_NODEREF         RefParent;              // set before added to tree
    K2LIST_LINK             ParentChildListLink;    // protected by parent's sec
    RootDevNode_pf_OnEvent  mfOnEvent;              // set before added to tree

    //
    // in use when node is in a state when the process token is non-null
    //
    K2TREE_NODE             GlobalDriverInstanceTreeNode;

    //
    // reference held while driver object exists
    //
    ROOTDEV_NODEREF         DriverNodeSelfRef;

    //
    // wait tokens for RPC object thread
    //
    K2OS_WAITABLE_TOKEN     mObjectCallWaitTokens[2];   // token[0] is for the call.  token[1] is used on object thread during call for client disconnect notification


    K2OS_CRITSEC            Sec;
    struct _InSec {
        //
        // hierarchy
        //
        K2LIST_ANCHOR           ChildList;

        //
        // current state
        //
        RootDevNodeStateType    mState;

        //
        // driver that should be tried next
        //
        char *                  mpDriverCandidate;

        //
        // plat context from kernel
        //
        char *                  mpMountedInfo;          // var-length input used to mount to kernel
        UINT_PTR                mMountedInfoBytes;
        UINT_PTR                mPlatContext;           // nonzero if mounted - unique identifier returned from mount to kernel
        UINT8 *                 mpPlatInfo;             // var-length output returned from mount to kernel
        UINT_PTR                mPlatInfoBytes;

        //
        // ACPI info if specified
        //
        ROOTACPI_NODEINFO *     mpAcpiNodeInfo;

        //
        // currently held resources
        //
        K2LIST_ANCHOR           IrqResList;
        K2LIST_ANCHOR           IoResList;
        K2LIST_ANCHOR           PhysResList;

        struct {
            K2OS_PROCESS_TOKEN      mProcessToken;
            K2OS_MAILSLOT_TOKEN     mMailslotToken;
            ROOTDEV_TIMER           Timer;
            UINT_PTR                mLastStatus;
            BOOL                    mIsEnabled;
            BOOL                    mObjectExists;
            UINT_PTR                mForcedExitCountLeft;
        } Driver;

    } InSec;

};

extern ROOTDEV_NODE     gRootDevNode;
extern K2OS_CRITSEC     gRootDevNode_RefSec;
extern ROOTDEV_NODE *   gpRootDevNode_CleanupList;

void RootDev_Init(void);

#if REF_DEBUG
void RootDevNode_CreateRef_Debug(ROOTDEV_NODEREF *apRef, ROOTDEV_NODE *apDevNode, char const *apFile, int aLine);
#define RootDevNode_CreateRef(r,d) RootDevNode_CreateRef_Debug(r,d,__FILE__,__LINE__)
void RootDevNode_ReleaseRef_Debug(ROOTDEV_NODEREF *apRef, char const *apFile, int aLine);
#define RootDevNode_ReleaseRef(r) RootDevNode_ReleaseRef_Debug(r,__FILE__,__LINE__)
#else
void RootDevNode_CreateRef(ROOTDEV_NODEREF *apRef, ROOTDEV_NODE *apDevNode);
void RootDevNode_ReleaseRef(ROOTDEV_NODEREF *apRef);
#endif

void RootDevNode_Cleanup(ROOTDEV_NODE *apDevNode);

BOOL RootDevNode_Init(ROOTDEV_NODE *apDevNode, ROOTDEV_NODEREF *apRef);
void RootDevNode_OnEvent(ROOTDEV_NODE *apDevNode, ROOTDEV_EVENT *apEvent);
void RootDevNode_OnDriverEvent(ROOTDEV_NODE *apDevNode, ROOTDEV_EVENT *apDriverEvent);

//
// -------------------------------------------------------------------------
//

enum _RootDriverEventType
{
    RootDriverEvent_Invalid = 0,

    RootDriverEvent_CandidateChanged,
    RootDriverEvent_StartFailed,
    RootDriverEvent_ObjectCreated,
    RootDriverEvent_MailslotSet,
    RootDriverEvent_ObjectDeleted,
    RootDriverEvent_Stopped,
    RootDriverEvent_Purged,

    RootDriverEvent_SetRunning,
    RootDriverEvent_NewChild,

    RootDriverEventType_Count
};

enum _RootEventType
{
    RootEvent_Invalid = 0,

    RootEvent_DriverEvent,
    
    RootEventType_Count
};

struct _ROOTDEV_EVENT
{
    INT_PTR volatile        mRefCount;
    RootEvent_pf_Release    mfRelease;

    BOOL                    mOnEventList;
    K2LIST_LINK             EventListLink;

    ROOTDEV_NODEREF         RefDevNode;

    RootEventType           mType;
    RootDriverEventType     mDriverType;
};


void            RootDevMgr_Init(void);
ROOTDEV_EVENT * RootDevMgr_AllocEvent(UINT_PTR aSize, ROOTDEV_NODE *apDevNode);
void            RootDevMgr_AddTimer(ROOTDEV_TIMER *apTimer, UINT_PTR aTimeoutMs, RootTimer_pf_ExpiryCallback afCallback, ROOTDEV_NODE *apDevNode, UINT_PTR aContextVal);
void            RootDevMgr_DelTimer(ROOTDEV_TIMER *apTimer);
void            RootDevMgr_EventOccurred(ROOTDEV_EVENT *apEvent);

extern UINT_PTR gDevMgrInterfaceInstanceId;

//
// -------------------------------------------------------------------------
//

typedef struct _ROOTDEV_DRIVER_NEWCHILD_EVENT ROOTDEV_DRIVER_NEWCHILD_EVENT;
struct _ROOTDEV_DRIVER_NEWCHILD_EVENT
{
    ROOTDEV_EVENT   DevEvent;
    UINT64          mAddress;
    ROOTDEV_NODEREF RefNewDevNode;
};

void RootDriver_Init(void);

extern K2OS_CRITSEC  gRootDriverInstanceTreeSec;
extern K2TREE_ANCHOR gRootDriverInstanceTree;

//
// -------------------------------------------------------------------------
// 

void RootNodeAcpiDriver_Start(void);

//
// -------------------------------------------------------------------------
// 

#if __cplusplus
}
#endif

#endif // __ROOT_H