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

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IMX6/IMX6UsdhcLib.h>

#define SDMMC_STATUS_AKE_SEQ_ERROR              0x00000008
#define SDMMC_STATUS_APP_CMD                    0x00000020
#define SDMMC_STATUS_FX_EVENT                   0x00000040
#define SDMMC_STATUS_READY_FOR_DATA             0x00000100
#define SDMMC_STATUS_CURRENT_STATE_MASK         0x00001E00
#define SDMMC_STATUS_CURRENT_STATE_SHL          9
#define SDMMC_STATUS_ERASE_RESET                0x00002000
#define SDMMC_STATUS_CARD_ECC_DISABLED          0x00004000
#define SDMMC_STATUS_WP_ERASE_SKIP              0x00008000
#define SDMMC_STATUS_CSD_OVERWRITE              0x00010000
#define SDMMC_STATUS_ERROR                      0x00080000
#define SDMMC_STATUS_CC_ERROR                   0x00100000
#define SDMMC_STATUS_CARD_ECC_FAILED            0x00200000
#define SDMMC_STATUS_ILLEGAL_COMMAND            0x00400000
#define SDMMC_STATUS_COM_CRC_ERROR              0x00800000
#define SDMMC_STATUS_LOCK_UNLOCK_FAILED         0x01000000
#define SDMMC_STATUS_CARD_IS_LOCKED             0x02000000
#define SDMMC_STATUS_WP_VIOLATION               0x04000000
#define SDMMC_STATUS_ERASE_PARAM                0x08000000
#define SDMMC_STATUS_ERASE_SEQ_ERROR            0x10000000
#define SDMMC_STATUS_BLOCK_LEN_ERROR            0x20000000
#define SDMMC_STATUS_ADDRESS_ERROR              0x40000000
#define SDMMC_STATUS_OUT_OF_RANGE               0x80000000

#define SDMMC_STATUS_STATE_IDLE     0
#define SDMMC_STATUS_STATE_READY    1
#define SDMMC_STATUS_STATE_IDENT    2
#define SDMMC_STATUS_STATE_STBY     3
#define SDMMC_STATUS_STATE_TRAN     4
#define SDMMC_STATUS_STATE_DATA     5
#define SDMMC_STATUS_STATE_RCV      6
#define SDMMC_STATUS_STATE_PRG      7
#define SDMMC_STATUS_STATE_DIS      8

#define SDMMC_OCR_MASK_3V3                      0x00FF8000
#define SDMMC_OCR_HIGH_CAP_SUPPORT              0x40000000
#define SDMMC_OCR_POWERED_STATUS                0x80000000

static
UINT32 sRead32(IMX6_USDHC *apUsdhc, UINT32 aOffset)
{
    return MmioRead32(apUsdhc->mRegs_USDHC + aOffset);
}

static 
void sWrite32(IMX6_USDHC *apUsdhc, UINT32 aOffset, UINT32 aValue)
{
    MmioWrite32(apUsdhc->mRegs_USDHC + aOffset, aValue);
}

static
void sAnd32(IMX6_USDHC *apUsdhc, UINT32 aOffset, UINT32 aValue)
{
    UINT32 reg;
    reg = sRead32(apUsdhc, aOffset);
    reg &= aValue;
    sWrite32(apUsdhc, aOffset, reg);
}

static
void sOr32(IMX6_USDHC *apUsdhc, UINT32 aOffset, UINT32 aValue)
{
    UINT32 reg;
    reg = sRead32(apUsdhc, aOffset);
    reg |= aValue;
    sWrite32(apUsdhc, aOffset, reg);
}

