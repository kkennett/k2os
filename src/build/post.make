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

#========================================================================================
# ARCHITECTURE OPTIONS
#========================================================================================
ifneq ($(TARGET_SPECIFIC_ARCH),)
ifneq ($(K2_ARCH),$(TARGET_SPECIFIC_ARCH))
$(error This component for $(TARGET_SPECIFIC_ARCH) only)
endif
endif
ifeq ($(K2_ARCH),A32)
GCCOPT += -march=armv7-a
endif
ifeq ($(K2_ARCH),X32)
GCCOPT += -march=i686 -m32 -malign-double 
endif
ifeq ($(K2_ARCH),A64)
GCCOPT += -march=armv8-a
endif
ifeq ($(K2_ARCH),X64)
GCCOPT += -march=x86-64 -m64 -malign-double 
endif


#========================================================================================
# CHECK TARGET TYPES AND FORM TARGET PATH
#========================================================================================
ifeq ($(TARGET_TYPE),BIN)
	K2_TARGET_PATH := $(K2_TARGET_BASE)/bin/$(K2_BUILD_SPEC)/$(K2_SUBPATH)
else
	ifeq ($(TARGET_TYPE),OBJ)
		K2_TARGET_PATH := $(K2_TARGET_BASE)/obj/$(K2_BUILD_SPEC)/$(K2_SUBPATH)
	else
		ifeq ($(TARGET_TYPE),LIB)
			ifneq ($(K2_KERNEL),)
				K2_TARGET_PATH := $(K2_TARGET_BASE)/lib/kern/$(K2_BUILD_SPEC)/$(K2_SUBPATH)
			else
				K2_TARGET_PATH := $(K2_TARGET_BASE)/lib/$(K2_BUILD_SPEC)/$(K2_SUBPATH)
			endif
		else
			ifeq ($(TARGET_TYPE),ELF)
				ifneq ($(K2_KERNEL),)
					K2_TARGET_PATH := $(K2_TARGET_BASE)/elf/kern/$(K2_BUILD_SPEC)
				else
					K2_TARGET_PATH := $(K2_TARGET_BASE)/elf/$(K2_BUILD_SPEC)
				endif
			else
				ifeq ($(TARGET_TYPE),XDL)
					ifneq ($(K2_KERNEL),)
						K2_TARGET_PATH := $(K2_TARGET_BASE)/xdl/kern/$(K2_BUILD_SPEC)
					else
						K2_TARGET_PATH := $(K2_TARGET_BASE)/xdl/$(K2_BUILD_SPEC)
					endif
				else
					ifeq ($(TARGET_TYPE),IMAGE)
						K2_TARGET_PATH := $(K2_IMAGE_BASE)/$(K2_BUILD_SPEC)
					else
						$(error No Valid TARGET_TYPE defined.)
					endif
				endif
			endif
		endif
	endif
endif

K2_OBJECT_PATH := $(K2_TEMP_BASE)/obj/$(K2_BUILD_SPEC)/$(K2_SUBPATH)
OBJECT_FROM_SOURCE = $(K2_OBJECT_PATH)/$(addsuffix .o, $(basename $(source)))
OBJECTS = $(foreach source, $(SOURCES), $(OBJECT_FROM_SOURCE))

ifneq ($(K2_KERNEL),)
K2_SPEC_KERNEL := -k
else
K2_SPEC_KERNEL := 
endif

.PHONY: default clean always

#========================================================================================
# DEFAULT OPTIONS AND RULES
#========================================================================================
GCCOPT     += -c -nostdinc -fno-common -Wall -I $(K2_ROOT)/src/shared/inc
GCCOPT_S   += 
GCCOPT_C   += 
GCCOPT_CPP += 
LDOPT      += -nostdlib
LDOPT      += -static
LDOPT      += --no-undefined
LDENTRY    ?= -e __entry

BUILD_CONTROL_FILES = makefile $(K2_ROOT)/src/build/pre.make $(K2_ROOT)/src/build/post.make $(K2_ROOT)/src/$(K2_OS)/build/k2ospre.make $(K2_ROOT)/src/$(K2_OS)/build/k2osspec_$(K2_OS).make

