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

K2STAT
PCIBusRpc_Create(
    K2OS_RPC_OBJ                aObject,
    K2OS_RPC_OBJ_CREATE const * apCreate,
    UINT32 *                    apRetContext
)
{
    PCIBUS *pPciBus;

    if (0 != apCreate->mCreatorProcessId)
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pPciBus = (PCIBUS *)apCreate->mCreatorContext;

    pPciBus->mRpcObj = aObject;
    *apRetContext = (UINT32)pPciBus;

    return K2STAT_NO_ERROR;
}

K2STAT
PCIBusRpc_OnAttach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aProcessId,
    UINT32 *        apRetUseContext
)
{
    if (0 != aProcessId)
        return K2STAT_ERROR_NOT_ALLOWED;
    return K2STAT_NO_ERROR;
}

K2STAT
PCIBusRpc_OnDetach(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext,
    UINT32          aUseContext
)
{
    return K2STAT_NO_ERROR;
}

K2STAT
PCIBusRpc_Call(
    K2OS_RPC_OBJ_CALL const *   apCall,
    UINT32 *                    apRetUsedOutBytes
)
{
    PCIBUS *                            pPciBus;
    K2TREE_NODE *                       pTreeNode;
    PCIBUS_CHILD *                      pChild;
    K2STAT                              stat;
    K2OS_PCIBUS_CFG_READ_IN const *     pReadIn;
    K2OS_PCIBUS_CFG_WRITE_IN const *    pWriteIn;
    UINT64                              val64;

    pPciBus = (PCIBUS *)apCall->mObjContext;

    pTreeNode = K2TREE_Find(&pPciBus->ChildIdTree, ((K2OS_PCIBUS_CFG_READ_IN const *)apCall->Args.mpInBuf)->mBusChildId);
    if (NULL == pTreeNode)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }
    pChild = K2_GET_CONTAINER(PCIBUS_CHILD, pTreeNode, ChildIdTreeNode);

    switch (apCall->Args.mMethodId)
    {
    case K2OS_PCIBUS_METHOD_CFG_READ:
        if ((apCall->Args.mInBufByteCount != sizeof(K2OS_PCIBUS_CFG_READ_IN)) ||
            (apCall->Args.mOutBufByteCount != sizeof(UINT32)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            pReadIn = (K2OS_PCIBUS_CFG_READ_IN const *)apCall->Args.mpInBuf;
            stat = pPciBus->mfConfigRead(pPciBus, &pChild->DeviceLoc, pReadIn->Loc.mOffset, &val64, pReadIn->Loc.mWidth);
            if (!K2STAT_IS_ERROR(stat))
            {
                *((UINT32 *)apCall->Args.mpOutBuf) = (UINT32)val64;
                *apRetUsedOutBytes = sizeof(UINT32);
            }
        }
        break;

    case K2OS_PCIBUS_METHOD_CFG_WRITE:
        if ((apCall->Args.mInBufByteCount != sizeof(K2OS_PCIBUS_CFG_WRITE_IN)) ||
            (apCall->Args.mOutBufByteCount != 0))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            pWriteIn = (K2OS_PCIBUS_CFG_WRITE_IN const *)apCall->Args.mpInBuf;
            val64 = pWriteIn->mValue;
            stat = pPciBus->mfConfigWrite(pPciBus, &pChild->DeviceLoc, pWriteIn->Loc.mOffset, &val64, pWriteIn->Loc.mWidth);
        }
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    }

    return stat;
}

K2STAT
PCIBusRpc_Delete(
    K2OS_RPC_OBJ    aObject,
    UINT32          aObjContext
)
{
    PCIBUS *    pPciBus;

    pPciBus = (PCIBUS *)aObjContext;

    K2OSKERN_Panic("*** PCI Bus Driver Delete (%08X)!\n", pPciBus);

    return K2STAT_ERROR_UNKNOWN;
}

K2STAT
CreateInstance(
    K2OS_DEVCTX aDevCtx,
    void **     appRetDriverContext
)
{
    PCIBUS *    pPciBus;
    K2STAT      stat;

    pPciBus = (PCIBUS *)K2OS_Heap_Alloc(sizeof(PCIBUS));
    if (NULL == pPciBus)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pPciBus, sizeof(PCIBUS));

    pPciBus->mDevCtx = aDevCtx;
    *appRetDriverContext = pPciBus;

    return K2STAT_NO_ERROR;
}

