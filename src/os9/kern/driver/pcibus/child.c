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
    UINT32          mountFlags;
    
    pPciBus = apChild->mpParent;

    address = (((UINT64)(apChild->DeviceLoc.Device)) << 16) | ((UINT64)(apChild->DeviceLoc.Function));

    mountFlags = 0;
    if (apChild->Ident.mClassCode == PCI_CLASS_BRIDGE)
    {
        switch (apChild->Ident.mSubClassCode)
        {
        case PCI_BRIDGE_SUBCLASS_ISA:
            mountFlags = K2OS_BUSTYPE_ISA;
            break;
        case PCI_BRIDGE_SUBCLASS_EISA:
            mountFlags = K2OS_BUSTYPE_EISA;
            break;
        case PCI_BRIDGE_SUBCLASS_PCI:
            mountFlags = K2OS_BUSTYPE_PCI;
            break;
        case PCI_BRIDGE_SUBCLASS_PCMCIA:
            mountFlags = K2OS_BUSTYPE_PCMCIA;
            break;
        case PCI_BRIDGE_SUBCLASS_CARDBUS:
            mountFlags = K2OS_BUSTYPE_CARDBUS;
            break;
        default:
            mountFlags = K2OS_BUSTYPE_HOST;
            break;
        }
    }
    else
    {
        mountFlags = K2OS_BUSTYPE_HOST;
    }

    stat = K2OSDDK_MountChild(pPciBus->mDevCtx, mountFlags, &address, &apChild->Ident, &apChild->mInstanceId);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to mount child device object %d (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->mInstanceId, stat);
        return;
    }

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
                            resDef.mType = K2OS_RESTYPE_IO;
                            resDef.mId = ix;
                            resDef.Io.Range.mBasePort = (UINT16)(apChild->mBarVal[ix] & PCI_BAR_BASEMASK_IO);
                            resDef.Io.Range.mSizeBytes = apChild->mBarSize[ix];
                            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->mInstanceId, &resDef);
                            if (K2STAT_IS_ERROR(stat))
                            {
                                K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add child %04X/%04X io range (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                                break;
                            }
                        }
                        else
                        {
                            K2MEM_Zero(&resDef, sizeof(resDef));
                            resDef.mType = K2OS_RESTYPE_PHYS;
                            resDef.mId = ix;
                            resDef.Phys.Range.mBaseAddr = apChild->mBarVal[ix] & PCI_BAR_BASEMASK_MEMORY;
                            resDef.Phys.Range.mSizeBytes = apChild->mBarSize[ix];
                            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->mInstanceId, &resDef);
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
            K2MEM_Zero(&resDef, sizeof(resDef));
            resDef.mType = K2OS_RESTYPE_IRQ;
            resDef.mId = 0;
            resDef.Irq.Config.Line.mIsActiveLow = TRUE;
            resDef.Irq.Config.mSourceIrq = (UINT8)val64;
            stat = K2OSDDK_AddChildRes(pPciBus->mDevCtx, apChild->mInstanceId, &resDef);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Failed to add child %04X/%04X irq (%08X)\n", pPciBus->mSegNum, pPciBus->mSegNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                return;
            }
        }
    }

    apChild->mResOk = TRUE;
}
