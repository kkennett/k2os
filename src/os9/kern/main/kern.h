
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
//   OF THIS SOFTWARE, EVEN IFK ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __KERN_H
#define __KERN_H

#include <kern/k2osplat.h>
#include <k2osstor.h>
#include "kerndef.inc"
#include <lib/k2rofshelp.h>
#include <lib/k2ring.h>
#include "kerniface.h"
#include <lib/k2ramheap.h>
#include <lib/k2heap.h>
#include "kernshared.h"
#include "kernexec.h"

/* --------------------------------------------------------------------------------- */

#define KTRACE_ON                   0
#define DEBUG_REF                   0
#define SENTINEL_REF                0
#define DEBUG_LOCK                  0
#define K2OSKERN_TRACE_THREAD_LIFE  0

#if DEBUG_LOCK
#define K2OSKERN_SeqLock(x)     K2OSKERN_DebugSeqLock(x,__FILE__,__LINE__)
BOOL K2_CALLCONV_REGS K2OSKERN_DebugSeqLock(K2OSKERN_SEQLOCK * apLock, char const *apFile, int aLine);
#endif

//
// sanity checks
//
#if K2_TARGET_ARCH_IS_ARM
K2_STATIC_ASSERT(K2OSKERN_PTE_PRESENT_BIT == A32_PTE_PRESENT);
K2_STATIC_ASSERT(K2OS_KVA_ARCHSPEC_BASE == A32_HIGH_VECTORS_ADDRESS);
#else
K2_STATIC_ASSERT(K2OSKERN_PTE_PRESENT_BIT == X32_PTE_PRESENT);
#endif


/* --------------------------------------------------------------------------------- */

typedef struct _K2OSKERN_CPUCORE            K2OSKERN_CPUCORE;

typedef struct _K2OSKERN_OBJ_HEADER         K2OSKERN_OBJ_HEADER;
typedef struct _K2OSKERN_OBJ_PROCESS        K2OSKERN_OBJ_PROCESS;
typedef struct _K2OSKERN_OBJ_THREAD         K2OSKERN_OBJ_THREAD;
typedef struct _K2OSKERN_OBJ_PAGEARRAY      K2OSKERN_OBJ_PAGEARRAY;
typedef struct _K2OSKERN_OBJ_VIRTMAP        K2OSKERN_OBJ_VIRTMAP;
typedef struct _K2OSKERN_OBJ_NOTIFY         K2OSKERN_OBJ_NOTIFY;
typedef struct _K2OSKERN_OBJ_GATE           K2OSKERN_OBJ_GATE;
typedef struct _K2OSKERN_OBJ_ALARM          K2OSKERN_OBJ_ALARM;
typedef struct _K2OSKERN_OBJ_SEM            K2OSKERN_OBJ_SEM;
typedef struct _K2OSKERN_OBJ_SEMUSER        K2OSKERN_OBJ_SEMUSER;
typedef struct _K2OSKERN_OBJ_INTERRUPT      K2OSKERN_OBJ_INTERRUPT;
typedef struct _K2OSKERN_OBJ_MAILBOX        K2OSKERN_OBJ_MAILBOX;
typedef struct _K2OSKERN_OBJ_MAILBOXOWNER   K2OSKERN_OBJ_MAILBOXOWNER;
typedef struct _K2OSKERN_OBJ_MAILSLOT       K2OSKERN_OBJ_MAILSLOT;
typedef struct _K2OSKERN_OBJ_IFINST         K2OSKERN_OBJ_IFINST;
typedef struct _K2OSKERN_OBJ_IFENUM         K2OSKERN_OBJ_IFENUM;
typedef struct _K2OSKERN_OBJ_IFSUBS         K2OSKERN_OBJ_IFSUBS;
typedef struct _K2OSKERN_OBJ_IPCEND         K2OSKERN_OBJ_IPCEND;
typedef struct _K2OSKERN_OBJ_NOTIFYPROXY    K2OSKERN_OBJ_NOTIFYPROXY;


/* --------------------------------------------------------------------------------- */

typedef struct  _K2OSKERN_PHYSTRACK K2OSKERN_PHYSTRACK;
typedef struct  _K2OSKERN_PHYSSCAN  K2OSKERN_PHYSSCAN;
typedef struct  _K2OSKERN_PHYSRES   K2OSKERN_PHYSRES;
typedef enum    _KernPhysPageList   KernPhysPageList;

enum _KernPhysPageList
{
    KernPhysPageList_None,          //  0       Not on a page list

    //
    // global lists here
    //
    KernPhysPageList_Overhead,      // vector page, ap start pages, etc.
    KernPhysPageList_Kern_ThreadPages,

    KernPhysPageList_Count,         // -----------------------------------

    //
    // per-process lists here
    //

    KernPhysPageList_Proc_Res_Dirty,
    KernPhysPageList_Proc_Res_Clean,
    KernPhysPageList_Proc_PageTables,
    KernPhysPageList_Proc_ThreadPages,
    KernPhysPageList_Proc_Working,
    KernPhysPageList_Proc_Token,

    KernPhysPageList_KernThread_Stack,

    // 31 is max
};

struct _K2OSKERN_PHYSTRACK
{
    union
    {
        UINT32  mAsUINT32;
        struct
        {
            UINT32  Exists : 1;
            UINT32  Free   : 1;
            UINT32  UC_CAP : 1;
            UINT32  WC_CAP : 1;
            UINT32  WT_CAP : 1;
            UINT32  WB_CAP : 1;
            UINT32  UCE_CAP : 1;
            UINT32  WP_CAP : 1;
            UINT32  RP_CAP : 1;
            UINT32  XP_CAP : 1;
            UINT32  PageListIx : 5;
            UINT32  BlockSize : 5;
            UINT32  Unused : 12;
        } Field;
    } Flags;
    void *                  mpOwnerProc;    // used if page is on mixed-process list
    K2LIST_LINK             ListLink;
};
K2_STATIC_ASSERT(sizeof(K2OSKERN_PHYSTRACK) == K2OS_PHYSTRACK_BYTES);
K2_STATIC_ASSERT(sizeof(K2OSKERN_PHYSTRACK) == sizeof(K2OS_PHYSTRACK_UEFI));

struct _K2OSKERN_PHYSSCAN
{
    K2OSKERN_PHYSTRACK *    mpCurTrack;
    UINT32                  mPhysAddr;
    UINT32                  mTrackLeft;
};

struct _K2OSKERN_PHYSRES
{
    UINT32 volatile mPageCount;
};

/* --------------------------------------------------------------------------------- */

typedef enum   _KernDpcPrioType         KernDpcPrioType;
typedef struct _K2OSKERN_DPC            K2OSKERN_DPC;
typedef struct _K2OSKERN_DPC_SIMPLE     K2OSKERN_DPC_SIMPLE;

enum _KernDpcPrioType
{
    KernDpcPrioType_Invalid,

    KernDpcPrio_Lo,
    KernDpcPrio_Med,
    KernDpcPrio_Hi,

    KernDpcPrioType_Count
};

typedef void (*K2OSKERN_pf_DPC)(K2OSKERN_CPUCORE volatile *apThisCore, void *apKey);

struct _K2OSKERN_DPC
{
    K2OSKERN_pf_DPC *   mpKey;          //   called via (*mpKey)(apThisCore, mpKey);
    K2LIST_LINK         DpcListLink;
};

struct _K2OSKERN_DPC_SIMPLE
{
    K2OSKERN_pf_DPC Func;       // KernCpu_QueueDpc(&simple.Dpc, &simple.Func);
    K2OSKERN_DPC    Dpc;
};

/* --------------------------------------------------------------------------------- */

typedef enum _KernObjType           KernObjType;
typedef struct _K2OSKERN_OBJREF     K2OSKERN_OBJREF;

enum _KernObjType
{
    KernObjType_Invalid = 0,

    KernObj_Process,        // 1
    KernObj_Thread,         // 2
    KernObj_PageArray,      // 3
    KernObj_VirtMap,        // 4
    KernObj_Notify,         // 5
    KernObj_Gate,           // 6
    KernObj_Alarm,          // 7
    KernObj_Sem,            // 8
    KernObj_SemUser,        // 9 
    KernObj_Interrupt,      // 10
    KernObj_Mailbox,        // 11
    KernObj_MailboxOwner,   // 12
    KernObj_Mailslot,       // 13
    KernObj_IfInst,         // 14
    KernObj_IfEnum,         // 15
    KernObj_IfSubs,         // 16
    KernObj_IpcEnd,         // 17
    KernObj_NotifyProxy,    // 18

    KernObjType_Count
};

#define K2OSKERN_OBJ_FLAG_PERMANENT         0x8000
#define K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO  0x0001

struct _K2OSKERN_OBJREF
{
#if SENTINEL_REF
    UINT32          mSentinel0;
#endif
    union
    {
        K2OSKERN_OBJ_HEADER *       AsAny;
        K2OSKERN_OBJ_PROCESS *      AsProc;
        K2OSKERN_OBJ_THREAD *       AsThread;
        K2OSKERN_OBJ_PAGEARRAY *    AsPageArray;
        K2OSKERN_OBJ_VIRTMAP *      AsVirtMap;
        K2OSKERN_OBJ_NOTIFY *       AsNotify;
        K2OSKERN_OBJ_GATE *         AsGate;
        K2OSKERN_OBJ_ALARM *        AsAlarm;
        K2OSKERN_OBJ_SEM *          AsSem;
        K2OSKERN_OBJ_SEMUSER *      AsSemUser;
        K2OSKERN_OBJ_INTERRUPT *    AsInterrupt;
        K2OSKERN_OBJ_MAILBOX *      AsMailbox;
        K2OSKERN_OBJ_MAILBOXOWNER * AsMailboxOwner;
        K2OSKERN_OBJ_MAILSLOT *     AsMailslot;
        K2OSKERN_OBJ_IFINST *       AsIfInst;
        K2OSKERN_OBJ_IFENUM *       AsIfEnum;
        K2OSKERN_OBJ_IFSUBS *       AsIfSubs;
        K2OSKERN_OBJ_IPCEND *		AsIpcEnd;
        K2OSKERN_OBJ_NOTIFYPROXY *  AsNotifyProxy;
    };
#if SENTINEL_REF
    UINT32          mSentinel1;
#endif
    K2LIST_LINK     RefObjListLink;
#if SENTINEL_REF
    UINT32          mSentinel2;
    K2LIST_ANCHOR * mpRefList;
    UINT32          mSentinel3;
#endif
#if DEBUG_REF
    char const *    mpFile;
    int             mLine;
#endif
};

struct _K2OSKERN_OBJ_HEADER
{
    UINT16              mObjType;
    UINT16              mObjFlags;
    UINT32 volatile     mTokenCount;
    K2LIST_ANCHOR       RefObjList;
    K2OSKERN_DPC_SIMPLE ObjDpc;
};

/* --------------------------------------------------------------------------------- */

typedef struct _K2OSKERN_TLBSHOOT           K2OSKERN_TLBSHOOT;
typedef struct _K2OSKERN_COREMEMORY         K2OSKERN_COREMEMORY;
typedef enum   _KernIciType                 KernIciType;
typedef enum   _KernCpuCoreEventType        KernCpuCoreEventType;
typedef struct _K2OSKERN_CPUCORE_EVENT      K2OSKERN_CPUCORE_EVENT;
typedef struct _K2OSKERN_CPUCORE_EVENT      K2OSKERN_CPUCORE_EVENT;
typedef struct _K2OSKERN_CPUCORE_ICI        K2OSKERN_CPUCORE_ICI;
typedef enum   _KernTickModeType            KernTickModeType;

#if K2_TARGET_ARCH_IS_INTEL
typedef struct _K2OSKERN_ARCH_EXEC_CONTEXT  K2OSKERN_ARCH_EXEC_CONTEXT;
struct _K2OSKERN_ARCH_EXEC_CONTEXT
{
    UINT32      DS;                     // last pushed by int common
    X32_PUSHA   REGS;                   // pushed by int common - 8 32-bit words
    UINT32      Exception_Vector;       // pushed by int stub
    UINT32      Exception_ErrorCode;    // pushed auto by CPU or by int stub
    UINT32      EIP;                    // pushed auto by CPU  (iret instruction starts restore from here)
    UINT32      CS;                     // pushed auto by CPU
    UINT32      EFLAGS;                 // pushed auto by CPU

    UINT32      ESP;                    // pushed auto by CPU : only present when CS = USER
    UINT32      SS;                     // pushed first, auto by CPU : only present when CS = USER
};
#elif K2_TARGET_ARCH_IS_ARM
#define K2OSKERN_ARCH_EXEC_CONTEXT A32_CONTEXT
#else
#error Unknown Arch
#endif

enum _KernIciType
{
    KernIciType_Invalid = 0,
    
    KernIci_Wakeup,
    KernIci_TlbInv,
    KernIci_Panic,
    KernIci_StopProc,
    KernIci_KernThreadTlbInv,

    KernIciType_Count
};

enum _KernCpuCoreEventType
{
    KernCpuCoreEventType_Invalid = 0,

