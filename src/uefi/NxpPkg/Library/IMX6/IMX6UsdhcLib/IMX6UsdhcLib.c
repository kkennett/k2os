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

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/IMX6/IMX6UsdhcLib.h>

#define USDHCLIB_Check(x,size) \
typedef int Check ## x [(sizeof(x) == size) ? 1 : -1];

// -----------------------------------------------------------------------------

typedef union _USDHC_BLK_ATT
{
    struct
    {
        UINT32  BLKSIZE     : 13;
        UINT32  rz13        : 3;
        UINT32  BLKCNT      : 16;
    };
    UINT32  Raw;
} REG_USDHC_BLK_ATT;
USDHCLIB_Check(REG_USDHC_BLK_ATT, sizeof(UINT32));

typedef union _USDHC_CMD_XFR_TYP
{
    struct
    {
        UINT32  rz0         : 16;
        UINT32  RSPTYP      : 2;
        UINT32  rz18        : 1;
        UINT32  CCCEN       : 1;
        UINT32  CICEN       : 1;
        UINT32  DPSEL       : 1;
        UINT32  CMDTYP      : 2;
        UINT32  CMDINX      : 6;
        UINT32  rz30        : 2;
    };
    UINT32  Raw;
} REG_USDHC_CMD_XFR_TYP;
USDHCLIB_Check(REG_USDHC_CMD_XFR_TYP, sizeof(UINT32));

typedef union _USDHC_PRES_STATE
{
    struct
    {
        UINT32  CIHB        : 1;
        UINT32  CDIHB       : 1;
        UINT32  DLA         : 1;
        UINT32  SDSTB       : 1;
        UINT32  IPGOFF      : 1;
        UINT32  HCKOFF      : 1;
        UINT32  PEROFF      : 1;
        UINT32  SDOFF       : 1;
        UINT32  WTA         : 1;
        UINT32  RTA         : 1;
        UINT32  BWEN        : 1;
        UINT32  BREN        : 1;
        UINT32  RTR         : 1;
        UINT32  rz13        : 3;
        UINT32  CINST       : 1;
        UINT32  rz17        : 1;
        UINT32  CDPL        : 1;
        UINT32  WPSPL       : 1;
        UINT32  rz20        : 3;
        UINT32  CLSL        : 1;
        UINT32  DLSL        : 8;
    };
    UINT32  Raw;
} REG_USDHC_PRES_STATE;
USDHCLIB_Check(REG_USDHC_PRES_STATE, sizeof(UINT32));

typedef union _USDHC_PROT_CTRL
{
    struct
    {
        UINT32  LCTL                : 1;
        UINT32  DTW                 : 2;
        UINT32  D3CD                : 1;
        UINT32  EMODE               : 2;
        UINT32  CDTL                : 1;
        UINT32  CDSS                : 1;
        UINT32  DMASEL              : 2;
        UINT32  rz10                : 6;
        UINT32  SABGREQ             : 1;
        UINT32  CREQ                : 1;
        UINT32  RWCTL               : 1;
        UINT32  IABG                : 1;
        UINT32  RD_DONE_NO_8CLK     : 1;
        UINT32  MUST_WRITE_0_21     : 2;
        UINT32  MUST_WRITE_1        : 1;
        UINT32  WECINT              : 1;
        UINT32  WECINS              : 1;
        UINT32  WECRM               : 1;
        UINT32  BURST_LEN_EN        : 3;
        UINT32  NON_EXACT_BLK_RD    : 1;
        UINT32  MUST_WRITE_0_31     : 1;
    };
    UINT32  Raw;
} REG_USDHC_PROT_CTRL;
USDHCLIB_Check(REG_USDHC_PROT_CTRL, sizeof(UINT32));

typedef union _USDHC_SYS_CTRL
{
    struct
    {
        UINT32  MUST_WRITE_F    : 4;
        UINT32  DVS             : 4;
        UINT32  SDCLKFS         : 8;
        UINT32  DTOCV           : 4;
        UINT32  rz20            : 2;
        UINT32  MUST_WRITE_0    : 1;
        UINT32  IPP_RST_N       : 1;
        UINT32  RSTA            : 1;
        UINT32  RSTC            : 1;
        UINT32  RSTD            : 1;
        UINT32  INITA           : 1;
        UINT32  rz29            : 4;
    };
    UINT32  Raw;
} REG_USDHC_SYS_CTRL;
USDHCLIB_Check(REG_USDHC_SYS_CTRL, sizeof(UINT32));

