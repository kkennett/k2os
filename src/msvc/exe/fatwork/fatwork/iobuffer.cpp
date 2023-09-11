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
#include "fatwork.h"

class MyBufferCache;

class MyBufferPage : public IBufferPage
{
public:
    MyBufferPage(MyBufferCache *apParentCache, UINT_PTR aVirtAddr);
    virtual ~MyBufferPage(void);
    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    UINT_PTR GetBaseBlockIx(void)
    {
        return TreeNode.mUserVal;
    }

    UINT8 * GetPtr(void)
    {
        return (UINT8 *)mVirtAddr;
    }

    void Damage(void);
    void SetLru(void);

    MyBufferCache * const   mpParentCache;
    UINT_PTR const          mVirtAddr;

    UINT_PTR        mRefs;
    K2TREE_NODE     TreeNode;
    UINT_PTR        mMask;  // low 16 valid, high 16 dirty
    K2LIST_LINK     FreeListLink;
};

class MyBufferCache : public IBufferCache
{
public:
    MyBufferCache(UINT_PTR aBlockCount, UINT_PTR aBlockSizeBytes) : 
        mBlockCount(aBlockCount),
        mBlockSizeBytes(aBlockSizeBytes),
        mBlocksPerPage(K2_VA_MEMPAGE_BYTES / aBlockSizeBytes)
    {
        mRefs = 1;
        K2TREE_Init(&TreeAnchor, NULL);
        K2LIST_Init(&FreeList);
        K2OS_RwLock_Init(&RwLock);
    }

    virtual ~MyBufferCache(void)
    {
        K2_ASSERT(0 == TreeAnchor.mNodeCount);
        K2_ASSERT(0 == FreeList.mNodeCount);
        K2OS_RwLock_Done(&RwLock);
    }

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    UINT_PTR GetBlocksPerPage(void)
    {
        return mBlocksPerPage;
    }

    IBufferPage *   AcquireBlockPage(UINT_PTR aBlockIx, UINT_PTR *apRetOffset);
    void            PurgeFree(void);

    UINT_PTR const  mBlockCount;
    UINT_PTR const  mBlockSizeBytes;
    UINT_PTR const  mBlocksPerPage;

    K2LIST_ANCHOR   FreeList;
    K2TREE_ANCHOR   TreeAnchor;
    UINT_PTR        mRefs;
    K2OS_RWLOCK     RwLock;
};

MyBufferPage::MyBufferPage(MyBufferCache *apParentCache, UINT_PTR aVirtAddr) : mpParentCache(apParentCache), mVirtAddr(aVirtAddr)
{
    mRefs = 1;
    K2MEM_Zero(&TreeNode, sizeof(TreeNode));
    mMask = 0;
    mpParentCache->AddRef();
}

MyBufferPage::~MyBufferPage(void)
{
    K2_ASSERT(NULL == TreeNode.mpLeftChild);
    VirtualFree((LPVOID)mVirtAddr, 0, MEM_RELEASE);
    mpParentCache->Release();
}

IBufferPage *
MyBufferCache::AcquireBlockPage(
    UINT_PTR    aBlockIx, 
    UINT_PTR *  apRetOffset
)
{
    K2TREE_NODE *   pTreeNode;
    MyBufferPage *  pPage;
    MyBufferPage *  pNewPage;
    UINT_PTR        pageBase;
    UINT_PTR        pageOffset;
    UINT_PTR        virtAddr;
    UINT_PTR        freeCount;

    pageBase = (aBlockIx / mBlocksPerPage) * mBlocksPerPage;
    pageOffset = aBlockIx - pageBase;

    freeCount = 0;

    K2OS_RwLock_ReadLock(&RwLock);

    pTreeNode = K2TREE_Find(&TreeAnchor, pageBase);
    if (NULL != pTreeNode)
    {
        pPage = K2_GET_CONTAINER(MyBufferPage, pTreeNode, TreeNode);
        pPage->AddRef();
    }
    else
    {
        freeCount = FreeList.mNodeCount;
        pPage = NULL;
    }

    K2OS_RwLock_ReadUnlock(&RwLock);

    if (NULL != pTreeNode)
    {
        return pPage;
    }

    if (0 != freeCount)
    {
        // there is likely a free page we can use
        K2OS_RwLock_WriteLock(&RwLock);

        if (0 != FreeList.mNodeCount)
        {
            // move a free page into the tree
            pPage = K2_GET_CONTAINER(MyBufferPage, FreeList.mpHead, FreeListLink);
            pPage->AddRef();
            pPage->mMask = 0;
            pPage->TreeNode.mUserVal = pageBase;
            K2TREE_Insert(&TreeAnchor, pPage->TreeNode.mUserVal, &pPage->TreeNode);
        }

        K2OS_RwLock_WriteUnlock(&RwLock);

        if (NULL != pPage)
        {
            return pPage;
        }
    }

    // create a new page 

    virtAddr = (UINT_PTR)VirtualAlloc(0, K2_VA_MEMPAGE_BYTES, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (0 == virtAddr)
    {
        return NULL;
    }

    pNewPage = new MyBufferPage(this, virtAddr);
    if (NULL == pNewPage)
    {
        VirtualFree((LPVOID)virtAddr, 0, MEM_RELEASE);
        return NULL;
    }

    K2OS_RwLock_WriteLock(&RwLock);

    pTreeNode = K2TREE_Find(&TreeAnchor, pageBase);
    if (NULL != pTreeNode)
    {
        pPage = K2_GET_CONTAINER(MyBufferPage, pTreeNode, TreeNode);
        pPage->AddRef();
    }
    else
    {
        pNewPage->TreeNode.mUserVal = pageBase;
        K2TREE_Insert(&TreeAnchor, pNewPage->TreeNode.mUserVal, &pNewPage->TreeNode);
    }

    K2OS_RwLock_WriteUnlock(&RwLock);

    if (NULL != pTreeNode)
    {
        pNewPage->Release();
        return pPage;
    }

    return pNewPage;
}

IBufferCache * 
CreateBufferCache(
    UINT_PTR aBlockCount,
    UINT_PTR aBlockSizeBytes
)
{
    K2_ASSERT(0 != aBlockCount);
    K2_ASSERT(0 != aBlockSizeBytes);
    K2_ASSERT(0 == (aBlockSizeBytes & (aBlockSizeBytes - 1)));
    K2_ASSERT(aBlockSizeBytes <= K2_VA_MEMPAGE_BYTES);
    return new MyBufferCache(aBlockCount, aBlockSizeBytes);
}
