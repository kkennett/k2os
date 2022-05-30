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
    K2TREE_ANCHOR * apTree,
    K2TREE_NODE *   apNode
)
{
    K2TREE_NODE *   pOther;
    K2TREE_NODE *   nil;

    nil = &apTree->NilNode;

    pOther = apNode->mpRightChild;

    apNode->mpRightChild = pOther->mpLeftChild;

    if (pOther->mpLeftChild != nil)
    {
        K2TREE_NODE_SETPARENT(pOther->mpLeftChild, apNode);
    }

    K2TREE_NODE_SETPARENT(pOther, K2TREE_NODE_PARENT(apNode));

    if (apNode == K2TREE_NODE_PARENT(apNode)->mpLeftChild)
    {
        K2TREE_NODE_PARENT(apNode)->mpLeftChild = pOther;
    }
    else
    {
        K2TREE_NODE_PARENT(apNode)->mpRightChild = pOther;
    }

    pOther->mpLeftChild = apNode;

    K2TREE_NODE_SETPARENT(apNode, pOther);
}

void 
iK2TREE_RotateRight(
    K2TREE_ANCHOR * apTree,
    K2TREE_NODE *   apNode
)
{
    K2TREE_NODE *   pOther;
    K2TREE_NODE *   nil;

    nil = &apTree->NilNode;

    pOther = apNode->mpLeftChild;

    apNode->mpLeftChild = pOther->mpRightChild;

    if (pOther->mpRightChild != nil)
    {
        K2TREE_NODE_SETPARENT(pOther->mpRightChild, apNode);
    }

    K2TREE_NODE_SETPARENT(pOther, K2TREE_NODE_PARENT(apNode));

    if (apNode == K2TREE_NODE_PARENT(apNode)->mpLeftChild)
    {
        K2TREE_NODE_PARENT(apNode)->mpLeftChild = pOther;
    }
    else
    {
        K2TREE_NODE_PARENT(apNode)->mpRightChild = pOther;
    }

    pOther->mpRightChild = apNode;

    K2TREE_NODE_SETPARENT(apNode, pOther);
}