typedef union _USDHC_INT_STATUS
{
    struct
    {
        UINT32  CC      : 1;
        UINT32  TC      : 1;
        UINT32  BGE     : 1;
        UINT32  DINT    : 1;
        UINT32  BWR     : 1;
        UINT32  BRR     : 1;
        UINT32  CINS    : 1;
        UINT32  CRM     : 1;
        UINT32  CINT    : 1;
        UINT32  rz9     : 3;
        UINT32  RTE     : 1;
        UINT32  rz13    : 1;
        UINT32  TP      : 1;
        UINT32  rz15    : 1;
        UINT32  CTOE    : 1;
        UINT32  CCE     : 1;
        UINT32  CEBE    : 1;
        UINT32  CIE     : 1;
        UINT32  DTOE    : 1;
        UINT32  DCE     : 1;
        UINT32  DEBE    : 1;
        UINT32  rz23    : 1;
        UINT32  AC12E   : 1;
        UINT32  rz25    : 1;
        UINT32  TNE     : 1;
        UINT32  rz27    : 1;
        UINT32  DMAE    : 1;
        UINT32  rz29    : 3;
    };
    UINT32  Raw;
} REG_USDHC_INT_STATUS;
USDHCLIB_Check(REG_USDHC_INT_STATUS, sizeof(UINT32));

typedef union _USDHC_INT_STATUS_EN
{
    struct
    {
        UINT32  CCSEN   : 1;
        UINT32  TCSEN   : 1;
        UINT32  BGESEN  : 1;
        UINT32  DINTSEN : 1;
        UINT32  BWRSEN  : 1;
        UINT32  BRRSEN  : 1;
        UINT32  CINSSEN : 1;
        UINT32  CRMSEN  : 1;
        UINT32  CINTSEN : 1;
        UINT32  rz9     : 3;
        UINT32  RTESEN  : 1;
        UINT32  rz13    : 1;
        UINT32  TPSEN   : 1;
        UINT32  rz15    : 1;
        UINT32  CTOESEN : 1;
        UINT32  CCESEN  : 1;
        UINT32  CEBESEN : 1;
        UINT32  CIESEN  : 1;
        UINT32  DTOESEN : 1;
        UINT32  DCESEN  : 1;
        UINT32  DEBESEN : 1;
        UINT32  rz23    : 1;
        UINT32  AC12ESEN: 1;
        UINT32  rz25    : 1;
        UINT32  TNESEN  : 1;
        UINT32  rz27    : 1;
        UINT32  DMAESEN : 1;
        UINT32  rz29    : 3;
    };
    UINT32  Raw;
} REG_USDHC_INT_STATUS_EN;
USDHCLIB_Check(REG_USDHC_INT_STATUS_EN, sizeof(UINT32));

typedef union _USDHC_INT_SIGNAL_EN
{
    struct
    {
        UINT32  CCIEN   : 1;
        UINT32  TCIEN   : 1;
        UINT32  BGEIEN  : 1;
        UINT32  DINTIEN : 1;
        UINT32  BWRIEN  : 1;
        UINT32  BRRIEN  : 1;
        UINT32  CINSIEN : 1;
        UINT32  CRMIEN  : 1;
        UINT32  CINTIEN : 1;
        UINT32  rz9     : 3;
        UINT32  RTEIEN  : 1;
        UINT32  rz13    : 1;
        UINT32  TPIEN   : 1;
        UINT32  rz15    : 1;
        UINT32  CTOEIEN : 1;
        UINT32  CCEIEN  : 1;
        UINT32  CEBEIEN : 1;
        UINT32  CIEIEN  : 1;
        UINT32  DTOEIEN : 1;
        UINT32  DCEIEN  : 1;
        UINT32  DEBEIEN : 1;
        UINT32  rz23    : 1;
        UINT32  AC12EIEN: 1;
        UINT32  rz25    : 1;
        UINT32  TNEIEN  : 1;
        UINT32  rz27    : 1;
        UINT32  DMAEIEN : 1;
        UINT32  rz29    : 3;
    };
    UINT32  Raw;
} REG_USDHC_INT_SIGNAL_EN;
USDHCLIB_Check(REG_USDHC_INT_SIGNAL_EN, sizeof(UINT32));

