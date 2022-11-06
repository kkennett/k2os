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
#include "k2vfs.h"

enum ScanHandle
{
    ScanHandle_Dir = 0,
    ScanHandle_Sub,
    ScanHandle_Stop,
    ScanHandle_Count
};

char const *    gpVfsRootSpec;
UINT_PTR        gVfsRootSpecLen;

static HANDLE   sghWait[ScanHandle_Count];
static char     sgScanBuf[1028];

K2TREE_ANCHOR VfsNode::WorkTree;


int 
VfsNode::sTimeCompare(
    UINT_PTR        aKey, 
    K2TREE_NODE *   apNode
)
{
    FILETIME const *pTimeKey;
    VfsFile *       pVfsFile;

    K2_ASSERT(0 != aKey);
    pTimeKey = (FILETIME const *)aKey;
    pVfsFile = K2_GET_CONTAINER(VfsFile, apNode, WorkTreeNode);
    K2_ASSERT(pVfsFile->mInWorkTree);
    return CompareFileTime(pTimeKey, &pVfsFile->mTimeStamp);
}

int 
VfsNode::sNameCompare(
    UINT_PTR        aKey, 
    K2TREE_NODE *   apTreeNode
)
{
    VfsNode * pItem;

    K2_ASSERT(0 != aKey);
    pItem = K2_GET_CONTAINER(VfsNode, apTreeNode, ParentsChildTreeNode);
    return K2ASC_CompIns((char const *)aKey, pItem->mpNameZ);
}

VfsNode::VfsNode(
    VfsFolder *     apParentFolder,
    char *          apNameZ,
    UINT_PTR        aNameLen,
    bool            aExistsInFs,
    bool            aIsFolder
) :
    mpParentFolder(apParentFolder),
    mIsFolder(aIsFolder),
    mpNameZ(apNameZ),
    mNameLen(aNameLen),
    mDepth((apParentFolder == NULL) ? 0 : apParentFolder->mDepth + 1),
    mExistsInFs(aExistsInFs)
{
    char *pChk;
    char ch;

    mRefCount = 1;
    mVersion = 1;
    mFullPathLen = (NULL == mpParentFolder) ? 0 : (mpParentFolder->mFullPathLen + 1);
    mFullPathLen += mNameLen;
    if (NULL != mpParentFolder)
    {
        mpParentFolder->AddRef();
        K2TREE_Insert(mIsFolder ? &mpParentFolder->FolderChildTree : &mpParentFolder->FileChildTree, (UINT_PTR)mpNameZ, &ParentsChildTreeNode);
    }
    else
    {
        K2MEM_Zero(&ParentsChildTreeNode, sizeof(ParentsChildTreeNode));
    }
    mLastIter = VfsFolder::sgCurIter;

    pChk = mpNameZ + mNameLen;
    do {
        --pChk;
        ch = *pChk;
        if ('.' == ch)
        {
            pChk++;
            break;
        }
    } while (pChk != mpNameZ);
    mExtOffset = (UINT_PTR)(pChk - mpNameZ);
}

VfsNode::~VfsNode(
    void
)
{
    K2_ASSERT(0 == mRefCount);
    if (NULL != mpParentFolder)
    {
        K2TREE_Remove(mIsFolder ? &mpParentFolder->FolderChildTree : &mpParentFolder->FileChildTree, &ParentsChildTreeNode);
        mpParentFolder->Release();
    }
    delete[] mpNameZ;
    mFullPathLen = 0;
}

void 
VfsNode::AddRef(
    void
)
{
    K2ATOMIC_Add((volatile INT_PTR *)&mRefCount, 1);
}

void 
VfsNode::Release(
    void
)
{
    INT_PTR refs;

    K2_ASSERT(0 != mRefCount);

    refs = K2ATOMIC_Add((volatile INT_PTR *)&mRefCount, -1);

    if (0 != refs)
        return;

    delete this;
}

UINT_PTR
VfsNode::GenName(
    char *  apFullPathBuf
) const
{
    UINT_PTR result;

    if (NULL != mpParentFolder)
    {
        result = mpParentFolder->GenName(apFullPathBuf);
        apFullPathBuf += result;
        *apFullPathBuf = '\\';
        apFullPathBuf++;
        ++result;
    }
    else
    {
        result = 0;
    }

    result += K2ASC_Copy(apFullPathBuf, mpNameZ);

    return result;
}

