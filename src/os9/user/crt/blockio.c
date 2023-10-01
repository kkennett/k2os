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
#include "crtuser.h"

K2OS_BLOCKIO_TOKEN          
K2OS_BlockIo_Attach(
    K2OS_IFINST_ID      aIfInstId,
    UINT32              aAccess,
    UINT32              aShare,
    K2OS_MAILBOX_TOKEN  aTokNotifyMailbox
)
{
    if ((0 == aIfInstId) ||
        (0 == (aAccess & K2OS_ACCESS_RW)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }
    return (K2OS_BLOCKIO_TOKEN)CrtKern_SysCall4(K2OS_SYSCALL_ID_BLOCKIO_ATTACH, aIfInstId, aAccess, aShare, (UINT32)aTokNotifyMailbox);
}

BOOL                        
K2OS_BlockIo_GetMedia(
    K2OS_BLOCKIO_TOKEN  aTokBlockIo,
    K2OS_STORAGE_MEDIA *apRetMedia
)
{
    BOOL                result;
    K2OS_THREAD_PAGE *  pThreadPage;

    if (NULL == aTokBlockIo)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    result = (BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_BLOCKIO_GETMEDIA, (UINT32)aTokBlockIo);
    if (!result)
    {
        return FALSE;
    }

    if (NULL != apRetMedia)
    {
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

        K2MEM_Copy(apRetMedia, &pThreadPage->mMiscBuffer, sizeof(K2OS_STORAGE_MEDIA));
    }

    return TRUE;
}

K2OS_BLOCKIO_RANGE          
K2OS_BlockIo_RangeCreate(
    K2OS_BLOCKIO_TOKEN  aTokBlockIo,
    UINT32              aRangeBase,
    UINT32              aRangeBlockCount,
    BOOL                aMakePrivate
)
{
    if ((NULL == aTokBlockIo) ||
        (0 == aRangeBlockCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return 0;
    }
    return (K2OS_BLOCKIO_RANGE)CrtKern_SysCall4(
        K2OS_SYSCALL_ID_BLOCKIO_RANGE_CREATE,
        (UINT32)aTokBlockIo,
        aRangeBase,
        aRangeBlockCount,
        (UINT32)aMakePrivate);
}

BOOL                        
K2OS_BlockIo_RangeDelete(
    K2OS_BLOCKIO_TOKEN  aTokBlockIo,
    K2OS_BLOCKIO_RANGE  aRange
)
{
    if ((NULL == aTokBlockIo) ||
        (NULL == aRange))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }
    return (BOOL)CrtKern_SysCall2(
        K2OS_SYSCALL_ID_BLOCKIO_RANGE_CREATE,
        (UINT32)aTokBlockIo,
        (UINT32)aRange);
}

BOOL
K2OS_BlockIo_Read(
    K2OS_BLOCKIO_TOKEN  aTokBlockIo,
    K2OS_BLOCKIO_RANGE  aBlockIoRange,
    UINT32              aBytesOffset,
    void *              apBuffer,
    UINT32              aByteCount
)
{
    if ((NULL == aTokBlockIo) ||
        (NULL == aBlockIoRange) ||
        (NULL == apBuffer) ||
        (0 == aByteCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }
    return (BOOL)CrtKern_SysCall6(K2OS_SYSCALL_ID_BLOCKIO_TRANSFER,
        (UINT32)aTokBlockIo,
        (UINT32)aBlockIoRange,
        (UINT32)aBytesOffset,
        (UINT32)aByteCount,
        0,
        (UINT32)apBuffer
    );
}

BOOL
K2OS_BlockIo_Write(
    K2OS_BLOCKIO_TOKEN  aTokBlockIo,
    K2OS_BLOCKIO_RANGE  aBlockIoRange,
    UINT32              aBytesOffset,
    void const *        apBuffer,
    UINT32              aByteCount
)
{
    if ((NULL == aTokBlockIo) ||
        (NULL == aBlockIoRange) ||
        (NULL == apBuffer) ||
        (0 == aByteCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }
    return (BOOL)CrtKern_SysCall6(K2OS_SYSCALL_ID_BLOCKIO_TRANSFER,
        (UINT32)aTokBlockIo,
        (UINT32)aBlockIoRange,
        (UINT32)aBytesOffset,
        (UINT32)aByteCount,
        1,
        (UINT32)apBuffer
    );
}

BOOL
K2OS_BlockIo_Erase(
    K2OS_BLOCKIO_TOKEN  aTokBlockIo,
    K2OS_BLOCKIO_RANGE  aBlockIoRange,
    UINT32              aBytesOffset,
    UINT32              aByteCount
)
{
    if ((NULL == aTokBlockIo) ||
        (NULL == aBlockIoRange) ||
        (0 == aByteCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }
    return (BOOL)CrtKern_SysCall4(K2OS_SYSCALL_ID_BLOCKIO_ERASE,
        (UINT32)aTokBlockIo,
        (UINT32)aBlockIoRange,
        (UINT32)aBytesOffset,
        (UINT32)aByteCount
    );
}
