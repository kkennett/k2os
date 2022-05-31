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
#include "idlx.h"

K2STAT
iK2DLXSUPP_DoCallback(
    void *  apAcqContext,
    DLX *   apDlx,
    BOOL    aIsLoad
    )
{
    BOOL                callEntrypoint;
    K2STAT              status;
    K2_EXCEPTION_TRAP   trap;
    DLX_pf_ENTRYPOINT   entryPoint;

    if (apDlx->mpElf->e_machine != K2ELF32_TARGET_MACHINE_TYPE)
        return 0;
    
    if (gpK2DLXSUPP_Vars->Host.PreCallback != NULL)
        callEntrypoint = gpK2DLXSUPP_Vars->Host.PreCallback(apAcqContext, apDlx->mHostFile, aIsLoad, apDlx);
    else
        callEntrypoint = FALSE;

    if ((apDlx->mEntrypoint != 0) && (callEntrypoint))
    {
        entryPoint = (DLX_pf_ENTRYPOINT)apDlx->mEntrypoint;
        gpK2DLXSUPP_Vars->mAcqDisabled = TRUE;
        status = K2_EXTRAP(&trap, entryPoint(apDlx, aIsLoad ? DLX_ENTRY_REASON_LOAD : DLX_ENTRY_REASON_UNLOAD));
        gpK2DLXSUPP_Vars->mAcqDisabled = FALSE;
        if (gpK2DLXSUPP_Vars->Host.PostCallback != NULL)
            status = gpK2DLXSUPP_Vars->Host.PostCallback(apAcqContext, apDlx->mHostFile, status, apDlx);
        return status;
    }

    return K2STAT_OK;
}

