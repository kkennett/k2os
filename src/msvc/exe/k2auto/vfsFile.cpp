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

VfsFile::VfsFile(
    VfsFolder * apParentFolder, 
    char *      apNameZ, 
    UINT_PTR    aNameLen,
    bool        aExistsInFs
) :
    VfsNode(apParentFolder, apNameZ, aNameLen, aExistsInFs, false)
{
    K2MEM_Zero(&mTimeStamp, sizeof(mTimeStamp));
    K2LIST_Init(&UserList);
    mEventIter = 0;
    mFileState = VfsFileState_DoesNotExist;
    K2MEM_Zero(&WorkTreeNode, sizeof(WorkTreeNode));
    mInWorkTree = false;
}

VfsFile::~VfsFile(
    void
)
{
    K2MEM_Zero(&mTimeStamp, sizeof(mTimeStamp));
}

void
VfsFile::AddRef(
    void
)
{
    VfsNode::AddRef();
    if (UserList.mNodeCount > 0)
    {
        //
        // a file with users cannot be in discoverpending state
        // so switch it to create pending
        //
        if (VfsFileState_DiscoverPending == mFileState)
            mFileState = VfsFileState_CreatePending;
    }
}

void 
VfsFile::Release(
    void
)
{
    if (VfsFileState_CreatePending == mFileState)
    {
        //
        // a file with no users cannot be in createpending state
        // so switch it back to discoverpending
        //
        if (UserList.mNodeCount == 0)
            mFileState = VfsFileState_DiscoverPending;
    }
    VfsNode::Release();
}

bool 
VfsFile::Exists(
    void
) const
{
    // impossible to call this on a file that is pending discovery
    K2_ASSERT(mFileState != VfsFileState_DiscoverPending);
    return ((mFileState != VfsFileState_DoesNotExist) &&
        (mFileState != VfsFileState_CreatePending));
}

bool 
VfsFile::IsNewerThan(
    VfsFile const *apOtherFile
) const
{
    LONG l;
    
    l = CompareFileTime(&mTimeStamp, &apOtherFile->mTimeStamp);

    return (l > 0) ? true : false;
}

bool 
VfsFile::IsOlderThan(
    VfsFile const *apOtherFile
) const
{
    LONG l;

    l = CompareFileTime(&mTimeStamp, &apOtherFile->mTimeStamp);

    return (l < 0) ? true : false;
}

