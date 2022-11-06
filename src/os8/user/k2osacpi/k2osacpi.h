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

#ifndef __K2OSACPI_H
#define __K2OSACPI_H

#include <k2os.h>
#include <acenv.h>
#include <actypes.h>
#include <actbl.h>
#include <acpiosxf.h>
#include <acrestyp.h>
#include <acpixf.h>
#include <acexcep.h>
#include <aclocal.h>
#include <acobject.h>

typedef
ACPI_STATUS
(*K2OSACPI_pf_ReadPciConfiguration)(
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width
    );

typedef
ACPI_STATUS
(*K2OSACPI_pf_WritePciConfiguration)(
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  Value,
    UINT32                  Width
    );

K2STAT
K2OSACPI_SetPciConfigOverride(
    K2OSACPI_pf_ReadPciConfiguration    afRead,
    K2OSACPI_pf_WritePciConfiguration      afWrite
);

ACPI_STATUS
K2OSACPI_RunDeviceNumericMethod(
    ACPI_HANDLE     Device,
    char const *    MethodName,
    UINT64 *        apRetValue
);

typedef struct _K2OSACPI_PREDFINED_NAME K2OSACPI_PREDFINED_NAME;
struct _K2OSACPI_PREDFINED_NAME
{
    char            *Name;
    char            *Description;
};
extern const K2OSACPI_PREDFINED_NAME AslPredefinedInfo[];

typedef struct _K2OSACPI_DEVICE_ID K2OSACPI_DEVICE_ID;
struct _K2OSACPI_DEVICE_ID
{
    char            *Name;
    char            *Description;
};
extern const K2OSACPI_DEVICE_ID AslDeviceIds[];

extern K2LIST_ANCHOR    gK2OSACPI_IntrList;
extern K2LIST_ANCHOR    gK2OSACPI_IntrFreeList;
extern K2OS_CRITSEC     gK2OSACPI_CacheSec;
extern K2LIST_ANCHOR    gK2OSACPI_CacheList;

typedef K2OS_INTERRUPT_TOKEN (K2_CALLCONV_REGS *K2OS_pf_Root_HookIntr)(K2OS_IRQ_CONFIG const *apConfig);

typedef struct _K2OSACPI_INIT K2OSACPI_INIT;
struct _K2OSACPI_INIT
{
    UINT_PTR                mFwBasePhys;
    UINT_PTR                mFwBaseVirt;
    UINT_PTR                mFwSizeBytes;
    UINT_PTR                mFacsPhys;
    UINT_PTR                mFacsVirt;
    UINT_PTR                mXFacsPhys;
    UINT_PTR                mXFacsVirt;
    K2OS_pf_Root_HookIntr   mfHookIntr;
};

void K2OSACPI_Init(K2OSACPI_INIT const *apInit);

typedef void (*K2OSACPI_pf_EnumCallback)(ACPI_HANDLE ahParent, ACPI_HANDLE ahChild, UINT_PTR aContext);
ACPI_STATUS K2OSACPI_EnumChildren(ACPI_HANDLE ahParent, K2OSACPI_pf_EnumCallback aCallback, UINT_PTR aContext);

UINT32      K2OSACPI_RunSTA(ACPI_HANDLE DeviceObject);

#endif // __K2OSACPI_H