#if 0
static
void
sDumpRegs(
    IMX6_USDHC *    apUSDHC
)
{
    DebugPrint(0xFFFFFFFF, "\nDUMP\n----\n");
    DebugPrint(0xFFFFFFFF, "DS_ADDR              %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_DS_ADDR));
    DebugPrint(0xFFFFFFFF, "BLK_ATT              %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_BLK_ATT));
    DebugPrint(0xFFFFFFFF, "CMD_ARG              %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG));
    DebugPrint(0xFFFFFFFF, "CMD_XFR_TYP          %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP));
    DebugPrint(0xFFFFFFFF, "CMD_RSP0             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0));
    DebugPrint(0xFFFFFFFF, "CMD_RSP1             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP1));
    DebugPrint(0xFFFFFFFF, "CMD_RSP2             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP2));
    DebugPrint(0xFFFFFFFF, "CMD_RSP3             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP3));
//    DebugPrint(0xFFFFFFFF, "DATA_BUFF_ACC_PORT   %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT));
    DebugPrint(0xFFFFFFFF, "PRES_STATE           %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE));
    DebugPrint(0xFFFFFFFF, "PROT_CTRL            %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_PROT_CTRL));
    DebugPrint(0xFFFFFFFF, "SYS_CTRL             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL));
    DebugPrint(0xFFFFFFFF, "INT_STATUS           %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS));
    DebugPrint(0xFFFFFFFF, "INT_STATUS_EN        %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS_EN));
    DebugPrint(0xFFFFFFFF, "INT_SIGNAL_EN        %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_SIGNAL_EN));
    DebugPrint(0xFFFFFFFF, "AUTOCMD12_ERR_STATUS %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_AUTOCMD12_ERR_STATUS));
    DebugPrint(0xFFFFFFFF, "HOST_CTRL_CAP        %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_HOST_CTRL_CAP));
    DebugPrint(0xFFFFFFFF, "WTMK_LVL             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_WTMK_LVL));
    DebugPrint(0xFFFFFFFF, "MIX_CTRL             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_MIX_CTRL));
    DebugPrint(0xFFFFFFFF, "FORCE_EVENT          %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_FORCE_EVENT));
    DebugPrint(0xFFFFFFFF, "ADMA_ERR_STATUS      %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_ADMA_ERR_STATUS));
    DebugPrint(0xFFFFFFFF, "ADMA_SYS_ADDR        %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_ADMA_SYS_ADDR));
    DebugPrint(0xFFFFFFFF, "DLL_CTRL             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_DLL_CTRL));
    DebugPrint(0xFFFFFFFF, "DLL_STATUS           %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_DLL_STATUS));
    DebugPrint(0xFFFFFFFF, "CLK_TUNE_CTRL_STATUS %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS));
    DebugPrint(0xFFFFFFFF, "VEND_SPEC            %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC));
    DebugPrint(0xFFFFFFFF, "MMC_BOOT             %08X\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_MMC_BOOT));
    DebugPrint(0xFFFFFFFF, "VEND_SPEC2           %08X\n\n", sRead32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC2));
}
#endif

static
void
sClearIntStatus(
    IMX6_USDHC * apUSDHC
)
{
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS, sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS));
}

void
sAllClocks(
    IMX6_USDHC *    apUSDHC,
    BOOLEAN         aOn,
    BOOLEAN         aInTransferMode
)
{
    UINT32 regVal;

    if (!aOn)
    {
        // turning clock off
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC);
        if (regVal & IMX6_USDHC_VEND_SPEC_CARD_CLK_SOFT_EN)
        {
            do
            {
                regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
            } while (0 == (regVal & IMX6_USDHC_PRES_STATE_SDSTB));
        }
    }

    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC);

    if (aOn)
    {
        // turning clock on
        regVal |= IMX6_USDHC_VEND_SPEC_CARD_CLK_SOFT_EN;
        if (!aInTransferMode)
            regVal |= IMX6_USDHC_VEND_SPEC_FRC_SDCLK_ON;
    }
    else
    {
        // turning clock off
        regVal &= ~IMX6_USDHC_VEND_SPEC_CARD_CLK_SOFT_EN;
        if (!aInTransferMode)
            regVal &= ~IMX6_USDHC_VEND_SPEC_FRC_SDCLK_ON;
    }

    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC, 0x00800000 | regVal);

    if (aOn)
    {
        // turning clock on
        do
        {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_SDOFF));
        while (0 == (regVal & IMX6_USDHC_PRES_STATE_SDSTB))
        {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        }
    }
    else
    {
        // turning clock off
        do
        {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 == (regVal & IMX6_USDHC_PRES_STATE_SDOFF));
    }
}

void
DoRSTA(
    IMX6_USDHC *    apUSDHC,
    BOOLEAN         aInTransferMode
)
{
    UINT32 regVal;

//    DebugPrint(0xFFFFFFFF, "+RSTA\n");

    sAllClocks(apUSDHC, FALSE, aInTransferMode);
   
    sOr32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL, IMX6_USDHC_SYS_CTRL_RSTA | 0x0000000F);
    do
    {
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL);
    } while (0 != (regVal & IMX6_USDHC_SYS_CTRL_RSTA));

    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS_EN, 
        IMX6_USDHC_INT_MASK & ~(IMX6_USDHC_INT_CINS |
            IMX6_USDHC_INT_CRM |
            IMX6_USDHC_INT_BRR |
            IMX6_USDHC_INT_BWR));
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_BLK_ATT, SDMMC_BLOCK_SIZE);
//    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_MMC_BOOT, 0);
//    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_MIX_CTRL, 0x80000000);
//    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS, 0);
//    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC, 0x20007809);
//    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_DLL_CTRL, 0);
//    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_TUNING_CTRL, 0);
    sClearIntStatus(apUSDHC);

    sAllClocks(apUSDHC, TRUE, aInTransferMode);

//    DebugPrint(0xFFFFFFFF, "-RSTA\n");
}

BOOLEAN
EFIAPI
IMX6_USDHC_Initialize(
    IMX6_USDHC *    apUSDHC
)
{
    UINT32  regVal;
    UINT32  regVal2;
    int     retries;
    UINT32  ocr;
    UINT32  response[4];

    if (NULL == apUSDHC) return FALSE;
    if (NULL == apUSDHC->mpGpt) return FALSE;
    if (0 == apUSDHC->mpGpt->mRegs_GPT) return FALSE;
    if (0 == apUSDHC->mpGpt->mRate) return FALSE;
    if (0 == apUSDHC->mRegs_CCM) return FALSE;
    if (0 == apUSDHC->mRegs_USDHC) return FALSE;
    if (0 == apUSDHC->mUnitNum) return FALSE;

    //
    // ------------------------------------------------------
    //

    // enable the clock to the module
    regVal = MmioRead32(apUSDHC->mRegs_CCM + IMX6_CCM_OFFSET_CCGR6);
    ocr = IMX6_SHL_CCM_CCGR6_USDHC1;
    ocr += 2 * (apUSDHC->mUnitNum - 1);
    regVal &= ~((IMX6_RUN_AND_WAIT << ocr) | (IMX6_RUN_AND_WAIT << ocr));
    regVal |= ((IMX6_RUN_AND_WAIT << ocr) | (IMX6_RUN_AND_WAIT << ocr));
    MmioWrite32(apUSDHC->mRegs_CCM + IMX6_CCM_OFFSET_CCGR6, regVal);

    //
    // ------------------------------------------------------
    //

    // gate off clock
    sAllClocks(apUSDHC, FALSE, FALSE);

    // set module to get the base clock from PLL2 PDF2 (396Mhz)
    regVal = MmioRead32(apUSDHC->mRegs_CCM + IMX6_CCM_OFFSET_CSCMR1);
    if (0 != (regVal & 0x000F0000))
    {
        regVal &= ~0x000F0000;
        MmioWrite32(apUSDHC->mRegs_CCM + IMX6_CCM_OFFSET_CSCMR1, regVal);
    }

    // set default clock divisor in CSCDR1 to 2
    regVal = MmioRead32(apUSDHC->mRegs_CCM + IMX6_CCM_OFFSET_CSCDR1);
    switch (apUSDHC->mUnitNum)
    {
    case 1:
        regVal = (regVal & ~0x00003800) | 0x00000800;
        break;
    case 2:
        regVal = (regVal & ~0x00070000) | 0x00010000;
        break;
    case 3:
        regVal = (regVal & ~0x00380000) | 0x00080000;
        break;
    case 4:
        regVal = (regVal & ~0x01C00000) | 0x00400000;
        break;
    default:
        return FALSE;
    }
    regVal = MmioWrite32(apUSDHC->mRegs_CCM + IMX6_CCM_OFFSET_CSCDR1, regVal);

    //
    // reset all but don't turn the clocks on yet
    //
    sOr32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL, IMX6_USDHC_SYS_CTRL_RSTA | 0x0000000F);
    do
    {
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL);
    } while (0 != (regVal & IMX6_USDHC_SYS_CTRL_RSTA));

    // clear registers to some defaults
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS_EN, IMX6_USDHC_INT_MASK);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_BLK_ATT, SDMMC_BLOCK_SIZE);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_MMC_BOOT, 0);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_MIX_CTRL, 0x80000000);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS, 0);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC, 0x20007809);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_DLL_CTRL, 0);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_TUNING_CTRL, 0);
    sClearIntStatus(apUSDHC);

    // configure the SDCLKFS and DVS for close to 400Khz
    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL);
    regVal &= ~(IMX6_USDHC_SYS_CTRL_SDCLKFS_MASK | IMX6_USDHC_SYS_CTRL_DVS_MASK | IMX6_USDHC_SYS_CTRL_DTOCV_MASK);
    regVal |= ((0x40 << IMX6_USDHC_SYS_CTRL_SDCLKFS_SHL) & IMX6_USDHC_SYS_CTRL_SDCLKFS_MASK);
    regVal |= ((3 << IMX6_USDHC_SYS_CTRL_DVS_SHL) & IMX6_USDHC_SYS_CTRL_DVS_MASK);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL, regVal | 0x0000000F);

    // re-enable clock to the card - clock is stable on exit
    sAllClocks(apUSDHC, TRUE, FALSE);

    // put prot_ctrl back to default
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_PROT_CTRL, 0x08800020);

    // set the timeout to the maximum valud
    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL);
    regVal &= ~IMX6_USDHC_SYS_CTRL_DTOCV_MASK;
    regVal |= (0x0E << IMX6_USDHC_SYS_CTRL_DTOCV_SHL) & IMX6_USDHC_SYS_CTRL_DTOCV_MASK;
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL, regVal | 0x0000000F);

    // disable card insertion and removal interrupt status, and bwr and brr
    sAnd32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS_EN, 
        ~(IMX6_USDHC_INT_CINS | 
          IMX6_USDHC_INT_CRM |
          IMX6_USDHC_INT_BRR |
          IMX6_USDHC_INT_BWR)
    );

    sClearIntStatus(apUSDHC);

    // sent init clocks and wait for them to go out
    sOr32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL, IMX6_USDHC_SYS_CTRL_INITA | 0x0000000F);
    do
    {
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL);
    } while (0 != (regVal & IMX6_USDHC_SYS_CTRL_INITA));
    IMX6_GPT_DelayUs(apUSDHC->mpGpt, 100);

    //
    // ready to reset card
    //

    // Reset card
