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

K2OS_IFSUBS_TOKEN
K2OS_IfSubs_Create(
    K2OS_MAILBOX_TOKEN  aTokMailboxOwner,
    UINT32              aClassCode,
    K2_GUID128 const *  apSpecific,
    UINT32              aBacklogCount,
    BOOL                aProcSelfNotify,
    void *              apContext
)
{
    UINT32 *    pSpec;
    K2_GUID128  guid;

    if (0 == aBacklogCount)
    {
        aBacklogCount = 1;
    }
    else if (aBacklogCount > 255)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_TOO_BIG);
        return NULL;
    }

    if (aTokMailboxOwner == NULL)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return 0;
    }

    if (NULL == apSpecific)
    {
        K2MEM_Zero(&guid, sizeof(guid));
        apSpecific = &guid;
    }

    pSpec = (UINT32 *)apSpecific;
    return (K2OS_IFSUBS_TOKEN)CrtKern_SysCall8(K2OS_SYSCALL_ID_IFSUBS_CREATE,
        (UINT32)aTokMailboxOwner,
        aClassCode,
        pSpec[0], pSpec[1], pSpec[2], pSpec[3],
        (aProcSelfNotify ? 0xFFFF0000 : 0) | aBacklogCount,
        (UINT32)apContext);
}
