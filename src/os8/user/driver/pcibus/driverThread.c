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

UINT16                  gPciSegNum;
UINT16                  gPciBusNum;
BOOL                    gUseECAM;
K2OS_PAGEARRAY_TOKEN    gTokBusPageArray;
K2OSDRV_RES_IO          gBusIo;
K2OS_CRITSEC            gPCIBUS_ChildTreeSec;
K2TREE_ANCHOR           gPCIBUS_ChildTree;
UINT32                  gECAMBaseAddr;

K2STAT PCIBUS_ChildObject_Create(K2OSRPC_OBJECT_CREATE const *apCreate, UINT_PTR *apRetCreator);
K2STAT PCIBUS_ChildObject_Call(K2OSRPC_OBJECT_CALL const *apCall, UINT_PTR *apRetUsedOutBytes);
K2STAT PCIBUS_ChildObject_Delete(K2OSRPC_OBJECT_DELETE const *apDelete);

K2OSRPC_OBJECT_CLASS const gPciBusChild_ObjectClass =
{
    K2OSDRV_CHILDOBJECT_CLASS_ID,
    FALSE,
    PCIBUS_ChildObject_Create,
    PCIBUS_ChildObject_Call,
    PCIBUS_ChildObject_Delete
};

K2STAT 
PCIBUS_ChildObject_Create(
    K2OSRPC_OBJECT_CREATE const *   apCreate,
    UINT_PTR *                      apRetCreatorRef
)
{
    if (apCreate->mCallerProcessId != K2OS_Process_GetId())
    {
        //
        // only inproc creation is allowed
        //
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    *apRetCreatorRef = apCreate->mCallerContext;

    return K2STAT_NO_ERROR;
}

K2STAT 
PCIBUS_ChildObject_Call(
    K2OSRPC_OBJECT_CALL const * apCall, 
    UINT_PTR *                  apRetUsedOutBytes
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
PCIBUS_ChildObject_Delete(
    K2OSRPC_OBJECT_DELETE const *apDelete
)
{
    PCIBUS_CHILD *  pChild;

    pChild = (PCIBUS_CHILD *)apDelete->mCreatorRef;

    K2OS_CritSec_Enter(&gPCIBUS_ChildTreeSec);
    K2TREE_Remove(&gPCIBUS_ChildTree, &pChild->ChildTreeNode);
    K2OS_CritSec_Leave(&gPCIBUS_ChildTreeSec);

    if (gUseECAM)
    {
        K2OS_Token_Destroy(pChild->ECAM.mMapToken);
        pChild->ECAM.mMapToken = NULL;
        pChild->ECAM.mpCfg = NULL;
    }

    return K2STAT_NO_ERROR;
}

void
PCIBUS_MountChild(
    PCIBUS_CHILD *  apChild
)
{
    K2STAT          stat;
    UINT64          address;
    UINT64          val64;
    UINT_PTR        numBars;
    UINT_PTR        ix;
    UINT16          cmdReg;
    UINT32          barSave;
    UINT32          maskVal;
    K2OS_IRQ_CONFIG irqConfig;

    address = (((UINT64)(apChild->DeviceLoc.Device)) << 16) | ((UINT64)(apChild->DeviceLoc.Function));
    stat = K2OSDrv_MountChildDevice(apChild->mObjectId, K2OSDRV_BUSTYPE_PCI, &address, &apChild->Ident, &apChild->mSystemDeviceInstanceId);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to mount child device object %d (%08X)\n", gPciSegNum, gPciBusNum, apChild->mObjectId, stat);
        return;
    }

    if (!PCIBUS_CheckOverrideRes(apChild))
    {
        //
        // get header type
        //
        stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_HEADERTYPE, &val64, 8);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to read child %04X/%04X header type from config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
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
            stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_COMMAND, &val64, 16);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to read child %04X/%04X command reg from config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                return;
            }
            cmdReg = ((UINT32)val64);
            val64 &= ~7;
            stat = PCIBUS_ConfigWrite(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_COMMAND, &val64, 16);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to write child %04X/%04X command reg in config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                return;
            }

            for (ix = 0; ix < numBars; ix++)
            {
                stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * ix), &val64, 32);
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to read child %04X/%04X bar reg %d from config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, ix, stat);
                    break;
                }
                apChild->mBarVal[ix] = barSave = (UINT32)val64;

                if (barSave & PCI_BAR_TYPE_IO_BIT)
                    maskVal = PCI_BAR_BASEMASK_IO;
                else
                    maskVal = PCI_BAR_BASEMASK_MEMORY;

                val64 = maskVal | (barSave & ~maskVal);
                stat = PCIBUS_ConfigWrite(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * ix), &val64, 32);
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to write child %04X/%04X bar reg %d in config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, ix, stat);
                    break;
                }
                stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * ix), &val64, 32);
                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to read child %04X/%04X bar reg %d from config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, ix, stat);
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
                    K2OSDrv_DebugPrintf("  Bar[%d] = %s %08X for %08X\n", ix, (barSave & PCI_BAR_TYPE_IO_BIT) ? "IO" : "MEM", barSave & maskVal, apChild->mBarSize[ix]);

                    //
                    // restore the bar register
                    //
                    val64 = barSave;
                    stat = PCIBUS_ConfigWrite(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * ix), &val64, 32);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to write child %04X/%04X bar reg %d in config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, ix, stat);
                        break;
                    }
                }
            }

            //
            // restore command reg
            //
            val64 = cmdReg;
            stat = PCIBUS_ConfigWrite(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_COMMAND, &val64, 16);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to write child %04X/%04X command reg in config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
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
                            stat = K2OSDRV_AddChildIo(apChild->mObjectId, apChild->mBarVal[ix] & PCI_BAR_BASEMASK_IO, apChild->mBarSize[ix]);
                            if (K2STAT_IS_ERROR(stat))
                            {
                                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add child %04X/%04X io range (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                                break;
                            }
                        }
                        else
                        {
                            address = apChild->mBarVal[ix] & PCI_BAR_BASEMASK_MEMORY;
                            val64 = apChild->mBarSize[ix];
                            stat = K2OSDRV_AddChildPhys(apChild->mObjectId, &address, &val64);
                            if (K2STAT_IS_ERROR(stat))
                            {
                                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add child %04X/%04X memmap range (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
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
        stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_INTLINE, &val64, 8);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to read child %04X/%04X interrupt line from config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
            return;
        }

        if (val64 != 0xFF)
        {
            K2MEM_Zero(&irqConfig, sizeof(irqConfig));
            irqConfig.mSourceIrq = (UINT8)val64;
            irqConfig.Line.mIsActiveLow = TRUE;
            stat = K2OSDRV_AddChildIrq(apChild->mObjectId, &irqConfig);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to add child %04X/%04X irq (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, stat);
                return;
            }
        }
    }

    apChild->mResOk = TRUE;
}

