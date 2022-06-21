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

UINT_PTR            gMainThreadId;
K2OS_MAILBOX        gMainMailbox;
K2OS_MAILSLOT_TOKEN gMainMailslotToken;

void
RootMain_RecvMsg(
    K2OS_MSG const * apMsg
)
{
    switch (apMsg->mControl)
    {
    case K2OS_SYSTEM_MSG_IPC_REQUEST:
        //
        // attempt to connect to us over IPC
        //
        // msg.mPayload[0] is interface publish context
        // msg.mPayload[1] is requestor process id
        // msg.mPayload[2] is global request id
        //
        K2OS_Ipc_RejectRequest(apMsg->mPayload[2], K2STAT_ERROR_REJECTED);
        break;

    default:
        RootDebug_Printf("RootMain_RecvMsg(%08X,%08X,%08X,%08X)\n", apMsg->mControl, apMsg->mPayload[0], apMsg->mPayload[1], apMsg->mPayload[2]);
        break;
    }
}

UINT32
MainThread(
    char const *apArgs
)
{
    UINT_PTR        waitResult;
    K2OS_MSG        msg;

    gMainThreadId = K2OS_Thread_GetId();

    gMainMailbox = K2OS_Mailbox_Create(&gMainMailslotToken);
    K2_ASSERT(NULL != gMainMailbox);

    RootRes_Init();
    RootDev_Init();
    RootDriver_Init();
    RootAcpi_Init();
    RootDevMgr_Init();
    
    RootAcpi_Enable();
    RootNodeAcpiDriver_Start();

    do {
        waitResult = K2OS_Wait_OnMailbox(gMainMailbox, &msg, K2OS_TIMEOUT_INFINITE);
        if (waitResult != K2OS_THREAD_WAIT_MAILBOX_SIGNALLED)
        {
            RootDebug_Printf("*** Root wait on mailbox failed with unexpected status 0x%08X\n", waitResult);
            break;
        }
        if (!K2OS_System_ProcessMsg(&msg))
        {
            //
            // received a non-system message
            //
            RootMain_RecvMsg(&msg);
        }
    } while (1);

    //
    // root should never exit if it tries to it is an error
    //
    K2OS_RaiseException(K2STAT_EX_LOGIC);
    return 0;
}

