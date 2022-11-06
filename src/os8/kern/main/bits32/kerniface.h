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

#ifndef __KERNIFACE_H
#define __KERNIFACE_H

#if __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------------- */

#define K2OS_NUM_THREAD_TLS_SLOTS           64
#define K2OS_THREAD_DEFAULT_STACK_PAGES     3
#define K2OS_THREAD_PAGE_BUFFER_BYTES       1024

typedef struct _K2OS_USER_THREAD_PAGE K2OS_USER_THREAD_PAGE;
struct _K2OS_USER_THREAD_PAGE
{
    UINT32              mTlsValue[K2OS_NUM_THREAD_TLS_SLOTS];
    UINT32              mSysCall_Arg1;
    UINT32              mSysCall_Arg2;
    UINT32              mSysCall_Arg3;
    UINT32              mSysCall_Arg4_Result3;
    UINT32              mSysCall_Arg5_Result2;
    UINT32              mSysCall_Arg6_Result1;
    UINT32              mSysCall_Arg7_Result0;
    K2STAT              mLastStatus;
    UINT32              mCsDepth;
    K2OS_WAITABLE_TOKEN mWaitToken[K2OS_THREAD_WAIT_MAX_ITEMS + 1]; // +1 for mailbox Wait condition
    UINT8               mMiscBuffer[K2OS_THREAD_PAGE_BUFFER_BYTES];
    // process' initial thread's stack is at end of page until it gets its normal stack
};
K2_STATIC_ASSERT(sizeof(K2OS_USER_THREAD_PAGE) < K2_VA_MEMPAGE_BYTES);

/* --------------------------------------------------------------------------------- */

#define K2OS_SYSCALL_ID_CRT_INITXDL                 0
#define K2OS_SYSCALL_ID_PROCESS_EXIT                1
#define K2OS_SYSCALL_ID_PROCESS_START               2
#define K2OS_SYSCALL_ID_OUTPUT_DEBUG                3
#define K2OS_SYSCALL_ID_RAISE_EXCEPTION             4
#define K2OS_SYSCALL_ID_TOKEN_DESTROY               5
#define K2OS_SYSCALL_ID_NOTIFY_CREATE               6
#define K2OS_SYSCALL_ID_NOTIFY_SIGNAL               7
#define K2OS_SYSCALL_ID_THREAD_WAIT                 8
#define K2OS_SYSCALL_ID_THREAD_EXIT                 9
#define K2OS_SYSCALL_ID_THREAD_CREATE               10
#define K2OS_SYSCALL_ID_THREAD_SETAFF               11
#define K2OS_SYSCALL_ID_THREAD_GETEXITCODE          12
#define K2OS_SYSCALL_ID_PAGEARRAY_CREATE            13
#define K2OS_SYSCALL_ID_PAGEARRAY_GETLEN            14
#define K2OS_SYSCALL_ID_MAP_CREATE                  15
#define K2OS_SYSCALL_ID_MAP_ACQUIRE                 16
#define K2OS_SYSCALL_ID_MAP_ACQ_PAGEARRAY           17
#define K2OS_SYSCALL_ID_MAP_GET_INFO                18
#define K2OS_SYSCALL_ID_VIRT_RESERVE                19
#define K2OS_SYSCALL_ID_VIRT_GET                    20
#define K2OS_SYSCALL_ID_VIRT_RELEASE                21
#define K2OS_SYSCALL_ID_MAILBOX_CREATE              22
#define K2OS_SYSCALL_ID_MAILBOX_RECV_RES            23
#define K2OS_SYSCALL_ID_MAILBOX_CLOSE               24
#define K2OS_SYSCALL_ID_MAILSLOT_SEND               25
#define K2OS_SYSCALL_ID_GET_LAUNCH_INFO             26
#define K2OS_SYSCALL_ID_TRAP_MOUNT                  27
#define K2OS_SYSCALL_ID_TRAP_DISMOUNT               28
#define K2OS_SYSCALL_ID_GET_SYSINFO                 29
#define K2OS_SYSCALL_ID_GATE_CREATE                 30
#define K2OS_SYSCALL_ID_GATE_CHANGE                 31
#define K2OS_SYSCALL_ID_ALARM_CREATE                32
#define K2OS_SYSCALL_ID_SEM_CREATE                  33
#define K2OS_SYSCALL_ID_SEM_INC                     34
#define K2OS_SYSCALL_ID_TLS_ALLOC                   35
#define K2OS_SYSCALL_ID_TLS_FREE                    36
#define K2OS_SYSCALL_ID_IFENUM_CREATE               37
#define K2OS_SYSCALL_ID_IFENUM_RESET                38
#define K2OS_SYSCALL_ID_IFENUM_NEXT                 39
#define K2OS_SYSCALL_ID_IFSUBS_CREATE               40
#define K2OS_SYSCALL_ID_IFINST_CREATE               41
#define K2OS_SYSCALL_ID_IFINST_PUBLISH              42
#define K2OS_SYSCALL_ID_IFINST_REMOVE               43
#define K2OS_SYSCALL_ID_IPCEND_CREATE               44
#define K2OS_SYSCALL_ID_IPCEND_LOAD                 45
#define K2OS_SYSCALL_ID_IPCEND_SEND                 46
#define K2OS_SYSCALL_ID_IPCEND_ACCEPT               47
#define K2OS_SYSCALL_ID_IPCEND_MANUAL_DISCONNECT    48
#define K2OS_SYSCALL_ID_IPCEND_CLOSE                49
#define K2OS_SYSCALL_ID_IPCEND_REQUEST              50
#define K2OS_SYSCALL_ID_IPCREQ_REJECT               51
#define K2OS_SYSCALL_ID_INTR_SETENABLE              52
#define K2OS_SYSCALL_ID_INTR_END                    53
#define K2OS_SYSCALL_ID_ROOT_PROCESS_LAUNCH         54
#define K2OS_SYSCALL_ID_ROOT_IOPERMIT_ADD           55
#define K2OS_SYSCALL_ID_ROOT_PAGEARRAY_CREATEAT     56
#define K2OS_SYSCALL_ID_ROOT_TOKEN_EXPORT           57
#define K2OS_SYSCALL_ID_ROOT_PLATNODE_CREATE        58
#define K2OS_SYSCALL_ID_ROOT_PLATNODE_ADDRES        59
#define K2OS_SYSCALL_ID_ROOT_PLATNODE_HOOKINTR      60
#define K2OS_SYSCALL_ID_ROOT_GET_PROCEXIT           61
#define K2OS_SYSCALL_ID_ROOT_PROCESS_STOP           62
#define K2OS_SYSCALL_ID_ROOT_TOKEN_IMPORT           63
#define K2OS_SYSCALL_ID_DEBUG_BREAK                 64

