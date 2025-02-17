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

static UINT32 volatile      sgInit = 0;
static K2OS_CRITSEC         sgFsMgrSec;
static K2_GUID128 const     sgRpcServerClassId = K2OS_IFACE_RPC_SERVER;

K2OS_RPC_OBJ_HANDLE  gK2OSFS_FsMgrRpcObjHandle = NULL;
K2OS_IFINST_ID       gK2OSFS_FsMgrRpcServerIfInstId;

extern K2OS_CRITSEC     gK2OSFS_ClientTreeSec;
extern K2TREE_ANCHOR    gK2OSFS_ClientTree;

BOOL 
FsMgr_Connected(
    void
)
{
    UINT32              chk;
    K2OS_IFENUM_TOKEN   tokEnum;
    K2OS_IFINST_DETAIL  ifInstDetail;
    BOOL                ok;

    do {
        if (2 == sgInit)
            break;

        chk = K2ATOMIC_CompareExchange(&sgInit, 1, 0);
        if (chk == 0)
        {
            ok = K2OS_CritSec_Init(&sgFsMgrSec);
            if (ok)
            {
                tokEnum = K2OS_IfEnum_Create(FALSE, 0, K2OS_IFACE_CLASSCODE_FSMGR, NULL);
                if (NULL == tokEnum)
                {
                    ok = FALSE;
                }
                else
                {
                    chk = 1;
                    ok = K2OS_IfEnum_Next(tokEnum, &ifInstDetail, &chk);
                    K2OS_Token_Destroy(tokEnum);
                    if (ok)
                    {
                        gK2OSFS_FsMgrRpcObjHandle = K2OS_Rpc_AttachByIfInstId(ifInstDetail.mInstId, NULL);
                        if (NULL == gK2OSFS_FsMgrRpcObjHandle)
                        {
                            ok = FALSE;
                        }
                        else
                        {
                            tokEnum = K2OS_IfEnum_Create(TRUE, ifInstDetail.mOwningProcessId, K2OS_IFACE_CLASSCODE_RPC, &sgRpcServerClassId);
                            if (NULL == tokEnum)
                            {
                                ok = FALSE;
                            }
                            else
                            {
                                chk = 1;
                                ok = K2OS_IfEnum_Next(tokEnum, &ifInstDetail, &chk);
                                K2OS_Token_Destroy(tokEnum);
                                if (ok)
                                {
                                    gK2OSFS_FsMgrRpcServerIfInstId = ifInstDetail.mInstId;

                                    gK2OSFS_DefaultClient.mhRpcObj = K2OS_Rpc_CreateObj(gK2OSFS_FsMgrRpcServerIfInstId, &gFsClientRpcClassId, 0);
                                    if (NULL == gK2OSFS_DefaultClient.mhRpcObj)
                                    {
                                        ok = FALSE;
                                    }
                                    else
                                    {
                                        ok = K2OS_Rpc_GetObjId(gK2OSFS_DefaultClient.mhRpcObj, &gK2OSFS_DefaultClient.mObjId);
                                        if (ok)
                                        {
                                            ok = K2OS_CritSec_Init(&gK2OSFS_ClientTreeSec);
                                            if (ok)
                                            {
                                                K2TREE_Init(&gK2OSFS_ClientTree, NULL);
                                            }
                                        }
                                    }
                                    if (!ok)
                                    {
                                        gK2OSFS_FsMgrRpcServerIfInstId = 0;
                                    }
                                }
                            }
                            if (!ok)
                            {
                                K2OS_Rpc_Release(gK2OSFS_FsMgrRpcObjHandle);
                                gK2OSFS_FsMgrRpcObjHandle = NULL;
                            }
                        }
                    }
                }
                if (!ok)
                {
                    K2OS_CritSec_Done(&sgFsMgrSec);
                }
            }
            if (!ok)
            {
                K2ATOMIC_Exchange(&sgInit, 0);
                K2OS_Thread_SetLastStatus(K2STAT_ERROR_UNKNOWN);
                return FALSE;
            }

            K2ATOMIC_Exchange(&sgInit, 2);
            break;
        }

        do {
            if (1 != sgInit)
                break;
            K2_CpuReadBarrier();
            K2OS_Thread_Sleep(10);
        } while (1);

    } while (1);

    return TRUE;
}

