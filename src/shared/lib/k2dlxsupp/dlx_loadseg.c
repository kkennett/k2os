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
#include "idlx.h"

K2STAT
iK2DLXSUPP_LoadSegments(
    void *  apAcqContext,
    DLX *   apDlx
    )
{
    Elf32_Ehdr *        pElf;
    Elf32_Shdr *        pSecHdr;
    UINT32              segIx;
    UINT32              secIx;
    UINT32              count;
    K2STAT              status;
    DLX_INFO *          pInfo;
    UINT32              totalSpace;
    K2DLX_SECTOR *      pSector;
    UINT32              startAddr;
    UINT32              endAddr;
    UINT32              linkAddr;
    UINT32              secSegOffset;

    pElf = apDlx->mpElf;
    pSecHdr = apDlx->mpSecHdr;
    pInfo = apDlx->mpInfo;
    pSector = K2_GET_CONTAINER(K2DLX_SECTOR, apDlx, Module);
    K2MEM_Zero(&pSector->mSecAddr[3], (pElf->e_shnum - 3) * sizeof(UINT32));
    K2MEM_Zero(&pSector->mSecAddr[3 + pElf->e_shnum], (pElf->e_shnum - 3) * sizeof(UINT32));

    secIx = 3;

    for (segIx = DlxSeg_Text; segIx < DlxSeg_Count; segIx++)
    {
        count = K2_ROUNDUP(pInfo->SegInfo[segIx].mFileBytes, DLX_SECTOR_BYTES);
        if (count > 0)
        {
            count /= DLX_SECTOR_BYTES;

            if ((apDlx->mCurSector * DLX_SECTOR_BYTES) != pInfo->SegInfo[segIx].mFileOffset)
                return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);

            status = gpK2DLXSUPP_Vars->Host.ReadSectors(
                apAcqContext,
                apDlx->mHostFile, 
                (void *)apDlx->SegAlloc.Segment[segIx].mDataAddr, 
                count);

            if (K2STAT_IS_ERROR(status))
                return K2DLXSUPP_ERRORPOINT(status);

            if (pInfo->SegInfo[segIx].mCRC32 !=
                K2CRC_Calc32(0, (void const *)apDlx->SegAlloc.Segment[segIx].mDataAddr, pInfo->SegInfo[segIx].mFileBytes))
                return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_CRC_WRONG);

            apDlx->mCurSector += count;

            if (segIx == DlxSeg_Data)
            {
                // take care of zeroing out BSS
                count *= DLX_SECTOR_BYTES;
                totalSpace = K2_ROUNDUP(pInfo->SegInfo[DlxSeg_Data].mMemActualBytes, K2_VA32_MEMPAGE_BYTES);
                totalSpace -= count;
                if (totalSpace > 0)
                    K2MEM_Zero((void *)(apDlx->SegAlloc.Segment[segIx].mDataAddr + count), totalSpace);
            }

            startAddr = pInfo->SegInfo[segIx].mLinkAddr;
            endAddr = startAddr + pInfo->SegInfo[segIx].mMemActualBytes;
            while ((pSecHdr[secIx].sh_addr >= startAddr) &&
                    (pSecHdr[secIx].sh_addr < endAddr))
            {
                // addralign is segment index for section
                pSecHdr[secIx].sh_addralign = segIx;
                secSegOffset = pSecHdr[secIx].sh_addr - startAddr;
                pSector->mSecAddr[secIx] = secSegOffset + apDlx->SegAlloc.Segment[segIx].mDataAddr;
                linkAddr = apDlx->SegAlloc.Segment[segIx].mLinkAddr;
                if (linkAddr != 0)
                    pSector->mSecAddr[secIx + apDlx->mpElf->e_shnum] = secSegOffset + linkAddr;
                secIx++;
            }
        }
    }

    // 
    // confirm we have done all sections, and all nonempty sections have addresses
    //
    if (secIx != pElf->e_shnum)
        return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
    for (secIx = 3; secIx < pElf->e_shnum; secIx++)
    {
        if ((pSecHdr[secIx].sh_size > 0) && 
            ((pSector->mSecAddr[secIx] == 0) || 
             (pSecHdr[secIx].sh_addralign < DlxSeg_Text) ||
             (pSecHdr->sh_addralign >= DlxSeg_Count)))
            return K2DLXSUPP_ERRORPOINT(K2STAT_DLX_ERROR_FILE_CORRUPTED);
    }

    return 0;
}
