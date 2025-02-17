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

#include "kern.h"

typedef struct _VOLPAGER VOLPAGER;
struct _VOLPAGER
{
    K2OS_FILESYS_GETDETAIL_OUT  Detail;
    K2OS_FILE                   mFile;
};

UINT32
KernPaging_FsThread(
    void *apArg
)
{
    VOLPAGER *  pPager;

    pPager = (VOLPAGER *)apArg;

    K2OSKERN_Debug("Pager thread for filesystem number %d runs\n", pPager->Detail.mFsNumber);

    K2OS_File_Close(pPager->mFile);

    return 0;
}

void
KernPaging_FileSys_Arrived(
    K2OS_IFINST_ID aFileSysIfInstId
)
{
    K2OS_RPC_OBJ_HANDLE         hRpc;
    K2OS_RPC_CALLARGS           args;
    K2OS_FILESYS_GETDETAIL_OUT  detail;
    K2STAT                      stat;
    UINT32                      actualOut;
    K2OS_FSCLIENT               fsClient;
    char                        fsPath[16];
    VOLPAGER *                  pPager;
    K2OS_THREAD_TOKEN           tokThread;
    K2OS_FILE                   fsFile;

    hRpc = K2OS_Rpc_AttachByIfInstId(aFileSysIfInstId, NULL);
    if (NULL == hRpc)
    {
        K2OSKERN_Debug("***Pager: Failed to attach to filesystem with IfInstId %d\n", aFileSysIfInstId);
        return;
    }

    args.mpInBuf = NULL;
    args.mInBufByteCount = 0;
    args.mpOutBuf = (UINT8 *)&detail;
    args.mOutBufByteCount = sizeof(detail);
    args.mMethodId = K2OS_FileSys_Method_GetDetail;

    actualOut = 0;
    stat = K2OS_Rpc_Call(hRpc, &args, &actualOut);

    K2OS_Rpc_Release(hRpc);

    if ((K2STAT_IS_ERROR(stat)) || (actualOut != sizeof(K2OS_FILESYS_GETDETAIL_OUT)))
    {
        K2OSKERN_Debug("***Pager: Failed to get details of filesystem with IfInstId %d\n", aFileSysIfInstId);
        return;
    }

    if (!detail.mCanDoPaging)
        return;

    K2OSKERN_Debug("Pager: FileSys ifinstid %d number %d with flags %08X and attach context %d can do paging\n", aFileSysIfInstId, detail.mFsNumber, detail.mFsProv_Flags, detail.mAttachContext);

    //
    // set up the filesys paging thread
    //
    K2ASC_PrintfLen(fsPath, 15, "/fs/%d", detail.mFsNumber);
    fsPath[15] = 0;
    fsClient = K2OS_FsClient_Create();
    if (NULL == fsClient)
    {
        K2OSKERN_Debug("***Pager: Could not create fsClient to service filesystem number %d on IfInstId %d\n", detail.mFsNumber, aFileSysIfInstId);
        return;
    }

    if (!K2OS_FsClient_SetBaseDir(fsClient, fsPath))
    {
        K2OSKERN_Debug("***Pager: Could not set pager fsClient base dir to root of FileSys number %d\n", detail.mFsNumber);
    }
    else
    {
        fsFile = K2OS_FsClient_OpenFile(fsClient, "k2osvirt.mem", K2OS_ACCESS_RW, 0, K2OS_FileOpen_CreateOrTruncate, 0, K2_FSATTRIB_SYSTEM | K2_FSATTRIB_HIDDEN);
        if (NULL == fsFile)
        {
            K2OSKERN_Debug("***Pager: Could not create/truncate virtual memory file on FileSys number %d\n", detail.mFsNumber);
        }
        else
        {
            pPager = (VOLPAGER *)K2OS_Heap_Alloc(sizeof(VOLPAGER));
            if (NULL == pPager)
            {
                K2OSKERN_Debug("***Pager: memory alloc failed for FileSys number %d\n", detail.mFsNumber);
            }
            else
            {
                K2MEM_Zero(pPager, sizeof(VOLPAGER));

                pPager->mFile = fsFile;

                K2MEM_Copy(&pPager->Detail, &detail, sizeof(detail));

                K2ASC_PrintfLen(fsPath, 15, "Fs %d Pager", detail.mFsNumber);
                fsPath[15] = 0;

                tokThread = K2OS_Thread_Create(fsPath, KernPaging_FsThread, pPager, NULL, NULL);
                if (NULL == tokThread)
                {
                    K2OSKERN_Debug("***Pager: could not create service thread for fsnumber %d\n", detail.mFsNumber);
                    K2OS_Heap_Free(pPager);
                    pPager = NULL;
                }
                else
                {
                    K2OS_Token_Destroy(tokThread);
                }
            }
            if (NULL == pPager)
            {
                K2OS_File_Close(fsFile);
            }
        }
    }

    K2OS_FsClient_Destroy(fsClient);
}

