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
#include <lib/k2win32.h>
#include <AccCtrl.h>
#include <AclAPI.h>

#define PIPE_CHUNK_BUFFER_BYTES 1024

void PipeServerDataPort::RunConnected(bool *apStopNow)
{
    HANDLE              h[4];
    DataPortBuffer *    pCurrentRead;
    DataPortBuffer *    pCurrentWrite;
    DWORD               waitResult;
    bool                ok;
    DWORD               ioBytes;
    int                 gle;

    ResetEvent(mhManualDisconnectEvent);
    ResetEvent(mhDisconnectedEvent);
    SetEvent(mhConnectedEvent);

    pCurrentRead = pCurrentWrite = NULL;

    h[0] = mhStopEvent;
    h[1] = mhManualDisconnectEvent;
    h[2] = mhReadCompletedEvent;

    do {
        //
        // empty input until we have an outstanding read
        //
        while (NULL == pCurrentRead)
        {
            pCurrentRead = DataPortBuffer::Alloc(PIPE_CHUNK_BUFFER_BYTES);
            if (NULL == pCurrentRead)
                break;
            ioBytes = 0;
            ZeroMemory(&mOvRecv, sizeof(mOvRecv));
            ResetEvent(mhReadCompletedEvent);
            mOvRecv.hEvent = mhReadCompletedEvent;
            ok = ReadFile(mhPipe, pCurrentRead->Ptr(), (DWORD)pCurrentRead->SizeBytes(), &ioBytes, &mOvRecv);
            if (ok)
            {
                pCurrentRead->SetFull(ioBytes);
                mInQueue.Queue(pCurrentRead);
                pCurrentRead = NULL;
            }
            else
            {
                gle = GetLastError();
                if (gle != ERROR_IO_PENDING)
                {
                    delete pCurrentRead;
                    pCurrentRead = NULL;
                    break;
                }
            }
        }
        if (NULL == pCurrentRead)
        {
            break;
        }

        //
        // shove output until we have an outstanding write
        //
        gle = 0;
        if (NULL == pCurrentWrite)
        {
            do {
                pCurrentWrite = mOutQueue.Dequeue();
                if (NULL == pCurrentWrite)
                {
                    h[3] = mOutQueue.NotEmptyEvent();
                    break;
                }
                ioBytes = 0;
                ZeroMemory(&mOvSend, sizeof(mOvSend));
                ResetEvent(mhWriteCompletedEvent);
                mOvSend.hEvent = mhWriteCompletedEvent;
                ok = WriteFile(mhPipe, pCurrentWrite->Ptr(), (DWORD)pCurrentWrite->FullBytes(), &ioBytes, &mOvSend);
                if (ok)
                {
                    if (ioBytes != pCurrentWrite->FullBytes())
                    {
                        DebugBreak();
                    }
                    delete pCurrentWrite;
                    pCurrentWrite = NULL;
                }
                else
                {
                    gle = GetLastError();
                    if (gle != ERROR_IO_PENDING)
                    {
                        delete pCurrentWrite;
                        pCurrentWrite = NULL;
                        break;
                    }
                    h[3] = mhWriteCompletedEvent;
                    gle = 0;
                }
            } while (NULL == pCurrentWrite);
        }
        else
        {
            h[3] = mOutQueue.NotEmptyEvent();
        }
        if (0 != gle)
        {
            break;
        }

        waitResult = WaitForMultipleObjects(4, h, FALSE, INFINITE);
        if (waitResult == WAIT_FAILED)
        {
            break;
        }
        if (waitResult == WAIT_OBJECT_0)
        {
            //
            // stop event
            //
            *apStopNow = true;
            break;
        }
        else if (waitResult == WAIT_OBJECT_0 + 1)
        {
            //
            // manual close event
            //
            break;
        }
        else if (waitResult == WAIT_OBJECT_0 + 2)
        {
            //
            // read completed event
            //
            if (NULL == pCurrentRead)
            {
                DebugBreak();
            }
            ok = GetOverlappedResult(mhPipe, &mOvRecv, &ioBytes, TRUE);
            if (!ok)
            {
                delete pCurrentRead;
                pCurrentRead = NULL;
                break;
            }
            pCurrentRead->SetFull(ioBytes);
            mInQueue.Queue(pCurrentRead);
            pCurrentRead = NULL;
        }
        else if (waitResult == WAIT_OBJECT_0 + 3)
        {
            if (NULL != pCurrentWrite)
            {
                //
                // async write completed
                //
                ioBytes = 0;
                ok = GetOverlappedResult(mhPipe, &mOvSend, &ioBytes, TRUE);
                if (!ok)
                {
                    delete pCurrentWrite;
                    pCurrentWrite = NULL;
                    break;
                }
                if (ioBytes != pCurrentWrite->FullBytes())
                {
                    DebugBreak();
                }
                delete pCurrentWrite;
                pCurrentWrite = NULL;
            }
            else
            {
                //
                // new write buffer appeared in output queue
                //
            }
        }
        else
        {
            DebugBreak();
        }
    } while (1);

    DisconnectNamedPipe(mhPipe);

    if (NULL != pCurrentRead)
    {
        CancelIoEx(mhPipe, &mOvRecv);
        WaitForSingleObject(mhReadCompletedEvent, INFINITE);
        delete pCurrentRead;
        pCurrentRead = NULL;
    }

    if (NULL != pCurrentWrite)
    {
        CancelIoEx(mhPipe, &mOvSend);
        WaitForSingleObject(mhWriteCompletedEvent, INFINITE);
        delete pCurrentWrite;
        pCurrentWrite = NULL;
    }

    mInQueue.Purge();
    mOutQueue.Purge();

    ResetEvent(mhConnectedEvent);
    SetEvent(mhDisconnectedEvent);
}

