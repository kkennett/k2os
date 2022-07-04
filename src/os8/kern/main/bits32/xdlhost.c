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

#include "kern.h"
#include "../../shared/lib/k2xdl/xdl_struct.h"

#if 0

void
KernDlxSupp_TrackBuiltIn(
    K2OSKERN_DLX_TRACK *    apDlxTrack,
    DLX *                   apDlx,
    UINT32                  aModulePageLinkAddr,
    K2DLXSUPP_HOST_FILE *   apInOutHostFile
)
{
    K2_ASSERT(NULL == apDlxTrack->mpDlx);
    apDlxTrack->mpDlx = apDlx;

    *apInOutHostFile = (K2DLXSUPP_HOST_FILE)apDlxTrack;

    K2LIST_AddAtTail(&gData.Dlx.KernLoadedList, &apDlxTrack->KernLoadedListLink);
}

void 
KernDlxSupp_AtReInit(
    DLX *                   apDlx, 
    UINT32                  aModulePageLinkAddr, 
    K2DLXSUPP_HOST_FILE *   apInOutHostFile
)
{
    //
    // this happens at kernel dlx_entry.
    // this allows static kernel objects to be intiailized 
    // so they are valid prior to any use.
    //
    if (apDlx == gData.mpShared->LoadInfo.mpDlxCrt)
    {
        KernDlxSupp_TrackBuiltIn(&gData.Dlx.TrackCrt, apDlx, aModulePageLinkAddr, apInOutHostFile);
    }
    else if (apDlx == gData.Dlx.mpPlat)
    {
        KernDlxSupp_TrackBuiltIn(&gData.Dlx.TrackPlat, apDlx, aModulePageLinkAddr, apInOutHostFile);
    }
    else if (apDlx == gData.Dlx.mpKern)
    {
        KernDlxSupp_TrackBuiltIn(&gData.Dlx.TrackKern, apDlx, aModulePageLinkAddr, apInOutHostFile);
    }
    else
    {
        K2_ASSERT(0);
    }
}

UINT32 
KernXdl_FindClosestSymbol(
    K2OSKERN_OBJ_PROCESS *  apCurProc,
    UINT32                  aAddr,
    char *                  apRetSymName,
    UINT32                  aRetSymNameBufLen
)
{
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_DLX_SEGMENT *  pSeg;
    K2OSKERN_DLX_TRACK *    pDlxTrack;
    BOOL                    disp;
    K2LIST_LINK *           pDlxListLink;
    DLX *                   pDlx;
    UINT32                  segStart;

    if (aAddr >= K2OS_KVA_KERN_BASE)
    {
        disp = K2OSKERN_SeqLock(&gData.Dlx.KernLoadedListSeqLock);

        pTreeNode = K2TREE_FindOrAfter(&gData.Dlx.KernSegTree, aAddr);
        if (pTreeNode == NULL)
        {
            pTreeNode = K2TREE_LastNode(&gData.Dlx.KernSegTree);
            pTreeNode = K2TREE_PrevNode(&gData.Dlx.KernSegTree, pTreeNode);
            if (pTreeNode != NULL)
            {
                pSeg = K2_GET_CONTAINER(K2OSKERN_DLX_SEGMENT, pTreeNode, KernSegTreeNode);
                K2_ASSERT(pSeg->KernSegTreeNode.mUserVal < aAddr);
            }
        }
        else
        {
            if (pTreeNode->mUserVal != aAddr)
            {
                pTreeNode = K2TREE_PrevNode(&gData.Dlx.KernSegTree, pTreeNode);
                if (pTreeNode != NULL)
                {
                    pSeg = K2_GET_CONTAINER(K2OSKERN_DLX_SEGMENT, pTreeNode, KernSegTreeNode);
                    K2_ASSERT(pSeg->KernSegTreeNode.mUserVal < aAddr);
                }
                else
                {
                    pSeg = NULL;
                }
            }
            else
            {
                pSeg = K2_GET_CONTAINER(K2OSKERN_DLX_SEGMENT, pTreeNode, KernSegTreeNode);
            }
        }

        if (pSeg != NULL)
        {
            pDlxTrack = K2_GET_CONTAINER(K2OSKERN_DLX_TRACK, pSeg, Seg[pSeg->mIndex]);
            DLX_AddrToName(pDlxTrack->mpDlx, aAddr, pSeg->mIndex, apRetSymName, aRetSymNameBufLen);
        }
        else
        {
            K2ASC_PrintfLen(apRetSymName, aRetSymNameBufLen, "(kernel?)|%08X", aAddr);
        }

        K2OSKERN_SeqUnlock(&gData.Dlx.KernLoadedListSeqLock, disp);

        return aAddr;
    }

    if (aAddr == 0x7FFFF004)
    {
        K2ASC_CopyLen(apRetSymName, "SYSCALL_RETURN", aRetSymNameBufLen);
        return aAddr;
    }

    pDlxListLink = (NULL == apCurProc->mpUserDlxList) ? NULL : apCurProc->mpUserDlxList->mpHead;

    if (NULL != pDlxListLink)
    {
        do
        {
            //
            // look in code symbols for this dlx
            //
            pDlx = K2_GET_CONTAINER(DLX, pDlxListLink, ListLink);
            pDlxListLink = pDlxListLink->mpNext;

            segStart = pDlx->SegAlloc.Segment[DlxSeg_Text].mLinkAddr;
            if ((aAddr >= segStart) &&
                (aAddr < segStart + pDlx->mpInfo->SegInfo[DlxSeg_Text].mMemActualBytes))
            {
                DLX_AddrToName(pDlx, aAddr, DlxSeg_Text, apRetSymName, aRetSymNameBufLen);
                return aAddr;
            }

        } while (NULL != pDlxListLink);
    }

    K2ASC_CopyLen(apRetSymName, "User-Unknown", aRetSymNameBufLen);

    return aAddr;
}

