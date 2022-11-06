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
#include "../../../../shared/lib/k2xdl/xdl_struct.h"

void
KernXdlHost_TrackBuiltIn(
    K2OSKERN_XDL_TRACK *apXdlTrack,
    XDL *               apXdl,
    UINT32              aModulePageLinkAddr,
    K2XDL_HOST_FILE *   apInOutHostFile
)
{
    K2_ASSERT(NULL == apXdlTrack->mpXdl);
    apXdlTrack->mpXdl = apXdl;

    *apInOutHostFile = (K2XDL_HOST_FILE)apXdlTrack;

    K2LIST_AddAtTail(&gData.Xdl.KernLoadedList, &apXdlTrack->KernLoadedListLink);
}

void 
KernXdlHost_AtReInit(
    XDL *               apXdl, 
    UINT32              aModulePageLinkAddr, 
    K2XDL_HOST_FILE *   apInOutHostFile
)
{
    //
    // this happens at kernel xdl_entry.
    // this allows static kernel objects to be intiailized 
    // so they are valid prior to any use.
    //
    if (apXdl == gData.mpShared->LoadInfo.mpXdlCrt)
    {
        KernXdlHost_TrackBuiltIn(&gData.Xdl.TrackCrt, apXdl, aModulePageLinkAddr, apInOutHostFile);
    }
    else if (apXdl == gData.Xdl.mpPlat)
    {
        KernXdlHost_TrackBuiltIn(&gData.Xdl.TrackPlat, apXdl, aModulePageLinkAddr, apInOutHostFile);
    }
    else if (apXdl == gData.Xdl.mpKern)
    {
        KernXdlHost_TrackBuiltIn(&gData.Xdl.TrackKern, apXdl, aModulePageLinkAddr, apInOutHostFile);
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
    K2OSKERN_XDL_SEGMENT *  pSeg;
    K2OSKERN_XDL_TRACK *    pXdlTrack;
    BOOL                    disp;
    K2LIST_LINK *           pXdlListLink;
    XDL *                   pXdl;
    UINT32                  segStart;

    if (aAddr >= K2OS_KVA_KERN_BASE)
    {
        disp = K2OSKERN_SeqLock(&gData.Xdl.KernLoadedListSeqLock);

        pTreeNode = K2TREE_FindOrAfter(&gData.Xdl.KernSegTree, aAddr);
        if (pTreeNode == NULL)
        {
            pTreeNode = K2TREE_LastNode(&gData.Xdl.KernSegTree);
            pTreeNode = K2TREE_PrevNode(&gData.Xdl.KernSegTree, pTreeNode);
            if (pTreeNode != NULL)
            {
                pSeg = K2_GET_CONTAINER(K2OSKERN_XDL_SEGMENT, pTreeNode, KernSegTreeNode);
                K2_ASSERT(pSeg->KernSegTreeNode.mUserVal < aAddr);
            }
        }
        else
        {
            if (pTreeNode->mUserVal != aAddr)
            {
                pTreeNode = K2TREE_PrevNode(&gData.Xdl.KernSegTree, pTreeNode);
                if (pTreeNode != NULL)
                {
                    pSeg = K2_GET_CONTAINER(K2OSKERN_XDL_SEGMENT, pTreeNode, KernSegTreeNode);
                    K2_ASSERT(pSeg->KernSegTreeNode.mUserVal < aAddr);
                }
                else
                {
                    pSeg = NULL;
                }
            }
            else
            {
                pSeg = K2_GET_CONTAINER(K2OSKERN_XDL_SEGMENT, pTreeNode, KernSegTreeNode);
            }
        }

        if (pSeg != NULL)
        {
            pXdlTrack = K2_GET_CONTAINER(K2OSKERN_XDL_TRACK, pSeg, Seg[pSeg->mSegmentIx]);
            XDL_FindAddrName(pXdlTrack->mpXdl, aAddr, apRetSymName, aRetSymNameBufLen);
        }
        else
        {
            K2ASC_PrintfLen(apRetSymName, aRetSymNameBufLen, "(kernel?)|%08X", aAddr);
        }

        K2OSKERN_SeqUnlock(&gData.Xdl.KernLoadedListSeqLock, disp);

        return aAddr;
    }

    if (aAddr == 0x7FFFF004)
    {
        K2ASC_CopyLen(apRetSymName, "SYSCALL_RETURN", aRetSymNameBufLen);
        return aAddr;
    }

    pXdlListLink = (NULL == apCurProc->mpUserXdlList) ? NULL : apCurProc->mpUserXdlList->mpHead;

    if (NULL != pXdlListLink)
    {
        do
        {
            //
            // look in code symbols for this xdl
            //
            pXdl = K2_GET_CONTAINER(XDL, pXdlListLink, ListLink);
            pXdlListLink = pXdlListLink->mpNext;
            segStart = pXdl->SegAddrs.mLink[XDLSegmentIx_Text];
            if ((aAddr >= segStart) &&
                (aAddr < segStart + pXdl->mpHeader->Segment[XDLSegmentIx_Text].mMemActualBytes))
            {
                XDL_FindAddrName(pXdl, aAddr, apRetSymName, aRetSymNameBufLen);
                return aAddr;
            }

        } while (NULL != pXdlListLink);
    }

    K2ASC_CopyLen(apRetSymName, "User-Unknown", aRetSymNameBufLen);

    return aAddr;
}

K2STAT 
KernXdlHost_CritSec(
    BOOL aEnter
)
{
    return K2STAT_OK;
}

K2STAT 
KernXdlHost_Open(
    K2XDL_OPENARGS const *  apArgs, 
    K2XDL_HOST_FILE *       apRetHostFile, 
    UINT_PTR *              apRetModuleDataAddr, 
    UINT_PTR *              apRetModuleLinkAddr
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
KernXdlHost_ReadSectors(
    K2XDL_LOADCTX const *   apLoadCtx, 
    void *                  apBuffer, 
    UINT64 const *          apSectorCount
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT
KernXdlHost_Prepare(
    K2XDL_LOADCTX const *   apLoadCtx, 
    BOOL                    aKeepSymbols, 
    XDL_FILE_HEADER const * apFileHdr, 
    K2XDL_SEGMENT_ADDRS *   apRetSegAddrs
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

BOOL 
KernXdlHost_PreCallback(
    K2XDL_LOADCTX const *   apLoadCtx, 
    BOOL                    aIsLoad, 
    XDL *                   apXdl
)
{
    K2_ASSERT(0);
    return FALSE;
}

void
KernXdlHost_PostCallback(
    K2XDL_LOADCTX const *   apLoadCtx, 
    K2STAT                  aUserStatus,
    XDL *                   apXdl
)
{
    K2_ASSERT(0);
}

K2STAT
KernXdlHost_Finalize(
    K2XDL_LOADCTX const *   apLoadCtx,
    BOOL                    aKeepSymbols,
    XDL_FILE_HEADER const * apFileHdr,
    K2XDL_SEGMENT_ADDRS *   apUpdateSegAddrs
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT 
KernXdlHost_Purge(
    K2XDL_LOADCTX const *       apLoadCtx, 
    K2XDL_SEGMENT_ADDRS const * apSegAddrs
)
{
    K2_ASSERT(0);
    return K2STAT_ERROR_NOT_IMPL;
}

void
KernXdl_AtXdlEntry(
    void
)
{
    K2STAT  stat;

    K2OSKERN_SeqInit(&gData.Xdl.KernLoadedListSeqLock);
    K2LIST_Init(&gData.Xdl.KernLoadedList);

    stat = K2XDL_Init((void *)K2OS_KVA_LOADERPAGE_BASE, NULL, TRUE, TRUE);
    while (K2STAT_IS_ERROR(stat));

    //
    // reinit below will set up the stock objects
    //
    stat = XDL_Acquire("k2osplat.xdl", 0, &gData.Xdl.mpPlat);
    while (K2STAT_IS_ERROR(stat));

    stat = XDL_Acquire("k2oskern.xdl", 0, &gData.Xdl.mpKern);
    while (K2STAT_IS_ERROR(stat));

    //
    // second init is with support functions. reinit will get called, and all xdl
    // have been acquired
    //
    gData.Xdl.Host.AtReInit = KernXdlHost_AtReInit;
    stat = K2XDL_Init((void *)K2OS_KVA_LOADERPAGE_BASE, &gData.Xdl.Host, TRUE, TRUE);
    while (K2STAT_IS_ERROR(stat));
}

void
KernXdl_AddOneBuiltinXdl(
    K2OSKERN_XDL_TRACK *apXdlTrack
)
{
    UINT_PTR                ix;
    XDL_FILE_HEADER const * pHeader;

    pHeader = apXdlTrack->mpXdl->mpHeader;

    for (ix = 0; ix < XDLSegmentIx_Relocs; ix++)
    {
        apXdlTrack->Seg[ix].mSegmentIx = ix;

        if ((pHeader->Segment[ix].mMemActualBytes > 0) &&
            (pHeader->Segment[ix].mLinkAddr != 0))
        {
            apXdlTrack->Seg[ix].mSizeBytes = (UINT_PTR)pHeader->Segment[ix].mMemActualBytes;
            apXdlTrack->Seg[ix].KernSegTreeNode.mUserVal = (UINT_PTR)pHeader->Segment[ix].mLinkAddr;
            K2TREE_Insert(&gData.Xdl.KernSegTree, apXdlTrack->Seg[ix].KernSegTreeNode.mUserVal, &apXdlTrack->Seg[ix].KernSegTreeNode);
        }
    }
}

void
KernXdl_Init(
    void
)
{
    BOOL                    ok;
    K2STAT                  stat;
    K2LIST_LINK *           pListLink;
    K2OSKERN_XDL_TRACK *    pXdlTrack;

    K2TREE_Init(&gData.Xdl.KernSegTree, NULL);

    ok = KernCritSec_Init(&gData.Xdl.Sec);
    K2_ASSERT(ok);

    gData.Xdl.Host.CritSec = KernXdlHost_CritSec;
    gData.Xdl.Host.Open = KernXdlHost_Open;
    gData.Xdl.Host.ReadSectors = KernXdlHost_ReadSectors;
    gData.Xdl.Host.Prepare = KernXdlHost_Prepare;
    gData.Xdl.Host.PreCallback = KernXdlHost_PreCallback;
    gData.Xdl.Host.PostCallback = KernXdlHost_PostCallback;
    gData.Xdl.Host.Finalize = KernXdlHost_Finalize;
    gData.Xdl.Host.Purge = KernXdlHost_Purge;
    gData.Xdl.Host.AtReInit = NULL;

    stat = K2XDL_Init((void *)K2OS_KVA_LOADERPAGE_BASE, &gData.Xdl.Host, TRUE, TRUE);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    pListLink = gData.Xdl.KernLoadedList.mpHead;
    do {
        pXdlTrack = K2_GET_CONTAINER(K2OSKERN_XDL_TRACK, pListLink, KernLoadedListLink);
        pListLink = pListLink->mpNext;
        KernXdl_AddOneBuiltinXdl(pXdlTrack);
    } while (pListLink != NULL);
}

