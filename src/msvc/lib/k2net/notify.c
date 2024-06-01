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

#include "ik2net.h"

K2NET_NOTIFY * gpNotifyList;
K2NET_NOTIFY * gpNotifyListLast;

void
K2NET_QueueNotify(
    K2NET_LAYER *       apLayer,
    K2NET_NotifyType    aNotifyType,
    UINT32              aNotifyData
)
{
    K2NET_NOTIFY *    pNew;

    pNew = (K2NET_NOTIFY *)K2NET_MemAlloc(sizeof(K2NET_NOTIFY));
    if (NULL == pNew)
        return;
    pNew->mpLayer = apLayer;
    pNew->mNotifyType = aNotifyType;
    pNew->mNotifyData = aNotifyData;
    pNew->mpNext = NULL;
    if (NULL == gpNotifyListLast)
    {
        gpNotifyList = pNew;
    }
    else
    {
        gpNotifyListLast->mpNext = pNew;
    }
    gpNotifyListLast = pNew;
}

void
K2NET_DeliverNotifications(
    void
)
{
    K2NET_NOTIFY *pNotify;

    if (NULL == gpNotifyList)
        return;

    do {
        pNotify = gpNotifyList;
        gpNotifyList = gpNotifyList->mpNext;
        if (NULL == gpNotifyList)
        {
            gpNotifyListLast = NULL;
        }

        if (pNotify->mNotifyType == K2NET_Notify_HostIsUp)
        {
            K2_ASSERT(!pNotify->mpLayer->mHostInformedUp);
            pNotify->mpLayer->mHostInformedUp = TRUE;
        }
        else if (pNotify->mNotifyType == K2NET_Notify_HostIsDown)
        {
            K2_ASSERT(pNotify->mpLayer->mHostInformedUp);
            pNotify->mpLayer->mHostInformedUp = FALSE;
        }

        printf("%s(%s - %s, %d)\n", __FUNCTION__, pNotify->mpLayer->mName, K2NET_gNotifyNames[pNotify->mNotifyType], pNotify->mNotifyData);

        if (NULL != pNotify->mpLayer->Api.OnNotify)
        {
            pNotify->mpLayer->Api.OnNotify(&pNotify->mpLayer->Api, pNotify->mNotifyType, pNotify->mNotifyData);
        }

        K2NET_MemFree(pNotify);

    } while (NULL != gpNotifyList);

    printf("\n--------------------------------------------------------------\n\n");
}

