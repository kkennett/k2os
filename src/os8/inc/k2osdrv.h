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
#ifndef __K2OSDRV_H
#define __K2OSDRV_H

#include "k2os.h"
#include <spec/acpi.h>
#include <spec/k2pci.h>
#include <spec/smbios.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

#define K2OS_MSG_CONTROL_DRV_CLASS      0x70000000 

#define K2OS_MSG_CONTROL_DRV(x)         (K2OS_MSG_CONTROL_DRV_CLASS | ((x) & 0x0FFFFFFF))

#define K2OS_MSG_CONTROL_DRV_START      K2OS_MSG_CONTROL_DRV(0)
#define K2OS_MSG_CONTROL_DRV_FACTORY    K2OS_MSG_CONTROL_DRV(1)

//
//------------------------------------------------------------------------
//

// {4C5DC782-D61B-4471-BDBF-BC89A1562316}
#define K2OS_IFACE_ID_DEVICE_BUS                { 0x4c5dc782, 0xd61b, 0x4471, { 0xbd, 0xbf, 0xbc, 0x89, 0xa1, 0x56, 0x23, 0x16 } }

#define K2OSDRV_BUSTYPE_INVALID     0
#define K2OSDRV_BUSTYPE_CPU         1
#define K2OSDRV_BUSTYPE_PCI         2
#define K2OSDRV_BUSTYPE_USB         3
#define K2OSDRV_BUSTYPE_IDE         4
#define K2OSDRV_BUSTYPE_COUNT       5

// {A5EFEA0E-0F07-4071-A659-61B7C420AF1C}
#define K2OSDRV_HOSTOBJECT_CLASS_ID             { 0xa5efea0e, 0xf07, 0x4071, { 0xa6, 0x59, 0x61, 0xb7, 0xc4, 0x20, 0xaf, 0x1c } }

#define K2OSDRV_HOSTOBJECT_METHOD_SET_MAILSLOT 1
typedef struct _K2OSDRV_SET_MAILSLOT_IN K2OSDRV_SET_MAILSLOT_IN;
struct _K2OSDRV_SET_MAILSLOT_IN
{
    K2OS_MAILSLOT_TOKEN mMailslotToken;
};

#define K2OSDRV_HOSTOBJECT_METHOD_ENUM_RES     2
typedef struct _K2OSDRV_ENUM_RES_OUT K2OSDRV_ENUM_RES_OUT;
struct _K2OSDRV_ENUM_RES_OUT
{
    UINT32  mAcpiInfoBytes;
    UINT32  mCountIo;
    UINT32  mCountPhys;
    UINT32  mCountIrq;
};

#define K2OSDRV_RESTYPE_ACPI    0
#define K2OSDRV_RESTYPE_IO      1
#define K2OSDRV_RESTYPE_PHYS    2
#define K2OSDRV_RESTYPE_IRQ     3

#define K2OSDRV_HOSTOBJECT_METHOD_GET_RES  3
typedef struct _K2OSDRV_GET_RES_IN K2OSDRV_GET_RES_IN;
struct _K2OSDRV_GET_RES_IN
{
    UINT32  mResType;
    UINT32  mIndex;
};

typedef K2OS_IOPORT_RANGE K2OSDRV_RES_IO;

typedef struct _K2OSDRV_RES_PHYS K2OSDRV_RES_PHYS;
struct _K2OSDRV_RES_PHYS
{
    K2OS_PHYSADDR_RANGE     Range;
    K2OS_PAGEARRAY_TOKEN    mPageArrayToken;
};

typedef struct _K2OSDRV_RES_IRQ K2OSDRV_RES_IRQ;
struct _K2OSDRV_RES_IRQ
{
    K2OS_IRQ_CONFIG         Config;
    K2OS_INTERRUPT_TOKEN    mInterruptToken;
};

#define K2OSDRV_HOSTOBJECT_METHOD_MOUNT_CHILD       4

#define K2OSDRV_MOUNTFLAGS_BUSTYPE_MASK     0xFF

typedef struct _K2OSDRV_MOUNT_CHILD_IN K2OSDRV_MOUNT_CHILD_IN;
struct _K2OSDRV_MOUNT_CHILD_IN
{
    UINT32          mChildObjectId;
    UINT32          mMountFlags;
    UINT64          mBusSpecificAddress;
    K2_DEVICE_IDENT Ident;
};

typedef struct _K2OSDRV_MOUNT_CHILD_OUT K2OSDRV_MOUNT_CHILD_OUT;
struct _K2OSDRV_MOUNT_CHILD_OUT
{
    UINT_PTR    mSystemDeviceInstanceId;
};

#define K2OSDRV_HOSTOBJECT_METHOD_SET_ENABLE        5

typedef struct _K2OSDRV_SET_ENABLE_IN K2OSDRV_SET_ENABLE_IN;
struct _K2OSDRV_SET_ENABLE_IN
{
    BOOL    mSetEnable;
};

#define K2OSDRV_HOSTOBJECT_METHOD_RUN_ACPI         6

#define K2OSDRV_RUN_ACPI_FLAG_HAS_INPUT     1

typedef struct _K2OSDRV_RUN_ACPI_IN K2OSDRV_RUN_ACPI_IN;
struct _K2OSDRV_RUN_ACPI_IN
{
    UINT32  mMethod;
    UINT32  mFlags;
    UINT32  mIn;
};

