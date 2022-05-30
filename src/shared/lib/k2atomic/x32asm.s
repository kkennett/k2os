/*   
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
*/
#include <k2asmx32.inc>

/*-------------------------------------------------------------------------------*/
// UINT32 K2_CALLCONV_REGS K2ATOMIC_CompareExchange32(UINT32 volatile *apMem, UINT32 aNewVal, UINT32 aComparand);
BEGIN_X32_PROC(K2ATOMIC_CompareExchange32)
    mov %eax, dword ptr [%esp + 4]
    lock cmpxchg dword ptr [%ecx], %edx
    ret 4 
END_X32_PROC(K2ATOMIC_CompareExchange32)

/*-------------------------------------------------------------------------------*/
// UINT32 K2_CALLCONV_REGS K2ATOMIC_CompareExchange64(UINT64 volatile *apMem, UINT64 aNewVal, UINT64 aComparand);
BEGIN_X32_PROC(K2ATOMIC_CompareExchange64)
    push        %ebp  
    mov         %ebp,%esp  
    mov         %eax,dword ptr [%ebp+16]  
    mov         %edx,dword ptr [%ebp+20]  
    push        %ebx  
    push        %esi  
    mov         %esi,%ecx  
    mov         %ecx,dword ptr [%ebp+12]  
    mov         %ebx,dword ptr [%ebp+8]  
    lock cmpxchg8b qword ptr [%esi]  
    pop         %esi  
    pop         %ebx  
    pop         %ebp  
    ret         16  
END_X32_PROC(K2ATOMIC_CompareExchange64)

/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/

    .end
