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

XDL_INF := ../../../k2oscrt_user.inf

XDL_PLACEMENT := -p 0x402000

SOURCES += getfsdata.s
SOURCES += x32trap.c
SOURCES += x32trapasm.s

#
# functions are exported from these libraries for general use 
#
STATIC_LIBS += @shared/lib/k2mem
STATIC_LIBS += @shared/lib/k2asc
STATIC_LIBS += @shared/lib/k2list
STATIC_LIBS += @shared/lib/k2atomic
STATIC_LIBS += @shared/lib/k2tree
STATIC_LIBS += @shared/lib/k2crc

#
# these are other libraries with pieces consumed here but 
# not exported
#
STATIC_LIBS += @shared/lib/k2xdl
STATIC_LIBS += @shared/lib/k2elf
STATIC_LIBS += @shared/lib/k2bit
STATIC_LIBS += @shared/lib/k2rofshelp
STATIC_LIBS += @shared/lib/k2heap
STATIC_LIBS += @shared/lib/k2ramheap
STATIC_LIBS += @shared/lib/k2ring

STATIC_LIBS += @shared/lib/k2archx32

STATIC_LIBS += @$(K2_OS)/user/crt
STATIC_LIBS += @$(K2_OS)/user/crt/bits32

include $(K2_ROOT)/src/build/post.make


