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
#ifdef _MSC_VER
#define ENABLE_DUMP 0
#define USE_STDIO   1
#else
#define ENABLE_DUMP 0
#define USE_STDIO   0
#endif

#if ENABLE_DUMP
#if USE_STDIO
#include <stdio.h>
#define     DUMPF   printf
#endif
#endif

#include "idlx.h"

#if ENABLE_DUMP

#if !USE_STDIO
UINT32 CrtDbg_Printf(char const *apFormat, ...);
#define     DUMPF   CrtDbg_Printf        
#endif

static char const * const sgpSegName[DlxSeg_Count] =
{
    "DLXINFO",
    "TEXT",
    "READ",
    "DATA",
    "SYM",
    "RELOC",
    "OTHER"
};

static void sPrintGuid(K2_GUID128 const *apGuid)
{
    DUMPF("{%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X}",
        apGuid->mData1, apGuid->mData2, apGuid->mData3,
        (((UINT16)(apGuid->mData4[0])) << 8) | ((UINT16)(apGuid->mData4[1])),
        apGuid->mData4[2], apGuid->mData4[3], apGuid->mData4[4],
        apGuid->mData4[5], apGuid->mData4[6], apGuid->mData4[7]);
}

static
void
sDumpSymTree(
    K2TREE_ANCHOR *apAnchor
    )
{
    K2DLX_SYMTREE_NODE *  pSymTreeNode;
    K2TREE_NODE *         pTreeNode;

    pTreeNode = K2TREE_FirstNode(apAnchor);
    K2_ASSERT(pTreeNode != NULL);
    do
    {
        pSymTreeNode = K2_GET_CONTAINER(K2DLX_SYMTREE_NODE, pTreeNode, TreeNode);
        DUMPF("       %08X %s\n", pSymTreeNode->TreeNode.mUserVal, pSymTreeNode->mpSymName);
        pTreeNode = K2TREE_NextNode(apAnchor, pTreeNode);
    } while (pTreeNode != NULL);
}

static
void
sDumpExports(DLX_EXPORTS_SECTION *apSec)
{
    UINT32 ix;

    DUMPF("      COUNT  %d\n", apSec->mCount);
    DUMPF("      CRCAPI %08X\n", apSec->mCRC32);
    for (ix = 0;ix < apSec->mCount;ix++)
        DUMPF("      %3d: %08X %s\n", ix, apSec->Export[ix].mAddr, ((char *)apSec) + apSec->Export[ix].mNameOffset);
}

