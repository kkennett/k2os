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

#include "pcibus.h"

BOOL
PciBus_CheckOverrideIDE(
    PCIBUS_CHILD *  apChild
)
{
    K2STAT          stat;
    UINT64          val64;
    UINT8           progIf;
    UINT32          intLine;
    UINT32          ix;
    UINT32          barVal[5];
    UINT32          irqVal[2];
    K2OSDDK_RESDEF  resDef;
    PCIBUS *        pPciBus;

    pPciBus = apChild->mpParent;

    //
    // we are going to override unless we don't have to
    //
    progIf = apChild->Ident.mProgIF;

    if (0 == (0x01 & progIf))
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
            stat = pPciBus->mfConfigWrite(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Could not write ide progif1 (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
                return TRUE;
            }
            stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Could not read ide progif1 (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
                return TRUE;
            }
            progIf = apChild->Ident.mProgIF = (UINT8)val64;
        }
    }

    if (0 == (0x04 & progIf))
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
            stat = pPciBus->mfConfigWrite(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Could not write ide progif2 (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
                return TRUE;
            }
            stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Could not read ide progif2 (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
                return TRUE;
            }
            progIf = apChild->Ident.mProgIF = (UINT8)val64;
        }
    }

    //
    // int line may have changed if we enabled native pci.  so read line now
    //
    stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_INTLINE, &val64, 8);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): Could not read ide int line (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
        return TRUE;
    }
    intLine = (UINT8)val64;

    if (0xFF == intLine)
    {
        irqVal[0] = irqVal[1] = 0xFF;
    }
    else
    {
        irqVal[0] = intLine;
        irqVal[1] = 0xFF;
    }

    //
    // read the BARs
    //
    for (ix = 0; ix < 5; ix++)
    {
        stat = PciBus_ReadBAR(apChild, ix, &barVal[ix]);
        if (K2STAT_IS_ERROR(stat))
            return stat;
    }

    //
    // primary channel bars 0 and 1
    //
    if (0 != barVal[0])
    {
        if (0 == barVal[1])
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): IDE primary channel has base bar set but control bar clear\n", pPciBus->mSegNum, pPciBus->mBusNum);
            return TRUE;
        }
    }
    else
    {
        // must not be in PCI native mode
        if (0 != (0x01 & progIf))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): IDE primary channel in pci native mode but bar0 is empty\n", pPciBus->mSegNum, pPciBus->mBusNum);
            return TRUE;
        }

        if (0 != barVal[1])
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): legacy IDE primary channel has base bar clear but control bar set\n", pPciBus->mSegNum, pPciBus->mBusNum);
            return TRUE;
        }

        // use legacy addresses
        barVal[0] = 0x1F0 | PCI_BAR_TYPE_IO_BIT;
        barVal[1] = 0x3F6 | PCI_BAR_TYPE_IO_BIT;
        if (0xFF == irqVal[0])
            irqVal[0] = 14;
    }

    // 
    // secondary channel bars 2 and 3
    //
    if (0 != barVal[2])
    {
        if (0 == barVal[3])
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): IDE secondary channel has base bar set but control bar clear\n", pPciBus->mSegNum, pPciBus->mBusNum);
            return TRUE;
        }
    }
    else
    {
        // must not be in PCI native mode
        if (0 != (0x04 & progIf))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): IDE secondary channel in pci native mode but bar0 is empty\n", pPciBus->mSegNum, pPciBus->mBusNum);
            return TRUE;
        }

        if (0 != barVal[3])
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): legacy IDE secondary channel has base bar clear but control bar set\n", pPciBus->mSegNum, pPciBus->mBusNum);
            return TRUE;
        }

        // use legacy addresses
        barVal[2] = 0x170 | PCI_BAR_TYPE_IO_BIT;
        barVal[3] = 0x376 | PCI_BAR_TYPE_IO_BIT;
        if (0xFF == irqVal[1])
            irqVal[1] = 15;
    }

    //
    // populate access to primary channel and its irq
    //
    if (0 != (barVal[0] & PCI_BAR_TYPE_IO_BIT))
    {
        K2MEM_Zero(&resDef, sizeof(resDef));
        resDef.mType = K2OS_RESTYPE_IO;
        resDef.mId = 0;
        resDef.Io.Range.mBasePort = barVal[0] & PCI_BAR_BASEMASK_IO;
        resDef.Io.Range.mSizeBytes = 8;
        stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
    }
    else
    {
        K2MEM_Zero(&resDef, sizeof(resDef));
        resDef.mType = K2OS_RESTYPE_PHYS;
        resDef.mId = 0;
        resDef.Phys.Range.mBaseAddr = (UINT32)(barVal[0] & PCI_BAR_BASEMASK_MEMORY);
        resDef.Phys.Range.mSizeBytes = 8;
        stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add IDE primary channel io BAR range0 (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
    }
    else
    {
        if (0 != (barVal[1] & PCI_BAR_TYPE_IO_BIT))
        {
            K2MEM_Zero(&resDef, sizeof(resDef));
            resDef.mType = K2OS_RESTYPE_IO;
            resDef.mId = 1;
            resDef.Io.Range.mBasePort = barVal[1] & PCI_BAR_BASEMASK_IO;
            resDef.Io.Range.mSizeBytes = 4;
            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
        }
        else
        {
            K2MEM_Zero(&resDef, sizeof(resDef));
            resDef.mType = K2OS_RESTYPE_PHYS;
            resDef.mId = 1;
            resDef.Phys.Range.mBaseAddr = (UINT32)(barVal[1] & PCI_BAR_BASEMASK_MEMORY);
            resDef.Phys.Range.mSizeBytes = 4;
            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
        }
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add IDE primary channel mem BAR range1 (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
        }
    }
    if (0xFF != irqVal[0])
    {
        K2MEM_Zero(&resDef, sizeof(resDef));
        resDef.mType = K2OS_RESTYPE_IRQ;
        resDef.mId = 0;
        resDef.Irq.Config.mSourceIrq = irqVal[0];
        resDef.Irq.Config.Line.mIsActiveLow = TRUE;
        stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add IDE primary channel irq (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
        }
    }

    //
    // populate access to secondary channel and its irq
    //
    if (0 != (barVal[2] & PCI_BAR_TYPE_IO_BIT))
    {
        K2MEM_Zero(&resDef, sizeof(resDef));
        resDef.mType = K2OS_RESTYPE_IO;
        resDef.mId = 2;
        resDef.Io.Range.mBasePort = barVal[2] & PCI_BAR_BASEMASK_IO;
        resDef.Io.Range.mSizeBytes = 8;
        stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
    }
    else
    {
        K2MEM_Zero(&resDef, sizeof(resDef));
        resDef.mType = K2OS_RESTYPE_PHYS;
        resDef.mId = 2;
        resDef.Phys.Range.mBaseAddr = (UINT32)(barVal[2] & PCI_BAR_BASEMASK_MEMORY);
        resDef.Phys.Range.mSizeBytes = 8;
        stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add IDE secondary channel io BAR range0 (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
    }
    else
    {
        if (0 != (barVal[3] & PCI_BAR_TYPE_IO_BIT))
        {
            K2MEM_Zero(&resDef, sizeof(resDef));
            resDef.mType = K2OS_RESTYPE_IO;
            resDef.mId = 3;
            resDef.Io.Range.mBasePort = barVal[3] & PCI_BAR_BASEMASK_IO;
            resDef.Io.Range.mSizeBytes = 4;
            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
        }
        else
        {
            K2MEM_Zero(&resDef, sizeof(resDef));
            resDef.mType = K2OS_RESTYPE_PHYS;
            resDef.mId = 3;
            resDef.Phys.Range.mBaseAddr = (UINT32)(barVal[3] & PCI_BAR_BASEMASK_MEMORY);
            resDef.Phys.Range.mSizeBytes = 4;
            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
        }
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add IDE secondary channel mem BAR range1 (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
        }
    }
    if (0xFF != irqVal[1])
    {
        K2MEM_Zero(&resDef, sizeof(resDef));
        resDef.mType = K2OS_RESTYPE_IRQ;
        resDef.mId = 1;
        resDef.Irq.Config.mSourceIrq = irqVal[1];
        resDef.Irq.Config.Line.mIsActiveLow = TRUE;
        stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add IDE secondary channel irq (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
        }
    }

    //
    // add bus master address if it is indicated
    //
    if (0 != barVal[4])
    {
        //
        // bus master bar set.
        //
        if (0 != (barVal[4] & PCI_BAR_TYPE_IO_BIT))
        {
            K2MEM_Zero(&resDef, sizeof(resDef));
            resDef.mType = K2OS_RESTYPE_IO;
            resDef.mId = 4;
            resDef.Io.Range.mBasePort = barVal[4] & PCI_BAR_BASEMASK_IO;
            resDef.Io.Range.mSizeBytes = 16;
            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
        }
        else
        {
            K2MEM_Zero(&resDef, sizeof(resDef));
            resDef.mType = K2OS_RESTYPE_PHYS;
            resDef.mId = 4;
            resDef.Phys.Range.mBaseAddr = (UINT32)(barVal[4] & PCI_BAR_BASEMASK_MEMORY);
            resDef.Phys.Range.mSizeBytes = 16;
            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
        }
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add bus master range for IDE (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, stat);
        }
    }

    return TRUE;
}

BOOL
PciBus_CheckOverrideRes(
    PCIBUS_CHILD *  apChild
)
{
//    K2OSKERN_Debug("CLASS %d, SUBCLASS %d\n", apChild->Ident.mClassCode, apChild->Ident.mSubClassCode);
    if ((apChild->Ident.mClassCode == PCI_CLASS_STORAGE) &&
        (apChild->Ident.mSubClassCode == PCI_STORAGE_SUBCLASS_IDE))
    {
        return PciBus_CheckOverrideIDE(apChild);
    }

    return FALSE;
}
