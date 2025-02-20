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
#ifndef __IDE_H
#define __IDE_H

#include <k2osddk.h>
#include <k2osdev_blockio.h>

/* ------------------------------------------------------------------------- */

typedef struct _IDE_CONTROLLER      IDE_CONTROLLER;

typedef struct _IDE_CHANNEL         IDE_CHANNEL;
#define IDE_CHANNEL_PRIMARY         0
#define IDE_CHANNEL_SECONDARY       1

typedef struct _IDE_DEVICE          IDE_DEVICE;
#define IDE_CHANNEL_DEVICE_MASTER   0
#define IDE_CHANNEL_DEVICE_SLAVE    1

typedef struct _IDE_DEVICE_USER IDE_DEVICE_USER;
struct _IDE_DEVICE_USER
{
    UINT32          mProcessId;
    IDE_DEVICE *    mpDevice;
    K2LIST_LINK     DeviceUserListLink;
};

/* ------------------------------------------------------------------------- */

// status register
#define ATA_SR_BSY                  0x80    // Busy
#define ATA_SR_DRDY                 0x40    // Drive ready
#define ATA_SR_DF                   0x20    // Drive write fault
#define ATA_SR_DSC                  0x10    // Drive seek complete
#define ATA_SR_DRQ                  0x08    // Data request ready
#define ATA_SR_CORR                 0x04    // Corrected data
#define ATA_SR_IDX                  0x02    // Index
#define ATA_SR_ERR                  0x01    // Error

// error register 
#define ATA_ER_BBK                  0x80    // Bad block
#define ATA_ER_UNC                  0x40    // Uncorrectable data
#define ATA_ER_MC                   0x20    // Media changed
#define ATA_ER_IDNF                 0x10    // ID mark not found
#define ATA_ER_MCR                  0x08    // Media change request
#define ATA_ER_ABRT                 0x04    // Command aborted
#define ATA_ER_TK0NF                0x02    // Track 0 not found
#define ATA_ER_AMNF                 0x01    // No address mark

// ATA commands
#define ATA_CMD_READ_PIO            0x20
#define ATA_CMD_READ_PIO_EXT        0x24
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_WRITE_PIO           0x30
#define ATA_CMD_WRITE_PIO_EXT       0x34
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_WRITE_DMA_EXT       0x35
#define ATA_CMD_CACHE_FLUSH         0xE7
#define ATA_CMD_CACHE_FLUSH_EXT     0xEA
#define ATA_CMD_PACKET              0xA0
#define ATA_CMD_IDENTIFY_PACKET     0xA1
#define ATA_CMD_IDENTIFY            0xEC
#define ATA_CMD_SET_FEATURES        0xEF
#define ATA_CMD_GET_MEDIA_STATUS    0xDA

// ATAPI commands
#define ATAPI_CMD_READ              0xA8
#define ATAPI_CMD_EJECT             0x1B

// Directions:
#define ATA_DIR_READ                0x00
#define ATA_DIR_WRITE               0x01

#define ATA_DCR_INT_MASK            0x02
#define ATA_DCR_SW_RESET            0x04
#define ATA_DCR_HOB_SETUP           0x80

#define ATA_HDDSEL_HEAD_MASK        0x0F
#define ATA_HDDSEL_DEVICE_SEL       0x10
#define ATA_HDDSEL_LBA_MODE         0x40
#define ATA_HDDSEL_STATIC           0xA0

#define ATA_STATUS_MASTER_ACTIVE    0x01
#define ATA_STATUS_SLAVE_ACTIVE     0x02
#define ATA_STATUS_WRITING          0x40

#define ATA_MODE_LBA28              1
#define ATA_MODE_LBA48              2

#define ATA_SECTOR_BYTES            512

/* ------------------------------------------------------------------------- */

#define ATA_IDENTIFY_GENCONFIG_RESPONSE_INCOMPLETE      0x0004
#define ATA_IDENTIFY_GENCONFIG_MEDIA_IS_REMOVABLE       0x0080
#define ATA_IDENTIFY_GENCONFIG_DEVICE_IS_NOT_ATA        0x8000

