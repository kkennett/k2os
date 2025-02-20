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

#include <k2systype.h>

//
// this is statically linked into every XDL.  This is not built by the windows
// compiler.
//
#ifdef _MSC_VER 
#error GCC only
#endif

extern "C"
{

//
// *name* used by GCC linker for global module variable referenced by
// __cxa_atexit function in libgcc.  we have to hold onto this variable
// and use it in module destruction
//
void * __dso_handle = NULL;

//
// generated by k2elf2xdl
//
extern XDL_ELF_ANCHOR const * const gpXdlAnchor;

//
// C++ constructors/destructors
//
typedef void (*__vfpt)();
typedef void (*__vfpv)(void *);

extern __attribute__((weak)) __vfpt __ctors[];
extern __attribute__((weak)) UINT32 __ctors_count;

//
// libgcc functions
//
int  __cxa_atexit(__vfpv f, void * a, XDL *apXdl);
void __call_dtors(XDL *apXdl);

#if K2_TARGET_ARCH_IS_ARM

void 
__aeabi_idiv0(
    void
) 
K2_CALLCONV_NAKED;

void 
__aeabi_idiv0(
    void
)
{
    asm("mov r0, %[code]\n" : : [code]"r"(K2STAT_EX_ZERODIVIDE));
    asm("mov r12, %[targ]\n" : : [targ]"r"((UINT32)K2_RaiseException));
    asm("bx r12\n");
}

void
__aeabi_ldiv0(
    void
)
K2_CALLCONV_NAKED;

void
__aeabi_ldiv0(
    void
)
{
    asm("mov r0, %[code]\n" : : [code]"r"(K2STAT_EX_ZERODIVIDE));
    asm("mov r12, %[targ]\n" : : [targ]"r"((UINT32)K2_RaiseException));
    asm("bx r12\n");
}

int
__aeabi_atexit(
    void *  object, 
    __vfpv  destroyer, 
    void *  dso_handle
)
{
    return __cxa_atexit(destroyer, object, (XDL *)dso_handle);
}

#endif

XDL *
XDL_GetModule(
    void
    )
{
    return (XDL *)__dso_handle;
}

};  // extern "C"

static 
void 
__call_ctors(
    void
)
{
    UINT32 i;
    UINT32 c;

    c = (UINT32)&__ctors_count;
    if (c==0)
        return;
    for(i=0;i<c;i++)
        (*__ctors[i])();
}

extern "C" K2STAT K2_CALLCONV_REGS __attribute__((weak))
xdl_entry(
    XDL *   apXdl,
    UINT32  aReason
)
{
    return K2STAT_NO_ERROR;
}

extern "C" 
K2STAT 
K2_CALLCONV_REGS 
__K2OS_xdl_crt(
    XDL *   apXdl,
    UINT32  aReason
)
{
    K2STAT ret;

    __dso_handle = (void *)apXdl;

    if (aReason == 0xFEEDF00D)
    {
        // this will never execute as aReason will never equal 0xFEEDFOOD
        // it is here to pull in exports
        __dso_handle = (void *)(*((UINT32 *)gpXdlAnchor));
    }

    if (aReason == XDL_ENTRY_REASON_LOAD)
    {
        if ((((UINT32)&__ctors_count)!=0) &&
            (((UINT32)&__ctors)!=0))
            __call_ctors();
    }

    ret = xdl_entry(apXdl, aReason);

    if ((aReason == XDL_ENTRY_REASON_UNLOAD) || (ret != K2STAT_OK))
    {
        if ((((UINT32)&__ctors_count)!=0) &&
            (((UINT32)&__ctors)!=0))
            __call_dtors(apXdl);
    }

    return ret;
}

