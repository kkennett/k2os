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

#include "k2osexec.h"

K2TREE_ANCHOR        gFsNodeMapTree;
K2OSKERN_SEQLOCK     gFsNodeMapTreeSeqLock;

BOOL
K2OSEXEC_KernFile_Init(
    K2OSKERN_FILE *apKernFile
)
{
    K2MEM_Zero(apKernFile, sizeof(K2OSKERN_FILE));
    if (!K2OS_CritSec_Init(&apKernFile->Sec))
        return FALSE;
    apKernFile->mRefCount = 1;
    K2LIST_Init(&apKernFile->Locked.UseList);
    return TRUE;
}

UINT32 
K2OSEXEC_KernFile_AddRef(
    K2OSKERN_FILE *apKernFile
)
{
    return K2ATOMIC_Inc(&apKernFile->mRefCount);
}

UINT32 
K2OSEXEC_KernFile_Release(
    K2OSKERN_FILE *apKernFile
)
{
    UINT32              result;
    BOOL                disp;
    K2OSKERN_FSNODE *   pFsNode;

    disp = K2OSKERN_SeqLock(&gFsNodeMapTreeSeqLock);

    result = K2ATOMIC_Dec(&apKernFile->mRefCount);
    if (0 == result)
    {
        K2_ASSERT(apKernFile != gFsMgr.mpKernFileForRoot);

        K2_ASSERT(0 == apKernFile->Locked.UseList.mNodeCount);

        pFsNode = (K2OSKERN_FSNODE *)apKernFile->MapTreeNode.mUserVal;
        
        if (0 != apKernFile->MapTreeNode.mUserVal)
        {
            K2TREE_Remove(&gFsNodeMapTree, &apKernFile->MapTreeNode);
        }
    }
    else
    {
        pFsNode = NULL;
    }

    K2OSKERN_SeqUnlock(&gFsNodeMapTreeSeqLock, disp);

    if (0 == result)
    {
        //
        // kernfile removed from map tree so we can 
        // purge it now - any simultaneous attach will
        // get a new kernfile and not this one
        //
        K2OS_CritSec_Done(&apKernFile->Sec);

        K2OS_Heap_Free(apKernFile);

        if (NULL != pFsNode)
        {
            pFsNode->Static.Ops.Kern.Release(pFsNode);
        }
    }

    return result;
}

typedef struct _AcquireArgs AcquireArgs;
struct _AcquireArgs
{
    char const *        mpPath;
    UINT32              mAccess;
    UINT32              mShare;
    K2OS_FileOpenType   mOpenType;
    UINT32              mOpenFlags;
    UINT32              mNewFileAttrib;
};