//    DebugPrint(0xFFFFFFFF, "SD: CMD0\n");
    retries = 2;
    do {
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & (IMX6_USDHC_PRES_STATE_CIHB | IMX6_USDHC_PRES_STATE_CDIHB)));
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 0);
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD0 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;
//        DebugPrint(0xFFFFFFFF, "SD: CMD0 timed out\n");
        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for CMD0\n");
//            ASSERT(0);
            return FALSE;
        }
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
    } while (TRUE);
    if (0 != (regVal & (IMX6_USDHC_INT_CI | IMX6_USDHC_INT_CEB)))
    {
//        DebugPrint(0xFFFFFFFF, "CMD0 INT_STATUS %08X\n", regVal);
//        ASSERT(0);
        return FALSE;
    }

    //
    // -----------------------------------------------------------------------------------------
    //

    //
    // CMD8 - try 3.3V first
    //
//    DebugPrint(0xFFFFFFFF, "SD: CMD8\n");
    do {
        do {
            sClearIntStatus(apUSDHC);
            do {
                regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
            } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
            sClearIntStatus(apUSDHC);
            sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 0x1AA);
            sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD8 & 0xFFFF0000);
            do {
                regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
            } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
            if (0 == (regVal & IMX6_USDHC_INT_CTO))
                break;
