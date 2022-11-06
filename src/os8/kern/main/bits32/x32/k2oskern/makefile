#   
#   BSD 3-Clause License
#   
#   Copyright (c) 2020, Kurt Kennett
#   All rights reserved.
#   
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#   
#   1. Redistributions of source code must retain the above copyright notice, this
#      list of conditions and the following disclaimer.
#   
#   2. Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#   
#   3. Neither the name of the copyright holder nor the names of its
#      contributors may be used to endorse or promote products derived from
#      this software without specific prior written permission.
#   
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

include $(K2_ROOT)/src/$(K2_OS)/build/k2ospre.make

TARGET_TYPE = XDL

TARGET_SPECIFIC_ARCH = X32

K2_KERNEL := TRUE

XDL_INF := ../../k2oskern.inf

SOURCES += x32init.c
SOURCES += x32setintr.c
SOURCES += x32time.c
SOURCES += x32gdt.c 
SOURCES += x32gdtasm.s 
SOURCES += x32idt.c 
SOURCES += x32idtasm.s 
SOURCES += x32intrasm.s
SOURCES += x32intrhand.c
SOURCES += x32pte.c
SOURCES += x32devintr.c
SOURCES += x32cpu.c
SOURCES += x32cpuasm.s
SOURCES += x32tlb.c
SOURCES += x32tss.c
SOURCES += x32kerndbg.c
SOURCES += x32ici.c
SOURCES += x32virt.c
SOURCES += x32monitorasm.s
SOURCES += x32user.c
SOURCES += x32thread.c

STATIC_LIBS += @shared/lib/k2archx32
STATIC_LIBS += @shared/lib/k2rofshelp
STATIC_LIBS += @shared/lib/k2heap
STATIC_LIBS += @shared/lib/k2ramheap
STATIC_LIBS += @shared/lib/k2ring
STATIC_LIBS += @shared/lib/k2xdl
STATIC_LIBS += @shared/lib/k2frame

STATIC_KERNEL_LIBS += @$(K2_OS)/kern/main/bits32

include $(K2_ROOT)/src/build/post.make