UINT_PTR 
DriverThread(
    UINT_PTR aBusDriverProcessId,
    UINT_PTR aBusDriverChildObjectId
)
{
    K2STAT                      stat;
    K2OSDRV_ENUM_RES_OUT        resEnum;
    K2OSDRV_RES_PHYS            resPhys;
    UINT_PTR                    resIx;
    PCIBUS_CHILD *              pChild;
    K2TREE_NODE *               pTreeNode;

    K2OSDrv_DebugPrintf("PciBus(proc %d), bus driver proc %d, child object %d\n", K2OS_Process_GetId(), aBusDriverProcessId, aBusDriverChildObjectId);

    //
    // acpi method must exist and return the bus number
    //
    resIx = 0;
    stat = K2OSDRV_RunAcpiMethod("_BBN", 0, 0, &resIx);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** PciBus: RunAcpi(BBN) failed (0x%08X)\n", stat);
        return K2STAT_ERROR_NOT_EXIST;
    }
    gPciBusNum = (UINT16)resIx;

    resIx = 0;
    stat = K2OSDRV_RunAcpiMethod("_SEG", 0, 0, &resIx);
    if (K2STAT_IS_ERROR(stat))
    {
        gPciSegNum = 0;
    }
    else
    {
        gPciSegNum = (UINT16)resIx;
    }

    K2MEM_Zero(&resEnum, sizeof(resEnum));
    stat = K2OSDrv_EnumRes(&resEnum);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** PciBus: Enum resources failed (0x%08X)\n", stat);
        return stat;
    }

    gUseECAM = FALSE;

    if (resEnum.mCountPhys > 0)
    {
        //
        // find the ECAM range by matching size
        //
        for (resIx = 0; resIx < resEnum.mCountPhys; resIx++)
        {
            K2MEM_Zero(&resPhys, sizeof(resPhys));

            stat = K2OSDrv_GetResPhys(resIx, &resPhys);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Could not get phys resource 0 (0x%08X)\n", gPciSegNum, gPciBusNum, stat);
                return stat;
            }

            if (resPhys.Range.mSizeBytes == 0x100000)
                break;

            K2OS_Token_Destroy(resPhys.mPageArrayToken);
        }

        if (resIx == resEnum.mCountPhys)
        {
            K2OSDrv_DebugPrintf("!!! PciBus(%d,%d): Phys resource(s) found but they are not the correct size for a pci bus\n", gPciSegNum, gPciBusNum);
        }
        else
        {
            gTokBusPageArray = resPhys.mPageArrayToken;
            gECAMBaseAddr = resPhys.Range.mBaseAddr;
            gUseECAM = TRUE;
            K2OSDrv_DebugPrintf("PciBus(%d,%d): Using ECAM at %08X\n", gPciSegNum, gPciBusNum, gECAMBaseAddr);
        }
    }

    if (!gUseECAM)
    {
#if K2_TARGET_ARCH_IS_INTEL
        if (resEnum.mCountIo > 0)
        {
            for (resIx = 0; resIx < resEnum.mCountIo; resIx++)
            {
                K2MEM_Zero(&gBusIo, sizeof(gBusIo));
                stat = K2OSDrv_GetResIo(0, &gBusIo);
                if (!K2STAT_IS_ERROR(stat))
                {
                    if (gBusIo.mSizeBytes == 8)
                        break;
                }
            }
            if (resIx == resEnum.mCountIo)
            {
                resEnum.mCountIo = 0;
            }
            else
            {
                K2OSDrv_DebugPrintf("PciBus(%d,%d): Using IO Bus (%04X,%04X)\n", gPciSegNum, gPciBusNum, gBusIo.mBasePort, gBusIo.mSizeBytes);
            }
        }
        if (0 == resEnum.mCountIo)
        {
            K2OSDrv_DebugPrintf("*** PciBus(%d,%d): no usable phys or io resources\n", gPciSegNum, gPciBusNum);
            return K2STAT_ERROR_NOT_EXIST;
        }
#else
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): no usable ECAM range found\n", gPciSegNum, gPciBusNum);
        return K2STAT_ERROR_NOT_EXIST;
