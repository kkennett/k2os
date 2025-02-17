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
#ifndef __K2OSKERN_H
#define __K2OSKERN_H

#include <k2os.h>
#include <spec/acpi.h>

#if K2_TARGET_ARCH_IS_INTEL
#include <lib/k2archx32.h>
#endif
#if K2_TARGET_ARCH_IS_ARM
#include <lib/k2archa32.h>
#endif

#include <lib/k2rwlock.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

typedef UINT32 (K2_CALLCONV_REGS *K2OSKERN_pf_GetCpuIndex)(void);
UINT32 K2_CALLCONV_REGS K2OSKERN_GetCpuIndex(void);

//
//------------------------------------------------------------------------
//

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_SetIntr)(BOOL aEnable);
BOOL K2_CALLCONV_REGS K2OSKERN_SetIntr(BOOL aEnable);

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_GetIntr)(void);
BOOL K2_CALLCONV_REGS K2OSKERN_GetIntr(void);

//
//------------------------------------------------------------------------
//

struct _K2OSKERN_SEQLOCK
{
    UINT8 mQpaque[3 * K2OS_CACHELINE_BYTES];
};
typedef struct _K2OSKERN_SEQLOCK K2OSKERN_SEQLOCK;

typedef void (K2_CALLCONV_REGS *K2OSKERN_pf_SeqInit)(K2OSKERN_SEQLOCK * apLock);
void K2_CALLCONV_REGS K2OSKERN_SeqInit(K2OSKERN_SEQLOCK * apLock);

typedef BOOL (K2_CALLCONV_REGS *K2OSKERN_pf_SeqLock)(K2OSKERN_SEQLOCK * apLock);
BOOL K2_CALLCONV_REGS K2OSKERN_SeqLock(K2OSKERN_SEQLOCK * apLock);

typedef void  (K2_CALLCONV_REGS *K2OSKERN_pf_SeqUnlock)(K2OSKERN_SEQLOCK * apLock, BOOL aDisp);
void K2_CALLCONV_REGS K2OSKERN_SeqUnlock(K2OSKERN_SEQLOCK * apLock, BOOL aLockDisp);

//
//------------------------------------------------------------------------
//

#define K2OSKERN_THREADED_RWLOCK_SLOT_COUNT 8

struct _K2OSKERN_THREADED_RWLOCK
{
    K2RWLOCK            RwLock;
    K2OSKERN_SEQLOCK    SeqLock;
    UINT8               mSlotsMem[K2OS_CACHELINE_BYTES * (K2OSKERN_THREADED_RWLOCK_SLOT_COUNT + 1)];
};

typedef struct _K2OSKERN_THREADED_RWLOCK K2OSKERN_THREADED_RWLOCK;

typedef void (*K2OSKERN_pf_RwLockInit)(K2OSKERN_THREADED_RWLOCK *apLock);
void K2OSKERN_RwLockInit(K2OSKERN_THREADED_RWLOCK *apLock);

typedef void (*K2OSKERN_pf_RwLockDone)(K2OSKERN_THREADED_RWLOCK *apLock);
void K2OSKERN_RwLockDone(K2OSKERN_THREADED_RWLOCK *apLock);

typedef void (*K2OSKERN_pf_RwLockReadLock)(K2OSKERN_THREADED_RWLOCK *apLock);
void K2OSKERN_RwLockReadLock(K2OSKERN_THREADED_RWLOCK *apLock);

typedef void (*K2OSKERN_pf_RwLockReadUnlock)(K2OSKERN_THREADED_RWLOCK *apLock);
void K2OSKERN_RwLockReadUnlock(K2OSKERN_THREADED_RWLOCK *apLock);

typedef void (*K2OSKERN_pf_RwLockUpgradeReadLockToWriteLock)(K2OSKERN_THREADED_RWLOCK *apLock);
void K2OSKERN_RwLockUpgradeReadLockToWriteLock(K2OSKERN_THREADED_RWLOCK *apLock);

typedef void (*K2OSKERN_pf_RwLockWriteLock)(K2OSKERN_THREADED_RWLOCK *apLock);
void K2OSKERN_RwLockWriteLock(K2OSKERN_THREADED_RWLOCK *apLock);

