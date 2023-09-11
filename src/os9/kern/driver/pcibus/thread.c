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
StartDriver(
    PCIBUS *apPciBus
)
{
    K2STAT          stat;
    UINT32          resIx;
    K2TREE_NODE *   pTreeNode;
    PCIBUS_CHILD *  pChild;

    //
    // acpi method must exist and return the bus number
    //
    resIx = 0;
    stat = K2OSDDK_RunAcpiMethod(apPciBus->mDevCtx, K2_MAKEID4('_', 'B', 'B', 'N'), 0, 0, &resIx);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus: RunAcpi(BBN) failed (0x%08X)\n", stat);
        return K2STAT_ERROR_NOT_EXIST;
    }
    apPciBus->mBusNum = (UINT16)resIx;

    resIx = 0;
    stat = K2OSDDK_RunAcpiMethod(apPciBus->mDevCtx, K2_MAKEID4('_', 'S', 'E', 'G'), 0, 0, &resIx);
    if (K2STAT_IS_ERROR(stat))
    {
        apPciBus->mSegNum = 0;
    }
    else
    {
        apPciBus->mSegNum = (UINT16)resIx;
    }

    stat = K2OSDDK_GetInstanceInfo(apPciBus->mDevCtx, &apPciBus->Ident, &apPciBus->mCountIo, &apPciBus->mCountPhys, &apPciBus->mCountIrq);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus: Enum resources failed (0x%08X)\n", stat);
        return stat;
    }

    apPciBus->mUseECAM = FALSE;

    if (apPciBus->mCountPhys > 0)
    {
        //
        // find the ECAM range by matching size
        //
        for (resIx = 0; resIx < apPciBus->mCountPhys; resIx++)
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

        if (resIx == apPciBus->mCountPhys)
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
        if (apPciBus->mCountIo > 0)
        {
            for (resIx = 0; resIx < apPciBus->mCountIo; resIx++)
            {
                stat = K2OSDDK_GetRes(apPciBus->mDevCtx, K2OS_RESTYPE_IO, 0, &apPciBus->BusIoRes);
                if (!K2STAT_IS_ERROR(stat))
                {
                    if (apPciBus->BusIoRes.Def.Io.Range.mSizeBytes == 8)
                        break;
                }
            }
            if (resIx == apPciBus->mCountIo)
            {
                apPciBus->mCountIo = 0;
            }
            else
            {
                K2OSKERN_Debug("PciBus(%d,%d): Using IO Bus (%04X,%04X)\n", apPciBus->mSegNum, apPciBus->mBusNum, apPciBus->BusIoRes.Def.Io.Range.mBasePort, apPciBus->BusIoRes.Def.Io.Range.mSizeBytes);
            }
        }
        if (0 == apPciBus->mCountIo)
        {
            K2OSKERN_Debug("*** PciBus(%d,%d): no usable phys or io resources\n", apPciBus->mSegNum, apPciBus->mBusNum);
            return K2STAT_ERROR_NOT_EXIST;
        }
#else
        K2OSKERN_Debug("*** PciBus(%d,%d): no usable ECAM range found\n", apPciBus->mSegNum, apPciBus->mBusNum);
        return K2STAT_ERROR_NOT_EXIST;
#endif
    }

    if (!K2OS_CritSec_Init(&apPciBus->ChildTreeSec))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): critsec init failed (0x%08X)\n", apPciBus->mSegNum, apPciBus->mBusNum, K2OS_Thread_GetLastStatus());
        return K2OS_Thread_GetLastStatus();
    }

    K2TREE_Init(&apPciBus->ChildTree, NULL);

    PciBus_InitAndDiscover(apPciBus);

    if (0 == apPciBus->ChildTree.mNodeCount)
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): No children found\n", apPciBus->mSegNum, apPciBus->mBusNum);
        return K2STAT_ERROR_NO_INTERFACE;
    }

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
OnDdkMessage(
    PCIBUS *        apPciBus,
    K2OS_MSG const *apMsg
)
{
    K2STAT stat;

    if (apMsg->mShort == K2OS_SYSTEM_MSG_DDK_SHORT_START)
    {
        stat = StartDriver(apPciBus);
    }
    else
    {
        K2OSKERN_Debug("PCIBus: caught unsupported DDK message %d\n", apMsg->mShort);
        stat = K2STAT_NO_ERROR;
    }

    return stat;
}

UINT32
PciBus_Instance_Thread(
    PCIBUS *    apPciBus
)
{
    K2OS_MSG    msg;
    K2STAT      stat;

    stat = K2OSDDK_SetMailslot(apPciBus->mDevCtx, (UINT32)apPciBus->mTokMailbox);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    do {
        if (!K2OS_Mailbox_Recv(apPciBus->mTokMailbox, &msg, K2OS_TIMEOUT_INFINITE))
        {
            stat = K2OS_Thread_GetLastStatus();
            K2OSKERN_Debug("Mailbox Recv returned failure - %08X\n", stat);
            break;
        }

        if (msg.mType == K2OS_SYSTEM_MSGTYPE_DDK)
        {
            stat = OnDdkMessage(apPciBus, &msg);
            if (K2STAT_IS_ERROR(stat))
                break;
        }
        else
        {
            K2OSKERN_Debug("PCIBus: caught unsupported type message 0x%04X\n", msg.mType);
        }
    } while (1);

    return K2OSDDK_DriverStopped(apPciBus->mDevCtx, stat);
}
 