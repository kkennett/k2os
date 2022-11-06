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
AcpiOsCreateLock(
    ACPI_SPINLOCK           *OutHandle)
{
    K2OS_CRITSEC *pLock;

    pLock = (K2OS_CRITSEC *)K2OS_Heap_Alloc(sizeof(K2OS_CRITSEC));
    if (pLock == NULL)
    {
        K2_ASSERT(0);
        return AE_ERROR;
    }

    if (!K2OS_CritSec_Init(pLock))
    {
        K2OS_Heap_Free(pLock);
        return AE_ERROR;
    }

    *OutHandle = (ACPI_SPINLOCK)pLock;

    return AE_OK;
}

void
AcpiOsDeleteLock(
    ACPI_SPINLOCK           Handle)
{
    K2OS_CRITSEC *pLock;

    pLock = (K2OS_CRITSEC *)Handle;
    K2_ASSERT(NULL != pLock);

    K2OS_CritSec_Done(pLock);

    K2OS_Heap_Free(pLock);
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock(
    ACPI_SPINLOCK   Handle)
{
    K2OS_CritSec_Enter((K2OS_CRITSEC *)Handle);
    return (ACPI_CPU_FLAGS)0;
}

void
AcpiOsReleaseLock(
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
    K2OS_CritSec_Leave((K2OS_CRITSEC *)Handle);
}

