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

#include "pcibus.h"

BOOL
PCIBUS_CheckOverrideIDE(
    PCIBUS_CHILD *apChild
)
{
    K2STAT          stat;
    UINT64          val64;
    UINT64          size64;
    UINT8           progIf;
    UINT_PTR        intLine;
    K2OS_IRQ_CONFIG irqConfig;
    UINT_PTR        ix;
    UINT32          barVal[5];
    BOOL            legacyIo[2];
    BOOL            legacyIrqs;

    //
    // we are going to override unless we don't have to
    //
    progIf = apChild->Ident.mProgIF;

    if (0 != (0x01 & progIf))
    {
        //
        // primary channel is not in pci native mode
        //
        if (0 != (0x02 & progIf))
        {
            //
            // flag says we should be able to change it into pci native mode
            //
            val64 = progIf | 0x02;
            stat = PCIBUS_ConfigWrite(&apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Could not write ide progif1 (%08X)\n", gPciSegNum, gPciBusNum, stat);
                return TRUE;
            }
            stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Could not read ide progif1 (%08X)\n", gPciSegNum, gPciBusNum, stat);
                return TRUE;
            }
            progIf = apChild->Ident.mProgIF = (UINT8)val64;
        }
    }

    if (0 != (0x04 & progIf))
    {
        //
        // secondary channel is not in pci native mode
        //
        if (0 != (0x08 & progIf))
        {
            //
            // flag says we should be able to change it into pci native mode
            //
            val64 = progIf | 0x08;
            stat = PCIBUS_ConfigWrite(&apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Could not write ide progif2 (%08X)\n", gPciSegNum, gPciBusNum, stat);
                return TRUE;
            }
            stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Could not read ide progif2 (%08X)\n", gPciSegNum, gPciBusNum, stat);
                return TRUE;
            }
            progIf = apChild->Ident.mProgIF = (UINT8)val64;
        }
    }

    //
    // int line may have changed if we enabled native pci.  so read line now
    //
    stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_INTLINE, &val64, 8);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Could not read ide int line (%08X)\n", gPciSegNum, gPciBusNum, stat);
        return TRUE;
    }
    intLine = (UINT8)val64;

    legacyIrqs = (0xFF == intLine) ? TRUE : FALSE;

    //
    // read the BARs
    //
    legacyIo[0] = FALSE;
    for (ix = 0; ix < 5; ix++)
    {
        stat = PCIBUS_ReadBAR(apChild, ix, &barVal[ix]);
        if (0 == barVal[ix])
        {
            legacyIo[0] = TRUE;
        }
        if (K2STAT_IS_ERROR(stat))
            return stat;
    }

    if ((!legacyIrqs) && (!legacyIo[0]))
    {
        //
        // all BARs specified and irq is set too.  so don't override
        //
        return FALSE;
    }

    //
    // if we get here we are overriding
    //

    legacyIo[0] = legacyIo[1] = FALSE;

    if (0 != barVal[4])
    {
        //
        // bus master bar set. so just use that
        //
        if (0 != (barVal[4] & PCI_BAR_TYPE_IO_BIT))
        {
            stat = K2OSDRV_AddChildIo(apChild->mObjectId, barVal[4] & PCI_BAR_BASEMASK_IO, 16);
        }
        else
        {
            val64 = barVal[4] & PCI_BAR_BASEMASK_MEMORY;
            size64 = 16;
            stat = K2OSDRV_AddChildPhys(apChild->mObjectId, &val64, &size64);
        }
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add bus master range for IDE (%08X)\n", gPciSegNum, gPciBusNum, stat);
        }
    }
    else
    {
        if (0 != (0x01 & progIf))
        {
            if ((0 == barVal[0]) || (0 == barVal[1]))
            {
                //
                // pci native mode specified but one of the bars is zero so use the legacy ports
                //
                progIf &= ~0x01;
                apChild->Ident.mProgIF = progIf;
                legacyIo[0] = TRUE;
            }
            else
            {
                legacyIo[0] = FALSE;
            }
        }
        else
        {
            legacyIo[0] = TRUE;
        }

        if (0 != (0x04 & progIf))
        {
            if ((0 == barVal[2]) || (0 == barVal[3]))
            {
                //
                // pci native mode specified but one of the bars is zero so use the legacy ports
                //
                progIf &= ~0x04;
                apChild->Ident.mProgIF = progIf;
                legacyIo[1] = TRUE;
            }
            else
            {
                legacyIo[1] = FALSE;
            }
        }
        else
        {
            legacyIo[1] = TRUE;
        }

        //
        // now we populate ios based on the flags
        //

        if (legacyIo[0])
        {
            stat = K2OSDRV_AddChildIo(apChild->mObjectId, 0x1F0, 8);
            if (!K2STAT_IS_ERROR(stat))
            {
                stat = K2OSDRV_AddChildIo(apChild->mObjectId, 0x3F6, 4);
            }
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add at least part of IDE primary channel io override (%08X)\n", gPciSegNum, gPciBusNum, stat);
            }
        }
        else
        {
            if (0 != (barVal[0] & PCI_BAR_TYPE_IO_BIT))
            {
                stat = K2OSDRV_AddChildIo(apChild->mObjectId, barVal[0] & PCI_BAR_BASEMASK_IO, 8);
            }
            else
            {
                val64 = barVal[0] & PCI_BAR_BASEMASK_MEMORY;
                size64 = 8;
                stat = K2OSDRV_AddChildPhys(apChild->mObjectId, &val64, &size64);
            }
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add IDE primary channel io BAR range0 (%08X)\n", gPciSegNum, gPciBusNum, stat);
            }
            else
            {
                if (0 != (barVal[1] & PCI_BAR_TYPE_IO_BIT))
                {
                    stat = K2OSDRV_AddChildIo(apChild->mObjectId, barVal[1] & PCI_BAR_BASEMASK_IO, 4);
                }
                else
                {
                    val64 = barVal[1] & PCI_BAR_BASEMASK_MEMORY;
                    size64 = 4;
                    stat = K2OSDRV_AddChildPhys(apChild->mObjectId, &val64, &size64);
                }
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add IDE primary channel mem BAR range1 (%08X)\n", gPciSegNum, gPciBusNum, stat);
                }
            }
        }

        if (legacyIo[1])
        {
            stat = K2OSDRV_AddChildIo(apChild->mObjectId, 0x170, 8);
            if (!K2STAT_IS_ERROR(stat))
            {
                stat = K2OSDRV_AddChildIo(apChild->mObjectId, 0x376, 4);
            }
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add at least part of IDE secondary channel io override (%08X)\n", gPciSegNum, gPciBusNum, stat);
            }
        }
        else
        {
            if (0 != (barVal[2] & PCI_BAR_TYPE_IO_BIT))
            {
                stat = K2OSDRV_AddChildIo(apChild->mObjectId, barVal[2] & PCI_BAR_BASEMASK_IO, 8);
            }
            else
            {
                val64 = barVal[2] & PCI_BAR_BASEMASK_MEMORY;
                size64 = 8;
                stat = K2OSDRV_AddChildPhys(apChild->mObjectId, &val64, &size64);
            }
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add IDE secondary channel io BAR range0 (%08X)\n", gPciSegNum, gPciBusNum, stat);
            }
            else
            {
                if (0 != (barVal[3] & PCI_BAR_TYPE_IO_BIT))
                {
                    stat = K2OSDRV_AddChildIo(apChild->mObjectId, barVal[3] & PCI_BAR_BASEMASK_IO, 4);
                }
                else
                {
                    val64 = barVal[3] & PCI_BAR_BASEMASK_MEMORY;
                    size64 = 4;
                    stat = K2OSDRV_AddChildPhys(apChild->mObjectId, &val64, &size64);
                }
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add IDE secondary channel mem BAR range1 (%08X)\n", gPciSegNum, gPciBusNum, stat);
                }
            }
        }
    }

    //
    // last add irq(s)
    //
    if (legacyIrqs)
    {
        K2MEM_Zero(&irqConfig, sizeof(irqConfig));
        irqConfig.mSourceIrq = 14;
        irqConfig.Line.mIsActiveLow = TRUE;
        stat = K2OSDRV_AddChildIrq(apChild->mObjectId, &irqConfig);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to override IDE irq 14 for bus master (%08X)\n", gPciSegNum, gPciBusNum, stat);
        }
        intLine = 15;
    }
    K2MEM_Zero(&irqConfig, sizeof(irqConfig));
    irqConfig.mSourceIrq = intLine;
    irqConfig.Line.mIsActiveLow = TRUE;
    stat = K2OSDRV_AddChildIrq(apChild->mObjectId, &irqConfig);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to override IDE irq %d for bus master (%08X)\n", gPciSegNum, gPciBusNum, intLine, stat);
    }

    return TRUE;
}

BOOL
PCIBUS_CheckOverrideRes(
    PCIBUS_CHILD *apChild
)
{
    K2OSDrv_DebugPrintf("CLASS %d, SUBCLASS %d\n", apChild->Ident.mClassCode, apChild->Ident.mSubClassCode);
    if ((apChild->Ident.mClassCode == PCI_CLASS_STORAGE) &&
        (apChild->Ident.mSubClassCode == PCI_STORAGE_SUBCLASS_IDE))
    {
        return PCIBUS_CheckOverrideIDE(apChild);
    }

    return FALSE;
}