K2STAT 
KernDlxSupp_CritSec(
    BOOL aEnter
)
{
    if (!gData.mThreaded)
        return K2STAT_OK;

    if (aEnter)
        KernCritSec_Enter(&gData.Dlx.Sec);
    else
        KernCritSec_Leave(&gData.Dlx.Sec);

    return K2STAT_OK;
}

K2STAT 
KernDlxSupp_AcqAlreadyLoaded(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
KernDlxSupp_Open(
    void *                  apAcqContext, 
    char const *            apFileSpec, 
    char const *            apNamePart, 
    UINT32                  aNamePartLen, 
    K2DLXSUPP_OPENRESULT *  apRetResult
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
KernDlxSupp_ReadSectors(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile, 
    void *              apBuffer, 
    UINT32              aSectorCount
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
KernDlxSupp_Prepare(
    void *                  apAcqContext,
    K2DLXSUPP_HOST_FILE     aHostFile,
    DLX_INFO *              apInfo,
    UINT32                  aInfoSize,
    BOOL                    aKeepSymbols,
    K2DLXSUPP_SEGALLOC *    apRetAlloc
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

BOOL 
KernDlxSupp_PreCallback(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile, 
    BOOL                aIsLoad, 
    DLX *               apDlx
)
{
    K2_ASSERT(0);
    return FALSE;
}

K2STAT 
KernDlxSupp_PostCallback(
    void *              apAcqContext, 
    K2DLXSUPP_HOST_FILE aHostFile, 
    K2STAT              aUserStatus, 
    DLX *               apDlx
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
KernDlxSupp_Finalize(
    void *                  apAcqContext, 
    K2DLXSUPP_HOST_FILE     aHostFile, 
    K2DLXSUPP_SEGALLOC *    apUpdateAlloc
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
KernDlxSupp_RefChange(
    K2DLXSUPP_HOST_FILE aHostFile, 
    DLX *               apDlx, 
    INT32               aRefChange
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
KernDlxSupp_Purge(
    K2DLXSUPP_HOST_FILE aHostFile
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
KernDlxSupp_ErrorPoint(
    char const *apFile, 
    int         aLine, 
    K2STAT      aStatus
)
{
    K2OSKERN_Debug("DLX_ERRORPOINT(%s %d - %08X\n", apFile, aLine, aStatus);
    K2_ASSERT(0 == aStatus);
    return aStatus;
}

void
KernDlx_AtXdlEntry(
    void
)
{
    K2STAT  stat;

    K2OSKERN_SeqInit(&gData.Dlx.KernLoadedListSeqLock);
    K2LIST_Init(&gData.Dlx.KernLoadedList);

    stat = K2DLXSUPP_Init((void *)K2OS_KVA_LOADERPAGE_BASE, NULL, TRUE, TRUE, NULL);
    while (K2STAT_IS_ERROR(stat));

    //
    // reinit below will set up the stock objects
    //
    stat = DLX_Acquire("k2osplat.dlx", NULL, &gData.Dlx.mpPlat, NULL, NULL);
    while (K2STAT_IS_ERROR(stat));

//    stat = DLX_Acquire("k2osacpi.dlx", NULL, &gData.Dlx.mpAcpi, NULL, NULL);
//    while (K2STAT_IS_ERROR(stat));

    stat = DLX_Acquire("k2oskern.dlx", NULL, &gData.Dlx.mpKern, NULL, NULL);
    while (K2STAT_IS_ERROR(stat));

    //
    // second init is with support functions. reinit will get called, and all dlx
    // have been acquired
    //
    gData.Dlx.Host.mHostSizeBytes = sizeof(gData.Dlx.Host);
    gData.Dlx.Host.AtReInit = KernDlxSupp_AtReInit;
    stat = K2DLXSUPP_Init((void *)K2OS_KVA_LOADERPAGE_BASE, &gData.Dlx.Host, TRUE, TRUE, NULL);
    while (K2STAT_IS_ERROR(stat));
}

void
KernDlx_AddOneBuiltinDlx(
    K2OSKERN_DLX_TRACK *apDlxTrack
)
{
    K2STAT              stat;
    DLX_SEGMENT_INFO    segInfo[DlxSeg_Count];
    UINT32              ix;

    stat = K2DLXSUPP_GetInfo(apDlxTrack->mpDlx, NULL, NULL, segInfo, NULL, &apDlxTrack->mpFileName);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

//    K2OSKERN_Debug("File[%s]\n", apDlxTrack->mpFileName);
    for (ix = 0; ix < DlxSeg_Reloc; ix++)
    {
        apDlxTrack->Seg[ix].mIndex = ix;
//        K2OSKERN_Debug("  %d: %08X %08X\n", ix, segInfo[ix].mLinkAddr, segInfo[ix].mMemActualBytes);
        if ((segInfo[ix].mMemActualBytes > 0) &&
            (segInfo[ix].mLinkAddr != 0))
        {
            apDlxTrack->Seg[ix].mSizeBytes = segInfo[ix].mMemActualBytes;
            apDlxTrack->Seg[ix].KernSegTreeNode.mUserVal = segInfo[ix].mLinkAddr;
            K2TREE_Insert(&gData.Dlx.KernSegTree, apDlxTrack->Seg[ix].KernSegTreeNode.mUserVal, &apDlxTrack->Seg[ix].KernSegTreeNode);
        }
    }
}

void
KernDlx_Init(
    void
)
{
    BOOL                    ok;
    K2STAT                  stat;
    K2LIST_LINK *           pListLink;
    K2OSKERN_DLX_TRACK *    pDlxTrack;

    K2TREE_Init(&gData.Dlx.KernSegTree, NULL);

    ok = KernCritSec_Init(&gData.Dlx.Sec);
    K2_ASSERT(ok);

    gData.Dlx.Host.AtReInit = NULL;
    gData.Dlx.Host.CritSec = KernDlxSupp_CritSec;
    gData.Dlx.Host.AcqAlreadyLoaded = KernDlxSupp_AcqAlreadyLoaded;
    gData.Dlx.Host.Open = KernDlxSupp_Open;
    gData.Dlx.Host.ReadSectors = KernDlxSupp_ReadSectors;
    gData.Dlx.Host.Prepare = KernDlxSupp_Prepare;
    gData.Dlx.Host.PreCallback = KernDlxSupp_PreCallback;
    gData.Dlx.Host.PostCallback = KernDlxSupp_PostCallback;
    gData.Dlx.Host.Finalize = KernDlxSupp_Finalize;
    gData.Dlx.Host.RefChange = KernDlxSupp_RefChange;
    gData.Dlx.Host.Purge = KernDlxSupp_Purge;
    gData.Dlx.Host.ErrorPoint = KernDlxSupp_ErrorPoint;

    stat = K2DLXSUPP_Init((void *)K2OS_KVA_LOADERPAGE_BASE, &gData.Dlx.Host, TRUE, TRUE, NULL);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    pListLink = gData.Dlx.KernLoadedList.mpHead;
    do {
        pDlxTrack = K2_GET_CONTAINER(K2OSKERN_DLX_TRACK, pListLink, KernLoadedListLink);
        pListLink = pListLink->mpNext;

        KernDlx_AddOneBuiltinDlx(pDlxTrack);

    } while (pListLink != NULL);
}

#endif

UINT32
KernXdl_FindClosestSymbol(
    K2OSKERN_OBJ_PROCESS *  apCurProc,
    UINT32                  aAddr,
    char *                  apRetSymName,
    UINT32                  aRetSymNameBufLen
)
{
    if (0 != aRetSymNameBufLen)
        *apRetSymName = 0;
    return 0;
}

void
KernXdl_Init(
    void
)
{

}

void
KernXdl_AtXdlEntry(
    void
)
{

}