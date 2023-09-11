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
#include "k2vfs.h"

VfsFolder * VfsFolder::sgpRoot = NULL;
UINT64      VfsFolder::sgCurIter = 1;
bool        VfsFolder::sgAfterFirstScan = false;

VfsFolder::VfsFolder(
    VfsFolder * apParentFolder, 
    char *      apNameZ,
    UINT_PTR    aNameLen, 
    bool        aExistsInFs
) :
    VfsNode(apParentFolder, apNameZ, aNameLen, aExistsInFs, true)
{
    K2TREE_Init(&FolderChildTree, sNameCompare);
    K2TREE_Init(&FileChildTree, sNameCompare);
}

VfsFolder::~VfsFolder(void)
{
    K2_ASSERT(0 == FolderChildTree.mNodeCount);
    K2_ASSERT(0 == FileChildTree.mNodeCount);
}

VfsNode *
VfsFolder::AcquireOrCreateSub(
    char const *apNameZ,
    UINT_PTR    aNameLen,
    bool        aIsFolder
)
{
    char const *    pChk;
    char            ch;
    UINT_PTR        compLen;
    VfsNode *       pSub;
    K2TREE_NODE *   pTreeNode;

    K2_ASSERT(NULL != apNameZ);
    K2_ASSERT(0 != aNameLen);

    pChk = apNameZ;
    do
    {
        ch = *pChk;
        if ((ch == 0) ||
            (ch == '\\') ||
            (ch == '/'))
            break;
        pChk++;
    } while (0 != --aNameLen);

    compLen = (UINT_PTR)(pChk - apNameZ);
    if (0 == compLen)
    {
        return NULL;
    }

    char *pNameBuf = new char[(compLen + 4) & ~3];
    K2_ASSERT(NULL != pNameBuf);
    K2ASC_CopyLen(pNameBuf, apNameZ, compLen);
    pNameBuf[compLen] = 0;

    if ((0 == ch) || (0 == aNameLen))
    {
        //
        // we are at the end of the path with this last component in pNameBuf,compLen
        //
        if (aIsFolder)
        {
            pTreeNode = K2TREE_Find(&FolderChildTree, (UINT_PTR)pNameBuf);
        }
        else
        {
            pTreeNode = K2TREE_Find(&FileChildTree, (UINT_PTR)pNameBuf);
        }

        if (NULL != pTreeNode)
        {
            //
            // already exists
            //
            delete[] pNameBuf;
            pSub = K2_GET_CONTAINER(VfsNode, pTreeNode, ParentsChildTreeNode);
            pSub->AddRef();
        }
        else
        {
            if (aIsFolder)
            {
                pSub = new VfsFolder(this, pNameBuf, compLen, false);
            }
            else
            {
                pSub = new VfsFile(this, pNameBuf, compLen, false);
            }
            K2_ASSERT(NULL != pSub);
        }

        return pSub;
    }

    //
    // not at the end of the path
    //
    pTreeNode = K2TREE_Find(&FolderChildTree, (UINT_PTR)pNameBuf);
    if (NULL == pTreeNode)
    {
        pSub = new VfsFolder(this, pNameBuf, compLen, false);
        K2_ASSERT(NULL != pSub);
    }
    else
    {
        //
        // next segment already exists
        //
        delete[] pNameBuf;
        pSub = K2_GET_CONTAINER(VfsNode, pTreeNode, ParentsChildTreeNode);
    }

    //
    // new subnode found or created, and we were not at the end of the path. so recurse
    //
    K2_ASSERT(NULL != pSub);
    return ((VfsFolder *)pSub)->AcquireOrCreateSub(apNameZ + compLen + 1, aNameLen, aIsFolder);
}