char * 
VfsNode::AllocFullPath(
    UINT_PTR    aExtraChars, 
    UINT_PTR *  apRetLen
) const
{
    char *      pRet;
    UINT_PTR    len;

    len = mNameLen;
    if (NULL != mpParentFolder)
    {
        len += 1 + mpParentFolder->mFullPathLen;
    }
    pRet = new char[(len + aExtraChars + 4) & ~3];
    K2_ASSERT(NULL != pRet);

    GenName(pRet);

    if (NULL != apRetLen)
        *apRetLen = len;

    return pRet;
}

void
Vfs_Run(
    Vfs_pf_Eval                 aEvalFunc, 
    Vfs_pf_OnDiscoverNewFile    aOnDiscoverNew
)
{
    char *          pRootName;
    char *          pEnd;
    FILETIME        fileTime;
    DWORD           waitResult;
    K2TREE_NODE *   pTreeNode;
    VfsFile *       pVfsFile;

    pRootName = new char[(gVfsRootSpecLen + 4) & ~3];
    K2_ASSERT(NULL != pRootName);
    K2ASC_CopyLen(pRootName, gpVfsRootSpec, gVfsRootSpecLen);
    pRootName[gVfsRootSpecLen] = 0;

    DWORD fAttr = GetFileAttributes(pRootName);
    if (INVALID_FILE_ATTRIBUTES == fAttr)
    {
        printf("Specified root [%s] does not exist\n", pRootName);
        ExitProcess(-6);
    }
    else if (0 == (fAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        printf("Specified root [%s] is not a directory\n", pRootName);
        ExitProcess(-5);
    }

    VfsFolder::sgpRoot = new VfsFolder(NULL, pRootName, gVfsRootSpecLen, true);
    K2_ASSERT(NULL != VfsFolder::sgpRoot);

    K2MEM_Zero(sghWait, sizeof(HANDLE) * ScanHandle_Count);
    K2MEM_Zero(sgScanBuf, sizeof(sgScanBuf));

    do {
        sghWait[ScanHandle_Dir] = FindFirstChangeNotification(
            gpVfsRootSpec,
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_LAST_WRITE
        );
        if (INVALID_HANDLE_VALUE == sghWait[ScanHandle_Dir])
            break;

        do {
            sghWait[ScanHandle_Sub] = FindFirstChangeNotification(
                gpVfsRootSpec,
                TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_LAST_WRITE
            );
            if (INVALID_HANDLE_VALUE == sghWait[ScanHandle_Sub])
                break;

            sghWait[ScanHandle_Stop] = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (NULL != sghWait[ScanHandle_Stop])
            {
                K2TREE_Init(&VfsNode::WorkTree, VfsNode::sTimeCompare);
                do {
                    pEnd = &sgScanBuf[VfsFolder::Root().GenName(sgScanBuf)];

                    VfsFolder::sgCurIter++;

                    GetSystemTimeAsFileTime(&fileTime);

                    VfsFolder::Root().SubScan(sgScanBuf, pEnd, &fileTime);

                    waitResult = WaitForMultipleObjects(ScanHandle_Count, sghWait, FALSE, 0);
                    if (waitResult == WAIT_TIMEOUT)
                    {
                        pTreeNode = K2TREE_FirstNode(&VfsNode::WorkTree);
                        if (NULL != pTreeNode)
                        {
                            do {
                                pVfsFile = K2_GET_CONTAINER(VfsFile, pTreeNode, WorkTreeNode);
                                K2_ASSERT(pVfsFile->mInWorkTree);
                                pTreeNode = K2TREE_NextNode(&VfsNode::WorkTree, pTreeNode);
                                pVfsFile->OnFsExec(aOnDiscoverNew);
                                // pVfsFile may be GONE here
                            } while (NULL != pTreeNode);
                        }
                        if (!aEvalFunc())
                        {
                            waitResult = WaitForMultipleObjects(ScanHandle_Count, sghWait, FALSE, INFINITE);
                            if ((waitResult != (WAIT_OBJECT_0 + ScanHandle_Dir)) &&
                                (waitResult != (WAIT_OBJECT_0 + ScanHandle_Sub)))
                                break;
                        }
                    }
                    FindNextChangeNotification(sghWait[ScanHandle_Dir]);
                    FindNextChangeNotification(sghWait[ScanHandle_Sub]);

                    VfsFolder::sgAfterFirstScan = true;

                } while (true);

                CloseHandle(sghWait[ScanHandle_Stop]);
            }

            FindCloseChangeNotification(sghWait[ScanHandle_Sub]);

        } while (0);

        FindCloseChangeNotification(sghWait[ScanHandle_Dir]);

    } while (0);
}

