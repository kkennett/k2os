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

#ifndef __K2OSEXEC_H
#define __K2OSEXEC_H

#include <k2osplat.h>
#include <k2osddk.h>
#include "../main/kernexec.h"
#include "../k2osacpi/k2osacpi.h"
#include <spec/k2pci.h>
#include <k2osdev_blockio.h>

#if __cplusplus
extern "C" {
#endif

//
// -------------------------------------------------------------------------
// 

#define REF_DEBUG   1

typedef struct  _DEVNODE            DEVNODE;
typedef struct  _DEVNODE_REF        DEVNODE_REF;
typedef struct  _DEVEVENT           DEVEVENT;
typedef enum    _DevNodeEventType   DevNodeEventType;
typedef enum    _DevNodeStateType   DevNodeStateType;
typedef enum    _DevNodeIntentType  DevNodeIntentType;
typedef struct  _DEV_RES            DEV_RES;
typedef struct  _DEV_IO             DEV_IO;
typedef struct  _DEV_IRQ            DEV_IRQ;
typedef struct  _DEV_PHYS           DEV_PHYS;
typedef struct  _ACPI_NODE          ACPI_NODE;
typedef struct  _BLOCKIO            BLOCKIO;
typedef struct  _BLOCKIO_RANGE      BLOCKIO_RANGE;

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
    BOOL                mPending;
    K2LIST_LINK         ListLink;

    DEVNODE_REF         DevNodeRef;

    DevNodeEventType    mType;
    UINT32              mArg;
};

enum _DevNodeStateType
{
    DevNodeState_Invalid = 0,

    DevNodeState_GoingOnline_PreInstantiate,
    DevNodeState_GoingOnline_WaitStarted,
    DevNodeState_Online,
    DevNodeState_Online_Unresponsive,

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
    K2OSDDK_RES     DdkRes;
    UINT32          mAcpiContext;   // can be zero if representing a PCI bus range found in MCFG
    K2OSPLAT_RES    mPlatRes;       // mounted in PLAT
    K2LIST_LINK     ListLink;
};

#define DRIVER_INSTANTIATE_TIMEOUT_MS   5000
#define DRIVER_START_TIMEOUT_MS         5000

typedef K2STAT (*K2OS_pf_Driver_CreateInstance)(K2OS_DEVCTX const aDevMgrContext, void **appRetDriverContext);
typedef K2STAT (*K2OS_pf_Driver_Call)(void *apDriverContext);

struct _DEVNODE
{
    K2LIST_ANCHOR       RefList;

    DEVNODE_REF         RefToParent;
    struct {
        K2LIST_LINK     ParentChildListLink;
    } InParentSec;
    UINT64              mParentRelativeBusAddress;
    UINT32              mParentsChildInstanceId;

    DEVNODE_REF         RefToSelf_OwnedByParent;

    UINT32              mBusType;
    K2_DEVICE_IDENT     DeviceIdent;

    struct {
        K2TREE_NODE                     InstanceTreeNode;
        void *                          mpContext;
        K2OS_pf_Driver_CreateInstance   mfCreateInstance;
        K2OS_pf_Driver_Call             mfStartDriver;
        K2OS_pf_Driver_Call             mfStopDriver;
        K2OS_pf_Driver_Call             mfDeleteInstance;
        K2OSKERN_pf_Hook_Key            mfIntrHookKey;
    } Driver;

    struct {
        DEVNODE_REF     RefToSelf_TimerActive;
        BOOL            mOnQueue;
        UINT64          mTargetHfTick;
        K2LIST_LINK     ListLink;
        UINT32          mArg;
    } InTimerSec;

    K2OS_CRITSEC    Sec;

    struct {
        //
        // Child dev nodes
        //
        UINT32              mLastChildInstanceId;
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
        UINT32              mMountedInfoBytes;
        K2OSPLAT_DEV        mPlatDev;               // nonzero if mounted - unique identifier returned from mount to kernel
        UINT8 *             mpPlatInfo;             // var-length output returned from mount to kernel
        UINT32              mPlatInfoBytes;

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
            UINT32          mLastStatus;
            BOOL            mEnabled;
            K2OS_XDL        mXdl;
        } Driver;

        struct {
            K2LIST_ANCHOR   List;
        } BlockIo;

    } InSec;
};

extern DEVNODE * const gpDevTree;

void Dev_Init(void);
void Dev_CreateRef(DEVNODE_REF *apRef, DEVNODE *apNode);
void Dev_ReleaseRef(DEVNODE_REF *apRef);

