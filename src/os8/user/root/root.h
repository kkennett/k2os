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

typedef struct _DEVNODE             DEVNODE;
typedef struct _DEVNODE_REF         DEVNODE_REF;
typedef struct _DEVEVENT            DEVEVENT;
typedef enum   _DevNodeEventType    DevNodeEventType;
typedef struct _ACPI_NODE           ACPI_NODE;
typedef enum   _DevNodeStateType    DevNodeStateType;
typedef enum   _DevNodeIntentType   DevNodeIntentType;
typedef struct _DEV_RES             DEV_RES;
typedef struct _DEV_IO              DEV_IO;
typedef struct _DEV_IRQ             DEV_IRQ;
typedef struct _DEV_PHYS            DEV_PHYS;

struct _DEVNODE_REF
{
    DEVNODE *       mpDevNode;
    K2LIST_LINK     ListLink;
#if REF_DEBUG
    char const *    mpFile;
    int             mLine;
#endif
};

enum _DevNodeEventType
{
    DevNodeEvent_Error = 0,

    DevNodeEvent_TimerExpired,
    DevNodeEvent_ChildStopped,
    DevNodeEvent_ChildRefsHitZero,
    DevNodeEvent_NewChildMounted,
    DevNodeEvent_ChildDriverSpecChanged,
    DevNodeEvent_ChildCameOnline,
    DevNodeEvent_ChildWentOffline,

    DevNodeEventType_Count
};

struct _DEVEVENT
{
    BOOL                    mPending;
    K2LIST_LINK             ListLink;

    DEVNODE_REF             DevNodeRef;

    DevNodeEventType        mType;
    UINT_PTR                mArg;
};


#define ROOTPLAT_MOUNT_MAX_BYTES    1024

enum _DevNodeStateType
{
    DevNodeState_Invalid = 0,
    
    DevNodeState_GoingOnline_WaitObject,
    DevNodeState_GoingOnline_WaitMailbox,
    DevNodeState_Online,
    DevNodeState_GoingOffline_WaitProcExit,
    DevNodeState_GoingOffline_WaitObjDel,
    DevNodeState_GoingOffline_DelayKill,

    // all offline states go after this
    DevNodeState_Offline_Stopped,               // mIntent == NormalStop
    DevNodeState_Offline_ErrorStopped,          // mIntent == ErrorStop
    DevNodeState_Offline_NoDriver,
    DevNodeState_Offline_CheckForDriver,

    DevNodeStateType_Count
};
#define DEVNODESTATE_OFFLINE_FIRST DevNodeState_Offline_Stopped

enum   _DevNodeIntentType
{
    DevNodeIntent_None = 0,

    DevNodeIntent_ErrorStop,
    DevNodeIntent_NormalStop,
    DevNodeIntent_Restart,                      // when reach offline state, check for driver. if no driver go to Offline_NoDriver. Else try to start it
    DevNodeIntent_DeferToProcessExitCode,       // when process has exited, figure out state based on its exit code
    DevNodeIntent_DisableOnly,

    DevNodeIntentType_Count
};

struct _DEV_RES
{
    UINT_PTR        mAcpiContext;   // can be zero if representing a PCI bus range found in MCFG
    UINT_PTR        mPlatContext;   // mounted in kernel platform
    K2LIST_LINK     ListLink;
};

struct _DEV_IO
{
    DEV_RES         Res;
    K2OSDRV_RES_IO  DrvResIo;
};

struct _DEV_IRQ
{
    DEV_RES         Res;
    K2OSDRV_RES_IRQ DrvResIrq;
};

struct _DEV_PHYS
{
    DEV_RES             Res;
    K2OSDRV_RES_PHYS    DrvResPhys;
};

struct _DEVNODE
{
    K2LIST_ANCHOR       RefList;

    DEVNODE_REF         RefToParent;
    struct {
        K2LIST_LINK     ParentChildListLink;
    } InParentSec;

