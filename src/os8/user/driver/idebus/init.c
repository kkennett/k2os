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

#include "idebus.h"

K2STAT
IDEBUS_InitAndDiscoverChannel(
    UINT_PTR aChannel
)
{
    if (!K2OS_CritSec_Init(&gChannel[aChannel].Sec))
    {
        K2OSDrv_DebugPrintf("*** IdeBus(%d): critsec init for channel %d failed (0x%08X)\n", K2OS_Process_GetId(), aChannel, K2OS_Thread_GetLastStatus());
        return K2OS_Thread_GetLastStatus();
    }


    return K2STAT_NO_ERROR;
}

K2STAT 
IDEBUS_InitAndDiscover(
    void
)
{
    K2STAT      stat;
    UINT_PTR    ix;

    K2MEM_Zero(&gChannel, sizeof(gChannel));

    for (ix = 0; ix < 4; ix++)
    {
        gChannel[ix >> 1].Device[ix & 1].mLocation = ix;
    }

    for (ix = 0; ix < 2; ix++)
    {
        stat = IDEBUS_InitAndDiscoverChannel(ix);
        if (K2STAT_IS_ERROR(stat))
            return stat;
    }

    return K2STAT_NO_ERROR;
}