#define ATA_IDENTIFY_TRUSTED_SUPPORTED                  0x0001

#define ATA_IDENTIFY_CAPS_LONG_ALIGN_MASK               0x00000003
#define ATA_IDENTIFY_CAPS_DMA_SUPPORTED                 0x00000100
#define ATA_IDENTIFY_CAPS_LBA_SUPPORTED                 0x00000200
#define ATA_IDENTIFY_CAPS_IORDY_DISABLE                 0x00000400
#define ATA_IDENTIFY_CAPS_IORDY_SUPPORTED               0x00000800
#define ATA_IDENTIFY_CAPS_SBY_TIMER_SUPPORT             0x00002000

#define ATA_IDENTIFY_TRANS_SENS_VALID                   0x0003
#define ATA_IDENTIFY_TRANS_SENS_FREEFALL_MASK           0xFF00
#define ATA_IDENTIFY_TRANS_SENS_FREEFALL_SHL            8

#define ATA_IDENTIFY_FEATURE1_MULTISECTOR_VALID         0x01
#define ATA_IDENTIFY_FEATURE1_SANITIZE_SUPPORT          0x10
#define ATA_IDENTIFY_FEATURE1_CRYPTO_SUPP               0x20
#define ATA_IDENTIFY_FEATURE1_OVERWR_SUPP               0x40
#define ATA_IDENTIFY_FEATURE1_BLOCK_ERASE_SUPP          0x80

#define ATA_IDENTIFY_FEATURE2_MULTIDMA_SUPP_MASK        0x00FF
#define ATA_IDENTIFY_FEATURE2_MULTIDMA_ACTIVE_MASK      0xFF00

#define ATA_IDENTIFY_FEATURE3_ADV_PIO_MODES_MASK        0x00FF

#define ATA_IDENTIFY_FEATURE4_ZONED_CAPS_MASK           0x0003
#define ATA_IDENTIFY_FEATURE4_NV_WRITE_CACHE            0x0004
#define ATA_IDENTIFY_FEATURE4_EXT_USER_SUPP             0x0008
#define ATA_IDENTIFY_FEATURE4_DEV_ENC_ALL_DATA          0x0010
#define ATA_IDENTIFY_FEATURE4_READZ_TRIM_SUPP           0x0020
#define ATA_IDENTIFY_FEATURE4_12BIT_COMM_SUPP           0x0040
#define ATA_IDENTIFY_FEATURE4_IEEE1667                  0x0080
#define ATA_IDENTIFY_FEATURE4_DLD_MCODE_DMA_SUPP        0x0100
#define ATA_IDENTIFY_FEATURE4_PWD_DMA_SUPP              0x0200
#define ATA_IDENTIFY_FEATURE4_WRITE_BUF_DMA_SUPP        0x0400
#define ATA_IDENTIFY_FEATURE4_READ_BUF_DMA_SUPP         0x0800
#define ATA_IDENTIFY_FEATURE4_DEVCFG_DMA_SUPP           0x1000
#define ATA_IDENTIFY_FEATURE4_LPSAERCS_SUPP             0x2000
#define ATA_IDENTIFY_FEATURE4_DET_READ_AFTER_TRIM_SUPP  0x4000
#define ATA_IDENTIFY_FEATURE4_CFAST_SPEC_SUPP           0x8000

#define ATA_IDENTIFY_QUEUEDEPTH_MASK                    0x001F

#define ATA_IDENTIFY_VERSION_SUPP_ATA6                  0x0040

