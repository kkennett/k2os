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

//
//------------------------------------------------------------------------
//

// {4C5DC782-D61B-4471-BDBF-BC89A1562316}
#define K2OS_IFACE_ID_DEVICE_BUS                { 0x4c5dc782, 0xd61b, 0x4471, { 0xbd, 0xbf, 0xbc, 0x89, 0xa1, 0x56, 0x23, 0x16 } }

#define K2OSDRV_BUSTYPE_INVALID     0
#define K2OSDRV_BUSTYPE_CPU         1
#define K2OSDRV_BUSTYPE_PCI         2
#define K2OSDRV_BUSTYPE_USB         3
#define K2OSDRV_BUSTYPE_COUNT       4

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

#define K2OSDRV_HOSTOBJECT_METHOD_GET_RESINFO  3
typedef struct _K2OSDRV_GET_RESINFO_IN K2OSDRV_GET_RESINFO_IN;
struct _K2OSDRV_GET_RESINFO_IN
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

#define K2OSDRV_HOSTOBJECT_METHOD_GET_PCIBUS_PARAM     4
typedef struct _K2OSDRV_PCIBUS_PARAM_OUT K2OSDRV_PCIBUS_PARAM_OUT;
struct _K2OSDRV_PCIBUS_PARAM_OUT
{
    K2PCI_BUS_ID            BusId;
    UINT_PTR                mBaseAddr;
    K2OS_PAGEARRAY_TOKEN    mPageArrayToken;
};

#define K2OSDRV_HOSTOBJECT_METHOD_MOUNT_CHILD      5

#define K2OSDRV_MOUNTFLAGS_BUSTYPE_MASK     0xFF

typedef struct _K2OSDRV_MOUNT_CHILD_IN K2OSDRV_MOUNT_CHILD_IN;
struct _K2OSDRV_MOUNT_CHILD_IN
{
    UINT32          mChildObjectId;
    UINT32          mMountFlags;
    UINT64          mBusSpecificAddress;
    K2_DEVICE_IDENT Ident;
};

#define K2OSDRV_HOSTOBJECT_METHOD_SET_ENABLE        6

typedef struct _K2OSDRV_SET_ENABLE_IN K2OSDRV_SET_ENABLE_IN;
struct _K2OSDRV_SET_ENABLE_IN
{
    BOOL    mSetEnable;
};

//
//------------------------------------------------------------------------
//

// {EFD22518-B2C9-4990-B08B-54252F61203B}
#define K2OSDRV_CHILDOBJECT_CLASS_ID           { 0xefd22518, 0xb2c9, 0x4990, { 0xb0, 0x8b, 0x54, 0x25, 0x2f, 0x61, 0x20, 0x3b } }

//
//------------------------------------------------------------------------
//

typedef UINT_PTR (*K2OSDrv_DriverThread)(char const *apArgs);

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
K2OSDrv_GetPciBusParam(
    K2OSDRV_PCIBUS_PARAM_OUT * apRetBusParam;
);

K2STAT
K2OSDrv_MountChildDevice(
    UINT_PTR                aChildObjectId,
    UINT_PTR                aMountFlags,
    UINT64 const *          apBusSpecificAddress,
    K2_DEVICE_IDENT const * apIdent
);

K2STAT
K2OSDrv_SetEnable(
    BOOL aSetEnable
);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSDRV_H