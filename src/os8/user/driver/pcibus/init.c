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

void PCIBUS_DiscoverECAM(void);

#if K2_TARGET_ARCH_IS_INTEL

void PCIBUS_DiscoverIo(void);

#include <lib/k2archx32.h>

#define MAKE_PCICONFIG_ADDR(bus,device,function,reg) \
    (0x80000000ul | \
    ((((UINT32)bus)&0xFFul) << 16) | \
    ((((UINT32)device)&0x1Ful) << 11) | \
    ((((UINT32)function)&0x7ul) << 8) | \
    ((((UINT32)reg)&0xFC)))

PCIBUS_pf_ConfigRead     PCIBUS_ConfigRead;
PCIBUS_pf_ConfigWrite    PCIBUS_ConfigWrite;

K2STAT 
PCIBUS_Io_ConfigRead(
    K2PCI_DEVICE_LOC *  apLoc,
    UINT32              aReg,
    UINT64 *            apRetValue,
    UINT32              aWidth
)
{
    UINT32 addr;
    UINT32 val32;

    K2_ASSERT(!gUseECAM);
    K2_ASSERT(apLoc->Segment == gPciSegNum);
    K2_ASSERT(apLoc->Bus == gPciBusNum);
    K2_ASSERT(apLoc->Device < 32);
    K2_ASSERT(apLoc->Function < 8);

    if ((0 != (aWidth & 7)) ||
        ((aWidth >> 3) > 4))
        return K2STAT_ERROR_BAD_SIZE;

    if (aReg & 1)
    {
        if (aWidth != 8)
            return K2STAT_ERROR_BAD_ALIGNMENT;
    }
    else if ((aReg & 3) == 2)
    {
        if (aWidth > 16)
            return K2STAT_ERROR_BAD_ALIGNMENT;
    }

    addr = MAKE_PCICONFIG_ADDR(gPciBusNum, apLoc->Device, apLoc->Function, aReg);
    X32_IoWrite32(addr, gBusIo.mBasePort);
    val32 = X32_IoRead32(gBusIo.mBasePort + 4) >> ((aReg & 3) * 8);

    switch (aWidth)
    {
    case 8:
        *apRetValue = (UINT64)(val32 & 0xFF);
        break;
    case 16:
        *apRetValue = (UINT64)(val32 & 0xFFFF);
        break;
    case 32:
        *apRetValue = (UINT64)val32;
        break;
    default:
        *apRetValue = 0;
        return K2STAT_ERROR_BAD_ALIGNMENT;
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
PCIBUS_Io_ConfigWrite(
    K2PCI_DEVICE_LOC *  apLoc,
    UINT32              aReg,
    UINT64 const *      apValue,
    UINT32              aWidth
)
{
    UINT32 addr;
    UINT32 val32;
    UINT32 mask32;

    K2_ASSERT(!gUseECAM);
    K2_ASSERT(apLoc->Segment == gPciSegNum);
    K2_ASSERT(apLoc->Bus == gPciBusNum);
    K2_ASSERT(apLoc->Device < 32);
    K2_ASSERT(apLoc->Function < 8);

    if ((0 == aWidth) ||
        (0 != (aWidth & 7)) ||
        (4 < (aWidth >> 3)))
    {
        return K2STAT_ERROR_BAD_SIZE;
    }

    if (aReg & 1)
    {
        if (aWidth != 8)
            return K2STAT_ERROR_BAD_ALIGNMENT;
    }
    else if ((aReg & 3) == 2)
    {
        if (aWidth > 16)
            return K2STAT_ERROR_BAD_ALIGNMENT;
    }

    switch (aWidth)
    {
    case 8:
        val32 = ((UINT32)(*apValue)) & 0xFF;
        mask32 = 0xFF;
        break;
    case 16:
        val32 = ((UINT32)(*apValue)) & 0xFFFF;
        mask32 = 0xFFFF;
        break;
    case 32:
        val32 = ((UINT32)(*apValue));
        mask32 = 0xFFFFFFFF;
        break;
    default:
        return K2STAT_ERROR_BAD_ALIGNMENT;
    }

    val32 <<= ((aReg & 3) * 8);
    mask32 <<= ((aReg & 3) * 8);

    addr = MAKE_PCICONFIG_ADDR(gPciBusNum, apLoc->Device, apLoc->Function, aReg);
    X32_IoWrite32(addr, gBusIo.mBasePort);
    X32_IoWrite32(((X32_IoRead32(gBusIo.mBasePort + 4) & ~mask32) | val32), gBusIo.mBasePort + 4);

    return K2STAT_NO_ERROR;
}

K2STAT PCIBUS_ECAM_ConfigRead(K2PCI_DEVICE_LOC *apLoc, UINT32 aReg, UINT64 *apRetValue, UINT32 aWidth);
K2STAT PCIBUS_ECAM_ConfigWrite(K2PCI_DEVICE_LOC *apLoc, UINT32 aReg, UINT64 const *apValue, UINT32 aWidth);

void
PCIBUS_InitAndDiscover(
    void
)
{
    if (gUseECAM)
    {
        PCIBUS_DiscoverECAM();
        PCIBUS_ConfigRead = PCIBUS_ECAM_ConfigRead;
        PCIBUS_ConfigWrite = PCIBUS_ECAM_ConfigWrite;
    }
    else
    {
        PCIBUS_ConfigRead = PCIBUS_Io_ConfigRead;
        PCIBUS_ConfigWrite = PCIBUS_Io_ConfigWrite;
        PCIBUS_DiscoverIo();
    }
}

#else

void
PCIBUS_ConfigInit(
    void
)
{
    K2_ASSERT(gUseECAM);
    PCIBUS_DiscoverECAM();
}

#endif

K2STAT
#if K2_TARGET_ARCH_IS_INTEL
PCIBUS_ECAM_ConfigRead(
#else
PCIBUS_ConfigRead(
#endif
    K2PCI_DEVICE_LOC *  apLoc,
    UINT32              aReg,
    UINT64 *            apRetValue,
    UINT32              aWidth
)
{
    K2STAT              stat;
    K2TREE_NODE *       pTreeNode;
    UINT32 volatile *   pu;
    PCIBUS_CHILD *      pChild;
    UINT32              val32;

    K2_ASSERT(gUseECAM);
    K2_ASSERT(apLoc->Segment == gPciSegNum);
    K2_ASSERT(apLoc->Bus == gPciBusNum);
    K2_ASSERT(apLoc->Device < 32);
    K2_ASSERT(apLoc->Function < 8);

    K2OS_CritSec_Enter(&gPCIBUS_ChildTreeSec);

    do {
        pTreeNode = K2TREE_Find(&gPCIBUS_ChildTree, (((UINT32)apLoc->Device) << 16) | ((UINT32)apLoc->Function));
        if (NULL == pTreeNode)
        {
            stat = K2STAT_ERROR_NOT_FOUND;
            break;
        }

        if ((0 != (aWidth & 7)) ||
            ((aWidth >> 3) > 4))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
            break;
        }

        if (aReg & 1)
        {
            if (aWidth != 8)
            {
                stat = K2STAT_ERROR_BAD_ALIGNMENT;
                break;
            }
        }
        else if ((aReg & 3) == 2)
        {
            if (aWidth > 16)
            {
                stat = K2STAT_ERROR_BAD_ALIGNMENT;
                break;
            }
        }

        pChild = K2_GET_CONTAINER(PCIBUS_CHILD, pTreeNode, ChildTreeNode);
        K2_ASSERT(NULL != pChild->ECAM.mpCfg);

        pu = &pChild->ECAM.mpCfg->mAsUINT32[(aReg & 0xFC) >> 2];

        val32 = (*pu) >> ((aReg & 3) * 8);

        switch (aWidth)
        {
        case 8:
            *apRetValue = (UINT64)(val32 & 0xFF);
            break;
        case 16:
            *apRetValue = (UINT64)(val32 & 0xFFFF);
            break;
        case 32:
            *apRetValue = (UINT64)val32;
            break;
        default:
            *apRetValue = 0;
            return K2STAT_ERROR_BAD_ALIGNMENT;
        }

        stat = K2STAT_NO_ERROR;

    } while (0);

    K2OS_CritSec_Leave(&gPCIBUS_ChildTreeSec);

    return stat;
}

K2STAT
#if K2_TARGET_ARCH_IS_INTEL
PCIBUS_ECAM_ConfigWrite(
#else
PCIBUS_ConfigWrite(
#endif
    K2PCI_DEVICE_LOC *  apLoc,
    UINT32              aReg,
    UINT64 const *      apValue,
    UINT32              aWidth
)
{
    K2STAT              stat;
    K2TREE_NODE *       pTreeNode;
    UINT32 volatile *   pu;
    PCIBUS_CHILD *      pChild;
    UINT32              val32;
    UINT32              mask32;

    K2_ASSERT(gUseECAM);
    K2_ASSERT(apLoc->Segment == gPciSegNum);
    K2_ASSERT(apLoc->Bus == gPciBusNum);
    K2_ASSERT(apLoc->Device < 32);
    K2_ASSERT(apLoc->Function < 8);

    K2OS_CritSec_Enter(&gPCIBUS_ChildTreeSec);

    do {
        pTreeNode = K2TREE_Find(&gPCIBUS_ChildTree, (((UINT32)apLoc->Device) << 16) | ((UINT32)apLoc->Function));
        if (NULL == pTreeNode)
        {
            stat = K2STAT_ERROR_NOT_FOUND;
            break;
        }

        if ((0 != (aWidth & 7)) ||
            ((aWidth >> 3) > 4))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
            break;
        }

        if (aReg & 1)
        {
            if (aWidth != 8)
            {
                stat = K2STAT_ERROR_BAD_ALIGNMENT;
                break;
            }
        }
        else if ((aReg & 3) == 2)
        {
            if (aWidth > 16)
            {
                stat = K2STAT_ERROR_BAD_ALIGNMENT;
                break;
            }
        }

        pChild = K2_GET_CONTAINER(PCIBUS_CHILD, pTreeNode, ChildTreeNode);
        K2_ASSERT(NULL != pChild->ECAM.mpCfg);

        switch (aWidth)
        {
        case 8:
            val32 = ((UINT32)(*apValue)) & 0xFF;
            mask32 = 0xFF;
            break;
        case 16:
            val32 = ((UINT32)(*apValue)) & 0xFFFF;
            mask32 = 0xFFFF;
            break;
        case 32:
            val32 = ((UINT32)(*apValue));
            mask32 = 0xFFFFFFFF;
            break;
        default:
            return K2STAT_ERROR_BAD_ALIGNMENT;
        }

        val32 <<= ((aReg & 3) * 8);
        mask32 <<= ((aReg & 3) * 8);

        pu = &pChild->ECAM.mpCfg->mAsUINT32[(aReg & 0xFC) >> 2];

        *pu = (((*pu) & ~mask32) | val32);

        stat = K2STAT_NO_ERROR;

    } while (0);

    K2OS_CritSec_Leave(&gPCIBUS_ChildTreeSec);

    return stat;
}

#if K2_TARGET_ARCH_IS_INTEL

void
PCIBUS_DiscoverIo(
    void
)
{
    K2STAT              stat;
    UINT_PTR            ixDev;
    UINT_PTR            ixFunc;
    UINT64              val;
    K2PCI_DEVICE_LOC    loc;
    UINT16              vendorId;
    UINT16              deviceId;
    PCIBUS_CHILD *      pChild;

    //
    // scan the bus for child devices and create them
    //
    loc.Segment = gPciSegNum;
    loc.Bus = gPciBusNum;
    for (ixDev = 0; ixDev < 32; ixDev++)
    {
        loc.Device = (UINT16)ixDev;
        for (ixFunc = 0; ixFunc < 8; ixFunc++)
        {
            loc.Function = (UINT16)ixFunc;
            stat = PCIBUS_ConfigRead(&loc, PCI_CONFIG_TYPEX_OFFSET_VENDORID, &val, 16);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed config read for %d,%d vendorid (%08X)\n", gPciSegNum, gPciBusNum, ixDev, ixFunc, stat);
            }
            else
            {
                vendorId = (UINT16)val;
                if (0xFFFF != vendorId)
                {
                    stat = PCIBUS_ConfigRead(&loc, PCI_CONFIG_TYPEX_OFFSET_DEVICEID, &val, 16);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed config read for %d,%d deviceid (%08X)\n", gPciSegNum, gPciBusNum, ixDev, ixFunc, stat);
                    }
                    else
                    {
                        deviceId = (UINT16)val;
                        if (0xFFFF != deviceId)
                        {
                            pChild = (PCIBUS_CHILD *)K2OS_Heap_Alloc(sizeof(PCIBUS_CHILD));
                            if (NULL != pChild)
                            {
                                K2MEM_Zero(pChild, sizeof(PCIBUS_CHILD));
                                pChild->DeviceLoc.Segment = gPciSegNum;
                                pChild->DeviceLoc.Bus = gPciBusNum;
                                pChild->DeviceLoc.Device = (UINT16)ixDev;
                                pChild->DeviceLoc.Function = (UINT16)ixFunc;
                                pChild->Ident.mVendorId = vendorId;
                                pChild->Ident.mDeviceId = deviceId;

                                stat = PCIBUS_ConfigRead(&pChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_REVISION, &val, 8);
                                if (!K2STAT_IS_ERROR(stat))
                                {
                                    pChild->Ident.mRevision = (UINT8)(val & 0xFF);
                                }

                                stat = PCIBUS_ConfigRead(&pChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_PROGIF, &val, 8);
                                if (!K2STAT_IS_ERROR(stat))
                                {
                                    pChild->Ident.mProgIF = (UINT8)(val & 0xFF);
                                }

                                stat = PCIBUS_ConfigRead(&pChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_SUBCLASSCODE, &val, 8);
                                if (!K2STAT_IS_ERROR(stat))
                                {
                                    pChild->Ident.mSubClassCode = (UINT8)(val & 0xFF);
                                }

                                stat = PCIBUS_ConfigRead(&pChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_CLASSCODE, &val, 8);
                                if (!K2STAT_IS_ERROR(stat))
                                {
                                    pChild->Ident.mClassCode = (UINT8)(val & 0xFF);
                                }

                                stat = PCIBUS_ConfigRead(&pChild->DeviceLoc, PCI_CONFIG_TYPEX_OFFSET_HEADERTYPE, &val, 8);
                                if (!K2STAT_IS_ERROR(stat))
                                {
                                    if (0 == (val & 3))
                                    {
                                        stat = PCIBUS_ConfigRead(&pChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_SUBVENDORID, &val, 16);
                                        if (!K2STAT_IS_ERROR(stat))
                                        {
                                            pChild->Ident.mSubsystemVendorId = (UINT16)(val & 0xFFFF);
                                        }

                                        stat = PCIBUS_ConfigRead(&pChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_SUBID, &val, 16);
                                        if (!K2STAT_IS_ERROR(stat))
                                        {
                                            pChild->Ident.mSubsystemId = (UINT16)(val & 0xFFFF);
                                        }
                                    }
                                }

                                stat = K2OSRPC_Object_Create(0, &gPciBusChild_ObjectClass.ClassId, (UINT_PTR)pChild, &pChild->mObjectId, &pChild->mOwnUsageId);
                                if (K2STAT_IS_ERROR(stat))
                                {
                                    K2OS_Heap_Free(pChild);
                                    pChild = NULL;
                                }
                                else
                                {
                                    K2OS_CritSec_Enter(&gPCIBUS_ChildTreeSec);
                                    pChild->ChildTreeNode.mUserVal = (((UINT32)pChild->DeviceLoc.Device) << 16) | ((UINT32)pChild->DeviceLoc.Function);
                                    K2TREE_Insert(&gPCIBUS_ChildTree, pChild->ChildTreeNode.mUserVal, &pChild->ChildTreeNode);
                                    K2OS_CritSec_Leave(&gPCIBUS_ChildTreeSec);
                                    K2OSDrv_DebugPrintf("PciBus: Found %d/%d/%d/%d == %04X/%04X\n", gPciSegNum, gPciBusNum, ixDev, ixFunc, vendorId, deviceId);
                                }
                            }
                            else
                            {
                                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed memory allocation for child device\n", gPciSegNum, gPciBusNum);
                            }
                        }
                    }
                }
            }
        }
    }
}