void
VfsFolder::SubScan(
    char *      apBuf,
    char *      apEnd,
    FILETIME *  apDeleteFileTime
)
{
    HANDLE          hFind;
    UINT_PTR        len;
    WIN32_FIND_DATA findData;
    K2TREE_NODE *   pTreeNode;
    VfsFolder *     pSubFolder;
    char *          pSubName;
    VfsFile *   pSubFile;
    UINT_PTR        pathLen;
    bool            vfsChanged;

    K2ASC_Copy(apEnd, "\\*");
    apEnd++;
    pathLen = (UINT_PTR)(apEnd - apBuf);

    vfsChanged = false;

    hFind = FindFirstFile(apBuf, &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            if (0 == (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            {
                len = K2ASC_Copy(apEnd, findData.cFileName);

                if (0 != (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    if ((0 != K2ASC_Comp(findData.cFileName, ".")) &&
                        (0 != K2ASC_Comp(findData.cFileName, "..")))
                    {
                        pTreeNode = K2TREE_Find(&FolderChildTree, (UINT_PTR)findData.cFileName);
                        if (NULL != pTreeNode)
                        {
                            pSubFolder = K2_GET_CONTAINER(VfsFolder, pTreeNode, ParentsChildTreeNode);
                            if (!pSubFolder->mExistsInFs)
                            {
                                pSubFolder->AddRef();
                                ++pSubFolder->mVersion;
                                pSubFolder->mExistsInFs = true;
                            }
                        }
                        else
                        {
                            pSubName = new char[(len + 4) & ~3];
                            K2_ASSERT(NULL != pSubName);
                            K2ASC_Copy(pSubName, findData.cFileName);
                            pSubFolder = new VfsFolder(this, pSubName, len, true);
                            K2_ASSERT(NULL != pSubFolder);
                        }

                        pSubFolder->mLastIter = sgCurIter;
                        pSubFolder->SubScan(apBuf, apEnd + len, apDeleteFileTime);
                    }
                }
                else
                {
                    pTreeNode = K2TREE_Find(&FileChildTree, (UINT_PTR)findData.cFileName);
                    if (NULL != pTreeNode)
                    {
                        pSubFile = K2_GET_CONTAINER(VfsFile, pTreeNode, ParentsChildTreeNode);
                        if (!pSubFile->mExistsInFs)
                        {
                            pSubFile->AddRef();
                            pSubFile->mExistsInFs = true;
                            K2MEM_Copy(&pSubFile->mTimeStamp, &findData.ftLastWriteTime, sizeof(FILETIME));
                            ++pSubFile->mVersion;
                            pSubFile->OnFsEvent(VfsFileEvent_Created);
                        }
                        else
                        {
                            if (0 < (CompareFileTime(&findData.ftLastWriteTime, &pSubFile->mTimeStamp)))
                            {
                                K2_ASSERT(sgAfterFirstScan);
                                ++pSubFile->mVersion;
                                K2MEM_Copy(&pSubFile->mTimeStamp, &findData.ftLastWriteTime, sizeof(FILETIME));
                                pSubFile->OnFsEvent(VfsFileEvent_Updated);
                            }
                        }
                    }
                    else
                    {
                        pSubName = new char[(len + 4) & ~3];
                        K2_ASSERT(NULL != pSubName);
                        K2ASC_Copy(pSubName, findData.cFileName);
                        pSubFile = new VfsFile(this, pSubName, len, true);
                        K2_ASSERT(NULL != pSubFile);
                        K2MEM_Copy(&pSubFile->mTimeStamp, &findData.ftLastWriteTime, sizeof(FILETIME));
                        pSubFile->OnFsEvent(VfsFileEvent_Discovered);
                    }

                    pSubFile->mLastIter = sgCurIter;
                }
            }
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }

    if (!sgAfterFirstScan)
        return;

    //
    // now search for and process deletions
    //

    pTreeNode = K2TREE_FirstNode(&FolderChildTree);
    if (NULL != pTreeNode)
    {
        do {
            pSubFolder = K2_GET_CONTAINER(VfsFolder, pTreeNode, ParentsChildTreeNode);
            pTreeNode = K2TREE_NextNode(&FolderChildTree, pTreeNode);
            if ((pSubFolder->mExistsInFs) && (pSubFolder->mLastIter != sgCurIter))
            {
                K2ASC_Copy(apEnd, pSubFolder->mpNameZ);
                pSubFolder->FsFolderTreeDeleted(apBuf, apEnd + pSubFolder->mNameLen, apDeleteFileTime);
                pSubFolder->FsFileTreeDeleted(apBuf, apEnd + pSubFolder->mNameLen, apDeleteFileTime);
                pSubFolder->mExistsInFs = false;
                pSubFolder->Release();
            }
        } while (NULL != pTreeNode);
    }

    pTreeNode = K2TREE_FirstNode(&FileChildTree);
    if (NULL != pTreeNode)
    {
        do {
            pSubFile = K2_GET_CONTAINER(VfsFile, pTreeNode, ParentsChildTreeNode);
            pTreeNode = K2TREE_NextNode(&FileChildTree, pTreeNode);
            if ((pSubFile->mExistsInFs) && (pSubFile->mLastIter != sgCurIter))
            {
                pSubFile->mExistsInFs = false;
                K2ASC_Copy(apEnd, pSubFile->mpNameZ);
                K2MEM_Copy(&pSubFile->mTimeStamp, apDeleteFileTime, sizeof(FILETIME));
                if (++apDeleteFileTime->dwLowDateTime == 0)
                    ++apDeleteFileTime->dwHighDateTime;
                pSubFile->OnFsEvent(VfsFileEvent_Deleted);
                pSubFile->Release();
            }
        } while (NULL != pTreeNode);
    }
}

void
VfsFolder::FsFolderTreeDeleted(
    char *          apBuf, 
    char *          apEnd, 
    FILETIME const *apFileTime
)
{
    K2TREE_NODE *   pTreeNode;
    VfsNode *       pNode;
    VfsFolder *     pFolderItem;
    UINT_PTR        pathLen;

    *apEnd = '\\';
    apEnd++;
    *apEnd = 0;
    pathLen = (UINT_PTR)(apEnd - apBuf);

    pTreeNode = K2TREE_FirstNode(&FolderChildTree);
    if (NULL != pTreeNode)
    {
        do {
            pNode = K2_GET_CONTAINER(VfsNode, pTreeNode, ParentsChildTreeNode);
            pTreeNode = K2TREE_NextNode(&FolderChildTree, pTreeNode);
            if (pNode->mExistsInFs)
            {
                K2ASC_Copy(apEnd, pNode->mpNameZ);
                pFolderItem = (VfsFolder *)pNode;
                pFolderItem->FsFolderTreeDeleted(apBuf, apEnd + pNode->mNameLen, apFileTime);
                pFolderItem->FsFileTreeDeleted(apBuf, apEnd + pNode->mNameLen, apFileTime);
                pNode->mExistsInFs = false;
                pNode->Release();
            }
        } while (NULL != pTreeNode);
    }

    --apEnd;
    *apEnd = 0;
}

void
VfsFolder::FsFileTreeDeleted(
    char *          apBuf,
    char *          apEnd,
    FILETIME const *apFileTime
)
{
    K2TREE_NODE *   pTreeNode;
    VfsNode *       pNode;
    UINT_PTR        pathLen;

    *apEnd = '\\';
    apEnd++;
    *apEnd = 0;
    pathLen = (UINT_PTR)(apEnd - apBuf);

    pTreeNode = K2TREE_FirstNode(&FileChildTree);
    if (NULL != pTreeNode)
    {
        do {
            pNode = K2_GET_CONTAINER(VfsNode, pTreeNode, ParentsChildTreeNode);
            pTreeNode = K2TREE_NextNode(&FileChildTree, pTreeNode);
            if (pNode->mExistsInFs)
            {
                K2ASC_Copy(apEnd, pNode->mpNameZ);
                K2MEM_Copy(&((VfsFile *)pNode)->mTimeStamp, apFileTime, sizeof(FILETIME));
                pNode->mExistsInFs = false;
                ((VfsFile *)pNode)->OnFsEvent(VfsFileEvent_Deleted);
                pNode->Release();
            }
        } while (NULL != pTreeNode);
    }

    --apEnd;
    *apEnd = 0;
}
