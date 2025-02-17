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

#ifndef __KERNIFACE_H
#define __KERNIFACE_H

#if __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------------- */

#define K2OS_NUM_THREAD_TLS_SLOTS           64
#define K2OS_THREAD_DEFAULT_STACK_PAGES     3
#define K2OS_THREAD_PAGE_BUFFER_BYTES       1024
#define K2OS_THREAD_TEMP_BUF_PAGE_COUNT     16

typedef struct _K2OS_THREAD_PAGE K2OS_THREAD_PAGE;
struct _K2OS_THREAD_PAGE
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
    UINT32              mContext;
    UINT32              mTrapStackTop;  // must be at this offset! 0x128
    K2OS_WAITABLE_TOKEN mWaitToken[K2OS_THREAD_WAIT_MAX_ITEMS];
    UINT8               mMiscBuffer[K2OS_THREAD_PAGE_BUFFER_BYTES];
    // process' initial thread's stack is at end of page until it gets its normal stack
};
K2_STATIC_ASSERT(sizeof(K2OS_THREAD_PAGE) < K2_VA_MEMPAGE_BYTES);

/* --------------------------------------------------------------------------------- */

#define K2OS_SYSCALL_ID_CRT_INITXDL                 0   // fast
#define K2OS_SYSCALL_ID_PROCESS_START               1
#define K2OS_SYSCALL_ID_PROCESS_EXIT                2
#define K2OS_SYSCALL_ID_DEBUG_BREAK                 3
#define K2OS_SYSCALL_ID_RAISE_EXCEPTION             4
#define K2OS_SYSCALL_ID_TOKEN_CLONE                 5
#define K2OS_SYSCALL_ID_TOKEN_SHARE                 6
#define K2OS_SYSCALL_ID_TOKEN_DESTROY               7
#define K2OS_SYSCALL_ID_NOTIFY_CREATE               8
#define K2OS_SYSCALL_ID_NOTIFY_SIGNAL               9 
#define K2OS_SYSCALL_ID_THREAD_WAIT                 10
#define K2OS_SYSCALL_ID_THREAD_EXIT                 11
#define K2OS_SYSCALL_ID_THREAD_CREATE               12
#define K2OS_SYSCALL_ID_THREAD_SETAFF               13
#define K2OS_SYSCALL_ID_THREAD_GETEXITCODE          14
#define K2OS_SYSCALL_ID_THREAD_SET_NAME             15
#define K2OS_SYSCALL_ID_THREAD_GET_NAME             16
#define K2OS_SYSCALL_ID_PAGEARRAY_CREATE            17
#define K2OS_SYSCALL_ID_PAGEARRAY_GETLEN            18
#define K2OS_SYSCALL_ID_MAP_CREATE                  19
#define K2OS_SYSCALL_ID_MAP_ACQUIRE                 20
#define K2OS_SYSCALL_ID_MAP_GET_INFO                21
#define K2OS_SYSCALL_ID_CACHE_OP                    22
#define K2OS_SYSCALL_ID_VIRT_RESERVE                23
#define K2OS_SYSCALL_ID_VIRT_GET                    24
#define K2OS_SYSCALL_ID_VIRT_RELEASE                25
#define K2OS_SYSCALL_ID_GATE_CREATE                 26
#define K2OS_SYSCALL_ID_GATE_CHANGE                 27
#define K2OS_SYSCALL_ID_ALARM_CREATE                28
#define K2OS_SYSCALL_ID_SEM_CREATE                  29
#define K2OS_SYSCALL_ID_SEM_INC                     30
#define K2OS_SYSCALL_ID_TLS_ALLOC                   31  // fast
#define K2OS_SYSCALL_ID_TLS_FREE                    32  // fast
#define K2OS_SYSCALL_ID_MAILBOX_CREATE              33
#define K2OS_SYSCALL_ID_MAILBOXOWNER_RECVRES        34
#define K2OS_SYSCALL_ID_MAILBOXOWNER_RECVLAST       35
#define K2OS_SYSCALL_ID_MAILSLOT_GET                36
#define K2OS_SYSCALL_ID_MAILBOX_SENTFIRST           37  // fast/slow
#define K2OS_SYSCALL_ID_IFINST_CREATE               38
#define K2OS_SYSCALL_ID_IFINST_GETID                39  // fast
#define K2OS_SYSCALL_ID_IFINST_SETMAILBOX           40
#define K2OS_SYSCALL_ID_IFINST_CONTEXTOP            41  // fast
#define K2OS_SYSCALL_ID_IFINST_PUBLISH              42
#define K2OS_SYSCALL_ID_IFINSTID_GETDETAIL          43
#define K2OS_SYSCALL_ID_IFENUM_CREATE               44 
#define K2OS_SYSCALL_ID_IFENUM_RESET                45
#define K2OS_SYSCALL_ID_IFSUBS_CREATE               46
#define K2OS_SYSCALL_ID_IPCEND_CREATE               47
#define K2OS_SYSCALL_ID_IPCEND_LOAD                 48  // fast 
#define K2OS_SYSCALL_ID_IPCEND_SEND                 49
#define K2OS_SYSCALL_ID_IPCEND_ACCEPT               50
#define K2OS_SYSCALL_ID_IPCEND_MANUAL_DISCONNECT    51 
#define K2OS_SYSCALL_ID_IPCEND_REQUEST              52
#define K2OS_SYSCALL_ID_IPCREQ_REJECT               53
#define K2OS_SYSCALL_ID_SYSPROC_REGISTER            54
#define K2OS_SYSCALL_ID_SIGNAL_CHANGE               55
#define K2OS_SYSCALL_ID_OUTPUT_DEBUG                56
#define K2OS_SYSCALL_ID_GET_TIME                    57

