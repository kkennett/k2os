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
#include <lib/fatutil/cfatutil.h>
#include <lib/memory/cmemory.h>


errCode
FATUTIL_GetNextCluster(
    FATUTIL_INFO const *apFATInfo
,   UINT8 const *       apFATData
,   UINT32              aClusterNumber
,   UINT32 *            apRetNextCluster
)
{
    if ((!apFATInfo) || (!apFATData) || (!apRetNextCluster))
        return ERRCODE_BADARG;
    if ((apFATInfo->mFATFormat < FAT_FSFORMAT_FAT12) ||
        (apFATInfo->mFATFormat > FAT_FSFORMAT_FAT32))
        return ERRCODE_INVALIDDATA;
    if ((aClusterNumber < 2) ||
        (aClusterNumber >= (apFATInfo->mCountOfClusters+2)))
        return ERRCODE_BADARG;

    switch (apFATInfo->mFATFormat)
    {
    case FAT_FSFORMAT_FAT12:
        return sNextFat12Cluster(apFATData,aClusterNumber,apRetNextCluster);
    case FAT_FSFORMAT_FAT16:
        return sNextFat16Cluster((UINT16 const *)apFATData,aClusterNumber,apRetNextCluster);
    case FAT_FSFORMAT_FAT32:
        return sNextFat32Cluster((UINT32 const *)apFATData,aClusterNumber,apRetNextCluster);
    }

    return ERRCODE_UNKNOWN;
}