    KernCpuCoreEvent_Thread_SysCall,
    KernCpuCoreEvent_Thread_Exception,
    KernCpuCoreEvent_SchedTimer_Fired,
    KernCpuCoreEvent_Ici,
    KernCpuCoreEvent_Interrupt,

    KernCpuCoreEventType_Count
};

enum _KernTickModeType
{
    KernTickModeType_Invalid = 0,

    KernTickMode_Idle,
    KernTickMode_Kern,
    KernTickMode_Thread,

    KernTickModeType_Count
};

struct _K2OSKERN_CPUCORE_EVENT
{
    KernCpuCoreEventType    mEventType;
    UINT32                  mSrcCoreIx;
    UINT64                  mEventHfTick;
    K2LIST_LINK             ListLink;
};

struct _K2OSKERN_CPUCORE_ICI
{
    K2OSKERN_CPUCORE_EVENT volatile CpuCoreEvent;
    KernIciType                     mIciType;
    void *                          mpArg;
};

struct _K2OSKERN_TLBSHOOT
{
    UINT32 volatile         mCoresRemaining;
    K2OSKERN_OBJ_PROCESS *  mpProc;
    UINT32                  mVirtBase;
    UINT32                  mPageCount;
};

struct _K2OSKERN_CPUCORE
{
    UINT32              mCoreIx;

    BOOL                mIsExecuting;
    BOOL                mIsInMonitor;
    BOOL                mIsTimerRunning;
    BOOL                mIsIdle;

    BOOL                mInDebugMode;
    UINT32              mLockStack;

    BOOL                mNoDpcQueue;
    K2LIST_ANCHOR       DpcHi;
    K2LIST_ANCHOR       DpcMed;
    K2LIST_ANCHOR       DpcLo;

    KernTickModeType    mTickMode;
    UINT64              mModeStartHfTick;
    UINT64              mKernHfTicks;
    UINT64              mIdleHfTicks;

    K2LIST_ANCHOR                       MigratedList;
    K2LIST_ANCHOR                       RunList;
    K2LIST_ANCHOR                       RanList;
    K2OSKERN_OBJ_THREAD * volatile      mpMigratingThreadList;

    K2OSKERN_OBJREF                     MappedProcRef;

    K2OSKERN_OBJ_THREAD * volatile      mpActiveThread; // only set by this CPU. read by anybody

    K2LIST_ANCHOR                       PendingEventList;
    K2OSKERN_CPUCORE_ICI                IciFromOtherCore[K2OS_MAX_CPU_COUNT];

#if K2_TARGET_ARCH_IS_ARM
    UINT32            mActiveIrq;
#endif
};

#define K2OSKERN_COREMEMORY_STACKS_BYTES  ((K2_VA_MEMPAGE_BYTES - sizeof(K2OSKERN_CPUCORE)) + (K2_VA_MEMPAGE_BYTES * 3))

struct _K2OSKERN_COREMEMORY
{
    K2OSKERN_CPUCORE    CpuCore;  /* must be first thing in struct */
    UINT8               mStacks[K2OSKERN_COREMEMORY_STACKS_BYTES];
};
K2_STATIC_ASSERT(sizeof(K2OSKERN_COREMEMORY) == (4 * K2_VA_MEMPAGE_BYTES));

#define K2OSKERN_COREIX_TO_CPUCORE(x)       ((K2OSKERN_CPUCORE volatile *)(K2OS_KVA_COREMEMORY_BASE + ((x) * (K2_VA_MEMPAGE_BYTES * 4))))
#define K2OSKERN_GET_CURRENT_CPUCORE        K2OSKERN_COREIX_TO_CPUCORE(K2OSKERN_GetCpuIndex())

#define K2OSKERN_COREIX_TO_COREMEMORY(x)    ((K2OSKERN_COREMEMORY *)(K2OS_KVA_COREMEMORY_BASE + ((x) * (K2_VA_MEMPAGE_BYTES * 4))))
#define K2OSKERN_GET_CURRENT_COREMEMORY     K2OSKERN_COREIX_TO_COREMEMORY(K2OSKERN_GetCpuIndex())

/* --------------------------------------------------------------------------------- */

typedef struct _K2OSKERN_TIMERITEM K2OSKERN_TIMERITEM;
struct _K2OSKERN_TIMERITEM
{
    BOOL                    mIsMacroWait;
    UINT64                  mHfTicks;
    K2LIST_LINK             GlobalQueueListLink;
};

/* --------------------------------------------------------------------------------- */

typedef enum _KernSchedItemType KernSchedItemType;
enum _KernSchedItemType
{
    KernSchedItemType_Invalid,

    KernSchedItem_Thread_SysCall,
    KernSchedItem_Aborted_Running_Thread,
    KernSchedItem_SchedTimer_Fired,
    KernSchedItem_Thread_ResumeDeferral_Completed,
    KernSchedItem_Thread_Exception,
    KernSchedItem_Alarm_Cleanup,
    KernSchedItem_SemUser_Cleanup,
    KernSchedItem_Interrupt,
    KernSchedItem_NotifyProxy,
    KernSchedItem_ProcStopped,
    KernSchedItem_KernThread_Exit,
    KernSchedItem_KernThread_SemInc,
    KernSchedItem_KernThread_StartProc,
    KernSchedItem_KernThread_Wait,
    KernSchedItem_KernThread_WaitIo,
    KernSchedItem_KernThread_SignalNotify,
    KernSchedItem_KernThread_SetAffinity,
    KernSchedItem_KernThread_MountAlarm,
    KernSchedItem_KernThread_ChangeGate,
    KernSchedItem_KernThread_StartThread,
    KernSchedItem_KernThread_LastTokenDestroyed,
    KernSchedItem_KernThread_MailboxSentFirst,
    KernSchedItem_KernThread_MailboxRecvLast,
    KernSchedItem_KernThread_IfInstPublish,
    KernSchedItem_KernThread_IpcEndManualDisconnect,
    KernSchedItem_KernThread_IpcRejectRequest,
    KernSchedItem_KernThread_IpcAccept,

    KernSchedItemType_Count
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_SEM_INC K2OSKERN_SCHED_ITEM_ARGS_SEM_INC;
struct _K2OSKERN_SCHED_ITEM_ARGS_SEM_INC
{
    UINT32 mCount;
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_GATE_CHANGE K2OSKERN_SCHED_ITEM_ARGS_GATE_CHANGE;
struct _K2OSKERN_SCHED_ITEM_ARGS_GATE_CHANGE
{
    UINT32 mNewState;
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_THREAD_CREATE K2OSKERN_SCHED_ITEM_ARGS_THREAD_CREATE;
struct _K2OSKERN_SCHED_ITEM_ARGS_THREAD_CREATE
{
    UINT32  mThreadToken;
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_PROCESS_CREATE K2OSKERN_SCHED_ITEM_ARGS_PROCESS_CREATE;
struct _K2OSKERN_SCHED_ITEM_ARGS_PROCESS_CREATE
{
    UINT32  mProcessToken;
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_THREAD_SETAFF K2OSKERN_SCHED_ITEM_ARGS_THREAD_SETAFF;
struct _K2OSKERN_SCHED_ITEM_ARGS_THREAD_SETAFF
{
    UINT32  mNewMask;
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_ALARM_CREATE K2OSKERN_SCHED_ITEM_ARGS_ALARM_CREATE;
struct _K2OSKERN_SCHED_ITEM_ARGS_ALARM_CREATE
{
    UINT32 mAlarmToken;
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_IFINST_PUBLISH K2OSKERN_SCHED_ITEM_ARGS_IFINST_PUBLISH;
struct _K2OSKERN_SCHED_ITEM_ARGS_IFINST_PUBLISH
{
    UINT32 mClassCode;
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_IPC_ACCEPT K2OSKERN_SCHED_ITEM_ARGS_IPC_ACCEPT;
struct _K2OSKERN_SCHED_ITEM_ARGS_IPC_ACCEPT
{
    K2OSKERN_OBJREF     RemoteEndRef;
    K2OSKERN_OBJREF     LocalMapOfRemoteBufferRef;
    K2OS_VIRTMAP_TOKEN  mTokLocalMapOfRemote;
    K2OSKERN_OBJREF     RemoteMapOfLocalBufferRef;
    K2OS_VIRTMAP_TOKEN  mTokRemoteMapOfLocal;
};

typedef struct _K2OSKERN_SCHED_ITEM_ARGS_IPC_REJECT K2OSKERN_SCHED_ITEM_ARGS_IPC_REJECT;
struct _K2OSKERN_SCHED_ITEM_ARGS_IPC_REJECT
{
    UINT32    mRequestId;
    UINT32    mReasonCode;
};

typedef union _K2OSKERN_SCHED_ITEM_ARGS K2OSKERN_SCHED_ITEM_ARGS;
union _K2OSKERN_SCHED_ITEM_ARGS
{
    K2OSKERN_SCHED_ITEM_ARGS_GATE_CHANGE    Gate_Change;
    K2OSKERN_SCHED_ITEM_ARGS_THREAD_CREATE  Thread_Create;
    K2OSKERN_SCHED_ITEM_ARGS_PROCESS_CREATE Process_Create;
    K2OSKERN_SCHED_ITEM_ARGS_THREAD_SETAFF  Thread_SetAff;
    K2OSKERN_SCHED_ITEM_ARGS_ALARM_CREATE   Alarm_Create;
    K2OSKERN_SCHED_ITEM_ARGS_SEM_INC        Sem_Inc;
    K2OSKERN_SCHED_ITEM_ARGS_IFINST_PUBLISH IfInst_Publish;
    K2OSKERN_SCHED_ITEM_ARGS_IPC_ACCEPT     Ipc_Accept;
    K2OSKERN_SCHED_ITEM_ARGS_IPC_REJECT     Ipc_Reject;
};

typedef struct _K2OSKERN_SCHED_ITEM K2OSKERN_SCHED_ITEM;
struct _K2OSKERN_SCHED_ITEM
{
    KernSchedItemType               mSchedItemType;
    K2OSKERN_OBJREF                 ObjRef;
    UINT64                          mHfTick;
    K2OSKERN_SCHED_ITEM_ARGS        Args;
    union
    {
        K2OSKERN_SCHED_ITEM * volatile  mpNextItem;
        K2LIST_LINK                     ListLink;
    };
};

/* --------------------------------------------------------------------------------- */

typedef struct _K2OSKERN_PROCPAGELIST       K2OSKERN_PROCPAGELIST;
typedef struct _K2OSKERN_TOKEN_INUSE        K2OSKERN_TOKEN_INUSE;
typedef union  _K2OSKERN_TOKEN              K2OSKERN_TOKEN;
typedef struct _K2OSKERN_TOKEN_PAGE_HDR     K2OSKERN_TOKEN_PAGE_HDR;
typedef union  _K2OSKERN_TOKEN_PAGE         K2OSKERN_TOKEN_PAGE;
typedef struct _K2OSKERN_PROCHEAP_NODE      K2OSKERN_PROCHEAP_NODE;
typedef struct _K2OSKERN_PROCHEAP_NODEBLOCK K2OSKERN_PROCHEAP_NODEBLOCK;
typedef struct _K2OSKERN_PROCVIRT           K2OSKERN_PROCVIRT;
typedef struct _K2OSKERN_PROCPAGELIST       K2OSKERN_PROCPAGELIST;
typedef struct _K2OSKERN_PROCTOKEN          K2OSKERN_PROCTOKEN;
typedef struct _K2OSKERN_PROCTHREAD         K2OSKERN_PROCTHREAD;
typedef enum   _KernProcStateType           KernProcStateType;
typedef struct _K2OSKERN_KERN_TOKEN         K2OSKERN_KERN_TOKEN;
typedef struct _K2OSKERN_VIRTHEAP_NODE      K2OSKERN_VIRTHEAP_NODE;
typedef struct _K2OSKERN_PROCIPCEND         K2OSKERN_PROCIPCEND;
typedef struct _K2OSKERN_PROCMBOXOWNER      K2OSKERN_PROCMBOXOWNER;

struct _K2OSKERN_KERN_TOKEN
{
    UINT32              mSentinel1;
    K2OSKERN_OBJREF     Ref;
    UINT32              mSentinel2;
};

struct _K2OSKERN_TOKEN_INUSE
{
    K2OSKERN_OBJREF         Ref;
    UINT32                  mTokValue;
};

union _K2OSKERN_TOKEN
{
    K2OSKERN_TOKEN_INUSE    InUse;
    K2LIST_LINK             FreeListLink;
};

K2_STATIC_ASSERT(sizeof(K2OSKERN_TOKEN) >= sizeof(K2LIST_LINK));
#define K2OSKERN_TOKENS_PER_PAGE   (K2_VA_MEMPAGE_BYTES / sizeof(K2OSKERN_TOKEN))

struct _K2OSKERN_TOKEN_PAGE_HDR
{
    UINT32 mPageIndex;     // index of this page in the process sparse token page array
    UINT32 mInUseCount;    // count of tokens in this page that are in use
};

union _K2OSKERN_TOKEN_PAGE
{
    K2OSKERN_TOKEN_PAGE_HDR    Hdr;
    K2OSKERN_TOKEN             Tokens[K2OSKERN_TOKENS_PER_PAGE];
};

#define K2OSKERN_TOKEN_KERNEL_BIT       0x80000000
#define K2OSKERN_TOKEN_SALT_MASK        0x7FF00000
#define K2OSKERN_TOKEN_SALT_DEC         0x00100000
#define K2OSKERN_TOKEN_MAX_VALUE        0x000FFFFF
#define K2OSKERN_TOKEN_VALID_BIT        0x00000001

#define K2OSKERN_MAX_TOKENS_PER_PROC   ((K2OSKERN_TOKEN_MAX_VALUE + 1) / sizeof(K2OSKERN_TOKENS_PER_PAGE))

K2_STATIC_ASSERT(sizeof(K2OSKERN_TOKEN_PAGE) <= K2_VA_MEMPAGE_BYTES);

struct _K2OSKERN_PROCHEAP_NODE
{
    K2HEAP_NODE HeapNode;
    UINT8       mIxInBlock;
    UINT8       mUserOwned;
    UINT16      mMapCount;      // cannot be freed by user if this is nonzero
};

struct _K2OSKERN_PROCHEAP_NODEBLOCK
{
    K2LIST_LINK             BlockListLink;
    UINT64                  mUseMask;
    K2OSKERN_PROCHEAP_NODE  Node[64];
};

struct _K2OSKERN_PROCVIRT
{
    K2OSKERN_PHYSTRACK *                mpPageDirTrack;

