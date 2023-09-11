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
#include "K2OsLoader.h"

void Loader_Arch_Transition(void);

void Loader_TransitionToKernel(void)
{
    K2STAT      status;
    EFI_STATUS  efiStatus;

    //
    // DO NOT DO *EFI* DEBUG PRINTS HERE UNLESS WE ARE FAILING
    // AS DOING SO WILL ALLOCATE MEMORY WHICH WILL ALTER
    // THE MEMORY MAP
    //
    do
    {
        efiStatus = gBS->ExitBootServices(gData.mLoaderImageHandle, gData.mMemMapKey);
        if (efiStatus == EFI_SUCCESS)
            break;

        //
        // DO NOT DO DEBUG PRINTS AFTER UPDATING THE MEMORY MAP BEFORE
        // WE DO ExitBootServices AGAIN
        //
        efiStatus = Loader_UpdateMemoryMap();
        if (efiStatus != EFI_SUCCESS)
        {
            K2Printf(L"UpdateMemoryMap failed with %r\n", efiStatus);
            return;
        }

        status = Loader_AssignRuntimeVirtual();
        if (K2STAT_IS_ERROR(status))
            return;

    } while (1);

    efiStatus = gRT->SetVirtualAddressMap(
        gData.LoadInfo.mEfiMapSize,
        gData.LoadInfo.mEfiMemDescSize,
        gData.LoadInfo.mEfiMemDescVer,
        (EFI_MEMORY_DESCRIPTOR *)gData.mpMemoryMap
        );

    if (efiStatus != EFI_SUCCESS)
        return;

    //
    // lock down the virtual to physical mappings to their arch-specific
    // meanings all at once now
    //
    K2VMAP32_RealizeArchMappings(&gData.Map);

    //
    // EFI system table is good to go. Pointers already cleared out
    // and CRC already recomputed
    //

    //
    // SetVirtualAddressMap and ConvertPointer are physical still
    // so clear them out and recompute the CRC
    //
    gRT->SetVirtualAddressMap = 0;
    gRT->ConvertPointer = 0;
    gRT->Hdr.CRC32 = 0;
    gRT->Hdr.CRC32 = K2CRC_Calc32(0, gRT, gRT->Hdr.HeaderSize);

    K2MEM_Copy((void *)(gData.LoadInfo.mTransitionPageAddr + (K2_VA_MEMPAGE_BYTES / 2)), &gData.LoadInfo, sizeof(gData.LoadInfo));

    Loader_Arch_Transition();

    while (1);
}

