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

void
LoadAllBuiltInXdl(
    void
)
{
    K2ROFS_DIR const *  pDir;
    K2ROFS_FILE const * pFile;
    UINT32              fileIx;
    char const *        pFileName;
    char const *        pExt;
    char                ch;

    pDir = K2ROFS_ROOTDIR(gData.BuiltIn.mpRofs);
    K2_ASSERT(NULL != pDir);

    pDir = K2ROFSHELP_SubDir(gData.BuiltIn.mpRofs, pDir, "kern");
    K2_ASSERT(NULL != pDir);

    K2_ASSERT(0 != pDir->mFileCount);

    for (fileIx = 0; fileIx < pDir->mFileCount; fileIx++)
    {
        pFile = K2ROFSHELP_SubFileIx(gData.BuiltIn.mpRofs, pDir, fileIx);

        pFileName = K2ROFS_NAMESTR(gData.BuiltIn.mpRofs, pFile->mName);

        pExt = pFileName;
        do
        {
            ch = *pExt;
            pExt++;
            if (ch == '.')
            {
                if (0 == K2ASC_CompIns(pExt, "xdl"))
                {
                    K2OS_Xdl_Acquire(pFileName);
                }
                break;
            }
        } while (0 != ch);
    }
}

K2STAT 
K2OSKERN_SysProc_Notify(
    K2OS_MSG const *apMsg
)
{
    BOOL                        doSignal;
    K2STAT                      stat;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThisThread;

    if ((apMsg->mMsgType == K2OS_SYSTEM_MSGTYPE_SYSPROC) &&
        (apMsg->mShort == K2OS_SYSTEM_MSG_SYSPROC_SHORT_RUN))
    {
        if (!gData.mSystemStarted)
        {
//            K2OSKERN_Debug("Started\n");
            //
            // system is started
            //
            gData.mSystemStarted = TRUE;

            //
            // load all builtin drivers
            //
            LoadAllBuiltInXdl();
        }
    }

    doSignal = FALSE;
    stat = KernSysProc_Notify(TRUE, apMsg, &doSignal);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("***Failed to send sysproc msg\n");
        return stat;
    }

    if (!doSignal)
    {
        return K2STAT_NO_ERROR;
    }

    disp = K2OSKERN_SetIntr(FALSE);
    K2_ASSERT(disp);

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    pThisThread = pThisCore->mpActiveThread;
    K2_ASSERT(pThisThread->mIsKernelThread);

    pSchedItem = &pThisThread->SchedItem;
    pSchedItem->mSchedItemType = KernSchedItem_KernThread_SignalNotify;
    KernObj_CreateRef(&pSchedItem->ObjRef, gData.SysProc.RefNotify.AsAny);

    KernThread_CallScheduler(pThisCore);

    // interrupts will be back on again here
    KernObj_ReleaseRef(&pSchedItem->ObjRef);

    if (0 != pThisThread->Kern.mSchedCall_Result)
    {
        stat = K2STAT_NO_ERROR;
    }
    else
    {
        stat = pThisThread->mpKernRwViewOfThreadPage->mLastStatus;
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OSKERN_Debug("*** SysProc Notify failed %08X\n", stat);
    }

    return stat;
}
