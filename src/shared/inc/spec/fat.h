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
#ifndef __SPEC_FAT_H
#define __SPEC_FAT_H

//
// --------------------------------------------------------------------------------- 
//

#include <k2basetype.h>
#include <spec/fatdef.inc>

#ifdef __cplusplus
extern "C" {
#endif

/* 
see :
    Microsoft Extensible Firmware Initiative
    FAT32 File System Specification
    Version 1.03, December 6 2000
*/


K2_PACKED_PUSH
struct _FAT_MBR_PARTITION_ENTRY
{
    UINT8  mBootInd;           /* If 80h means this is boot partition */
    UINT8  mFirstHead;         /* Partition starting head based 0 */
    UINT8  mFirstSector;       /* Partition starting sector based 1 */
    UINT8  mFirstTrack;        /* Partition starting track based 0 */
    UINT8  mFileSystem;        /* Partition type signature field */
    UINT8  mLastHead;          /* Partition ending head based 0 */
    UINT8  mLastSector;        /* Partition ending sector based 1 */
    UINT8  mLastTrack;         /* Partition ending track based 0 */
    UINT32 mStartSector;       /* Logical starting sector based 0 */
    UINT32 mTotalSectors;      /* Total logical sectors in partition */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_MBR_PARTITION_ENTRY FAT_MBR_PARTITION_ENTRY;

//
// FAT volume:
//
//     <RESERVED REGION>|<FAT REGION>|<ROOT DIR REGION>|<DATA REGION>
//     <BPB> <BPB>      |<FAT><FAT2> |<FAT12/16 ONLY>  
//
//  All FAT volumes must have a BPB in the first sector of the reserved region
//  which is sector 0 or the 'boot sector'
//
// 0    boot sector 0
// 1    FSINFO (FAT32 ONLY), FAT32 boot sector 1
// 2    backup boot sector (FAT16 ONLY)
// 3    reserved/ vendor specific
// 4    CAN BE START OF FAT
// 5
// 6    backup boot sector (FAT32 ONLY)
// 7    backup FSINFO (FAT32 ONLY)
//
// MS formats with 32 reserved sectors prior to the first FAT 
//

K2_PACKED_PUSH
struct _FAT_GENERIC_BOOTSECTOR
{
    UINT8   BS_jmpBoot[3];      /*   0 */
    UINT8   BS_OEMName[8];      /*   3 CAN BE ANYTHING */
    UINT16  BPB_BytesPerSec;    /*  11 NOT ALIGNED - CAN ONLY BE 512, 1024, 2048, or 4096 */
    UINT8   BPB_SecPerClus;     /*  13 - CAN ONLY BE POW2 BETWEEN 1 and 128 INCLUSIVE */
    UINT16  BPB_ResvdSecCnt;    /*  14 - USED TO ALIGN !!DATA AREA!! TO CLUSTER ALIGNMENT */
    UINT8   BPB_NumFATs;        /*  16 */
    UINT16  BPB_RootEntCnt;     /*  17 NOT ALIGNED - FAT16 SHOULD USE 512 */
    UINT16  BPB_TotSec16;       /*  19 NOT ALIGNED */
    UINT8   BPB_Media;          /*  21 - 0xF0 to 0xFF. F8 is FIXED, F0 is REMOVABLE */
    UINT16  BPB_FATSz16;        /*  22 SECTORS OCCUPIED BY ONE FAT; FAT32 THIS IS ZERO */
    UINT16  BPB_SecPerTrk;      /*  24 INT13 USE */
    UINT16  BPB_NumHeads;       /*  26 INT13 USE */
    UINT32  BPB_HiddSec;        /*  28 INT13 VOL OFFSET ON MEDIA. UNPARTITIONED MUST BE ZERO */
    UINT32  BPB_TotSec32;       /*  32 FAT32 MUST BE NONZERO */
    UINT8   BS_NonGeneric[474]; /*  36 */
    UINT16  BS_Signature;       /*  510 - 0x55, 0xAA, or 0xAA55 as UINT16 */
    // ZERO FILL IF SECTOR SIZE > 512 BYTES */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_GENERIC_BOOTSECTOR FAT_GENERIC_BOOTSECTOR;

K2_PACKED_PUSH
struct _FAT_IBMPC_INIT_TABLE
{
    UINT8   mSpecify1;                  /* 00 */
    UINT8   mSpecify2;                  /* 01 */
    UINT8   mMotorOffDelay;             /* 02 */
    UINT8   mBytesPerSector;            /* 03 */
    UINT8   mSectorsPerTrack;           /* 04 */
    UINT8   mInterSectorGap;            /* 05 */
    UINT8   mDataLength;                /* 06 */
    UINT8   mFormatGapLength;           /* 07 */
    UINT8   mFillerF6;                  /* 08 */
    UINT8   mHeadSettleTimeMs;          /* 09 */
    UINT8   mMotorStartTimeOctoSec;     /* 0A */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_IBMPC_INIT_TABLE FAT_IBMPC_INIT_TABLE;

K2_PACKED_PUSH
struct _FAT_BOOTSECTOR12OR16
{
    UINT8   BS_jmpBoot[3];      /*   0 */
    UINT8   BS_OEMName[8];      /*   3 CAN BE ANYTHING */
    UINT16  BPB_BytesPerSec;    /*  11 NOT ALIGNED - CAN ONLY BE 512, 1024, 2048, or 4096 */
    UINT8   BPB_SecPerClus;     /*  13 - CAN ONLY BE POW2 BETWEEN 1 and 128 INCLUSIVE */
    UINT16  BPB_ResvdSecCnt;    /*  14 - USED TO ALIGN !!DATA AREA!! TO CLUSTER ALIGNMENT */
    UINT8   BPB_NumFATs;        /*  16 */
    UINT16  BPB_RootEntCnt;     /*  17 NOT ALIGNED - FAT16 SHOULD USE 512 */
    UINT16  BPB_TotSec16;       /*  19 NOT ALIGNED */
    UINT8   BPB_Media;          /*  21 - 0xF0 to 0xFF. F8 is FIXED, F0 is REMOVABLE */
    UINT16  BPB_FATSz16;        /*  22 SECTORS OCCUPIED BY ONE FAT; FAT32 THIS IS ZERO */
    UINT16  BPB_SecPerTrk;      /*  24 INT13 USE */
    UINT16  BPB_NumHeads;       /*  26 INT13 USE */
    UINT32  BPB_HiddSec;        /*  28 INT13 VOL OFFSET ON MEDIA. UNPARTITIONED MUST BE ZERO */
    UINT32  BPB_TotSec32;       /*  32 FAT32 MUST BE NONZERO */
    // NONGENERIC BELOW HERE
    UINT8   BS_DrvNum;          /*  36 INT13 - 0x80 (FIXED) or 0x00 (REMOVABLE) */
    UINT8   BS_Reserved1;       /*  37 SET ZERO */
    UINT8   BS_BootSig;         /*  38 - SET TO 0x29 IF EITHER OF NEXT TWO FIELDS ARE NONZERO */
    UINT32  BS_VolID;           /*  39 NOT ALIGNED - SERIAL NUMBER - GENERATE BY COMBINING DATE & TIME */
    UINT8   BS_VolLab[11];      /*  43 - MATCHES 11-BYTE VOLUME LABEL IN ROOT DIR. WHEN NO LABEL = "NO NAME    " */
    UINT8   BS_FilSysType[8];   /*  54 - "FAT12   ", "FAT16   ", or "FAT     " - INFORMATIONAL ONLY */
    UINT8   BS_Code[448];       /*  62 + 448 = 510 */
    UINT16  BS_Signature;       /*  510 - 0x55, 0xAA, or 0xAA55 as UINT16 */
    // ZERO FILL IF SECTOR SIZE > 512 BYTES */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_BOOTSECTOR12OR16 FAT_BOOTSECTOR12OR16;

K2_PACKED_PUSH
struct _FAT_BOOTSECTOR32
{
    UINT8   BS_jmpBoot[3];      /*   0 */
    UINT8   BS_OEMName[8];      /*   3 CAN BE ANYTHING */
    UINT16  BPB_BytesPerSec;    /*  11 NOT ALIGNED - CAN ONLY BE 512, 1024, 2048, or 4096 */
    UINT8   BPB_SecPerClus;     /*  13 - CAN ONLY BE POW2 BETWEEN 1 and 128 INCLUSIVE */
    UINT16  BPB_ResvdSecCnt;    /*  14 - USED TO ALIGN !!DATA AREA!! TO CLUSTER ALIGNMENT */
    UINT8   BPB_NumFATs;        /*  16 */
    UINT16  BPB_RootEntCnt;     /*  17 NOT ALIGNED - FAT16 SHOULD USE 512 */
    UINT16  BPB_TotSec16;       /*  19 NOT ALIGNED */
    UINT8   BPB_Media;          /*  21 - 0xF0 to 0xFF. F8 is FIXED, F0 is REMOVABLE */
    UINT16  BPB_FATSz16;        /*  22 SECTORS OCCUPIED BY ONE FAT; FAT32 THIS IS ZERO */
    UINT16  BPB_SecPerTrk;      /*  24 INT13 USE */
    UINT16  BPB_NumHeads;       /*  26 INT13 USE */
    UINT32  BPB_HiddSec;        /*  28 INT13 VOL OFFSET ON MEDIA. UNPARTITIONED MUST BE ZERO */
    UINT32  BPB_TotSec32;       /*  32 FAT32 MUST BE NONZERO */
    // NONGENERIC BELOW HERE
    UINT32  BPB_FATSz32;        /*  36 SECTORS OCCUPIED BY ONE FAT */
    UINT16  BPB_ExtFlags;       /*  40 ; BIT0-3 ACTIVE FAT; BIT7 (0) MIRRORED AT RUNTIME TO ALL, OR (1) ONLY ONE ACTIVE */
    UINT16  BPB_FSVer;          /*  42 SET ZERO */
    UINT32  BPB_RootClus;       /*  44 SET TO 2 OR FIRST NON-BAD CLUSTER AFTER THAT */
    UINT16  BPB_FSInfo;         /*  48 SECTOR # IN RESERVED AREA OF ACTIVE FSINFO */
    UINT16  BPB_BkBootSec;      /*  50 SET TO ZERO OR SIX IF THERE IS A COPY OF THE BACKUP BOOT SECTOR */
    UINT8   BPB_Reserved[12];   /*  52 */
    UINT8   BS_DrvNum;          /*  64 INT13 - 0x80 (FIXED) or 0x00 (REMOVABLE) */
    UINT8   BS_Reserved1;       /*  65 SET ZERO */
    UINT8   BS_BootSig;         /*  66 - SET TO 0x29 IF EITHER OF NEXT TWO FIELDS ARE NONZERO */
    UINT32  BS_VolID;           /*  67 NOT ALIGNED - SERIAL NUMBER - GENERATE BY COMBINING DATE & TIME */
    UINT8   BS_VolLab[11];      /*  71 - MATCHES 11-BYTE VOLUME LABEL IN ROOT DIR. WHEN NO LABEL = "NO NAME    " */
    UINT8   BS_FilSysType[8];   /*  82 - "FAT32   " */
    UINT8   BS_Code[420];       /*  90 + 420 = 510 */
    UINT16  BS_Signature;       /* 510 - 0x55, 0xAA, or 0xAA55 as UINT16 */
    // ZERO FILL IF SECTOR SIZE > 512 BYTES */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_BOOTSECTOR32 FAT_BOOTSECTOR32;

K2_PACKED_PUSH
struct _FAT32_FSINFO
{
    UINT32  FSI_LeadSig;        /*   0 0x41615252 */
    UINT8   FSI_Reserved1[480]; /*   4 ZERO */
    UINT32  FSI_StructSig;      /* 484 0x61417272 */
    UINT32  FSI_Free_Count;     /* 488 LAST KNOWN FREE CLUSTER COUNT */
    UINT32  FSI_Nxt_Free;       /* 492 FIRST AVAILABLE FREE CLUSTER */
    UINT8   FSI_Reserved2[12];  /* 496 ZERO */
    UINT8   FSI_TrailSig;       /* 509 0xAA550000 */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT32_FSINFO FAT32_FSINFO;

// name can contain $%'-_@~`!(){}^#& 
// create, rename, or modify sets the archive bit
// long name can also contain +,;=[]
// leading and trailing spaces are ignored

K2_PACKED_PUSH
struct _FAT_DIRENTRY
{
    UINT8   DIR_Name[11];           /*  0 UPPER CASE ONLY. ILLEGAL are < 0x20, 0x22, 0x2A-0x2C, 0x2E-0x2F, 0x3A-0x3F, 0x5B-0x5D, 0x7C */
    UINT8   DIR_Attr;               /* 11 ONLY THE ROOT DIR CAN HAVE AN ENTRY WITH THE VOLUME_ID BIT SET */
    UINT8   DIR_NTRes;              /* 12 ZERO */
    UINT8   DIR_CrtTimeTenth;       /* 13 tenths of a second 0 <= x <= 199 */
    UINT16  DIR_CrtTime;            /* 14 gran 2 seconds */
    UINT16  DIR_CrtDate;            /* 16 */
    UINT16  DIR_LstAccDate;         /* 18 must = DIR_WrtDate */
    UINT16  DIR_FstClusHI;          /* 20 FAT32 only, must be set zero otherwise, zero for volume label, or for .. entry in a directory off the root */
    UINT16  DIR_WrtTime;            /* 22 must = DIR_CrtTime at file creation */
    UINT16  DIR_WrtDate;            /* 24 most = DIR_CrtDate at file createion */
    UINT16  DIR_FstClusLO;          /* 26 zero for volume label, or for .. entry in a directory off the root, or for an empty file */
    UINT32  DIR_FileSize;           /* 28 in bytes;  must be zero for directories */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_DIRENTRY FAT_DIRENTRY;

K2_PACKED_PUSH
struct _FAT_LONGENTRY
{
    UINT8   LDIR_Ord;               /*  0 */
    UINT16  LDIR_Name1[5];          /*  1 chars 1-5 */
    UINT8   LDIR_Attr;              /* 11 : always == 0x0F */
    UINT8   LDIR_Type;              /* 12 ZERO */
    UINT8   LDIR_Chksum;            /* 13 */
    UINT16  LDIR_Name2[6];          /* 14 chars 6-11 */
    UINT16  LDIR_FstClusLO;         /* 26 ZERO */
    UINT16  LDIR_Name3[2];          /* 28 chars 12-13 */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_LONGENTRY FAT_LONGENTRY;

#ifdef __cplusplus
};  // extern "C"
#endif

/* ------------------------------------------------------------------------- */

#endif  // __SPEC_FAT_H