typedef union _USDHC_AUTOCMD12_ERR_STATUS
{
    struct
    {
        UINT32  AC12NE          : 1;
        UINT32  AC12TOE         : 1;
        UINT32  AC12EBE         : 1;
        UINT32  AC12CE          : 1;
        UINT32  AC12IE          : 1;
        UINT32  rz5             : 2;
        UINT32  CNIBAC12E       : 1;
        UINT32  rz8             : 24;
    };
    UINT32  Raw;
} REG_USDHC_AUTOCMD12_ERR_STATUS;
USDHCLIB_Check(REG_USDHC_AUTOCMD12_ERR_STATUS, sizeof(UINT32));

typedef union _USDHC_HOST_CTRL_CAP
{
    struct
    {
        UINT32  rz0                 : 16;
        UINT32  MBL                 : 3;
        UINT32  rz19                : 1;
        UINT32  ADMAS               : 1;
        UINT32  HSS                 : 1;
        UINT32  DMAS                : 1;
        UINT32  SRS                 : 1;
        UINT32  VS33                : 1;
        UINT32  VS30                : 1;
        UINT32  VS18                : 1;
        UINT32  rz27                : 5;
    };
    UINT32  Raw;
} REG_USDHC_HOST_CTRL_CAP;
USDHCLIB_Check(REG_USDHC_HOST_CTRL_CAP, sizeof(UINT32));

typedef union _USDHC_WTMK_LVL
{
    struct
    {
        UINT32  RD_WML          : 8;
        UINT32  RD_BRST_LEN     : 5;
        UINT32  rz13            : 3;
        UINT32  WR_WML          : 8;
        UINT32  WR_BRST_LEN     : 5;
        UINT32  rz29            : 3;
    };
    UINT32  Raw;
} REG_USDHC_WTMK_LVL;
USDHCLIB_Check(REG_USDHC_WTMK_LVL, sizeof(UINT32));

typedef union _USDHC_MIXER_CTRL
{
    struct
    {
        UINT32  DMAEN           : 1;
        UINT32  BCEN            : 1;
        UINT32  AC12EN          : 1;
        UINT32  DDR_EN          : 1;
        UINT32  DTDSEL          : 1;
        UINT32  MSBSEL          : 1;
        UINT32  NIBBLE_POS      : 1;
        UINT32  AC23EN          : 1;
        UINT32  rz8             : 14;
        UINT32  EXE_TUNE        : 1;
        UINT32  SMP_CLK_SEL     : 1;
        UINT32  AUTO_TUNE_EN    : 1;
        UINT32  FBCLK_SEL       : 1;
        UINT32  rz26            : 3;
        UINT32  MUST_WRITE_0    : 2;
        UINT32  MUST_WRITE_1    : 1;
    };
    UINT32  Raw;
} REG_USDHC_MIXER_CTRL;
USDHCLIB_Check(REG_USDHC_MIXER_CTRL, sizeof(UINT32));

typedef union _USDHC_FORCE_EVENT
{
    struct
    {
        UINT32  FEVTAC12NE      : 1;
        UINT32  FEVTAC12TOE     : 1;
        UINT32  FEVTAC12CE      : 1;
        UINT32  FEVTAC12EBE     : 1;
        UINT32  FEVTAC12IE      : 1;
        UINT32  rz5             : 2;
        UINT32  FEVTNIBAC12E    : 1;
        UINT32  rz8             : 8;
        UINT32  FEVTCTOE        : 1;
        UINT32  FEVTCCE         : 1;
        UINT32  FEVTCEBE        : 1;
        UINT32  FEVTCIE         : 1;
        UINT32  FEVTDTOE        : 1;
        UINT32  FEVTDCE         : 1;
        UINT32  FEVTDEBE        : 1;
        UINT32  rz23            : 1;
        UINT32  FEVTAC12E       : 1;
        UINT32  rz25            : 1;
        UINT32  FEVTTNE         : 1;
        UINT32  rz27            : 1;
        UINT32  FEVTDMAE        : 1;
        UINT32  rz29            : 2;
        UINT32  FEVTCINT        : 1;
    };
    UINT32  Raw;
} REG_USDHC_FORCE_EVENT;
USDHCLIB_Check(REG_USDHC_FORCE_EVENT, sizeof(UINT32));

