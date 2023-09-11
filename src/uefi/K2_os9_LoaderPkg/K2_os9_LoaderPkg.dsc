#   
#   BSD 3-Clause License
#   
#   Copyright (c) 2023, Kurt Kennett
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

[Defines]
    PLATFORM_NAME                  = K2_os9_LoaderPkg
    PLATFORM_GUID                  = 9F88B13B-D0D6-498C-8A6C-A7EBFEFD64C9
    PLATFORM_VERSION               = 1.0
    DSC_SPECIFICATION              = 0x00010006
    OUTPUT_DIRECTORY               = Build/K2_os9_Loader
    SUPPORTED_ARCHITECTURES        = IA32|ARM
    BUILD_TARGETS                  = DEBUG|RELEASE
    SKUID_IDENTIFIER               = DEFAULT

[LibraryClasses.common]
    BaseMemoryLib               |MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
    UefiApplicationEntryPoint   |MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
    UefiBootServicesTableLib    |MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    UefiRuntimeServicesTableLib |MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
    MemoryAllocationLib         |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    PrintLib                    |MdePkg/Library/BasePrintLib/BasePrintLib.inf
    UefiLib                     |MdePkg/Library/UefiLib/UefiLib.inf
    BaseLib                     |MdePkg/Library/BaseLib/BaseLib.inf
    PcdLib                      |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
    DevicePathLib               |MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf

[LibraryClasses.ARM]
    #
    # It is not possible to prevent the ARM compiler for generic intrinsic functions.
    # This library provides the instrinsic functions generate by a given compiler.
    # [LibraryClasses.ARM] and NULL mean link this library into all ARM images.
    #
    NULL    |ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

    #
    # GCC uses the stack checking library
    #
    NULL    |MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

    #
    # For cache operations
    #
    ArmLib  |ArmPkg/Library/ArmLib/ArmBaseLib.inf

    #
    # For getting CPU information HOB
    #
    HobLib  |MdePkg/Library/DxeHobLib/DxeHobLib.inf

[Components]
    K2_os9_LoaderPkg/K2OsLoader/K2OsLoader.inf

