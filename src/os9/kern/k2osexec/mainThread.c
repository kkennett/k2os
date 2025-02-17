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

#include "k2osexec.h"

UINT32          gMainThreadId;
EXEC_PLAT       gPlat;
K2OSKERN_DDK    gKernDdk;

void
Plat_Init(
    void
)
{
    K2OS_XDL xdl;

    K2MEM_Zero(&gPlat, sizeof(gPlat));

    xdl = K2OS_Xdl_Acquire("k2osplat");
    if (NULL == xdl)
    {
        K2OSKERN_Panic("***k2osexec could not find k2osplat!\n");
        return;
    }

    gPlat.DeviceCreate = (K2OSPLAT_pf_DeviceCreate)K2OS_Xdl_FindExport(xdl, TRUE, "K2OSPLAT_DeviceCreate");
    if (NULL == gPlat.DeviceCreate)
    {
        K2OSKERN_Panic("***k2osexec could not find K2OSPLAT_DeviceCreate!\n");
        return;
    }
    gPlat.DeviceAddRes = (K2OSPLAT_pf_DeviceAddRes)K2OS_Xdl_FindExport(xdl, TRUE, "K2OSPLAT_DeviceAddRes");
    if (NULL == gPlat.DeviceAddRes)
    {
        K2OSKERN_Panic("***k2osexec could not find K2OSPLAT_DeviceAddRes!\n");
        return;
    }
    gPlat.DeviceRemove = (K2OSPLAT_pf_DeviceRemove)K2OS_Xdl_FindExport(xdl, TRUE, "K2OSPLAT_DeviceRemove");
    if (NULL == gPlat.DeviceRemove)
    {
        K2OSKERN_Panic("***k2osexec could not find K2OSPLAT_DeviceRemove!\n");
        return;
    }
}

void
SysProc_Start(
    K2OSEXEC_INIT *apInit
)
{
    UINT32              id;
    K2OS_PROCESS_TOKEN  tokProc;

    id = 0;
    tokProc = K2OS_System_CreateProcess(":sysproc", NULL, &id);
    if (NULL == tokProc)
    {
        K2OSKERN_Panic("*** Sysproc process create failed\n");
    }
    K2_ASSERT(K2OS_SYSPROC_ID == id);
    K2OS_Token_Destroy(tokProc);

    apInit->WaitSysProcReady();
}

UINT32
MainThread(
    K2OSEXEC_INIT *apInit
)
{
    gMainThreadId = K2OS_Thread_GetId();

    K2MEM_Copy(&gKernDdk, &apInit->DdkInit, sizeof(K2OSKERN_DDK));

    //
    // bring up fsmgr so we can bring up the built-in filesystem
    //
    FsMgr_Init(apInit->mpRootFsNode, apInit->mpFsRootFsNode, apInit->mfFsNodeInit);

    // this installs the rofs bulit-in file system
    Rofs_Init(apInit->mpRofs);

    // xdl loads and file accesses to the built-in filesystem should work now

    SysProc_Start(apInit);

    Plat_Init();

    Dev_Init();

    ACPI_Init(&apInit->AcpiInit);

    DevMgr_Init();

    VolMgr_Init();

    ACPI_Enable();

    ACPI_StartSystemBusDriver();

    DevMgr_Run();

    K2OS_RaiseException(K2STAT_EX_LOGIC);

    return 0;
}

 