#define ATA_IDENTIFY_CMDSET1_SMART                      0x0001
#define ATA_IDENTIFY_CMDSET1_SECMODE                    0x0002
#define ATA_IDENTIFY_CMDSET1_REMOVABLE_MEDIA            0x0004
#define ATA_IDENTIFY_CMDSET1_PWR_MGMT                   0x0008
#define ATA_IDENTIFY_CMDSET1_WRITE_CACHE                0x0020
#define ATA_IDENTIFY_CMDSET1_LOOK_AHEAD                 0x0040
#define ATA_IDENTIFY_CMDSET1_RELEASE_INTR               0x0080
#define ATA_IDENTIFY_CMDSET1_SERVICE_INTR               0x0100
#define ATA_IDENTIFY_CMDSET1_DEVICE_RESET               0x0200
#define ATA_IDENTIFY_CMDSET1_HOST_PROT_AREA             0x0400
#define ATA_IDENTIFY_CMDSET1_WRITE_BUFFER               0x1000
#define ATA_IDENTIFY_CMDSET1_READ_BUFFER                0x2000
#define ATA_IDENTIFY_CMDSET1_NOP                        0x4000

#define ATA_IDENTIFY_CMDSET2_DLD_MICROCODE              0x0001
#define ATA_IDENTIFY_CMDSET2_DMA_QUEUED                 0x0002
#define ATA_IDENTIFY_CMDSET2_CFA                        0x0004
#define ATA_IDENTIFY_CMDSET2_ADVANCED_PM                0x0008
#define ATA_IDENTIFY_CMDSET2_MSN                        0x0010
#define ATA_IDENTIFY_CMDSET2_POWER_UP_IN_SBY            0x0020
#define ATA_IDENTIFY_CMDSET2_MANUAL_POWER_UP            0x0040
#define ATA_IDENTIFY_CMDSET2_SEX_MAX                    0x0100
#define ATA_IDENTIFY_CMDSET2_ACOUSTICS                  0x0200
#define ATA_IDENTIFY_CMDSET2_BIG_LBA                    0x0400
#define ATA_IDENTIFY_CMDSET2_DEVCFG_OVERLAY             0x0800
#define ATA_IDENTIFY_CMDSET2_FLUSH_CACHE                0x1000
#define ATA_IDENTIFY_CMDSET2_FLUSH_CACHE_EXT            0x2000
#define ATA_IDENTIFY_CMDSET2_VALID_MASK                 0xC000

#define ATA_IDENTIFY_CMDSET3_SMART_ERRLOG               0x0001
#define ATA_IDENTIFY_CMDSET3_SMART_SELF_TST             0x0002
#define ATA_IDENTIFY_CMDSET3_MEDIA_SERIAL_NUM           0x0004
#define ATA_IDENTIFY_CMDSET3_MEDIA_CARD_PASSTHRU        0x0008
#define ATA_IDENTIFY_CMDSET3_STREAMING                  0x0010
#define ATA_IDENTIFY_CMDSET3_GP_LOGGING                 0x0040
#define ATA_IDENTIFY_CMDSET3_WRITE_FUA                  0x0040
#define ATA_IDENTIFY_CMDSET3_WRITE_QUEUED_FUA           0x0080
#define ATA_IDENTIFY_CMDSET3_WWN_64BIT                  0x0100
#define ATA_IDENTIFY_CMDSET3_URG_READ_STREAM            0x0200
#define ATA_IDENTIFY_CMDSET3_URG_WRITE_STREAM           0x0400
#define ATA_IDENTIFY_CMDSET3_IDLE_WITH_UNLOAD           0x2000
#define ATA_IDENTIFY_CMDSET3_WORD_VALID_MASK            0xC000

#define ATA_IDENTIFY_ULTRADMA_SUPPORT_MASK              0x00FF
#define ATA_IDENTIFY_ULTRADMA_ACTIVE_MASK               0xFF00
#define ATA_IDENTIFY_ULTRADMA_ACTIVE_SHL                8

#define ATA_IDENTIFY_APMLEVEL_MASK                      0x00FF

#define ATA_IDENTIFY_ACOUSTIC_CURRENT_MASK              0x00FF
#define ATA_IDENTIFY_ACOUSTIC_RECOMM_MASK               0xFF00
#define ATA_IDENTIFY_ACOUSTIC_RECOMM_SHL                8