void        Dev_NodeLocked_OnEvent(DEVNODE *apNode, DevNodeEventType aType, UINT32 aArg);

DEVNODE *   Dev_NodeLocked_CreateChildNode(DEVNODE *apParent, UINT64 const *apParentRelativeBusAddress, UINT32 aChildBusType, K2_DEVICE_IDENT const *apChildIdent);
void        Dev_NodeLocked_DeleteChildNode(DEVNODE *apParent, DEVNODE *apChild);

K2STAT      Dev_NodeLocked_MountChild(DEVNODE *apParent, UINT32 aMounsFlagsAndBusType, UINT64 const *apBusSpecificAddress, K2_DEVICE_IDENT const *apIdent, UINT32 *apRetChildInstanceId);

K2STAT      Dev_NodeLocked_OnSetMailbox(DEVNODE *apNode, K2OS_MAILBOX_TOKEN aTokMailbox);
//void        Dev_NodeLocked_OnStop(DEVNODE *apNode);

K2STAT      Dev_NodeLocked_AddRes(DEVNODE *apDevNode, K2OSDDK_RESDEF const *apResDef, void * aAcpiContext);

K2STAT      Dev_NodeLocked_Enable(DEVNODE *apNode);
//K2STAT      Dev_NodeLocked_Disable(DEVNODE *apNode, DevNodeIntentType aIntent);

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

void ACPI_Init(K2OSACPI_INIT *apInit);
void ACPI_Enable(void);
void ACPI_StartSystemBusDriver(void);
BOOL ACPI_MatchAndAttachChild(ACPI_NODE *apParentAcpiNode, UINT64 const *apBusAddrOfChild, DEVNODE *apChildNode);
void ACPI_NodeLocked_PopulateRes(DEVNODE *apDevNode);
void ACPI_Describe(ACPI_NODE * apAcpiNode, char **appOut, UINT32 *apLeft);

//
// -------------------------------------------------------------------------
// 

void DevMgr_Init(void);
BOOL DevMgr_QueueEvent(DEVNODE * apNode, DevNodeEventType aType, UINT32 aArg);
void DevMgr_Run(void);
void DevMgr_NodeLocked_AddTimer(DEVNODE *apNode, UINT32 aTimeoutMs);
void DevMgr_NodeLocked_DelTimer(DEVNODE *apNode);

//
// -------------------------------------------------------------------------
// 

typedef struct _EXEC_PLAT EXEC_PLAT;
struct _EXEC_PLAT
{
    K2OSPLAT_pf_DeviceCreate    DeviceCreate;
    K2OSPLAT_pf_DeviceAddRes    DeviceAddRes;
    K2OSPLAT_pf_DeviceRemove    DeviceRemove;
};

//
//------------------------------------------------------------------------
//

typedef void (*WorkerThread_pf_WorkFunc)(void *apArg);

K2STAT WorkerThread_Exec(WorkerThread_pf_WorkFunc aWorkFunc, void *apArg);

//
//------------------------------------------------------------------------
//

struct _BLOCKIO_RANGE
{
    UINT32          mStartBlock;
    UINT32          mBlockCount;
    UINT32          mOwner;
    UINT32          mId;
    UINT32          mPrivate;
    K2LIST_LINK     ListLink;
};

struct _BLOCKIO
{
    DEVNODE *                   mpDevNode;
    K2LIST_LINK                 DevNodeBlockIoListLink;
    void *                      mpDriverContext;

    K2OS_IFINST_TOKEN           mTokIfInst;
    K2OS_IFINST_ID              mIfInstId;

    K2OSDDK_BLOCKIO_REGISTER    Register;

    UINT32                      mMediaBlockCount;
    UINT32                      mMediaBlockSizeBytes;

    K2OS_CRITSEC                Sec;
    K2OS_NOTIFY_TOKEN           mTokNotify;
    K2OS_THREAD_TOKEN           mTokThread;
    UINT32                      mThreadId;
    K2OSEXEC_IOTHREAD           IoThread;
    K2LIST_ANCHOR               IopList;

    UINT32                      mLastRangeId;
    K2LIST_ANCHOR               RangeList;
};

UINT32 BlockIo_DeviceThread(void *apArg);

//
//------------------------------------------------------------------------
//

extern UINT32       gMainThreadId;
extern EXEC_PLAT    gPlat;
extern K2OSKERN_DDK gKernDdk;

//
// -------------------------------------------------------------------------
// 

#if __cplusplus
}
#endif

#endif // __K2OSKERN_H