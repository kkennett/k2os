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
#ifndef __K2OSLOADER_H__
#define __K2OSLOADER_H__

#include <IndustryStandard/Smbios.h>
#include <IndustryStandard/Acpi50.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>

#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/GraphicsOutput.h>

#include <lib/k2xdl.h>

#include <k2osbase.h>
#include <kern/lib/k2vmap32.h>
#include <lib/k2elf.h>
#include <lib/k2sort.h>

#if K2_TARGET_ARCH_IS_ARM
#include <Library/ArmLib.h>
#else

#endif

typedef struct _EFIXDL EFIXDL;
struct _EFIXDL
{
    EFI_PHYSICAL_ADDRESS    mSegPhys[XDLSegmentIx_Count];
    UINTN                   mSegBytes[XDLSegmentIx_Count];
    UINTN                   mSegTargetLink[XDLSegmentIx_Relocs];
};

typedef struct _EFIFILE EFIFILE;
struct _EFIFILE
{
    K2LIST_LINK             ListLink;  // must be first thing in structure
    EFI_PHYSICAL_ADDRESS    mPageAddr_Phys;
    UINTN                   mPageAddr_Virt;
    EFI_FILE_PROTOCOL *     mpProt;
    EFI_FILE_INFO *         mpFileInfo;
    UINT64                  mCurPos;
    char                    mFileName[XDL_NAME_MAX_LEN + 1];
    EFIXDL *                mpXdl;
};

typedef struct _LOADER_DATA LOADER_DATA;
struct _LOADER_DATA
{
    EFI_HANDLE										mLoaderImageHandle;
    K2LIST_ANCHOR									EfiFileList;
    EFI_PHYSICAL_ADDRESS							mLoaderPagePhys;
    K2VMAP32_CONTEXT								Map;
    EFI_MEMORY_DESCRIPTOR *							mpMemoryMap;
    UINTN											mMemMapKey;
    K2OS_UEFI_LOADINFO								LoadInfo;           // this is copied to transition page
    SMBIOS_STRUCTURE_POINTER *                      mpSmbios;
	EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  * mpAcpi;
    UINT32                                          mRofsPages;
    UINT32                                          mPreRuntimeHigh;
};

extern LOADER_DATA gData;

EFI_STATUS  sysXDL_Init(IN EFI_HANDLE ImageHandle);
void        sysXDL_Done(void);

K2STAT      sysXDL_CritSec(BOOL aEnter);
K2STAT      sysXDL_Open(K2XDL_OPENARGS const *apArgs, K2XDL_HOST_FILE *apRetHostFile, UINT_PTR *apRetModuleDataAddr, UINT_PTR *apRetModuleLinkAddr);
K2STAT      sysXDL_ReadSectors(K2XDL_LOADCTX const *apLoadCtx, void *apBuffer, UINT64 const *apSectorCount);
K2STAT      sysXDL_Prepare(K2XDL_LOADCTX const *apLoadCtx, BOOL aKeepSymbols, XDL_FILE_HEADER const *apFileHdr, K2XDL_SEGMENT_ADDRS *apRetSegAddrs);
BOOL        sysXDL_PreCallback(K2XDL_LOADCTX const *apLoadCtx, BOOL aIsLoad, XDL *apXdl);
void        sysXDL_PostCallback(K2XDL_LOADCTX const *apLoadCtx, K2STAT aUserStatus, XDL *apXdl);
K2STAT      sysXDL_Finalize(K2XDL_LOADCTX const *apLoadCtx, BOOL aKeepSymbols, XDL_FILE_HEADER const * apFileHdr, K2XDL_SEGMENT_ADDRS *apUpdateSegAddrs);
K2STAT      sysXDL_Purge(K2XDL_LOADCTX const *apLoadCtx, K2XDL_SEGMENT_ADDRS const * apSegAddrs);
BOOL        sysXDL_ConvertLoadPtr(UINT32 * apAddr);

EFI_STATUS  Loader_InitArch(void);
void        Loader_DoneArch(void);

EFI_STATUS  Loader_CountCpus(void);
K2STAT      Loader_AssembleAcpi(void);
K2STAT      Loader_CreateVirtualMap(void);
K2STAT      Loader_TrackEfiMap(void);
EFI_STATUS  Loader_UpdateMemoryMap(void);
K2STAT      Loader_AssignRuntimeVirtual(void);
void        Loader_TransitionToKernel(void);
K2STAT      Loader_MapAllXDL(void);

void        Loader_Boot(XDL *apXdlKern);

UINTN       K2Printf(CHAR16 const * apFormat, ...);

#endif /* __K2OSLOADER_H__ */