typedef void (*K2OSKERN_pf_RwLockWriteUnlock)(K2OSKERN_THREADED_RWLOCK *apLock);
void K2OSKERN_RwLockWriteUnlock(K2OSKERN_THREADED_RWLOCK *apLock);

//
//------------------------------------------------------------------------
//

typedef UINT32 (*K2OSKERN_pf_Debug)(char const *apFormat, ...);
UINT32 K2OSKERN_Debug(char const *apFormat, ...);

typedef void (*K2OSKERN_pf_Panic)(char const *apFormat, ...);
void K2OSKERN_Panic(char const *apFormat, ...);

//
//------------------------------------------------------------------------
//

typedef void (K2_CALLCONV_REGS *K2OSKERN_pf_MicroStall)(UINT32 aMicroseconds);
void K2_CALLCONV_REGS K2OSKERN_MicroStall(UINT32 aMicroseconds);

//
//------------------------------------------------------------------------
//

typedef K2STAT (*K2OSKERN_pf_Sysproc_Notify)(K2OS_MSG const *apMsg);
K2STAT K2OSKERN_SysProc_Notify(K2OS_MSG const *apMsg);

//
//------------------------------------------------------------------------
//

//
// these are what the hooks can be told to do
//
typedef enum _KernIntrActionType KernIntrActionType;
enum _KernIntrActionType
{
    KernIntrAction_Invalid = 0,

    KernIntrAction_AtIrq,       // go see if the device is interrupting. return KernIntrDispType
    KernIntrAction_Enable,      // enable the interrupt at the device.  return value ignored
    KernIntrAction_Disable,     // disable the interrupt at the device. return value ignored
    KernIntrAction_Done,        // driver indicating interrupt is finished. return value ignored

    KernIntrActionType_Count
};

//
// these are what the hooks can return when they are given the action KernIntrAction_Eval
//
typedef enum _KernIntrDispType KernIntrDispType;
enum _KernIntrDispType
{
    KernIntrDisp_None = 0,          // this isn't for me (i am not interrupting)
    KernIntrDisp_Handled,           // handled in the hook during irq. do not pulse the interrupt gate
    KernIntrDisp_Handled_Disable,   // handled in the hook during irq. do not pulse the interrupt gate. leave the irq disabled
    KernIntrDisp_Fire,              // pulse the interrupt gate, but do not need interrupt to be disabled
    KernIntrDisp_Fire_Disable,      // pulse the interrupt gate and leave the irq disabled

    KernIntrDispType_Count
};

//
// this is a hook function that returns what to do for the interrupt when it is called
//
typedef KernIntrDispType (*K2OSKERN_pf_Hook_Key)(void *apKey, KernIntrActionType aAction);

//
// create K2OSKERN_IRQ and install it via Arch intr call
//
typedef BOOL (*K2OSKERN_pf_IrqDefine)(K2OS_IRQ_CONFIG *apConfig);
BOOL K2OSKERN_IrqDefine(K2OS_IRQ_CONFIG const *apConfig);

//
// create K2OSKERN_OBJ_INTERRUPT and add it to the IRQ interrupt list
// setting the hook key in the object
//
typedef K2OS_INTERRUPT_TOKEN (*K2OSKERN_pf_IrqHook)(UINT32 aSourceIrq, K2OSKERN_pf_Hook_Key **appKey);
K2OS_INTERRUPT_TOKEN K2OSKERN_IrqHook(UINT32 aSourceIrq, K2OSKERN_pf_Hook_Key *appKey);

//
// raw enable or disable the specified irq using the arch API
//
typedef BOOL (*K2OSKERN_pf_IrqSetEnable)(UINT32 aSourceIrq, BOOL aSetEnable);
BOOL K2OSKERN_IrqSetEnable(UINT32 aSourceIrq, BOOL aSetEnable);

//
// used by drivers to act on the interrupt tokens they receive
//
typedef BOOL (*K2OSKERN_pf_IntrVoteIrqEnable)(K2OS_INTERRUPT_TOKEN aTokIntr, BOOL aSetEnable);
BOOL K2OSKERN_IntrVoteIrqEnable(K2OS_INTERRUPT_TOKEN aTokIntr, BOOL aSetEnable);

