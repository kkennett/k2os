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

void
NetDev_Insert_Timer(
    NETDEV *        apNetDev,
    NETDEV_TIMER *  apNew
)
{
    NETDEV_TIMER *pFind;
    NETDEV_TIMER *pPrev;

    if (NULL == apNetDev->mpTimers)
    {
        apNew->mpNext = NULL;
        apNetDev->mpTimers = apNew;
    }
    else
    {
        pFind = apNetDev->mpTimers;
        pPrev = NULL;
        do {
            if (apNew->mLeft < pFind->mLeft)
            {
                pFind->mLeft -= apNew->mLeft;
                apNew->mpNext = pFind;
                if (NULL == pPrev)
                {
                    apNetDev->mpTimers = apNew;
                }
                else
                {
                    pPrev->mpNext = apNew;
                }
                break;
            }
            apNew->mLeft -= pFind->mLeft;
            if (NULL == pFind->mpNext)
            {
                pFind->mpNext = apNew;
                apNew->mpNext = NULL;
                break;
            }
            pPrev = pFind;
            pFind = pFind->mpNext;
        } while (1);
    }
}

void
NetDev_OnTimeExpired(
    NETDEV *    apNetDev,
    UINT32      aElapsedMs
)
{
    NETDEV_TIMER *  pTimer;

    NetDev_Arp_OnTimeExpired(apNetDev, aElapsedMs);
    NetDev_Ip_OnTimeExpired(apNetDev, aElapsedMs);

    if (NULL == apNetDev->mpTimers)
        return;

    do {
        pTimer = apNetDev->mpTimers;
        if (pTimer->mLeft > aElapsedMs)
        {
            pTimer->mLeft -= aElapsedMs;
            break;
        }
        aElapsedMs -= pTimer->mLeft;
        pTimer->mLeft = 0;
        while (pTimer->mLeft == 0)
        {
            apNetDev->mpTimers = pTimer->mpNext;

            pTimer->mLeft = pTimer->mPeriod;
            NetDev_Insert_Timer(apNetDev, pTimer);

            // this may remove the timer
            pTimer->mfCallback(apNetDev, pTimer);

            pTimer = apNetDev->mpTimers;
            if (NULL == pTimer)
                break;
        }
    } while ((aElapsedMs > 0) && (NULL != apNetDev->mpTimers));
}

NETDEV_TIMER *
NetDev_AddTimer(
    NETDEV *                    apNetDev,
    UINT32                      aPeriod,
    NETDEV_TIMER_pf_Callback    afCallback
)
{
    NETDEV_TIMER *    pNew;

    pNew = (NETDEV_TIMER *)K2OS_Heap_Alloc(sizeof(NETDEV_TIMER));
    if (NULL != pNew)
    {
        pNew->mLeft = aPeriod;
        pNew->mPeriod = aPeriod;
        pNew->mfCallback = afCallback;
        NetDev_Insert_Timer(apNetDev, pNew);
    }

    return pNew;
}

void
NetDev_DelTimer(
    NETDEV *        apNetDev,
    NETDEV_TIMER *  apTimer
)
{
    NETDEV_TIMER *    pFind;
    NETDEV_TIMER *    pPrev;

    if (NULL == apNetDev->mpTimers)
        return;

    pFind = apNetDev->mpTimers;
    pPrev = NULL;
    do {
        if (pFind == apTimer)
        {
            if (NULL == pPrev)
            {
                apNetDev->mpTimers = pFind->mpNext;
                if (NULL != apNetDev->mpTimers)
                {
                    apNetDev->mpTimers->mLeft += pFind->mLeft;
                }
            }
            else
            {
                pPrev->mpNext = pFind->mpNext;
                if (NULL != pFind->mpNext)
                {
                    pFind->mpNext->mLeft += pFind->mLeft;
                }
            }

            K2OS_Heap_Free(pFind);
            return;
        }
        pPrev = pFind;
        pFind = pFind->mpNext;
    } while (NULL != pFind);
}
