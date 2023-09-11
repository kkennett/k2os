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
#include "fatwork.h"

BOOL
K2OS_RwLock_Init(
    K2OS_RWLOCK *apRwLock
)
{
    BOOL ok;

    if (NULL == apRwLock)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    ok = FALSE;

    do {
        InitializeCriticalSection(&apRwLock->CsReaderOuter);

        do {
            InitializeCriticalSection(&apRwLock->CsReaderChain);

            do {
                InitializeCriticalSection(&apRwLock->CsReaderFirst);

                do {
                    InitializeCriticalSection(&apRwLock->CsWriter);

                    do {
                        InitializeCriticalSection(&apRwLock->CsWriterFirst);

                        apRwLock->mReadersCount = 0;
                        apRwLock->mWritersCount = 0;
                        ok = TRUE;

                    } while (0);

                    if (!ok)
                    {
                        DeleteCriticalSection(&apRwLock->CsWriter);
                    }

                } while (0);

                if (!ok)
                {
                    DeleteCriticalSection(&apRwLock->CsReaderFirst);
                }

            } while (0);

            if (!ok)
            {
                DeleteCriticalSection(&apRwLock->CsReaderChain);
            }

        } while (0);

        if (!ok)
        {
            DeleteCriticalSection(&apRwLock->CsReaderOuter);
        }

    } while (0);

    return ok;
}

BOOL
K2OS_RwLock_ReadLock(
    K2OS_RWLOCK *apRwLock
)
{
    if (NULL == apRwLock)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    EnterCriticalSection(&apRwLock->CsReaderOuter);
    EnterCriticalSection(&apRwLock->CsReaderChain);
    EnterCriticalSection(&apRwLock->CsReaderFirst);
    if (0 == apRwLock->mReadersCount++)
    {
        EnterCriticalSection(&apRwLock->CsWriter);
    }
    LeaveCriticalSection(&apRwLock->CsReaderFirst);
    LeaveCriticalSection(&apRwLock->CsReaderChain);
    LeaveCriticalSection(&apRwLock->CsReaderOuter);

    return TRUE;
}

BOOL
K2OS_RwLock_ReadUnlock(
    K2OS_RWLOCK *apRwLock
)
{
    if (NULL == apRwLock)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    EnterCriticalSection(&apRwLock->CsReaderFirst);
    if (0 == --apRwLock->mReadersCount)
    {
        LeaveCriticalSection(&apRwLock->CsWriter);
    }
    LeaveCriticalSection(&apRwLock->CsReaderFirst);

    return TRUE;
}

BOOL
K2OS_RwLock_WriteLock(
    K2OS_RWLOCK *apRwLock
)
{
    if (NULL == apRwLock)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    EnterCriticalSection(&apRwLock->CsWriterFirst);
    if (0 == apRwLock->mWritersCount++)
    {
        EnterCriticalSection(&apRwLock->CsReaderChain);
    }
    LeaveCriticalSection(&apRwLock->CsWriterFirst);
    EnterCriticalSection(&apRwLock->CsWriter);

    return TRUE;
}

BOOL
K2OS_RwLock_WriteUnlock(
    K2OS_RWLOCK *apRwLock
)
{
    if (NULL == apRwLock)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    LeaveCriticalSection(&apRwLock->CsWriter);
    EnterCriticalSection(&apRwLock->CsWriterFirst);
    if (0 == --apRwLock->mWritersCount)
    {
        LeaveCriticalSection(&apRwLock->CsReaderChain);
    }
    LeaveCriticalSection(&apRwLock->CsWriterFirst);

    return TRUE;
}

BOOL
K2OS_RwLock_Done(
    K2OS_RWLOCK *apRwLock
)
{
    if (NULL == apRwLock)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    DeleteCriticalSection(&apRwLock->CsWriterFirst);
    InitializeCriticalSection(&apRwLock->CsWriter);
    InitializeCriticalSection(&apRwLock->CsReaderFirst);
    InitializeCriticalSection(&apRwLock->CsReaderChain);
    InitializeCriticalSection(&apRwLock->CsReaderOuter);
    apRwLock->mReadersCount = 0;
    apRwLock->mWritersCount = 0;

    return TRUE;
}