    DEVNODE_REF         RefToSelf_OwnedByParent;

    UINT_PTR            mBusType;         
    K2_DEVICE_IDENT     DeviceIdent;      
    UINT_PTR            mParentDriverOwned_RemoteObjectId;

    struct {
        K2TREE_NODE     InstanceTreeNode;
    } Driver;

    struct {
        DEVNODE_REF     RefToSelf_TimerActive;
        BOOL            mOnQueue;
        UINT_PTR        mRemainingMs;
        K2LIST_LINK     ListLink;
        UINT_PTR        mArg;
    } InTimerSec;

    K2OS_CRITSEC    Sec;
    struct {
        //
        // Child dev nodes
        //
        K2LIST_ANCHOR       ChildList;

        //
        // current state
        //
        DevNodeStateType    mState;

        //
        // current intent
        //
        DevNodeIntentType   mIntent;

        //
        // if attached to an ACPI node
        //
        ACPI_NODE *         mpAcpiNode;

        //
        // plat context from kernel
        //
        char *              mpMountedInfo;          // var-length input used to mount to kernel
        UINT_PTR            mMountedInfoBytes;
        UINT_PTR            mPlatContext;           // nonzero if mounted - unique identifier returned from mount to kernel
        UINT8 *             mpPlatInfo;             // var-length output returned from mount to kernel
        UINT_PTR            mPlatInfoBytes;

        //
        // currently held resources
        //
        K2LIST_ANCHOR       IrqResList;
        K2LIST_ANCHOR       IoResList;
        K2LIST_ANCHOR       PhysResList;

        //
        // candidate driver
        //
        char *              mpDriverCandidate;

        struct {
            char *                  mpLaunchCmd;
            K2OS_PROCESS_TOKEN      mProcessToken;
            K2OS_MAILSLOT_TOKEN     mMailslotToken;
            UINT_PTR                mLastStatus;
            BOOL                    mObjectExists;
            BOOL                    mEnabled;
            UINT_PTR                mForcedExitCountLeft;
        } Driver;

    } InSec;
};

void        Dev_Init(void);

void        Dev_CreateRef(DEVNODE_REF *apRef, DEVNODE *apNode);
void        Dev_ReleaseRef(DEVNODE_REF *apRef);

void        Dev_NodeLocked_OnEvent(DEVNODE *apNode, DevNodeEventType aType, UINT_PTR aArg);

DEVNODE *   Dev_NodeLocked_CreateChildNode(DEVNODE *apParent, K2_DEVICE_IDENT const *apIdent);
void        Dev_NodeLocked_DeleteChildNode(DEVNODE *apParent, DEVNODE *apChild);

K2STAT      Dev_NodeLocked_OnDriverHostObjCreate(DEVNODE *apNode);
void        Dev_NodeLocked_OnDriverHostObjDelete(DEVNODE *apNode);

K2STAT      Dev_NodeLocked_OnSetMailbox(DEVNODE *apNode, UINT_PTR aRemoteToken);
void        Dev_NodeLocked_OnStop(DEVNODE *apNode);

K2STAT      Dev_NodeLocked_OnEnumRes(DEVNODE *apNode, K2OSDRV_ENUM_RES_OUT *apOut);
K2STAT      Dev_NodeLocked_OnGetRes(DEVNODE *apNode, K2OSDRV_GET_RES_IN const *apArgsIn, UINT8 *apOutBuf, UINT_PTR aOutBufBytes, UINT_PTR *apRetUsedOutBytes);
K2STAT      Dev_NodeLocked_RunAcpiMethod(DEVNODE *apNode, K2OSDRV_RUN_ACPI_IN const *apArgs, UINT_PTR *apRetResult);

K2STAT      Dev_NodeLocked_MountChild(DEVNODE *apNode, K2OSDRV_MOUNT_CHILD_IN const *apArgsIn, K2OSDRV_MOUNT_CHILD_OUT *apArgsOut);

