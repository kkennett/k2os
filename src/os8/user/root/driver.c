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
#include "root.h"

K2TREE_ANCHOR    gDriverInstanceTree;
K2OS_CRITSEC     gDriverInstanceTreeSec;

K2STAT
Driver_HostObj_Create(
    K2OSRPC_OBJECT_CREATE const *   apCreate,
    UINT_PTR *                      apRetRef
)
{
    K2TREE_NODE *   pTreeNode;
    DEVNODE *       pDevNode;
    K2STAT          stat;

    K2OS_CritSec_Enter(&gDriverInstanceTreeSec);

    pTreeNode = K2TREE_Find(&gDriverInstanceTree, apCreate->mCallerProcessId);
    if (NULL != pTreeNode)
    {
        pDevNode = K2_GET_CONTAINER(DEVNODE, pTreeNode, Driver.InstanceTreeNode);
    }
    else
    {
        pDevNode = NULL;
    }

    K2OS_CritSec_Leave(&gDriverInstanceTreeSec);

    if (NULL == pDevNode)
        return K2STAT_ERROR_NOT_FOUND;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    stat = Dev_NodeLocked_OnDriverHostObjCreate(pDevNode);
    if (!K2STAT_IS_ERROR(stat))
    {
        *apRetRef = (UINT_PTR)pDevNode;
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_SetMailbox(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                          stat;
    K2OSDRV_SET_MAILSLOT_IN const * pArgsIn;

    if ((apCall->Args.mInBufByteCount < sizeof(K2OSDRV_SET_MAILSLOT_IN)) ||
        (apCall->Args.mOutBufByteCount != 0))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pArgsIn = (K2OSDRV_SET_MAILSLOT_IN const *)apCall->Args.mpInBuf;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    stat = Dev_NodeLocked_OnSetMailbox(apDevNode, (UINT_PTR)pArgsIn->mMailslotToken);

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_EnumRes(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                  stat;
    K2OSDRV_ENUM_RES_OUT *  pOut;

    if ((apCall->Args.mInBufByteCount != 0) ||
        (apCall->Args.mOutBufByteCount != sizeof(K2OSDRV_ENUM_RES_OUT)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pOut = (K2OSDRV_ENUM_RES_OUT *)apCall->Args.mpOutBuf;

    K2MEM_Zero(pOut, sizeof(K2OSDRV_ENUM_RES_OUT));

    K2OS_CritSec_Enter(&apDevNode->Sec);

    stat = Dev_NodeLocked_OnEnumRes(apDevNode, pOut);

    if (!K2STAT_IS_ERROR(stat))
    {
        *apRetUsedOutBytes = sizeof(K2OSDRV_ENUM_RES_OUT);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_GetRes(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                      stat;
    K2OSDRV_GET_RES_IN const *  pArgsIn;

    if ((apCall->Args.mInBufByteCount != sizeof(K2OSDRV_GET_RES_IN)) ||
        (apCall->Args.mOutBufByteCount == 0))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pArgsIn = (K2OSDRV_GET_RES_IN const *)apCall->Args.mpInBuf;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    stat = Dev_NodeLocked_OnGetRes(apDevNode, pArgsIn, apCall->Args.mpOutBuf, apCall->Args.mOutBufByteCount, apRetUsedOutBytes);

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_MountChild(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                          stat;
    K2OSDRV_MOUNT_CHILD_IN const *  pArgsIn;
    K2OSDRV_MOUNT_CHILD_OUT *       pArgsOut;
    UINT_PTR                        busType;

    if ((apCall->Args.mInBufByteCount != sizeof(K2OSDRV_MOUNT_CHILD_IN)) ||
        (apCall->Args.mOutBufByteCount != sizeof(K2OSDRV_MOUNT_CHILD_OUT)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pArgsIn = (K2OSDRV_MOUNT_CHILD_IN const *)apCall->Args.mpInBuf;
    
    pArgsOut = (K2OSDRV_MOUNT_CHILD_OUT *)apCall->Args.mpOutBuf;

    busType = (pArgsIn->mMountFlags & K2OSDRV_MOUNTFLAGS_BUSTYPE_MASK);

    if ((K2OSDRV_BUSTYPE_INVALID == busType) ||
        (K2OSDRV_BUSTYPE_COUNT <= busType))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2OS_CritSec_Enter(&apDevNode->Sec);

    if (apDevNode->InSec.Driver.mEnabled)
    {
        stat = K2STAT_ERROR_ENABLED;
    }
    else
    {
        stat = Dev_NodeLocked_MountChild(apDevNode, pArgsIn, pArgsOut);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_RunAcpiMethod(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                      stat;
    K2OSDRV_RUN_ACPI_IN const * pIn;
    K2OSDRV_RUN_ACPI_OUT *      pOut;
    char                        acpiMethod[8];

    if ((apCall->Args.mInBufByteCount != sizeof(K2OSDRV_RUN_ACPI_IN)) ||
        (apCall->Args.mOutBufByteCount != sizeof(K2OSDRV_RUN_ACPI_OUT)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pIn = (K2OSDRV_RUN_ACPI_IN const *)apCall->Args.mpInBuf;
    pOut = (K2OSDRV_RUN_ACPI_OUT *)apCall->Args.mpOutBuf;

    K2MEM_Copy(acpiMethod, &pIn->mMethod, sizeof(UINT32));
    acpiMethod[4] = 0;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    stat = Dev_NodeLocked_RunAcpiMethod(apDevNode, pIn, &pOut->mResult);
    if (!K2STAT_IS_ERROR(stat))
    {
        *apRetUsedOutBytes = sizeof(K2OSDRV_RUN_ACPI_OUT);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_SetEnable(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                          stat;
    K2OSDRV_SET_ENABLE_IN const *   pArgsIn;

    if ((apCall->Args.mInBufByteCount != sizeof(K2OSDRV_SET_ENABLE_IN)) ||
        (apCall->Args.mOutBufByteCount != 0))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pArgsIn = (K2OSDRV_SET_ENABLE_IN const *)apCall->Args.mpInBuf;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    if (pArgsIn->mSetEnable)
    {
        stat = Dev_NodeLocked_Enable(apDevNode);
    }
    else
    {
        stat = Dev_NodeLocked_Disable(apDevNode, DevNodeIntent_DisableOnly);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_AddChildRes(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                              stat;
    K2OSDRV_ADD_CHILD_RES_IN const *    pArgsIn;

    if ((apCall->Args.mInBufByteCount != sizeof(K2OSDRV_ADD_CHILD_RES_IN)) ||
        (apCall->Args.mOutBufByteCount != 0))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pArgsIn = (K2OSDRV_ADD_CHILD_RES_IN const *)apCall->Args.mpInBuf;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    stat = Dev_NodeLocked_AddChildRes(apDevNode, pArgsIn);

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_NewDevUser(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                          stat;
    K2OSDRV_NEW_DEV_USER_IN const * pArgsIn;
    K2OSDRV_NEW_DEV_USER_OUT *      pArgsOut;

    if ((apCall->Args.mInBufByteCount != sizeof(K2OSDRV_NEW_DEV_USER_IN)) ||
        (apCall->Args.mOutBufByteCount != sizeof(K2OSDRV_NEW_DEV_USER_OUT)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pArgsIn = (K2OSDRV_NEW_DEV_USER_IN const *)apCall->Args.mpInBuf;
    pArgsOut = (K2OSDRV_NEW_DEV_USER_OUT *)apCall->Args.mpOutBuf;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    stat = Dev_NodeLocked_NewDevUser(apDevNode, pArgsIn, pArgsOut);

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_AcceptDevUser(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    DEVNODE *                   apDevNode
)
{
    K2STAT                              stat;
    K2OSDRV_ACCEPT_DEV_USER_IN const *  pArgsIn;

    if ((apCall->Args.mInBufByteCount != sizeof(K2OSDRV_ACCEPT_DEV_USER_IN)) ||
        (apCall->Args.mOutBufByteCount != 0))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pArgsIn = (K2OSDRV_ACCEPT_DEV_USER_IN const *)apCall->Args.mpInBuf;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    stat = Dev_NodeLocked_AcceptDevUser(apDevNode, pArgsIn);

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
Driver_HostObj_Call(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes
)
{
    DEVNODE *  pDevNode;
    K2STAT     stat;

    pDevNode = (DEVNODE *)apCall->mCreatorRef;

    stat = K2STAT_ERROR_UNKNOWN;
    switch (apCall->Args.mMethodId)
    {
    case K2OSDRV_HOSTOBJECT_METHOD_SET_MAILSLOT:
        stat = Driver_HostObj_SetMailbox(apCall, apRetUsedOutBytes, pDevNode);
        break;

    case K2OSDRV_HOSTOBJECT_METHOD_ENUM_RES:
        stat = Driver_HostObj_EnumRes(apCall, apRetUsedOutBytes, pDevNode);
        break;

    case K2OSDRV_HOSTOBJECT_METHOD_GET_RES:
        stat = Driver_HostObj_GetRes(apCall, apRetUsedOutBytes, pDevNode);
        break;

    case K2OSDRV_HOSTOBJECT_METHOD_MOUNT_CHILD:
        stat = Driver_HostObj_MountChild(apCall, apRetUsedOutBytes, pDevNode);
        break;

    case K2OSDRV_HOSTOBJECT_METHOD_SET_ENABLE:
        stat = Driver_HostObj_SetEnable(apCall, apRetUsedOutBytes, pDevNode);
        break;

    case K2OSDRV_HOSTOBJECT_METHOD_RUN_ACPI:
        stat = Driver_HostObj_RunAcpiMethod(apCall, apRetUsedOutBytes, pDevNode);
        break;

    case K2OSDRV_HOSTOBJECT_METHOD_ADD_CHILD_RES:
        stat = Driver_HostObj_AddChildRes(apCall, apRetUsedOutBytes, pDevNode);
        break;

    case K2OSDRV_HOSTOBJECT_METHOD_NEW_DEV_USER:
        stat = Driver_HostObj_NewDevUser(apCall, apRetUsedOutBytes, pDevNode);
        break;

    case K2OSDRV_HOSTOBJECT_METHOD_ACCEPT_DEV_USER:
        stat = Driver_HostObj_AcceptDevUser(apCall, apRetUsedOutBytes, pDevNode);
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    }

    return stat;
}

K2STAT
Driver_HostObj_Delete(
    K2OSRPC_OBJECT_DELETE const * apDelete
)
{
    DEVNODE *  pDevNode;

    pDevNode = (DEVNODE *)apDelete->mCreatorRef;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    Dev_NodeLocked_OnDriverHostObjDelete(pDevNode);

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return K2STAT_NO_ERROR;
}

void
Driver_Init(
    void
)
{
    static K2OSRPC_OBJECT_CLASS const sDriverHostObjectClassId =
    {
        K2OSDRV_HOSTOBJECT_CLASS_ID,
        TRUE,
        Driver_HostObj_Create,
        Driver_HostObj_Call,
        Driver_HostObj_Delete

    };
    BOOL ok;

    ok = K2OS_CritSec_Init(&gDriverInstanceTreeSec);
    K2_ASSERT(ok);

    K2TREE_Init(&gDriverInstanceTree, NULL);

    //
    // ready to publish driver host object class availability
    //
    ok = K2OSRPC_ObjectClass_Publish(&sDriverHostObjectClassId, 0);
    K2_ASSERT(ok);
}