//            DebugPrint(0xFFFFFFFF, "SD: CMD8 for 3.3V timed out\n");
            IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
        } while (0 != retries--);
        if (0 == (regVal & (IMX6_USDHC_INT_CI | IMX6_USDHC_INT_CEB | IMX6_USDHC_INT_CRC)))
            break;
        if (0 == retries)
            break;
        DoRSTA(apUSDHC, FALSE);
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 100000);
    } while (TRUE);

    if (0 != (regVal & (IMX6_USDHC_INT_CI | IMX6_USDHC_INT_CEB | IMX6_USDHC_INT_CRC)))
    {
        //
        // try 1.8V
        //
        do {
            do {
                sClearIntStatus(apUSDHC);
                do {
                    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
                } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
                sClearIntStatus(apUSDHC);
                sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 0x2AA);
                sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD8 & 0xFFFF0000);
                do {
                    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
                } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
                if (0 == (regVal & IMX6_USDHC_INT_CTO))
                    break;
//                DebugPrint(0xFFFFFFFF, "SD: CMD8 for 1.8V timed out\n");
                IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
            } while (0 != retries--);
            if (0 == (regVal & (IMX6_USDHC_INT_CI | IMX6_USDHC_INT_CEB | IMX6_USDHC_INT_CRC)))
                break;
            if (0 == retries)
                break;
            DoRSTA(apUSDHC, FALSE);
            IMX6_GPT_DelayUs(apUSDHC->mpGpt, 100000);
        } while (TRUE);

        if (0 != (regVal & (IMX6_USDHC_INT_CI | IMX6_USDHC_INT_CEB | IMX6_USDHC_INT_CRC)))
        {
//            DebugPrint(0xFFFFFFFF, "Unable to completed CMD8 (%08X)\n", regVal);
//            ASSERT(0);
            return FALSE;
        }
        sOr32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC, IMX6_USDHC_VEND_SPEC_VSELECT);
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 100000);
        ocr = 0;
