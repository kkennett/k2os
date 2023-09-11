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
#include <lib/k2tree.h>

void 
iK2TREE_RotateLeft(
    K2TREE_ANCHOR * apAnchor, 
    K2TREE_NODE *   apNode
    );

void 
iK2TREE_RotateRight(
    K2TREE_ANCHOR * apAnchor, 
    K2TREE_NODE *   apNode
    );

static
void 
sInsertHelper(
    K2TREE_ANCHOR * apTree,
    UINT_PTR        aKey,
    K2TREE_NODE *   apInsNode
)
{
    K2TREE_NODE *   pWork;
    K2TREE_NODE *   pHold;
    K2TREE_NODE *   nil;

    nil = &apTree->NilNode;

    apInsNode->mpLeftChild = nil;
    apInsNode->mpRightChild = nil;

    pHold = &apTree->RootNode;

    pWork = apTree->RootNode.mpLeftChild;
    while (pWork != nil)
    {
        pHold = pWork;
        if (0 > apTree->mfCompareKeyToNode(aKey, pWork))
        {
            pWork = pWork->mpLeftChild;
        }
        else
        {
            pWork = pWork->mpRightChild;
        }
    }

    K2TREE_NODE_SETPARENT(apInsNode, pHold);

    if ((pHold == &apTree->RootNode) ||
        (0 > apTree->mfCompareKeyToNode(aKey, pHold)))
    {
        pHold->mpLeftChild = apInsNode;
    }
    else
    {
        pHold->mpRightChild = apInsNode;
    }
}

void 
K2TREE_Insert(
    K2TREE_ANCHOR * apTree,
    UINT_PTR        aKey,
    K2TREE_NODE *   apInsNode
)
{
    K2TREE_NODE *   pOther;

    sInsertHelper(apTree, aKey, apInsNode);

    K2TREE_NODE_SETRED(apInsNode);

    while (K2TREE_NODE_ISRED(K2TREE_NODE_PARENT(apInsNode)))
    {
        if (K2TREE_NODE_PARENT(apInsNode) == K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode))->mpLeftChild)
        {
            pOther = K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode))->mpRightChild;
            if (K2TREE_NODE_ISRED(pOther))
            {
                K2TREE_NODE_SETBLACK(K2TREE_NODE_PARENT(apInsNode));
                K2TREE_NODE_SETBLACK(pOther);
                K2TREE_NODE_SETRED(K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode)));
                apInsNode = K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode));
            }
            else
            {
                if (apInsNode == K2TREE_NODE_PARENT(apInsNode)->mpRightChild)
                {
                    apInsNode = K2TREE_NODE_PARENT(apInsNode);
                    iK2TREE_RotateLeft(apTree, apInsNode);
                }
                K2TREE_NODE_SETBLACK(K2TREE_NODE_PARENT(apInsNode));
                K2TREE_NODE_SETRED(K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode)));
                iK2TREE_RotateRight(apTree, K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode)));
            }
        }
        else
        {
            pOther = K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode))->mpLeftChild;
            if (K2TREE_NODE_ISRED(pOther))
            {
                K2TREE_NODE_SETBLACK(K2TREE_NODE_PARENT(apInsNode));
                K2TREE_NODE_SETBLACK(pOther);
                K2TREE_NODE_SETRED(K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode)));
                apInsNode = K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode));
            }
            else
            {
                if (apInsNode == K2TREE_NODE_PARENT(apInsNode)->mpLeftChild)
                {
                    apInsNode = K2TREE_NODE_PARENT(apInsNode);
                    iK2TREE_RotateRight(apTree, apInsNode);
                }
                K2TREE_NODE_SETBLACK(K2TREE_NODE_PARENT(apInsNode));
                K2TREE_NODE_SETRED(K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode)));
                iK2TREE_RotateLeft(apTree, K2TREE_NODE_PARENT(K2TREE_NODE_PARENT(apInsNode)));
            }
        }
    }
    K2TREE_NODE_SETBLACK(apTree->RootNode.mpLeftChild);
    apTree->mNodeCount++;
}
