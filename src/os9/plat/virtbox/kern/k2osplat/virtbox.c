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

#include "..\pc.h"
#include "..\dbgser\dbgser.h"

static UINT32 gCount = 0;
static UINT32 gResCount = 0;

K2OS_PLATINIT_MAP * 
K2_CALLCONV_REGS 
K2OSPLAT_Init(
    K2OS_PLATINIT_MAP const *apMapped
)
{
    PC_DBGSER_Init();

    //
    // no built in mappings needed
    //
    return NULL;
}

void
K2_CALLCONV_REGS
K2OSPLAT_DebugOut(
    UINT8 aByte
)
{
    if (aByte == 0)
        aByte = '~';
    else
    if (aByte == '\n')
        PC_DBGSER_OutByte('\r');
    PC_DBGSER_OutByte(aByte);
}

BOOL
K2_CALLCONV_REGS
K2OSPLAT_DebugIn(
    UINT8 *apRetData
)
{
    return PC_DBGSER_InByte(apRetData);
}

K2OSPLAT_DEV 
K2OSPLAT_DeviceCreate(
    K2OSPLAT_DEV            aParent,
    UINT32                  aName,
    UINT32                  aDeviceContext,
    K2_DEVICE_IDENT const * apDeviceIdent,
    UINT8  *                apMountInfoIo,
    UINT32                  aMountInfoBufferBytes,
    UINT32 *                apMountInfoBytesIo
)
{
    UINT32          bytesIn;
    char const *    pScan;

    if (NULL == apMountInfoBytesIo)
        return (K2OSPLAT_DEV)++gCount;

    bytesIn = *apMountInfoBytesIo;
    *apMountInfoBytesIo = 0;

    //    K2OSKERN_Debug("K2OSPLAT_DeviceCreate(%d, %.4s, %08X, %d)\n", aParent, (char const *)&aName, aDeviceContext, aBytesIn);
    //    K2OSKERN_Debug("  MountInfo: \"%.*s\"\n", aBytesIn, (char *)apMountInfoIo);

    if (0 != bytesIn)
    {
        //
        // first forced driver is the built-in PCI bus so we can see the ide controller
        //
        pScan = (char const *)apMountInfoIo;
        do {
            pScan = K2ASC_FindCharConstIns('H', pScan);
            if (NULL == pScan)
                break;
            if (0 == K2ASC_CompInsLen(pScan, "H(PNP0A03)", 10))
            {
                //
                // this is the root pci bus. force to built-in pci bus driver
                //
                *apMountInfoBytesIo = K2ASC_Copy((char *)apMountInfoIo, ":pcibus.xdl");
                return (K2OSPLAT_DEV)++gCount;
            }
            pScan++;
        } while ((*pScan) != 0);

    }

    //
    // second forced driver is for the built-in IDE controller so we can load stuff
    //
    if ((apDeviceIdent->mVendorId == 0x8086) &&
        (apDeviceIdent->mDeviceId == 0x7111))
    {
        //
        // this is the ide controller. force the driver to the built in ide driver
        //
        *apMountInfoBytesIo = K2ASC_Copy((char *)apMountInfoIo, ":ide.xdl");
        return (K2OSPLAT_DEV)++gCount;
    }

    //
    // third forced driver is the ramdisk
    //
    if (0 == K2ASC_CompInsLen((char const *)apMountInfoIo, "RAMDISK;", 8))
    {
        *apMountInfoBytesIo = K2ASC_Copy((char *)apMountInfoIo, ":ramdisk.xdl");
        return (K2OSPLAT_DEV)++gCount;
    }

    //
    // we don't care what this is right now
    //

    return (K2OSPLAT_DEV)++gCount;
}

BOOL 
K2OSPLAT_DeviceRemove(
    K2OSPLAT_DEV aDevice
)
{
//    K2OSKERN_Debug("K2OSPLAT_DeviceRemove(%d)\n", aDevice);
    return TRUE;
}

K2OSPLAT_RES 
K2OSPLAT_DeviceAddRes(
    K2OSPLAT_DEV aDevice,
    UINT32 aResType,
    UINT32 aResInfo1,
    UINT32 aResInfo2
)
{
//    K2OSKERN_Debug("K2OSPLAT_DeviceAddRes(%d, %d, %08X, %08X)\n", aDevice, aResType, aResInfo1, aResInfo2);
    return (K2OSPLAT_RES)++gResCount;
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



