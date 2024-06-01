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

#include "netmgr.h"

NETDEV_BUFFER * 
NetDev_BufferGet(
    NETDEV *apNetDev
)
{
    NETDEV_BUFFER * pBuffer;

    pBuffer = apNetDev->mpFreeBufFirst;

    if (NULL == pBuffer)
    {
        pBuffer = K2OS_Heap_Alloc(sizeof(NETDEV_BUFFER));
        if (NULL == pBuffer)
            return NULL;
    }
    else
    {
        K2_ASSERT(NULL == pBuffer->mpNetDev);
        apNetDev->mpFreeBufFirst = pBuffer->mpNext;
        if (NULL == pBuffer->mpNext)
        {
            apNetDev->mpFreeBufLast = NULL;
        }
    }

    K2MEM_Zero(pBuffer, sizeof(NETDEV_BUFFER));
    pBuffer->mpNetDev = apNetDev;

    return pBuffer;
}

void            
NetDev_BufferPut(
    NETDEV_BUFFER * apBuffer
)
{
    NETDEV *        pNetDev;
    NETDEV_BUFFER * pLast;

    pNetDev = apBuffer->mpNetDev;
    K2_ASSERT(NULL != pNetDev);

    K2MEM_Zero(apBuffer, sizeof(NETDEV_BUFFER));

    pLast = pNetDev->mpFreeBufLast;
    if (pLast != NULL)
    {
        pLast->mpNext = apBuffer;
    }
    else
    {
        K2_ASSERT(NULL == pNetDev->mpFreeBufFirst);
        pNetDev->mpFreeBufFirst = apBuffer;
    }

    pNetDev->mpFreeBufLast = apBuffer;
}

