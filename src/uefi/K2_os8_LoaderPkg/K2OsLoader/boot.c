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
#include "K2OsLoader.h"

void
sSetupGraphics(void)
{
    EFI_STATUS                          status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *      pGop;
    //    EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION pixel;

    status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&pGop);
    if (!EFI_ERROR(status))
    {
        if ((NULL != pGop->Mode) && (NULL != pGop->Mode->Info))
        {
            gData.LoadInfo.BootGraf.mFrameBufferPhys = (UINT32)pGop->Mode->FrameBufferBase;
            gData.LoadInfo.BootGraf.mFrameBufferBytes = (UINT32)pGop->Mode->FrameBufferSize;
            K2MEM_Copy(&gData.LoadInfo.BootGraf.ModeInfo, pGop->Mode->Info, sizeof(K2EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
#if 0
            //
            // clear to blue
            //
            pixel.Raw = 0;
            pixel.Pixel.Blue = 255;
            pGop->Blt(
                pGop,
                &pixel.Pixel,
                EfiBltVideoFill,
                0,
                0,
                0,
                0,
                pGop->Mode->Info->HorizontalResolution,
                pGop->Mode->Info->VerticalResolution,
                0);
#endif
        }
    }
}

void
Loader_Boot(
    XDL * apXdlKern
)
{
    K2STAT                  stat;
    XDL_FILE_HEADER const * pHeader;

    stat = Loader_CreateVirtualMap();
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Loader_CreateVirtualMap() failed (%08X)\n", stat);
        return;
    }

    stat = Loader_MapAllXDL();
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Loader_MapAllXDL() failed (%08X)\n", stat);
        return;
    }

    stat = Loader_AssembleAcpi();
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Loader_AssembleAcpi() failed (%08X)\n", stat);
        return;
    }

    sSetupGraphics();
 
#if 1
    gData.LoadInfo.mDebugPageVirt = gData.LoadInfo.mKernArenaLow;
    gData.LoadInfo.mKernArenaLow += K2_VA_MEMPAGE_BYTES;
#if K2_TARGET_ARCH_IS_ARM
    stat = K2VMAP32_MapPage(&gData.Map, gData.LoadInfo.mDebugPageVirt, 0x02020000, K2OS_MAPTYPE_KERN_DEVICEIO);   // IMX6 UART 1
//                                        stat = K2VMAP32_MapPage(&gData.Map, gData.LoadInfo.mDebugPageVirt, 0x021E8000, K2OS_MAPTYPE_KERN_DEVICEIO);     // IMX6 UART 2
#else
    stat = K2VMAP32_MapPage(&gData.Map, gData.LoadInfo.mDebugPageVirt, 0x000B8000, K2OS_MAPTYPE_KERN_DEVICEIO);
#endif
    //                        K2Printf(L"DebugPageVirt = 0x%08X\n", gData.LoadInfo.mDebugPageVirt);
    if (K2STAT_IS_ERROR(stat))
    {
        K2Printf(L"*** Boot Failed (%08X)\n", stat);
        return;
    }
#else
    gData.LoadInfo.mDebugPageVirt = 0;
#endif

    K2Printf(L"\n----------------\nTransition...\n");

    stat = Loader_TrackEfiMap();
    if (K2STAT_IS_ERROR(stat))
        return;

    //
    // DO NOT ALLOCATE ANY MEMORY AFTER THIS
    //
    // DO NOT DO DEBUG PRINTS AFTER THIS
    //

    //
    // must get header pointer before handoff, because handoff
    // will change the address to a virtual one that is no good
    // inside UEFI
    //
    stat = XDL_GetHeaderPtr(apXdlKern, &pHeader);
    if (K2STAT_IS_ERROR(stat))
        return;

    //
    // switch over to virtual addresses for everything
    //
    stat = K2XDL_Handoff(
        &gData.LoadInfo.mpXdlCrt,
        sysXDL_ConvertLoadPtr,
        (XDL_pf_ENTRYPOINT *)&gData.LoadInfo.mSysVirtEntry);
    if (K2STAT_IS_ERROR(stat))
        return;

    //
    // kernel xdl virtual entrypoint needs to go into loadinfo
    //
    gData.LoadInfo.mKernXdlEntry = (UINTN)pHeader->mEntryPoint;

    //
    // should never return from this
    //
    Loader_TransitionToKernel();
}