typedef BOOL (*K2OSKERN_pf_IntrDone)(K2OS_INTERRUPT_TOKEN aTokIntr);
BOOL K2OSKERN_IntrDone(K2OS_INTERRUPT_TOKEN aTokIntr);

//
//------------------------------------------------------------------------
//

typedef K2STAT (*K2OSKERN_pf_PrepIo)(BOOL aUseHwDma, BOOL aIsWrite, UINT32 aProcId, UINT32 *apAddr, UINT32 *apSizeBytes, K2OS_TOKEN *apRetToken);
K2STAT K2OSKERN_PrepIo(BOOL aUseHwDma, BOOL aIsWrite, UINT32 aProcId, UINT32 *apAddr, UINT32 *apSizeBytes, K2OS_TOKEN *apRetToken);

typedef BOOL (*K2OSKERN_pf_GetFirmwareTable)(K2_GUID128 const *apId, UINT32 *apIoBytes, void *apBuffer);
BOOL K2OSKERN_GetFirmwareTable(K2_GUID128 const *apId, UINT32 *apIoBytes, void *apBuffer);

//
//------------------------------------------------------------------------
//

typedef struct _K2OSKERN_FILE           K2OSKERN_FILE;

typedef struct _K2OSKERN_FSNODE         K2OSKERN_FSNODE;
typedef struct _K2OSKERN_FSNODE_OPS     K2OSKERN_FSNODE_OPS;

typedef struct _K2OSKERN_FSPROV         K2OSKERN_FSPROV;
typedef struct _K2OSKERN_FSPROV_OPS     K2OSKERN_FSPROV_OPS;

typedef struct _K2OSKERN_FILESYS        K2OSKERN_FILESYS;
typedef struct _K2OSKERN_FILESYS_OPS    K2OSKERN_FILESYS_OPS;

typedef void    (*K2OSKERN_pf_FsNodeInit)(K2OSKERN_FILESYS *apFileSys, K2OSKERN_FSNODE *apFsNode);
typedef void    (*K2OSKERN_pf_FsShutdown)(K2OSKERN_FILESYS *apFileSys);
typedef K2STAT  (*K2OSKERN_pf_FsAcquireChild)(K2OSKERN_FILESYS *apFileSys, K2OSKERN_FSNODE *apFsNode, char const *apChildName, K2OS_FileOpenType aOpenType, UINT32 aAccess, UINT32 aNewFileAttrib, K2OSKERN_FSNODE **appRetFsNode);
typedef void    (*K2OSKERN_pf_FsFreeClosedNode)(K2OSKERN_FILESYS *apFileSys, K2OSKERN_FSNODE *apFsNode);

struct _K2OSKERN_FILESYS_OPS
{
    // provided by kernel
    struct
    {
        K2OSKERN_pf_FsNodeInit      FsNodeInit;
        K2OSKERN_pf_FsShutdown      FsShutdown;
    } Kern;
    // filesys fills these in
    struct
    {
        K2OSKERN_pf_FsAcquireChild      AcquireChild;
        K2OSKERN_pf_FsFreeClosedNode    FreeClosedNode;
    } Fs;
};

struct _K2OSKERN_FILESYS
{
    struct
    {
        K2OSKERN_FSPROV *   mpKernFsProv;
        void *              mpAttachContext;
    } Kern;
    struct
    {
        UINT32  mProvInstanceContext;
        BOOL    mReadOnly;
        BOOL    mCaseSensitive;
        BOOL    mDoNotUseForPaging;
    } Fs;
    K2OSKERN_FILESYS_OPS    Ops;
};

typedef struct _K2OSKERN_FSFILE_LOCK K2OSKERN_FSFILE_LOCK;
struct _K2OSKERN_FSFILE_LOCK
{
    K2OSKERN_FSNODE *   mpFsNode;
    UINT8 *             mpData;
    UINT32              mLockedByteCount;
};

