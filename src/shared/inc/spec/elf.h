//   
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
//
#ifndef __SPEC_ELF_H
#define __SPEC_ELF_H

//
// --------------------------------------------------------------------------------- 
//

#include <k2basetype.h>

#include <spec/elfdef.inc>

#ifdef __cplusplus
extern "C" {
#endif

//
// From
//    System V Application Binary Interface - DRAFT - 10 June 2013
//
//    http://www.sco.com/developers/gabi/latest/contents.html
//

typedef UINT32  Elf32_Addr;
typedef UINT32  Elf32_Off;
typedef UINT16  Elf32_Half;
typedef INT32   Elf32_Sword;
typedef UINT32  Elf32_Word;

typedef UINT64  Elf64_Addr;
typedef UINT64  Elf64_Off;
typedef UINT16  Elf64_Half;
typedef UINT32  Elf64_Word;
typedef INT32   Elf64_Sword;
typedef UINT64  Elf64_Xword;
typedef INT64   Elf64_Sxword;

K2_PACKED_PUSH
struct _Elf32_Ehdr 
{
    UINT8       e_ident[EI_NIDENT];
    Elf32_Half  e_type;
    Elf32_Half  e_machine;
    Elf32_Word  e_version;
    Elf32_Addr  e_entry;
    Elf32_Off   e_phoff;
    Elf32_Off   e_shoff;
    Elf32_Word  e_flags;
    Elf32_Half  e_ehsize;
    Elf32_Half  e_phentsize;
    Elf32_Half  e_phnum;
    Elf32_Half  e_shentsize;
    Elf32_Half  e_shnum;
    Elf32_Half  e_shstrndx;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf32_Ehdr Elf32_Ehdr;

K2_PACKED_PUSH
struct _Elf64_Ehdr
{
    UINT8           e_ident[EI_NIDENT];
    Elf64_Half      e_type;
    Elf64_Half      e_machine;
    Elf64_Word      e_version;
    Elf64_Addr      e_entry;
    Elf64_Off       e_phoff;
    Elf64_Off       e_shoff;
    Elf64_Word      e_flags;
    Elf64_Half      e_ehsize;
    Elf64_Half      e_phentsize;
    Elf64_Half      e_phnum;
    Elf64_Half      e_shentsize;
    Elf64_Half      e_shnum;
    Elf64_Half      e_shstrndx;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf64_Ehdr Elf64_Ehdr;

K2_PACKED_PUSH
struct _Elf32_Shdr 
{
    Elf32_Word    sh_name;
    Elf32_Word    sh_type;
    Elf32_Word    sh_flags;
    Elf32_Addr    sh_addr;
    Elf32_Off     sh_offset;
    Elf32_Word    sh_size;
    Elf32_Word    sh_link;
    Elf32_Word    sh_info;
    Elf32_Word    sh_addralign;
    Elf32_Word    sh_entsize;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf32_Shdr Elf32_Shdr;

K2_PACKED_PUSH
struct _Elf64_Shdr
{
    Elf64_Word  sh_name;
    Elf64_Word  sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr  sh_addr;
    Elf64_Off   sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word  sh_link;
    Elf64_Word  sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf64_Shdr Elf64_Shdr;

K2_PACKED_PUSH
struct _Elf32_Chdr 
{
    Elf32_Word    ch_type;
    Elf32_Word    ch_size;
    Elf32_Word    ch_addralign;
}  K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf32_Chdr Elf32_Chdr;

K2_PACKED_PUSH
struct _Elf32_Sym
{
    Elf32_Word  st_name;
    Elf32_Addr  st_value;
    Elf32_Word  st_size;
    UINT8       st_info;
    UINT8       st_other;
    Elf32_Half  st_shndx;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf32_Sym Elf32_Sym;

K2_PACKED_PUSH
struct _Elf64_Sym
{
    Elf64_Word      st_name;
    UINT8           st_info;
    UINT8           st_other;
    Elf64_Half      st_shndx;
    Elf64_Addr      st_value;
    Elf64_Xword     st_size;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf64_Sym Elf64_Sym;

K2_PACKED_PUSH
struct _Elf32_Rel
{
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf32_Rel Elf32_Rel;

K2_PACKED_PUSH
struct _Elf64_Rel
{
    Elf64_Addr  r_offset;
    Elf64_Xword r_info;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf64_Rel Elf64_Rel;


K2_PACKED_PUSH
struct _Elf32_Phdr
{
    Elf32_Word  p_type;
    Elf32_Off   p_offset;
    Elf32_Addr  p_vaddr;
    Elf32_Addr  p_paddr;
    Elf32_Word  p_filesz;
    Elf32_Word  p_memsz;
    Elf32_Word  p_flags;
    Elf32_Word  p_align;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf32_Phdr Elf32_Phdr;

K2_PACKED_PUSH
struct _Elf64_Phdr
{
    Elf64_Word  p_type;
    Elf64_Word  p_flags;
    Elf64_Off   p_offset;
    Elf64_Addr  p_vaddr;
    Elf64_Addr  p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf64_Phdr Elf64_Phdr;

K2_PACKED_PUSH
struct _Elf_LibRec
{
    char    mName[16];
    char    mTime[12];
    char    mJunk[20];
    char    mFileBytes[10];
    char    mMagic[2];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _Elf_LibRec Elf_LibRec;
K2_STATIC_ASSERT(sizeof(Elf_LibRec) == ELF_LIBREC_LENGTH);

#define ELF32_ST_TYPE(st_info)                  (((UINT8)(st_info)) & 0xF)
#define ELF32_ST_BIND(st_info)                  ((((UINT8)(st_info))>>4) & 0xF)
#define ELF32_ST_INFO(binding,stype)            ((((UINT8)(binding))<<4) | (((UINT8)(stype))&0xF))

#define ELF64_ST_TYPE(st_info)                  (((UINT8)(st_info))&0xF)
#define ELF64_ST_BIND(st_info)                  ((((UINT8)(st_info))>>4)&0xF)
#define ELF64_ST_INFO(binding,stype)            ((((UINT8)(binding))<<4) | (((UINT8)(stype))&0xF))

#define ELF32_ST_VISIBILITY(st_other)           (((UINT8)(st_other)) & 0x3)

#define ELF32_R_SYM(r_info)                     (((Elf32_Word)(r_info))>>8)
#define ELF32_R_TYPE(r_info)                    (((Elf32_Word)(r_info))&0xFF)
#define ELF32_R_INFO(symbol, rtype)             ((((Elf32_Word)(symbol))<<8)|((((Elf32_Word)(rtype))&0xFF)))

#define ELF64_R_SYM(r_info)                     ((UINT32) (((((Elf64_Word)(r_info))>>16)>>16)&0x0FFFFFFFFull))
#define ELF64_R_TYPE(r_info)                    ((UINT32)  (((Elf64_Word)(r_info))&0x0FFFFFFFFull))

#define ELF32_MAKE_RELOC_INFO(symbol, rtype)    ((((Elf32_Word)(symbol))<<8) | ((((Elf32_Word)(rtype))&0xFF)))
#define ELF64_MAKE_RELOC_INFO(symbol, rtype)    (((((Elf64_Xword)(symbol))<<16)<<16) | ((UINT32)(rtype)))

#define ELF32_MAKE_SYMBOL_INFO(binding,stype)   ((((UINT8)(binding))<<4) | (((UINT8)(stype))&0xF))

#ifdef __cplusplus
};  // extern "C"
#endif

/* ------------------------------------------------------------------------- */

#endif  // __SPEC_ELF_H

