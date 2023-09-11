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
#include <Library/CacheMaintenanceLib.h>
#include <Library/IMX6/IMX6UsdhcLib.h>

BOOLEAN
EFIAPI
IMX6_USDHC_Setup(
    IMX6_USDHC *    apUSDHC,
    UINT32          aUnitNum,
    UINT32          aRegs_CCM,
    IMX6_GPT *      apGpt
)
{
    ZeroMem(apUSDHC, sizeof(IMX6_USDHC));
    switch (aUnitNum)
    {
    case 1:
        apUSDHC->mRegs_USDHC = IMX6_PHYSADDR_USDHC1;
        break;
    case 2:
        apUSDHC->mRegs_USDHC = IMX6_PHYSADDR_USDHC2;
        break;
    case 3:
        apUSDHC->mRegs_USDHC = IMX6_PHYSADDR_USDHC3;
        break;
    case 4:
        apUSDHC->mRegs_USDHC = IMX6_PHYSADDR_USDHC4;
        break;
    default:
//        DEBUG((EFI_D_ERROR, "***Unsupported USDHC selected (%d)\n", aUnitNum));
        return FALSE;
    }
    apUSDHC->mUnitNum = aUnitNum;
    apUSDHC->mRegs_CCM = aRegs_CCM;
    apUSDHC->mpGpt = apGpt;

    return TRUE;
}

