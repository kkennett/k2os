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

int
RunProgram(
    char const *    apWorkDir,
    char const *    apCmdLine,
    char const *    apEnviron,
    char **         appRetOutput,
    UINT_PTR *      apRetOutputLen
)
{
    STARTUPINFO         startInfo;
    PROCESS_INFORMATION procInfo;
    SECURITY_ATTRIBUTES sa;
    HANDLE              hWriteToChildStdIn;
    HANDLE              hReadFromChildStdIn;
    HANDLE              hChildStdIn;
    HANDLE              hChildOutput;
    HANDLE              hChildStdOutErr;
    int                 result;
    char *              pUseCmdLine;

    K2_ASSERT(NULL != apCmdLine);

    result = (int)strlen(apCmdLine);
    pUseCmdLine = new char[((size_t)result) + 1];
    if (NULL == pUseCmdLine)
        return ERROR_OUTOFMEMORY;
    if (0 != result)
        CopyMemory(pUseCmdLine, apCmdLine, result * sizeof(char));
    pUseCmdLine[result] = 0;

    do {
        ZeroMemory(&sa, sizeof(sa));
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        sa.nLength = sizeof(sa);

        // making &sa non null means handles CAN be inherited
        if (!CreatePipe(&hReadFromChildStdIn, &hWriteToChildStdIn, &sa, 0))
        {
            result = GetLastError();
            break;
        }

        result = E_FAIL;

        do {
            if (!DuplicateHandle(
                GetCurrentProcess(), hWriteToChildStdIn,
                GetCurrentProcess(), &hChildStdIn,
                0, TRUE, DUPLICATE_SAME_ACCESS))
            {
                result = GetLastError();
                break;
            }

            do {
                char *pPath = MakeTempFileName(NULL, NULL);
                if (NULL == pPath)
                {
                    result = ERROR_OUTOFMEMORY;
                    break;
                }

                hChildOutput = CreateFile(
                    pPath,
                    GENERIC_READ | GENERIC_WRITE,
                    0, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
                    NULL);

                result = GetLastError();

                delete[] pPath;

                if (hChildOutput == INVALID_HANDLE_VALUE)
                    break;

                result = 0;

                do {
                    if (!DuplicateHandle(
                        GetCurrentProcess(), hChildOutput,
                        GetCurrentProcess(), &hChildStdOutErr,
                        0, TRUE, DUPLICATE_SAME_ACCESS))
                    {
                        result = GetLastError();
                        break;
                    }

                    do {
                        ZeroMemory(&startInfo, sizeof(startInfo));
                        startInfo.cb = sizeof(startInfo);
                        startInfo.dwFlags = STARTF_USESTDHANDLES;
                        startInfo.hStdInput = hChildStdIn;
                        startInfo.hStdOutput = hChildStdOutErr;
                        startInfo.hStdError = hChildStdOutErr;

                        ZeroMemory(&procInfo, sizeof(procInfo));

                        if (!CreateProcess(
                            NULL,
                            pUseCmdLine,
                            NULL, NULL, TRUE,
                            NORMAL_PRIORITY_CLASS | CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW,
                            (LPVOID)apEnviron,
                            apWorkDir,
                            &startInfo,
                            &procInfo))
                        {
                            result = GetLastError();
                            break;
                        }

                        WaitForSingleObject(procInfo.hProcess, INFINITE);

                        if (!GetExitCodeProcess(procInfo.hProcess, (LPDWORD)&result))
                        {
                            result = GetLastError();
                        }
                        else
                        {
                            DWORD fileBytes;
                            fileBytes = GetFileSize(hChildOutput, NULL);
                            if ((0 == fileBytes) ||
                                (INVALID_FILE_SIZE == fileBytes))
                            {
                                *appRetOutput = NULL;
                                *apRetOutputLen = 0;
                            }
                            else
                            {
                                //
                                // output should be available here
                                //
                                HANDLE hMap = CreateFileMapping(hChildOutput,
                                    NULL, PAGE_READONLY, 0, fileBytes, NULL);
                                if (NULL == hMap)
                                {
                                    *appRetOutput = NULL;
                                    *apRetOutputLen = 0;
                                }
                                else
                                {
                                    char const *pData;
                                    pData = (char const *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, fileBytes);
                                    if (NULL == pData)
                                    {
                                        *appRetOutput = NULL;
                                        *apRetOutputLen = 0;
                                    }
                                    else
                                    {
                                        *appRetOutput = new char[(size_t)fileBytes + 1];
                                        if (NULL == *appRetOutput)
                                        {
                                            *apRetOutputLen = 0;
                                        }
                                        else
                                        {
                                            CopyMemory(*appRetOutput, pData, fileBytes);
                                            *((*appRetOutput) + fileBytes) = 0;
                                            *apRetOutputLen = fileBytes;
                                        }
                                        UnmapViewOfFile(pData);
                                    }
                                    CloseHandle(hMap);
                                }
                            }
                        }

                        CloseHandle(procInfo.hThread);
                        CloseHandle(procInfo.hProcess);

                    } while (0);

                    CloseHandle(hChildStdOutErr);

                } while (0);

                CloseHandle(hChildOutput);

            } while (0);

            CloseHandle(hChildStdIn);

        } while (0);

        CloseHandle(hReadFromChildStdIn);
        CloseHandle(hWriteToChildStdIn);

    } while (0);

    if (NULL != pUseCmdLine)
        delete[] pUseCmdLine;

    return result;
}