$(K2_OBJECT_PATH)/%.o : %.s  $(BUILD_CONTROL_FILES)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
	@echo $(TARGET_NAMEONLY) - Preprocess $<
	@if exist $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).d) del $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).d)
	gcc -x assembler-with-cpp -E $(GCCOPT) $(GCCOPT_S) -D_ASSEMBLER -MT '$(K2_OBJECT_PATH)/$(notdir $@)' -MD -MF $(K2_OBJECT_PATH)/$(basename $<).dx -o $(K2_OBJECT_PATH)/$(basename $<).s $<
	@if exist $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).dx) ren $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).dx) $(notdir $(basename $<).d)
	@echo $(TARGET_NAMEONLY) - Assemble $<
	gcc $(GCCOPT) -o $(K2_OBJECT_PATH)/$(basename $<).o $(K2_OBJECT_PATH)/$(basename $<).s

$(K2_OBJECT_PATH)/%.o : %.c $(BUILD_CONTROL_FILES)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
	@echo $(TARGET_NAMEONLY) - Compile $<
	@if exist $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).d) del $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).d)
	gcc -x c $(GCCOPT) $(GCCOPT_C) $(GCCOPT_DEBUG) -MT '$(K2_OBJECT_PATH)/$(notdir $@)' -MD -MF $(K2_OBJECT_PATH)/$(basename $<).dx -o $(K2_OBJECT_PATH)/$(basename $<).o $<
	@if exist $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).dx) ren $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).dx) $(notdir $(basename $<).d)

$(K2_OBJECT_PATH)/%.o : %.cpp $(BUILD_CONTROL_FILES)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
	@echo $(TARGET_NAMEONLY) - Compile $<
	@if exist $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).d) del $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).d)
	gcc -x c++ $(GCCOPT) $(GCCOPT_C) $(GCCOPT_CPP) $(GCCOPT_DEBUG) -MT '$(K2_OBJECT_PATH)/$(notdir $@)' -MD -MF $(K2_OBJECT_PATH)/$(basename $<).dx -o $(K2_OBJECT_PATH)/$(basename $<).o $<
	@if exist $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).dx) ren $(subst /,\,$(K2_OBJECT_PATH)/$(basename $<).dx) $(notdir $(basename $<).d)


#========================================================================================
# TARGET_TYPE == OBJ
#========================================================================================
ifeq ($(TARGET_TYPE),OBJ)
K2_TARGET_TYPE_DEFINED := TRUE

K2_TARGET_NAME_SPEC := $(K2_TARGET_NAME).o
K2_TARGET_FULL_SPEC := $(K2_TARGET_PATH)/$(K2_TARGET_NAME_SPEC)

default: $(K2_TARGET_FULL_SPEC)

ONE_OBJECT = $(foreach source, $(firstword $(SOURCES)), $(OBJECT_FROM_SOURCE))

$(K2_TARGET_FULL_SPEC): $(ONE_OBJECT) $(BUILD_CONTROL_FILES)
	@-if not exist $(subst /,\,$(K2_TARGET_PATH)) md $(subst /,\,$(K2_TARGET_PATH))
	@echo -------- Placing OBJ $@ --------
	@copy $(subst /,\,$(ONE_OBJECT)) $(subst /,\,$(K2_TARGET_FULL_SPEC)) > nul

endif


#========================================================================================
# TARGET_TYPE == LIB
#========================================================================================
ifeq ($(TARGET_TYPE),LIB)

K2_TARGET_NAME_SPEC := $(K2_TARGET_NAME).lib
K2_TARGET_FULL_SPEC := $(K2_TARGET_PATH)/$(K2_TARGET_NAME_SPEC)

default: $(K2_TARGET_FULL_SPEC)

$(K2_TARGET_FULL_SPEC): $(OBJECTS) $(BUILD_CONTROL_FILES)
	@-if not exist $(subst /,\,$(K2_TARGET_PATH)) md $(subst /,\,$(K2_TARGET_PATH))
	@echo -------- Creating LIB $@ --------
	@ar rco $@ $(OBJECTS)

endif
	
#========================================================================================

$(K2_TARGET_BASE)/lib/$(K2_BUILD_SPEC)/%.lib : always
	@MAKE -S -C $(K2_ROOT)/src/$(subst $(K2_TARGET_BASE)/lib/$(K2_BUILD_SPEC)/,,$(@D))
	@echo.

ONE_K2_STATIC_LIB = $(K2_TARGET_BASE)/lib/$(K2_BUILD_SPEC)/$(basename $(1))/$(notdir $(1)).lib
EXPAND_ONE_STATIC_LIB = $(if $(findstring @,$(libdep)), $(call ONE_K2_STATIC_LIB,$(subst @,,$(libdep))),$(libdep))
STATIC_LIBRARIES = $(foreach libdep, $(STATIC_LIBS), $(EXPAND_ONE_STATIC_LIB))

