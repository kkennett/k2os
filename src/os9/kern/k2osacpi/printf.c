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

#define ENABLE_ACPI_DEBUG_OUTPUT 0

#if ENABLE_ACPI_DEBUG_OUTPUT

#define LOCBUFFER_CHARS 80

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf(
    const char              *Format,
    ...)
{
    VALIST  vList;
    char    locBuffer[LOCBUFFER_CHARS];

    locBuffer[0] = 0;

    K2_VASTART(vList, Format);

    K2ASC_PrintfVarLen(locBuffer, LOCBUFFER_CHARS, Format, vList);

    K2_VAEND(vList);

    locBuffer[LOCBUFFER_CHARS - 1] = 0;

    K2OS_Debug_OutputString(locBuffer);
}

void
AcpiOsVprintf(
    const char              *Format,
    va_list                 Args)
{
    char    locBuffer[LOCBUFFER_CHARS];

    locBuffer[0] = 0;
    K2ASC_PrintfVarLen(locBuffer, LOCBUFFER_CHARS, Format, Args);
    locBuffer[LOCBUFFER_CHARS - 1] = 0;
    K2OS_Debug_OutputString(locBuffer);
}

#else

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf(
    const char              *Format,
    ...)
{
}

void
AcpiOsVprintf(
    const char              *Format,
    va_list                 Args)
{
}

#endif