K2STAT
StartDriver(
    PCIBUS *apPciBus
)
{
    // {A59D0ED6-79C8-49C8-9D0A-5F6F1E393EE2}
    static K2OS_RPC_OBJ_CLASSDEF const sPciBusClassDef = {
        { 0xa59d0ed6, 0x79c8, 0x49c8, { 0x9d, 0xa, 0x5f, 0x6f, 0x1e, 0x39, 0x3e, 0xe2 } },
        PCIBusRpc_Create,
        PCIBusRpc_OnAttach,
        PCIBusRpc_OnDetach,
        PCIBusRpc_Call,
        PCIBusRpc_Delete
    };
    static K2_GUID128 sPciBusIfaceId = K2OS_IFACE_PCIBUS;

    K2STAT                      stat;
    UINT32                      resIx;
    K2TREE_NODE *               pTreeNode;
    PCIBUS_CHILD *              pChild;
    K2OS_RPC_OBJ_HANDLE         busRpcHandle;
    K2OS_RPC_CALLARGS           callArgs;
    K2OS_ACPIBUS_RUNMETHOD_IN   runMethodIn;
    K2OS_ACPIBUS_RUNMETHOD_OUT  runMethodOut;
    UINT32                      rpcOut;

    stat = K2OSDDK_GetInstanceInfo(apPciBus->mDevCtx, &apPciBus->InstInfo);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus: Enum resources failed (0x%08X)\n", stat);
        return stat;
    }

    busRpcHandle = K2OS_Rpc_AttachByIfInstId(apPciBus->InstInfo.mBusIfInstId, NULL);
    if (NULL == busRpcHandle)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OSKERN_Debug("*** PciBus: Coult not attach to parent bus RPC object (0x%08X)\n", stat);
        return stat;
    }

    //
    // acpi method must exist and return the bus number
    //
    K2MEM_Zero(&callArgs, sizeof(callArgs));
    K2MEM_Zero(&runMethodIn, sizeof(runMethodIn));
    K2MEM_Zero(&runMethodOut, sizeof(runMethodOut));
    callArgs.mInBufByteCount = sizeof(runMethodIn);
    callArgs.mOutBufByteCount = sizeof(runMethodOut);
    callArgs.mpInBuf = (UINT8 const *)&runMethodIn;
    callArgs.mpOutBuf = (UINT8 *)&runMethodOut;
    callArgs.mMethodId = K2OS_ACPIBUS_METHOD_RUNMETHOD;
    runMethodIn.mDevCtx = apPciBus->mDevCtx;
    runMethodIn.mMethod = K2_MAKEID4('_', 'B', 'B', 'N');
    rpcOut = 0;
    stat = K2OS_Rpc_Call(busRpcHandle, &callArgs, &rpcOut);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus: RunAcpi(BBN) failed (0x%08X)\n", stat);
        K2OS_Rpc_Release(busRpcHandle);
        return K2STAT_ERROR_NOT_EXIST;
    }
    if (rpcOut != sizeof(runMethodOut))
    {
        K2OSKERN_Debug("*** PciBus: RunAcpi(BBN) returned wrong size output (%d)\n", rpcOut);
        K2OS_Rpc_Release(busRpcHandle);
        return K2STAT_ERROR_BAD_SIZE;
    }
    apPciBus->mBusNum = (UINT16)runMethodOut.mResult;

    runMethodIn.mMethod = K2_MAKEID4('_', 'S', 'E', 'G');
    rpcOut = 0;
    stat = K2OS_Rpc_Call(busRpcHandle, &callArgs, &rpcOut);
    if ((K2STAT_IS_ERROR(stat)) || (rpcOut != sizeof(runMethodOut)))
    {
        apPciBus->mSegNum = 0;
    }
    else
    {
        apPciBus->mSegNum = (UINT16)runMethodOut.mResult;
    }

    K2OS_Rpc_Release(busRpcHandle);

    apPciBus->mUseECAM = FALSE;

    if (apPciBus->InstInfo.mCountPhys > 0)
    {
        //
        // find the ECAM range by matching size
        //
        for (resIx = 0; resIx < apPciBus->InstInfo.mCountPhys; resIx++)
        {
            stat = K2OSDDK_GetRes(apPciBus->mDevCtx, K2OS_RESTYPE_PHYS, 0, &apPciBus->BusPhysRes);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSKERN_Debug("*** PciBus(%d,%d): Could not get phys resource 0 (0x%08X)\n", apPciBus->mSegNum, apPciBus->mBusNum, stat);
                return stat;
            }

            if (apPciBus->BusPhysRes.Def.Phys.Range.mSizeBytes == 0x100000)
                break;

            K2MEM_Zero(&apPciBus->BusPhysRes, sizeof(apPciBus->BusPhysRes));
        }

        if (resIx == apPciBus->InstInfo.mCountPhys)
        {
            K2OSKERN_Debug("!!! PciBus(%d,%d): Phys resource(s) found but they are not the correct size for a pci bus\n", apPciBus->mSegNum, apPciBus->mBusNum);
        }
        else
        {
            apPciBus->mECAMBaseAddr = apPciBus->BusPhysRes.Def.Phys.Range.mBaseAddr;
            apPciBus->mUseECAM = TRUE;
            K2OSKERN_Debug("PciBus(%d,%d): Using ECAM at %08X\n", apPciBus->mSegNum, apPciBus->mBusNum, apPciBus->mECAMBaseAddr);
        }
    }

    if (!apPciBus->mUseECAM)
    {
#if K2_TARGET_ARCH_IS_INTEL
        if (apPciBus->InstInfo.mCountIo > 0)
        {
            for (resIx = 0; resIx < apPciBus->InstInfo.mCountIo; resIx++)
            {
                stat = K2OSDDK_GetRes(apPciBus->mDevCtx, K2OS_RESTYPE_IO, 0, &apPciBus->BusIoRes);
                if (!K2STAT_IS_ERROR(stat))
                {
                    if (apPciBus->BusIoRes.Def.Io.Range.mSizeBytes == 8)
                        break;
                }
            }
            if (resIx == apPciBus->InstInfo.mCountIo)
            {
                apPciBus->InstInfo.mCountIo = 0;
            }
            else
            {
                K2OSKERN_Debug("PciBus(%d,%d): Using IO Bus (%04X,%04X)\n", apPciBus->mSegNum, apPciBus->mBusNum, apPciBus->BusIoRes.Def.Io.Range.mBasePort, apPciBus->BusIoRes.Def.Io.Range.mSizeBytes);
            }
        }
        if (0 == apPciBus->InstInfo.mCountIo)
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): no usable phys or io resources\n", apPciBus->mSegNum, apPciBus->mBusNum);
            return K2STAT_ERROR_NOT_EXIST;
        }