#========================================================================================

$(K2_TARGET_BASE)/lib/kern/$(K2_BUILD_SPEC)/%.lib : always
	@MAKE -S -C $(K2_ROOT)/src/$(subst $(K2_TARGET_BASE)/lib/kern/$(K2_BUILD_SPEC)/,,$(@D))
	@echo.

ONE_K2_STATIC_KERNEL_LIB = $(K2_TARGET_BASE)/lib/kern/$(K2_BUILD_SPEC)/$(basename $(1))/$(notdir $(1)).lib
EXPAND_ONE_STATIC_KERNEL_LIB = $(if $(findstring @,$(libdep)), $(call ONE_K2_STATIC_KERNEL_LIB,$(subst @,,$(libdep))),$(libdep))
STATIC_KERNEL_LIBRARIES = $(foreach libdep, $(STATIC_KERNEL_LIBS), $(EXPAND_ONE_STATIC_KERNEL_LIB))

#========================================================================================
# TARGET_TYPE == ELF
#========================================================================================
ifeq ($(TARGET_TYPE),ELF)

K2_TARGET_NAME_SPEC := $(K2_TARGET_NAME).elf
K2_TARGET_FULL_SPEC := $(K2_TARGET_PATH)/$(K2_TARGET_NAME_SPEC)

ifeq ($(K2_ARCH_BITS),32)
LINKER_SCRIPT ?= $(K2_ROOT)/src/build/gcc_link32.l
else
LINKER_SCRIPT ?= $(K2_ROOT)/src/build/gcc_link64.l
endif

LDOPT += --script $(LINKER_SCRIPT) -Map $(K2_TARGET_PATH)/$(K2_TARGET_NAME).map

default: $(K2_TARGET_FULL_SPEC)

ifneq ($(K2_KERNEL),)
LIBRARIES = $(STATIC_LIBRARIES) $(STATIC_KERNEL_LIBRARIES)
else
LIBRARIES = $(STATIC_LIBRARIES)
endif

$(K2_TARGET_FULL_SPEC): $(OBJECTS) $(LIBRARIES) $(BUILD_CONTROL_FILES)
	@-if not exist $(subst /,\,$(K2_TARGET_PATH)) md $(subst /,\,$(K2_TARGET_PATH))
	@-if not exist $(subst /,\,$(K2_OBJECT_PATH)) md $(subst /,\,$(K2_OBJECT_PATH))
	@echo -------- Linking ELF $@ --------
	@ld $(LDOPT) $(LDENTRY) -o $@ -( $(LIBGCC_PATH) $(OBJECTS) $(LIBRARIES) -)

endif

#========================================================================================
# TARGET_TYPE == XDL
#========================================================================================
ifeq ($(TARGET_TYPE),XDL)

ifeq ($(XDL_INF),)
$(error No XDL_INF specified)
endif

GCCOPT += -DK2_XDL

ifeq ($(XDL_STACK),)
XDL_STACK := 0
endif

ifeq ($(K2_ARCH_BITS),32)
LINKER_SCRIPT ?= $(K2_ROOT)/src/build/gcc_link32.l
else
LINKER_SCRIPT ?= $(K2_ROOT)/src/build/gcc_link64.l
endif

LDOPT += -q 
LDOPT += --script $(LINKER_SCRIPT)

ifeq ($(basename $(K2_TARGET_NAME)),k2oscrt)

ifeq ($(K2_KERNEL),)
LDENTRY := -e __k2oscrt_user_entry
else
LDENTRY := -e __k2oscrt_kern_entry
endif
CRTSTUB_OBJ := 

else

LDENTRY := -e __K2OS_xdl_crt
CRTSTUB_OBJ := $(K2_TARGET_BASE)/obj/$(K2_BUILD_SPEC)/$(K2_OS)/crtstub/crtstub.o
ifneq ($(K2_KERNEL),)
IMPORT_KERNEL_LIBS += @$(K2_OS)/kern/crt/bits$(K2_ARCH_BITS)/$(K2_ARCH)/k2oscrt
else
IMPORT_LIBS += @$(K2_OS)/user/crt/bits$(K2_ARCH_BITS)/$(K2_ARCH)/k2oscrt
endif