typedef union _USDHC_ADMA_ERR_STATUS
{
    struct
    {
        UINT32  ADMAES  : 2;
        UINT32  ADMALME : 1;
        UINT32  ADMADCE : 1;
        UINT32  rz4     : 28;
    };
    UINT32  Raw;
} REG_USDHC_ADMA_ERR_STATUS;
USDHCLIB_Check(REG_USDHC_ADMA_ERR_STATUS, sizeof(UINT32));

typedef union _USDHC_DLL_CTRL
{
    struct
    {
        UINT32  DLL_CTRL_ENABLE             : 1;
        UINT32  DLL_CTRL_RESET              : 1;
        UINT32  DLL_CTRL_SLV_FORCE_UPD      : 1;
        UINT32  DLL_CTRL_SLV_DLY_TARGET0    : 4;
        UINT32  DLL_CTRL_GATE_UPDATE        : 1;
        UINT32  DLL_CTRL_SLV_OVERRIDE       : 1;
        UINT32  DLL_CTRL_SLV_OVERRIDE_VAL   : 7;
        UINT32  DLL_CTRL_SLV_DLY_TARGET1    : 3;
        UINT32  rz19                        : 1;
        UINT32  DLL_CTRL_SLV_UPDATE_INT     : 8;
        UINT32  DLL_CTRL_REF_UPDATE_INT     : 4;
    };
    UINT32  Raw;
} REG_USDHC_DLL_CTRL;
USDHCLIB_Check(REG_USDHC_DLL_CTRL, sizeof(UINT32));

typedef union _USDHC_DLL_STATUS
{
    struct
    {
        UINT32  DLL_STS_SLV_LOCK    : 1;
        UINT32  DLL_STS_REF_LOCK    : 1;
        UINT32  DLL_STS_SLV_SEL     : 7;
        UINT32  DLL_STS_REF_SEL     : 7;
        UINT32  rz16                : 16;
    };
    UINT32  Raw;
} REG_USDHC_DLL_STATUS;
USDHCLIB_Check(REG_USDHC_DLL_STATUS, sizeof(UINT32));

typedef union _USDHC_CLK_TUNE_CTRL_STATUS
{
    struct
    {
        UINT32  DLY_CELL_SET_POST   : 4;
        UINT32  DLY_CELL_SET_OUT    : 4;
        UINT32  DLY_CELL_SET_PRE    : 7;
        UINT32  NXT_ERR             : 1;
        UINT32  TAP_SEL_POST        : 4;
        UINT32  TAP_SEL_OUT         : 4;
        UINT32  TAP_SEL_PRE         : 7;
        UINT32  PRE_ERR             : 1;
    };
    UINT32  Raw;
} REG_USDHC_CLK_TUNE_CTRL_STATUS;
USDHCLIB_Check(REG_USDHC_CLK_TUNE_CTRL_STATUS, sizeof(UINT32));

