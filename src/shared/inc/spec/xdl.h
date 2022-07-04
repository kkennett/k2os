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
#ifndef __SPEC_XDL_H
#define __SPEC_XDL_H

//
// --------------------------------------------------------------------------------- 
//

#include <spec/elf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XDL XDL;

//
// Elf Extension definitions
//
#define XDL_ELF_SHF_TYPE_MASK       0x00300000
#define XDL_ELF_SHF_TYPE_EXPORTS    0x00100000
#define XDL_ELF_SHF_TYPE_IMPORTS    0x00200000

enum _XDLExportType
{
    XDLExport_Text,
    XDLExport_Read,
    XDLExport_Data,

    XDLExportType_Count
};
typedef enum _XDLExportType XDLExportType;

K2_PACKED_PUSH
typedef struct _XDL_ELF_ANCHOR XDL_ELF_ANCHOR;
struct _XDL_ELF_ANCHOR
{
    UINT64      mAnchor[XDLExportType_Count]; 
    K2_GUID128  Id;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

#define XDL_SECTOR_BITS             9
#define XDL_SECTOR_BYTES            (1 << XDL_SECTOR_BITS)
#define XDL_SECTOROFFSET_MASK       (XDL_SECTOR_BYTES - 1)
#define XDL_SECTORINDEX_MASK        (~XDL_SECTOROFFSET_MASK)
#define XDL_SECTORS_PER_PAGE        (K2_VA_MEMPAGE_BYTES / XDL_SECTOR_BYTES)

enum _XDLSegmentIx
{
    XDLSegmentIx_Text,          // code sectors
    XDLSegmentIx_Read,          // read only data sectors
    XDLSegmentIx_Data,          // initialized data sectors
    XDLSegmentIx_DebugInfo,     // discardable - module-specific debug info sectors
    XDLSegmentIx_Relocs,        // discardable - all info to permit relocation sectors
    XDLSegmentIx_Imports,       // discardable - import records
    XDLSegmentIx_Other,         // discardable - other data for file (only)

    XDLSegmentIx_Count
};
typedef enum _XDLSegmentIx XDLSegmentIx;

K2_PACKED_PUSH
typedef struct _XDL_FILE_SECTION XDL_FILE_SECTION;
struct _XDL_FILE_SECTION
{
    UINT64  mSectorCount;
    UINT64  mMemActualBytes;
    UINT64  mLinkAddr;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

#define XDL_FILE_HEADER_MARKER              K2_MAKEID4('K','2','O','S')

#define XDL_FILE_HEADER_FLAG_KERNEL_ONLY    0x0001

#define XDL_NAME_MAX_LEN                    63

K2_PACKED_PUSH
typedef struct _XDL_FILE_HEADER XDL_FILE_HEADER;
struct _XDL_FILE_HEADER
{
    UINT32              mHeaderCrc32;
    UINT32              mMarker;            // XDL_FILE_HEADER_MARKER
    UINT64              mPlacement;         // if nonzero, mPlacement must == &mPlacement, and mFirstSectionSectorOffset must be 8 (4kB, 1 page)
    UINT32              mHeaderSizeBytes;
    UINT8               mElfClass;
    UINT8               mElfMachine;
    UINT16              mFlags;
    UINT64              mEntryPoint;
    UINT32              mEntryStackReq;
    UINT32              mImportCount;
    UINT64              mFirstSectionSectorOffset;    // offset to XDLSegmentIx_Text. all sections forced linear after that
    XDL_FILE_SECTION    Section[XDLSegmentIx_Count];
    K2_GUID128          Id;
    UINT64              mReadExpOffset[XDLExportType_Count];
    UINT32              mNameLen;
    char                mName[XDL_NAME_MAX_LEN + 1];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(XDL_SECTOR_BYTES >= sizeof(XDL_FILE_HEADER));

K2_PACKED_PUSH
typedef struct _XDL_IMPORT XDL_IMPORT;
struct _XDL_IMPORT
{
    UINT32          mSizeBytes;         // size of this import record
    UINT32          mReserved;          // reserved do not use
    K2_GUID128      ID;                 // ID of XDL we import from
    char            mFileName[4];       // name of XDL we import from
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _XDL_EXPORT32_REF XDL_EXPORT32_REF;
struct _XDL_EXPORT32_REF
{
    Elf32_Addr  mAddr;          // export address 
    Elf32_Word  mNameOffset;    // offset from start of exports section to name of export 
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _XDL_EXPORT64_REF XDL_EXPORT64_REF;
struct _XDL_EXPORT64_REF
{
    Elf64_Addr  mAddr;          // export address 
    Elf64_Word  mNameOffset;    // offset from start of exports section to name of export 
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _XDL_EXPORTS_SECTION_HEADER XDL_EXPORTS_SECTION_HEADER;
struct _XDL_EXPORTS_SECTION_HEADER
{
    UINT32  mCount;
    UINT32  mCRC32;
    // followed by at least one XDL_EXPORT??_REF record
} K2_PACKED_ATTRIB;
K2_PACKED_POP

//
// --------------------------------------------------------------------------------- 
//

#ifndef XDL_ENTRY_REASON_UNLOAD
#define XDL_ENTRY_REASON_UNLOAD ((UINT_PTR)-1)
#endif

#ifndef XDL_ENTRY_REASON_LOAD
#define XDL_ENTRY_REASON_LOAD   1
#endif

typedef K2STAT (K2_CALLCONV_REGS *XDL_pf_ENTRYPOINT)(XDL * apXdl, UINT_PTR aReason);
K2STAT K2_CALLCONV_REGS xdl_entry(XDL *apXdl, UINT_PTR aReason);

//
// --------------------------------------------------------------------------------- 
//

K2STAT  XDL_Acquire(char const *apFilePath, UINT_PTR aContext, XDL **appRetXdl);
K2STAT  XDL_Release(XDL *apXdl);
K2STAT  XDL_GetHeaderPtr(XDL *apXdl, XDL_FILE_HEADER const **appRetHeaderPtr);
K2STAT  XDL_FindExport(XDL *apXdl, XDLExportType aType, char const *apName, UINT_PTR *apRetAddr);
K2STAT  XDL_AcquireContaining(UINT_PTR aAddr, XDL **appRetXdl, UINT_PTR *apRetSection);
K2STAT  XDL_FindAddrName(UINT_PTR aAddr, char *apRetNameBuffer, UINT_PTR aBufferLen);

//
// --------------------------------------------------------------------------------- 
//

#ifdef __cplusplus
};  // extern "C"
#endif

//
// --------------------------------------------------------------------------------- 
//

#endif  // __SPEC_XDL_H