endif

#========================================================================================

K2_TARGET_NAME_SPEC := $(K2_TARGET_NAME).xdl
K2_TARGET_FULL_SPEC := $(K2_TARGET_PATH)/$(K2_TARGET_NAME_SPEC)
K2_TARGET_ELFNAME_SPEC := $(K2_TARGET_NAME).elf
K2_TARGET_ELFFULL_SPEC := $(K2_TARGET_PATH)/srcelf/$(K2_SUBPATH)/$(K2_TARGET_ELFNAME_SPEC)

default: $(K2_TARGET_FULL_SPEC)

#========================================================================================

ifneq ($(K2_KERNEL),)
K2_TARGET_EXPORTLIB_PATH := $(K2_TARGET_BASE)/xdllib/kern/$(K2_BUILD_SPEC)/$(K2_SUBPATH)
else
K2_TARGET_EXPORTLIB_PATH := $(K2_TARGET_BASE)/xdllib/$(K2_BUILD_SPEC)/$(K2_SUBPATH)
endif

K2_TARGET_EXPORTLIB = $(K2_TARGET_EXPORTLIB_PATH)/$(K2_TARGET_NAME).lib

K2_TARGET_KERNEL_IMPORTLIB_PATH := $(K2_TARGET_BASE)/xdllib/kern/$(K2_BUILD_SPEC)
K2_TARGET_IMPORTLIB_PATH := $(K2_TARGET_BASE)/xdllib/$(K2_BUILD_SPEC)

#========================================================================================

$(CRTSTUB_OBJ) : always
	@MAKE -S -C $(K2_ROOT)/src/$(subst $(K2_TARGET_BASE)/obj/$(K2_BUILD_SPEC)/,,$(@D))
	@echo.

#========================================================================================

$(K2_TARGET_IMPORTLIB_PATH)/%.lib: always 
	@MAKE -S -C $(K2_ROOT)/src/$(subst $(K2_TARGET_BASE)/xdllib/$(K2_BUILD_SPEC)/,,$(@D))
	@echo.

ONE_K2_IMPORT_LIB = $(K2_TARGET_IMPORTLIB_PATH)/$(basename $(1))/$(notdir $(1)).lib
EXPAND_ONE_IMPORT_LIB = $(if $(findstring @,$(libdep)), $(call ONE_K2_IMPORT_LIB,$(subst @,,$(libdep))),$(libdep))
IMPORT_LIBRARIES = $(foreach libdep, $(IMPORT_LIBS), $(EXPAND_ONE_IMPORT_LIB))

#========================================================================================

$(K2_TARGET_KERNEL_IMPORTLIB_PATH)/%.lib: always 
	@MAKE -S -C $(K2_ROOT)/src/$(subst $(K2_TARGET_BASE)/xdllib/kern/$(K2_BUILD_SPEC)/,,$(@D))
	@echo.

ONE_K2_IMPORT_KERNEL_LIB = $(K2_TARGET_KERNEL_IMPORTLIB_PATH)/$(basename $(1))/$(notdir $(1)).lib
EXPAND_ONE_IMPORT_KERNEL_LIB = $(if $(findstring @,$(libdep)), $(call ONE_K2_IMPORT_KERNEL_LIB,$(subst @,,$(libdep))),$(libdep))
IMPORT_KERNEL_LIBRARIES = $(foreach libdep, $(IMPORT_KERNEL_LIBS), $(EXPAND_ONE_IMPORT_KERNEL_LIB))

#========================================================================================

ifneq ($(K2_KERNEL),)
LIBRARIES = $(STATIC_LIBRARIES) $(STATIC_KERNEL_LIBRARIES) $(IMPORT_LIBRARIES) $(IMPORT_KERNEL_LIBRARIES)
else
LIBRARIES = $(STATIC_LIBRARIES) $(IMPORT_LIBRARIES)
endif

XDL_INF_O := $(K2_OBJECT_PATH)/exp_$(K2_TARGET_NAME).o
EXPORT_CMD := k2export -i $(XDL_INF) -o $(XDL_INF_O) $(LIBGCC_PATH) $(OBJECTS) $(LIBRARIES) $(CRTSTUB_OBJ)

