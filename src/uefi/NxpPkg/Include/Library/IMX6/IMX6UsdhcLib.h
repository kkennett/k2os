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
#ifndef __IMX6USDHCLIB_H__
#define __IMX6USDHCLIB_H__

#include <Uefi.h>
#include "../../IMX6def.inc"
#include <Library/IMX6/IMX6ClockLib.h>
#include <Library/IMX6/IMX6GptLib.h>

// -----------------------------------------------------------------------------

#define SDMMC_BLOCK_SIZE    512

#define SDMMC_STATIC_ASSERT_SIZE(x,size) \
typedef int SDMMC_STATIC_ASSERT_SIZE_ ## x [(sizeof(x) == size) ? 1 : -1];

typedef struct _SDMMC_CID
{
    UINT32  rz0 : 1;
    UINT32  CRC : 7;
    UINT32  MDT : 8;
    UINT32  PSN_LO : 16;

    UINT32  PSN_HI : 16;
    UINT32  PRV : 8;
    UINT32  PNM_LO : 8;

    UINT32  PNM_MID;

    UINT32  PNM_HI : 8;
    UINT32  OID : 8;
    UINT32  CBX : 2;
    UINT32  rz119 : 6;
    UINT32  MID : 8;
} SDMMC_CID;
SDMMC_STATIC_ASSERT_SIZE(SDMMC_CID, sizeof(UINT32) * 4);

typedef struct _SDMMC_CSDV1
{
    UINT32  NOT_USED : 1;
    UINT32  CRC : 7;
    UINT32  RESERVED_1 : 2;
    UINT32  FILE_FORMAT : 2;
    UINT32  TMP_WRITE_PROTECT : 1;
    UINT32  PERM_WRITE_PROTECT : 1;
    UINT32  COPY : 1;
    UINT32  FILE_FORMAT_GRP : 1;
    UINT32  RESERVED_2 : 5;
    UINT32  WRITE_BL_PARTIAL : 1;
    UINT32  WRITE_BL_LEN : 4;
    UINT32  R2W_FACTOR : 3;
    UINT32  RESERVED_3 : 2;
    UINT32  WP_GRP_ENABLE : 1;

    UINT32  WP_GRP_SIZE : 7;
    UINT32  ERASE_SECTOR_SIZE : 7;
    UINT32  ERASE_BLK_EN : 1;
    UINT32  C_SIZE_MULT : 3;
    UINT32  VDD_W_CURR_MAX : 3;
    UINT32  VDD_W_CURR_MIN : 3;
    UINT32  VDD_R_CURR_MAX : 3;
    UINT32  VDD_R_CURR_MIN : 3;
    UINT32  C_SIZELow : 2;

    UINT32  C_SIZEHigh : 10;
    UINT32  RESERVED_4 : 2;
    UINT32  DSR_IMP : 1;
    UINT32  READ_BLK_MISALIGN : 1;
    UINT32  WRITE_BLK_MISALIGN : 1;
    UINT32  READ_BL_PARTIAL : 1;
    UINT32  READ_BL_LEN : 4;
    UINT32  CCC : 12;

    UINT32  TRAN_SPEED : 8;
    UINT32  NSAC : 8;
    UINT32  TAAC : 8;
    UINT32  RESERVED_5 : 6;
    UINT32  CSD_STRUCTURE : 2;
} SDMMC_CSDV1;
SDMMC_STATIC_ASSERT_SIZE(SDMMC_CSDV1, sizeof(UINT32) * 4);

typedef struct _SDMMC_CSDV2
{
    UINT32  NOT_USED : 1;
    UINT32  CRC : 7;
    UINT32  RESERVED_1 : 2;
    UINT32  FILE_FORMAT : 2;
    UINT32  TMP_WRITE_PROTECT : 1;
    UINT32  PERM_WRITE_PROTECT : 1;
    UINT32  COPY : 1;
    UINT32  FILE_FORMAT_GRP : 1;
    UINT32  RESERVED_2 : 5;
    UINT32  WRITE_BL_PARTIAL : 1;
    UINT32  WRITE_BL_LEN : 4;
    UINT32  R2W_FACTOR : 3;
    UINT32  RESERVED_3 : 2;
    UINT32  WP_GRP_ENABLE : 1;

    UINT32  WP_GRP_SIZE : 7;
    UINT32  SECTOR_SIZE : 7;
    UINT32  ERASE_BLK_EN : 1;
    UINT32  RESERVED_4 : 1;
    UINT32  C_SIZELow16 : 16;

    UINT32  C_SIZEHigh6 : 6;
    UINT32  RESERVED_5 : 6;
    UINT32  DSR_IMP : 1;
    UINT32  READ_BLK_MISALIGN : 1;
    UINT32  WRITE_BLK_MISALIGN : 1;
    UINT32  READ_BL_PARTIAL : 1;
    UINT32  READ_BL_LEN : 4;
    UINT32  CCC : 12;

    UINT32  TRAN_SPEED : 8;
    UINT32  NSAC : 8;
    UINT32  TAAC : 8;
    UINT32  RESERVED_6 : 6;
    UINT32  CSD_STRUCTURE : 2;
} SDMMC_CSDV2;
SDMMC_STATIC_ASSERT_SIZE(SDMMC_CSDV2, sizeof(UINT32) * 4);

typedef struct _SDMMC_CSDANY
{
    UINT32  Reg[3];
    UINT32  TRAN_SPEED : 8;
    UINT32  NSAC : 8;
    UINT32  TAAC : 8;
    UINT32  RESERVED_5 : 6;
    UINT32  CSD_STRUCTURE : 2;
} SDMMC_CSDANY;
SDMMC_STATIC_ASSERT_SIZE(SDMMC_CSDANY, sizeof(UINT32) * 4);

typedef union _SDMMC_CSD
{
    SDMMC_CSDV1     V1;
    SDMMC_CSDV2     V2;
    SDMMC_CSDANY    ANY;
} SDMMC_CSD;

typedef struct _IMX6_SDCARD_INFO IMX6_SDCARD_INFO;
struct _IMX6_SDCARD_INFO
{
    UINT32      RCA;
    SDMMC_CID   CID;
    SDMMC_CSD   CSD;
    BOOLEAN     HighCapCard;
    UINT32      MediaId;
    UINT32      BlockSize;
    UINT32      BlockCount;
};

typedef struct _IMX6_USDHC IMX6_USDHC;
struct _IMX6_USDHC
{
    UINT32              mUnitNum;
    UINT32              mRegs_USDHC;
    UINT32              mRegs_CCM;
    IMX6_GPT *          mpGpt;
    IMX6_SDCARD_INFO    Info;
};

BOOLEAN
EFIAPI
IMX6_USDHC_Setup(
    IMX6_USDHC *    apUSDHC,
    UINT32          aUnitNum,
    UINT32          aRegs_CCM,
    IMX6_GPT *      apGpt
);

BOOLEAN
EFIAPI
IMX6_USDHC_Initialize(
    IMX6_USDHC *    apUSDHC
);

EFI_STATUS
EFIAPI
IMX6_USDHC_DoTransfer(
    IMX6_USDHC *    apUSDHC,
    BOOLEAN         aIsWrite,
    EFI_LBA         Lba,
    UINTN           BufferSizeBytes,
    VOID *          Buffer
);


// -----------------------------------------------------------------------------

#endif  // __IMX6USDHCLIB_H__
