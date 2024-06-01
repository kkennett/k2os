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

#include <lib/k2rundown.h>

BOOL    
K2RUNDOWN_Init(
    K2RUNDOWN *apRundown
)
{
    apRundown->mState = 0;

    apRundown->mTokGate = K2OS_Gate_Create(FALSE);

    if (NULL == apRundown->mTokGate)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
K2RUNDOWN_Get(
    K2RUNDOWN *apRundown
)
{
    UINT32 v;

    do {
        v = apRundown->mState;
        if (v & K2RUNDOWN_TRIGGER)
            return FALSE;
    } while (v != K2ATOMIC_CompareExchange(&apRundown->mState, v + 1, v));

    return TRUE;
}

void    
K2RUNDOWN_Put(
    K2RUNDOWN *apRundown
)
{
    UINT32 v;

    do {
        v = apRundown->mState;
        K2_ASSERT(0 != (v & ~K2RUNDOWN_TRIGGER));
    } while (v != K2ATOMIC_CompareExchange(&apRundown->mState, v - 1, v));

    if (v == 1)
    {
        if (v & K2RUNDOWN_TRIGGER)
        {
            K2OS_Gate_Open(apRundown->mTokGate);
        }
    }
}

void    
K2RUNDOWN_Wait(
    K2RUNDOWN *apRundown
)
{
    UINT32          v;
    K2OS_WaitResult waitResult;

    do {
        v = apRundown->mState;
        if (0 != (v & K2RUNDOWN_TRIGGER))
        {
            // already triggered by somebody else
            break;
        }
    } while (v != K2ATOMIC_CompareExchange(&apRundown->mState, v | K2RUNDOWN_TRIGGER, v));

    if (v == 0)
    {
        // nobody was using stuff when we triggered, so we open the gate in case anybody
        // else waits for the rundown
        K2OS_Gate_Open(apRundown->mTokGate);
    }
    else if (0 != (v & ~K2RUNDOWN_TRIGGER))
    {
        // somebody was using stuff when we triggered, so wait for them to be done
        K2OS_Thread_WaitOne(&waitResult, apRundown->mTokGate, K2OS_TIMEOUT_INFINITE);
    }
}

void    
K2RUNDOWN_Done(
    K2RUNDOWN *apRundown
)
{
    K2RUNDOWN_Wait(apRundown);
    K2OS_Token_Destroy(apRundown->mTokGate);
    apRundown->mTokGate = NULL;
}