typedef struct _K2OSDRV_RUN_ACPI_OUT K2OSDRV_RUN_ACPI_OUT;
struct _K2OSDRV_RUN_ACPI_OUT
{
    UINT32  mResult;
};

#define K2OSDRV_HOSTOBJECT_METHOD_ADD_CHILD_RES     7

typedef struct _K2OSDRV_ADD_CHILD_RES_IN K2OSDRV_ADD_CHILD_RES_IN;
struct _K2OSDRV_ADD_CHILD_RES_IN
{
    UINT32  mChildObjectId;
    UINT32  mResType;
    union {
        K2OS_PHYSADDR_RANGE PhysRange;
        K2OS_IRQ_CONFIG     IrqConfig;
        K2OS_IOPORT_RANGE   IoPortRange;
    } Desc;
};

#define K2OSDRV_HOSTOBJECT_METHOD_NEW_DEV_USER      8

typedef struct _K2OSDRV_NEW_DEV_USER_IN K2OSDRV_NEW_DEV_USER_IN;
struct _K2OSDRV_NEW_DEV_USER_IN
{
    UINT32  mSystemDeviceInstanceId;
    UINT32  mUserObjectId;
};

typedef struct _K2OSDRV_NEW_DEV_USER_OUT K2OSDRV_NEW_DEV_USER_OUT;
struct _K2OSDRV_NEW_DEV_USER_OUT
{
    UINT32              mUserProcessId;
    K2OS_MAILSLOT_TOKEN mUserMailslot;
};

#define K2OSDRV_HOSTOBJECT_METHOD_ACCEPT_DEV_USER   9

typedef struct _K2OSDRV_ACCEPT_DEV_USER_IN K2OSDRV_ACCEPT_DEV_USER_IN;
struct _K2OSDRV_ACCEPT_DEV_USER_IN
{
    UINT32  mUserObjectId;
    BOOL    mSetAccept;
};


// {2CFFA285-8BB5-44A3-B8AD-D99FABC18120}
#define K2OSDRV_CLIENTOBJECT_CLASS_ID             { 0x2cffa285, 0x8bb5, 0x44a3, { 0xb8, 0xad, 0xd9, 0x9f, 0xab, 0xc1, 0x81, 0x20 } }

#define K2OSDRV_CLIENTOBJECT_METHOD_1       1




//
//------------------------------------------------------------------------
//

// {EFD22518-B2C9-4990-B08B-54252F61203B}
#define K2OSDRV_CHILDOBJECT_CLASS_ID           { 0xefd22518, 0xb2c9, 0x4990, { 0xb0, 0x8b, 0x54, 0x25, 0x2f, 0x61, 0x20, 0x3b } }

//
//------------------------------------------------------------------------
//

typedef UINT_PTR (*K2OSDrv_DriverThread)(UINT_PTR aBusDriverProcessId, UINT_PTR aBusDriverChildObjectId);

//
//------------------------------------------------------------------------
//

UINT_PTR
K2OSDrv_DebugPrintf(
    char const *apFormat,
    ...
);

K2STAT
K2OSDrv_EnumRes(
    K2OSDRV_ENUM_RES_OUT * apEnum
);

K2STAT
K2OSDrv_GetAcpiInfo(
    char *      apOutBuf,
    UINT_PTR    aOutBufBytes
);

K2STAT
K2OSDrv_GetResIo(
    UINT_PTR            aIndex,
    K2OSDRV_RES_IO *    apRetResIo
);

K2STAT
K2OSDrv_GetResPhys(
    UINT_PTR            aIndex,
    K2OSDRV_RES_PHYS *  apRetResPhys
);

K2STAT
K2OSDrv_GetResIrq(
    UINT_PTR            aIndex,
    K2OSDRV_RES_IRQ *   apRetResIrq
);

K2STAT
K2OSDrv_MountChildDevice(
    UINT_PTR                aChildObjectId,
    UINT_PTR                aMountFlags,
    UINT64 const *          apBusSpecificAddress,
    K2_DEVICE_IDENT const * apIdent,
    UINT_PTR *              apRetSystemDeviceInstanceId
);

K2STAT
K2OSDrv_SetEnable(
    BOOL aSetEnable
);

K2STAT
K2OSDRV_RunAcpiMethod(
    char const *apMethodName,
    UINT_PTR    aFlags,
    UINT_PTR    aIn,
    UINT_PTR *  apOut
);

K2STAT
K2OSDRV_AddChildPhys(
    UINT_PTR        aChildObjectId,
    UINT64 const *  apPhysBase,
    UINT64 const *  aSizeBytes
);

K2STAT
K2OSDRV_AddChildIo(
    UINT_PTR        aChildObjectId,
    UINT_PTR        aIoBase,
    UINT_PTR        aIoSizeBytes
);

K2STAT
K2OSDRV_AddChildIrq(
    UINT_PTR                aChildObjectId,
    K2OS_IRQ_CONFIG const * apIrqConfig
);

//
//------------------------------------------------------------------------
//

K2STAT
K2OSDRV_NewDevUser(
    UINT_PTR                aSystemDeviceInstanceId,
    UINT_PTR                aUserObjectId,
    UINT_PTR *              apRetUserProc,
    K2OS_MAILSLOT_TOKEN *   apRetUserMailslot
);

K2STAT
K2OSDRV_AcceptDevUser(
    UINT_PTR    aUserObjectId,
    BOOL        aSetAccept
);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSDRV_H