BOOL 
K2OS_FsMgr_FormatVolume(
    K2OS_IFINST_ID      aVolIfInstId,
    K2_GUID128 const *  apFsGuid,
    K2_GUID128 const *  apUniqueId,
    UINT32              aParam1,
    UINT32              aParam2
)
{
    K2OS_FSMGR_FORMAT_VOLUME_IN formatIn;
    K2OS_RPC_CALLARGS           callArgs;
    UINT32                      actualOut;
    K2STAT                      stat;

    if ((0 == aVolIfInstId) ||
        (NULL == apFsGuid) ||
        (NULL == apUniqueId))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (!FsMgr_Connected())
        return FALSE;

    formatIn.mVolIfInstId = aVolIfInstId;
    K2MEM_Copy(&formatIn.mFsGuid, apFsGuid, sizeof(K2_GUID128));
    K2MEM_Copy(&formatIn.mUniqueId, apUniqueId, sizeof(K2_GUID128));
    formatIn.mParam1 = aParam1;
    formatIn.mParam2 = aParam2;

    callArgs.mMethodId = K2OS_FsMgr_Method_FormatVolume;
    callArgs.mpInBuf = (UINT8 const *)&formatIn;
    callArgs.mInBufByteCount = sizeof(K2OS_FSMGR_FORMAT_VOLUME_IN);
    callArgs.mpOutBuf = NULL;
    callArgs.mOutBufByteCount = 0;

    actualOut = 0;
    stat = K2OS_Rpc_Call(gK2OSFS_FsMgrRpcObjHandle, &callArgs, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL 
K2OS_FsMgr_CleanVolume(
    K2OS_IFINST_ID aVolIfInstId
)
{
    K2OS_FSMGR_CLEAN_VOLUME_IN  cleanIn;
    K2OS_RPC_CALLARGS           callArgs;
    UINT32                      actualOut;
    K2STAT                      stat;

    if (0 == aVolIfInstId)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (!FsMgr_Connected())
        return FALSE;

    cleanIn.mVolIfInstId = aVolIfInstId;

    callArgs.mMethodId = K2OS_FsMgr_Method_CleanVolume;
    callArgs.mpInBuf = (UINT8 const *)&cleanIn;
    callArgs.mInBufByteCount = sizeof(K2OS_FSMGR_CLEAN_VOLUME_IN);
    callArgs.mpOutBuf = NULL;
    callArgs.mOutBufByteCount = 0;

    actualOut = 0;
    stat = K2OS_Rpc_Call(gK2OSFS_FsMgrRpcObjHandle, &callArgs, &actualOut);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL 
K2OS_FsMgr_Mount(
    K2OS_IFINST_ID      aFileSysIfInstId,
    UINT32              aAccess,
    UINT32              aShare,
    char const *        apPath
)
{
    char const *            pScan;
    char const *            pMax;
    char                    ch;
    UINT32                  pathLen;
    K2OS_FSMGR_MOUNT_IN *   pMountIn;
    K2STAT                  stat;
    K2OS_RPC_CALLARGS       callArgs;

    if ((0 == aFileSysIfInstId) ||
        (NULL == apPath))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    ch = *apPath;
    if ((('/' != ch) && ('\\' != ch)) ||
        (0 == apPath[1]))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pScan = apPath + 2;
    pMax = pScan + K2OS_FSITEM_MAX_COMPONENT_NAME_LENGTH;
    do {
        if (0 == *pScan)
            break;
        if (++pScan == pMax)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }
    } while (1);

    pathLen = (UINT32)(pScan - apPath);

    pMountIn = (K2OS_FSMGR_MOUNT_IN *)K2OS_Heap_Alloc(sizeof(K2OS_FSMGR_MOUNT_IN) - 4 + pathLen + 1);
    if (NULL == pMountIn)
        return FALSE;

    pMountIn->mFileSysIfInstId = aFileSysIfInstId;
    pMountIn->mAccess = aAccess;
    pMountIn->mShare = aShare;
    pMountIn->mPathLen = pathLen;
    K2MEM_Copy(pMountIn->mPath, apPath, pathLen);
    pMountIn->mPath[pathLen] = 0;
    
    if (FsMgr_Connected())
    {
        callArgs.mMethodId = K2OS_FsMgr_Method_Mount;
        callArgs.mpInBuf = (UINT8 const *)pMountIn;
        callArgs.mInBufByteCount = sizeof(K2OS_FSMGR_MOUNT_IN) - 4 + pathLen + 1;
        callArgs.mpOutBuf = NULL;
        callArgs.mOutBufByteCount = 0;

        pathLen = 0;
        stat = K2OS_Rpc_Call(gK2OSFS_FsMgrRpcObjHandle, &callArgs, &pathLen);
    }
    else
    {
        stat = K2STAT_ERROR_UNKNOWN;
    }

    K2OS_Heap_Free(pMountIn);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL 
K2OS_FsMgr_Dismount(
    char const *apPath
)
{
    char const *                pScan;
    char const *                pMax;
    char                        ch;
    UINT32                      pathLen;
    K2OS_FSMGR_DISMOUNT_IN *    pDismountIn;
    K2STAT                      stat;
    K2OS_RPC_CALLARGS           callArgs;

    if (NULL == apPath)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    ch = *apPath;
    if ((('/' != ch) && ('\\' != ch)) ||
        (0 == apPath[1]))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pScan = apPath + 2;
    pMax = pScan + K2OS_FSITEM_MAX_COMPONENT_NAME_LENGTH;
    do {
        if (0 == *pScan)
            break;
        if (++pScan == pMax)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }
    } while (1);

    pathLen = (UINT32)(pScan - apPath);

    pDismountIn = (K2OS_FSMGR_DISMOUNT_IN *)K2OS_Heap_Alloc(sizeof(K2OS_FSMGR_DISMOUNT_IN) - 4 + pathLen + 1);
    if (NULL == pDismountIn)
        return FALSE;

    K2MEM_Copy(pDismountIn->mPath, apPath, pathLen);
    pDismountIn->mPath[pathLen] = 0;

    if (FsMgr_Connected())
    {
        callArgs.mMethodId = K2OS_FsMgr_Method_Dismount;
        callArgs.mpInBuf = (UINT8 const *)pDismountIn;
        callArgs.mInBufByteCount = sizeof(K2OS_FSMGR_DISMOUNT_IN) - 4 + pathLen + 1;
        callArgs.mpOutBuf = NULL;
        callArgs.mOutBufByteCount = 0;

        pathLen = 0;
        stat = K2OS_Rpc_Call(gK2OSFS_FsMgrRpcObjHandle, &callArgs, &pathLen);
    }
    else
    {
        stat = K2STAT_ERROR_UNKNOWN;
    }

    K2OS_Heap_Free(pDismountIn);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL
K2OS_FsMgr_SetNotifyMailbox(
    K2OS_MAILBOX_TOKEN aTokMailbox
)
{
    if (!FsMgr_Connected())
        return FALSE;
    return K2OS_Rpc_SetNotifyTarget(gK2OSFS_FsMgrRpcObjHandle, aTokMailbox);
}