K2STAT      Dev_NodeLocked_Enable(DEVNODE *apNode);
K2STAT      Dev_NodeLocked_Disable(DEVNODE *apNode, DevNodeIntentType aIntent);

K2STAT      Dev_NodeLocked_AddChildRes(DEVNODE *apNode, K2OSDRV_ADD_CHILD_RES_IN const *apArgsIn);

K2STAT      Dev_NodeLocked_NewDevUser(DEVNODE *apNode, K2OSDRV_NEW_DEV_USER_IN const *apArgsIn, K2OSDRV_NEW_DEV_USER_OUT *apArgsOut);
K2STAT      Dev_NodeLocked_AcceptDevUser(DEVNODE *apNode, K2OSDRV_ACCEPT_DEV_USER_IN const *apArgsIn);

extern DEVNODE * const gpDevTree;

//
// -------------------------------------------------------------------------
// 

extern UINT_PTR            gMainThreadId;
extern K2OS_MAILBOX        gMainMailbox;
extern K2OS_MAILSLOT_TOKEN gMainMailslotToken;

//
// -------------------------------------------------------------------------
// 

UINT_PTR Debug_Printf(char const *apFormat, ...);

//
// -------------------------------------------------------------------------
// 

void Res_Init(void);
BOOL Res_IoHeap_Reserve(K2OS_IOADDR_INFO const *apIoInfo, UINT_PTR aContext, BOOL aExclusive);
BOOL Res_PhysHeap_Reserve(K2OS_PHYSADDR_INFO const *apPhysInfo, UINT_PTR aContext, BOOL aExclusive);

//
// -------------------------------------------------------------------------
// 

struct _ACPI_NODE
{
    ACPI_HANDLE         mhObject;

    ACPI_NODE *         mpParentAcpiNode;
    K2LIST_ANCHOR       ChildAcpiNodeList;
    K2LIST_LINK         ParentChildListLink;

    char                mName[8];       // 4 chars. [4] is zero
    char const *        mpHardwareId;
    BOOL                mHasUniqueId;
    UINT64              mUniqueId;

    ACPI_DEVICE_INFO *  mpDeviceInfo;

    ACPI_BUFFER         CurrentAcpiRes;
};

void   Acpi_Init(void);
void   Acpi_Enable(void);
void   Acpi_StartDriver(void);
void   Acpi_Describe(ACPI_NODE *apAcpiNode, char **appOut, UINT_PTR *apLeft);
BOOL   Acpi_MatchAndAttachChild(ACPI_NODE *apAcpiNode, UINT64 const *apBusAddress, DEVNODE *apDevNode);
K2STAT Acpi_RunMethod(ACPI_NODE *apAcpiNode, char const *apMethod, BOOL aHasInput, UINT32 aInput, UINT32 *apRetOutput);
void   Acpi_NodeLocked_PopulateRes(DEVNODE *apNode);

//
// -------------------------------------------------------------------------
//

void DevMgr_Init(void);
void DevMgr_Run(void);

BOOL DevMgr_QueueEvent(DEVNODE *apDevNode, DevNodeEventType aType, UINT_PTR aArg);

void DevMgr_NodeLocked_AddTimer(DEVNODE *apNode, UINT_PTR aTimeoutMs);
void DevMgr_NodeLocked_DelTimer(DEVNODE *apNode);

//
// -------------------------------------------------------------------------
//

#define DRIVER_CREATEOBJECT_TIMEOUT_MS      30000
#define DRIVER_SETMAILBOX_TIMEOUT_MS        10000
#define DRIVER_PROCEXIT_CHECK_INTERVAL_MS   400
#define DRIVER_DELAYKILL_CHECK_INTERVAL_MS  500

void Driver_Init(void);

extern K2TREE_ANCHOR    gDriverInstanceTree;
extern K2OS_CRITSEC     gDriverInstanceTreeSec;

//
// -------------------------------------------------------------------------
// 

#if __cplusplus
}
#endif

#endif // __ROOT_H