void
KernPaging_FileSys_Departed(
    K2OS_IFINST_ID aFileSysIfInstId
)
{
    K2OSKERN_Debug("%s - %d\n", __FUNCTION__, aFileSysIfInstId);
}

UINT32
KernPaging_Thread(
    void *apArg
)
{
    static const K2_GUID128 sFileSysIface = K2OS_IFACE_FILESYS;
    K2OS_WaitResult     waitResult;
    UINT32              pagingChain;
    K2OS_IFSUBS_TOKEN   tokIfSubs;
    K2OS_WAITABLE_TOKEN tokWait[2];
    K2OS_MSG            msg;

    //
    // set up wait
    //
    tokWait[0] = gData.Paging.mTokSignal;
    tokWait[1] = K2OS_Mailbox_Create();
    K2_ASSERT(NULL != tokWait[1]);

    //
    // we need to know about file systems arriving and departing
    //
    tokIfSubs = K2OS_IfSubs_Create(tokWait[1], K2OS_IFACE_CLASSCODE_FILESYS, &sFileSysIface, 4, TRUE, NULL);
    K2_ASSERT(NULL != tokIfSubs);

    // 
    // started
    //
    K2OS_Notify_Signal((K2OS_SIGNAL_TOKEN)apArg);

    do
    {
        K2OS_Thread_WaitMany(&waitResult, 2, tokWait, FALSE, K2OS_TIMEOUT_INFINITE);

        if (waitResult == K2OS_Wait_Signalled_0)
        {
            // 
            // paging signal
            //
            K2OSKERN_Debug("<pager wake>\n");
            pagingChain = K2ATOMIC_Exchange(&gData.Paging.mListHead, 0);
            K2OSKERN_Debug("paging chain = %08X\n", pagingChain);
        }
        else
        {
            // 
            // interface subscription pop
            //
            K2_ASSERT(waitResult == (K2OS_Wait_Signalled_0 + 1));
            if (K2OS_Mailbox_Recv(tokWait[1], &msg))
            {
                if (msg.mMsgType == K2OS_SYSTEM_MSGTYPE_IFINST)
                {
                    if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                    {
                        // file systems must be in the kernel
                        if (0 == msg.mPayload[2])
                        {
                            KernPaging_FileSys_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                        }
                    }
                    else if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_DEPART)
                    {
                        // file system providers must be in the kernel
                        if (0 == msg.mPayload[2])
                        {
                            KernPaging_FileSys_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                        }
                    }
                    else
                    {
                        K2OSKERN_Debug("*** Pager received unexpected IFINST message (%04X)\n", msg.mShort);
                    }
                }
                else
                {
                    K2OSKERN_Debug("*** Pager received unexpected message type (%04X)\n", msg.mMsgType);
                }
            }
            else
            {
                K2OSKERN_Debug("*** Pager mailbox signalled but msg recv failed\n");
            }
        }

    } while (1);

    K2OSKERN_Panic("Pager Thread loop exited\n");

    return 0;
}

void    
KernPaging_Init(
    void
)
{
    K2STAT              stat;
    K2OS_THREAD_TOKEN   tokThread;
    K2OS_SIGNAL_TOKEN   tokStarted;
    K2OS_WaitResult     waitResult;

    stat = KernNotify_Create(FALSE, &gData.Paging.NotifyRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Panic("*** Could not create paging notify\n");
    }

    stat = KernToken_Create(gData.Paging.NotifyRef.AsAny, &gData.Paging.mTokSignal);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Panic("*** Could not create paging notify token\n");
    }

    tokStarted = K2OS_Notify_Create(FALSE);
    K2_ASSERT(NULL != tokStarted);

    tokThread = K2OS_Thread_Create("Pager", KernPaging_Thread, (void *)tokStarted, NULL, NULL);
    K2_ASSERT(NULL != tokThread);

    K2OS_Thread_WaitOne(&waitResult, tokStarted, K2OS_TIMEOUT_INFINITE);

    K2OS_Token_Destroy(tokStarted);
    K2OS_Token_Destroy(tokThread);
}
