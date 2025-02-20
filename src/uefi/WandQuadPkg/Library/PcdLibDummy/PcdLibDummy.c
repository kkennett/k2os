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
/** @file
  A emptry template implementation of PCD Library.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>


/**
  This function provides a means by which SKU support can be established in the PCD infrastructure.

  Sets the current SKU in the PCD database to the value specified by SkuId.  SkuId is returned.

  @param[in]  SkuId The SKU value that will be used when the PCD service will retrieve and
                    set values associated with a PCD token.

  @return Return the SKU ID that just be set.

**/
UINTN
EFIAPI
LibPcdSetSku(
    IN UINTN   SkuId
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetSku(%d)\n", SkuId);
    return SkuId;
}

/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the 8-bit value for the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the 8-bit value for the token specified by TokenNumber.

**/
UINT8
EFIAPI
LibPcdGet8(
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGet8(%d)\n", TokenNumber);
    return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the 16-bit value for the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the 16-bit value for the token specified by TokenNumber.

**/
UINT16
EFIAPI
LibPcdGet16 (
  IN UINTN             TokenNumber
  )
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGet16(%d)\n", TokenNumber);
    return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the 32-bit value for the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the 32-bit value for the token specified by TokenNumber.

**/
UINT32
EFIAPI
LibPcdGet32(
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGet32(%d)\n", TokenNumber);
    return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the 64-bit value for the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the 64-bit value for the token specified by TokenNumber.

**/
UINT64
EFIAPI
LibPcdGet64 (
  IN UINTN             TokenNumber
  )
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGet64(%d)\n", TokenNumber);
  return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the pointer to the buffer of the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the pointer to the token specified by TokenNumber.

**/
VOID *
EFIAPI
LibPcdGetPtr (
  IN UINTN             TokenNumber
  )
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetPtr(%d)\n", TokenNumber);
  return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the Boolean value of the token specified by TokenNumber.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the Boolean value of the token specified by TokenNumber.

**/
BOOLEAN
EFIAPI
LibPcdGetBool(
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetBool(%d)\n", TokenNumber);
    return FALSE;
}



/**
  This function provides a means by which to retrieve the size of a given PCD token.

  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Returns the size of the token specified by TokenNumber.

**/
UINTN
EFIAPI
LibPcdGetSize (
  IN UINTN             TokenNumber
  )
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetSize(%d)\n", TokenNumber);
    return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the 8-bit value for the token specified by TokenNumber and Guid.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid The pointer to a 128-bit unique value that designates
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the UINT8.

**/
UINT8
EFIAPI
LibPcdGetEx8(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetEx8(%g,%d)\n", Guid, TokenNumber);
    return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the 16-bit value for the token specified by TokenNumber and Guid.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid The pointer to a 128-bit unique value that designates
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the UINT16.

**/
UINT16
EFIAPI
LibPcdGetEx16(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetEx16(%g,%d)\n", Guid, TokenNumber);
    return 0;
}



/**
  Returns the 32-bit value for the token specified by TokenNumber and Guid.
  If Guid is NULL, then ASSERT().

  @param[in]  Guid The pointer to a 128-bit unique value that designates
              which namespace to retrieve a value from.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.

  @return Return the UINT32.

**/
UINT32
EFIAPI
LibPcdGetEx32(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetEx32(%g,%d)\n", Guid, TokenNumber);
    return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the 64-bit value for the token specified by TokenNumber and Guid.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that designates
                            which namespace to retrieve a value from.
  @param[in]  TokenNumber   The PCD token number to retrieve a current value for.

  @return Return the UINT64.

**/
UINT64
EFIAPI
LibPcdGetEx64(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetEx64(%g,%d)\n", Guid, TokenNumber);
    return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the pointer to the buffer of token specified by TokenNumber and Guid.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that designates
                            which namespace to retrieve a value from.
  @param[in]  TokenNumber   The PCD token number to retrieve a current value for.

  @return Return the VOID* pointer.

**/
VOID *
EFIAPI
LibPcdGetExPtr(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetExPtr(%g,%d)\n", Guid, TokenNumber);
    return 0;
}



/**
  This function provides a means by which to retrieve a value for a given PCD token.

  Returns the Boolean value of the token specified by TokenNumber and Guid.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that designates
                            which namespace to retrieve a value from.
  @param[in]  TokenNumber   The PCD token number to retrieve a current value for.

  @return Return the BOOLEAN.

**/
BOOLEAN
EFIAPI
LibPcdGetExBool(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetExBool(%g,%d)\n", Guid, TokenNumber);
    return FALSE;
}



/**
  This function provides a means by which to retrieve the size of a given PCD token.

  Returns the size of the token specified by TokenNumber and Guid.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that designates
                            which namespace to retrieve a value from.
  @param[in]  TokenNumber   The PCD token number to retrieve a current value for.

  @return Return the size.

**/
UINTN
EFIAPI
LibPcdGetExSize(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetExSize(%g,%d)\n", Guid, TokenNumber);
    return 0;
}



#ifndef DISABLE_NEW_DEPRECATED_INTERFACES
/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 8-bit value for the token specified by TokenNumber
  to the value specified by Value.  Value is returned.

  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The 8-bit value to set.

  @return Return the value that was set.

**/
UINT8
EFIAPI
LibPcdSet8(
    IN UINTN             TokenNumber,
    IN UINT8             Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSet8(%d,%d)\n", TokenNumber, Value);
    return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 16-bit value for the token specified by TokenNumber
  to the value specified by Value.  Value is returned.

  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The 16-bit value to set.

  @return Return the value that was set.

**/
UINT16
EFIAPI
LibPcdSet16(
    IN UINTN             TokenNumber,
    IN UINT16            Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSet16(%d,%d)\n", TokenNumber, Value);
    return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 32-bit value for the token specified by TokenNumber
  to the value specified by Value.  Value is returned.

  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The 32-bit value to set.

  @return Return the value that was set.

**/
UINT32
EFIAPI
LibPcdSet32(
    IN UINTN             TokenNumber,
    IN UINT32            Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSet32(%d,%d)\n", TokenNumber, Value);
    return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 64-bit value for the token specified by TokenNumber
  to the value specified by Value.  Value is returned.

  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The 64-bit value to set.

  @return Return the value that was set.

**/
UINT64
EFIAPI
LibPcdSet64(
    IN UINTN             TokenNumber,
    IN UINT64            Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSet64(%d,%d)\n", TokenNumber, Value);
    return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets a buffer for the token specified by TokenNumber to the value
  specified by Buffer and SizeOfBuffer.  Buffer is returned.
  If SizeOfBuffer is greater than the maximum size support by TokenNumber,
  then set SizeOfBuffer to the maximum size supported by TokenNumber and
  return NULL to indicate that the set operation was not actually performed.

  If SizeOfBuffer is set to MAX_ADDRESS, then SizeOfBuffer must be set to the
  maximum size supported by TokenName and NULL must be returned.

  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[in]      TokenNumber   The PCD token number to set a current value for.
  @param[in, out] SizeOfBuffer  The size, in bytes, of Buffer.
  @param[in]      Buffer        A pointer to the buffer to set.

  @return Return the pointer for the buffer been set.

**/
VOID *
EFIAPI
LibPcdSetPtr (
  IN        UINTN             TokenNumber,
  IN OUT    UINTN             *SizeOfBuffer,
  IN CONST  VOID              *Buffer
  )
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetPtr(%d,%d,%p)\n", TokenNumber, *SizeOfBuffer, Buffer);
    *SizeOfBuffer = 0;
    return (VOID *)Buffer;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the Boolean value for the token specified by TokenNumber
  to the value specified by Value.  Value is returned.

  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The boolean value to set.

  @return Return the value that was set.

**/
BOOLEAN
EFIAPI
LibPcdSetBool(
    IN UINTN             TokenNumber,
    IN BOOLEAN           Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetBool(%d,%d)\n", TokenNumber, Value);
    return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 8-bit value for the token specified by TokenNumber and
  Guid to the value specified by Value. Value is returned.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The 8-bit value to set.

  @return Return the value that was set.

**/
UINT8
EFIAPI
LibPcdSetEx8 (
  IN CONST GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
{
  DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx8(%g,%d,%d)\n", Guid, TokenNumber, Value);
  return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 16-bit value for the token specified by TokenNumber and
  Guid to the value specified by Value. Value is returned.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The 16-bit value to set.

  @return Return the value that was set.

**/
UINT16
EFIAPI
LibPcdSetEx16(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber,
    IN UINT16            Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx16(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 32-bit value for the token specified by TokenNumber and
  Guid to the value specified by Value. Value is returned.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The 32-bit value to set.

  @return Return the value that was set.

**/
UINT32
EFIAPI
LibPcdSetEx32(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber,
    IN UINT32            Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx32(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 64-bit value for the token specified by TokenNumber and
  Guid to the value specified by Value. Value is returned.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The 64-bit value to set.

  @return Return the value that was set.

**/
UINT64
EFIAPI
LibPcdSetEx64(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber,
    IN UINT64            Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx64(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return Value;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets a buffer for the token specified by TokenNumber to the value specified by
  Buffer and SizeOfBuffer.  Buffer is returned.  If SizeOfBuffer is greater than
  the maximum size support by TokenNumber, then set SizeOfBuffer to the maximum size
  supported by TokenNumber and return NULL to indicate that the set operation
  was not actually performed.

  If Guid is NULL, then ASSERT().
  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[in]  Guid              The pointer to a 128-bit unique value that
                                designates which namespace to set a value from.
  @param[in]  TokenNumber       The PCD token number to set a current value for.
  @param[in, out] SizeOfBuffer  The size, in bytes, of Buffer.
  @param[in]  Buffer            A pointer to the buffer to set.

  @return Return the pinter to the buffer been set.

**/
VOID *
EFIAPI
LibPcdSetExPtr(
    IN      CONST GUID        *Guid,
    IN      UINTN             TokenNumber,
    IN OUT  UINTN             *SizeOfBuffer,
    IN      VOID              *Buffer
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx8(%g,%d,%d,%p)\n", Guid, TokenNumber, *SizeOfBuffer, Buffer);
    return Buffer;
}



/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the Boolean value for the token specified by TokenNumber and
  Guid to the value specified by Value. Value is returned.

  If Guid is NULL, then ASSERT().

  @param[in]  Guid          The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in]  TokenNumber   The PCD token number to set a current value for.
  @param[in]  Value         The Boolean value to set.

  @return Return the value that was set.

**/
BOOLEAN
EFIAPI
LibPcdSetExBool(
    IN CONST GUID        *Guid,
    IN UINTN             TokenNumber,
    IN BOOLEAN           Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetExBool(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return Value;
}
#endif

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 8-bit value for the token specified by TokenNumber
  to the value specified by Value.

  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The 8-bit value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSet8S(
    IN UINTN          TokenNumber,
    IN UINT8          Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSet8S(%d,%d)\n", TokenNumber, Value);
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 16-bit value for the token specified by TokenNumber
  to the value specified by Value.

  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The 16-bit value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSet16S(
    IN UINTN          TokenNumber,
    IN UINT16         Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSet16S(%d,%d)\n", TokenNumber, Value);
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 32-bit value for the token specified by TokenNumber
  to the value specified by Value.

  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The 32-bit value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSet32S(
    IN UINTN          TokenNumber,
    IN UINT32         Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSet32S(%d,%d)\n", TokenNumber, Value);
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 64-bit value for the token specified by TokenNumber
  to the value specified by Value.

  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The 64-bit value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSet64S(
    IN UINTN          TokenNumber,
    IN UINT64         Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSet64S(%d,%d)\n", TokenNumber, Value);
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets a buffer for the token specified by TokenNumber to the value specified
  by Buffer and SizeOfBuffer. If SizeOfBuffer is greater than the maximum size
  support by TokenNumber, then set SizeOfBuffer to the maximum size supported by
  TokenNumber and return EFI_INVALID_PARAMETER to indicate that the set operation
  was not actually performed.

  If SizeOfBuffer is set to MAX_ADDRESS, then SizeOfBuffer must be set to the
  maximum size supported by TokenName and EFI_INVALID_PARAMETER must be returned.

  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[in]      TokenNumber   The PCD token number to set a current value for.
  @param[in, out] SizeOfBuffer  The size, in bytes, of Buffer.
  @param[in]      Buffer        A pointer to the buffer to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSetPtrS(
    IN       UINTN    TokenNumber,
    IN OUT   UINTN    *SizeOfBuffer,
    IN CONST VOID     *Buffer
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetPtrS(%d,%d,%p)\n", TokenNumber, *SizeOfBuffer, Buffer);
    *SizeOfBuffer = 0;
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the boolean value for the token specified by TokenNumber
  to the value specified by Value.

  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The boolean value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSetBoolS(
    IN UINTN          TokenNumber,
    IN BOOLEAN        Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetBoolS(%d,%d)\n", TokenNumber, Value);
    return Value;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 8-bit value for the token specified by TokenNumber
  to the value specified by Value.

  If Guid is NULL, then ASSERT().

  @param[in] Guid           The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The 8-bit value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSetEx8S(
    IN CONST GUID     *Guid,
    IN UINTN          TokenNumber,
    IN UINT8          Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx8S(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 16-bit value for the token specified by TokenNumber
  to the value specified by Value.

  If Guid is NULL, then ASSERT().

  @param[in] Guid           The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The 16-bit value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSetEx16S(
    IN CONST GUID     *Guid,
    IN UINTN          TokenNumber,
    IN UINT16         Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx8S(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 32-bit value for the token specified by TokenNumber
  to the value specified by Value.

  If Guid is NULL, then ASSERT().

  @param[in] Guid           The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The 32-bit value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSetEx32S (
  IN CONST GUID     *Guid,
  IN UINTN          TokenNumber,
  IN UINT32         Value
  )
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx32S(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the 64-bit value for the token specified by TokenNumber
  to the value specified by Value.

  If Guid is NULL, then ASSERT().

  @param[in] Guid           The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The 64-bit value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSetEx64S (
  IN CONST GUID     *Guid,
  IN UINTN          TokenNumber,
  IN UINT64         Value
  )
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx64S(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets a buffer for the token specified by TokenNumber to the value specified by
  Buffer and SizeOfBuffer. If SizeOfBuffer is greater than the maximum size
  support by TokenNumber, then set SizeOfBuffer to the maximum size supported by
  TokenNumber and return EFI_INVALID_PARAMETER to indicate that the set operation
  was not actually performed.

  If Guid is NULL, then ASSERT().
  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[in]      Guid          Pointer to a 128-bit unique value that
                                designates which namespace to set a value from.
  @param[in]      TokenNumber   The PCD token number to set a current value for.
  @param[in, out] SizeOfBuffer  The size, in bytes, of Buffer.
  @param[in]      Buffer        A pointer to the buffer to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSetExPtrS (
  IN CONST GUID     *Guid,
  IN       UINTN    TokenNumber,
  IN OUT   UINTN    *SizeOfBuffer,
  IN       VOID     *Buffer
  )
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetEx32S(%g,%d,%d,%p)\n", Guid, TokenNumber, *SizeOfBuffer, Buffer);
    *SizeOfBuffer = 0;
    return 0;
}

/**
  This function provides a means by which to set a value for a given PCD token.

  Sets the boolean value for the token specified by TokenNumber
  to the value specified by Value.

  If Guid is NULL, then ASSERT().

  @param[in] Guid           The pointer to a 128-bit unique value that
                            designates which namespace to set a value from.
  @param[in] TokenNumber    The PCD token number to set a current value for.
  @param[in] Value          The boolean value to set.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPcdSetExBoolS(
    IN CONST GUID     *Guid,
    IN UINTN          TokenNumber,
    IN BOOLEAN        Value
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdSetExBoolS(%g,%d,%d)\n", Guid, TokenNumber, Value);
    return 0;
}

/**
  Set up a notification function that is called when a specified token is set.

  When the token specified by TokenNumber and Guid is set,
  then notification function specified by NotificationFunction is called.
  If Guid is NULL, then the default token space is used.

  If NotificationFunction is NULL, then ASSERT().

  @param[in]  Guid      The pointer to a 128-bit unique value that designates which
                        namespace to set a value from.  If NULL, then the default
                        token space is used.
  @param[in]  TokenNumber   The PCD token number to monitor.
  @param[in]  NotificationFunction  The function to call when the token
                                    specified by Guid and TokenNumber is set.

**/
VOID
EFIAPI
LibPcdCallbackOnSet(
    IN CONST GUID               *Guid, OPTIONAL
    IN UINTN                    TokenNumber,
    IN PCD_CALLBACK             NotificationFunction
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdCallbackOnSet(%g,%d,%p)\n", Guid, TokenNumber, NotificationFunction);
}



/**
  Disable a notification function that was established with LibPcdCallbackonSet().

  Disable a notification function that was previously established with LibPcdCallbackOnSet().

  If NotificationFunction is NULL, then ASSERT().
  If LibPcdCallbackOnSet() was not previously called with Guid, TokenNumber,
  and NotificationFunction, then ASSERT().

  @param[in]  Guid          Specify the GUID token space.
  @param[in]  TokenNumber   Specify the token number.
  @param[in]  NotificationFunction The callback function to be unregistered.

**/
VOID
EFIAPI
LibPcdCancelCallback(
    IN CONST GUID               *Guid, OPTIONAL
    IN UINTN                    TokenNumber,
    IN PCD_CALLBACK             NotificationFunction
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdCancelCallback(%g,%d,%p)\n", Guid, TokenNumber, NotificationFunction);
}



/**
  Retrieves the next token in a token space.

  Retrieves the next PCD token number from the token space specified by Guid.
  If Guid is NULL, then the default token space is used.  If TokenNumber is 0,
  then the first token number is returned.  Otherwise, the token number that
  follows TokenNumber in the token space is returned.  If TokenNumber is the last
  token number in the token space, then 0 is returned.

  If TokenNumber is not 0 and is not in the token space specified by Guid, then ASSERT().

  @param[in]  Guid        The pointer to a 128-bit unique value that designates which namespace
                          to set a value from.  If NULL, then the default token space is used.
  @param[in]  TokenNumber The previous PCD token number.  If 0, then retrieves the first PCD
                          token number.

  @return The next valid token number.

**/
UINTN
EFIAPI
LibPcdGetNextToken(
    IN CONST GUID               *Guid, OPTIONAL
    IN UINTN                    TokenNumber
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetNextToken(%g,%d)\n", Guid, TokenNumber);
    return 0;
}



/**
  Used to retrieve the list of available PCD token space GUIDs.

  Returns the PCD token space GUID that follows TokenSpaceGuid in the list of token spaces
  in the platform.
  If TokenSpaceGuid is NULL, then a pointer to the first PCD token spaces returned.
  If TokenSpaceGuid is the last PCD token space GUID in the list, then NULL is returned.

  @param  TokenSpaceGuid  The pointer to a PCD token space GUID.

  @return The next valid token namespace.

**/
GUID *
EFIAPI
LibPcdGetNextTokenSpace(
    IN CONST GUID  *TokenSpaceGuid
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetNextTokenSpace(%g)\n", TokenSpaceGuid);
    return NULL;
}


/**
  Sets a value of a patchable PCD entry that is type pointer.

  Sets the PCD entry specified by PatchVariable to the value specified by Buffer
  and SizeOfBuffer.  Buffer is returned.  If SizeOfBuffer is greater than
  MaximumDatumSize, then set SizeOfBuffer to MaximumDatumSize and return
  NULL to indicate that the set operation was not actually performed.
  If SizeOfBuffer is set to MAX_ADDRESS, then SizeOfBuffer must be set to
  MaximumDatumSize and NULL must be returned.

  If PatchVariable is NULL, then ASSERT().
  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[out] PatchVariable     A pointer to the global variable in a module that is
                                the target of the set operation.
  @param[in] MaximumDatumSize   The maximum size allowed for the PCD entry specified by PatchVariable.
  @param[in, out] SizeOfBuffer  A pointer to the size, in bytes, of Buffer.
  @param[in] Buffer             A pointer to the buffer to used to set the target variable.

  @return Return the pointer to the buffer that was set.

**/
VOID *
EFIAPI
LibPatchPcdSetPtr(
    OUT       VOID        *PatchVariable,
    IN        UINTN       MaximumDatumSize,
    IN OUT    UINTN       *SizeOfBuffer,
    IN CONST  VOID        *Buffer
)
{
    ASSERT(PatchVariable != NULL);
    ASSERT(SizeOfBuffer != NULL);

    if (*SizeOfBuffer > 0) {
        ASSERT(Buffer != NULL);
    }

    if ((*SizeOfBuffer > MaximumDatumSize) ||
        (*SizeOfBuffer == MAX_ADDRESS)) {
        *SizeOfBuffer = MaximumDatumSize;
        return NULL;
    }

    DebugPrint(0xFFFFFFFF, "LibPatchPcdSetPtr(%p,%d,%d,%p)\n", PatchVariable, MaximumDatumSize, *SizeOfBuffer, Buffer);

    return (VOID *)Buffer;
}

/**
  Sets a value of a patchable PCD entry that is type pointer.

  Sets the PCD entry specified by PatchVariable to the value specified
  by Buffer and SizeOfBuffer. If SizeOfBuffer is greater than MaximumDatumSize,
  then set SizeOfBuffer to MaximumDatumSize and return RETURN_INVALID_PARAMETER
  to indicate that the set operation was not actually performed.
  If SizeOfBuffer is set to MAX_ADDRESS, then SizeOfBuffer must be set to
  MaximumDatumSize and RETURN_INVALID_PARAMETER must be returned.

  If PatchVariable is NULL, then ASSERT().
  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[out] PatchVariable     A pointer to the global variable in a module that is
                                the target of the set operation.
  @param[in] MaximumDatumSize   The maximum size allowed for the PCD entry specified by PatchVariable.
  @param[in, out] SizeOfBuffer  A pointer to the size, in bytes, of Buffer.
  @param[in] Buffer             A pointer to the buffer to used to set the target variable.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPatchPcdSetPtrS(
    OUT      VOID     *PatchVariable,
    IN       UINTN    MaximumDatumSize,
    IN OUT   UINTN    *SizeOfBuffer,
    IN CONST VOID     *Buffer
)
{
    ASSERT(PatchVariable != NULL);
    ASSERT(SizeOfBuffer != NULL);

    if (*SizeOfBuffer > 0) {
        ASSERT(Buffer != NULL);
    }

    if ((*SizeOfBuffer > MaximumDatumSize) ||
        (*SizeOfBuffer == MAX_ADDRESS)) {
        *SizeOfBuffer = MaximumDatumSize;
        return RETURN_INVALID_PARAMETER;
    }

    DebugPrint(0xFFFFFFFF, "LibPatchPcdSetPtrS(%p,%d,%d,%p)\n", PatchVariable, MaximumDatumSize, *SizeOfBuffer, Buffer);

    return RETURN_SUCCESS;
}

/**
  Sets a value and size of a patchable PCD entry that is type pointer.

  Sets the PCD entry specified by PatchVariable to the value specified by Buffer
  and SizeOfBuffer.  Buffer is returned.  If SizeOfBuffer is greater than
  MaximumDatumSize, then set SizeOfBuffer to MaximumDatumSize and return
  NULL to indicate that the set operation was not actually performed.
  If SizeOfBuffer is set to MAX_ADDRESS, then SizeOfBuffer must be set to
  MaximumDatumSize and NULL must be returned.

  If PatchVariable is NULL, then ASSERT().
  If SizeOfPatchVariable is NULL, then ASSERT().
  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[out] PatchVariable     A pointer to the global variable in a module that is
                                the target of the set operation.
  @param[out] SizeOfPatchVariable A pointer to the size, in bytes, of PatchVariable.
  @param[in] MaximumDatumSize   The maximum size allowed for the PCD entry specified by PatchVariable.
  @param[in, out] SizeOfBuffer  A pointer to the size, in bytes, of Buffer.
  @param[in] Buffer             A pointer to the buffer to used to set the target variable.

  @return Return the pointer to the buffer been set.

**/
VOID *
EFIAPI
LibPatchPcdSetPtrAndSize(
    OUT       VOID        *PatchVariable,
    OUT       UINTN       *SizeOfPatchVariable,
    IN        UINTN       MaximumDatumSize,
    IN OUT    UINTN       *SizeOfBuffer,
    IN CONST  VOID        *Buffer
)
{
    ASSERT(PatchVariable != NULL);
    ASSERT(SizeOfPatchVariable != NULL);
    ASSERT(SizeOfBuffer != NULL);

    if (*SizeOfBuffer > 0) {
        ASSERT(Buffer != NULL);
    }

    if ((*SizeOfBuffer > MaximumDatumSize) ||
        (*SizeOfBuffer == MAX_ADDRESS)) {
        *SizeOfBuffer = MaximumDatumSize;
        return NULL;
    }

    DebugPrint(0xFFFFFFFF, "LibPatchPcdSetPtrAndSize(%p,%p,%d,%d,%p)\n", PatchVariable, SizeOfPatchVariable, MaximumDatumSize, *SizeOfBuffer, Buffer);

    return (VOID *)Buffer;
}

/**
  Sets a value and size of a patchable PCD entry that is type pointer.

  Sets the PCD entry specified by PatchVariable to the value specified
  by Buffer and SizeOfBuffer. If SizeOfBuffer is greater than MaximumDatumSize,
  then set SizeOfBuffer to MaximumDatumSize and return RETURN_INVALID_PARAMETER
  to indicate that the set operation was not actually performed.
  If SizeOfBuffer is set to MAX_ADDRESS, then SizeOfBuffer must be set to
  MaximumDatumSize and RETURN_INVALID_PARAMETER must be returned.

  If PatchVariable is NULL, then ASSERT().
  If SizeOfPatchVariable is NULL, then ASSERT().
  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().

  @param[out] PatchVariable     A pointer to the global variable in a module that is
                                the target of the set operation.
  @param[out] SizeOfPatchVariable A pointer to the size, in bytes, of PatchVariable.
  @param[in] MaximumDatumSize   The maximum size allowed for the PCD entry specified by PatchVariable.
  @param[in, out] SizeOfBuffer  A pointer to the size, in bytes, of Buffer.
  @param[in] Buffer             A pointer to the buffer to used to set the target variable.

  @return The status of the set operation.

**/
RETURN_STATUS
EFIAPI
LibPatchPcdSetPtrAndSizeS(
    OUT      VOID     *PatchVariable,
    OUT      UINTN    *SizeOfPatchVariable,
    IN       UINTN    MaximumDatumSize,
    IN OUT   UINTN    *SizeOfBuffer,
    IN CONST VOID     *Buffer
)
{
    ASSERT(PatchVariable != NULL);
    ASSERT(SizeOfPatchVariable != NULL);
    ASSERT(SizeOfBuffer != NULL);

    if (*SizeOfBuffer > 0) {
        ASSERT(Buffer != NULL);
    }

    if ((*SizeOfBuffer > MaximumDatumSize) ||
        (*SizeOfBuffer == MAX_ADDRESS)) {
        *SizeOfBuffer = MaximumDatumSize;
        return RETURN_INVALID_PARAMETER;
    }

    DebugPrint(0xFFFFFFFF, "LibPatchPcdSetPtrAndSizeS(%p,%p,%d,%d,%p)\n", PatchVariable, SizeOfPatchVariable, MaximumDatumSize, *SizeOfBuffer, Buffer);

    return RETURN_SUCCESS;
}

/**
  Retrieve additional information associated with a PCD token.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  If TokenNumber is not in the default token space specified, then ASSERT().

  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName.
**/
VOID
EFIAPI
LibPcdGetInfo(
    IN        UINTN           TokenNumber,
    OUT       PCD_INFO        *PcdInfo
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetInfo(%d,%p)\n", TokenNumber,PcdInfo);
}

/**
  Retrieve additional information associated with a PCD token.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  If TokenNumber is not in the token space specified by Guid, then ASSERT().

  @param[in]    Guid        The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName.
**/
VOID
EFIAPI
LibPcdGetInfoEx(
    IN CONST  GUID            *Guid,
    IN        UINTN           TokenNumber,
    OUT       PCD_INFO        *PcdInfo
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetInfoEx(%d,%p)\n", TokenNumber, PcdInfo);
}

/**
  Retrieve the currently set SKU Id.

  @return   The currently set SKU Id. If the platform has not set at a SKU Id, then the
            default SKU Id value of 0 is returned. If the platform has set a SKU Id, then the currently set SKU
            Id is returned.
**/
UINTN
EFIAPI
LibPcdGetSku(
    VOID
)
{
    DebugPrint(0xFFFFFFFF, "DummyPcdLib::LibPcdGetSku()\n");
    return 0;
}

