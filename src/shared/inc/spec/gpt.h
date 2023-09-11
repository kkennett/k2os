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
#ifndef __SPEC_GPT_H
#define __SPEC_GPT_H

//
// --------------------------------------------------------------------------------- 
//

#include <k2basetype.h>
#include <spec/gptdef.inc>

#ifdef __cplusplus
extern "C" {
#endif

K2_PACKED_PUSH

typedef struct _GPT_HEADER  GPT_HEADER;
typedef struct _GPT_SECTOR  GPT_SECTOR;
typedef struct _GPT_ENTRY   GPT_ENTRY;

struct _GPT_HEADER
{
    UINT8       Signature[8];       //  "EFI PART"
    UINT32      Revision;           //  0x00010000
    UINT32      HeaderSize;
    UINT32      HeaderCRC32;
    UINT32      Reserved;
    UINT64      MyLBA;
    UINT64      AlternateLBA;
    UINT64      FirstUsableLBA;
    UINT64      LastUsableLBA;
    K2_GUID128  DiskGuid;
    UINT64      PartitionEntryLBA;
    UINT32      NumberOfPartitionEntries;
    UINT32      SizeOfPartitionEntry;
    UINT32      PartitionEntryArrayCRC32;
} K2_PACKED_ATTRIB;
K2_STATIC_ASSERT(sizeof(GPT_HEADER) == 92);

struct _GPT_SECTOR
{
    GPT_HEADER   Header;
    UINT8        Reserved[GPT_SECTOR_BYTES - 92];
} K2_PACKED_ATTRIB;
K2_STATIC_ASSERT(sizeof(GPT_SECTOR) == GPT_SECTOR_BYTES);

struct _GPT_ENTRY
{
    K2_GUID128  PartitionTypeGuid;
    K2_GUID128  UniquePartitionGuid;
    UINT64      StartingLBA;
    UINT64      EndingLBA;
    UINT64      Attributes;
    UINT16      UnicodePartitionName[36];
} K2_PACKED_ATTRIB;
K2_STATIC_ASSERT(sizeof(GPT_ENTRY) == 128);

K2_PACKED_POP

#ifdef __cplusplus
};  // extern "C"
#endif

/* ------------------------------------------------------------------------- */

#endif  // __SPEC_GPT_H

