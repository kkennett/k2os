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

#include "ik2osacpi.h"

K2OS_CRITSEC  gK2OSACPI_CacheSec;
K2LIST_ANCHOR gK2OSACPI_CacheList;

K2OS_CRITSEC  gK2OSACPI_CallbackSec;

K2OSACPI_INIT gInit = { { 0, }, 0, };

void 
K2OSACPI_Init(
    K2OSACPI_INIT const *apInit
)
{
    K2MEM_Copy(&gInit, apInit, sizeof(K2OSACPI_INIT));
}

ACPI_STATUS
AcpiOsInitialize(
    void)
{
    K2_ASSERT(gInit.FwInfo.mFwBasePhys != 0);

    if (!K2OS_CritSec_Init(&gK2OSACPI_CacheSec))
    {
        return AE_ERROR;
    }

    if (!K2OS_CritSec_Init(&gK2OSACPI_CallbackSec))
    {
        return AE_ERROR;
    }

    K2LIST_Init(&gK2OSACPI_CacheList);
    
    return AE_OK;
}

ACPI_STATUS
AcpiOsTerminate(
    void)
{
    K2OS_CritSec_Done(&gK2OSACPI_CacheSec);

    return AE_OK;
}
