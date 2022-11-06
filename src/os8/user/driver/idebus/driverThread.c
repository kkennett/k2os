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

#include "idebus.h"

K2OSDRV_RES_IO  gBusIo;
UINT_PTR        gPopMask;
IDE_CHANNEL     gChannel[2];

K2STAT IDEBUS_ChildObject_Create(K2OSRPC_OBJECT_CREATE const *apCreate, UINT_PTR *apRetCreator);
K2STAT IDEBUS_ChildObject_Call(K2OSRPC_OBJECT_CALL const *apCall, UINT_PTR *apRetUsedOutBytes);
K2STAT IDEBUS_ChildObject_Delete(K2OSRPC_OBJECT_DELETE const *apDelete);

K2OSRPC_OBJECT_CLASS const gIdeBusChild_ObjectClass =
{
    K2OSDRV_CHILDOBJECT_CLASS_ID,
    FALSE,
    IDEBUS_ChildObject_Create,
    IDEBUS_ChildObject_Call,
    IDEBUS_ChildObject_Delete
};

K2STAT 
IDEBUS_ChildObject_Create(
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
IDEBUS_ChildObject_Call(
    K2OSRPC_OBJECT_CALL const * apCall, 
    UINT_PTR *                  apRetUsedOutBytes
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
IDEBUS_ChildObject_Delete(
    K2OSRPC_OBJECT_DELETE const *apDelete
)
{
    IDE_DEVICE *pChild;

    pChild = (IDE_DEVICE *)apDelete->mCreatorRef;

    pChild->mObjectId = 0;
    pChild->mMountOk = FALSE;

    return K2STAT_NO_ERROR;
}

void
IDEBUS_MountChild(
    IDE_DEVICE *apChild
)
{
    K2STAT  stat;
    UINT64  address;

    address = apChild->mLocation;
    stat = K2OSDrv_MountChildDevice(apChild->mObjectId, K2OSDRV_BUSTYPE_IDE, &address, &apChild->Ident, &apChild->mSystemDeviceInstanceId);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** IdeBus(%d): Failed to mount child device loc %d (%08X)\n", K2OS_Process_GetId(), apChild->mLocation, stat);
        return;
    }

    apChild->mMountOk = TRUE;
}

UINT_PTR 
DriverThread(
    UINT_PTR aBusDriverProcessId,
    UINT_PTR aBusDriverChildObjectId
)
{
    K2STAT                  stat;
    K2OSDRV_ENUM_RES_OUT    resEnum;
    UINT_PTR                resIx;

    K2OSDrv_DebugPrintf("IdeBus(proc %d), bus driver proc %d, child object %d\n", K2OS_Process_GetId(), aBusDriverProcessId, aBusDriverChildObjectId);
    
    K2MEM_Zero(&resEnum, sizeof(resEnum));
    stat = K2OSDrv_EnumRes(&resEnum);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSDrv_DebugPrintf("*** IdeBus(%d): Enum resources failed (0x%08X)\n", K2OS_Process_GetId(), stat);
        return stat;
    }

    if (resEnum.mCountIo == 0)
    {
        K2OSDrv_DebugPrintf("*** IdeBus(%d): no usable io range found\n", K2OS_Process_GetId());
        return K2STAT_ERROR_NOT_EXIST;
    }

    for (resIx = 0; resIx < resEnum.mCountIo; resIx++)
    {
        K2MEM_Zero(&gBusIo, sizeof(gBusIo));

        stat = K2OSDrv_GetResIo(resIx, &gBusIo);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSDrv_DebugPrintf("*** IdeBus(%d): Could not get io resource 0 (0x%08X)\n", K2OS_Process_GetId(), stat);
            return stat;
        }

        if (gBusIo.mSizeBytes == 0x10)
            break;
    }

    if (resIx == resEnum.mCountIo)
    {
        K2OSDrv_DebugPrintf("!!! IdeBus(%d): Io resource(s) found but they are not the correct size for an IDE bus\n", K2OS_Process_GetId());
        return K2STAT_ERROR_NOT_FOUND;
    }

    //
    // publish the rpc object class for bus children
    //
    if (!K2OSRPC_ObjectClass_Publish(&gIdeBusChild_ObjectClass, 0))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2OSDrv_DebugPrintf("*** IdeBus(%d): Failed to publish RPC bus child object class (%08X)\n", K2OS_Process_GetId(), stat);
        return stat;
    }

    gPopMask = 0;

    stat = IDEBUS_InitAndDiscover();
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (0 == gPopMask)
    {
        K2OSDrv_DebugPrintf("*** IdeBus(%d): No devices found\n", K2OS_Process_GetId());
        return 0;
    }

    //
    // mount the bus children
    //
    for (resIx = 0; resIx < 4; resIx++)
    {
        if (gPopMask & (1 << resIx))
        {
            IDEBUS_MountChild(&gChannel[resIx >> 1].Device[resIx & 1]);
        }
    }

    //
    // enable
    //
    K2OSDrv_SetEnable(TRUE);

    do {
        K2OS_Thread_Sleep(1000);
        K2OSDrv_DebugPrintf("IdeBus_Tick()\n");
    } while (1);

    return 0;
}