K2STAT
iAcquire(
    K2OSKERN_FSNODE *   apFsNode,
    AcquireArgs *       apArgs,
    K2OSKERN_FILE **    appRetFile
)
{
    UINT32              len;
    char const *        pEnd;
    char                ch;
    char                saveCh;
    BOOL                disp;
    K2STAT              stat;
    K2TREE_NODE *       pTreeNode;
    K2OSKERN_FSNODE *   pChildFsNode;
    K2OSKERN_FILESYS *  pFileSys;
    K2OSKERN_FILE *     pNewKernFile;
    K2OSKERN_FILE *     pUseKernFile;
    char                tempBuf[K2OS_FSITEM_MAX_COMPONENT_NAME_LENGTH + 1];

    // eat all slashes and backslashes at the beginning of the component
    do
    {
        ch = *apArgs->mpPath;
        if (('\\' != ch) && ('/' != ch))
            break;
        apArgs->mpPath++;
    } while (0 != ch);

    // check for illegal character as first char in component name
    if ((ch <= ' ') || (ch >= 127))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    // deal with . and ..
    if (ch == '.')
    {
        // path so far ends in ~.
        ch = apArgs->mpPath[1];
        if (ch == '.')
        {
            // path so far ends in ~..
            ch = apArgs->mpPath[2];
            if ((ch == '/') || (ch == '\\') || (0 == ch))
            {
                // path so far ends in ~../ or ~..<nul>
                // ascent does not take a reference
                if (NULL == apFsNode->Static.mpParentDir)
                {
                    return K2STAT_ERROR_BAD_ARGUMENT;
                }
                apArgs->mpPath += 2;
                return iAcquire(apFsNode->Static.mpParentDir, apArgs, appRetFile);
            }
            // path does not end in ~..<nul> or ~../
        }
        else if ((ch == '/') || (ch == '\\') || (0 == ch))
        {
            // path so far ends in ~./
            // staying here does not take a reference
            apArgs->mpPath++;
            return iAcquire(apFsNode, apArgs, appRetFile);
        }
    }

    // ok we are at an actual component name now not <nul> or . or ..
    // and the component name is not empty
    pEnd = apArgs->mpPath + 1;
    saveCh = ch;
    do
    {
        ch = *pEnd;
        if (((ch < ' ') || (ch >= 127)) || (ch == '/') || (ch == '\\'))
            break;
        saveCh = ch;
        pEnd++;
    } while (1);

    if ((saveCh <= ' ') || ((ch != 0) && (ch != '/') && (ch != '\\')))
    {
        // invalid character found in path or space is last char in component name
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    // we are opening from the current point to *pEnd
    len = (UINT32)(pEnd - apArgs->mpPath);
    K2ASC_CopyLen(tempBuf, apArgs->mpPath, len);
    tempBuf[len] = 0;

    pNewKernFile = (K2OSKERN_FILE *)K2OS_Heap_Alloc(sizeof(K2OSKERN_FILE));
    if (NULL == pNewKernFile)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    if (!K2OSEXEC_KernFile_Init(pNewKernFile))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
    }
    else
    {
        //
        // try to get next component in the path
        //
        disp = K2OSKERN_SeqLock(&apFsNode->ChangeSeqLock);
        pTreeNode = K2TREE_Find(&apFsNode->Locked.ChildTree, (UINT_PTR)tempBuf);
        if (NULL != pTreeNode)
        {
            // this part already exists
            pChildFsNode = K2_GET_CONTAINER(K2OSKERN_FSNODE, pTreeNode, ParentLocked.ParentsChildTreeNode);
            // reference taken on descent
            pChildFsNode->Static.Ops.Kern.AddRef(pChildFsNode);
        }
        else
        {
            pChildFsNode = NULL;
        }
        K2OSKERN_SeqUnlock(&apFsNode->ChangeSeqLock, disp);

        if (0 == ch)
        {
            //
            // this is the last component in the path
            //
            if (NULL == pChildFsNode)
            {
                pFileSys = apFsNode->Static.mpFileSys;
                if (NULL == pFileSys)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                }
                else
                {
                    if (NULL != pFileSys->Ops.Fs.AcquireChild)
                    {
                        pChildFsNode = NULL;
                        stat = pFileSys->Ops.Fs.AcquireChild(pFileSys, apFsNode, tempBuf, apArgs->mOpenType, apArgs->mAccess, apArgs->mNewFileAttrib, &pChildFsNode);
                        if (!K2STAT_IS_ERROR(stat))
                        {
                            K2_ASSERT(NULL != pChildFsNode);
                        }
                        else
                        {
                            K2_ASSERT(NULL == pChildFsNode);
                        }
                    }
                    else
                    {
                        stat = K2STAT_ERROR_NOT_FOUND;
                    }
                }
            }
            else
            {
                stat = K2STAT_NO_ERROR;
            }
        }
        else
        {
            //
            // this is not the last component in the path
            // so we need to open the next component as a directory
            //
            if (NULL == pChildFsNode)
            {
                pFileSys = apFsNode->Static.mpFileSys;
                if (NULL == pFileSys)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                }
                else
                {
                    if (NULL != pFileSys->Ops.Fs.AcquireChild)
                    {
                        pChildFsNode = NULL;
                        stat = pFileSys->Ops.Fs.AcquireChild(pFileSys, apFsNode, tempBuf, K2OS_FileOpen_Existing, K2OS_ACCESS_R | apArgs->mAccess, 0, &pChildFsNode);
                        if (!K2STAT_IS_ERROR(stat))
                        {
                            K2_ASSERT(NULL != pChildFsNode);
                            if (!pChildFsNode->Static.mIsDir)
                            {
                                //
                                // intermediate component exists but is not a directory
                                //
                                stat = K2STAT_ERROR_NOT_FOUND;
                                pChildFsNode->Static.Ops.Kern.Release(pChildFsNode);
                                pChildFsNode = NULL;
                            }
                            else
                            {
                                stat = K2STAT_NO_ERROR;
                            }
                        }
                        else
                        {
                            //
                            // next component does not exist in the target file system
                            //
                            K2_ASSERT(NULL == pChildFsNode);
                        }
                    }
                    else
                    {
                        stat = K2STAT_ERROR_NOT_FOUND;
                    }
                }

                if (K2STAT_IS_ERROR(stat))
                {
                    K2_ASSERT(NULL == pChildFsNode);
                }
            }
            else
            {
                if (!pChildFsNode->Static.mIsDir)
                {
                    // intermediate path component is not a directory
                    stat = K2STAT_ERROR_BAD_ARGUMENT;
                    pChildFsNode->Static.Ops.Kern.Release(pChildFsNode);
                    pChildFsNode = NULL;
                }
                else
                {
                    stat = K2STAT_NO_ERROR;
                }
            }
        }

        if (NULL != pChildFsNode)
        {
            K2_ASSERT(!K2STAT_IS_ERROR(stat));

            // 
            // open or get a ref to the open
            //
            disp = K2OSKERN_SeqLock(&gFsNodeMapTreeSeqLock);
            pTreeNode = K2TREE_Find(&gFsNodeMapTree, (UINT_PTR)pChildFsNode);
            if (NULL != pTreeNode)
            {
                pUseKernFile = K2_GET_CONTAINER(K2OSKERN_FILE, pTreeNode, MapTreeNode);
                K2OSEXEC_KernFile_AddRef(pUseKernFile);
                //
                // pChildFsNode ref already held by pUseKernFile
                // pNewKernFile will get deleted below
                //
            }
            else
            {
                // attach new kernfile
                // this is the only place other than init
                // where a kernfile can point at an fsnode
                pNewKernFile->MapTreeNode.mUserVal = (UINT32)pChildFsNode;
                K2TREE_Insert(&gFsNodeMapTree, (UINT_PTR)pChildFsNode, &pNewKernFile->MapTreeNode);
                pUseKernFile = pNewKernFile;
                pNewKernFile->Static.mAccess = apArgs->mAccess;
                pNewKernFile->Static.mShare = apArgs->mShare;
                pNewKernFile = NULL;
                //
                // need ref for entry in the tree
                //
                pChildFsNode->Static.Ops.Kern.AddRef(pChildFsNode);
            }
            K2OSKERN_SeqUnlock(&gFsNodeMapTreeSeqLock, disp);

            //
            // this will undo the extra reference in the case that the
            // file did not have an associated Kernfile 
            //
            pChildFsNode->Static.Ops.Kern.Release(pChildFsNode);

            // 
            // we now hold a reference to pUseKernFile
            // and that holds a reference to the fsnode
            //

            if (0 == ch)
            {
                //
                // we just acquired or created the leaf
                //
                *appRetFile = pUseKernFile;
            }
            else
            {
                //
                // we acquired an intermediate directory,
                // so we descend and release the dir on the
                // way out
                //
                apArgs->mpPath = pEnd;

                K2_ASSERT(0 != pUseKernFile->MapTreeNode.mUserVal);
                stat = iAcquire((K2OSKERN_FSNODE *)pUseKernFile->MapTreeNode.mUserVal, apArgs, appRetFile);

                K2OSEXEC_KernFile_Release(pUseKernFile);
            }
        }

        if (NULL != pNewKernFile)
        {
            K2OSEXEC_KernFile_Release(pNewKernFile);
            pNewKernFile = NULL;
        }
    }

    if (NULL != pNewKernFile)
    {
        K2OS_Heap_Free(pNewKernFile);
    }

    return stat;
}