#if 0
static
void
sDumpRegs(
    IMX6_USDHC *    apUSDHC
)
{
    DebugPrint(0xFFFFFFFF, "\nDUMP\n----\n");
    DebugPrint(0xFFFFFFFF, "DS_ADDR              %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_DS_ADDR));
    DebugPrint(0xFFFFFFFF, "BLK_ATT              %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_BLK_ATT));
    DebugPrint(0xFFFFFFFF, "CMD_ARG              %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_ARG));
    DebugPrint(0xFFFFFFFF, "CMD_XFR_TYP          %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_XFR_TYP));
    DebugPrint(0xFFFFFFFF, "CMD_RSP0             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_RSP0));
    DebugPrint(0xFFFFFFFF, "CMD_RSP1             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_RSP1));
    DebugPrint(0xFFFFFFFF, "CMD_RSP2             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_RSP2));
    DebugPrint(0xFFFFFFFF, "CMD_RSP3             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_RSP3));
//    DebugPrint(0xFFFFFFFF, "DATA_BUFF_ACC_PORT   %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT));
    DebugPrint(0xFFFFFFFF, "PRES_STATE           %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE));
    DebugPrint(0xFFFFFFFF, "PROT_CTRL            %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PROT_CTRL));
    DebugPrint(0xFFFFFFFF, "SYS_CTRL             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_SYS_CTRL));
    DebugPrint(0xFFFFFFFF, "INT_STATUS           %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS));
    DebugPrint(0xFFFFFFFF, "INT_STATUS_EN        %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS_EN));
    DebugPrint(0xFFFFFFFF, "INT_SIGNAL_EN        %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_SIGNAL_EN));
    DebugPrint(0xFFFFFFFF, "AUTOCMD12_ERR_STATUS %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_AUTOCMD12_ERR_STATUS));
    DebugPrint(0xFFFFFFFF, "HOST_CTRL_CAP        %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_HOST_CTRL_CAP));
    DebugPrint(0xFFFFFFFF, "WTMK_LVL             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_WTMK_LVL));
    DebugPrint(0xFFFFFFFF, "MIX_CTRL             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_MIX_CTRL));
    DebugPrint(0xFFFFFFFF, "FORCE_EVENT          %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_FORCE_EVENT));
    DebugPrint(0xFFFFFFFF, "ADMA_ERR_STATUS      %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_ADMA_ERR_STATUS));
    DebugPrint(0xFFFFFFFF, "ADMA_SYS_ADDR        %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_ADMA_SYS_ADDR));
    DebugPrint(0xFFFFFFFF, "DLL_CTRL             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_DLL_CTRL));
    DebugPrint(0xFFFFFFFF, "DLL_STATUS           %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_DLL_STATUS));
    DebugPrint(0xFFFFFFFF, "CLK_TUNE_CTRL_STATUS %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS));
    DebugPrint(0xFFFFFFFF, "VEND_SPEC            %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC));
    DebugPrint(0xFFFFFFFF, "MMC_BOOT             %08X\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_MMC_BOOT));
    DebugPrint(0xFFFFFFFF, "VEND_SPEC2           %08X\n\n", MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC2));
}
#endif

static
void
sClearIntStatus(
    IMX6_USDHC * apUSDHC
)
{
    MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS, MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS));
}

void
DoRSTA(
    IMX6_USDHC *    apUSDHC,
    BOOLEAN         aInTransferMode
);

EFI_STATUS
IMX6_USDHC_TransferOne(
    IMX6_USDHC *    apUSDHC,
    BOOLEAN         IsWrite,
    IN EFI_LBA      Lba,
    IN OUT VOID *   Buffer
)
{
    UINT32      regVal;
    int         retries;
    UINT32 *    pWork;
    UINTN       left;

//    DebugPrint(0xFFFFFFFF, "Single block %d, %a\n", (UINT32)Lba, IsWrite ? "write" : "read");
    retries = 2;
    if (0 != (((UINTN)Buffer) & 3))
        return EFI_INVALID_PARAMETER;

    do {
        do {
            regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & (IMX6_USDHC_PRES_STATE_CIHB | IMX6_USDHC_PRES_STATE_CDIHB)));

        // set WML to 1
        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_WTMK_LVL);
        if (IsWrite)
        {
            regVal &= ~IMX6_USDHC_WTMK_LVL_WR_WBL_MASK;
            regVal |= ((1 << IMX6_USDHC_WTMK_LVL_WR_WBL_SHL) & IMX6_USDHC_WTMK_LVL_WR_WBL_MASK);
        }
        else
        {
            regVal &= ~IMX6_USDHC_WTMK_LVL_RD_WML_MASK;
            regVal |= ((1 << IMX6_USDHC_WTMK_LVL_RD_WML_SHL) & IMX6_USDHC_WTMK_LVL_RD_WML_MASK);
        }
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_WTMK_LVL, regVal);

        // set BLKCNT to 1
        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_BLK_ATT);
        regVal &= ~IMX6_USDHC_BLK_ATT_BLKCNT_MASK;
        regVal |= ((1 << IMX6_USDHC_BLK_ATT_BLKCNT_SHL) & IMX6_USDHC_BLK_ATT_BLKCNT_MASK);
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_BLK_ATT, regVal);

        // set properties of access
        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_MIX_CTRL);
        regVal &= ~(IMX6_USDHC_MIX_CTRL_MSBSEL |
            IMX6_USDHC_MIX_CTRL_AC12EN |
            IMX6_USDHC_MIX_CTRL_DMAEN |
            IMX6_USDHC_MIX_CTRL_BCEN);
        if (IsWrite)
        {
            regVal &= ~IMX6_USDHC_MIX_CTRL_DTDSEL;
        }
        else
        {
            regVal |= IMX6_USDHC_MIX_CTRL_DTDSEL;
        }
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_MIX_CTRL, regVal);

        // set LBA to read from or write to
        if (FALSE == apUSDHC->Info.HighCapCard)
            Lba *= SDMMC_BLOCK_SIZE;
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_ARG, (UINT32)Lba);

        // clear interrupt status
        sClearIntStatus(apUSDHC);

        // exec command
        if (IsWrite)
        {
            MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD24 & 0xFFFF0000);
        }
        else
        {
            MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD17 & 0xFFFF0000);
        }

        // wait for command status
        do
        {
            regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;

//        DebugPrint(0xFFFFFFFF, "SD: single block i/o int_status = %08X\n", regVal);

        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "SD: CTO x 3 on single block i/o\n");
            return EFI_DEVICE_ERROR;
        }

        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
        if (0 != (regVal & (IMX6_USDHC_PRES_STATE_CIHB | IMX6_USDHC_PRES_STATE_CDIHB)))
        {
            DoRSTA(apUSDHC, TRUE);
        }

        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);

    } while (TRUE);

//    DebugPrint(0xFFFFFFFF, "Data Transfer Active...\n");
    left = SDMMC_BLOCK_SIZE / sizeof(UINT32);
    pWork = (UINT32 *)Buffer;
    do
    {
        if (IsWrite)
        {
            // bleagh
            do
            {
                regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
            } while (0 == (regVal & IMX6_USDHC_PRES_STATE_BWEN));
            MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT, *pWork);
        }
        else
        {
            // yummy
            do
            {
                regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
            } while (0 == (regVal & IMX6_USDHC_PRES_STATE_BREN));
            *pWork = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT);
        }
        pWork++;
    } while (--left);
//    DebugPrint(0xFFFFFFFF, "...Data Transfer Complete\n");

    return EFI_SUCCESS;
}

EFI_STATUS
IMX6_USDHC_TransferMany(
    IMX6_USDHC *    apUSDHC,
    BOOLEAN         IsWrite,
    IN EFI_LBA      Lba,
    IN UINTN        BlockCount,
    IN OUT VOID *   Buffer
)
{
    UINT32      regVal;
    int         retries;
    UINTN       left;

    retries = 2;

//    DebugPrint(0xFFFFFFFF, "Multi block %d, %d, %a addr %08X\n", (UINT32)Lba, BlockCount, IsWrite ? "write" : "read", Buffer);

    do {
        do {
            regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
        } while (0 != (regVal & (IMX6_USDHC_PRES_STATE_CIHB | IMX6_USDHC_PRES_STATE_CDIHB)));

        // set WML to 1
        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_WTMK_LVL);
        left = SDMMC_BLOCK_SIZE / sizeof(UINT32);
        if (IsWrite)
        {
            regVal &= ~IMX6_USDHC_WTMK_LVL_WR_WBL_MASK;
            regVal |= ((left << IMX6_USDHC_WTMK_LVL_WR_WBL_SHL) & IMX6_USDHC_WTMK_LVL_WR_WBL_MASK);
        }
        else
        {
            regVal &= ~IMX6_USDHC_WTMK_LVL_RD_WML_MASK;
            regVal |= ((left << IMX6_USDHC_WTMK_LVL_RD_WML_SHL) & IMX6_USDHC_WTMK_LVL_RD_WML_MASK);
        }
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_WTMK_LVL, regVal);

        // data source or target
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_DS_ADDR, (UINT32)Buffer);

        // set BLKCNT
        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_BLK_ATT);
        regVal &= ~IMX6_USDHC_BLK_ATT_BLKCNT_MASK;
        regVal |= ((BlockCount << IMX6_USDHC_BLK_ATT_BLKCNT_SHL) & IMX6_USDHC_BLK_ATT_BLKCNT_MASK);
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_BLK_ATT, regVal);

        // use system DMA
        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC);
        if (0 != (regVal & IMX6_USDHC_VEND_SPEC_EXT_DMA_EN))
        {
            regVal &= ~IMX6_USDHC_VEND_SPEC_EXT_DMA_EN;
            MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_VEND_SPEC, regVal);
        }

        // set properties of access
        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_MIX_CTRL);
        regVal |= IMX6_USDHC_MIX_CTRL_MSBSEL |
            IMX6_USDHC_MIX_CTRL_AC12EN |
            IMX6_USDHC_MIX_CTRL_DMAEN |
            IMX6_USDHC_MIX_CTRL_BCEN;
        if (IsWrite)
        {
            regVal &= ~IMX6_USDHC_MIX_CTRL_DTDSEL;
        }
        else
        {
            regVal |= IMX6_USDHC_MIX_CTRL_DTDSEL;
        }
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_MIX_CTRL, regVal);

        // set LBA to read from
        if (FALSE == apUSDHC->Info.HighCapCard)
            Lba *= SDMMC_BLOCK_SIZE;
        MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_ARG, (UINT32)Lba);

        // clear interrupt status
        sClearIntStatus(apUSDHC);

        // get ready for data to be transferred to/from the target buffer by DMA
        WriteBackInvalidateDataCacheRange(Buffer, BlockCount * SDMMC_BLOCK_SIZE);

        // exec command
        if (IsWrite)
        {
            MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD25 & 0xFFFF0000);
        }
        else
        {
            MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_CMD_XFR_TYP, IMX6_USDHC_CMD18 & 0xFFFF0000);
        }

        // wait for command status
        do 
        {
            regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS);
        } while (0 == (regVal & (IMX6_USDHC_INT_CC | IMX6_USDHC_INT_CTO)));
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
        {
            //
            // clear command complete interrupt status but leave the other ones alone
            //
            MmioWrite32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS, IMX6_USDHC_INT_CC);
            break;
        }

        //
        // timeout for command
        //
        if (0 == (regVal & IMX6_USDHC_INT_CTO))
            break;