#endif
    }

    if (!K2OS_CritSec_Init(&gPCIBUS_ChildTreeSec))
    {
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): critsec init failed (0x%08X)\n", gPciSegNum, gPciBusNum, K2OS_Thread_GetLastStatus());
        return K2OS_Thread_GetLastStatus();
    }

    K2TREE_Init(&gPCIBUS_ChildTree, NULL);

    //
    // publish the rpc object class for bus children
    //
    if (!K2OSRPC_ObjectClass_Publish(&gPciBusChild_ObjectClass, 0))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to publish RPC bus child object class (%08X)\n", gPciSegNum, gPciBusNum, stat);
        return stat;
    }

    PCIBUS_InitAndDiscover();

    if (0 == gPCIBUS_ChildTree.mNodeCount)
    {
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): No children found\n", gPciSegNum, gPciBusNum);
        return 0;
    }

    //
    // mount the bus children
    //
    pTreeNode = K2TREE_FirstNode(&gPCIBUS_ChildTree);
    do {
        pChild = K2_GET_CONTAINER(PCIBUS_CHILD, pTreeNode, ChildTreeNode);
        pTreeNode = K2TREE_NextNode(&gPCIBUS_ChildTree, pTreeNode);
        PCIBUS_MountChild(pChild);
    } while (NULL != pTreeNode);   // only do first one for now

    //
    // finally enable
    //
    K2OSDrv_SetEnable(TRUE);

    do {
        K2OS_Thread_Sleep(1000);
        K2OSDrv_DebugPrintf("PciBus_Tick()\n");
    } while (1);

    return 0;
}