#else
        K2OSKERN_Debug("*** PciBus(%d,%d): no usable ECAM range found\n", apPciBus->mSegNum, apPciBus->mBusNum);
        return K2STAT_ERROR_NOT_EXIST;
#endif
    }

    // 
    // get PCI routing table
    //
    stat = K2OSDDK_GetPciIrqRoutingTable(apPciBus->mDevCtx, (void **)&apPciBus->mpRoutingTable);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): no routing table\n", apPciBus->mSegNum, apPciBus->mBusNum);
    }

    K2TREE_Init(&apPciBus->ChildTree, NULL);
    K2TREE_Init(&apPciBus->ChildIdTree, NULL);

    PciBus_InitAndDiscover(apPciBus);

    if (0 == apPciBus->ChildTree.mNodeCount)
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): No children found\n", apPciBus->mSegNum, apPciBus->mBusNum);
        return K2STAT_ERROR_NO_INTERFACE;
    }

    //
    // create rpc object for the bus now
    //

    apPciBus->mRpcClass = K2OS_RpcServer_Register(&sPciBusClassDef, (UINT32)apPciBus);
    if (NULL == apPciBus->mRpcClass)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to register RPC class (0x%08X)\n", apPciBus->mSegNum, apPciBus->mBusNum, stat);
        return stat;
    }

    apPciBus->mRpcObjHandle = K2OS_Rpc_CreateObj(0, &sPciBusClassDef.ClassId, (UINT32)apPciBus);
    K2_ASSERT(NULL != apPciBus->mRpcObjHandle);
    K2_ASSERT(NULL != apPciBus->mRpcObj);

    apPciBus->mRpcIfInst = K2OS_RpcObj_AddIfInst(apPciBus->mRpcObj, K2OS_IFACE_CLASSCODE_BUSDRIVER, &sPciBusIfaceId, &apPciBus->mRpcIfInstId, FALSE);
    K2_ASSERT(apPciBus->mRpcIfInst != NULL);
    K2OSKERN_Debug("PCI Bus driver ifinstId = %d\n", apPciBus->mRpcIfInstId);

    K2OSDDK_DriverStarted(apPciBus->mDevCtx);

    //
    // mount the bus children
    //
    pTreeNode = K2TREE_FirstNode(&apPciBus->ChildTree);
    do {
        pChild = K2_GET_CONTAINER(PCIBUS_CHILD, pTreeNode, ChildTreeNode);
        pTreeNode = K2TREE_NextNode(&apPciBus->ChildTree, pTreeNode);
        PciBus_MountChild(pChild);
    } while (NULL != pTreeNode);   // only do first one for now

    //
    // finally enable
    //
    K2OSDDK_SetEnable(apPciBus->mDevCtx, TRUE);

    return K2STAT_NO_ERROR;
}

K2STAT
StopDriver(
    PCIBUS *    apPciBus
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
DeleteInstance(
    PCIBUS *    apPciBus
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}