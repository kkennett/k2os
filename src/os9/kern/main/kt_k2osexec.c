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
KernExec_Threaded_Init(
    void
)
{
    XDL_pf_ENTRYPOINT       execEntryPoint;
    K2STAT                  stat;
    XDL_FILE_HEADER const * pXdlHeader;

    //
    // loader should have checked all this in UEFI when PLAT XDL was loaded
    // but we check stuff again here
    //
    stat = XDL_FindExport(
        gData.Xdl.mpExec,
        XDLProgData_Text,
        "MainThread",
        (UINT32 *)&gData.Exec.mfMainThreadEntryPoint);
    while (K2STAT_IS_ERROR(stat));

    stat = XDL_GetHeaderPtr(gData.Xdl.mpExec, &pXdlHeader);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    execEntryPoint = (XDL_pf_ENTRYPOINT)(UINT32)pXdlHeader->mEntryPoint;

    stat = execEntryPoint(gData.Xdl.mpExec, XDL_ENTRY_REASON_LOAD);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
}