static
void
sDumpOneDlx(DLX *apDlx)
{
    DLX *                   pImportFrom;
    DLX_INFO *              pInfo;
    Elf32_Ehdr *            pElf;
    Elf32_Shdr *            pSecHdr;
    UINT32                  ix;
    DLX_EXPORTS_SECTION *   pExp;
    DLX_IMPORT *            pImport;

    DUMPF("%08X %d \"%.*s\"\n", (UINT32)apDlx, apDlx->mIntNameLen, apDlx->mIntNameLen, apDlx->mpIntName);
    DUMPF("  p%08X n%08X\n", (UINT32)apDlx->ListLink.mpPrev, (UINT32)apDlx->ListLink.mpNext);
    DUMPF("  %s\n", apDlx->mFlags & K2DLXSUPP_FLAG_FULLY_LOADED ? "FULLY LOADED" : "IN LOAD");
    DUMPF("  %s\n", apDlx->mFlags & K2DLXSUPP_FLAG_PERMANENT ? "PERMANENT" : "TRANSIENT");
    DUMPF("  REFS        %08X\n", apDlx->mRefs);
    DUMPF("  ENTRY       0x%08X (%s CALLED)\n", (UINT32)apDlx->mEntrypoint, apDlx->mFlags & K2DLXSUPP_FLAG_ENTRY_CALLED ? "" : "NOT");
    DUMPF("  HOSTFILE    0x%08X\n", (UINT32)apDlx->mHostFile);
    DUMPF("  CURSECTOR   %d\n", apDlx->mCurSector);
    DUMPF("  SECTORCOUNT %d\n", apDlx->mSectorCount);
    DUMPF("  RELSECS     %d\n", apDlx->mRelocSectionCount);
    DUMPF("  HDRBYTES    %d\n", apDlx->mHdrBytes);
    DUMPF("  EXPCODELOAD 0x%08X\n", (UINT32)apDlx->mpExpCodeDataAddr);
    DUMPF("  EXPREADLOAD 0x%08X\n", (UINT32)apDlx->mpExpReadDataAddr);
    DUMPF("  EXPDATALOAD 0x%08X\n", (UINT32)apDlx->mpExpDataDataAddr);
    DUMPF("  SEGMENTS:\n");
    for (ix = 0; ix < DlxSeg_Count; ix++)
    {
        DUMPF("    %d: %08X --------<%s>\n",
            ix,
            apDlx->SegAlloc.Segment[ix].mLinkAddr,
            sgpSegName[ix]);
        if ((ix >= DlxSeg_Text) && (ix <= DlxSeg_Read))
        {
            if (apDlx->SymTree[ix - DlxSeg_Text].mNodeCount > 0)
                sDumpSymTree(&apDlx->SymTree[ix - DlxSeg_Text]);
        }
    }
    
    pInfo = apDlx->mpInfo;
    DUMPF("  INFO @ 0x%08X:\n", (UINT32)pInfo);
    DUMPF("    ELFCRC   0x%08X\n", pInfo->mElfCRC);
    DUMPF("    ID       "); sPrintGuid(&pInfo->ID); DUMPF("\n");
    DUMPF("    STACK    %08X\n", pInfo->mEntryStackReq);
    DUMPF("    SEGMENTS:\n");
    for (ix = 0; ix < DlxSeg_Count; ix++)
    {
        DUMPF("      %d: @ %08X for %08X <%s>\n",
            ix,
            pInfo->SegInfo[ix].mLinkAddr,
            pInfo->SegInfo[ix].mMemActualBytes,
            sgpSegName[ix]);
    }
    pExp = pInfo->mpExpCode;
    DUMPF("    EXP CODE 0x%08X\n", (UINT32)pExp);
    if (pExp != NULL)
        sDumpExports(pExp);
    pExp = pInfo->mpExpRead;
    DUMPF("    EXP READ 0x%08X\n", (UINT32)pExp);
    if (pExp != NULL)
        sDumpExports(pExp);
    pExp = pInfo->mpExpData;
    DUMPF("    EXP DATA 0x%08X\n", (UINT32)pExp);
    if (pExp != NULL)
        sDumpExports(pExp);

    DUMPF("    %d IMPORTS\n", pInfo->mImportCount);
    pImport = (DLX_IMPORT *)(((UINT8 *)pInfo) + (sizeof(DLX_INFO) - sizeof(UINT32) + apDlx->mIntNameFieldLen));
    for (ix = 0;ix < pInfo->mImportCount;ix++)
    {
        pImportFrom = (DLX *)pImport->mReserved;
        DUMPF("      %d: %08X %.*s\n", ix, (UINT32)pImportFrom, pImportFrom->mIntNameLen, pImportFrom->mpIntName);
        pImport = (DLX_IMPORT *)(((UINT8 *)pImport) + pImport->mSizeBytes);
    }

    pElf = apDlx->mpElf;

    pSecHdr = apDlx->mpSecHdr;

    DUMPF("  ELF @ %08X; SECHDRS 0x%08X\n", (UINT32)pElf, (UINT32)pSecHdr);
    DUMPF("    %s MODE\n", (pElf->e_flags & DLX_EF_KERNEL_ONLY) ? "KERNEL" : "ANY");
    DUMPF("    %d SECTIONS\n", pElf->e_shnum);
    for (ix = 1;ix < pElf->e_shnum;ix++)
    {
        DUMPF("      %2d: @%08X type %2d flags %08X size %08X\n",
            ix,
            pSecHdr[ix].sh_addr,
            pSecHdr[ix].sh_type,
            pSecHdr[ix].sh_flags,
            pSecHdr[ix].sh_size);
    }
}