typedef union _USDHC_VEND_SPEC
{
    struct
    {
        UINT32  EXT_DMA_EN              : 1;
        UINT32  VSELECT                 : 1;
        UINT32  CONFLICT_CHK_EN         : 1;
        UINT32  AC12_WR_CHKBUSY_EN      : 1;
        UINT32  DATA3_CD_POL            : 1;
        UINT32  CD_POL                  : 1;
        UINT32  WP_POL                  : 1;
        UINT32  CLKONJ_IN_ABORT         : 1;
        UINT32  FRC_SDCLK_ON            : 1;
        UINT32  MUST_WRITE_0_9          : 2;
        UINT32  IPG_CLK_SOFT_EN         : 1;
        UINT32  HCLK_SOFT_EN            : 1;
        UINT32  IPG_PERCLK_SOFT_EN      : 1;
        UINT32  CARD_CLK_SOFT_EN        : 1;
        UINT32  CRC_CHK_DIS             : 1;
        UINT32  INT_ST_VAL              : 8;
        UINT32  MUST_WRITE_0_24         : 5;
        UINT32  MUST_WRITE_1            : 1;
        UINT32  MUST_WRITE_0_30         : 1;
        UINT32  CMD_BYTE_EN             : 1;
    };
    UINT32  Raw;
} REG_USDHC_VEND_SPEC;
USDHCLIB_Check(REG_USDHC_VEND_SPEC, sizeof(UINT32));

typedef union _USDHC_MMC_BOOT
{
    struct
    {
        UINT32  DTOCV_ACK           : 4;
        UINT32  BOOT_ACK            : 1;
        UINT32  BOOT_MODE           : 1;
        UINT32  BOOT_EN             : 1;
        UINT32  AUTO_SABG_EN        : 1;
        UINT32  DISABLE_TIME_OUT    : 1;
        UINT32  rz9                 : 7;
        UINT32  BOOT_BLK_CNT        : 16;
    };
    UINT32  Raw;
} REG_USDHC_MMC_BOOT;
USDHCLIB_Check(REG_USDHC_MMC_BOOT, sizeof(UINT32));

typedef union _USDHC_VEND_SPEC2
{
    struct
    {
        UINT32  SDR104_TIMING_DIS       : 1;
        UINT32  SDR104_OE_DIS           : 1;
        UINT32  SDR104_NSD_DIS          : 1;
        UINT32  CARD_INT_D3_TEST        : 1;
        UINT32  TUNING_8BIT_EN          : 1;
        UINT32  TUNING_16BIT_EN         : 1;
        UINT32  TUNING_CMD_EN           : 1;
        UINT32  CARD_INT_AUTO_CLR_DIS   : 1;
        UINT32  rz8                     : 24;
    };
    UINT32  Raw;
} REG_USDHC_VEND_SPEC2;
USDHCLIB_Check(REG_USDHC_VEND_SPEC2, sizeof(UINT32));

// -----------------------------------------------------------------------------

UINT32
EFIAPI
IMX6_USDHC_GetRate(
    UINT32 Regs_CCM,
    UINT32 UnitNum,
    UINT32 Regs_USDHC
)
{
    REG_USDHC_MIXER_CTRL    MIXER_CTRL;
    REG_USDHC_SYS_CTRL      SYS_CTRL;
    REG_USDHC_PRES_STATE    PRES_STATE;
    UINT32                  Divisor;

    ASSERT(Regs_CCM != 0);
    ASSERT((UnitNum > 0) && (UnitNum < 5));
    ASSERT(Regs_USDHC != 0);

    do
    {
        PRES_STATE.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
    } while (!PRES_STATE.SDSTB);

    // get initial divider which will be a power of 2
    SYS_CTRL.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL);
    Divisor = SYS_CTRL.SDCLKFS;
    Divisor <<= 1;
    ASSERT((Divisor & (Divisor - 1)) == 0);

    // if operating DDR then divisor is doubled
    MIXER_CTRL.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_MIX_CTRL);
    if (MIXER_CTRL.DDR_EN)
        Divisor <<= 1;

    // secondary divisor
    Divisor *= (SYS_CTRL.DVS + 1);
    ASSERT(Divisor != 0);

    // rate is root clock divided by divisor
    return IMX6_CLOCK_GetRate_USDHC_SRC(Regs_CCM, UnitNum) / Divisor;
}

static
UINT32
sHighestBitPos(
    UINT32 v
    )
{
    UINT32 ret;

    ASSERT(v != 0);

    ret = 0;

    if (v & 0xFFFF0000)
    {
        ret += 16;
        v >>= 16;
    }
    if (v & 0xFF00)
    {
        ret += 8;
        v >>= 8;
    }
    if (v & 0xF0)
    {
        ret += 4;
        v >>= 4;
    }
    if (v & 0xC)
    {
        ret += 2;
        v >>= 2;
    }
    if (v & 2)
        ret++;

    return ret;
}

