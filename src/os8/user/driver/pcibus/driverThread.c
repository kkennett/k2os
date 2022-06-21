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

typedef struct _PCIBUS_CHILD PCIBUS_CHILD;
struct _PCIBUS_CHILD
{
    K2PCI_DEVICE_ID Id;
    K2OS_MAP_TOKEN  mMapToken;
    PCICFG *        mpCfg;
    UINT_PTR        mObjectId;
    UINT_PTR        mOwnUsageId;

    K2LIST_LINK     ChildListLink;
};

static K2OS_CRITSEC   gPCIBUS_ChildListSec;
static K2LIST_ANCHOR  gPCIBUS_ChildList;

static const K2_GUID128 const sgPciBusChild_ClassId = K2OSDRV_CHILDOBJECT_CLASS_ID;

K2STAT PCIBUS_ChildObject_Create(K2OSRPC_OBJECT_CREATE const *apCreate, UINT_PTR *apRetCreator);
K2STAT PCIBUS_ChildObject_Call(K2OSRPC_OBJECT_CALL const *apCall, UINT_PTR *apRetUsedOutBytes);
K2STAT PCIBUS_ChildObject_Delete(K2OSRPC_OBJECT_DELETE const *apDelete);

static const K2OSRPC_OBJECT_CLASS sgPciBusChild_ObjectClass =
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

    K2OS_CritSec_Enter(&gPCIBUS_ChildListSec);
    K2LIST_Remove(&gPCIBUS_ChildList, &pChild->ChildListLink);
    K2OS_CritSec_Leave(&gPCIBUS_ChildListSec);

    K2OS_Token_Destroy(pChild->mMapToken);
    pChild->mMapToken = NULL;

    return K2STAT_NO_ERROR;
}

UINT_PTR 
DriverThread(
    char const *apArgs
)
{
    K2STAT                      stat;
    K2OSDRV_PCIBUS_PARAM_OUT    pciBusParam;
    UINT16                      ixDev;
    UINT16                      ixFunc;
    PCIBUS_CHILD *              pChild;
    PCICFG *                    pCfg;
    UINT32                      virtAddr;
    K2OS_MAP_TOKEN              mapToken;
    K2LIST_LINK *               pListLink;
    UINT64                      address;
    K2_DEVICE_IDENT             ident;

    if (!K2OS_CritSec_Init(&gPCIBUS_ChildListSec))
    {
        K2OSDrv_DebugPrintf("PciBus critsec init failed, error 0x%08X\n", K2OS_Thread_GetLastStatus());
        return K2OS_Thread_GetLastStatus();
    }
    K2LIST_Init(&gPCIBUS_ChildList);

    //
    // try to get bus parameters. if this fails this is not a pci bus
    //
    stat = K2OSDrv_GetPciBusParam(&pciBusParam);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("***Error 0x%08X retrieving pcibus params\n", stat);
        return stat;
    }

    //
    // create the rpc server for pci bus children
    //
    if (!K2OSRPC_ObjectClass_Publish(&sgPciBusChild_ObjectClass, 0))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2OSDrv_DebugPrintf("***Error 0x%08X publishing bus child object class\n", stat);
        return stat;
    }

    //
    // scan the bus for child devices and create them
    //
    for (ixDev = 0; ixDev < 32; ixDev++)
    {
        for (ixFunc = 0; ixFunc < 8; ixFunc++)
        {
            pCfg = NULL;
            virtAddr = K2OS_Virt_Reserve(1);
            if (0 == virtAddr)
            {
                K2OSDrv_DebugPrintf("**out of virtual memory enumerating pci bus %d\n", pciBusParam.BusId.Bus);
            }
            else
            {
                mapToken = K2OS_Map_Create(
                    pciBusParam.mPageArrayToken, 
                    (ixDev * 8) + ixFunc, 1, 
                    virtAddr, K2OS_MapType_MemMappedIo_ReadWrite);
                if (NULL != mapToken)
                {
                    pCfg = (PCICFG *)virtAddr;
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
                    pChild->Id.Segment = (UINT16)pciBusParam.BusId.Segment;
                    pChild->Id.Bus = (UINT16)pciBusParam.BusId.Bus;
                    pChild->Id.Device = ixDev;
                    pChild->Id.Function = ixFunc;

                    pChild->mpCfg = pCfg;
                    pChild->mMapToken = mapToken;

                    stat = K2OSRPC_Object_Create(0, &sgPciBusChild_ClassId, (UINT_PTR)pChild, &pChild->mObjectId, &pChild->mOwnUsageId);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OS_Heap_Free(pChild);
                        pChild = NULL;
                    }
                    else
                    {
                        K2OS_CritSec_Enter(&gPCIBUS_ChildListSec);
                        K2LIST_AddAtTail(&gPCIBUS_ChildList, &pChild->ChildListLink);
                        K2OS_CritSec_Leave(&gPCIBUS_ChildListSec);
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

    if (0 == gPCIBUS_ChildList.mNodeCount)
    {
        K2OSDrv_DebugPrintf("***PCIBUS: No children found\n");
        return 0;
    }

    pListLink = gPCIBUS_ChildList.mpHead;
    do {
        pChild = K2_GET_CONTAINER(PCIBUS_CHILD, pListLink, ChildListLink);
        pListLink = pListLink->mpNext;

        address = (((UINT64)(pChild->Id.Device)) << 16) | ((UINT64)(pChild->Id.Function));

        K2MEM_Zero(&ident, sizeof(ident));
        ident.mVendorId = pChild->mpCfg->AsType0.mVendorId;
        ident.mDeviceId = pChild->mpCfg->AsType0.mDeviceId;
        ident.mRevision = pChild->mpCfg->AsType0.mRevision;
        ident.mProgIF = pChild->mpCfg->AsType0.mProgIF;
        ident.mSubClassCode = pChild->mpCfg->AsType0.mSubClassCode;
        ident.mClassCode = pChild->mpCfg->AsType0.mClassCode;
        if ((pChild->mpCfg->AsTypeX.mHeaderType & 3) == 0)
        {
            ident.mSubsystemId = pChild->mpCfg->AsType0.mSubsystemId;
            ident.mSubsystemVendorId = pChild->mpCfg->AsType0.mSubsystemVendorId;
        }

        stat = K2OSDrv_MountChildDevice(pChild->mObjectId, K2OSDRV_BUSTYPE_PCI, &address, &ident);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("***PCIBUS: Failed to mount child device object %d\n", pChild->mObjectId);
        }

    } while (NULL != pListLink);

    K2OSDrv_SetEnable(TRUE);

    do {
        K2OS_Thread_Sleep(1000);
        K2OSDrv_DebugPrintf("PciBus_Tick()\n");
    } while (1);

    return 0;
}