#define ATA_IDENTIFY_PHYSLOGSECSIZE_LOG_PER_PHYS_MASK   0x000F
#define ATA_IDENTIFY_PHYSLOGSECSIZE_LOGSEC_LONG_256     0x1000
#define ATA_IDENTIFY_PHYSLOGSECSIZE_MULT_LOG_PER_PHYS   0x2000

#define ATA_IDENTIFY_CMDSETEXT_WRITE_READ_VERIFY        0x0002
#define ATA_IDENTIFY_CMDSETEXT_WRITE_UNCORR_EXT         0x0004
#define ATA_IDENTIFY_CMDSETEXT_READ_WRITE_LOG_DMA_EXT   0x0008
#define ATA_IDENTIFY_CMDSETEXT_DLD_MICRO_MODE3          0x0010
#define ATA_IDENTIFY_CMDSETEXT_FREEFALL_CONTROL         0x0020
#define ATA_IDENTIFY_CMDSETEXT_SENSE_DATA_REPORT        0x0040
#define ATA_IDENTIFY_CMDSETEXT_EXT_POWER_COND           0x0080
#define ATA_IDENTIFY_CMDSETEXT_WORD_VALID_MASK          0xC000

#define ATA_IDENTIFY_BLOCKALIGN_VALUE_MASK              0x3FFF
#define ATA_IDENTIFY_BLOCKALIGN_SUPPORTED               0x4000

#define ATA_IDENTIFY_WRV_SECCOUNT_MODE_MASK             0x00FF

#define ATA_IDENTIFY_TRANSPORT_VER_MAJOR                0x0FFF
#define ATA_IDENTIFY_TRANSPORT_TYPE_MASK                0xF000
#define ATA_IDENTIFY_TRANSPORT_TYPE_SHL                 12

#define ATA_IDENTIFY_SIGCHECK_SIG_MASK                  0x00FF
#define ATA_IDENTIFY_SIGCHECK_CHKSUM_MASK               0xFF00
#define ATA_IDENTIFY_SIGCHECK_CHKSUM_SHL                8
#define ATA_IDENTIFY_SIG_CORRECT                        0xA5

#define ATA_IDENTIFY_DATA_BYTES                         512

