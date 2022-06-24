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
#define XDL_ELF_SHF_TYPE_MASK       SHF_MASKOS
#define XDL_ELF_SHF_TYPE_EXPORTS    0x00100000
#define XDL_ELF_SHF_TYPE_IMPORTS    0x00200000
#define XDL_ELF_SHF_TYPE_ANCHOR     0x00400000

typedef enum _XDLExportType XDLExportType;
enum _XDLExportType
{
    XDLExport_Text,
    XDLExport_Read,
    XDLExport_Data,

    XDLExportType_Count
};

K2_PACKED_PUSH
typedef struct _XDL_ELF_ANCHOR XDL_ELF_ANCHOR;
struct _XDL_ELF_ANCHOR
{
    UINT64  mAnchor[XDLExportType_Count]; 
} K2_PACKED_ATTRIB;
K2_PACKED_POP

#define XDL_SECTOR_BITS             9
#define XDL_SECTOR_BYTES            (1 << XDL_SECTOR_BITS)
#define XDL_SECTOROFFSET_MASK       (XDL_SECTOR_BYTES - 1)
#define XDL_SECTORINDEX_MASK        (~XDL_SECTOROFFSET_MASK)
#define XDL_SECTORS_PER_PAGE        (K2_VA_MEMPAGE_BYTES / XDL_SECTOR_BYTES)

typedef enum _XDLSectionIx XDLSectionIx;
enum _XDLSectionIx
{
    XDLSectionIx_Text,          // code sectors
    XDLSectionIx_Read,          // read only data sectors
    XDLSectionIx_Exports,       // export record sectors
    XDLSectionIx_InitData,      // initialized data sectors
    XDLSectionIx_OtherLoad,     // undefined must-load sectors
    XDLSectionIx_UninitData,    // uninitialized data (offset and count always zero)
    XDLSectionIx_DebugInfo,     // module-specific debug info sectors
    XDLSectionIx_Relocs,        // discardable - all info to permit relocation sectors
    XDLSectionIx_Imports,       // discardable - import records
    XDLSectionIx_Security,      // discardable - security validation information
    XDLSectionIx_Other,         // discardable - other data for file (only)

    XDLSectionIx_Count
};

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

K2_PACKED_PUSH
typedef struct _XDL_FILE_HEADER XDL_FILE_HEADER;
struct _XDL_FILE_HEADER
{
    UINT32              mHeaderCrc32;
    UINT32              mMarker;            // 'K2OS'
    UINT64              mPlacement;
    UINT32              mHeaderSizeBytes;
    UINT8               mElfClass;
    UINT8               mElfMachine;
    UINT16              mFlags;
    UINT64              mEntryPoint;
    UINT32              mEntryStackReq;
    UINT32              mImportCount;
    UINT64              mFirstSectionOffset;    // offset to XDLSectionIx_Text. all sections forced linear after that
    XDL_FILE_SECTION    Section[XDLSectionIx_Count];
    char                mFileName[4];           // undecorated base name, null terminated, at least 4 bytes
} K2_PACKED_ATTRIB;
K2_PACKED_POP

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

#ifdef __cplusplus
};  // extern "C"
#endif

//
// --------------------------------------------------------------------------------- 
//

#endif  // __SPEC_XDL_H

