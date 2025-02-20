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

#include "k2osexec.h"

K2STAT 
K2OSDDK_GetInstanceInfo(
    K2OS_DEVCTX         aDevCtx,
    K2OSDDK_INSTINFO *  apRetInstInfo
)
{
    DEVNODE *   pDevNode;
    K2STAT      stat;

    if ((NULL == aDevCtx) ||
        (NULL == apRetInstInfo))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2MEM_Zero(apRetInstInfo, sizeof(K2OSDDK_INSTINFO));

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (pDevNode->InSec.mState < DevNodeState_GoingOnline_WaitStarted)
    {
        stat = K2STAT_ERROR_NOT_RUNNING;
    }
    else
    {
        stat = K2STAT_NO_ERROR;

        K2MEM_Copy(&apRetInstInfo->Ident, &pDevNode->DeviceIdent, sizeof(K2_DEVICE_IDENT));
        apRetInstInfo->mCountIo = pDevNode->InSec.IoResList.mNodeCount;
        apRetInstInfo->mCountPhys = pDevNode->InSec.PhysResList.mNodeCount;
        apRetInstInfo->mCountIrq = pDevNode->InSec.IrqResList.mNodeCount;
        apRetInstInfo->mBusType = pDevNode->mBusType;
        apRetInstInfo->mBusIfInstId = pDevNode->mBusIfInstId;
        apRetInstInfo->mBusChildId = pDevNode->mParentsChildInstanceId;
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

K2STAT 
K2OSDDK_GetRes(
    K2OS_DEVCTX     aDevCtx,
    UINT32          aResType,
    UINT32          aIndex,
    K2OSDDK_RES *   apRetRes
)
{
    K2LIST_LINK *   pListLink;
    K2LIST_ANCHOR * pList;
    DEVNODE *       pDevNode;
    K2STAT          stat;
    DEV_RES *       pDevRes;

    if ((NULL == aDevCtx) || (NULL == apRetRes))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pDevNode = (DEVNODE *)aDevCtx;

    // setting list pointer, no access outside sec
    switch (aResType)
    {
    case K2OS_RESTYPE_IO:
        pList = &pDevNode->InSec.IoResList;
        break;
    case K2OS_RESTYPE_PHYS:
        pList = &pDevNode->InSec.PhysResList;
        break;
    case K2OS_RESTYPE_IRQ:
        pList = &pDevNode->InSec.IrqResList;
        break;
    default:
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (pDevNode->InSec.mState < DevNodeState_GoingOnline_WaitStarted)
    {
        stat = K2STAT_ERROR_NOT_RUNNING;
    }
    else
    {
        if (aIndex >= pList->mNodeCount)
        {
            stat = K2STAT_ERROR_OUT_OF_BOUNDS;
        }
        else
        {
            stat = K2STAT_NO_ERROR;

            pListLink = pList->mpHead;
            if (aIndex > 0)
            {
                do {
                    pListLink = pListLink->mpNext;
                } while (--aIndex);
            }
            pDevRes = K2_GET_CONTAINER(DEV_RES, pListLink, ListLink);
            K2MEM_Copy(apRetRes, &pDevRes->DdkRes, sizeof(K2OSDDK_RES));
        }
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

K2STAT 
K2OSDDK_MountChild(
    K2OS_DEVCTX             aDevCtx,
    UINT32                  aFlags,
    UINT64 const *          apBusSpecificAddress,
    K2_DEVICE_IDENT const * apIdent,
    K2OS_IFINST_ID          aBusIfInstId,
    UINT32 *                apRetChildInstanceId
)
{
    DEVNODE *   pDevNode;
    K2STAT      stat;
    UINT32      busType;

    if ((NULL == aDevCtx) ||
        (NULL == apBusSpecificAddress) ||
        (NULL == apIdent) ||
        (NULL == apRetChildInstanceId))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    busType = aFlags & K2OSDDK_MOUNTCHILD_FLAGS_BUSTYPE_MASK;
    if ((busType < K2OS_BUSTYPE_MIN) ||
        (busType > K2OS_BUSTYPE_MAX))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (pDevNode->InSec.mState != DevNodeState_Online)
    {
        stat = K2STAT_ERROR_NOT_RUNNING;
    }
    else
    {
        stat = Dev_NodeLocked_MountChild(pDevNode, aFlags, apBusSpecificAddress, apIdent, aBusIfInstId, apRetChildInstanceId);
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

K2STAT 
K2OSDDK_SetEnable(
    K2OS_DEVCTX aDevCtx,
    BOOL        aSetEnable
)
{
    DEVNODE *   pDevNode;
    K2STAT      stat;

    if (NULL == aDevCtx)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (pDevNode->InSec.mState != DevNodeState_Online)
    {
        stat = K2STAT_ERROR_NOT_RUNNING;
    }
    else
    {
        if (aSetEnable)
        {
            stat = Dev_NodeLocked_Enable(pDevNode);
        }
        else
        {
            K2_ASSERT(0);
        }
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

K2STAT 
ACPI_RunMethod(
    K2OS_DEVCTX aDevCtx,
    UINT32      aMethod,
    UINT32      aFlags,
    UINT32      aInput,
    UINT32 *    apRetResult
)
{
    DEVNODE *   pDevNode;
    K2STAT      stat;
    UINT64      val64;

    if ((NULL == aDevCtx) || (0 == aMethod))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (pDevNode->InSec.mState < DevNodeState_GoingOnline_WaitStarted)
    {
        stat = K2STAT_ERROR_NOT_RUNNING;
    }
    else if (NULL == pDevNode->InSec.mpAcpiNode)
    {
        stat = K2STAT_ERROR_NO_INTERFACE;
    }
    else
    {
        val64 = 0;
        stat = K2OSACPI_RunDeviceNumericMethod(pDevNode->InSec.mpAcpiNode->mhObject, (char const *)&aMethod, &val64);
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    if (!K2STAT_IS_ERROR(stat))
    {
        if (NULL != apRetResult)
        {
            *apRetResult = (UINT32)val64;
        }
    }

    return stat;
}

K2STAT 
K2OSDDK_AddChildRes(
    K2OS_DEVCTX             aDevCtx,
    UINT32                  aChildInstanceId,
    K2OSDDK_RESDEF const *  apResDef
)
{
    DEVNODE *       pDevNode;
    K2STAT          stat;
    K2LIST_LINK *   pListLink;
    DEVNODE *       pChildNode;

    if ((NULL == aDevCtx) || 
        (0 == aChildInstanceId) ||
        (NULL == apResDef))
    {
        K2_ASSERT(0);
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (pDevNode->InSec.mState != DevNodeState_Online)
    {
        stat = K2STAT_ERROR_NOT_RUNNING;
    }
    else
    {
        pChildNode = NULL;
        pListLink = pDevNode->InSec.ChildList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pChildNode = K2_GET_CONTAINER(DEVNODE, pListLink, InParentSec.ParentChildListLink);
                if (aChildInstanceId == pChildNode->mParentsChildInstanceId)
                    break;
                pChildNode = NULL;
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
        if (NULL == pListLink)
        {
            stat = K2STAT_ERROR_NOT_FOUND;
        }
        else
        {
            K2_ASSERT(NULL != pChildNode);

            K2OS_CritSec_Enter(&pChildNode->Sec);

            if (pChildNode->InSec.mState == DevNodeState_Online)
            {
                stat = K2STAT_ERROR_RUNNING;
            }
            else
            {
                stat = Dev_NodeLocked_AddRes(pChildNode, apResDef, NULL);
            }

            K2OS_CritSec_Leave(&pChildNode->Sec);
        }
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

K2STAT
K2OSDDK_DriverStarted(
    K2OS_DEVCTX aDevCtx
)
{
    DEVNODE *   pDevNode;
    K2STAT      stat;

    if (NULL == aDevCtx)
    {
        K2_ASSERT(0);
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (pDevNode->InSec.mState != DevNodeState_GoingOnline_WaitStarted)
    {
        K2_ASSERT(0);
        stat = K2STAT_ERROR_API_ORDER;
    }
    else
    {
        pDevNode->InSec.mState = DevNodeState_Online;
        stat = K2STAT_NO_ERROR;
//        K2OSKERN_Debug("Driver(\"%s\") instance %08X started\n", pDevNode->InSec.mpDriverCandidate, pDevNode->Driver.mpContext);
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

K2STAT
K2OSDDK_DriverStopped(
    K2OS_DEVCTX aDevCtx,
    K2STAT      aResult
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2OS_PAGEARRAY_TOKEN 
K2OSDDK_PageArray_CreateIo(
    UINT32  aFlags,
    UINT32  aPageCountPow2,
    UINT32 *apRetPhysBase
)
{
    return gKernDdk.PageArray_CreateIo(aFlags, aPageCountPow2, apRetPhysBase);
}

K2STAT 
K2OSDDK_GetPciIrqRoutingTable(
    K2OS_DEVCTX aDevCtx,
    void **     appRetTable
)
{
    DEVNODE *           pDevNode;
    K2STAT              stat;
    ACPI_STATUS         acpiStatus;
    ACPI_BUFFER         acpiBuffer;
    ACPI_DEVICE_INFO *  pDevInfo;

    if (NULL == aDevCtx)
    {
        K2_ASSERT(0);
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pDevNode = (DEVNODE *)aDevCtx;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (NULL != pDevNode->InSec.mpAcpiNode)
    {
        pDevInfo = pDevNode->InSec.mpAcpiNode->mpDeviceInfo;
        if (NULL != pDevInfo)
        {
            if (0 != (pDevInfo->Flags & ACPI_PCI_ROOT_BRIDGE))
            {
                acpiBuffer.Length = ACPI_ALLOCATE_BUFFER;
                acpiBuffer.Pointer = NULL;
                acpiStatus = AcpiGetIrqRoutingTable(pDevNode->InSec.mpAcpiNode->mhObject, &acpiBuffer);
                if ((!ACPI_FAILURE(acpiStatus)) && (NULL != acpiBuffer.Pointer) && (0 != acpiBuffer.Length))
                {
                    *appRetTable = (void *)acpiBuffer.Pointer;
                    stat = K2STAT_NO_ERROR;
                }
                else
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                }
            }
            else
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
            }
        }
        else
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
    }
    else
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