//        DebugPrint(0xFFFFFFFF, "SD: 1.8V card\n");
    }
    else
    {
//        DebugPrint(0xFFFFFFFF, "SD: 3.3V card\n");
//        ASSERT((sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0) & 0x1FF) == 0x1AA);  // check pattern response
        if ((sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0) & 0x1FF) != 0x1AA)
            return FALSE;
        ocr = SDMMC_OCR_HIGH_CAP_SUPPORT;
    }

    //
    // card should be idle now
    // ----------------------------------------------------------------
    //

//    DebugPrint(0xFFFFFFFF, "SD: CMD55/ACMD41 Inquiry\n");
    do {
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
    } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
    sClearIntStatus(apUSDHC);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 0);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD55 & 0xFFFF0000);
    do {
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
    } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
    if (0 != (regVal & IMX6_USDHC_INT_CTO))
    {
//        DebugPrint(0xFFFFFFFF, "CMD55 for ACMD41 Inquiry timed out (%08X)\n", regVal);
//        ASSERT(0);
        return FALSE;
    }
    do {
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
    } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
    sClearIntStatus(apUSDHC);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 0);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_ACMD41 & 0xFFFF0000);
    do {
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
    } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
    if (0 != (regVal & IMX6_USDHC_INT_CTO))
    {
//        DebugPrint(0xFFFFFFFF, "ACMD41 Inquiry timed out (%08X)\n", regVal);
//        ASSERT(0);
        return FALSE;
    }

    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0);
//    DebugPrint(0xFFFFFFFF, "SD: CMD55/ACMD41 Inquiry result %08X\n", regVal);
    ocr |= regVal;

    // 
    // can now initialize with 'real' ACMD41 which may take time since the card is powering up
    //
    do {
//        DebugPrint(0xFFFFFFFF, "SD: CMD55/ACMD41 with OCR = %08X\n", ocr);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 0);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD55 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
        if (0 != (regVal & IMX6_USDHC_INT_CTO))
        {
//            DebugPrint(0xFFFFFFFF, "CMD55 for ACMD41 timed out (%08X)\n", regVal);
//            ASSERT(0);
            return FALSE;
        }
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, ocr);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_ACMD41 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
        if (0 != (regVal & IMX6_USDHC_INT_CTO))
        {
            //DebugPrint(0xFFFFFFFF, "ACMD41 timed out (%08X)\n", regVal);
//            ASSERT(0);
            return FALSE;
        }
        regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0);
//        DebugPrint(0xFFFFFFFF, "SD: CMD55/ACMD41 result %08X\n", regVal);
        if (0 != (regVal & SDMMC_OCR_POWERED_STATUS))
            break;
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 250000);
    } while (TRUE);

    if (0 != (regVal & SDMMC_OCR_HIGH_CAP_SUPPORT))
    {
        apUSDHC->Info.HighCapCard = TRUE;
    }
    else
    {
        apUSDHC->Info.HighCapCard = FALSE;
    }

    // CMD2 ALL_SEND_CID