typedef INT_PTR (*K2OSKERN_pf_FsNode_AddRef)(K2OSKERN_FSNODE *apFsNode);
typedef INT_PTR (*K2OSKERN_pf_FsNode_Release)(K2OSKERN_FSNODE *apFsNode);
typedef K2STAT  (*K2OSKERN_pf_FsNode_GetSizeBytes)(K2OSKERN_FSNODE *apFsNode, UINT64 *apRetSizeBytes);
typedef K2STAT  (*K2OSKERN_pf_FsNode_GetTime)(K2OSKERN_FSNODE *apFsNode, UINT64 *apRetTime);
typedef K2STAT  (*K2OSKERN_pf_FsNode_LockData)(K2OSKERN_FSNODE *apFsNode, UINT64 const *apOffset, UINT32 aByteCount, BOOL aWriteable, K2OSKERN_FSFILE_LOCK **appRetFileLock);
typedef void    (*K2OSKERN_pf_FsNode_UnlockData)(K2OSKERN_FSFILE_LOCK *apLock);

struct _K2OSKERN_FSNODE_OPS
{
    struct
    {
        K2OSKERN_pf_FsNode_AddRef       AddRef;
        K2OSKERN_pf_FsNode_Release      Release;
    } Kern;
    struct
    {
        K2OSKERN_pf_FsNode_GetSizeBytes GetSizeBytes;
        K2OSKERN_pf_FsNode_GetTime      GetTime;
        K2OSKERN_pf_FsNode_LockData     LockData;
        K2OSKERN_pf_FsNode_UnlockData   UnlockData;
    } Fs;
};

struct _K2OSKERN_FSNODE
{
    INT_PTR volatile        mRefCount;

    K2OSKERN_SEQLOCK        ChangeSeqLock;

    struct
    {
        K2TREE_NODE         ParentsChildTreeNode;
    } ParentLocked;

    struct
    {
        K2TREE_ANCHOR   ChildTree;
        UINT32          mCurrentShare;
        UINT32          mFsAttrib;      // dir bit is never changeable. see Static.mIsDir
    } Locked;

    struct
    {
        K2OSKERN_FILESYS *  mpFileSys;
        K2OSKERN_FSNODE *   mpParentDir;
        BOOL                mIsDir;
        K2OSKERN_FSNODE_OPS Ops;
        UINT32              mFsContext;
        char                mName[K2OS_FSITEM_MAX_COMPONENT_NAME_LENGTH + 1];
    } Static;
};

//
//------------------------------------------------------------------------
//

// {DFCE881F-24CE-4EF3-9AEC-1524D07F2C1D}
#define K2OS_IFACE_FSPROV               { 0xdfce881f, 0x24ce, 0x4ef3, { 0x9a, 0xec, 0x15, 0x24, 0xd0, 0x7f, 0x2c, 0x1d } }

typedef enum _K2OS_FsProv_Method K2OS_FsProv_Method;
enum _K2OS_FsProv_Method
{
    K2OS_FsProv_Method_Invalid = 0,

    K2OS_FsProv_Method_GetCount,
    K2OS_FsProv_Method_GetEntry,

    K2OS_FsProv_Method_Count
};

#define K2OSKERN_FSPROV_NAME_MAX_LEN    63

typedef K2STAT (*K2OSKERN_FsProv_pf_Probe)(K2OSKERN_FSPROV *apProv, void *apAttachContext, BOOL *apRetMatch);
typedef K2STAT (*K2OSKERN_FsProv_pf_Attach)(K2OSKERN_FSPROV *apProv, K2OSKERN_FILESYS *apFileSys, K2OSKERN_FSNODE *apRootFsNode);

struct _K2OSKERN_FSPROV_OPS
{
    K2OSKERN_FsProv_pf_Probe    Probe;
    K2OSKERN_FsProv_pf_Attach   Attach;
};

#define K2OSKERN_FSPROV_FLAG_USE_VOLUME_IFINSTID    1
#define K2OSKERN_FSPROV_FLAG_USE_SMBCONN_IFINSTID   2

struct _K2OSKERN_FSPROV
{
    char                mName[K2OSKERN_FSPROV_NAME_MAX_LEN + 1];
    UINT_PTR            mFlags;
    K2OSKERN_FSPROV_OPS Ops;
};

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif


#endif // __K2OSKERN_H