//        DebugPrint(0xFFFFFFFF, "SD: multiple block i/o int_status = %08X\n", regVal);

        if (0 == retries--)
        {
//            DebugPrint(0xFFFFFFFF, "SD: CTO x 3 on multiple block i/o\n");
            return EFI_DEVICE_ERROR;
        }

        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE);
        if (0 != (regVal & (IMX6_USDHC_PRES_STATE_CIHB | IMX6_USDHC_PRES_STATE_CDIHB)))
        {
            DoRSTA(apUSDHC, TRUE);
        }

        IMX6_GPT_DelayUs(apUSDHC->mpGpt, 5000);

    } while (TRUE);

    //
    // data should be going out or coming in.  wait for TC or DINT
    //
//    DebugPrint(0xFFFFFFFF, "Data Transfer Active...\n");
    do
    {
        regVal = MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_INT_STATUS);
    } while (0 == (regVal & IMX6_USDHC_INT_TC));

//    DebugPrint(0xFFFFFFFF, "...Data Transfer Complete (ended %08X, status %08X)\n", (UINTN)(Buffer + (BlockCount * SDMMC_BLOCK_SIZE)), regVal);

    if (MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_DS_ADDR) != (UINTN)(Buffer + (BlockCount * SDMMC_BLOCK_SIZE)))
    {
//        DEBUG((EFI_D_ERROR, "DMA FAILURE\n"));
//        ASSERT(FALSE);
        return EFI_DEVICE_ERROR;
    }

    if (!IsWrite)
    {
        InvalidateDataCacheRange(Buffer, BlockCount * SDMMC_BLOCK_SIZE);
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IMX6_USDHC_DoTransfer(
    IMX6_USDHC *    apUSDHC,
    BOOLEAN         aIsWrite,
    EFI_LBA         Lba,
    UINTN           BufferSizeBytes,
    VOID *          Buffer
)
{
    EFI_STATUS Status;

    if (0 == apUSDHC->Info.RCA)
    {
//        DebugPrint(0xFFFFFFFF, "SD: Card not initialized\n");
        return EFI_DEVICE_ERROR;
    }

    if ((Buffer == NULL) ||
        ((((UINTN)Buffer) & 3) != 0))
        return EFI_INVALID_PARAMETER;

    if ((BufferSizeBytes % SDMMC_BLOCK_SIZE) != 0)
        return EFI_BAD_BUFFER_SIZE;

    BufferSizeBytes /= SDMMC_BLOCK_SIZE;

    // BufferSizeBytes measures BLOCKS now

    if ((Lba >= apUSDHC->Info.BlockCount) ||
        ((apUSDHC->Info.BlockCount - Lba) < BufferSizeBytes))
        return EFI_INVALID_PARAMETER;

    // SD clock must be on or something is seriously wrong
    ASSERT(0 == (MmioRead32(apUSDHC->mRegs_USDHC + IMX6_USDHC_OFFSET_PRES_STATE) & IMX6_USDHC_PRES_STATE_SDOFF));

    if (1 == BufferSizeBytes)
    {
        return IMX6_USDHC_TransferOne(apUSDHC, aIsWrite, Lba, Buffer);
    }

    if (0 == (((UINT32)Buffer) & 0xFFF))
    {
        // page-aligned many-block transfer; QEMU messes this up
        // so do a page to misalign first
        Status = IMX6_USDHC_TransferOne(apUSDHC, aIsWrite, Lba, Buffer);
        if (EFI_ERROR(Status))
            return Status;
        Lba++;
        Buffer = (VOID *)(((UINT32)Buffer) + SDMMC_BLOCK_SIZE);
        BufferSizeBytes--;
        if (BufferSizeBytes == SDMMC_BLOCK_SIZE)
        {
            return IMX6_USDHC_TransferOne(apUSDHC, aIsWrite, Lba, Buffer);
        }
    }

    return IMX6_USDHC_TransferMany(apUSDHC, aIsWrite, Lba, BufferSizeBytes, Buffer);
}