    K2OSKERN_SEQLOCK                    SeqLock;
    struct
    {
        UINT32                          mTopPt;
        UINT32                          mBotPt;

        K2HEAP_ANCHOR                   HeapAnchor;
        K2LIST_ANCHOR                   BlockList;
        K2LIST_ANCHOR                   FreeNodeList;
        K2OSKERN_PROCHEAP_NODEBLOCK *   mpBlockToFree;

        K2TREE_ANCHOR                   MapTree;
    } Locked;
};

struct _K2OSKERN_PROCPAGELIST
{
    K2OSKERN_SEQLOCK    SeqLock;
    struct
    {
        K2LIST_ANCHOR       Reserved_Dirty;
        K2LIST_ANCHOR       Reserved_Clean;
        K2LIST_ANCHOR       PageTables;
        K2LIST_ANCHOR       ThreadPages;
        K2LIST_ANCHOR       Working;
        K2LIST_ANCHOR       Tokens;
    } Locked;
};

struct _K2OSKERN_PROCTOKEN
{
    K2OSKERN_SEQLOCK        SeqLock;
    struct
    {
        K2OSKERN_TOKEN_PAGE **  mppPages;
        UINT32                  mPageCount;
        K2LIST_ANCHOR           FreeList;
        UINT32                  mSalt;
        UINT32                  mCount;
    } Locked;
};

struct _K2OSKERN_PROCTHREAD
{
    K2OSKERN_SEQLOCK        SeqLock;
    struct
    {
        // must hold seqlock to move threads onto or off of these lists
        K2LIST_ANCHOR           CreateList;
        K2LIST_ANCHOR           DeadList;
    } Locked;
    struct
    {
        // must hold seqlock moving threads onto or off of this list
        K2LIST_ANCHOR           ActiveList;
    } SchedLocked;
};

struct _K2OSKERN_PROCIPCEND
{
    K2OSKERN_SEQLOCK    SeqLock;
    struct
    {
        K2LIST_ANCHOR   List;
    } Locked;
};

struct _K2OSKERN_PROCMBOXOWNER
{
    K2OSKERN_SEQLOCK    SeqLock;
    struct
    {
        K2LIST_ANCHOR   List;
    } Locked;
};

enum _KernProcStateType
{
    KernProcStateType_Invalid,

    KernProcState_InRawCreate,
    KernProcState_InBuild,
    KernProcState_Launching,
    KernProcState_Starting,
    KernProcState_Running,

    KernProcState_Stopping,
    KernProcState_Stopped,      // order important

    KernProcStateType_Count
};

typedef enum _KernUserCrtSegType KernUserCrtSegType;
enum _KernUserCrtSegType
{
    KernUserCrtSeg_XdlHeader = 0,
    KernUserCrtSeg_Text,
    KernUserCrtSeg_Read,
    KernUserCrtSeg_Data,
    KernUserCrtSeg_Sym,

    KernUserCrtSegType_Count
};

struct _K2OSKERN_OBJ_PROCESS
{
    K2OSKERN_OBJ_HEADER             Hdr;

    UINT32                          mId;
    KernProcStateType               mState;
    K2LIST_LINK                     GlobalProcListLink;

    K2OS_PROC_START_INFO *          mpLaunchInfo;

    UINT32                          mVirtMapKVA;
    UINT32                          mVirtTransBase;
    UINT32                          mPhysTransBase;

    K2OSKERN_OBJREF                 XdlTrackPageArrayRef;
    K2OSKERN_OBJREF                 XdlTrackMapRef;

    K2OSKERN_OBJREF                 PublicApiMapRef;
    K2OSKERN_OBJREF                 TimerPageMapRef;
    K2OSKERN_OBJREF                 FileSysMapRef;
    K2OSKERN_OBJREF                 CrtMapRef[KernUserCrtSegType_Count];

    K2OSKERN_OBJREF                 RefCrtDataPageArray;

    K2LIST_ANCHOR *                 mpUserXdlList;

    K2OSKERN_PROCVIRT               Virt;
    K2OSKERN_PROCPAGELIST           PageList;
    K2OSKERN_PROCTOKEN              Token;
    K2OSKERN_PROCTHREAD             Thread;
    K2OSKERN_PROCIPCEND             IpcEnd;
    K2OSKERN_PROCMBOXOWNER          MboxOwner;

    K2OSKERN_TLBSHOOT               ProcStoppedTlbShoot;
    UINT32                          mIciSendMask;
    UINT32 volatile                 mStopCoresRemaining;
    UINT32                          mExitCode;
    K2OSKERN_SCHED_ITEM             StoppedSchedItem;
    K2OSKERN_OBJREF                 StoppingRef;
    BOOL                            mStopDpcRan;

    struct {
        K2LIST_ANCHOR               MacroWaitEntryList;
    } SchedLocked;

    UINT64 volatile                 mTlsBitmap;
};

/* --------------------------------------------------------------------------------- */

typedef struct _K2OSKERN_IRQ K2OSKERN_IRQ;
struct _K2OSKERN_IRQ
{
    K2OS_IRQ_CONFIG         Config;

    K2LIST_ANCHOR           InterruptList;

    K2TREE_NODE             IrqTreeNode;
};

struct _K2OSKERN_OBJ_INTERRUPT
{
    K2OSKERN_OBJ_HEADER             Hdr;

    K2OSKERN_IRQ *                  mpParentIrq;

    K2OSKERN_OBJREF                 GateRef;        // gate is pulsed on interrupt

    K2OSKERN_CPUCORE_EVENT volatile Event;

    BOOL                            mVoteForEnabled;
    BOOL                            mInService;

    K2OSKERN_SCHED_ITEM             SchedItem;

    K2LIST_LINK                     IrqInterruptListLink;

    K2OSKERN_pf_Hook_Key *          mpHookKey;
};

/* --------------------------------------------------------------------------------- */

typedef enum _KernThreadStateType       KernThreadStateType;
typedef enum _KernExProgress            KernExProgress;
typedef struct _K2OSKERN_THREAD_EX      K2OSKERN_THREAD_EX;
typedef struct _K2OSKERN_WAITENTRY      K2OSKERN_WAITENTRY;
typedef struct _K2OSKERN_MACROWAIT      K2OSKERN_MACROWAIT;
typedef struct _K2OSKERN_USER_THREAD    K2OSKERN_USER_THREAD;
typedef struct _K2OSKERN_KERN_THREAD    K2OSKERN_KERN_THREAD;
typedef struct _K2OSKERN_KERN_THREAD_IO K2OSKERN_KERN_THREAD_IO;

struct _K2OSKERN_WAITENTRY
{
    UINT16              mMacroIndex;        // index into K2OSKERN_MACROWAIT.WaitEntry[]
    UINT16              mMounted;
    K2LIST_LINK         ObjWaitListLink;
    K2OSKERN_OBJREF     ObjRef;
};

struct _K2OSKERN_MACROWAIT
{
    K2OSKERN_OBJ_THREAD *   mpWaitingThread;    /* backpointer to thread that is using this macrowait */
    UINT16                  mNumEntries;
    UINT8                   mTimerActive;
    UINT8                   mIsWaitAll;
    UINT32                  mWaitResult;
    K2OSKERN_TIMERITEM      TimerItem;          /* mIsMacroWait is TRUE */
    K2OSKERN_WAITENTRY      WaitEntry[K2OS_THREAD_WAIT_MAX_ITEMS];
};

typedef void (*KernUser_pf_SysCall)(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);

enum _KernThreadStateType
{
    KernThreadStateType_Invalid = 0,

    KernThreadState_Init,
    KernThreadState_InitNoPrep,
    KernThreadState_Created,
    KernThreadState_Migrating,
    KernThreadState_OnCpuLists,
    KernThreadState_Running,
    KernThreadState_InPaging,
    KernThreadState_InException,
    KernThreadState_Waiting,
    KernThreadState_WaitIo,
    KernThreadState_InScheduler,
    KernThreadState_InScheduler_ResumeDeferred,
    KernThreadState_Debug_Crashed,
    KernThreadState_InIo,
    KernThreadState_Exited,

    KernThreadStateType_Count
};

struct _K2OSKERN_THREAD_EX
{
    UINT32              mFaultAddr;
    K2STAT              mExCode;
    BOOL                mPageWasPresent;
    BOOL                mWasWrite;
    K2OSKERN_OBJREF     VirtMapRef;         // map containing fault address
    UINT32              mMapPageIx;
    UINT32              mIciSendMask;
    UINT32              mArchSpec[2];
};

struct _K2OSKERN_USER_THREAD
{
    UINT32                      mSysCall_Id;
    UINT32                      mSysCall_Arg0;
    UINT32                      mSysCall_Result;

    BOOL                        mIsInSysCall;

    K2OSKERN_ARCH_EXEC_CONTEXT  ArchExecContext;

    struct {
        K2LIST_LINK             ProcIoListLink;
        K2OSKERN_OBJREF         RefCurUserMap;
        UINT32                  mKernMapVirtAddr;
        K2OS_VIRTMAP_TOKEN      mTokCurKernMap;
    } Io;
};

struct _K2OSKERN_KERN_THREAD_IO
{
    K2OSKERN_OBJREF             NotifyRef;
    K2OSKERN_KERN_THREAD_IO *   mpChainNext;
};

struct _K2OSKERN_KERN_THREAD
{
    UINT32                  mStackPtr;

    void *                  mpArg;
    K2OSKERN_OBJREF         ResultRef;

    UINT32                  mSchedCall_Result;

    K2OS_XDL                mXdlRef;

    K2OSKERN_KERN_THREAD_IO Io;
    K2LIST_LINK             IoClusterWaitListLink;

    K2LIST_ANCHOR           HeldCsList;
#if K2_TARGET_ARCH_IS_ARM
    K2OSKERN_ARCH_EXEC_CONTEXT  ArchExecContext;
#endif
};

struct _K2OSKERN_OBJ_THREAD
{
    K2OSKERN_OBJ_HEADER             Hdr;

    BOOL                            mIsKernelThread;

    K2OSKERN_OBJREF                 RefProc;

    K2OSKERN_OBJREF                 StackMapRef;

    UINT32                          mGlobalIx;
    UINT32                          mExitCode;
    char                            mName[K2OS_THREAD_NAME_BUFFER_CHARS];

    K2OSKERN_THREAD_EX              LastEx;

    K2LIST_LINK                     OwnerThreadListLink;
    
    struct
    {
        UINT32 volatile     mIngressNext;

        K2TREE_NODE         PageAddrTreeNode;   // Invalid unless Link.Link.mpPrev is NULL

        union
        {
            K2LIST_ANCHOR   Anchor;             // Valid if List.Link.mpPrev is NULL
            K2LIST_LINK     Link;               // Always Valid
        } List;
    } Paging;

    UINT32                          mThreadPagePhys;
    K2OS_THREAD_PAGE *              mpKernRwViewOfThreadPage;

    K2OSKERN_CPUCORE_EVENT volatile CpuCoreEvent;

    K2OSKERN_CPUCORE volatile *     mpLastRunCore;
    K2OSKERN_OBJ_THREAD *           mpMigratingNext;

    KernThreadStateType             mState;

    K2OSKERN_OBJREF                 Running_SelfRef;

    K2OS_THREAD_CONFIG              Config;
    K2OSKERN_TLBSHOOT               TlbShoot;
    UINT32                          mIciSendMask;

    K2OSKERN_SCHED_ITEM             SchedItem;
    union
    {
        K2LIST_LINK                 SchedListLink;
        K2LIST_LINK                 CpuCoreThreadListLink;
    };

