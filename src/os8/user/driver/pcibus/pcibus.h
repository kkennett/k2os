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
#ifndef __PCIBUS_H
#define __PCIBUS_H

#include <k2osdrv.h>
#include <k2osrpc.h>

/* ------------------------------------------------------------------------- */

typedef struct _PCIBUS_CHILD PCIBUS_CHILD;
struct _PCIBUS_CHILD
{
    K2_DEVICE_IDENT     Ident;

    UINT_PTR            mSystemDeviceInstanceId;

    K2PCI_DEVICE_LOC    DeviceLoc;

    struct {
        K2OS_MAP_TOKEN      mMapToken;
        PCICFG volatile *   mpCfg;
    } ECAM;

    UINT_PTR            mObjectId;
    UINT_PTR            mOwnUsageId;

    BOOL                mResOk;
    UINT_PTR            mBarsFoundCount;
    UINT_PTR            mBarVal[6];
    UINT_PTR            mBarSize[6];

    K2TREE_NODE         ChildTreeNode;  // userval is ((loc.device << 16) | loc.function)
};

extern UINT16               gPciSegNum;
extern UINT16               gPciBusNum;
extern BOOL                 gUseECAM;
extern K2OS_PAGEARRAY_TOKEN gTokBusPageArray;
extern K2OSDRV_RES_IO       gBusIo;
extern K2OS_CRITSEC         gPCIBUS_ChildTreeSec;
extern K2TREE_ANCHOR        gPCIBUS_ChildTree;
extern UINT32               gECAMBaseAddr;

extern K2OSRPC_OBJECT_CLASS const gPciBusChild_ObjectClass;

/* ------------------------------------------------------------------------- */

void    PCIBUS_InitAndDiscover(void);

#if K2_TARGET_ARCH_IS_INTEL

typedef K2STAT(*PCIBUS_pf_ConfigRead)(K2PCI_DEVICE_LOC *apLoc, UINT32 aReg, UINT64 *apRetValue, UINT32 aWidth);
typedef K2STAT(*PCIBUS_pf_ConfigWrite)(K2PCI_DEVICE_LOC *apLoc, UINT32 aReg, UINT64 const *apValue, UINT32 aWidth);

extern PCIBUS_pf_ConfigRead     PCIBUS_ConfigRead;
extern PCIBUS_pf_ConfigWrite    PCIBUS_ConfigWrite;

#else

K2STAT  PCIBUS_ConfigRead(K2PCI_DEVICE_LOC *apLoc, UINT32 aReg, UINT64 *apRetValue, UINT32 aWidth);
K2STAT  PCIBUS_ConfigWrite(K2PCI_DEVICE_LOC *apLoc, UINT32 aReg, UINT64 const *apValue, UINT32 aWidth);

#endif

K2STAT  PCIBUS_WriteBAR(PCIBUS_CHILD *apChild, UINT_PTR aBarIx, UINT32 aValue);
K2STAT  PCIBUS_ReadBAR(PCIBUS_CHILD *apChild, UINT_PTR aBarIx, UINT32 *apRetVal);

/* ------------------------------------------------------------------------- */

BOOL    PCIBUS_CheckOverrideRes(PCIBUS_CHILD *apChild);

/* ------------------------------------------------------------------------- */

#endif // __PCIBUS_H