void PipeServerDataPort::RunWaitConnect(bool *apStopNow)
{
    HANDLE h[2];

    h[0] = mhConnectedEvent;
    h[1] = mhStopEvent;

    DWORD waitResult;

    do {
        waitResult = WaitForMultipleObjects(2, h, FALSE, INFINITE);
        
        if (waitResult == WAIT_OBJECT_0)
        {
            RunConnected(apStopNow);
            break;
        }

        if (waitResult == WAIT_OBJECT_0 + 1)
        {
            *apStopNow = TRUE;
            break;
        }

        DebugBreak();

    } while (1);
}

DWORD PipeServerDataPort::Thread(void)
{
    BOOL ok;
    int gle;
    bool stopNow;
    
    stopNow = false;

    do {
        ZeroMemory(&mOvConnect,sizeof(mOvConnect));
        mOvConnect.hEvent = mhConnectedEvent;

        ok = ConnectNamedPipe(mhPipe, &mOvConnect);
        gle = GetLastError();

        if ((ok) || (gle == ERROR_PIPE_CONNECTED))
        {
            RunConnected(&stopNow);
        }
        else
        {
            if (gle != ERROR_IO_PENDING)
            {
                DisconnectNamedPipe(mhPipe);
                Sleep(1000);
            }
            else
            {
                RunWaitConnect(&stopNow);
            }
        }
    } while (!stopNow);

    return 0;
}

DWORD WINAPI PipeServerDataPort::sThread(void *apParam)
{
    return ((PipeServerDataPort *)apParam)->Thread();
}

