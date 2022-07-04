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

TARGET_TYPE = LIB
K2_KERNEL := TRUE

SOURCES += xdl_entry.c
SOURCES += kernex.c
SOURCES += seqlock.c 
SOURCES += debug.c
SOURCES += panic.c
SOURCES += pte.c
SOURCES += phys.c
SOURCES += bit.c
SOURCES += virt.c
SOURCES += cpu.c
SOURCES += sched.c
SOURCES += intr.c
SOURCES += object.c
SOURCES += proc.c
SOURCES += user.c
SOURCES += pagearray.c
SOURCES += filesys.c
SOURCES += map.c
SOURCES += thread.c
SOURCES += timer.c
SOURCES += sched_locked.c
SOURCES += launch.c
SOURCES += xdlhost.c
SOURCES += plat.c
SOURCES += exec.c
SOURCES += info.c
SOURCES += notify.c
SOURCES += wait.c
SOURCES += gate.c
SOURCES += alarm.c
SOURCES += sem.c
SOURCES += mail.c
SOURCES += critsec.c
SOURCES += iface.c
SOURCES += sigproxy.c
SOURCES += ipcend.c
SOURCES += trace.c
SOURCES += bootgraf.c

include $(K2_ROOT)/src/build/post.make

