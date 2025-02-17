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

#include "k2osfs.h"

BOOL            
K2OS_File_GetInfo(
    K2OS_FILE           aFile,
    K2OS_FSITEM_INFO *  apRetInfo
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

BOOL            
K2OS_File_GetPos(
    K2OS_FILE   aFile,
    INT64 *     apRetAbsPointer
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

BOOL            
K2OS_File_SetPos(
    K2OS_FILE           aFile,
    K2OS_FilePosType    aPosType,
    INT64 const *       apPointerOffsetBytes
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

BOOL            
K2OS_File_GetSize(
    K2OS_FILE   aFile,
    UINT64 *    apRetFileSizeBytes
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

BOOL
K2OS_File_Read(
    K2OS_FILE   aFile,
    void *      apBuffer,
    UINT_PTR    aBytesToRead,
    UINT_PTR *  apRetBytesRead
)
{
    K2OS_FSFILE_READ_IN     paramIn;
    K2OS_FSFILE_READ_OUT    result;
    K2OS_RPC_CALLARGS       Args;
    UINT32                  actualOut;
    K2STAT                  stat;

    paramIn.TargetBufDesc.mAddress = (UINT32)apBuffer;
    paramIn.TargetBufDesc.mAttrib = 0;
    paramIn.TargetBufDesc.mBytesLength = aBytesToRead;
    paramIn.mBytesToRead = aBytesToRead;

    Args.mpInBuf = (UINT8 const *)&paramIn;
    Args.mInBufByteCount = sizeof(paramIn);
    Args.mpOutBuf = (UINT8 *)&result;
    Args.mOutBufByteCount = sizeof(result);
    Args.mMethodId = K2OS_FsFile_Method_Read;

    actualOut = 0;
    stat = K2OS_Rpc_Call((K2OS_RPC_OBJ_HANDLE)aFile, &Args, &actualOut);
    if (!K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(actualOut == sizeof(result));
        if (NULL != apRetBytesRead)
        {
            *apRetBytesRead = result.mBytesRed;
        }
        return TRUE;
    }

    K2OS_Thread_SetLastStatus(stat);

    return FALSE;
}

BOOL            
K2OS_File_Write(
    K2OS_FILE   aFile,
    void const *apBuffer,
    UINT_PTR    aBytesToWrite,
    UINT_PTR *  apRetBytesWritten
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

BOOL            
K2OS_File_SetEnd(
    K2OS_FILE aFile
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return FALSE;
}

UINT32          
K2OS_File_GetAccess(
    K2OS_FILE aFile
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return K2OS_ACCESS_INVALID;
}

UINT32          
K2OS_File_GetShare(
    K2OS_FILE aFile
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return K2OS_ACCESS_INVALID;
}

UINT32          
K2OS_File_GetAttrib(
    K2OS_FILE aFile
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return K2_FSATTRIB_INVALID;
}

UINT32          
K2OS_File_GetOpenFlags(
    K2OS_FILE aFile
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return K2OS_FSFLAGS_INVALID;
}

UINT32          
K2OS_File_GetBlockAlign(
    K2OS_FILE aFile
)
{
    K2OSRPC_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IMPL);
    return 0;
}

BOOL
K2OS_File_Close(
    K2OS_FILE aFile
)
{
    return K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)aFile);
}