typedef struct _ATA_IDENT_DATA ATA_IDENT_DATA;
K2_PACKED_PUSH
struct _ATA_IDENT_DATA
{
    UINT16 GenConfig;
    UINT16 NumCylinders;
    UINT16 SpecificConfiguration;
    UINT16 NumHeads;
    UINT16 Retired1[2];
    UINT16 NumSectorsPerTrack;
    UINT16 VendorUnique1[3];
    UINT8  SerialNumber[20];
    UINT16 Retired2[2];
    UINT16 Obsolete1;
    UINT8  FirmwareRevision[8];
    UINT8  ModelNumber[40];
    UINT8  MaximumBlockTransfer;
    UINT8  VendorUnique2;
    UINT16 TrustedComputing;
    UINT32 Caps;
    UINT16 ObsoleteWords51[2];
    UINT16 TransSens;
    UINT16 NumberOfCurrentCylinders;
    UINT16 NumberOfCurrentHeads;
    UINT16 CurrentSectorsPerTrack;
    UINT32 CurrentSectorCapacity;
    UINT8  CurrentMultiSectorSetting;
    UINT8  Feature1;
    UINT32 UserAddressableSectors;
    UINT16 ObsoleteWord62;
    UINT16 Feature2;
    UINT16 Feature3;
    UINT16 MinimumMWXferCycleTime;
    UINT16 RecommendedMWXferCycleTime;
    UINT16 MinimumPIOCycleTime;
    UINT16 MinimumPIOCycleTimeIORDY;
    UINT16 Feature4;
    UINT16 ReservedWords70[5];
    UINT16 QueueDepth;
    UINT16 SataCaps1;
    UINT16 SataCaps2;
    UINT16 SataFeatures1;
    UINT16 SataEnabled1;
    UINT16 MajorRevision;
    UINT16 MinorRevision;
    UINT16 CommandSetSupport1;
    UINT16 CommandSetSupport2;
    UINT16 CommandSetSupport3;
    UINT16 CommandSetActive1;
    UINT16 CommandSetActive2;
    UINT16 CommandSetActive3;
    UINT16 UltraDma;
    UINT16 SecErase1;
    UINT16 SecErase2;
    UINT16 ApmLevel;
    UINT16 MasterPasswordID;
    UINT16 HardwareResetResult;
    UINT16 Acoustic;
    UINT16 StreamMinRequestSize;
    UINT16 StreamingTransferTimeDMA;
    UINT16 StreamingAccessLatencyDMAPIO;
    UINT32 StreamingPerfGranularity;
    UINT32 Max48BitLBA[2];
    UINT16 StreamingTransferTime;
    UINT16 DsmCap;
    UINT16 PhysLogSectorSize;
    UINT16 InterSeekDelay;
    UINT16 WorldWideName[4];
    UINT16 ReservedForWorldWideName128[4];
    UINT16 ReservedForTlcTechnicalReport;
    UINT16 WordsPerLogicalSector[2];
    UINT16 CmdSetExtSupport;
    UINT16 CmdSetExtActive;
    UINT16 ReservedForExpandedSupportandActive[6];
    UINT16 MsnSupport;
    UINT16 SecurityStatus;
    UINT16 ReservedWord129[31];
    UINT16 CfaPowerModel;
    UINT16 ReservedForCfaWord161[7];
    UINT16 NominalFormFactor;
    UINT16 DataSetManagementFeature;
    UINT16 AdditionalProductID[4];
    UINT16 ReservedForCfaWord174[2];
    UINT16 CurrentMediaSerialNumber[30];
    UINT16 SCTCommandTransport;
    UINT16 ReservedWord207[2];
    UINT16 BlockAlign;
    UINT16 WriteReadVerifySectorCountMode3Only[2];
    UINT16 WriteReadVerifySectorCountMode2Only[2];
    UINT16 NvCacheCap;
    UINT16 NVCacheSizeLSW;
    UINT16 NVCacheSizeMSW;
    UINT16 NominalMediaRotationRate;
    UINT16 ReservedWord218;
    UINT8  NvCacheOption1;
    UINT8  NvCacheOption2;
    UINT16 WriteReadVerifySectorCountMode;
    UINT16 ReservedWord221;
    UINT16 Transport;
    UINT16 TransportMinorVersion;
    UINT16 ReservedWord224[6];
    UINT32 ExtendedNumberOfUserAddressableSectors[2];
    UINT16 MinBlocksPerDownloadMicrocodeMode03;
    UINT16 MaxBlocksPerDownloadMicrocodeMode03;
    UINT16 ReservedWord236[19];
    UINT16 SigCheck;
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

K2_STATIC_ASSERT(sizeof(ATA_IDENT_DATA) == ATA_IDENTIFY_DATA_BYTES);

/* ------------------------------------------------------------------------- */

typedef enum _IdeChanRegType IdeChanRegType;
enum _IdeChanRegType
{
    IdeChanReg_RW_DATA,
    IdeChanReg_R_ERROR,
    IdeChanReg_W_FEATURES,
    IdeChanReg_RW_SECCOUNT0,
    IdeChanReg_RW_LBA_LOW,
    IdeChanReg_RW_LBA_MID,
    IdeChanReg_RW_LBA_HI,
    IdeChanReg_RW_HDDEVSEL,
    IdeChanReg_R_STATUS,
    IdeChanReg_W_COMMAND,
    IdeChanReg_R_ASR,
    IdeChanReg_W_DCR,
    IdeChanRegType_Count
};

typedef struct _IDE_CHANNEL_REGS IDE_CHANNEL_REGS;
struct _IDE_CHANNEL_REGS
{
    UINT16  mBase;
    UINT16  mControl;
    BOOL    mIsPhys;
    UINT32  mRegAddr[IdeChanRegType_Count];
};

typedef enum _IdeStateType IdeStateType;
enum _IdeStateType
{
    IdeState_DeviceNotPresent = 0,