BOOLEAN
EFIAPI
IMX6_USDHC_CalcClockDivisors(
    UINT32      Regs_CCM,
    UINT32      UnitNum,
    UINT32      Regs_USDHC,
    UINT32      TargetFreq,
    UINT32 *    SDCLKFS,
    UINT32 *    DVS
    )
{
    UINT32                  divVal;
    UINT32                  bitPos;
    UINT32                  lowPos;
    REG_USDHC_MIXER_CTRL    MIXER_CTRL;

    ASSERT(Regs_CCM != 0);
    ASSERT((UnitNum > 0) && (UnitNum < 5));
    ASSERT(Regs_USDHC != 0);
    ASSERT(TargetFreq != 0);
    ASSERT(SDCLKFS != NULL);
    ASSERT(DVS != NULL);

    MIXER_CTRL.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_MIX_CTRL);

    lowPos = IMX6_CLOCK_GetRate_USDHC_SRC(Regs_CCM, UnitNum);

    divVal = (lowPos + (TargetFreq - 1)) / TargetFreq;
    ASSERT(divVal != 0);

    // get highest 4 bits
    bitPos = sHighestBitPos(divVal);
    if (bitPos > 3)
    {
        lowPos = bitPos - 3;
        if (divVal & ((1 << lowPos) - 1))
        {
            divVal &= ~((1 << lowPos) - 1);
            divVal += (1 << lowPos);
            bitPos = sHighestBitPos(divVal);
        }
    }

    lowPos = 0;
    while (!(divVal & 1))
    {
        divVal >>= 1;
        lowPos++;
    }

    bitPos = 8;
    if (MIXER_CTRL.DDR_EN)
        bitPos--;
    while (lowPos > bitPos)
    {
        divVal <<= 1;
        lowPos--;
    }

    if (divVal > 15)
        return FALSE;

    *SDCLKFS = (1 << (lowPos-1));
    *DVS = (divVal - 1);

    return TRUE;
}

void
EFIAPI
IMX6_USDHC_ChangeClockDivisors(
    UINT32 Regs_USDHC,
    UINT32 SDCLKFS,
    UINT32 DVS,
    UINT32 Regs_GPT,
    UINT32 GPTRate
    )
{
    REG_USDHC_PRES_STATE    PRES_STATE;
    REG_USDHC_VEND_SPEC     VEND_SPEC;
    REG_USDHC_SYS_CTRL      SYS_CTRL;

    ASSERT(Regs_USDHC != 0);
    ASSERT((SDCLKFS & ~0xFF) == 0);
    ASSERT((DVS & ~0x0F) == 0);
    ASSERT(Regs_GPT != 0);
    ASSERT(GPTRate != 0);

    // check for no-op
    SYS_CTRL.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL);
    if ((SYS_CTRL.SDCLKFS == SDCLKFS) &&
        (SYS_CTRL.DVS == DVS))
        return;

    // gate off the clock.
    // verify with PRES_STATE.SDOFF == 1
    VEND_SPEC.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC);
    VEND_SPEC.FRC_SDCLK_ON = 0;
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC, VEND_SPEC.Raw);
    do
    {
        PRES_STATE.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
    } while ((PRES_STATE.SDOFF == 0) || (PRES_STATE.SDSTB == 0));

    SYS_CTRL.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL);
    SYS_CTRL.SDCLKFS = SDCLKFS;
    SYS_CTRL.DVS = DVS;
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL, SYS_CTRL.Raw);

    // un-gate the clock.
    VEND_SPEC.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC);
    VEND_SPEC.FRC_SDCLK_ON = 1;
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC, VEND_SPEC.Raw);
    do
    {
        PRES_STATE.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
    } while ((PRES_STATE.SDOFF == 1) || (PRES_STATE.SDSTB == 0));

    // let card stabilize on new clock for 2ms before we send it a command
    IMX6_GPT_DelayUs(Regs_GPT, GPTRate, 2000);
}

