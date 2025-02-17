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

#include "kern.h"

void *
KernFirm_ConvertPointer(
    void * aPtr
)
{
    UINT32                  entCount;
    K2EFI_MEMORY_DESCRIPTOR desc;
    UINT8 const *           pWork;
    UINT32                  ix;
    UINT32                  physStart;
    UINT32                  physEnd;

    entCount = gData.mpShared->LoadInfo.mEfiMapSize / gData.mpShared->LoadInfo.mEfiMemDescSize;
    pWork = (UINT8 const *)K2OS_KVA_EFIMAP_BASE;
    ix = 0;
    do
    {
        K2MEM_Copy(&desc, pWork, sizeof(K2EFI_MEMORY_DESCRIPTOR));

        physStart = (UINT32)desc.PhysicalStart;
        physEnd = physStart + (desc.NumberOfPages * K2_VA_MEMPAGE_BYTES);

        if (desc.Attribute & K2EFI_MEMORYFLAG_RUNTIME)
        {
            if ((((UINT32)aPtr) >= physStart) && (((UINT32)aPtr) < physEnd))
            {
                return (void *)((((UINT32)aPtr) - physStart) + ((UINT32)desc.VirtualStart));
            }
        }

        pWork += gData.mpShared->LoadInfo.mEfiMemDescSize;
        ix++;
    } while (--entCount > 0);

    return NULL;
}

void 
KernFirm_GetTime(
    K2OS_TIME *apRetTime
)
{
    BOOL                    disp;
    K2EFI_SYSTEM_TABLE *    pST;
    K2EFI_RUNTIME_SERVICES *pRT;
    K2EFI_TIME              time;
    K2EFI_TIME_CAPABILITIES timeCaps;
    K2EFI_STATUS            efiStatus;
    UINT64                  tick;

    pST = gData.mpShared->LoadInfo.mpEFIST;
    pRT = pST->RuntimeServices;

    disp = K2OSKERN_SeqLock(&gData.Firm.SeqLock);

    efiStatus = pRT->GetTime(&time, &timeCaps);

    K2OSKERN_SeqUnlock(&gData.Firm.SeqLock, disp);

    K2OSKERN_Debug("EFI says %08X time is %04d-%02d-%02d %02d:%02d:%02d\n",
        efiStatus,
        time.Year, time.Month, time.Day,
        time.Hour, time.Minute, time.Second);

    apRetTime->mYear = time.Year;
    apRetTime->mMonth = time.Month;
    apRetTime->mDay = time.Day;
    apRetTime->mTimeZoneId = 0;
    apRetTime->mHour = time.Hour;
    apRetTime->mMinute = time.Minute;
    apRetTime->mSecond = time.Second;

    KernArch_GetHfTimerTick(&tick);
    KernTimer_MsTickFromHfTick(&tick, &tick);
    apRetTime->mMillisecond = (UINT16)(tick % 1000);
}

void 
KernFirm_Init(
    void
)
{
    K2EFI_SYSTEM_TABLE *        pST;
    UINTN                       ix;
    K2EFI_CONFIG_TABLE *        pTab;

    K2OSKERN_SeqInit(&gData.Firm.SeqLock);

    pST = gData.mpShared->LoadInfo.mpEFIST;
    K2_ASSERT(NULL != pST);

    K2OSKERN_Debug("\nEFI ST at %08X reports %d configuration table entries at %08X:\n", pST, pST->NumberOfTableEntries, pST->ConfigurationTable);

    pTab = pST->ConfigurationTable;
    for (ix = 0; ix < pST->NumberOfTableEntries; ix++)
    {
        pTab->VendorTable = KernFirm_ConvertPointer(pTab->VendorTable);
        if (NULL != pTab->VendorTable)
        {
            K2OSKERN_Debug("    %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X %08X\n",
                pTab->VendorGuid.mData1,
                pTab->VendorGuid.mData2,
                pTab->VendorGuid.mData3,
                pTab->VendorGuid.mData4[0], pTab->VendorGuid.mData4[1], pTab->VendorGuid.mData4[2], pTab->VendorGuid.mData4[3],
                pTab->VendorGuid.mData4[4], pTab->VendorGuid.mData4[5], pTab->VendorGuid.mData4[6], pTab->VendorGuid.mData4[7],
                pTab->VendorTable
            );
        }
        pTab++;
    }

    K2OSKERN_Debug("\n");
}