    IdeState_WaitIdent,
    IdeState_EvalIdent,
    IdeState_WaitMediaChange,
    IdeState_InService,

    IdeState_Count
};

struct _IDE_DEVICE
{
    IDE_CHANNEL *                   mpChannel;
    UINT32                          mDeviceIndex;

    UINT32                          mLocation;
    BOOL                            mIsATAPI;
    BOOL                            mIsRemovable;
    BOOL                            mMediaPresent;
    BOOL                            mMediaIsReadOnly;
    BOOL                            mHasMSN;
    IdeStateType                    mState;
    UINT32                          mWaitMs;
    UINT32                          mAtaMode;
    ATA_IDENT_DATA                  AtaIdent;
    K2OS_STORAGE_MEDIA              Media;
    K2OS_CRITSEC                    Sec;

    K2OS_SIGNAL_TOKEN               mTokTransferWaitNotify;
    K2OS_BLOCKIO_TRANSFER const *   mpTransfer;
    K2STAT                          mTransferStatus;
};

struct _IDE_CHANNEL
{
    IDE_CONTROLLER *                mpController;
    UINT32                          mChannelIndex;

    UINT32                          mThreadId;

    K2OS_CRITSEC                    Sec;

    UINT8                           mIrqMaskFlag;   // bit 1

    IDE_DEVICE                      Device[2];
    IDE_CHANNEL_REGS                ChanRegs;

    K2OSKERN_SEQLOCK                IrqSeqLock;
    K2OSDDK_RES *                   mpIrqRes;
    K2OSKERN_pf_Hook_Key            mIrqHook;

    K2OS_SIGNAL_TOKEN               mTokNotify;

    K2OSDDK_pf_BlockIo_NotifyKey *  mpNotifyKey;
};

struct _IDE_CONTROLLER
{
    K2OS_DEVCTX         mDevCtx;

    K2OSDDK_INSTINFO    InstInfo;

    K2OS_THREAD_TOKEN   mTokThread;
    UINT32              mThreadId;

    K2OS_MAILBOX_TOKEN  mTokMailbox;

    K2OSDDK_RES         ResIo[5];
    K2OSDDK_RES         ResPhys[5];
    K2OSDDK_RES         ResIrq[2];

    BOOL                mBusMasterIsPhys;
    UINT32              mBusMasterAddr;

    UINT32              mPopMask;

    IDE_CHANNEL         Channel[2];
};

K2STAT IDE_InitAndDiscover(IDE_CONTROLLER *apController);

UINT8  IDE_Read8(IDE_CHANNEL *apChannel, IdeChanRegType aReg);
void   IDE_Write8(IDE_CHANNEL *apChannel, IdeChanRegType aReg, UINT8 aValue);

UINT16 IDE_Read16(IDE_CHANNEL *apChannel, IdeChanRegType aReg);
void   IDE_Write16(IDE_CHANNEL *apChannel, IdeChanRegType aReg, UINT16 aValue);

UINT32 IDE_Read32(IDE_CHANNEL *apChannel, IdeChanRegType aReg);
void   IDE_Write32(IDE_CHANNEL *apChannel, IdeChanRegType aReg, UINT32 aValue);

UINT32 IDE_Channel_Thread(IDE_CHANNEL *apChannel);

K2STAT IDE_Device_GetMedia(IDE_DEVICE *apDevice, K2OS_STORAGE_MEDIA *apRetMedia);
K2STAT IDE_Device_Transfer(IDE_DEVICE *apDevice, K2OS_BLOCKIO_TRANSFER const *apTransfer);

/* ------------------------------------------------------------------------- */

#endif // __IDE_H