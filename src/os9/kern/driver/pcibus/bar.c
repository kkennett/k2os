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
PciBus_WriteBAR(
    PCIBUS_CHILD *  apChild,
    UINT32          aBarIx,
    UINT32          aValue
)
{
    PCIBUS *    pPciBus;
    K2STAT      stat;
    UINT64      val64;

    pPciBus = apChild->mpParent;
    val64 = aValue;
    stat = pPciBus->mfConfigWrite(
        pPciBus, 
        &apChild->DeviceLoc, 
        PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * aBarIx), 
        &val64, 
        32
    );
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to write child %04X/%04X bar reg %d in config space (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, aBarIx, stat);
    }
    return stat;
}

K2STAT
PciBus_ReadBAR(
    PCIBUS_CHILD *  apChild,
    UINT32          aBarIx,
    UINT32 *        apRetVal
)
{
    PCIBUS *    pPciBus;
    K2STAT      stat;
    UINT64      val64;

    pPciBus = apChild->mpParent;
    stat = pPciBus->mfConfigRead(
        pPciBus, 
        &apChild->DeviceLoc, 
        PCI_CONFIG_TYPE0_OFFSET_BAR0 + (sizeof(UINT32) * aBarIx), 
        &val64, 
        32
    );
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** PciBus(%d,%d): Failed to read child %04X/%04X bar reg %d from config space (%08X)\n", pPciBus->mSegNum, pPciBus->mBusNum, apChild->Ident.mVendorId, apChild->Ident.mDeviceId, aBarIx, stat);
        *apRetVal = 0;
    }
    else
    {
        *apRetVal = (UINT32)val64;
    }
    return stat;
}
