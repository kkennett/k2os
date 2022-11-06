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
#include "crtkern.h"

typedef struct _DTOR DTOR;
struct _DTOR
{
    __vfpv  mFunc;
    void *  mArg;
    XDL *   mpXdl;
    DTOR *  mpNext;
};

static DTOR *       sgpDTORList;
static K2OS_CRITSEC sgSec;

int __cxa_atexit(__vfpv f, void *a, XDL *apXdl)
{
    DTOR *  pDTOR;

    pDTOR = gShared.FuncTab.HeapAlloc(sizeof(DTOR));
    if (pDTOR == NULL)
        return -1;

    pDTOR->mFunc = f;
    pDTOR->mArg = a;
    pDTOR->mpXdl = apXdl;

    gShared.FuncTab.CritSecEnter(&sgSec);

    pDTOR->mpNext = sgpDTORList;
    sgpDTORList = pDTOR;

    gShared.FuncTab.CritSecLeave(&sgSec);

    return 0;
}

void __call_dtors(XDL *apXdl)
{
    DTOR *pPrev;
    DTOR *pScan;
    DTOR *pCall;
    DTOR *pCallEnd;

    pCall = NULL;
    pCallEnd = NULL;

    pPrev = NULL;

    gShared.FuncTab.CritSecEnter(&sgSec);

    pScan = sgpDTORList;

    if (pScan != NULL)
    {
        do
        {
            if (pScan->mpXdl == apXdl)
            {
                if (pCallEnd == NULL)
                    pCall = pScan;
                else
                    pCallEnd->mpNext = pScan;
                pCallEnd = pScan;

                if (pPrev == NULL)
                    sgpDTORList = pScan->mpNext;
                else
                    pPrev->mpNext = pScan->mpNext;

                pScan = pScan->mpNext;

                pCallEnd->mpNext = NULL;
            }
            else
            {
                pPrev = pScan;
                pScan = pScan->mpNext;
            }
        } while (pScan != NULL);
    }

    gShared.FuncTab.CritSecLeave(&sgSec);

    if (pCall == NULL)
        return;

    do
    {
        pCallEnd = pCall;
        pCall = pCall->mpNext;

        pCallEnd->mFunc(pCallEnd->mArg);

        gShared.FuncTab.HeapFree(pCallEnd);

    } while (pCall != NULL);
}

void
CrtKern_Threaded_PostInit(
    void
)
{
    sgpDTORList = NULL;
    gShared.FuncTab.CritSecInit(&sgSec);
}