void
VfsFile::OnFsEvent(
    VfsFileEvent aEvent
)
{
    switch (mFileState)
    {
    case VfsFileState_DoesNotExist:
        K2_ASSERT(VfsFileEvent_Updated != aEvent);
        K2_ASSERT(VfsFileEvent_Deleted != aEvent);
        K2_ASSERT(!mInWorkTree);
        K2TREE_Insert(&WorkTree, (UINT_PTR)&mTimeStamp, &WorkTreeNode);
        mInWorkTree = true;
        AddRef();
        if (VfsFileEvent_Created == aEvent)
        {
            //
            // user created this nonexistet file and now it exists
            //
            mFileState = VfsFileState_CreatePending;
        }
        else
        {
            K2_ASSERT(VfsFileEvent_Discovered == aEvent);
            //
            // unknown nonexistent file came into existence
            //
            mFileState = VfsFileState_DiscoverPending;
        }
        break;

    case VfsFileState_DiscoverPending:
        K2_ASSERT(VfsFileEvent_Discovered != aEvent);
        K2_ASSERT(VfsFileEvent_Created != aEvent);
        // ignore updated
        if (VfsFileEvent_Deleted == aEvent)
        {
            K2_ASSERT(mInWorkTree);
            mInWorkTree = false;
            mFileState = VfsFileState_DoesNotExist;
            K2TREE_Remove(&WorkTree, &WorkTreeNode);
            Release();  // this file may have been deleted after this call
        }
        break;

    case VfsFileState_Inert:
        K2_ASSERT(aEvent > VfsFileEvent_Created);
        K2_ASSERT(!mInWorkTree);
        K2TREE_Insert(&WorkTree, (UINT_PTR)&mTimeStamp, &WorkTreeNode);
        mInWorkTree = true;
        AddRef();
        if (VfsFileEvent_Updated == aEvent)
        {
            mFileState = VfsFileState_UpdatePending;
        }
        else
        {
            K2_ASSERT(VfsFileEvent_Deleted == aEvent);
            mFileState = VfsFileState_DeletePending;
        }
        break;

    case VfsFileState_CreatePending:
    case VfsFileState_UpdatePending:
        K2_ASSERT(VfsFileEvent_Discovered != aEvent);
        K2_ASSERT(VfsFileEvent_Created != aEvent);
        if (VfsFileEvent_Deleted == aEvent)
        {
            K2_ASSERT(mInWorkTree);
            mInWorkTree = false;
            mFileState = VfsFileState_DoesNotExist;
            K2TREE_Remove(&WorkTree, &WorkTreeNode);
            Release();  // this file may have been deleted after this call
        }
        break;

    case VfsFileState_DeletePending:
        if (aEvent != VfsFileEvent_Created)
        {
            K2_ASSERT(VfsFileEvent_Deleted == aEvent);
        }
        else
        {
            K2_ASSERT(VfsFileEvent_Created == aEvent);
            mFileState = VfsFileState_CreatePending;
        }
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
VfsFile::OnFsExec(
    Vfs_pf_OnDiscoverNewFile aOnDiscoverNew
)
{
    K2LIST_LINK *   pListLink;
    VfsUser *       pUser;
    char *          pFullPath;
    VfsFileState    oldFileState;

    //
    // must be a reason we are on work list
    //
    K2_ASSERT(VfsFileState_DoesNotExist != mFileState);
    K2_ASSERT(VfsFileState_Inert != mFileState);

    K2_ASSERT(mInWorkTree);
    K2TREE_Remove(&WorkTree, &WorkTreeNode);
    mInWorkTree = false;
    // release is below

    oldFileState = mFileState;
    if (VfsFileState_DeletePending == mFileState)
    {
        mFileState = VfsFileState_DoesNotExist;
    }
    else
    {
        mFileState = VfsFileState_Inert;
    }

    //
    // we are going to tell somebody something
    //
    pFullPath = AllocFullPath();

    if (VfsFileState_DiscoverPending == oldFileState)
    {
#if 0
        printf("DISCOV %08X%08X %s\n", mTimeStamp.dwHighDateTime, mTimeStamp.dwLowDateTime, pFullPath);
#endif
        K2_ASSERT(0 == UserList.mNodeCount);
        aOnDiscoverNew(pFullPath, pFullPath + GetFullPathLen() - mNameLen);
    }
    else
    {
        pListLink = UserList.mpHead;
        if (NULL != pListLink)
        {
#if 0
            if (VfsFileState_CreatePending == oldFileState)
            {
                printf("CREATE %08X%08X %s\n", mTimeStamp.dwHighDateTime, mTimeStamp.dwLowDateTime, pFullPath);
            }
            else if (VfsFileState_UpdatePending == oldFileState)
            {
                printf("UPDATE %08X%08X %s\n", mTimeStamp.dwHighDateTime, mTimeStamp.dwLowDateTime, pFullPath);
            }
            else if (VfsFileState_DeletePending == oldFileState)
            {
                printf("DELETE %08X%08X %s\n", mTimeStamp.dwHighDateTime, mTimeStamp.dwLowDateTime, pFullPath);
            }
            else
            {
                K2_ASSERT(0);
            }
#endif

            ++mEventIter;

            do {
                pUser = K2_GET_CONTAINER(VfsUser, pListLink, UserListLink);
                pListLink = pListLink->mpNext;
                pUser->mEventIter = mEventIter;
            } while (NULL != pListLink);

            do {
                pListLink = UserList.mpHead;
                if (NULL == pListLink)
                    break;

                do {
                    pUser = K2_GET_CONTAINER(VfsUser, pListLink, UserListLink);

                    pListLink = pListLink->mpNext;

                    if (pUser->mEventIter == mEventIter)
                    {
                        if (VfsFileState_CreatePending == oldFileState)
                        {
                            pUser->OnFsCreate(pFullPath);
                        }
                        else if (VfsFileState_UpdatePending == oldFileState)
                        {
                            pUser->OnFsUpdate(pFullPath);
                        }
                        else
                        {
                            pUser->OnFsDelete(pFullPath);
                        }
                    }

                } while (NULL != pListLink);

            } while (NULL != pListLink);
        }
    }

    Release();    // this file may have been deleted after this call

    delete[] pFullPath;
}

void
VfsFile::DeleteInFs(
    void
)
{
    char *pFullPath;

    pFullPath = AllocFullPath();

    if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(pFullPath))
    {
        if (!DeleteFile(pFullPath))
        {
            printf("*** Fail failed to delete [%s]\n", pFullPath);
        }
    }

    delete[] pFullPath;
}

VfsUser *
VfsFile::GetUserByIndex(
    UINT_PTR aIndex
) const
{
    K2LIST_LINK *pListLink;

    pListLink = UserList.mpHead;

    while (0 != aIndex)
    {
        if (NULL == pListLink)
            return NULL;

        pListLink = pListLink->mpNext;
    }

    return K2_GET_CONTAINER(VfsUser, pListLink, UserListLink);
}