BOOLEAN
EFIAPI
IMX6_USDHC_ResetController(
    UINT32 Regs_CCM,
    UINT32 UnitNum,
    UINT32 Regs_USDHC,
    UINT32 Regs_GPT,
    UINT32 GPTRate
    )
{
    REG_USDHC_SYS_CTRL          SYS_CTRL;
    REG_USDHC_PRES_STATE        PRES_STATE;
    REG_USDHC_VEND_SPEC         VEND_SPEC;
    UINT32                      Cur;
    UINT32                      End;

    ASSERT(Regs_CCM != 0);
    ASSERT((UnitNum > 0) && (UnitNum < 5));
    ASSERT(Regs_USDHC != 0);
    ASSERT(Regs_GPT != 0);
    ASSERT(GPTRate != 0);

    // gate off the clock.
    // verify with PRES_STATE.SDOFF == 1
    VEND_SPEC.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC);
    VEND_SPEC.FRC_SDCLK_ON = 0;
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC, VEND_SPEC.Raw);
    do
    {
        PRES_STATE.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
    } while ((PRES_STATE.SDOFF == 0) || (PRES_STATE.SDSTB == 0));

    // write initial values into all writeable registers
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_DS_ADDR             , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_BLK_ATT             , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_CMD_ARG             , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_CMD_XFR_TYP         , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT  , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_PROT_CTRL           , 0x08800020);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL            , 0x8080800F);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS_EN       , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_INT_SIGNAL_EN       , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_WTMK_LVL            , 0x08100810);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_MIX_CTRL            , 0x80000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_ADMA_SYS_ADDR       , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_DLL_CTRL            , 0x00000200);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS, 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC           , 0x20007809);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_MMC_BOOT            , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC2          , 0x00000000);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_TUNING_CTRL         , 0x00212800);

    // clear interrupt status
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS, IMX6_USDHC_INTREG_MASK);

    // reset the controller and wait for it to finish
    SYS_CTRL.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL);
    SYS_CTRL.RSTA = 1;
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL, SYS_CTRL.Raw);

    Cur = MmioRead32(Regs_GPT + IMX6_GPT_OFFSET_CNT);
    End = Cur + IMX6_GPT_TicksFromUs(GPTRate, 10 * 1000);
    if (End < Cur)
    {
        do
        {
            SYS_CTRL.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL);
            if (!SYS_CTRL.RSTA)
                break;
        } while (MmioRead32(Regs_GPT + IMX6_GPT_OFFSET_CNT) >= Cur);
    }
    if (SYS_CTRL.RSTA)
    {
        do
        {
            SYS_CTRL.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL);
            if (!SYS_CTRL.RSTA)
                break;
        } while (MmioRead32(Regs_GPT + IMX6_GPT_OFFSET_CNT) < End);
        if (SYS_CTRL.RSTA)
            return FALSE;
    }

    // do not generate interrupts but generate status for all events
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_INT_SIGNAL_EN, 0);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS_EN, IMX6_USDHC_INTREG_MASK);
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS, IMX6_USDHC_INTREG_MASK);

    // set to standard preinit clock rate. MIXER_CTRL MUST have been written before we do this
    IMX6_USDHC_CalcClockDivisors(Regs_CCM, UnitNum, Regs_USDHC, 400000, &Cur, &End);
    SYS_CTRL.Raw = 0;
    SYS_CTRL.MUST_WRITE_F = 0xF;
    SYS_CTRL.MUST_WRITE_0 = 0;
    SYS_CTRL.IPP_RST_N = 1;
    SYS_CTRL.SDCLKFS = Cur;
    SYS_CTRL.DVS = End;
    SYS_CTRL.DTOCV = 7;
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL, SYS_CTRL.Raw);

    // un-gate the clock.
    VEND_SPEC.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC);
    VEND_SPEC.FRC_SDCLK_ON = 1;
    MmioWrite32(Regs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC, VEND_SPEC.Raw);
    do
    {
        PRES_STATE.Raw = MmioRead32(Regs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
    } while ((PRES_STATE.SDOFF == 1) || (PRES_STATE.SDSTB == 0));

    // let card stabilize on new clock for 5ms before we send it a command
    IMX6_GPT_DelayUs(Regs_GPT, GPTRate, 5000);

    return TRUE;
}

