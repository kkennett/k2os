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
#ifndef __SPEC_DLX_H
#define __SPEC_DLX_H

//
// --------------------------------------------------------------------------------- 
//

#include <spec/elf.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// DLX opaque structure
//
typedef struct _DLX DLX;


//
// Elf Extension definitions
//
#define DLX_ELFOSABI_K2                 0xDA
#define DLX_ELFOSABIVER_DLX             0xAD

#define DLX_ET_DLX                      ET_OSSPEC_LO

#define DLX_SHF_TYPE_MASK               SHF_MASKOS
#define DLX_SHF_TYPE_EXPORTS            0x00100000
#define DLX_SHF_TYPE_IMPORTS            0x00200000
#define DLX_SHF_TYPE_DLXINFO            0x00400000

#define DLX_SHN_DLXINFO                 1
#define DLX_SHN_SHSTR                   2

#define DLX_SHT_DLX_EXPORTS             SHT_LOOS
#define DLX_SHT_DLX_IMPORTS             (SHT_LOOS + 1)

#define DLX_EF_KERNEL_ONLY              0x00010000

//
// File structure
//
#define DLX_SECTOR_BITS                 9
#define DLX_SECTOR_BYTES                (1 << DLX_SECTOR_BITS)
#define DLX_SECTOROFFSET_MASK           (DLX_SECTOR_BYTES - 1)
#define DLX_SECTORINDEX_MASK            (~DLX_SECTOROFFSET_MASK)
#define DLX_SECTORS_PER_PAGE            (K2_VA32_MEMPAGE_BYTES / DLX_SECTOR_BYTES)

enum _DLXSegmentIndex
{
    DlxSeg_Info,
    DlxSeg_Text,
    DlxSeg_Read,
    DlxSeg_Data,
    DlxSeg_Sym,
    DlxSeg_Reloc,
    DlxSeg_Other,
    DlxSeg_Count
};
typedef enum _DLXSegmentIndex DlxSegmentIndex;