//    DebugPrint(0xFFFFFFFF, "SD: CMD2\n");
    retries = 2;
    do {
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 0);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD2 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
//        DebugPrint(0xFFFFFFFF, "CMD2 INT_STATUS %08X\n", regVal);
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;
        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for CMD2\n");
//            ASSERT(0);
            return FALSE;
        }
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
    } while (1);
    response[0] = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0);
    response[1] = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP1);
    response[2] = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP2);
    response[3] = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP3);
    CopyMem(&apUSDHC->Info.CID, response, 4 * sizeof(UINT32));

    // CMD3 SEND_RCA
//    DebugPrint(0xFFFFFFFF, "SD: CMD3\n");
    retries = 2;
    do {
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 0);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD3 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
//        DebugPrint(0xFFFFFFFF, "CMD3 INT_STATUS %08X\n", regVal);
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;
        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for CMD3\n");
//            ASSERT(0);
            return FALSE;
        }
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
    } while (1);

    apUSDHC->Info.RCA = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0) & 0xFFFF0000;

    // CMD9 SEND_CSD
//    DebugPrint(0xFFFFFFFF, "SD: CMD9\n");
    retries = 2;
    do {
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, apUSDHC->Info.RCA);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD9 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
//        DebugPrint(0xFFFFFFFF, "CMD9 INT_STATUS %08X\n", regVal);
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;
        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for CMD9\n");
//            ASSERT(0);
            return FALSE;
        }
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
    } while (1);
    response[0] = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0);
    response[1] = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP1);
    response[2] = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP2);
    response[3] = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP3);
    CopyMem(((UINT8 *)&apUSDHC->Info.CSD) + 1, response, (4 * sizeof(UINT32)) - 1);

    if (apUSDHC->Info.CSD.ANY.CSD_STRUCTURE == 0)
    {
        // using CSD 1.0
        regVal = apUSDHC->Info.CSD.V1.READ_BL_LEN;
        apUSDHC->Info.BlockSize = (1 << regVal);
        regVal = (((UINT32)(apUSDHC->Info.CSD.V1.C_SIZEHigh)) << 2) | ((UINT32)apUSDHC->Info.CSD.V1.C_SIZELow);
        apUSDHC->Info.BlockCount = (regVal + 1) * (1 << (apUSDHC->Info.CSD.V1.C_SIZE_MULT + 2));
    }
    else
    {
        // using CSD 2.0
        regVal = (((UINT32)apUSDHC->Info.CSD.V2.C_SIZEHigh6) << 16) | (UINT32)(apUSDHC->Info.CSD.V2.C_SIZELow16);
        apUSDHC->Info.BlockSize = SDMMC_BLOCK_SIZE;
        apUSDHC->Info.BlockCount = (regVal << 10) + 1024;
    }

    apUSDHC->Info.MediaId = (((UINT32)(apUSDHC->Info.CID.PSN_HI)) << 16) | ((UINT32)apUSDHC->Info.CID.PSN_LO);

//    DebugPrint(0xFFFFFFFF, "SD: BlockSize %08X, BlockCount %08X\n", apUSDHC->Info.BlockSize, apUSDHC->Info.BlockCount);

    // get max card clock frequency
    regVal2 = apUSDHC->Info.CSD.ANY.TRAN_SPEED;
    regVal = (regVal2 >> 3) & 0xF;
    regVal2 &= 0x7;
    if ((regVal2 > 3) || (regVal == 0))
    {
//        DEBUG((EFI_D_ERROR, "Illegal TRAN_SPEED in CSD\n"));
//        ASSERT(0);
        return FALSE;
    }
//    transRate = ((sUnitFactor[regVal2] * sMultFactor[regVal]) * 100); // units of 1kBit/s but divide by 10 for frac
//    DebugPrint(0xFFFFFFFF, "SD: Max Rate %d\n", transRate);

    // speed up to target rate now - 198Mhz / 10 = 19.8Mhz
    sAllClocks(apUSDHC, FALSE, FALSE);
    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL);
    regVal &= ~IMX6_USDHC_SYS_CTRL_SDCLKFS_MASK;
    regVal |= ((1 << IMX6_USDHC_SYS_CTRL_SDCLKFS_SHL) & IMX6_USDHC_SYS_CTRL_SDCLKFS_MASK);
    regVal &= ~IMX6_USDHC_SYS_CTRL_DVS_MASK;
    regVal |= ((4 << IMX6_USDHC_SYS_CTRL_DVS_SHL) & IMX6_USDHC_SYS_CTRL_DVS_MASK);
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_SYS_CTRL, regVal | 0x0000000F);
    sAllClocks(apUSDHC, TRUE, FALSE);

