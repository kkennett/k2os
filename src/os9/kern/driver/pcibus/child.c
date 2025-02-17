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

UINT32 
PciBus_FindIrqLine(
    PCIBUS *    pPciBus,
    UINT32      aDevice,
    UINT8       aPin
)
{
    ACPI_PCI_ROUTING_ENTRY *pEntry;

    pEntry = pPciBus->mpRoutingTable;
    if (NULL == pEntry)
        return 0xFFFFFFFF;
    if (pEntry->Length == 0)
        return 0xFFFFFFFF;
    if ((aPin == 0) || (aPin > 4))  // entry pins go 0-3, pci cfg space pin #s go 1-4
        return 0xFFFFFFFF;
    do { 
        if (((UINT32)(pEntry->Address >> 16)) == aDevice)
        {
            if (pEntry->Pin == (aPin - 1))  // entry pins go 0-3, pci cfg space pin #s go 1-4
            {
                if (pEntry->Source[0] == 0)
                {
                    return pEntry->SourceIndex;
                }
                else
                {
                    // redirect to routing object - TBD
                    K2OSKERN_Debug("%02X [%s]\n", pEntry->Source[0], pEntry->Source);
                    K2_ASSERT(0);
                }
            }
        }
        pEntry = (ACPI_PCI_ROUTING_ENTRY *)(((UINT8 *)pEntry) + pEntry->Length);
    } while (pEntry->Length != 0);
    return 0xFFFFFFFF;
}