K2_PACKED_PUSH
struct _DLX_IMPORT
{
    UINT32          mSizeBytes;         // size of this import record
    UINT32          mReserved;          // reserved do not use
    K2_GUID128      ID;                 // ID of DLX we import from
    char            mFileName[4];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _DLX_IMPORT DLX_IMPORT;

K2_PACKED_PUSH
struct _DLX_EXPORT32_REF
{
    Elf32_Addr  mAddr;          // export address 
    Elf32_Word  mNameOffset;    // offset from start of exports section to name of export 
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _DLX_EXPORT32_REF DLX_EXPORT32_REF;

K2_PACKED_PUSH
struct _DLX_EXPORT64_REF
{
    Elf64_Addr  mAddr;          // export address 
    Elf64_Word  mNameOffset;    // offset from start of exports section to name of export 
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _DLX_EXPORT64_REF DLX_EXPORT64_REF;

K2_PACKED_PUSH
struct _DLX_SEGMENT32_INFO
{
    UINT32  mLinkAddr;          // address the segment is linked to start at
    UINT32  mMemActualBytes;    // # of bytes in those pages *Initialized* by loaded data
    UINT32  mFileOffset;        // offset into the file to the start of the initialized data
    UINT32  mFileBytes;         // bytes of data in the file
    UINT32  mCRC32;             // 32-bit CRC of uncompressed initialized data
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _DLX_SEGMENT32_INFO DLX_SEGMENT32_INFO;

K2_PACKED_PUSH
struct _DLX_SEGMENT64_INFO
{
    UINT64  mLinkAddr;          // address the segment is linked to start at
    UINT64  mMemActualBytes;    // # of bytes in those pages *Initialized* by loaded data
    UINT64  mFileOffset;        // offset into the file to the start of the initialized data
    UINT64  mFileBytes;         // bytes of data in the file
    UINT32  mCRC32;             // 32-bit CRC of uncompressed initialized data
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _DLX_SEGMENT64_INFO DLX_SEGMENT64_INFO;

K2_PACKED_PUSH
struct _DLX_EXPORTS32_SECTION
{
    UINT32              mCount;
    UINT32              mCRC32;
    DLX_EXPORT32_REF    Export[1];    // at least 1 for section to exist
                                      // followed by sorted API strings
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _DLX_EXPORTS32_SECTION DLX_EXPORTS32_SECTION;

K2_PACKED_PUSH
struct _DLX_EXPORTS64_SECTION
{
    UINT32              mCount;
    UINT32              mCRC32;
    DLX_EXPORT64_REF    Export[1];    // at least 1 for section to exist
                                      // followed by sorted API strings
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _DLX_EXPORTS64_SECTION DLX_EXPORTS64_SECTION;

K2_PACKED_PUSH
struct _DLX_INFO32
{
    UINT32                  mElfCRC;
    K2_GUID128              ID;
    DLX_EXPORTS32_SECTION * mpExpCode;
    DLX_EXPORTS32_SECTION * mpExpRead;
    DLX_EXPORTS32_SECTION * mpExpData;
    UINT32                  mImportCount;
    UINT32                  mEntryStackReq;
    DLX_SEGMENT32_INFO      SegInfo[DlxSeg_Count];
    char                    mFileName[4];
    // followed at 4 byte alignment by DLX_IMPORT * mImportCount
};
K2_PACKED_POP
typedef struct _DLX_INFO32 DLX_INFO32;

K2_PACKED_PUSH
struct _DLX_INFO64
{
    UINT32                  mElfCRC;
    K2_GUID128              ID;
    DLX_EXPORTS64_SECTION * mpExpCode;
    DLX_EXPORTS64_SECTION * mpExpRead;
    DLX_EXPORTS64_SECTION * mpExpData;
    UINT32                  mImportCount;
    UINT32                  mEntryStackReq;
    DLX_SEGMENT64_INFO      SegInfo[DlxSeg_Count];
    char                    mFileName[4];
    // followed at 4 byte alignment by DLX_IMPORT * mImportCount
};
K2_PACKED_POP
typedef struct _DLX_INFO64 DLX_INFO64;

#if K2_TARGET_ARCH_IS_32BIT
#define DLX_INFO    DLX_INFO32
#else
#define DLX_INFO    DLX_INFO64
#endif

//
// DLX entrypoint
//
#ifndef DLX_ENTRY_REASON_UNLOAD
#define DLX_ENTRY_REASON_UNLOAD ((UINT_PTR)-1)
#endif

#ifndef DLX_ENTRY_REASON_LOAD
#define DLX_ENTRY_REASON_LOAD   1
#endif

typedef K2STAT (K2_CALLCONV_REGS *DLX_pf_ENTRYPOINT)(DLX * apDlx, UINT_PTR aReason);

K2STAT
K2_CALLCONV_REGS
dlx_entry(
    DLX *       apDlx,
    UINT_PTR    aReason
);

//
// API
//

K2STAT
DLX_Acquire(
    char const *    apFileSpec,
    void *          apContext,
    DLX **          appRetDlx,
    UINT_PTR *      apRetEntryStackReq,
    K2_GUID128 *    apRetID
);

K2STAT
DLX_GetIdent(
    DLX *           apDlx,
    char *          apNameBuf,
    UINT_PTR        aNameBufLen,
    K2_GUID128  *   apRetID
);

K2STAT
DLX_AddRef(
    DLX *   apDlx
);

K2STAT
DLX_Release(
    DLX *   apDlx
);

K2STAT
DLX_FindExport(
    DLX *           apDlx,
    UINT_PTR        aDlxSegment,
    char const *    apExportName,
    UINT_PTR *      apRetAddr
);

K2STAT
DLX_AcquireContaining(
    UINT_PTR    aAddr,
    DLX **      appRetDlx,
    UINT_PTR *  apRetSegment
);

K2STAT
DLX_FindAddrName(
    UINT_PTR    aAddr,
    char *      apRetNameBuffer,
    UINT_PTR    aBufferLen
);

UINT_PTR
DLX_AddrToName(
    DLX *       apDlx,
    UINT_PTR    aAddr,
    UINT_PTR    aSegHint,
    char *      apRetNameBuffer,
    UINT_PTR    aBufferLen
);

#define K2STAT_MAKE_DLX_ERROR(x)                    K2STAT_MAKE_ERROR(K2STAT_FACILITY_DLX, (x))

#define K2STAT_DLX_ERROR_FILE_CORRUPTED             K2STAT_MAKE_DLX_ERROR(1)
#define K2STAT_DLX_ERROR_CRC_WRONG                  K2STAT_MAKE_DLX_ERROR(2)
#define K2STAT_DLX_ERROR_INSIDE_CALLBACK            K2STAT_MAKE_DLX_ERROR(3)
#define K2STAT_DLX_ERROR_ID_MISMATCH                K2STAT_MAKE_DLX_ERROR(4)
#define K2STAT_DLX_ERROR_CODE_IMPORTS_NOT_FOUND     K2STAT_MAKE_DLX_ERROR(5)
#define K2STAT_DLX_ERROR_READ_IMPORTS_NOT_FOUND     K2STAT_MAKE_DLX_ERROR(6)
#define K2STAT_DLX_ERROR_DATA_IMPORTS_NOT_FOUND     K2STAT_MAKE_DLX_ERROR(7)
#define K2STAT_DLX_ERROR_DURING_HANDOFF             K2STAT_MAKE_DLX_ERROR(8)

//
// --------------------------------------------------------------------------------- 
//

#ifdef __cplusplus
};  // extern "C"
#endif

//
// --------------------------------------------------------------------------------- 
//

#endif  // __SPEC_DLX_H