BOOL
K2OSKERN_GetFirmwareTable(
    K2_GUID128 const *  apId,
    UINT32 *            apIoBytes,
    void *              apBuffer
)
{
    K2EFI_SYSTEM_TABLE *    pST;
    K2EFI_CONFIG_TABLE *    pTab;
    UINT32                  count;

    if ((NULL == apId) ||
        (NULL == apIoBytes))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pST = gData.mpShared->LoadInfo.mpEFIST;
    
    count = pST->NumberOfTableEntries;
    
    if (0 == count)
        return FALSE;
    
    pTab = pST->ConfigurationTable;
    
    do
    {
        if (0 == K2MEM_Compare(&pTab->VendorGuid, apId, sizeof(K2_GUID128)))
        {
            if ((*apIoBytes) < sizeof(void *))
            {
                *apIoBytes = sizeof(void *);
                K2OS_Thread_SetLastStatus(K2STAT_ERROR_TOO_SMALL);
                return FALSE;
            }

            if (NULL == apBuffer)
            {
                K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
                return FALSE;
            }

            *apIoBytes = sizeof(void *);
            K2MEM_Copy(apBuffer, &pTab->VendorTable, sizeof(void *));
            return TRUE;
        }

        pTab++;

    } while (--count);

    return FALSE;
}


#if 0

K2_GUID128 gQemuQuadPeiBootCardMediaGuid = { 0xde4c4e86, 0x5e7f, 0x4a68, { 0x99, 0x6f, 0xd4, 0x79, 0xa4, 0xe6, 0xba, 0xc5 } };

K2EFI_BLOCK_IO_PROTOCOL *   pProto;

UINT8 * pAlloc;
UINT8 * pSector;

UINT32 ixProto;

pProto = NULL; ixProto = (UINT32)-1;

if (0 == K2MEM_Compare(&pTab->VendorGuid, &gQemuQuadPeiBootCardMediaGuid, sizeof(K2_GUID128)))
{
    ixProto = ix;
    pProto = (K2EFI_BLOCK_IO_PROTOCOL *)pTab->VendorTable;
}

if (NULL != pProto)
{
    K2OSKERN_Debug("StoreRunDxe proto found at index %d, ptr %08X\n", ixProto, pProto);
    K2OSKERN_Debug("  rev %08X%08X\n", (UINT32)(pProto->Revision >> 32), (UINT32)(pProto->Revision & 0xFFFFFFFFull));
    K2OSKERN_Debug("  Reset @ %08X\n", pProto->Reset);
    K2OSKERN_Debug("  Media @ %08X\n", pProto->Media);
    K2OSKERN_Debug("  Media.Id = %d\n", pProto->Media->MediaId);

    pAlloc = (UINT8 *)KernHeap_Alloc(pProto->Media->BlockSize * 2);
    pSector = (UINT8 *)(((((UINT32)pAlloc) + (pProto->Media->BlockSize - 1)) / pProto->Media->BlockSize) * pProto->Media->BlockSize);
    K2OSKERN_Debug("  SectorBuffer %08X\n", pSector);

    efiStatus = pProto->ReadBlocks(pProto, pProto->Media->MediaId, 1, pProto->Media->BlockSize, pSector);
    K2OSKERN_Debug("Read block 1 status = %08X\n", efiStatus);

    for (ix = 0; ix < 16; ix++)
    {
        K2OSKERN_Debug(" %02X", pSector[ix]);
    }
    K2OSKERN_Debug("\n");
}

#endif