static
void
sDumpDLX(void)
{
    K2LIST_LINK *pLink;

    DUMPF("DLX LIST:\n");
    DUMPF("--------------------\n");
    pLink = gpK2DLXSUPP_Vars->LoadedList.mpHead;
    while (pLink != NULL)
    {
        sDumpOneDlx(K2_GET_CONTAINER(DLX, pLink, ListLink));
        pLink = pLink->mpNext;
    }
    DUMPF("--------------------\n");
}
#endif

K2DLXSUPP_VARS * gpK2DLXSUPP_Vars = NULL;

int
iK2DLXSUPP_CompareUINT32(
    UINT32        aKey,
    K2TREE_NODE * apDlx
    )
{
    if (aKey < apDlx->mUserVal)
        return -1;
    if (aKey == apDlx->mUserVal)
        return 0;
    return 1;
}

K2STAT
K2DLXSUPP_Init(
    void *              apMemoryPage,
    K2DLXSUPP_HOST *    apSupp,
    BOOL                aKeepSym,
    BOOL                aReInit,
    K2DLXSUPP_PRELOAD * apPreload  // can only be non-NULL if aReInit is FALSE
    )
{
    K2LIST_LINK *   pListLink;
    DLX *           pDlx;
    UINT32          ixTree;

    if (apMemoryPage == NULL)
        return K2DLXSUPP_ERRORPOINT(K2STAT_ERROR_BAD_ARGUMENT);

    if (aReInit)
    {
        K2_ASSERT(NULL == apPreload);
    }

    gpK2DLXSUPP_Vars = (K2DLXSUPP_VARS *)apMemoryPage;
    if (!aReInit)
        K2MEM_Zero(apMemoryPage, K2_VA32_MEMPAGE_BYTES);

    if (apSupp != NULL)
    {
        K2_ASSERT(apSupp->mHostSizeBytes == sizeof(K2DLXSUPP_HOST));
        K2MEM_Copy(&gpK2DLXSUPP_Vars->Host, apSupp, sizeof(K2DLXSUPP_HOST));
    }
    else
    {
        K2MEM_Zero(&gpK2DLXSUPP_Vars->Host, sizeof(K2DLXSUPP_HOST));
        gpK2DLXSUPP_Vars->Host.mHostSizeBytes = sizeof(K2DLXSUPP_HOST);
    }

    gpK2DLXSUPP_Vars->mAcqDisabled = FALSE;

    gpK2DLXSUPP_Vars->mKeepSym = aKeepSym;

    if (!aReInit)
    {
        K2LIST_Init(&gpK2DLXSUPP_Vars->LoadedList);
        K2LIST_Init(&gpK2DLXSUPP_Vars->AcqList);
        if (NULL != apPreload)
        {
            iK2DLXSUPP_Preload(apPreload);
#if ENABLE_DUMP
            sDumpDLX();
#endif
        }
    }
    else
    {
        gpK2DLXSUPP_Vars->mHandedOff = FALSE;

        //
        // symbol trees in nodes need to be adjusted
        // to have thier comparison function addresses updated
        // to the current library's address
        //
        pListLink = gpK2DLXSUPP_Vars->LoadedList.mpHead;
        while (pListLink != NULL)
        {
            pDlx = K2_GET_CONTAINER(DLX, pListLink, ListLink);
            K2_ASSERT(pDlx->mLinkAddr == (UINT32)pDlx);
            for (ixTree = 0;ixTree < 3; ixTree++)
                pDlx->SymTree[ixTree].mfCompareKeyToNode = iK2DLXSUPP_CompareUINT32;
            pListLink = pListLink->mpNext;
        }

        //
        // must be done last
        //
        if (gpK2DLXSUPP_Vars->Host.AtReInit != NULL)
        {
            pListLink = gpK2DLXSUPP_Vars->LoadedList.mpHead;
            while (pListLink != NULL)
            {
                pDlx = K2_GET_CONTAINER(DLX, pListLink, ListLink);
                gpK2DLXSUPP_Vars->Host.AtReInit(pDlx, pDlx->mLinkAddr, &pDlx->mHostFile);
                pListLink = pListLink->mpNext;
            }
        }

#if ENABLE_DUMP
        sDumpDLX();
#endif
    }

    return K2STAT_OK;
}

