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

#include "ik2osdrv.h"

K2OS_MAILBOX        gMailbox;
UINT_PTR            gRootRpcInterfaceInstanceId;
UINT_PTR            gHostUsage;
K2OS_XDL            gDriverXdl;
K2OS_THREAD_TOKEN   gDriverThreadToken;
UINT_PTR            gDriverThreadId;

void
K2OSDRV_FindRootRpc(
    void
)
{
    static K2_GUID128 const sRpcServerIfaceId = K2OS_IFACE_ID_RPC_SERVER;

    K2OS_IFENUM_TOKEN   ifEnumToken;
    UINT_PTR            entryCount;
    K2OS_IFENUM_ENTRY   ifEntry;
    BOOL                ok;

    ifEnumToken = K2OS_IfEnum_Create(1, K2OS_IFCLASS_RPC, &sRpcServerIfaceId);
    if (NULL == ifEnumToken)
    {
        K2OSDrv_DebugPrintf("ifEnum error 0x%08X\n", K2OS_Thread_GetLastStatus());
    }
    K2_ASSERT(NULL != ifEnumToken);

    entryCount = 1;
    ok = K2OS_IfEnum_Next(ifEnumToken, &ifEntry, &entryCount);
    K2_ASSERT(ok);
    K2_ASSERT(entryCount == 1);

    gRootRpcInterfaceInstanceId = ifEntry.mGlobalInstanceId;

    K2OS_Token_Destroy(ifEnumToken);
}

