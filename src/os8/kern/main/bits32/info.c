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

#include "kern.h"

K2STAT 
KernInfo_Get(
    K2OSKERN_OBJ_PROCESS *  apCurProc,
    K2OSKERN_OBJ_THREAD *   apCurThread,
    UINT32                  aSysInfoId,
    UINT32                  aInfoIx,
    UINT8 *                 apRetUserBuffer,
    UINT32                  aUserBufferBytes,
    UINT32 *                apRetBytes
)
{
    UINT32                      count;
    K2OS_PHYSADDR_INFO const *  pPhys;
    K2OS_IOADDR_INFO const *    pIo;

    switch (aSysInfoId)
    {
    case K2OS_SYSINFO_CPUCOUNT:
        *apRetBytes = sizeof(UINT32);
        if (aUserBufferBytes < (*apRetBytes))
        {
            return K2STAT_ERROR_TOO_SMALL;
        }
        K2MEM_Copy(apRetUserBuffer, &gData.mCpuCoreCount, sizeof(UINT32));
        return K2STAT_NO_ERROR;

    case K2OS_SYSINFO_FWINFO:
        *apRetBytes = sizeof(K2OS_FWINFO);
        if (aUserBufferBytes < (*apRetBytes))
        {
            return K2STAT_ERROR_TOO_SMALL;
        }
        ((K2OS_FWINFO *)apRetUserBuffer)->mFwBasePhys = gData.mpShared->LoadInfo.mFwTabPagesPhys;
        ((K2OS_FWINFO *)apRetUserBuffer)->mFwSizeBytes = gData.mpShared->LoadInfo.mFwTabPageCount * K2_VA_MEMPAGE_BYTES;
        ((K2OS_FWINFO *)apRetUserBuffer)->mFacsPhys = gData.mpShared->LoadInfo.mFwFacsPhys;
        ((K2OS_FWINFO *)apRetUserBuffer)->mXFacsPhys = gData.mpShared->LoadInfo.mFwXFacsPhys;
        return K2STAT_NO_ERROR;

    case K2OS_SYSINFO_PHYSADDR:
        *apRetBytes = sizeof(K2OS_PHYSADDR_INFO);
        if (aUserBufferBytes < (*apRetBytes))
        {
            return K2STAT_ERROR_TOO_SMALL;
        }
        count = 0;
        KernArch_GetPhysTable(&count, &pPhys);
        if (aInfoIx < count)
        {
            K2MEM_Copy(apRetUserBuffer, &pPhys[aInfoIx], sizeof(K2OS_PHYSADDR_INFO));
            return K2STAT_NO_ERROR;
        }
        aInfoIx -= count;
        count = 0;
        KernPlat_GetPhysTable(&count, &pPhys);
        if (aInfoIx < count)
        {
            K2MEM_Copy(apRetUserBuffer, &pPhys[aInfoIx], sizeof(K2OS_PHYSADDR_INFO));
            return K2STAT_NO_ERROR;
        }
        aInfoIx -= count;
        return KernPhys_GetEfiChunk(aInfoIx, (K2OS_PHYSADDR_INFO *)apRetUserBuffer);

    case K2OS_SYSINFO_IOADDR:
        *apRetBytes = sizeof(K2OS_IOADDR_INFO);
        if (aUserBufferBytes < (*apRetBytes))
        {
            return K2STAT_ERROR_TOO_SMALL;
        }
        count = 0;
        KernArch_GetIoTable(&count, &pIo);
        if (aInfoIx < count)
        {
            K2MEM_Copy(apRetUserBuffer, &pIo[aInfoIx], sizeof(K2OS_IOADDR_INFO));
            return K2STAT_NO_ERROR;
        }
        aInfoIx -= count;
        count = 0;
        KernPlat_GetIoTable(&count, &pIo);
        if (aInfoIx < count)
        {
            K2MEM_Copy(apRetUserBuffer, &pIo[aInfoIx], sizeof(K2OS_IOADDR_INFO));
            return K2STAT_NO_ERROR;
        }
        return K2STAT_ERROR_NO_MORE_ITEMS;

    default:
        break;
    }

    *apRetBytes = 0;
    return K2STAT_ERROR_NOT_IMPL;
}

void    
KernInfo_SysCall_Get(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT32                  lockMapCount;
    K2OSKERN_OBJREF *       pLockedMapRefs;
    UINT32                  map0FirstPageIx;
    UINT32                  userTargetBuffer;
    UINT32                  userBufferBytes;
    UINT32                  ix;
    UINT32                  retBytes;

    pProc = apCurThread->ProcRef.Ptr.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    userTargetBuffer = pThreadPage->mSysCall_Arg2;
    userBufferBytes = pThreadPage->mSysCall_Arg3;
    if (0 == userBufferBytes)
        userTargetBuffer = 0;

    pThreadPage->mSysCall_Arg7_Result0 = 0;

    if ((userTargetBuffer >= K2OS_KVA_KERN_BASE) ||
        ((K2OS_KVA_KERN_BASE - userTargetBuffer) < userBufferBytes))
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        lockMapCount = (userBufferBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;
        pLockedMapRefs = NULL;
        if (0 != lockMapCount)
        {
            pLockedMapRefs = (K2OSKERN_OBJREF *)KernHeap_Alloc(lockMapCount * sizeof(K2OSKERN_OBJREF));
            if (NULL == pLockedMapRefs)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
            }
            else
            {
                K2MEM_Zero(pLockedMapRefs, sizeof(K2OSKERN_OBJREF) * lockMapCount);
                stat = KernProc_AcqMaps(
                    pProc,
                    userTargetBuffer,
                    userBufferBytes,
                    TRUE,
                    &lockMapCount,
                    pLockedMapRefs,
                    &map0FirstPageIx
                );
            }
        }

        if (!K2STAT_IS_ERROR(stat))
        {
            //
            // if used, target buffer maps acquried.  buffer cannot disappear
            // while we are writing into it
            //
            retBytes = 0;
            stat = KernInfo_Get(pProc, apCurThread, apCurThread->mSysCall_Arg0, pThreadPage->mSysCall_Arg1, (UINT8 *)userTargetBuffer, userBufferBytes, &retBytes);
            pThreadPage->mSysCall_Arg7_Result0 = retBytes;
        }

        if (NULL != pLockedMapRefs)
        {
            for (ix = 0; ix < lockMapCount; ix++)
            {
                KernObj_ReleaseRef(&pLockedMapRefs[ix]);
            }
            KernHeap_Free(pLockedMapRefs);
        }
    }

    apCurThread->mSysCall_Result = (UINT32)stat;

    if (K2STAT_IS_ERROR((K2STAT)(apCurThread->mSysCall_Result)))
    {
        pThreadPage->mLastStatus = apCurThread->mSysCall_Result;
    }
}

