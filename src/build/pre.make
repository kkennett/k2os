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

#MAKEFLAGS += -s
MAKEFLAGS += --stop

#========================================================================================
# these symbols must be defined before make is invoked
#========================================================================================

ifeq ($(K2_ROOT),)
$(error K2_ROOT not set)
endif

ifeq ($(K2_OS),)
$(error K2_OS not set)
endif

ifeq ($(K2_ARCH),)
$(error error - no arch set. use 'setarch xxx' at command line)
endif

ifeq ($(K2_ARCH),X32)
K2_ARCH_IS_INTEL := 1
K2_ARCH_IS_ARM   := 0
K2_ARCH_BITS     := 32
else
ifeq ($(K2_ARCH),A32)
K2_ARCH_IS_INTEL := 0
K2_ARCH_IS_ARM   := 1
K2_ARCH_BITS     := 32
else
ifeq ($(K2_ARCH),X64)
K2_ARCH_IS_INTEL := 1
K2_ARCH_IS_ARM   := 0
K2_ARCH_BITS     := 64
else
ifeq ($(K2_ARCH),A64)
K2_ARCH_IS_INTEL := 0
K2_ARCH_IS_ARM   := 1
K2_ARCH_BITS     := 64
else
$(error K2_ARCH set to invalid/unknown architecture ($(K2_ARCH)))
endif
endif
endif
endif

#========================================================================================
# ALL
#========================================================================================
empty := 
space := $(empty) $(empty)
comma := ,

K2_SUBPATH := $(shell $(K2_ROOT)/k2tools/k2setsub.bat)
K2_TARGET_NAME := $(notdir $(K2_SUBPATH))
K2_SOURCE_PATH := $(K2_ROOT)/src/$(K2_SUBPATH)
K2_TARGET_BASE ?= $(K2_ROOT)/bld/out/gcc
K2_TEMP_BASE   ?= $(K2_ROOT)/bld/tmp/gcc
K2_IMAGE_BASE  ?= $(K2_ROOT)/img

GCCOPT_SYSTEM += -DK2_OS=$(K2_OS)

SOURCES             := 
SOURCE_LIBS         :=
STATIC_LIBS         := $(AUX_STATIC_LIBS)
KERN_SOURCE_LIBS    := 
TARGET_LIBS         :=
GCCOPT              := $(GCCOPT_SYSTEM) -fdata-sections --function-sections
GCCOPT_S            :=
GCCOPT_C            :=
GCCOPT_CPP          := -fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables

#LDOPT               := --gc-sections
LDOPT               := 

ifneq ($(K2_MODE),FINAL)
K2_BUILD_SPEC := $(K2_ARCH)/Debug
GCCOPT_DEBUG  := -ggdb -g2
LDOPT         += 
else
K2_BUILD_SPEC := $(K2_ARCH)/NoDebug
LDOPT         += --strip-debug
endif

LIBGCC_PATH := $(K2_TOOLS_BIN_PATH)/libgcc.a