K2STAT 
K2OSEXEC_KernFile_Acquire(
    K2OSKERN_FILE *     apBaseDir,
    char const *        apPath,
    UINT32              aAccess,
    UINT32              aShare,
    K2OS_FileOpenType   aOpenType,
    UINT32              aOpenFlags,
    UINT32              aNewFileAttrib,
    K2OSKERN_FILE **    appRetFile
)
{
    K2STAT              stat;
    char                ch;
    K2OSKERN_FSNODE *   pFsNode;
    AcquireArgs         args;

    K2_ASSERT(NULL != appRetFile);
    *appRetFile = NULL;

    if ((aOpenType == K2OS_FileOpen_Always) ||
        (aOpenType == K2OS_FileOpen_CreateNew) ||
        (aOpenType == K2OS_FileOpen_CreateOrTruncate))
    {
        K2_ASSERT(0 == (aNewFileAttrib & K2_FSATTRIB_DIR));
    }

    ch = *apPath;
    if ((ch <= ' ') || (ch >= 127))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    // path is nonempty and starts with a non-space character

    if (('/' == ch) || ('\\' == ch))
    {
        // 
        // ignore base directory and start at root
        //
        pFsNode = (K2OSKERN_FSNODE *)gFsMgr.mpKernFileForRoot->MapTreeNode.mUserVal;
    }
    else
    {
        //
        // start at specified base directory
        // empty string case already checked above
        // so ch will not be zero
        //
        pFsNode = (K2OSKERN_FSNODE *)apBaseDir->MapTreeNode.mUserVal;
    }

    K2_ASSERT(NULL != pFsNode);

    pFsNode->Static.Ops.Kern.AddRef(pFsNode);

    args.mpPath = apPath;
    args.mAccess = (aAccess & K2OS_ACCESS_RW) | K2OS_ACCESS_R;
    args.mShare = aShare & K2OS_ACCESS_RW;
    args.mOpenType = aOpenType;
    args.mOpenFlags = aOpenFlags;
    args.mNewFileAttrib = aNewFileAttrib;

    stat = iAcquire(pFsNode, &args, appRetFile);

    pFsNode->Static.Ops.Kern.Release(pFsNode);

    if (!K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL != *appRetFile);
    }

    return stat;
}

