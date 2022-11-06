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
AcpiOsCreateSemaphore(
    UINT32                  MaxUnits,
    UINT32                  InitialUnits,
    ACPI_SEMAPHORE          *OutHandle)
{
    *OutHandle = K2OS_Semaphore_Create(MaxUnits, InitialUnits);
    if ((*OutHandle) == NULL)
    {
        K2_ASSERT(0);
        return AE_ERROR;
    }

    return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore(
    ACPI_SEMAPHORE          Handle)
{
    if (!K2OS_Token_Destroy(Handle))
    {
        K2_ASSERT(0);
        return AE_ERROR;
    }
    return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore(
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units,
    UINT16                  Timeout)
{
    BOOL    ok;
    UINT32  took;
    UINT32  waitResult;
    UINT32  k2Timeout;
    UINT32  elapsed;
    UINT64  snapTime;
    UINT64  newTime;

    K2_ASSERT(Units > 0);

    if (Timeout == ACPI_WAIT_FOREVER)
        k2Timeout = K2OS_TIMEOUT_INFINITE;
    else
        k2Timeout = (UINT32)Timeout;

    took = 0;
    K2OS_System_GetHfTick(&snapTime);
    do {
        waitResult = K2OS_Wait_One(Handle, k2Timeout);
        if (waitResult != K2OS_THREAD_WAIT_SIGNALLED_0)
        {
            break;
        }
        took++;
        if (took == Units)
            break;
        if (k2Timeout != K2OS_TIMEOUT_INFINITE)
        {
            K2OS_System_GetHfTick(&newTime);
            snapTime = newTime - snapTime;
            if (snapTime & 0xFFFFFFFF00000000ull)
                break;
            elapsed = (UINT32)snapTime;
            if (elapsed >= k2Timeout)
                break;
            k2Timeout -= elapsed;
            snapTime = newTime;
        }
    } while (1);

    if (took != Units)
    {
        if (took > 0)
        {
            ok = K2OS_Semaphore_Inc(Handle, took, &took);
            K2_ASSERT(ok);
        }
        return AE_ERROR;
    }
    return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore(
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units)
{
    BOOL    ok;
    UINT32  newCount;

    K2_ASSERT(Units > 0);

    ok = K2OS_Semaphore_Inc(Handle, Units, &newCount);
    K2_ASSERT(ok);

    return AE_OK;
}



