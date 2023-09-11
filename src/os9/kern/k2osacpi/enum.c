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

typedef struct _ICONTEXT ICONTEXT;
struct _ICONTEXT
{
    ACPI_HANDLE                 mhParent;
    K2OSACPI_pf_EnumCallback    mfUserCallback;
    UINT32                    mUserContext;
};

static
ACPI_STATUS 
iCallback(
    ACPI_HANDLE Object,
    UINT32      NestingLevel,
    void *      Context,
    void **     ReturnValue
)
{
    ICONTEXT * pContext;
    pContext = (ICONTEXT *)Context;
    pContext->mfUserCallback(pContext->mhParent, Object, pContext->mUserContext);
    return AE_OK;
}

ACPI_STATUS 
K2OSACPI_EnumChildren(
    ACPI_HANDLE                 ahParent,
    K2OSACPI_pf_EnumCallback    aCallback,
    UINT32                    aContext
)
{
    void *      ignored;
    ICONTEXT    context;
    ACPI_STATUS status;

    K2MEM_Zero(&context, sizeof(context));
    context.mhParent = ahParent;
    context.mfUserCallback = aCallback;
    context.mUserContext = aContext;

    status = AcpiUtAcquireMutex(ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE(status))
    {
        return status;
    }

    status = AcpiNsWalkNamespace(ACPI_TYPE_DEVICE, ahParent, 1, ACPI_NS_WALK_UNLOCK, iCallback, NULL, &context, &ignored);

    (void)AcpiUtReleaseMutex(ACPI_MTX_NAMESPACE);

    return status;
}