#define K2OS_SYSCALL_COUNT                          65

/* --------------------------------------------------------------------------------- */

typedef struct _CRT_LAUNCH_INFO CRT_LAUNCH_INFO;
struct _CRT_LAUNCH_INFO
{
    char    mArgStr[1024];
    char    mPath[1024];
};

//
//------------------------------------------------------------------------
//

#define K2OS_MAILBOX_OWNER_SENTINEL_1    K2_MAKEID4('M','b','o','x')
#define K2OS_MAILBOX_OWNER_SENTINEL_2    K2_MAKEID4('X','O','B','m')

#define K2OS_MAILBOX_OWNER_FLAG_DWORDS   8
K2_STATIC_ASSERT((K2OS_MAILBOX_OWNER_FLAG_DWORDS * 32 * sizeof(K2OS_MSG)) == K2_VA_MEMPAGE_BYTES);

typedef K2OS_WAITABLE_TOKEN K2OS_MAILBOX_TOKEN;
typedef K2OS_TOKEN          K2OS_IPCEND_TOKEN;

typedef struct _K2OS_MAILBOX_OWNER K2OS_MAILBOX_OWNER;
struct _K2OS_MAILBOX_OWNER
{
    UINT32              mSentinel1;

    K2OS_MAILBOX_TOKEN  mMailboxToken;

    UINT32              mRefs;

    K2OS_MSG *          mpRoMsgPage;                                    // 256 * 16-byte entries
    UINT32 volatile     mAvail;
    UINT32 volatile     mIxConsumer;                                    // 0 - 255 + wait flag high bit
    UINT32 volatile     mFlagBitArray[K2OS_MAILBOX_OWNER_FLAG_DWORDS * 2];   // 8 * 32 bits = 256 entries, * 16 bytes per = 4096 bytes
    // 256 bits for owner, 256 bits for reserve marker

    UINT32              mSentinel2;
};

/* --------------------------------------------------------------------------------- */

#define K2OS_SYSTEM_MSG_IPC_CREATED         (K2OS_MSG_CONTROL_SYSTEM_FUNC | K2OS_MSG_CONTROL_SYSTEM_IPC | 11)
#define K2OS_SYSTEM_MSG_IPC_RECV            (K2OS_MSG_CONTROL_SYSTEM_FUNC | K2OS_MSG_CONTROL_SYSTEM_IPC | 12)
#define K2OS_SYSTEM_MSG_IPC_CONNECTED       (K2OS_MSG_CONTROL_SYSTEM_FUNC | K2OS_MSG_CONTROL_SYSTEM_IPC | 13)
#define K2OS_SYSTEM_MSG_IPC_DISCONNECTED    (K2OS_MSG_CONTROL_SYSTEM_FUNC | K2OS_MSG_CONTROL_SYSTEM_IPC | 14)
#define K2OS_SYSTEM_MSG_IPC_REJECTED        (K2OS_MSG_CONTROL_SYSTEM_FUNC | K2OS_MSG_CONTROL_SYSTEM_IPC | 15)

/* --------------------------------------------------------------------------------- */

typedef void (K2_CALLCONV_REGS *CRT_pf_THREAD_ENTRY)(K2OS_pf_THREAD_ENTRY aUserEntry, void *apArgument);

/* --------------------------------------------------------------------------------- */

#if __cplusplus
}
#endif

#endif // __KERNIFACE_H