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

#include "..\pc.h"
#include "..\dbgser\dbgser.h"

K2OS_IOADDR_INFO gIoInfoTable[1] =
{
    { PC_SERIAL1_IOPORT, 8 },    // used for debug i/o
};

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

K2OSKERN_PLATINTR * 
K2_CALLCONV_REGS 
K2OSPLAT_OnIrq(
    K2OSKERN_PLATIRQ const *apIrq
)
{
    if ((NULL == apIrq) ||
        (apIrq->IntrList.mNodeCount == 0))
        return NULL;

    if (apIrq->IntrList.mNodeCount == 1)
    {
        //
        // should never get called with only one choice to make
        // but if you do, then return the first one in the list
        //
        return K2_GET_CONTAINER(K2OSKERN_PLATINTR, apIrq->IntrList.mpHead, IrqIntrListLink);
    }

    //
    // find out which device interrupted.  if you don't know then return NULL
    //

    return NULL;
}

void
K2OSPLAT_GetResTable(
    UINT32          aTableIx,
    UINT32 *        apRetCount,
    void const **   appRetTable
)
{
    if (1 == aTableIx)
    {
        *apRetCount = 1;
        *appRetTable = gIoInfoTable;
    }
    else
    {
        *apRetCount = 0;
    }
}

UINT_PTR 
K2OSPLAT_ForcedDriverQuery(
    K2OSKERN_PLATNODE const *   apNode,
    char *                      apIoBuf,
    UINT_PTR                    aIoBufSizeBytes,
    UINT_PTR                    aIoBufInCount
)
{
    char const * pScan;

    if (0 == aIoBufInCount)
    {
        return 0;
    }

    //
    // first forced driver is the PCI bus so we can see the ide controller
    //
    pScan = apIoBuf;
    do {
        pScan = K2ASC_FindCharConstIns('H', pScan);
        if (NULL == pScan)
            break;
        if (0 == K2ASC_CompInsLen(pScan, "H(PNP0A03)", 10))
        {
            //
            // this is the root pci bus. force to built-in pci bus driver
            //
            return K2ASC_Copy(apIoBuf, ":pcibus.xdl");
        }
        pScan++;
    } while ((*pScan) != 0);

    //
    // second forced driver is for the IDE controller so we can load stuff
    //
    pScan = apIoBuf;
    do {
        pScan = K2ASC_FindCharConstIns('V', pScan);
        if (NULL == pScan)
            break;
        if (0 == K2ASC_CompInsLen(pScan, "VEN(8086)", 9))
        {
            pScan = apIoBuf;
            do {
                pScan = K2ASC_FindCharConstIns('D', pScan);
                if (NULL == pScan)
                    break;
                if (0 == K2ASC_CompInsLen(pScan, "DEV(7111)", 9))
                {
                    //
                    // this is the ide controller. force the driver to the built in ide bus driver
                    //
                    return K2ASC_Copy(apIoBuf, ":idebus.xdl");
                }
                pScan++;
            } while ((*pScan) != 0);
   
            break;
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