    K2OSKERN_MACROWAIT              MacroWait;

    struct {
        K2LIST_ANCHOR               MacroWaitEntryList;
    } SchedLocked;

    UINT64                          mHfTicks;
    UINT64                          mQuantumHfTicksRemaining;

    union
    {
        K2OSKERN_USER_THREAD        User;
        K2OSKERN_KERN_THREAD        Kern;
    };
};

/* --------------------------------------------------------------------------------- */

#define K2OSKERN_PAGEARRAY_PAGE_PHYSADDR_MASK       K2_VA_PAGEFRAME_MASK
#define K2OSKERN_PAGEARRAY_PAGE_ATTR_MASK           K2_VA_MEMPAGE_OFFSET_MASK
#define K2OSKERN_PAGEARRAY_PAGE_ATTR_MAPCOUNT_MASK  0x000007FF
#define K2OSKERN_PAGEARRAY_PAGE_ATTR_EXTRA          0x00000800

typedef enum _KernPageArrayType KernPageArrayType;
enum _KernPageArrayType
{
    KernPageArrayType_Invalid = 0,

    KernPageArray_Track,
    KernPageArray_PreMap,
    KernPageArray_Sparse,
    KernPageArray_Spec,

    KernPageArrayType_Count
};

typedef struct _K2OSKERN_PAGEARRAY_TRACK K2OSKERN_PAGEARRAY_TRACK;
struct _K2OSKERN_PAGEARRAY_TRACK
{
    K2OSKERN_PHYSTRACK *    mpTrack;
};

typedef struct _K2OSKERN_PAGEARRAY_PREMAP K2OSKERN_PAGEARRAY_PREMAP;
struct _K2OSKERN_PAGEARRAY_PREMAP
{
    UINT32 *                mpPTEs;
};

typedef struct _K2OSKERN_PAGEARRAY_SPARSE K2OSKERN_PAGEARRAY_SPARSE;
struct _K2OSKERN_PAGEARRAY_SPARSE
{
    K2LIST_ANCHOR   TrackList;
    UINT32          mPages[1];
};

typedef struct _K2OSKERN_PAGEARRAY_SPEC K2OSKERN_PAGEARRAY_SPEC;
struct _K2OSKERN_PAGEARRAY_SPEC
{
    UINT32                  mBasePhys;
};

struct _K2OSKERN_OBJ_PAGEARRAY
{
    K2OSKERN_OBJ_HEADER     Hdr;
    KernPageArrayType       mPageArrayType;
    UINT32                  mUserPermit;        // PF_W, PF_R, K2OS_MEMPAGE_COPY_ON_WRITE
    UINT32                  mPageCount;

    // must be last thing in range object
    union
    {
        K2OSKERN_PAGEARRAY_TRACK    Track;
        K2OSKERN_PAGEARRAY_PREMAP   PreMap;
        K2OSKERN_PAGEARRAY_SPARSE   Sparse;
        K2OSKERN_PAGEARRAY_SPEC     Spec;
    } Data;
};

/* --------------------------------------------------------------------------------- */

struct _K2OSKERN_OBJ_NOTIFY
{
    K2OSKERN_OBJ_HEADER     Hdr;
    struct
    {
        BOOL                mSignalled;
        K2LIST_ANCHOR       MacroWaitEntryList;
    } SchedLocked;
};

/* --------------------------------------------------------------------------------- */

struct _K2OSKERN_OBJ_GATE
{
    K2OSKERN_OBJ_HEADER     Hdr;
    struct
    {
        BOOL                mOpen;
        K2LIST_ANCHOR       MacroWaitEntryList;
    } SchedLocked;
};

/* --------------------------------------------------------------------------------- */

struct _K2OSKERN_OBJ_ALARM
{
    K2OSKERN_OBJ_HEADER     Hdr;
    struct
    {
        UINT64              mHfTicks;
        BOOL                mIsPeriodic;
        K2LIST_ANCHOR       MacroWaitEntryList;
        BOOL                mTimerActive;
        K2OSKERN_TIMERITEM  TimerItem;          /* mIsMacroWait is FALSE */
    } SchedLocked;

    K2OSKERN_SCHED_ITEM     CleanupSchedItem;
};

/* --------------------------------------------------------------------------------- */

struct _K2OSKERN_OBJ_SEM
{
    K2OSKERN_OBJ_HEADER     Hdr;
    UINT32                mMaxCount;
    struct
    {
        UINT32            mCount;
        K2LIST_ANCHOR       MacroWaitEntryList;
    } SchedLocked;
};

struct _K2OSKERN_OBJ_SEMUSER
{
    K2OSKERN_OBJ_HEADER     Hdr;

    K2OSKERN_OBJREF         SemRef;

    K2OSKERN_SCHED_ITEM     CleanupSchedItem;

    struct {
        UINT32            mHeldCount;
    } SchedLocked;
};

/* --------------------------------------------------------------------------------- */

typedef struct _K2OSKERN_XDL_TRACK K2OSKERN_XDL_TRACK;
struct _K2OSKERN_XDL_TRACK
{
    XDL *                   mpXdl;
    char const *            mpName; // points into header mName field

    K2OSKERN_OBJREF         SegVirtMapRef[XDLSegmentIx_Count];
    K2LIST_LINK             KernLoadedListLink;
};

typedef struct _K2OSKERN_HOST_FILE K2OSKERN_HOST_FILE;
struct _K2OSKERN_HOST_FILE
{
    BOOL                mKeepSymbols;
    K2_GUID128          ID;
    K2OSKERN_OBJREF     RefXdlPageVirtMap;
    K2OSKERN_XDL_TRACK  Track;
    K2OS_FILE           mFile;
};

typedef enum _KernSegType KernSegType;
enum _KernSegType
{
    KernSegType_Invalid = 0,

    KernSeg_PreMap,
    KernSeg_Xdl_Page,
    KernSeg_Xdl_Part,
    KernSeg_Thread_Stack,
    KernSeg_Dyn,

    KernSegType_Count
};

typedef struct _K2OSKERN_KERNMAP_XDL_PART K2OSKERN_KERNMAP_XDL_PART;
struct _K2OSKERN_KERNMAP_XDL_PART
{
    XDLSegmentIx            mXdlSegmentIx;
    K2OSKERN_XDL_TRACK *    mpOwnerTrack;
};

typedef struct _K2OSKERN_KERNMAP_XDL_PAGE K2OSKERN_KERNMAP_XDL_PAGE;
struct _K2OSKERN_KERNMAP_XDL_PAGE
{
    K2XDL_HOST_FILE         mOwnerHostFile;
};

typedef struct _K2OSKERN_KERNMAP_THREAD_STACK K2OSKERN_KERNMAP_THREAD_STACK;
struct _K2OSKERN_KERNMAP_THREAD_STACK
{
    K2OSKERN_OBJ_THREAD *   mpThread;
};

typedef struct _K2OSKERN_KERN_MAP K2OSKERN_KERN_MAP;
struct _K2OSKERN_KERN_MAP
{
    KernSegType                 mSegType;
    UINT32                      mSizeBytes;
    union {
        K2OSKERN_KERNMAP_XDL_PART       XdlPart;
        K2OSKERN_KERNMAP_XDL_PAGE       XdlPage;
        K2OSKERN_KERNMAP_THREAD_STACK   ThreadStack;
    };
};

typedef struct _K2OSKERN_USER_MAP K2OSKERN_USER_MAP;
struct _K2OSKERN_USER_MAP
{
    K2OSKERN_PROCHEAP_NODE *    mpProcHeapNode;
};

struct _K2OSKERN_OBJ_VIRTMAP
{
    K2OSKERN_OBJ_HEADER     Hdr;

    K2OSKERN_OBJREF         PageArrayRef;
    UINT32                  mPageArrayStartPageIx;

    UINT32                  mPageCount;

    BOOL                    mIsDemandPaged;

    K2OS_VirtToPhys_MapType mVirtToPhysMapType;

    K2OSKERN_TLBSHOOT       TlbShoot;
    UINT32                  mIciSendMask;

    UINT32 *                mpPte;

    K2TREE_NODE             OwnerMapTreeNode;   // mUserVal is virtual base address

    K2OSKERN_OBJREF         ProcRef;            // if NULL this belongs to the kernel

    union {
        K2OSKERN_USER_MAP   User;
        K2OSKERN_KERN_MAP   Kern;
    };
};

/* --------------------------------------------------------------------------------- */

// kernel view of consumer page (there's more stuff!)
typedef struct _K2OSKERN_MAILBOX_CONSUMER_PAGE K2OSKERN_MAILBOX_CONSUMER_PAGE;
struct _K2OSKERN_MAILBOX_CONSUMER_PAGE
{
    K2OS_MAILBOX_CONSUMER_PAGE  UserVisible;
    K2OSKERN_OBJREF             ReserveHolder[K2OS_MAILBOX_MSG_COUNT];
};
K2_STATIC_ASSERT(sizeof(K2OSKERN_MAILBOX_CONSUMER_PAGE) <= K2_VA_MEMPAGE_BYTES);

struct _K2OSKERN_OBJ_MAILBOX
{
    K2OSKERN_OBJ_HEADER     Hdr;

    K2OSKERN_OBJREF         RefPageArray;
    UINT32                  mKernVirtAddr;

    BOOL                    mOwnerDead;

    K2OSKERN_OBJREF         RefGate;

    K2OSKERN_TLBSHOOT       PurgeTlbShoot;
    UINT32                  mIciSendMask;
};

struct _K2OSKERN_OBJ_MAILBOXOWNER
{
    K2OSKERN_OBJ_HEADER     Hdr;
    K2OSKERN_OBJREF         RefMailbox;
    K2OSKERN_OBJREF         RefProc;
    UINT32                  mProcVirt;
    K2OSKERN_OBJREF         RefProcMap;
    struct
    {
        K2LIST_LINK         ListLink;
    } OwnerMboxOwnerListLocked;
};

struct _K2OSKERN_OBJ_MAILSLOT
{
    K2OSKERN_OBJ_HEADER     Hdr;
    K2OSKERN_OBJREF         RefMailbox;
    K2OSKERN_OBJREF         RefProc;
    UINT32                  mProcVirt;
    K2OSKERN_OBJREF         RefProcMap[2];
};

/* --------------------------------------------------------------------------------- */

struct _K2OSKERN_OBJ_IFINST
{
    K2OSKERN_OBJ_HEADER     Hdr;

    K2OSKERN_OBJREF         RefProc;            // here because we don't neccessarily have a mailbox

    K2OSKERN_OBJREF         RefMailboxOwner;    // mailbox refproc will be the same as refproc above

    K2OSKERN_OBJREF         RefChild;           // child kernel object owned by this interface (not RPC). usually NULL

    BOOL                    mIsPublic;
    BOOL                    mIsDeparting;

    K2TREE_NODE             IdTreeNode;         // mUserVal is IFINST_ID
    UINT32                  mClassCode;

    UINT32 volatile         mContext;           // changed by atomic op in fast system call
    K2_GUID128              SpecificGuid;
};

struct _K2OSKERN_OBJ_IFENUM
{
    K2OSKERN_OBJ_HEADER     Hdr;

    UINT32                  mEntryCount;
    K2OS_IFINST_DETAIL *    mpEntries;
    K2OSKERN_SEQLOCK        SeqLock;
    struct {
        UINT32              mCurrentIx;
    } Locked;
};

struct _K2OSKERN_OBJ_IFSUBS
{
    K2OSKERN_OBJ_HEADER     Hdr;

    K2OSKERN_OBJREF         MailboxRef;
    BOOL                    mProcSelfNotify;

    UINT32                  mClassCode;
    BOOL                    mHasSpecific;
    K2_GUID128              Specific;

    INT32                   mBacklogInit;
    INT32 volatile          mBacklogRemaining;

    UINT32                  mUserContext;

    K2LIST_LINK             ListLink;
};

/* --------------------------------------------------------------------------------- */

typedef enum _KernIpcEndStateType       KernIpcEndStateType;
typedef struct _K2OSKERN_IPC_REQUEST    K2OSKERN_IPC_REQUEST;

enum _KernIpcEndStateType
{
    KernIpcEndStateType_Invalid,

    // during create, holding 4 mailbox reserve - create, connect, disconnect, delete
    KernIpcEndState_Disconnected,               // holding 3 mailbox reserve - connect, disconnect, delete
    KernIpcEndState_Connected,                  // holding 2 mailbox reserve - disconnect, delete
    KernIpcEndState_WaitDisAck,                 // holding 1 mailbox reserve - delete

    KernIpcEndStateType_Count
};

struct _K2OSKERN_IPC_REQUEST
{
    K2OS_IFINST_ID          mIfInstId;
    K2OSKERN_OBJ_IPCEND *   mpRequestor;
    K2TREE_NODE             TreeNode;
    UINT32                  mPending;
};

struct _K2OSKERN_OBJ_IPCEND
{
    K2OSKERN_OBJ_HEADER     Hdr;

    void *                  mpUserKey;

    K2OSKERN_OBJREF         MailboxOwnerRef;

