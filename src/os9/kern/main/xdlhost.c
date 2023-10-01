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
#include "../../../shared/lib/k2xdl/xdl_struct.h"

void
KernXdlHost_TrackBuiltIn(
    K2OSKERN_XDL_TRACK *apXdlTrack,
    XDL *               apXdl,
    UINT32              aModulePageLinkAddr,
    K2XDL_HOST_FILE *   apInOutHostFile
)
{
    K2_ASSERT(NULL == apXdlTrack->mpXdl);
    K2_ASSERT(NULL != apXdl);

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
        gData.Xdl.TrackCrt.mpName = "k2oscrt";
    }
    else if (apXdl == gData.Xdl.mpPlat)
    {
        KernXdlHost_TrackBuiltIn(&gData.Xdl.TrackPlat, apXdl, aModulePageLinkAddr, apInOutHostFile);
        gData.Xdl.TrackPlat.mpName = "k2osplat";
    }
    else if (apXdl == gData.Xdl.mpKern)
    {
        KernXdlHost_TrackBuiltIn(&gData.Xdl.TrackKern, apXdl, aModulePageLinkAddr, apInOutHostFile);
        gData.Xdl.TrackKern.mpName = "k2oskern";
    }
    else if (apXdl == gData.Xdl.mpAcpi)
    {
        KernXdlHost_TrackBuiltIn(&gData.Xdl.TrackAcpi, apXdl, aModulePageLinkAddr, apInOutHostFile);
        gData.Xdl.TrackAcpi.mpName = "k2osacpi";
    }
    else if (apXdl == gData.Xdl.mpExec)
    {
        KernXdlHost_TrackBuiltIn(&gData.Xdl.TrackExec, apXdl, aModulePageLinkAddr, apInOutHostFile);
        gData.Xdl.TrackExec.mpName = "k2osexec";
    }
    else
    {
        K2OSKERN_Panic("*** Unknown built in module\n");
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
    K2OSKERN_OBJ_VIRTMAP *  pVirtMap;
    K2OSKERN_XDL_TRACK *    pXdlTrack;
    BOOL                    disp;
    K2LIST_LINK *           pXdlListLink;
    XDL *                   pXdl;
    UINT32                  segStart;
    K2OSKERN_HOST_FILE *    pHostFile;

    if ((aAddr >= K2OS_KVA_KERN_BASE) || (NULL == apCurProc))
    {
        pVirtMap = NULL;

        disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);

        pTreeNode = K2TREE_FindOrAfter(&gData.VirtMap.Tree, aAddr);
        if (pTreeNode == NULL)
        {
            pTreeNode = K2TREE_LastNode(&gData.VirtMap.Tree);
            if (NULL != pTreeNode)
            {
                pTreeNode = K2TREE_PrevNode(&gData.VirtMap.Tree, pTreeNode);
                if (pTreeNode != NULL)
                {
                    pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
                    K2_ASSERT(pVirtMap->OwnerMapTreeNode.mUserVal < aAddr);
                }
            }
        }
        else
        {
            if (pTreeNode->mUserVal != aAddr)
            {
                pTreeNode = K2TREE_PrevNode(&gData.VirtMap.Tree, pTreeNode);
                if (pTreeNode != NULL)
                {
                    pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
                    K2_ASSERT(pVirtMap->OwnerMapTreeNode.mUserVal < aAddr);
                }
            }
            else
            {
                pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
            }
        }

        if (pVirtMap != NULL)
        {
            if (pVirtMap->Kern.mType == KernMap_Xdl_Part)
            {
                pXdlTrack = pVirtMap->Kern.XdlPart.mpOwnerTrack;
                K2_ASSERT(NULL != pXdlTrack->mpXdl);
                XDL_FindAddrName(pXdlTrack->mpXdl, aAddr, apRetSymName, aRetSymNameBufLen);
            }
            else if (pVirtMap->Kern.mType == KernMap_Xdl_Page)
            {
                pHostFile = (K2OSKERN_HOST_FILE *)pVirtMap->Kern.XdlPage.mOwnerHostFile;
                K2_ASSERT(NULL != pHostFile->Track.mpXdl);
                K2ASC_PrintfLen(apRetSymName, aRetSymNameBufLen, "%.*s|Page+%08X\n", pHostFile->Track.mpName);
            }
            else
            {
                pVirtMap = NULL;
            }
        }

        if (pVirtMap == NULL)
        {
            K2ASC_PrintfLen(apRetSymName, aRetSymNameBufLen, "(kernel?)|%08X", aAddr);
        }

        K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);

        return aAddr;
    }

    if (aAddr == 0x7FFFF004)
    {
        K2ASC_CopyLen(apRetSymName, "SYSCALL_RETURN", aRetSymNameBufLen);
        return aAddr;
    }

    K2_ASSERT(NULL != apCurProc);

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

    stat = XDL_Acquire("k2osacpi.xdl", 0, &gData.Xdl.mpAcpi);
    while (K2STAT_IS_ERROR(stat));

    stat = XDL_Acquire("k2oskern.xdl", 0, &gData.Xdl.mpKern);
    while (K2STAT_IS_ERROR(stat));

    stat = XDL_Acquire("k2osexec.xdl", 0, &gData.Xdl.mpExec);
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
    UINT32                  ix;
    XDL_FILE_HEADER const * pHeader;
    UINT32                  pageCount;
    K2OS_VirtToPhys_MapType mapType;

    pHeader = apXdlTrack->mpXdl->mpHeader;

    for (ix = XDLSegmentIx_Text; ix < XDLSegmentIx_Relocs; ix++)
    {
        if ((pHeader->Segment[ix].mMemActualBytes > 0) &&
            (pHeader->Segment[ix].mLinkAddr != 0))
        {
            pageCount = (((UINT32)pHeader->Segment[ix].mMemActualBytes) + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;

            K2_ASSERT(0 == (pHeader->Segment[ix].mLinkAddr & K2_VA_MEMPAGE_OFFSET_MASK));

            if (ix == XDLSegmentIx_Text)
            {
                mapType = K2OS_MapType_Text;
            }
            else if (ix == XDLSegmentIx_Data)
            {
                mapType = K2OS_MapType_Data_ReadWrite;
            }
            else
            {
                mapType = K2OS_MapType_Data_ReadOnly;
            }

            KernVirtMap_CreatePreMap(pHeader->Segment[ix].mLinkAddr, pageCount, mapType, &apXdlTrack->SegVirtMapRef[ix]);

            apXdlTrack->SegVirtMapRef[ix].AsVirtMap->Kern.mType = KernMap_Xdl_Part;
            apXdlTrack->SegVirtMapRef[ix].AsVirtMap->Kern.XdlPart.mXdlSegmentIx = ix;
            apXdlTrack->SegVirtMapRef[ix].AsVirtMap->Kern.XdlPart.mpOwnerTrack = apXdlTrack;

            apXdlTrack->SegVirtMapRef[ix].AsVirtMap->Kern.mSizeBytes = (UINT32)pHeader->Segment[ix].mMemActualBytes;
        }
    }
}

void
KernXdl_Init(
    void
)
{
    K2STAT                  stat;
    K2LIST_LINK *           pListLink;
    K2OSKERN_XDL_TRACK *    pXdlTrack;

    // segtree init in atxdlentry

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

