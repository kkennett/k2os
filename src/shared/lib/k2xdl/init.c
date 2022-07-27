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

#include "ixdl.h"

XDL_GLOBAL * gpXdlGlobal = NULL;

K2STAT 
K2XDL_Init(
    void *      apXdlControlPage, 
    K2XDL_HOST *apHost, 
    BOOL        aKeepSym, 
    BOOL        aReInit
)
{
    K2LIST_LINK *   pListLink;
    K2TREE_ANCHOR   treeAnchor;
    XDL *           pXdl;
    UINT_PTR        ixTree;

    if (apXdlControlPage == NULL)
        return K2STAT_ERROR_BAD_ARGUMENT;

    gpXdlGlobal = (XDL_GLOBAL *)apXdlControlPage;
    
    if (!aReInit)
        K2MEM_Zero(apXdlControlPage, K2_VA_MEMPAGE_BYTES);

    if (apHost != NULL)
    {
        K2MEM_Copy(&gpXdlGlobal->Host, apHost, sizeof(K2XDL_HOST));
    }
    else
    {
        K2MEM_Zero(&gpXdlGlobal->Host, sizeof(K2XDL_HOST));
    }

    gpXdlGlobal->mAcqDisabled = FALSE;

    gpXdlGlobal->mKeepSym = aKeepSym;

    if (!aReInit)
    {
        K2LIST_Init(&gpXdlGlobal->LoadedList);
        K2LIST_Init(&gpXdlGlobal->AcqList);
    }
    else
    {
        gpXdlGlobal->mHandedOff = FALSE;

        K2TREE_Init(&treeAnchor, NULL);

        //
        // symbol trees in nodes need to be adjusted
        // to have thier comparison function addresses updated
        // to the current library's address
        //
        pListLink = gpXdlGlobal->LoadedList.mpHead;
        while (pListLink != NULL)
        {
            pXdl = K2_GET_CONTAINER(XDL, pListLink, ListLink);
            K2_ASSERT(pXdl->mpLoadCtx->mModulePageLinkAddr == (UINT_PTR)pXdl);
            for (ixTree = 0; ixTree < XDLProgDataType_Count; ixTree++)
                pXdl->SymTree[ixTree].mfCompareKeyToNode = treeAnchor.mfCompareKeyToNode;
            pListLink = pListLink->mpNext;
        }

        //
        // must be done last
        //
        if (gpXdlGlobal->Host.AtReInit != NULL)
        {
            pListLink = gpXdlGlobal->LoadedList.mpHead;
            while (pListLink != NULL)
            {
                pXdl = K2_GET_CONTAINER(XDL, pListLink, ListLink);
                gpXdlGlobal->Host.AtReInit(pXdl, pXdl->mpLoadCtx->mModulePageLinkAddr, NULL);
                pListLink = pListLink->mpNext;
            }
        }
    }

    return K2STAT_OK;
}