#define K2OS_SYSCALL_COUNT                          58

typedef UINT32(K2_CALLCONV_REGS* K2OS_pf_SysCall)(UINT32 aId, UINT32 aArg0);
#define K2OS_SYSCALL ((K2OS_pf_SysCall)(K2OS_UVA_PUBLICAPI_SYSCALL))
#define K2OS_Kern_SysCall1(x,y) K2OS_SYSCALL((x),(y))

/* --------------------------------------------------------------------------------- */

typedef struct _K2OS_PROC_START_INFO K2OS_PROC_START_INFO;
struct _K2OS_PROC_START_INFO
{
    char    mArgStr[1024];
    char    mPath[1024];
};

/* --------------------------------------------------------------------------------- */

#if 0
#define K2OS_MAILBOX_MSG_COUNT              64
#define K2OS_MAILBOX_MSG_IX_MASK            0x0000003F
#define K2OS_MAILBOX_BITFIELD_DWORD_COUNT   6
#else
#define K2OS_MAILBOX_MSG_COUNT              256
#define K2OS_MAILBOX_MSG_IX_MASK            0x000000FF
#define K2OS_MAILBOX_BITFIELD_DWORD_COUNT   8
#endif
#define K2OS_MAILBOX_GATE_CLOSED_BIT        0x80000000

typedef struct _K2OS_CACHEALIGNED_ATOMIC K2OS_CACHEALIGNED_ATOMIC;
struct _K2OS_CACHEALIGNED_ATOMIC
{
    union {
        UINT32 volatile mVal;
        UINT8           mAlign[K2OS_CACHELINE_BYTES];
    };
};

typedef struct _K2OS_MAILBOX_CONSUMER_PAGE K2OS_MAILBOX_CONSUMER_PAGE;
struct _K2OS_MAILBOX_CONSUMER_PAGE
{
    K2OS_CACHEALIGNED_ATOMIC    IxConsumer;

    K2OS_CACHEALIGNED_ATOMIC    ReserveMask[K2OS_MAILBOX_BITFIELD_DWORD_COUNT];

    // more on this page that the kernel sees
};
K2_STATIC_ASSERT(sizeof(K2OS_MAILBOX_CONSUMER_PAGE) <= K2_VA_MEMPAGE_BYTES);