void
K2OSDRV_ConnectToRoot(
    void
)
{
    static K2_GUID128 const sDriverHostObjectClassId = K2OSDRV_HOSTOBJECT_CLASS_ID;

    K2OSDRV_SET_MAILSLOT_IN setMailslotIn;
    K2STAT                  stat;
    K2OSRPC_CALLARGS        args;

    if (!K2OSRPC_Object_Create(gRootRpcInterfaceInstanceId, &sDriverHostObjectClassId, 0, NULL, &gHostUsage))
    {
        K2OSDrv_DebugPrintf("***Failed to create driver host object for driver process %d (stat=%08X)\n", K2OS_Process_GetId(), K2OS_Thread_GetLastStatus());
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    K2MEM_Zero(&setMailslotIn, sizeof(setMailslotIn));
    gMailbox = K2OS_Mailbox_Create(&setMailslotIn.mMailslotToken);
    if ((NULL == gMailbox) || (NULL == setMailslotIn.mMailslotToken))
    {
        K2OSDrv_DebugPrintf("***Failed to create mailbox for driver process %d (stat=%08X)\n", K2OS_Process_GetId(), K2OS_Thread_GetLastStatus());
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    args.mMethodId = K2OSDRV_HOSTOBJECT_METHOD_SET_MAILSLOT;
    args.mpInBuf = (UINT8 const *)&setMailslotIn;
    args.mInBufByteCount = sizeof(setMailslotIn);
    args.mpOutBuf = NULL;
    args.mOutBufByteCount = 0;
    stat = K2OSRPC_Object_Call(gHostUsage, &args, NULL);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("***Failed to set mailbox for driver process %d (stat=%08X)\n", K2OS_Process_GetId(), stat);
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    //
    // gave mailslot token to root. we dont need it any more
    //
    K2OS_Token_Destroy(setMailslotIn.mMailslotToken);
}

UINT_PTR
K2OSDRV_DriverThreadEntry(
    void *apArgs
)
{
    char const *            pArgs;
    char const *            pHold;
    char                    ch;
    UINT_PTR                len;
    char *                  pXdlName;
    UINT_PTR                busDriverProcessId;
    UINT_PTR                busDriverChildObjectId;
    K2OSDrv_DriverThread    driverThreadFunc;

    pArgs = (char const *)apArgs;
    K2_ASSERT(NULL != pArgs);
    K2_ASSERT(0 != *pArgs);
    do {
        ch = *pArgs;
        if ((0 == ch) ||
            (' ' == ch) ||
            ('\t' == ch) ||
            ('\n' == ch) ||
            ('\r' == ch))
            break;
        pArgs++;
    } while (1);
    len = (UINT_PTR)(pArgs - ((char const *)apArgs));
    if (len == 0)
    {
        K2OSDrv_DebugPrintf("*** Arguments to run driver invalid.\n");
        K2OS_Process_Exit(K2STAT_ERROR_BAD_ARGUMENT);
    }
    pXdlName = (char *)K2OS_Heap_Alloc((len + 4) & ~3);
    if (NULL == pXdlName)
    {
        K2OSDrv_DebugPrintf("*** memory alloc for driver xdl name failed.\n");
        K2OS_Process_Exit(K2STAT_ERROR_OUT_OF_MEMORY);
    }
    K2MEM_Copy(pXdlName, apArgs, len);
    pXdlName[len] = 0;

    gDriverXdl = K2OS_Xdl_Acquire(pXdlName);
    if (NULL == gDriverXdl)
    {
        // file not found most probably
//        K2OSDrv_DebugPrintf("*** driver xdl acquire failed.\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }
    K2OS_Heap_Free(pXdlName);

    driverThreadFunc = (K2OSDrv_DriverThread)K2OS_Xdl_FindExport(gDriverXdl, TRUE, "DriverThread");
    if (NULL == driverThreadFunc)
    {
        K2OSDrv_DebugPrintf("*** driver did not have required 'DriverThread' export.\n");
        K2OS_Xdl_Release(gDriverXdl);
        gDriverXdl = NULL;
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    busDriverProcessId = 0;
    busDriverChildObjectId = 0;

    if (0 != ch)
    {
        //
        // eat space before next argument
        //
        do {
            ch = *pArgs;
            if ((0 != ch) &&
                (' ' != ch) &&
                ('\t' != ch) &&
                ('\n' != ch) &&
                ('\r' != ch))
                break;
            pArgs++;
        } while (1);

        //
        // if we're not at the end of the command line, next arg is the bus driver process id
        //
        if (0 != ch)
        {
            pHold = pArgs;
            pArgs++;
            do {
                ch = *pArgs;
                if ((0 == ch) ||
                    (' ' == ch) ||
                    ('\t' == ch) ||
                    ('\n' == ch) ||
                    ('\r' == ch))
                    break;
                pArgs++;
            } while (1);

            len = (UINT_PTR)(pArgs - pHold);
            busDriverProcessId = K2ASC_NumValue32Len(pHold, len);

            if (0 != ch)
            {
                //
                // eat space before next argument
                //
                do {
                    ch = *pArgs;
                    if ((0 != ch) &&
                        (' ' != ch) &&
                        ('\t' != ch) &&
                        ('\n' != ch) &&
                        ('\r' != ch))
                        break;
                    pArgs++;
                } while (1);

                //
                // if we're not at the end of the command line, next arg is the bus driver child object id
                //
                if (0 != ch)
                {
                    pHold = pArgs;
                    pArgs++;
                    do {
                        ch = *pArgs;
                        if ((0 == ch) ||
                            (' ' == ch) ||
                            ('\t' == ch) ||
                            ('\n' == ch) ||
                            ('\r' == ch))
                            break;
                        pArgs++;
                    } while (1);
                    len = (UINT_PTR)(pArgs - pHold);
                    busDriverChildObjectId = K2ASC_NumValue32Len(pHold, len);
                }
            }
        }
    }

    len = driverThreadFunc(busDriverProcessId, busDriverChildObjectId);

    K2OSDrv_DebugPrintf("*** Driver thread exited with result 0x%08X\n", len);
    K2OS_Process_Exit(len);

    return len;
}

UINT32
MainThread(
    char const *apArgs
)
{
    UINT_PTR    waitResult;
    K2OS_MSG    msg;

    K2OSDrv_DebugPrintf("Process %d - K2OSDRV_MainThread(\"%s\")\n", K2OS_Process_GetId(), apArgs);

    K2OSDRV_FindRootRpc();

    K2OSDRV_ConnectToRoot();

    do {
        waitResult = K2OS_Wait_OnMailbox(gMailbox, &msg, K2OS_TIMEOUT_INFINITE);
        if (waitResult != K2OS_THREAD_WAIT_MAILBOX_SIGNALLED)
        {
            K2OSDrv_DebugPrintf("***Process %d - Driver failed wait on mailbox (result=%08X)\n", K2OS_Process_GetId(), waitResult);
            break;
        }
        if (!K2OS_System_ProcessMsg(&msg))
        {
            switch (msg.mControl)
            {
            case K2OS_MSG_CONTROL_DRV_START:
//                K2OSDrv_DebugPrintf("Driver in process %d recv start command\n", K2OS_Process_GetId());
                gDriverThreadToken = K2OS_Thread_Create(K2OSDRV_DriverThreadEntry, (void *)apArgs, NULL, &gDriverThreadId);
                if (NULL == gDriverThreadToken)
                {
                    K2OSDrv_DebugPrintf("***Driver in process %d thread failed to start error 0x%08X\n", K2OS_Process_GetId(), K2OS_Thread_GetLastStatus());
                    K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
                }
                break;

            default:
                K2OSDrv_DebugPrintf("***Driver process %d recv unexpected msg with control=%08X\n", K2OS_Process_GetId(), msg.mControl);
                break;
            }
        }
    } while (1);

    K2OSRPC_Object_Release(gHostUsage);

    K2OS_Mailbox_Delete(gMailbox);

    K2OS_Token_Destroy(gDriverThreadToken);

    return 0;
}