void
PciBus_MountChild(
    PCIBUS_CHILD *  apChild
)
{
    K2STAT          stat;
    UINT64          address;
    UINT64          val64;
    UINT32          numBars;
    UINT32          ix;
    UINT16          cmdReg;
    UINT32          barSave;
    UINT32          maskVal;
    PCIBUS *        pPciBus;
    K2OSDDK_RESDEF  resDef;
    UINT32          irqLine;
    
    pPciBus = apChild->mpParent;

    address = (((UINT64)(apChild->DeviceLoc.Device)) << 16) | ((UINT64)(apChild->DeviceLoc.Function));

    stat = K2OSDDK_MountChild(pPciBus->mDevCtx, K2OS_BUSTYPE_PCI, &address, &apChild->Ident, pPciBus->mRpcIfInstId, &apChild->ChildIdTreeNode.mUserVal);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to mount child device object (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, stat);
        return;
    }

    K2TREE_Insert(&pPciBus->ChildIdTree, apChild->ChildIdTreeNode.mUserVal, &apChild->ChildIdTreeNode);

    if (!PciBus_CheckOverrideRes(apChild))
    {
        //
        // get header type
        //
        stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_HEADERTYPE, &val64, 8);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): Failed to read child %04X/%04X header type from config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
            return;
        }

        if (val64 < 2)
        {
            //
            // discover bars
            //
            if (val64 == 0)
                numBars = 6;    // type 0
            else
                numBars = 2;    // type 1

            //
            // save and disable memory, io, bus master capabilities while we mess with the BARs.
            //
            stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_COMMAND, &val64, 16);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Failed to read child %04X/%04X command reg from config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                return;
            }
            cmdReg = ((UINT32)val64);
            val64 &= ~7;
            stat = pPciBus->mfConfigWrite(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_COMMAND, &val64, 16);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Failed to write child %04X/%04X command reg in config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                return;
            }

            for (ix = 0; ix < numBars; ix++)
            {
                stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * ix), &val64, 32);
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSKERN_Debug("*** PciBus(%d,%d): Failed to read child %04X/%04X bar reg %d from config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, ix, stat);
                    break;
                }
                apChild->mBarVal[ix] = barSave = (UINT32)val64;

                if (barSave & PCI_BAR_TYPE_IO_BIT)
                    maskVal = PCI_BAR_BASEMASK_IO;
                else
                    maskVal = PCI_BAR_BASEMASK_MEMORY;

                val64 = maskVal | (barSave & ~maskVal);
                stat = pPciBus->mfConfigWrite(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * ix), &val64, 32);
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSKERN_Debug("*** PciBus(%d,%d): Failed to write child %04X/%04X bar reg %d in config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, ix, stat);
                    break;
                }
                stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * ix), &val64, 32);
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSKERN_Debug("*** PciBus(%d,%d): Failed to read child %04X/%04X bar reg %d from config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, ix, stat);
                    break;
                }
                val64 &= maskVal;

                if (0 != val64)
                {
                    //
                    // we found a bar
                    //
                    apChild->mBarsFoundCount++;
                    apChild->mBarSize[ix] = (UINT32)((~val64) + 1);
//                    K2OSKERN_Debug("  Bar[%d] = %s %08X for %08X\n", ix, (barSave & PCI_BAR_TYPE_IO_BIT) ? "IO" : "MEM", barSave & maskVal, apChild->mBarSize[ix]);

                    //
                    // restore the bar register
                    //
                    val64 = barSave;
                    stat = pPciBus->mfConfigWrite(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * ix), &val64, 32);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to write child %04X/%04X bar reg %d in config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, ix, stat);
                        break;
                    }
                }
            }

            //
            // restore command reg
            //
            val64 = cmdReg;
            stat = pPciBus->mfConfigWrite(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_COMMAND, &val64, 16);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Failed to write child %04X/%04X command reg in config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                return;
            }

            if (apChild->mBarsFoundCount)
            {
                for (ix = 0; ix < numBars; ix++)
                {
                    if (0 != apChild->mBarSize[ix])
                    {
                        if (0 != (apChild->mBarVal[ix] & PCI_BAR_TYPE_IO_BIT))
                        {
                            K2MEM_Zero(&resDef, sizeof(resDef));
                            resDef.mResType = K2OS_RESTYPE_IO;
                            resDef.mId = ix;
                            resDef.Io.Range.mBasePort = (UINT16)(apChild->mBarVal[ix] & PCI_BAR_BASEMASK_IO);
                            resDef.Io.Range.mSizeBytes = apChild->mBarSize[ix];
                            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
                            if (K2STAT_IS_ERROR(stat))
                            {
                                K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add child %04X/%04X io range (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                                break;
                            }
                        }
                        else
                        {
                            K2MEM_Zero(&resDef, sizeof(resDef));
                            resDef.mResType = K2OS_RESTYPE_PHYS;
                            resDef.mId = ix;
                            resDef.Phys.Range.mBaseAddr = apChild->mBarVal[ix] & PCI_BAR_BASEMASK_MEMORY;
                            resDef.Phys.Range.mSizeBytes = apChild->mBarSize[ix];
                            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
                            if (K2STAT_IS_ERROR(stat))
                            {
                                K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add child %04X/%04X memmap range (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (K2STAT_IS_ERROR(stat))
            return;

        //
        // add any IRQ now
        //
        stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_INTLINE, &val64, 8);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): Failed to read child %04X/%04X interrupt line from config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
            return;
        }
        if (val64 != 0xFF)
        {
            stat = pPciBus->mfConfigRead(pPciBus, &apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_INTPIN, &val64, 8);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Failed to read child %04X/%04X interrupt ping from config space (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
            }
            else
            {
                irqLine = PciBus_FindIrqLine(pPciBus, apChild->DeviceLoc.Device, (UINT8)val64);
                if (irqLine != 0xFFFFFFFF)
                {
                    K2MEM_Zero(&resDef, sizeof(resDef));
                    resDef.mResType = K2OS_RESTYPE_IRQ;
                    resDef.mId = 0;
                    resDef.Irq.Config.Line.mIsActiveLow = TRUE;
                    resDef.Irq.Config.mSourceIrq = (UINT16)irqLine;
                    stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->ChildIdTreeNode.mUserVal, &resDef);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add child %04X/%04X irq (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                        return;
                    }
                }
            }
        }
    }

    apChild->mResOk = TRUE;
}