K2STAT 
K2OSEXEC_KernFile_Read(
    K2OSKERN_FILE *     apKernFile,
    UINT32              aProcId,
    K2OS_BUFDESC const *apBufDesc,
    UINT64 const *      apOffset,
    UINT32              aByteCountReq,
    UINT32 *            apRetByteCountGot
)
{
    K2OSKERN_FSFILE_LOCK *  pLock;
    UINT32                  transCount;
    UINT64                  workOffset;
    K2OS_BUFDESC            bufDesc;
    K2OSKERN_MAPUSER        mapUser;
    K2STAT                  stat;
    UINT8 *                 pTarget;
    K2OSKERN_FSNODE *       pFsNode;

    K2MEM_Copy(&bufDesc, apBufDesc, sizeof(K2OS_BUFDESC));

    if (bufDesc.mBytesLength < aByteCountReq)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    stat = K2STAT_NO_ERROR;
    pFsNode = (K2OSKERN_FSNODE *)apKernFile->MapTreeNode.mUserVal;
    transCount = 0;
    workOffset = *apOffset;
    do
    {
        pLock = NULL;
        stat = pFsNode->Static.Ops.Fs.LockData(pFsNode, &workOffset, aByteCountReq, FALSE, &pLock);
        if (K2STAT_IS_ERROR(stat))
            break;

        if (0 != aProcId)
        {
            pTarget = NULL;
            mapUser = gKernDdk.MapUserBuffer(aProcId, &bufDesc, (UINT32 *)&pTarget);
            if (NULL == mapUser)
            {
                K2_ASSERT(NULL == pTarget);
            }
        }
        else
        {
            pTarget = (UINT8 *)bufDesc.mAddress;
        }

        if (NULL != pTarget)
        {
            // copy data and add to amount read
            K2MEM_Copy(pTarget, pLock->mpData, pLock->mLockedByteCount);
            transCount += pLock->mLockedByteCount;

            // update source
            aByteCountReq -= pLock->mLockedByteCount;
            workOffset += pLock->mLockedByteCount;

            // update target
            bufDesc.mAddress += pLock->mLockedByteCount;
            bufDesc.mBytesLength -= pLock->mLockedByteCount;

            if (0 != aProcId)
            {
                gKernDdk.UnmapUserBuffer(mapUser);
            }
        }
        else
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }

        pFsNode->Static.Ops.Fs.UnlockData(pLock);

        if (K2STAT_IS_ERROR(stat))
            break;

    } while (0 != aByteCountReq);

    if (NULL != apRetByteCountGot)
    {
        *apRetByteCountGot = transCount;
    }

    return stat;
}