PipeServerDataPort * 
PipeServerDataPort::Open(
    char const *apPipePath
)
{
    bool ok;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR *psd;
    PSID EveryoneSID;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    HANDLE hPipe;
    PipeServerDataPort *pPort;

    if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
        SECURITY_WORLD_RID,
        0, 0, 0, 0, 0, 0, 0,
        &EveryoneSID))
    {
        return NULL;
    }

    ok = false;
    hPipe = INVALID_HANDLE_VALUE;

    do {
        ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = FALSE;

        // Set up ACE
        EXPLICIT_ACCESS ace = { 0 };
        ace.grfAccessMode = SET_ACCESS;
        ace.grfAccessPermissions = GENERIC_ALL;
        ace.grfInheritance = NO_INHERITANCE;
        ace.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ace.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ace.Trustee.ptstrName = (LPTSTR)EveryoneSID;

        PACL acl = NULL;
        if (ERROR_SUCCESS != SetEntriesInAcl(1, &ace, nullptr, &acl))
            break;

        do {
            // Create security descriptor.
            psd = (SECURITY_DESCRIPTOR *)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
            if (NULL == psd)
                break;

            do {
                if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION))
                    break;

                if (!SetSecurityDescriptorDacl(psd, TRUE, acl, FALSE))
                    break;

                // Set security descriptor in security attributes
                sa.lpSecurityDescriptor = psd;

                hPipe = CreateNamedPipe(
                    apPipePath,
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                    1,
                    0,
                    PIPE_CHUNK_BUFFER_BYTES * 4,
                    0,
                    &sa);

                if (INVALID_HANDLE_VALUE != hPipe)
                    ok = true;

            } while (0);
        
            LocalFree(psd);

        } while (0);

        LocalFree(acl);

    } while (0);

    FreeSid(EveryoneSID);

    if (!ok)
        return NULL;

    ok = false;
    pPort = NULL;

    do {
        HANDLE hConnectedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == hConnectedEvent)
            break;

        do {

            HANDLE hDisconnectedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
            if (NULL == hDisconnectedEvent)
                break;

            do {

                HANDLE hReadCompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                if (NULL == hReadCompleteEvent)
                    break;

                do {

                    HANDLE hWriteCompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                    if (NULL == hWriteCompleteEvent)
                        break;

                    do {

                        HANDLE hManualDisconnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                        if (NULL == hManualDisconnectEvent)
                            break;

                        do {

                            HANDLE hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                            if (NULL == hStopEvent)
                                break;

                            do {
                                pPort = new PipeServerDataPort(
                                    hPipe,
                                    hConnectedEvent,
                                    hDisconnectedEvent,
                                    hReadCompleteEvent,
                                    hWriteCompleteEvent,
                                    hManualDisconnectEvent,
                                    hStopEvent
                                );
                                if (NULL == pPort)
                                    break;

                                ok = true;

                            } while (0);

                            if (!ok)
                            {
                                CloseHandle(hStopEvent);
                            }

                        } while (0);

                        if (!ok)
                        {
                            CloseHandle(hManualDisconnectEvent);
                        }

                    } while (0);

                    if (!ok)
                    {
                        CloseHandle(hWriteCompleteEvent);
                    }

                } while (0);

                if (!ok)
                {
                    CloseHandle(hReadCompleteEvent);
                }

            } while (0);

            if (!ok)
            {
                CloseHandle(hDisconnectedEvent);
            }

        } while (0);

        if (!ok)
        {
            CloseHandle(hConnectedEvent);
        }

    } while (0);

    if (!ok)
    {
        CloseHandle(hPipe);
        return NULL;
    }

    if ((!pPort->mInQueue.Init()) ||
        (!pPort->mOutQueue.Init()))
    {
        delete pPort;
        return NULL;
    }

    //
    // can start port thread now
    //
    pPort->mhThread = CreateThread(NULL, 0, PipeServerDataPort::sThread, pPort, 0, &pPort->mThreadId);
    if (NULL == pPort->mhThread)
    {
        delete pPort;
        return NULL;
    }

    return pPort;
}

void
PipeServerDataPort::Disconnect(
    void
)
{
    SetEvent(mhManualDisconnectEvent);
    if (GetCurrentThreadId() != mThreadId)
    {
        WaitForSingleObject(mhDisconnectedEvent, INFINITE);
    }
}

