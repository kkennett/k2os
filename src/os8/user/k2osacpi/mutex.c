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

#include "ik2osacpi.h"


ACPI_STATUS
AcpiOsCreateMutex(
    ACPI_MUTEX *OutHandle
)
{
    K2OS_CRITSEC *pSec;

    pSec = K2OS_Heap_Alloc(sizeof(K2OS_CRITSEC));
    if (NULL == pSec)
    {
        K2_ASSERT(0);
        return AE_ERROR;
    }
    if (!K2OS_CritSec_Init(pSec))
    {
        K2_ASSERT(0);
        K2OS_Heap_Free(pSec);
        return AE_ERROR;
    }
    *OutHandle = pSec;
    return AE_OK;
}

void
AcpiOsDeleteMutex(
    ACPI_MUTEX  Handle
)
{
    K2OS_CRITSEC *pSec;

    pSec = (K2OS_CRITSEC *)Handle;
    
    K2OS_CritSec_Done(pSec);
    
    K2OS_Heap_Free(pSec);
}

ACPI_STATUS
AcpiOsAcquireMutex(
    ACPI_MUTEX  Handle,
    UINT16      Timeout
)
{
    K2OS_CRITSEC *pSec;

    pSec = (K2OS_CRITSEC *)Handle;

    if (Timeout == ACPI_WAIT_FOREVER)
    {
        K2OS_CritSec_Enter(pSec);
    }
    else if (Timeout == ACPI_DO_NOT_WAIT)
    {
        if (!K2OS_CritSec_TryEnter(pSec))
            return AE_ERROR;
    }
    else
    {
        K2_ASSERT(0);
    }

    return AE_OK;
}

void
AcpiOsReleaseMutex(
    ACPI_MUTEX  Handle
)
{
    K2OS_CRITSEC *pSec;

    pSec = (K2OS_CRITSEC *)Handle;

    K2OS_CritSec_Leave(pSec);

}

