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

GCCOPT += -I $(K2_ROOT)/src/$(K2_OS)/user/crt

TARGET_TYPE = LIB

SOURCES += entry.c
SOURCES += atexit.c
SOURCES += thread.c
SOURCES += xdlhost.c
SOURCES += debug.c
SOURCES += heap.c
SOURCES += process.c
SOURCES += tls.c 
SOURCES += critsec.c 
SOURCES += exception.c
SOURCES += kern.c
SOURCES += token.c 
SOURCES += pagearray.c
SOURCES += map.c
SOURCES += virt.c
SOURCES += launch.c
SOURCES += time.c
SOURCES += system.c
SOURCES += cache.c
SOURCES += notify.c
SOURCES += mail.c
SOURCES += wait.c
SOURCES += gate.c
SOURCES += alarm.c
SOURCES += sem.c
SOURCES += iface.c
SOURCES += ipc.c
SOURCES += root.c
SOURCES += intr.c

include $(K2_ROOT)/src/build/post.make
