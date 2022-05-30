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
sK2TREE_RemoveHelper(
    K2TREE_ANCHOR * apTree,
    K2TREE_NODE *   apWork
)
{
    K2TREE_NODE *   pTopNode;
    K2TREE_NODE *   pOther;

    pTopNode = apTree->RootNode.mpLeftChild;
    while ((!K2TREE_NODE_ISRED(apWork)) &&
        (pTopNode != apWork))
    {
        if (apWork == K2TREE_NODE_PARENT(apWork)->mpLeftChild)
        {
            pOther = K2TREE_NODE_PARENT(apWork)->mpRightChild;
            if (K2TREE_NODE_ISRED(pOther))
            {
                K2TREE_NODE_SETBLACK(pOther);
                K2TREE_NODE_SETRED(K2TREE_NODE_PARENT(apWork));
                iK2TREE_RotateLeft(apTree, K2TREE_NODE_PARENT(apWork));
                pOther = K2TREE_NODE_PARENT(apWork)->mpRightChild;
            }
            if ((!K2TREE_NODE_ISRED(pOther->mpRightChild)) &&
                (!K2TREE_NODE_ISRED(pOther->mpLeftChild)))
            {
                K2TREE_NODE_SETRED(pOther);
                apWork = K2TREE_NODE_PARENT(apWork);
            }
            else
            {
                if (!K2TREE_NODE_ISRED(pOther->mpRightChild))
                {
                    K2TREE_NODE_SETBLACK(pOther->mpLeftChild);
                    K2TREE_NODE_SETRED(pOther);
                    iK2TREE_RotateRight(apTree, pOther);
                    pOther = K2TREE_NODE_PARENT(apWork)->mpRightChild;
                }
                K2TREE_NODE_COPYCOLOR(pOther, K2TREE_NODE_PARENT(apWork));
                K2TREE_NODE_SETBLACK(K2TREE_NODE_PARENT(apWork));
                K2TREE_NODE_SETBLACK(pOther->mpRightChild);
                iK2TREE_RotateLeft(apTree, K2TREE_NODE_PARENT(apWork));
                apWork = pTopNode;
            }
        }
        else
        {
            pOther = K2TREE_NODE_PARENT(apWork)->mpLeftChild;
            if (K2TREE_NODE_ISRED(pOther))
            {
                K2TREE_NODE_SETBLACK(pOther);
                K2TREE_NODE_SETRED(K2TREE_NODE_PARENT(apWork));
                iK2TREE_RotateRight(apTree, K2TREE_NODE_PARENT(apWork));
                pOther = K2TREE_NODE_PARENT(apWork)->mpLeftChild;
            }
            if ((!K2TREE_NODE_ISRED(pOther->mpRightChild)) &&
                (!K2TREE_NODE_ISRED(pOther->mpLeftChild)))
            {
                K2TREE_NODE_SETRED(pOther);
                apWork = K2TREE_NODE_PARENT(apWork);
            }
            else
            {
                if (!K2TREE_NODE_ISRED(pOther->mpLeftChild))
                {
                    K2TREE_NODE_SETBLACK(pOther->mpRightChild);
                    K2TREE_NODE_SETRED(pOther);
                    iK2TREE_RotateLeft(apTree, pOther);
                    pOther = K2TREE_NODE_PARENT(apWork)->mpLeftChild;
                }
                K2TREE_NODE_COPYCOLOR(pOther, K2TREE_NODE_PARENT(apWork));
                K2TREE_NODE_SETBLACK(K2TREE_NODE_PARENT(apWork));
                K2TREE_NODE_SETBLACK(pOther->mpLeftChild);
                iK2TREE_RotateRight(apTree, K2TREE_NODE_PARENT(apWork));
                apWork = pTopNode;
            }
        }
    }
    K2TREE_NODE_SETBLACK(apWork);
}

void 
K2TREE_Remove(
    K2TREE_ANCHOR * apTree,
    K2TREE_NODE *   apCutNode
)
{
    K2TREE_NODE *   pWork;
    K2TREE_NODE *   pOther;
    K2TREE_NODE *   nil;
    K2TREE_NODE *   root;
    BOOL            alt;

    nil = &apTree->NilNode;
    root = &apTree->RootNode;

    if ((apCutNode->mpLeftChild == nil) ||
        (apCutNode->mpRightChild == nil))
        pWork = apCutNode;
    else
    {
        pWork = apCutNode->mpRightChild;
        if (nil != pWork)
        {
            while (pWork->mpLeftChild != nil)
            {
                pWork = pWork->mpLeftChild;
            }
        }
        else
        {
            pOther = apCutNode;
            pWork = K2TREE_NODE_PARENT(pOther);
            while (pOther == pWork->mpRightChild)
            {
                pOther = pWork;
                pWork = K2TREE_NODE_PARENT(pWork);
            }
            if (pWork == root)
                pWork = nil;
        }
    }

    if (pWork->mpLeftChild == nil)
        pOther = pWork->mpRightChild;
    else
        pOther = pWork->mpLeftChild;
    K2TREE_NODE_SETPARENT(pOther, K2TREE_NODE_PARENT(pWork));

    if (root == K2TREE_NODE_PARENT(pOther))
    {
        root->mpLeftChild = pOther;
    }
    else
    {
        if (pWork == K2TREE_NODE_PARENT(pWork)->mpLeftChild)
        {
            K2TREE_NODE_PARENT(pWork)->mpLeftChild = pOther;
        }
        else
        {
            K2TREE_NODE_PARENT(pWork)->mpRightChild = pOther;
        }
    }

    alt = (pWork != apCutNode) ? TRUE : FALSE;

    if (!(K2TREE_NODE_ISRED(pWork)))
        sK2TREE_RemoveHelper(apTree, pOther);

    if (alt)
    {
        pWork->mpLeftChild = apCutNode->mpLeftChild;
        pWork->mpRightChild = apCutNode->mpRightChild;
        K2TREE_NODE_SETPARENT(pWork, K2TREE_NODE_PARENT(apCutNode));
        K2TREE_NODE_COPYCOLOR(pWork, apCutNode);

        K2TREE_NODE_SETPARENT(apCutNode->mpLeftChild, pWork);
        K2TREE_NODE_SETPARENT(apCutNode->mpRightChild, pWork);
        if (apCutNode == K2TREE_NODE_PARENT(apCutNode)->mpLeftChild)
        {
            K2TREE_NODE_PARENT(apCutNode)->mpLeftChild = pWork;
        }
        else
        {
            K2TREE_NODE_PARENT(apCutNode)->mpRightChild = pWork;
        }
    }
    apTree->mNodeCount--;
}