    K2OSKERN_OBJREF         VirtMapRef; 

    UINT32                  mMaxMsgBytes;
    UINT32                  mMaxMsgCount;

    struct
    {
        K2OSKERN_SEQLOCK    SeqLock;
        struct
        {
            K2OSKERN_IPC_REQUEST *  mpOpenRequest;

            KernIpcEndStateType     mState;
            struct
            {
                K2OSKERN_OBJREF         PartnerIpcEndRef;
                K2OSKERN_OBJREF         WriteMapRef;
                BOOL                    mTookPartnerReserve;
            } Connected;
        } Locked;
    } Connection;

    struct
    {
        K2LIST_LINK         ListLink;
    } OwnerIpcEndListLocked;
};

/* --------------------------------------------------------------------------------- */

struct _K2OSKERN_OBJ_NOTIFYPROXY
{
    K2OSKERN_OBJ_HEADER     Hdr;
    K2OSKERN_OBJREF         RefNotify;
    K2OSKERN_SCHED_ITEM     SchedItem;
    K2OSKERN_OBJREF         RefSelf;
};

/* --------------------------------------------------------------------------------- */

typedef struct _K2OSKERN_DBG_COMMAND K2OSKERN_DBG_COMMAND;
struct _K2OSKERN_DBG_COMMAND
{
    UINT32          mCommandId;
    UINT32 volatile mProcessedMask;
};

/* --------------------------------------------------------------------------------- */

struct _K2OSKERN_VIRTHEAP_NODE
{
    K2HEAP_NODE     HeapNode;
    INT32 volatile  mRefs;     // will remain in virtual heap until this hits zero
};

/* --------------------------------------------------------------------------------- */

typedef struct _KERN_DATA           KERN_DATA;
typedef struct _KERN_DATA_DEBUG     KERN_DATA_DEBUG;
typedef struct _KERN_DATA_TIMER     KERN_DATA_TIMER;
typedef struct _KERN_DATA_PHYS      KERN_DATA_PHYS;
typedef struct _KERN_DATA_VIRT      KERN_DATA_VIRT;
typedef struct _KERN_DATA_OBJ       KERN_DATA_OBJ;
typedef struct _KERN_DATA_PROC      KERN_DATA_PROC;
typedef struct _KERN_DATA_BUILTIN   KERN_DATA_BUILTIN;
typedef struct _KERN_DATA_FILESYS   KERN_DATA_FILESYS;
typedef struct _KERN_DATA_SCHED     KERN_DATA_SCHED;
typedef struct _KERN_DATA_USER      KERN_DATA_USER;
typedef struct _KERN_DATA_XDL       KERN_DATA_XDL;
typedef struct _KERN_DATA_VIRTMAP   KERN_DATA_VIRTMAP;
typedef struct _KERN_DATA_INTR      KERN_DATA_INTR;
typedef struct _KERN_DATA_PLAT      KERN_DATA_PLAT;
typedef struct _KERN_DATA_BOOTGRAF  KERN_DATA_BOOTGRAF;
typedef struct _KERN_DATA_THREAD    KERN_DATA_THREAD;
typedef struct _KERN_DATA_TOKEN     KERN_DATA_TOKEN;
typedef struct _KERN_DATA_EXEC      KERN_DATA_EXEC;
typedef struct _KERN_DATA_IFACE     KERN_DATA_IFACE;
typedef struct _KERN_DATA_IPCEND    KERN_DATA_IPCEND;
typedef struct _KERN_DATA_MBOXOWNER KERN_DATA_MBOXOWNER;
typedef struct _KERN_DATA_SYSPROC   KERN_DATA_SYSPROC;
typedef struct _KERN_DATA_RPC       KERN_DATA_RPC;
typedef struct _KERN_DATA_PAGING    KERN_DATA_PAGING;
typedef struct _KERN_DATA_FIRMWARE  KERN_DATA_FIRMWARE;

struct _KERN_DATA_DEBUG
{
    K2OSKERN_SEQLOCK        SeqLock;
    UINT32                  mLeadPanicCore;
    INT32 volatile          mCoresInPanicSpin;
    INT32 volatile          mCoresInDebugMode;

    K2OSKERN_DPC_SIMPLE     EnterDpc;
    K2OSKERN_DBG_COMMAND    Command;
    UINT32                  mIciSendMask;
    UINT32                  mLeadCoreIx;
};

struct _KERN_DATA_TIMER
{
    UINT32                          mFreq;
    UINT32                          mIoPhys;
    K2OSKERN_CPUCORE_EVENT volatile SchedCpuEvent;
};

struct _KERN_DATA_XDL
{
    K2OS_CRITSEC        Sec;

    K2OS_FSCLIENT       mFsClient;

    K2OSKERN_SEQLOCK    KernLoadedListSeqLock;
    K2LIST_ANCHOR       KernLoadedList;

    K2XDL_HOST          Host;

    K2OSKERN_XDL_TRACK  TrackCrt;

    XDL *               mpPlat;
    K2OSKERN_XDL_TRACK  TrackPlat;

    XDL *               mpKern;
    K2OSKERN_XDL_TRACK  TrackKern;

    XDL *               mpAcpi;
    K2OSKERN_XDL_TRACK  TrackAcpi;

    XDL *               mpExec;
    K2OSKERN_XDL_TRACK  TrackExec;
};

struct _KERN_DATA_VIRTMAP
{
    K2OSKERN_SEQLOCK    SeqLock;
    K2TREE_ANCHOR       Tree;
};

struct _KERN_DATA_PHYS
{
    UINT32 volatile     mPagesLeft;
    K2OSKERN_SEQLOCK    SeqLock;
    K2LIST_ANCHOR       PageList[KernPhysPageList_Count];
};

struct _KERN_DATA_VIRT
{
    UINT32                  mTopPt;     // top-down address of where first page after last free pagetable ENDS
    UINT32                  mBotPt;     // bottom-up address of where first byte after last free pagetable STARTS

    K2HEAP_ANCHOR           Heap;
    K2LIST_ANCHOR           HeapFreeNodeList;
    K2OSKERN_SEQLOCK        HeapSeqLock;
    K2OSKERN_VIRTHEAP_NODE  StockNode[2];

    K2RAMHEAP               RamHeap;
    K2OSKERN_SEQLOCK        RamHeapSeqLock;
    K2OSKERN_VIRTHEAP_NODE *mpRamHeapHunk;

    K2LIST_ANCHOR           PhysTrackList;

    BOOL                    mKernHeapThreaded;
};

struct _KERN_DATA_OBJ
{
    K2OSKERN_SEQLOCK    SeqLock;
    K2LIST_ANCHOR       Cache[KernObjType_Count];
};

struct _KERN_DATA_PROC
{
    INT32 volatile      mNextId;
    INT32 volatile      mExistCount;
    K2OSKERN_SEQLOCK    SeqLock;
    K2LIST_ANCHOR       List;
    UINT32 volatile     mNextThreadSlotIx;
    UINT32              mLastThreadSlotIx;
};

struct _KERN_DATA_BUILTIN
{
    K2ROFS const *      mpRofs;
    K2OSKERN_OBJREF     RefRofsVirtMap;
};

struct _KERN_DATA_FILESYS
{
    K2OSKERN_FSNODE RootFsNode;
    K2OSKERN_FSNODE FsRootFsNode;
};

struct _KERN_DATA_SCHED
{
    UINT32 volatile                 mReq;
    K2OSKERN_CPUCORE volatile *     mpSchedulingCore;
    K2OSKERN_SCHED_ITEM * volatile  mpPendingItemListHead;
    K2OSKERN_SCHED_ITEM             TimerSchedItem;

    INT32 volatile                  mCoreThreadCount[K2OS_MAX_CPU_COUNT];

    K2OSKERN_SEQLOCK                SeqLock;
    struct
    {
        UINT64                      mLastHfTick;
        K2LIST_ANCHOR               TimerQueue;
        UINT32                      mMigratedMask;
        K2LIST_ANCHOR               DefferedResumeList;
    } Locked;
};

struct _KERN_DATA_USER
{
    UINT32              mBotInitVirtBar;
    UINT32              mBotInitPt;
    UINT32              mTopInitPt;
    UINT32              mTopInitVirtBar;

    // xdlTrack is always 2 pages - xdl globals page and crt xdl page
    // crt xdl header is 1 page in source file
    UINT32              mCrtTextPagesCount; // starts at K2OS_UVA_LOW_BASE + 2 pages
    UINT32              mCrtReadPagesCount;
    UINT32              mCrtDataPagesCount;
    UINT32              mCrtSymPagesCount;

    UINT32              mInitPageCount;

    UINT32              mEntrypoint;

    K2OSKERN_OBJREF     RefVirtMapOfUserCrtDefaultDataSegment;

    K2OSKERN_OBJ_PAGEARRAY *mpKernUserCrtSeg[KernUserCrtSegType_Count];
    K2OSKERN_OBJ_PAGEARRAY  PublicApiPageArray;
    K2OSKERN_OBJ_PAGEARRAY  TimerPageArray;
};

struct _KERN_DATA_INTR
{
    K2OSKERN_SEQLOCK        SeqLock;
    struct {
        K2LIST_ANCHOR       IntrList;
        K2TREE_ANCHOR       IrqTree;
    } Locked;
};

struct _KERN_DATA_PLAT
{
    K2OSKERN_SEQLOCK            SeqLock;
    struct {
        K2TREE_ANCHOR           Tree;
    } Locked;

    K2OSPLAT_pf_DeviceCreate    mfDeviceCreate;
    K2OSPLAT_pf_DeviceRemove    mfDeviceRemove;
    K2OSPLAT_pf_DeviceAddRes    mfDeviceAddRes;
};

struct _KERN_DATA_BOOTGRAF
{
    ACPI_BGRT const *   mpBGRT;
};

struct _KERN_DATA_THREAD
{
    K2LIST_ANCHOR           DeferredCsInitList;
    K2LIST_ANCHOR           CreateList;
    K2LIST_ANCHOR           ActiveList;
    K2LIST_ANCHOR           DeadList;
    K2OSKERN_SEQLOCK        ListSeqLock;
    UINT64 volatile         mTlsUse;
};

struct _KERN_DATA_TOKEN
{
    K2LIST_ANCHOR           FreeList;
    K2OSKERN_SEQLOCK        SeqLock;
};

struct _KERN_DATA_EXEC
{
    K2OS_pf_THREAD_ENTRY    mfMainThreadEntryPoint;
};

struct _KERN_DATA_IFACE
{
    K2OSKERN_SEQLOCK        SeqLock;
    K2TREE_ANCHOR           IdTree;
    UINT32                  mLastId;
    K2LIST_ANCHOR           SubsList;
    UINT32                  mLastRequestId;
    K2TREE_ANCHOR           RequestTree;
};

struct _KERN_DATA_IPCEND
{
    K2OSKERN_SEQLOCK    SeqLock;
    K2LIST_ANCHOR       KernList;
};

struct _KERN_DATA_MBOXOWNER
{
    K2OSKERN_SEQLOCK    SeqLock;
    K2LIST_ANCHOR       KernList;
};

struct _KERN_DATA_SYSPROC
{
    K2OSKERN_OBJREF     RefKernVirtMap1;
    K2OSKERN_OBJREF     RefUserVirtMap1;

    K2OSKERN_OBJREF     RefKernVirtMap4;
    K2OSKERN_OBJREF     RefUserVirtMap4;

    K2OSKERN_OBJREF     RefNotify;

    K2OSKERN_OBJREF     RefReady1;      // kernel
    K2OS_SIGNAL_TOKEN   mTokReady1;     // thread

    K2OS_SIGNAL_TOKEN   mTokReady2;
};

struct _KERN_DATA_RPC
{
    K2OS_RPC_CLASS  mKernelServerClass;
};

struct _KERN_DATA_PAGING
{
    K2OSKERN_OBJREF     NotifyRef;
    K2OS_SIGNAL_TOKEN   mTokSignal;

    UINT32 volatile     mListHead;
};

struct _KERN_DATA_FIRMWARE
{
    K2OSKERN_SEQLOCK    SeqLock;
};

struct _KERN_DATA
{
    K2OSKERN_SHARED *       mpShared;
	UINT32 			        mCpuCoreCount;
    BOOL                    mCore0MonitorStarted;
    BOOL                    mSystemStarted;

    INT32 volatile          mSystemWideThreadCount;
    INT32 volatile          mSystemWideProcessCount;

