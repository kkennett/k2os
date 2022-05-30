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

K2_PACKED_PUSH
struct _FAT_GENERIC_BOOTSECTOR
{
    UINT8   BS_jmpBoot[3];      /*   0 */
    UINT8   BS_OEMName[8];      /*   3 */
    UINT16  BPB_BytesPerSec;    /*  11 NOT ALIGNED */
    UINT8   BPB_SecPerClus;     /*  13 */
    UINT16  BPB_ResvdSecCnt;    /*  14 */
    UINT8   BPB_NumFATs;        /*  16 */
    UINT16  BPB_RootEntCnt;     /*  17 NOT ALIGNED */
    UINT16  BPB_TotSec16;       /*  19 NOT ALIGNED */
    UINT8   BPB_Media;          /*  21 */
    UINT16  BPB_FATSz16;        /*  22 */
    UINT16  BPB_SecPerTrk;      /*  24 */
    UINT16  BPB_NumHeads;       /*  26 */
    UINT32  BPB_HiddSec;        /*  28 */
    UINT32  BPB_TotSec32;       /*  32 */
    UINT8   BS_NonGeneric[474]; /*  36 */
    UINT16  BS_Signature;       /*  510 */
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
struct _FAT_BOOTSECTOR12
{
    UINT8   BS_jmpBoot[3];      /*   0 */
    UINT8   BS_OEMName[8];      /*   3 */
    UINT16  BPB_BytesPerSec;    /*  11 NOT ALIGNED */
    UINT8   BPB_SecPerClus;     /*  13 */
    UINT16  BPB_ResvdSecCnt;    /*  14 */
    UINT8   BPB_NumFATs;        /*  16 */
    UINT16  BPB_RootEntCnt;     /*  17 NOT ALIGNED */
    UINT16  BPB_TotSec16;       /*  19 NOT ALIGNED */
    UINT8   BPB_Media;          /*  21 */
    UINT16  BPB_FATSz16;        /*  22 */
    UINT16  BPB_SecPerTrk;      /*  24 */
    UINT16  BPB_NumHeads;       /*  26 */
    UINT32  BPB_HiddSec;        /*  28 */
    UINT32  BPB_TotSec32;       /*  32 */
    UINT8   BS_DrvNum;          /*  36 */
    UINT8   BS_Reserved1;       /*  37 */
    UINT8   BS_BootSig;         /*  38 */
    UINT32  BS_VolID;           /*  39 NOT ALIGNED */
    UINT8   BS_VolLab[11];      /*  43 */
    UINT8   BS_FilSysType[8];   /*  54 */
    UINT8   BS_Code[455];       /*  55 + 455 = 510 */
    UINT16  BS_Signature;       /*  510 */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_BOOTSECTOR12 FAT_BOOTSECTOR12;

K2_PACKED_PUSH
struct _FAT_BOOTSECTOR16
{
    UINT8   BS_jmpBoot[3];      /*   0 */
    UINT8   BS_OEMName[8];      /*   3 */
    UINT16  BPB_BytesPerSec;    /*  11 NOT ALIGNED */
    UINT8   BPB_SecPerClus;     /*  13 */
    UINT16  BPB_ResvdSecCnt;    /*  14 */
    UINT8   BPB_NumFATs;        /*  16 */
    UINT16  BPB_RootEntCnt;     /*  17 NOT ALIGNED */
    UINT16  BPB_TotSec16;       /*  19 NOT ALIGNED */
    UINT8   BPB_Media;          /*  21 */
    UINT16  BPB_FATSz16;        /*  22 */
    UINT16  BPB_SecPerTrk;      /*  24 */
    UINT16  BPB_NumHeads;       /*  26 */
    UINT32  BPB_HiddSec;        /*  28 */
    UINT32  BPB_TotSec32;       /*  32 */
    UINT8   BS_DrvNum;          /*  36 */
    UINT8   BS_Reserved1;       /*  37 */
    UINT8   BS_BootSig;         /*  38 */
    UINT32  BS_VolID;           /*  39 NOT ALIGNED */
    UINT8   BS_VolLab[11];      /*  43 */
    UINT8   BS_FilSysType[8];   /*  54 */
    UINT8   BS_Code[455];       /*  55 + 455 = 510 */
    UINT16  BS_Signature;       /*  510 */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_BOOTSECTOR16 FAT_BOOTSECTOR16;

K2_PACKED_PUSH
struct _FAT_BOOTSECTOR32
{
    UINT8   BS_jmpBoot[3];      /*   0 */
    UINT8   BS_OEMName[8];      /*   3 */
    UINT16  BPB_BytesPerSec;    /*  11 NOT ALIGNED */
    UINT8   BPB_SecPerClus;     /*  13 */
    UINT16  BPB_ResvdSecCnt;    /*  14 */
    UINT8   BPB_NumFATs;        /*  16 */
    UINT16  BPB_RootEntCnt;     /*  17 NOT ALIGNED */
    UINT16  BPB_TotSec16;       /*  19 NOT ALIGNED */
    UINT8   BPB_Media;          /*  21 */
    UINT16  BPB_FATSz16;        /*  22 */
    UINT16  BPB_SecPerTrk;      /*  24 */
    UINT16  BPB_NumHeads;       /*  26 */
    UINT32  BPB_HiddSec;        /*  28 */
    UINT32  BPB_TotSec32;       /*  32 */
    UINT32  BPB_FATSz32;        /*  36 */
    UINT16  BPB_ExtFlags;       /*  40 */
    UINT16  BPB_FSVer;          /*  42 */
    UINT32  BPB_RootClus;       /*  44 */
    UINT16  BPB_FSInfo;         /*  48 */
    UINT16  BPB_BkBootSec;      /*  50 */
    UINT8   BPB_Reserved[12];   /*  52 */
    UINT8   BS_DrvNum;          /*  64 */
    UINT8   BS_Reserved1;       /*  65 */
    UINT8   BS_BootSig;         /*  66 */
    UINT32  BS_VolID;           /*  67 NOT ALIGNED */
    UINT8   BS_VolLab[11];      /*  71 */
    UINT8   BS_FilSysType[8];   /*  82 */
    UINT8   BS_Code[427];       /*  83 + 427 = 510 */
    UINT16  BS_Signature;       /* 510 */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_BOOTSECTOR32 FAT_BOOTSECTOR32;

K2_PACKED_PUSH
struct _FAT_DIRENTRY
{
    UINT8   mFileName[8];           /*  0 */
    UINT8   mExtension[3];          /*  8 */
    UINT8   mAttribute;             /* 11 */
    UINT8   mReserved1;             /* 12 */
    UINT8   mCreateFine;            /* 13 */
    UINT16  mCreateTime;            /* 14 */
    UINT16  mCreateDate;            /* 16 */
    UINT16  mLastModifyFine;        /* 18 */
    UINT16  mEAorHighCluster;       /* 20 */
    UINT16  mLastModifyTime;        /* 22 */
    UINT16  mLastModifyDate;        /* 24 */
    UINT16  mStartClusterLow;       /* 26 */
    UINT32  mBytesLength;           /* 28 */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_DIRENTRY FAT_DIRENTRY;

K2_PACKED_PUSH
struct _FAT_LONGENTRY
{
    UINT8   mSequenceNumber;        /*  0 */
    UINT16  mNameChars1[5];         /*  1 */
    UINT8   mAttribute;             /* 11 : always == 0x0F */
    UINT8   mCaseSpec;              /* 12 */
    UINT8   m83NameChecksum;        /* 13 */
    UINT16  mNameChars2[6];         /* 14 */
    UINT16  mMustBeZero;            /* 26 */
    UINT16  mNameChars3[2];         /* 28 */
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _FAT_LONGENTRY FAT_LONGENTRY;

#ifdef __cplusplus
};  // extern "C"
#endif

/* ------------------------------------------------------------------------- */

#endif  // __SPEC_FAT_H