$(K2_TARGET_ELFFULL_SPEC): $(OBJECTS) $(LIBRARIES) $(CRTSTUB_OBJ) $(XDL_INF) $(BUILD_CONTROL_FILES)
	@-if not exist $(subst /,\,$(K2_TARGET_PATH)) md $(subst /,\,$(K2_TARGET_PATH))
	@-if not exist $(subst /,\,$(K2_TARGET_EXPORTLIB_PATH)) md $(subst /,\,$(K2_TARGET_EXPORTLIB_PATH))
	@-if not exist $(subst /,\,$(K2_TARGET_PATH)/srcelf/$(K2_SUBPATH)) md $(subst /,\,$(K2_TARGET_PATH)/srcelf/$(K2_SUBPATH))
	@-if not exist $(subst /,\,$(K2_OBJECT_PATH)) md $(subst /,\,$(K2_OBJECT_PATH))
	@echo -------- Create Exports for XDL from ELF $@ --------
	@$(EXPORT_CMD)
	@echo -------- Linking ELF for XDL $@ --------
	@ld $(LDOPT) $(LDENTRY) -o $@ -( $(LIBGCC_PATH) $(OBJECTS) $(LIBRARIES) $(CRTSTUB_OBJ) $(XDL_INF_O) -)

$(K2_TARGET_FULL_SPEC): $(K2_TARGET_ELFFULL_SPEC)
	@echo -------- Creating XDL from ELF for $@ --------
	@k2elf2xdl $(K2_SPEC_KERNEL) $(XDL_PLACEMENT) -s $(XDL_STACK) -i $(K2_TARGET_ELFFULL_SPEC) -o $(K2_TARGET_FULL_SPEC) -l $(K2_TARGET_EXPORTLIB)
	
endif


#========================================================================================
# TARGET_TYPE == BIN
#========================================================================================
ifeq ($(TARGET_TYPE),BIN)

ifeq ($(LINKER_SCRIPT),)
$(error Linker script needd)
endif

LDOPT += -q 
LDOPT += --script $(LINKER_SCRIPT)

K2_TARGET_NAME_SPEC := $(K2_TARGET_NAME).bin
K2_TARGET_FULL_SPEC := $(K2_TARGET_PATH)/$(K2_TARGET_NAME_SPEC)
K2_TARGET_ELFNAME_SPEC := $(K2_TARGET_NAME).elf
K2_TARGET_ELFFULL_SPEC := $(K2_TARGET_PATH)/srcelf/$(K2_SUBPATH)/$(K2_TARGET_ELFNAME_SPEC)

default: $(K2_TARGET_FULL_SPEC)

$(K2_TARGET_ELFFULL_SPEC): $(OBJECTS) $(STATIC_LIBRARIES) $(BUILD_CONTROL_FILES)
	@-if not exist $(subst /,\,$(K2_TARGET_PATH)) md $(subst /,\,$(K2_TARGET_PATH))
	@-if not exist $(subst /,\,$(K2_TARGET_PATH)/srcelf/$(K2_SUBPATH)) md $(subst /,\,$(K2_TARGET_PATH)/srcelf/$(K2_SUBPATH))
	@-if not exist $(subst /,\,$(K2_OBJECT_PATH)) md $(subst /,\,$(K2_OBJECT_PATH))
	@echo -------- Linking ELF for XDL $@ --------
	@ld $(LDOPT) $(LDENTRY) -o $@ -( $(LIBGCC_PATH) $(OBJECTS) $(STATIC_LIBRARIES) -)

$(K2_TARGET_FULL_SPEC): $(K2_TARGET_ELFFULL_SPEC)
	@echo -------- Creating BIN from ELF for $@ --------
	@objcopy -O binary $(K2_TARGET_ELFFULL_SPEC) $(K2_TARGET_FULL_SPEC)
	
endif


#========================================================================================
# OS specific (IMAGE, any others)
#========================================================================================
include $(K2_ROOT)/src/$(K2_OS)/build/k2osspec_$(K2_OS).make


#========================================================================================
# Autodependencies
#========================================================================================
-include $(OBJECTS:.o=.d)


#========================================================================================
# optional clean
#========================================================================================
clean:
	@-if exist "$(subst /,\,$(K2_TARGET_FULL_SPEC))" del "$(subst /,\,$(K2_TARGET_FULL_SPEC))"
	@-if exist "$(subst /,\,$(K2_OBJECT_PATH))" rd /s /q "$(subst /,\,$(K2_OBJECT_PATH))"
	@echo --------$(K2_TARGET_FULL_SPEC) Cleaned --------


#========================================================================================