    KERN_DATA_DEBUG         Debug;
    KERN_DATA_TIMER         Timer;
    KERN_DATA_XDL           Xdl;
    KERN_DATA_VIRTMAP       VirtMap;
    KERN_DATA_PHYS          Phys;
    KERN_DATA_VIRT          Virt;
    KERN_DATA_PROC          Proc;
    KERN_DATA_OBJ           Obj;
    KERN_DATA_BUILTIN       BuiltIn;
    KERN_DATA_FILESYS       FileSys;
    KERN_DATA_SCHED         Sched;
    KERN_DATA_USER          User;
    KERN_DATA_INTR          Intr;
    KERN_DATA_PLAT          Plat;
    KERN_DATA_BOOTGRAF      BootGraf;
    KERN_DATA_THREAD        Thread;
    KERN_DATA_TOKEN         Token;
    KERN_DATA_EXEC          Exec;
    KERN_DATA_IFACE         Iface;
    KERN_DATA_IPCEND        IpcEnd;
    KERN_DATA_MBOXOWNER     MboxOwner;
    KERN_DATA_SYSPROC       SysProc;
    KERN_DATA_RPC           Rpc;
    KERN_DATA_PAGING        Paging;
    KERN_DATA_FIRMWARE      Firm;
};
extern KERN_DATA gData;

/* --------------------------------------------------------------------------------- */

void K2_CALLCONV_REGS Kern_Exec(void);

/* --------------------------------------------------------------------------------- */

//
// bit.c
//
UINT32 KernBit_LowestSet_Index(UINT32 x);
UINT32 KernBit_ExtractLowestSet(UINT32 x);
UINT32 KernBit_HighestSet_Index(UINT32 x);
UINT32 KernBit_ExtractHighestSet(UINT32 x);
BOOL   KernBit_IsPowerOfTwo(UINT32 x);
UINT32 KernBit_LowestSet_Index64(UINT64 *px);

/* --------------------------------------------------------------------------------- */

//
// kernex.c
//
void                     KernEx_Assert(char const *apFile, int aLineNum, char const *apCondition);
void K2_CALLCONV_REGS    KernEx_RaiseException(K2STAT aExceptionCode);
K2STAT  K2_CALLCONV_REGS KernEx_TrapDismount(K2_EXCEPTION_TRAP *apTrap);
void                     KernEx_PanicDump(K2OSKERN_CPUCORE volatile *apThisCore);

/* --------------------------------------------------------------------------------- */

//
// <arch>/k2oskern/*
//
void    KernArch_AtXdlEntry(void);
UINT32  KernArch_MakePTE(UINT32 aPhysAddr, UINT32 aPageMapAttr);
void    KernArch_InstallPageTable(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aVirtAddrPtMaps, UINT32 aPhysPtPageAddr);
UINT32  KernArch_SendIci(K2OSKERN_CPUCORE volatile * apThisCore, UINT32 aMask, KernIciType aIciType, void *apArg);
void    KernArch_Panic(K2OSKERN_CPUCORE volatile *apThisCore, BOOL aDumpStack);
void    KernArch_InvalidateTlbPageOnCurrentCore(UINT32 aVirtAddr);
BOOL    KernArch_PteMapsUserWriteable(UINT32 aPTE);
void    KernArch_VirtInit(void);
void    KernArch_LaunchCpuCores(void);
void    KernArch_UserInit(void);
void    KernArch_SetCoreToProcess(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_PROCESS *apProc);
BOOL    KernArch_UserProcPrep(K2OSKERN_OBJ_PROCESS *apProc);
void    KernArch_UserCrtPrep(K2OSKERN_OBJ_PROCESS *apProc, K2OSKERN_OBJ_THREAD *pInitThread);
void    KernArch_UserThreadPrep(K2OSKERN_OBJ_THREAD *apThread, UINT32 aEntryPoint, UINT32 aStackPtr, UINT32 aArg0, UINT32 aArg1);
void    KernArch_KernThreadPrep(K2OSKERN_OBJ_THREAD *apThread, UINT32 aEntryPoint, UINT32 * apStackPtr, UINT32 aArg0, UINT32 aArg1);
void    KernArch_ResumeThread(K2OSKERN_CPUCORE volatile *apThisCore);
void    KernArch_DumpThreadContext(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apThread);
void    KernArch_SchedTimer_Arm(K2OSKERN_CPUCORE volatile * apThisCore, UINT64 const *apDeltaHfTicks);
void    KernArch_InstallDevIrqHandler(K2OSKERN_IRQ *apIrq);
void    KernArch_SetDevIrqMask(K2OSKERN_IRQ *apIrq, BOOL aMask);
void    KernArch_RemoveDevIrqHandler(K2OSKERN_IRQ *apIrq);
void    KernArch_GetPhysTable(UINT32 *apRetCount, K2OS_PHYSADDR_RANGE const **appRetTable);
void    KernArch_GetIoTable(UINT32 *apRetCount, K2OS_IOPORT_RANGE const **appRetTable);
void    KernArch_GetHfTimerTick(UINT64 *apRetHfTick);
void    KernArch_IntsOff_SaveKernelThreadStateAndEnterMonitor(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apThread);
BOOL    K2_CALLCONV_REGS KernArch_ExTrapMount(K2_EXCEPTION_TRAP *apTrap);
void    KernArch_PopKernelTrap(K2OSKERN_OBJ_THREAD *apThread);
void    KernArch_PopUserTrap(K2OSKERN_OBJ_THREAD *apThread);

/* --------------------------------------------------------------------------------- */

//
// plat.c
//
void    KernPlat_Init(void);

void    KernPlat_IntrLocked_OnIrq(K2OSKERN_CPUCORE volatile * apThisCore, UINT64 const *apHfTick, K2OSKERN_IRQ *apIrq);

/* --------------------------------------------------------------------------------- */

//
// pte.c
//
void    KernPte_MakePageMap(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aVirtAddr, UINT32 aPhysAddr, UINT32 aPageMapAttr);
UINT32  KernPte_BreakPageMap(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aVirtAddr, UINT32 aNpFlags);

/* --------------------------------------------------------------------------------- */

//
// debug.c
//

void    KernDbg_Emitter(void *apContext, char aCh);
UINT32  KernDbg_OutputWithArgs(char const *apFormat, VALIST aList);
void    KernDbg_RawDumpLockStack(K2OSKERN_CPUCORE volatile *apThisCore);

/* --------------------------------------------------------------------------------- */

//
// phys.c
//

void    KernPhys_Init(void);
BOOL    KernPhys_Reserve_Init(K2OSKERN_PHYSRES *apRes, UINT32 aPageCount);
BOOL    KernPhys_Reserve_Add(K2OSKERN_PHYSRES *apRes, UINT32 aPageCount);
void    KernPhys_Reserve_Release(K2OSKERN_PHYSRES *apRes);

K2STAT  KernPhys_AllocSparsePages(K2OSKERN_PHYSRES *apRes, UINT32 aPageCount, K2LIST_ANCHOR *apRetTrackList);
K2STAT  KernPhys_AllocPow2Bytes(K2OSKERN_PHYSRES *apRes, UINT32 aPow2Bytes, K2OSKERN_PHYSTRACK **appRetTrack);
void    KernPhys_FreeTrack(K2OSKERN_PHYSTRACK *apTrack);
void    KernPhys_FreeTrackList(K2LIST_ANCHOR *apTrackList);

void    KernPhys_ScanInit(K2OSKERN_PHYSSCAN *apScan, K2LIST_ANCHOR *apTrackList, UINT32 aStartOffset);
UINT32  KernPhys_ScanIter(K2OSKERN_PHYSSCAN *apScan);
void    KernPhys_ScanToPhysPageArray(K2OSKERN_PHYSSCAN *apScan, UINT32 aSetLowBits, UINT32 *apArray);

void    KernPhys_CutTrackListIntoPages(K2LIST_ANCHOR *apTrackList, BOOL aSetProc, K2OSKERN_OBJ_PROCESS *apProc, BOOL aSetList, KernPhysPageList aList);

BOOL    KernPhys_InAllocatableRange(UINT32 aPhysBase, UINT32 aPageCount);

K2STAT  KernPhys_GetEfiChunk(UINT32 aInfoIx, K2OS_PHYSADDR_RANGE *apChunkInfo);

/* --------------------------------------------------------------------------------- */

//
// virt.c
//
void    KernVirt_Init(void);
void *  KernHeap_Alloc(UINT32 aByteCount);
BOOL    KernHeap_Free(void *aPtr);
UINT32  KernVirt_Reserve(UINT32 aPageCount);
BOOL    KernVirt_Release(UINT32 aVirtAddr);
BOOL    KernVirt_AddRefContaining(UINT32 aPagesAddr, K2OSKERN_VIRTHEAP_NODE **appRetVirtHeapNode);
void    KernVirt_Threaded_Init(void);

/* --------------------------------------------------------------------------------- */

//
// cpu.c
//
void    KernCpu_AtXdlEntry(void);
void    KernCpu_DrainEvents(K2OSKERN_CPUCORE volatile *apThisCore);
BOOL    KernCpu_ExecOneDpc(K2OSKERN_CPUCORE volatile *apThisCore, KernDpcPrioType aPrio);
void    KernCpu_QueueDpc(K2OSKERN_DPC *apDpc, K2OSKERN_pf_DPC *apKey, KernDpcPrioType aPrio);
void    KernCpu_QueueEvent(K2OSKERN_CPUCORE_EVENT volatile *apEvent);
void    KernCpu_CpuEvent_RecvIci(K2OSKERN_CPUCORE volatile *apThisCore, UINT64 const *apHfTick, UINT32 aSrcCoreIx);
void    KernCpu_Schedule(K2OSKERN_CPUCORE volatile *apThisCore);
void    KernCpu_TakeCurThreadOffThisCore(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apCurThread, KernThreadStateType aNewState);
void    KernCpu_MigrateThreadToCore(K2OSKERN_OBJ_THREAD *apThread, K2OSKERN_CPUCORE volatile *apTargetCore);
void    KernCpu_PanicSpin(K2OSKERN_CPUCORE volatile *apThisCore);
void    KernCpu_RunMonitor(void);
void    KernCpu_SetTickMode(K2OSKERN_CPUCORE volatile * apThisCore, KernTickModeType aNewMode);
void    KernCpu_TakeInMigratingThreads(K2OSKERN_CPUCORE volatile * apThisCore);
void    KernCpu_StopProc(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_PROCESS *apProc);

/* --------------------------------------------------------------------------------- */

//
// sched.c
//
void    KernSched_Init(void);
BOOL    KernSched_Exec(K2OSKERN_CPUCORE volatile *apThisCore);
void    KernSched_CpuEvent_TimerFired(K2OSKERN_CPUCORE volatile *apThisCore, UINT64 const *apHfTick);
void    KernSched_QueueItem(K2OSKERN_SCHED_ITEM *apItem);

/* --------------------------------------------------------------------------------- */

//
// intr.c
//
void    KernIntr_Init(void);
void    KernIntr_OnIci(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread, UINT32 aSrcCoreIx);
void    KernIntr_OnIrq(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_IRQ *apIrq);
void    KernIntr_OnSystemCall(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCallingThread, UINT32 *apRetFastResult);

void    KernIntr_CpuEvent_Fired(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_INTERRUPT *apInterrupt, UINT64 const *apHfTick);
void    KernIntr_Cleanup(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_INTERRUPT *apInterrupt);

/* --------------------------------------------------------------------------------- */

//
// object.c
//
void                    KernObj_Init(void);

K2OSKERN_OBJ_HEADER *   KernObj_Alloc(KernObjType aObjType);
void                    KernObj_Free(K2OSKERN_OBJ_HEADER *apHdr);

char const * const      KernObj_Name(KernObjType aType);
UINT32                  KernObj_ReleaseRef(K2OSKERN_OBJREF *apRef);

#if DEBUG_REF
#define KernObj_CreateRef(r,t)   KernObj_DebugCreateRef(r,t,__FILE__,__LINE__);
void KernObj_DebugCreateRef(K2OSKERN_OBJREF *apRef, K2OSKERN_OBJ_HEADER *apTarget, char const *apFile, int aLine);
#define KernObj_ReleaseRef(r)    KernObj_DebugReleaseRef(r,__FILE__,__LINE__);
UINT32 KernObj_DebugReleaseRef(K2OSKERN_OBJREF *apRef, char const *apFile, int aLine);
#else
void                    KernObj_CreateRef(K2OSKERN_OBJREF *apRef, K2OSKERN_OBJ_HEADER *apTarget);
#endif

K2STAT                  KernObj_Share(K2OSKERN_OBJ_PROCESS *apSrcProc, K2OSKERN_OBJ_HEADER *apObjHdr, K2OSKERN_OBJ_PROCESS *apTargetProc, UINT32 *apRetTokenValue);

/* --------------------------------------------------------------------------------- */

//
// proc.c
//
void    KernProc_Init(void);
BOOL    KernProc_FindAddRefById(UINT32 aProcId, K2OSKERN_OBJREF *apRetRef);
K2STAT  KernProc_FindMapAndCreateRef(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aProcVirtAddr, K2OSKERN_OBJREF *apFillMapRef, UINT32 * apRetMapPageIx);
K2STAT  KernProc_AcqMaps(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aProcVirtAddr, UINT32 aDataSize, BOOL aWriteable, UINT32 *apRetMapCount, K2OSKERN_OBJREF *apRetMapRefs, UINT32 * apRetMap0StartPage);
K2STAT  KernProc_TokenCreate(K2OSKERN_OBJ_PROCESS *apProc, K2OSKERN_OBJ_HEADER *apObjHdr, K2OS_TOKEN *apRetToken);
K2STAT  KernProc_TokenTranslate(K2OSKERN_OBJ_PROCESS *apProc, K2OS_TOKEN aToken, K2OSKERN_OBJREF *apRef);
K2STAT  KernProc_TokenDestroyInSysCall(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apCurThread, K2OS_TOKEN aToken);
K2STAT  KernProc_UserVirtHeapAlloc(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aPageCount, K2OSKERN_PROCHEAP_NODE **apRetProcHeapNode);
K2STAT  KernProc_UserVirtHeapAllocAt(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aVirtAddr, UINT32 aPageCount, K2OSKERN_PROCHEAP_NODE **apRetProcHeapNode);
K2STAT  KernProc_UserVirtHeapFree(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aVirtAddr);
void    KernProc_SysCall_TokenClone(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernProc_SysCall_TokenDestroy(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernProc_SysCall_TokenShare(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernProc_SysCall_VirtReserve(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);
void    KernProc_SysCall_VirtGet(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);
void    KernProc_SysCall_VirtRelease(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);
void    KernProc_SysCall_AtStart(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernProc_SysCall_Exit(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernProc_SysCall_GetExitCode(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernProc_ExitedDpc(K2OSKERN_CPUCORE volatile *apThisCore, void *apArg);
void    KernProc_StopDpc(K2OSKERN_CPUCORE volatile *apThisCore, void *apArg);
void    KernProc_Cleanup(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_PROCESS *apProc);
K2STAT  KernProc_Create(K2OSKERN_CPUCORE volatile *apThisCore, K2OS_PROC_START_INFO *apLaunchInfo, K2OSKERN_OBJREF *apRetRef);
BOOL    KernProc_SysCall_Fast_TlsAlloc(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
BOOL    KernProc_SysCall_Fast_TlsFree(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernProc_SysCall_CacheOp(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);
K2STAT  KernProc_TokenLocked_Destroy(K2OSKERN_OBJ_PROCESS *apProc, K2OS_TOKEN aToken, BOOL *apRetHitZero, K2OSKERN_OBJREF *apRetObjRefIfHitZero);
void    KernProc_SysCall_OutputDebug(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);

K2OS_VIRTMAP_TOKEN KernProc_Threaded_UserVirtMapCreate(UINT32 aProcessId, UINT32 aVirtResBase, UINT32 aVirtResPageCount, K2OS_PAGEARRAY_TOKEN aTokPageArray);
K2STAT             KernProc_Threaded_UserMap(UINT32 aProcessId, K2OS_PAGEARRAY_TOKEN aKernTokPageArray, UINT32 aPageCount, UINT32 *apRetUserVirtAddr, K2OS_VIRTMAP_TOKEN *apRetTokUserVirtMap);


/* --------------------------------------------------------------------------------- */

//
// user.c
//
void    KernUser_Init(void);

/* --------------------------------------------------------------------------------- */

//
// builtin.c
//
void    KernBuiltIn_Init(void);

/* --------------------------------------------------------------------------------- */

//
// filesys.c
//
void    KernFileSys_Init(void);


/* --------------------------------------------------------------------------------- */

//
// pagearray.c
//

K2STAT  KernPageArray_CreateSparseFromRes(K2OSKERN_PHYSRES *apRes, UINT32 aPageCount, UINT32 aUserPermit, K2OSKERN_OBJREF *apRetRef);
K2STAT  KernPageArray_CreateSparse(UINT32 aPageCount, UINT32 aUserPermit, K2OSKERN_OBJREF *apRetRef);
K2STAT  KernPageArray_CreateSpec(UINT32 aPhysAddr, UINT32 aPageCount, UINT32 aUserPermit, K2OSKERN_OBJREF *apRetRef);
K2STAT  KernPageArray_CreateTrack(K2OSKERN_PHYSTRACK *apTrack, UINT32 aUserPermit, K2OSKERN_OBJREF *apRetRef);
K2STAT  KernPageArray_CreatePreMap(UINT32 aVirtAddr, UINT32 aPageCount, UINT32 aUserPermit, K2OSKERN_OBJREF *apRetRef);
UINT32  KernPageArray_PagePhys(K2OSKERN_OBJ_PAGEARRAY *apPageArray, UINT32 aPageIx);
void    KernPageArray_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernPageArray_SysCall_GetLen(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);
void    KernPageArray_Cleanup(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_PAGEARRAY *apPageArray);

K2OS_PAGEARRAY_TOKEN K2OSKERN_PageArray_CreateAt(UINT32 aPhysBase, UINT32 aPageCount);
K2OS_PAGEARRAY_TOKEN K2OSKERN_PageArray_CreateIo(UINT32 aFlags, UINT32 aPageCountPow2, UINT32 *apRetPhysBase);
UINT32               K2OSKERN_PageArray_GetPagePhys(K2OS_PAGEARRAY_TOKEN aTokPageArray, UINT32 aPageIndex);

/* --------------------------------------------------------------------------------- */

//
// usermap.c
// virtmap.c
// kernmap.c
//
K2STAT  KernVirtMap_CreateUser(K2OSKERN_OBJ_PROCESS *apProc, K2OSKERN_OBJ_PAGEARRAY *apPageArray, UINT32 aStartPageOffset, UINT32 aProcVirtAddr, UINT32 aPageCount, K2OS_VirtToPhys_MapType aMapType, K2OSKERN_OBJREF *apRetMapRef);
void    KernVirtMap_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);
void    KernVirtMap_SysCall_Acquire(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);
void    KernVirtMap_SysCall_GetInfo(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD * apCurThread);
K2STAT  KernVirtMap_FindMapAndCreateRef(UINT32 aKernVirtAddr, K2OSKERN_OBJREF *apFillMapRef, UINT32 *apRetMapPageIx);
void    KernVirtMap_Cleanup(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_VIRTMAP *apMap);
K2STAT  KernVirtMap_Create(K2OSKERN_OBJ_PAGEARRAY *apPageArray, UINT32 aPageOffset, UINT32 aPageCount, UINT32 aVirtAddr, K2OS_VirtToPhys_MapType aMapType, K2OSKERN_OBJREF *apRetRef);
void    KernVirtMap_CreatePreMap(UINT32 aVirtAddr, UINT32 aPageCount, K2OS_VirtToPhys_MapType aMapType, K2OSKERN_OBJREF *apRetRef);
K2STAT  KernVirtMap_CreateThreadStack(K2OSKERN_OBJ_THREAD *apThread);

/* --------------------------------------------------------------------------------- */

//
// thread.c
// 
void    KernThread_Init(void);
UINT32 K2_CALLCONV_REGS KernThread_EntryPoint(UINT32 aFirstArg, UINT32 aSecondArg);
UINT32  KernThread_GetId(void);
void    KernThread_Exit(UINT32 aExitCode);
void    KernThread_Cleanup_Dpc(K2OSKERN_CPUCORE volatile * apThisCore, void *apKey);
K2STAT  KernThread_Create(K2OSKERN_OBJ_PROCESS *apProc, K2OS_THREAD_CONFIG const * apConfig, K2OSKERN_OBJ_THREAD **appRetThread);
void    KernThread_CpuEvent_SysCall(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apThread);
void    KernThread_CpuEvent_Exception(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apThread);
void    KernThread_SysCall_RaiseException(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernThread_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernThread_SysCall_Exit(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernThread_SysCall_SetAffinity(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernThread_SysCall_GetExitCode(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernThread_SysCall_DebugBreak(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernThread_SysCall_SetName(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernThread_SysCall_GetName(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);

void    KernThread_Cleanup(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apThread);

void    KernThread_CallScheduler(K2OSKERN_CPUCORE volatile *apThisCore);
void    KernThread_RecvThreadTlbInv(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_TLBSHOOT *apShoot);

/* --------------------------------------------------------------------------------- */

//
// timer.c
//
void    KernTimer_Init(void);
void    KernTimer_HfTickFromMsTick(UINT64 *apRetHfTick, UINT64 const *apMsTick);
void    KernTimer_MsTickFromHfTick(UINT64 *apRetMs, UINT64 const *apHfTicks);
void    KernTimer_GetTime(K2OS_TIME *apRetTime);
void    KernTimer_SysCall_GetTime(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);

/* --------------------------------------------------------------------------------- */

//
// notify.c
//
K2STAT  KernNotify_Create(BOOL aInitSignalled, K2OSKERN_OBJREF *apRetNotifyRef);
void    KernNotify_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernNotify_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_NOTIFY *apNotify);
K2STAT  KernNotify_Threaded_KernelSignal(K2OSKERN_OBJ_THREAD *apThisThread, K2OSKERN_OBJ_NOTIFY *apNotify);

/* --------------------------------------------------------------------------------- */

//
// gate.c
//
K2STAT  KernGate_Create(BOOL aInitOpen, K2OSKERN_OBJREF *apRetGateRef);
void    KernGate_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernGate_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_GATE *apGate);
K2STAT  KernGate_Threaded_KernelChange(K2OSKERN_OBJ_THREAD *apThisThread, K2OSKERN_OBJ_GATE *apGate, UINT32 aNewState);

/* --------------------------------------------------------------------------------- */

//
// notify or gate
//
void     KernSignal_SysCall_Change(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);


/* --------------------------------------------------------------------------------- */

//
// alarm.c
//
K2STAT  KernAlarm_Create(UINT64 const *apPeriodMs, BOOL aIsPeriodic, K2OSKERN_OBJREF *apRetAlarmRef);
void    KernAlarm_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernAlarm_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_ALARM *apAlarm);
void    KernAlarm_PostCleanupDpc(K2OSKERN_CPUCORE volatile *apThisCore, void *apArg);

/* --------------------------------------------------------------------------------- */

//
// sem.c
//
K2STAT  KernSem_Create(UINT32 aMaxCount, UINT32 aInitCount, K2OSKERN_OBJREF *apRetSemRef);
void    KernSem_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_SEM *apSem);
K2STAT  KernSem_Share(K2OSKERN_OBJ_SEM *apSem, K2OSKERN_OBJ_PROCESS *apTargetProc, UINT32 *apRetToken);

K2STAT  KernSemUser_Create(K2OSKERN_OBJ_SEM *apSem, UINT32 aInitHold, K2OSKERN_OBJREF *apRetSemUserRef);
void    KernSemUser_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernSemUser_SysCall_Inc(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernSemUser_Cleanup(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_SEMUSER *apSemUser);
void    KernSemUser_PostCleanupDpc(K2OSKERN_CPUCORE volatile *apThisCore, void *apArg);

/* --------------------------------------------------------------------------------- */

//
// wait.c
//
void    KernWait_SysCall(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);

/* --------------------------------------------------------------------------------- */

//
// xdlhost.c
//
void    KernXdl_AtXdlEntry(void);
void    KernXdl_Init(void);
UINT32  KernXdl_FindClosestSymbol(K2OSKERN_OBJ_PROCESS *apCurProc, UINT32 aAddr, char *apRetSymName, UINT32 aRetSymNameBufLen);

void    KernXdl_Threaded_Init(void);
K2STAT  KernXdl_Threaded_Acquire(char const *apFilePath, XDL **appRetXdl, UINT32 *apRetEntryStackReq, K2_GUID128 *apRetID);

/* --------------------------------------------------------------------------------- */

//
// bootgraf.c
//
void    KernBootGraf_Init(void);

/* --------------------------------------------------------------------------------- */

//
// acpi.c
//
void    KernAcpi_Init(void);

/* --------------------------------------------------------------------------------- */

//
// critsec.c
//
void    KernCritSec_Threaded_InitDeferred(void);
K2STAT  KernCritSec_Init(K2OS_CRITSEC *apKernSec);
void    KernCritSec_Destroy(K2OS_CRITSEC *apKernSec);

/* --------------------------------------------------------------------------------- */

//
// exec.c
//
void    KernExec_Threaded_Init(void);

/* --------------------------------------------------------------------------------- */

//
// token.c
//

#define KERN_TOKEN_SENTINEL1    K2_MAKEID4('T','O','K','N')
#define KERN_TOKEN_SENTINEL2    K2_MAKEID4('t','o','k','n')

K2STAT      KernToken_Create(K2OSKERN_OBJ_HEADER *apObjHdr, K2OS_TOKEN *apRetToken);
K2STAT      KernToken_Translate(K2OS_TOKEN aToken, K2OSKERN_OBJREF *apRef);
K2OS_TOKEN  KernToken_Threaded_CloneFromUser(UINT32 aProcessId, K2OS_TOKEN aUserToken);

/* --------------------------------------------------------------------------------- */

//
// mail.c
//

void    KernMailbox_RecvRes(K2OSKERN_OBJ_MAILBOX *apMailbox, UINT32 aSlotIx);
K2STAT  KernMailbox_Share(K2OSKERN_OBJ_MAILBOX  *apMailbox, K2OSKERN_OBJ_PROCESS *apTargetProc, UINT32 *apRetTargetProcTokenValue);

void    KernMailbox_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_MAILBOX *apMailbox);
void    KernMailbox_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
BOOL    KernMailbox_Reserve(K2OSKERN_OBJ_MAILBOX *apMailbox, UINT32 aCount);
void    KernMailbox_UndoReserve(K2OSKERN_OBJ_MAILBOX *apMailbox, UINT32 aCount);
K2STAT  KernMailbox_Deliver(K2OSKERN_OBJ_MAILBOX * apMailbox, K2OS_MSG const * apMsg, BOOL aDoAvailAlloc, K2OSKERN_OBJ_HEADER * apReserveUser, BOOL * apRetDoSignal);
void    KernMailbox_SysCall_SentFirst(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
BOOL    KernMailbox_InIntr_Fast_Check_SentFirst(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);

void    KernMailboxOwner_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_MAILBOXOWNER *apMailboxOwner);
void    KernMailboxOwner_SysCall_RecvRes(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernMailboxOwner_SysCall_RecvLast(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernMailboxOwner_AbortReserveHolders(K2OSKERN_OBJ_MAILBOXOWNER *apMailboxOwner);
void    KernMailslot_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_MAILSLOT *apMailslot);
void    KernMailslot_SysCall_Get(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);


/* --------------------------------------------------------------------------------- */

K2STAT  KernIfEnum_Create(BOOL aOneProc, UINT32 aTargetProcessId, UINT32 aClassCode, K2_GUID128 const *apGuid, K2OSKERN_OBJREF *apRetRef);
void    KernIfInst_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_IFINST *apIfInst);
void    KernIfInst_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernIfInst_SysCall_SetMailbox(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernIfInst_SysCall_Publish(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
UINT32  KernIfInst_SysCall_Fast_GetId(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
UINT32  KernIfInst_SysCall_Fast_ContextOp(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);

void    KernIfInstId_SysCall_GetDetail(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
K2STAT  KernIfInstId_GetDetail(K2OS_IFINST_ID aIfInstId, K2OS_IFINST_DETAIL *apRetDetail);

void    KernIfEnum_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_IFENUM *apIfInst);
void    KernIfEnum_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void    KernIfEnum_SysCall_Reset(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
K2STAT  KernIfEnum_Next(K2OSKERN_OBJ_IFENUM *apIfEnum, K2OS_IFINST_DETAIL *apEntryBuffer, UINT32 *apIoEntryBufferCount);

K2STAT  KernIfSubs_Create(K2OSKERN_OBJ_MAILBOX *apMailbox, UINT32 aClassCode, K2_GUID128 const *apSpecific, UINT32 aBacklogCount, BOOL aProcSelfNotify, UINT32 aContext, K2OSKERN_OBJREF *apRetRef);
void    KernIfSubs_Cleanup(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_IFSUBS *apIfInst);
void    KernIfSubs_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);

/* --------------------------------------------------------------------------------- */

//
// ipcend.c
//

K2STAT KernIpcEnd_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    void *                      apKey,
    K2OSKERN_OBJ_MAILBOXOWNER * apMailboxOwner,
    K2OSKERN_OBJ_VIRTMAP *      apVirtMap,
    UINT32                      aMsgConfig,
    K2OS_IPCEND_TOKEN *         apRetToken
);
K2STAT KernIpcEnd_SendRequest(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread, K2OSKERN_OBJ_IPCEND *apIpcEnd, UINT32 aIfInstId);
K2STAT KernIpcEnd_Accept(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread, K2OSKERN_OBJ_IPCEND *apIpcEnd, UINT32 aRequestId);
K2STAT KernIpcEnd_RejectRequest(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread, UINT32 aRequestId, UINT32 aReasonCode);
K2STAT KernIpcEnd_Load(K2OSKERN_OBJ_IPCEND * apIpcEnd);
K2STAT KernIpcEnd_Sent(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread, K2OSKERN_OBJ_IPCEND *apIpcEnd, UINT32 aByteCount);
K2STAT KernIpcEnd_ManualDisconnect(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread, K2OSKERN_OBJ_IPCEND *apIpcEnd);
void   KernIpcEnd_DisconnectAck(K2OSKERN_OBJ_IPCEND * apIpcEnd);

void KernIpcEnd_SysCall_Create(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void KernIpcEnd_SysCall_Send(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void KernIpcEnd_SysCall_Accept(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void KernIpcEnd_SysCall_RejectRequest(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void KernIpcEnd_SysCall_ManualDisconnect(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
void KernIpcEnd_SysCall_Request(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
BOOL KernIpcEnd_InIntr_SysCall_Load(K2OSKERN_OBJ_THREAD * apCurThread, K2OS_TOKEN aTokIpcEnd);
void KernIpcEnd_Cleanup(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_IPCEND *apIpcEnd);
void KernIpcEnd_RecvRes(K2OSKERN_OBJ_IPCEND *apIpcEnd, BOOL aDelivered);

void KernIpcEnd_Threaded_Init(void);

/* --------------------------------------------------------------------------------- */

//
// sysproc.c
//

void    KernSysProc_SysCall_Register(K2OSKERN_CPUCORE volatile * apThisCore, K2OSKERN_OBJ_THREAD *apCurThread);
K2STAT  KernSysProc_Notify(BOOL aFromKernelThread, K2OS_MSG const *apMsg, BOOL *apRetDoSignal);

void    K2OSKERN_WaitSysProcReady(void);

/* --------------------------------------------------------------------------------- */

//
// notifyproxy.c
//

BOOL    KernNotifyProxy_Fire(K2OSKERN_OBJ_NOTIFY * apNotify);
void    KernNotifyProxy_Cleanup(K2OSKERN_CPUCORE volatile *apThisCore, K2OSKERN_OBJ_NOTIFYPROXY *apNotifyProxy);


/* --------------------------------------------------------------------------------- */

//
// fsnode.c
//

void    KernFsNode_Dump(void);
int     KernFsNode_CaseNameCompare(UINT_PTR aKey, K2TREE_NODE *apNode);
int     KernFsNode_CaseInsNameCompare(UINT_PTR aKey, K2TREE_NODE *apNode);
void    KernFsNode_Init(K2OSKERN_FILESYS * apFileSys, K2OSKERN_FSNODE *apFsNode);
INT_PTR KernFsNode_AddRef(K2OSKERN_FSNODE *apFsNode);
INT_PTR KernFsNode_Release(K2OSKERN_FSNODE *apFsNode);
void    KernFsNode_DebugPrint(K2OSKERN_FSNODE *apFsNode);

/* --------------------------------------------------------------------------------- */

K2OSKERN_MAPUSER KernMapUser_Create(UINT32 aProcessId, K2OS_BUFDESC const *apBufDesc, UINT32 *apRetKernVirtAddr);
void             KernMapUser_Destroy(K2OSKERN_MAPUSER aMapUser);

/* --------------------------------------------------------------------------------- */

//
// firm.c
//

void KernFirm_Init(void);
void KernFirm_GetTime(K2OS_TIME *apRetTime);

/* --------------------------------------------------------------------------------- */

//
// rpc.c
//

K2STAT KernRpc_CreateObj(K2OS_RPC_OBJ aObject, K2OS_RPC_OBJ_CREATE const *apCreate, UINT32 *apRetContext);
K2STAT KernRpc_Attach(K2OS_RPC_OBJ aObject, UINT32 aObjContext, UINT32 aProcessId, UINT32 *apRetUseContext);
K2STAT KernRpc_Detach(K2OS_RPC_OBJ aObject, UINT32 aObjContext, UINT32 aUseContext);
K2STAT KernRpc_Call(K2OS_RPC_OBJ_CALL const *apCall, UINT32 *apRetUsedOutBytes);
K2STAT KernRpc_DeleteObj(K2OS_RPC_OBJ aObject, UINT32 aObjContext);

/* --------------------------------------------------------------------------------- */

//
// paging.c
//

void    KernPaging_Init(void);

/* --------------------------------------------------------------------------------- */

//
// trace.c
//
#if KTRACE_ON

#define KTRACE(pCore, count, args...)   KernTrace(pCore, count, args)
void    KernTrace(K2OSKERN_CPUCORE volatile *apThisCore, UINT32 aCount, ...);

void    KernTrace_Init(void);
#define KTRACE_INIT KernTrace_Init()

BOOL    KernTrace_SetEnable(BOOL aEnable);
#define KTRACE_SETENABLE(e) KernTrace_SetEnable(e)

void    KernTrace_Dump(void);
#define KTRACE_DUMP KernTrace_Dump()

#define KTRACE_CORE_ENTER_IDLE                          1
#define KTRACE_CORE_LEAVE_IDLE                          2
#define KTRACE_CORE_RESUME_THREAD                       3
#define KTRACE_CORE_STOP_PROC                           4
#define KTRACE_CORE_ABORT_THREAD                        5
#define KTRACE_CORE_SWITCH_NOPROC                       6
#define KTRACE_CORE_RECV_ICI                            7
#define KTRACE_CORE_SUSPEND_THREAD                      8
#define KTRACE_CORE_PANIC_SPIN                          9
#define KTRACE_CORE_EXEC_DPC                            10
#define KTRACE_CORE_ENTER_SCHEDULER                     11
#define KTRACE_CORE_LEAVE_SCHEDULER                     12
#define KTRACE_THREAD_STOPPED                           13
#define KTRACE_THREAD_QUANTUM_EXPIRED                   14
#define KTRACE_THREAD_RUN                               15
#define KTRACE_THREAD_EXCEPTION                         16
#define KTRACE_THREAD_SYSCALL                           17
#define KTRACE_THREAD_RAISE_EX                          18 
#define KTRACE_THREAD_MIGRATE                           19
#define KTRACE_THREAD_EXIT                              20
#define KTRACE_THREAD_START                             21
#define KTRACE_THREAD_KERNPAGE_CHECK_DPC                25
#define KTRACE_THREAD_KERNPAGE_SENDICI_DPC              26
#define KTRACE_THREAD_USERPAGE_CHECK_DPC                27
#define KTRACE_THREAD_USERPAGE_SENDICI_DPC              28
#define KTRACE_PROC_START                               29
#define KTRACE_PROC_BEGIN_STOP                          30
#define KTRACE_PROC_STOPPED                             31
#define KTRACE_PROC_TLBSHOOT_ICI                        32
#define KTRACE_PROC_STOP_DPC                            33
#define KTRACE_PROC_EXITED_DPC                          34
#define KTRACE_PROC_STOP_CHECK_DPC                      35
#define KTRACE_PROC_STOP_SENDICI_DPC                    36
#define KTRACE_PROC_TRANS_CHECK_DPC                     37
#define KTRACE_PROC_TRANS_SENDICI_DPC                   38
#define KTRACE_PROC_HIVIRT_CHECK_DPC                    39
#define KTRACE_PROC_HIVIRT_SENDICI_DPC                  40
#define KTRACE_PROC_LOVIRT_CHECK_DPC                    41
#define KTRACE_PROC_LOVIRT_SENDICI_DPC                  42
#define KTRACE_PROC_TOKEN_CHECK_DPC                     43
#define KTRACE_PROC_TOKEN_SENDICI_DPC                   44
#define KTRACE_TIMER_FIRED                              45
#define KTRACE_SCHED_EXEC_ITEM                          46
#define KTRACE_SCHED_EXEC_SYSCALL                       47
#define KTRACE_ALARM_POST_CLEANUP_DPC                   48
#define KTRACE_MAP_CLEAN_CHECK_DPC                      49
#define KTRACE_MAP_CLEAN_SENDICI_DPC                    50
#define KTRACE_OBJ_CLEANUP_DPC                          51
#define KTRACE_SEM_POST_CLEANUP_DPC                     52
#define KTRACE_KERN_TLBSHOOT_ICI                        53
#define KTRACE_PROC_TLBSHOOT_ICI_IGNORED                54
#define KTRACE_MAP_CLEAN_DONE                           56
#define KTRACE_CORE_ENTERED_DEBUG                       57
#define KTRACE_DEBUG_ENTER_CHECK_DPC                    58 
#define KTRACE_DEBUG_ENTER_SENDICI_DPC                  59
#define KTRACE_DEBUG_ENTERED                            60
#define KTRACE_THREAD_KERNMEM_TCHECK_DPC                61
#define KTRACE_THREAD_KERNMEM_SEG_SHOOT_START_DPC       62
#define KTRACE_THREAD_KERNMEM_SEG_SHOOT_SEND_DPC        63
#define KTRACE_THREAD_KERNMEM_SEG_SHOOT_CHECK_DPC       64
#define KTRACE_THREAD_KERNMEM_SEG_SHOOT_DONE            65
#define KTRACE_THREAD_KERNMEM_TSEND_DPC                 66
#define KTRACE_XDL_REMAP_CLEAN_CHECK_DPC                67
#define KTRACE_XDL_REMAP_CLEAN_DONE                     68
#define KTRACE_XDL_REMAP_CLEAN_SENDICI_DPC              69

#else
#define KTRACE(...)   
#define KTRACE_INIT 
#define KTRACE_SETENABLE(e)
#define KTRACE_DUMP
#endif

/* --------------------------------------------------------------------------------- */

#endif // __KERN_H