//    DebugPrint(0xFFFFFFFF, "SD: Clock at target rate\n");

    // select the card 
//    DebugPrint(0xFFFFFFFF, "SD: CMD7\n");
    retries = 2;
    do {
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, apUSDHC->Info.RCA);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD7 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
        //DebugPrint(0xFFFFFFFF, "CMD7 INT_STATUS %08X\n", regVal);
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;
        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for CMD7\n");
//            ASSERT(0);
            return FALSE;
        }
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
    } while (1);

//    DebugPrint(0xFFFFFFFF, "SD: Card selected\n");

    // check the mode
//    DebugPrint(0xFFFFFFFF, "SD: CMD13\n");
    retries = 2;
    do {
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, apUSDHC->Info.RCA);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD13 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;
//        DebugPrint(0xFFFFFFFF, "CMD13 INT_STATUS %08X\n", regVal);
        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for CMD13\n");
//            ASSERT(0);
            return FALSE;
        }
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
    } while (1);

    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_CMD_RSP0);
    if (((regVal & SDMMC_STATUS_CURRENT_STATE_MASK) >> SDMMC_STATUS_CURRENT_STATE_SHL) != SDMMC_STATUS_STATE_TRAN)
    {
//        DebugPrint(0xFFFFFFFF, "CARD STATUS = %08X, not in TRAN state after CMD7!\n");
//        ASSERT(0);
        return FALSE;
    }

    //
    // in transfer state, remove forced clock
    //
//    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC);
//    regVal &= ~IMX6_USDHC_VEND_SPEC_FRC_SDCLK_ON;
//   sWrite32(apUSDHC, IMX6_USDHC_OFFSET_VEND_SPEC, regVal);

    // set block length for all future transfers to the default
//    DebugPrint(0xFFFFFFFF, "SD: CMD16\n");
    retries = 2;
    do {
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, SDMMC_BLOCK_SIZE);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD16 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
//        DebugPrint(0xFFFFFFFF, "CMD16 INT_STATUS %08X\n", regVal);
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;
        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for CMD16\n");
//            ASSERT(0);
            return FALSE;
        }
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
    } while (1);
//    DebugPrint(0xFFFFFFFF, "SD: Default block length set\n");

    // set bus width to 4 bits
//    DebugPrint(0xFFFFFFFF, "SD: ACMD6\n");
    do {
        retries = 2;
        do {
            do {
                regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
            } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
            sClearIntStatus(apUSDHC);
            sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, apUSDHC->Info.RCA);
            sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD55 & 0xFFFF0000);
            do {
                regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
            } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
            if (0 == (regVal & IMX6_USDHC_INT_CTO))
                break;
            if (0 == retries--)
            {
//                DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for CMD55\n");
//                ASSERT(0);
                return FALSE;
            }
            IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
        } while (TRUE);

        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & IMX6_USDHC_PRES_STATE_CIHB));
        sClearIntStatus(apUSDHC);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_ARG, 2);
        sWrite32(apUSDHC, IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_ACMD6 & 0xFFFF0000);
        do {
            regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;
        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "****SD: 3 x CTO for ACMD6\n");
//            ASSERT(0);
            return FALSE;
        }
        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);
    } while (TRUE);

    regVal = sRead32(apUSDHC, IMX6_USDHC_OFFSET_PROT_CTRL);
    regVal &= ~IMX6_USDHC_PROT_CTRL_DTW_MASK;
    regVal |= (1 << IMX6_USDHC_PROT_CTRL_DTW_SHL) & IMX6_USDHC_PROT_CTRL_DTW_MASK;
    sWrite32(apUSDHC, IMX6_USDHC_OFFSET_PROT_CTRL, regVal);

//    DebugPrint(0xFFFFFFFF, "SD: Bus width set to 4\n");

    return TRUE;
}

