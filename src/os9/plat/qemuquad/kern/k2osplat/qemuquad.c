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

#include "..\qemuquad.h"

#define IMX6_UART_OFFSET_URXD       0x0000 
#define IMX6_UART_OFFSET_UTXD       0x0040 
#define IMX6_UART_OFFSET_USR2       0x0098 
#define IMX6_UART_OFFSET_UTS        0x00B4 

#define IMX6_UART_USR2_RDR          0x00000001

#define IMX6_UART_UTS_TXEMPTY       0x00000040

#define USE_UART_PHYSADDR 0x021E8000    // UART2

static UINT32 sgUartVirt = 0;

static K2OS_PHYSADDR_INFO sgPhysInfo[] =
{
    { USE_UART_PHYSADDR, K2_VA_MEMPAGE_BYTES },
};

static K2OS_PLATINIT_MAP sgInitMap[] =
{
    { USE_UART_PHYSADDR, 1, K2OS_MEMPAGE_ATTR_DEVICEIO | K2OS_MEMPAGE_ATTR_READWRITE, 0 },
    { 0, 0, 0, 0 }
};

K2OS_PLATINIT_MAP * 
K2_CALLCONV_REGS 
K2OSPLAT_Init(
    K2OS_PLATINIT_MAP const * apMapped
)
{
    if (NULL == apMapped)
    {
        return sgInitMap;
    }

    sgUartVirt = sgInitMap[0].mVirtAddr;
    K2_ASSERT(0 != sgUartVirt);
    K2_CpuWriteBarrier();

    return sgInitMap;
}

void
K2_CALLCONV_REGS
K2OSPLAT_DebugOut(
    UINT8 aByte
)
{
    if (aByte == 0)
        aByte = '~';
    else if (aByte == '\n')
    {
        while (0 == (MMREG_READ32(sgUartVirt, IMX6_UART_OFFSET_UTS) & IMX6_UART_UTS_TXEMPTY));
        MMREG_WRITE32(sgUartVirt, IMX6_UART_OFFSET_UTXD, '\r');
    }
    while (0 == (MMREG_READ32(sgUartVirt, IMX6_UART_OFFSET_UTS) & IMX6_UART_UTS_TXEMPTY));
    MMREG_WRITE32(sgUartVirt, IMX6_UART_OFFSET_UTXD, aByte);
}

BOOL
K2_CALLCONV_REGS
K2OSPLAT_DebugIn(
    UINT8 *apRetData
)
{
    if (0 == (MMREG_READ32(sgUartVirt, IMX6_UART_OFFSET_USR2) & IMX6_UART_USR2_RDR))
        return FALSE;
    *apRetData = (UINT8)(MMREG_READ32(sgUartVirt, IMX6_UART_OFFSET_URXD) & 0xFF);
    return TRUE;
}

BOOL
K2_CALLCONV_REGS 
K2OSPLAT_CheckIntr(
    K2OSKERN_PLATNODE const *apPlatNode
)
{
    return TRUE;
}

void
K2OSPLAT_GetResTable(
    UINT32          aTableIx,
    UINT32 *        apRetCount,
    void const **   appRetTable
)
{
    *apRetCount = sizeof(sgPhysInfo) / sizeof(K2OS_PHYSADDR_INFO);
    *appRetTable = sgPhysInfo;
}

UINT32 
K2OSPLAT_ForcedDriverQuery(
    K2OSKERN_PLATNODE const *   apNode,
    char *                      apIoBuf,
    UINT32                    aIoBufSizeBytes,
    UINT32                    aIoBufInCount
)
{
    char const * pScan;

    if (0 == aIoBufInCount)
    {
        return 0;
    }

    //
    // first forced driver is the USDHC controller
    //
    pScan = apIoBuf;
    do {
        pScan = K2ASC_FindCharConstIns('H', pScan);
        if (NULL == pScan)
            break;
        if (0 == K2ASC_CompInsLen(pScan, "H(FSCL0020)", 10))
        {
            //
            // this is the root pci bus. force to built-in pci bus driver
            //
            return K2ASC_Copy(apIoBuf, ":fsusdhc.xdl");
        }
        pScan++;
    } while ((*pScan) != 0);

    return 0;
}

K2STAT
K2_CALLCONV_REGS
xdl_entry(
    XDL *   apXdl,
    UINT32  aReason
)
{
    return 0;
}