typedef struct _K2OS_MAILBOX_MSGDATA_PAGE K2OS_MAILBOX_MSGDATA_PAGE;
struct _K2OS_MAILBOX_MSGDATA_PAGE
{
    K2OS_MSG Msg[K2OS_MAILBOX_MSG_COUNT];
};

typedef struct _K2OS_MAILBOX_PRODUCER_PAGE K2OS_MAILBOX_PRODUCER_PAGE;
struct _K2OS_MAILBOX_PRODUCER_PAGE
{
    K2OS_CACHEALIGNED_ATOMIC    IxProducer;

    K2OS_CACHEALIGNED_ATOMIC    AvailCount;

    K2OS_CACHEALIGNED_ATOMIC    OwnerMask[K2OS_MAILBOX_BITFIELD_DWORD_COUNT];
};
K2_STATIC_ASSERT(sizeof(K2OS_MAILBOX_PRODUCER_PAGE) <= K2_VA_MEMPAGE_BYTES);

/* --------------------------------------------------------------------------------- */

typedef K2OS_TOKEN K2OS_IPCEND_TOKEN;

/* --------------------------------------------------------------------------------- */

#define K2OS_SYSPROC_NOTIFY_MSG_COUNT               1024
#define K2OS_SYSPROC_NOTIFY_MSG_IX_MASK             0x3FF
#define K2OS_SYSPROC_NOTIFY_BITFIELD_DWORD_COUNT    32
#define K2OS_SYSPROC_NOTIFY_EMPTY_BIT               0x80000000

typedef struct _K2OS_SYSPROC_PAGE K2OS_SYSPROC_PAGE;
struct _K2OS_SYSPROC_PAGE
{
    K2OS_CACHEALIGNED_ATOMIC    IxProducer;

    K2OS_CACHEALIGNED_ATOMIC    IxConsumer;

    K2OS_CACHEALIGNED_ATOMIC    OwnerMask[K2OS_SYSPROC_NOTIFY_BITFIELD_DWORD_COUNT];
};
K2_STATIC_ASSERT(sizeof(K2OS_SYSPROC_PAGE) <= K2_VA_MEMPAGE_BYTES);

/* --------------------------------------------------------------------------------- */

// {EC475B48-A906-44A8-B2F6-23C0E9CE8E68}
#define K2OS_KERNEL_PROCESS_RPC_CLASS   { 0xec475b48, 0xa906, 0x44a8, { 0xb2, 0xf6, 0x23, 0xc0, 0xe9, 0xce, 0x8e, 0x68 } }

typedef enum _K2OS_KernelRpc_Method K2OS_KernelRpc_Method; 
enum _K2OS_KernelRpc_Method
{
    K2OS_KernelRpc_Method_Invalid = 0,

    K2OS_KernelRpc_Method_IfEnumNext,
    K2OS_KernelRpc_Method_GetLaunchInfo,

    K2OS_KernelRpc_Method_Count
}; 

typedef struct _K2OS_KERNELRPC_IFENUMNEXT_IN K2OS_KERNELRPC_IFENUMNEXT_IN;
struct _K2OS_KERNELRPC_IFENUMNEXT_IN
{
    K2OS_IFENUM_TOKEN   mTokEnum;
    UINT32              mCount;
    K2OS_BUFDESC        TargetBufDesc;
};

typedef struct _K2OS_KERNELRPC_IFENUMNEXT_OUT K2OS_KERNELRPC_IFENUMNEXT_OUT;
struct _K2OS_KERNELRPC_IFENUMNEXT_OUT
{
    UINT32  mCountGot;
};

typedef struct _K2OS_KERNELRPC_GETLAUNCHINFO_IN K2OS_KERNELRPC_GETLAUNCHINFO_IN;
struct _K2OS_KERNELRPC_GETLAUNCHINFO_IN
{
    K2OS_BUFDESC    TargetBufDesc;
};

/* --------------------------------------------------------------------------------- */

#if __cplusplus
}
#endif

#endif // __KERNIFACE_H