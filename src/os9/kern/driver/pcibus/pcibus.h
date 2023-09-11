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

#ifndef __PciBus_H
#define __PciBus_H

#include <k2osddk.h>
#include <spec/k2pci.h>

#if __cplusplus
extern "C" {
#endif

//
// -------------------------------------------------------------------------
// 

typedef struct _PCIBUS PCIBUS;

typedef K2STAT(*PciBus_pf_ConfigRead)(PCIBUS *apPciBus, K2PCI_DEVICE_LOC *apLoc, UINT32 aReg, UINT64 *apRetValue, UINT32 aWidth);
typedef K2STAT(*PciBus_pf_ConfigWrite)(PCIBUS *apPciBus, K2PCI_DEVICE_LOC *apLoc, UINT32 aReg, UINT64 const *apValue, UINT32 aWidth);

struct _PCIBUS
{
    K2OS_DEVCTX             mDevCtx;
    K2_DEVICE_IDENT         Ident;
    UINT32                  mCountIo;
    UINT32                  mCountPhys;
    UINT32                  mCountIrq;
    UINT16                  mSegNum;
    UINT16                  mBusNum;

    K2OSDDK_RES             BusPhysRes;
    BOOL                    mUseECAM;
    UINT32                  mECAMBaseAddr;

    K2OSDDK_RES             BusIoRes;
    
    K2OS_CRITSEC            ChildTreeSec;
    K2TREE_ANCHOR           ChildTree;

    K2OS_THREAD_TOKEN       mTokThread;
    UINT32                  mThreadId;

    PciBus_pf_ConfigRead    mfConfigRead;
    PciBus_pf_ConfigWrite   mfConfigWrite;

    K2OS_MAILBOX_TOKEN      mTokMailbox;
};

typedef struct _PCIBUS_CHILD PCIBUS_CHILD;
struct _PCIBUS_CHILD
{
    PCIBUS *            mpParent;

    K2_DEVICE_IDENT     Ident;

    UINT32              mInstanceId;

    K2PCI_DEVICE_LOC    DeviceLoc;

    struct {
        K2OS_VIRTMAP_TOKEN  mTokVirtMap;
        PCICFG volatile *   mpCfg;
    } ECAM;

    BOOL                mResOk;
    UINT32              mBarsFoundCount;
    UINT32              mBarVal[6];
    UINT32              mBarSize[6];

    K2TREE_NODE         ChildTreeNode;  // userval is ((loc.device << 16) | loc.function)
};

UINT32  PciBus_Instance_Thread(PCIBUS *apPciBus);

void    PciBus_InitAndDiscover(PCIBUS *apPciBus);
void    PciBus_MountChild(PCIBUS_CHILD *apChild);

//
// -------------------------------------------------------------------------
// 

K2STAT  PciBus_WriteBAR(PCIBUS_CHILD *apChild, UINT32 aBarIx, UINT32 aValue);
K2STAT  PciBus_ReadBAR(PCIBUS_CHILD *apChild, UINT32 aBarIx, UINT32 *apRetVal);

//
// -------------------------------------------------------------------------
// 

BOOL    PciBus_CheckOverrideRes(PCIBUS_CHILD *apChild);

//
// -------------------------------------------------------------------------
// 

#if __cplusplus
}
#endif



#endif // __PciBus_H