#endif

void
PCIBUS_DiscoverECAM(
    void
)
{
    K2STAT              stat;
    UINT_PTR            ixDev;
    UINT_PTR            ixFunc;
    UINT_PTR            virtAddr;
    K2OS_MAP_TOKEN      mapToken;
    PCICFG volatile *   pCfg;
    PCIBUS_CHILD *      pChild;

    for (ixDev = 0; ixDev < 32; ixDev++)
    {
        for (ixFunc = 0; ixFunc < 8; ixFunc++)
        {
            pCfg = NULL;
            virtAddr = K2OS_Virt_Reserve(1);
            if (0 == virtAddr)
            {
                K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Out of virtual memory enumerating\n", gPciSegNum, gPciBusNum);
            }
            else
            {
                mapToken = K2OS_Map_Create(
                    gTokBusPageArray,
                    (ixDev * 8) + ixFunc, 1,
                    virtAddr, K2OS_MapType_MemMappedIo_ReadWrite);
                if (NULL != mapToken)
                {
                    pCfg = (PCICFG volatile *)virtAddr;
                    if ((pCfg->AsType0.mVendorId == 0xFFFF) ||
                        (pCfg->AsType0.mDeviceId == 0xFFFF))
                    {
                        pCfg = NULL;
                        K2OS_Token_Destroy(mapToken);
                    }
                }
                if (NULL == pCfg)
                {
                    K2OS_Virt_Release(virtAddr);
                }
            }

            if (NULL != pCfg)
            {
                pChild = (PCIBUS_CHILD *)K2OS_Heap_Alloc(sizeof(PCIBUS_CHILD));
                if (NULL != pChild)
                {
                    K2MEM_Zero(pChild, sizeof(PCIBUS_CHILD));

                    pChild->DeviceLoc.Segment = gPciSegNum;
                    pChild->DeviceLoc.Bus = gPciBusNum;
                    pChild->DeviceLoc.Device = (UINT16)ixDev;
                    pChild->DeviceLoc.Function = (UINT16)ixFunc;
                    pChild->Ident.mVendorId = pCfg->AsType0.mVendorId;
                    pChild->Ident.mDeviceId = pCfg->AsType0.mDeviceId;
                    pChild->Ident.mRevision = pCfg->AsType0.mRevision;
                    pChild->Ident.mProgIF = pCfg->AsType0.mProgIF;
                    pChild->Ident.mSubClassCode = pCfg->AsType0.mSubClassCode;
                    pChild->Ident.mClassCode = pCfg->AsType0.mClassCode;
                    if ((pCfg->AsTypeX.mHeaderType & 3) == 0)
                    {
                        pChild->Ident.mSubsystemId = pCfg->AsType0.mSubsystemId;
                        pChild->Ident.mSubsystemVendorId = pCfg->AsType0.mSubsystemVendorId;
                    }

                    pChild->ECAM.mpCfg = pCfg;
                    pChild->ECAM.mMapToken = mapToken;

                    stat = K2OSRPC_Object_Create(0, &gPciBusChild_ObjectClass.ClassId, (UINT_PTR)pChild, &pChild->mObjectId, &pChild->mOwnUsageId);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OS_Heap_Free(pChild);
                        pChild = NULL;
                    }
                    else
                    {
                        K2OS_CritSec_Enter(&gPCIBUS_ChildTreeSec);
                        pChild->ChildTreeNode.mUserVal = (((UINT32)pChild->DeviceLoc.Device) << 16) | ((UINT32)pChild->DeviceLoc.Function);
                        K2TREE_Insert(&gPCIBUS_ChildTree, pChild->ChildTreeNode.mUserVal, &pChild->ChildTreeNode);
                        K2OS_CritSec_Leave(&gPCIBUS_ChildTreeSec);
                    }
                }
                if (NULL == pChild)
                {
                    K2OS_Token_Destroy(mapToken);
                    K2OS_Virt_Release(virtAddr);
                }
            }
            else
            {
                if (ixFunc == 0)
                {
                    //
                    // if func0 not populated, stop looking
                    //
                    break;
                }
            }
        }
    }
}

K2STAT
PCIBUS_WriteBAR(
    PCIBUS_CHILD *  apChild,
    UINT_PTR        aBarIx,
    UINT32          aValue
)
{
    K2STAT stat;
    UINT64 val64;

    val64 = aValue;
    stat = PCIBUS_ConfigWrite(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * aBarIx), &val64, 32);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to write child %04X/%04X bar reg %d in config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, aBarIx, stat);
    }
    return stat;
}

K2STAT
PCIBUS_ReadBAR(
    PCIBUS_CHILD *  apChild,
    UINT_PTR        aBarIx,
    UINT32 *        apRetVal
)
{
    K2STAT stat;
    UINT64 val64;

    stat = PCIBUS_ConfigRead(&apChild->DeviceLoc, PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * aBarIx), &val64, 32);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** PciBus(%d,%d): Failed to read child %04X/%04X bar reg %d from config space (%08X)\n", gPciSegNum, gPciBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, aBarIx, stat);
        *apRetVal = 0;
    }
    else
    {
        *apRetVal = (UINT32)val64;
    }
    return stat;
}