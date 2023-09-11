/*   
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
*/
#ifndef __SPEC_ELFDEF_INC
#define __SPEC_ELFDEF_INC

//
// --------------------------------------------------------------------------------- 
//

//
// From
//    System V Application Binary Interface - DRAFT - 10 June 2013
//
//    http://www.sco.com/developers/gabi/latest/contents.html
//

//
// file headers
//

#define EI_MAG0                         0
#define EI_MAG1                         1
#define EI_MAG2                         2
#define EI_MAG3                         3
#define EI_CLASS                        4
#define EI_DATA                         5
#define EI_VERSION                      6
#define EI_OSABI                        7
#define EI_ABIVERSION                   8
#define EI_PAD                          9
#define EI_NIDENT                       16

#define ELFMAG0                         0x7F
#define ELFMAG1                         0x45
#define ELFMAG2                         0x4C
#define ELFMAG3                         0x46

#define ELFCLASSNONE                    0
#define ELFCLASS32                      1
#define ELFCLASS64                      2

#define ELFDATANONE                     0
#define ELFDATA2LSB                     1
#define ELFDATA2MSB                     2

#define EV_NONE                         0
#define EV_CURRENT                      1

#define ELFOSABI_SYSV                   0
#define ELFOSABI_HPUX                   1
#define ELFOSABI_GNU                    3
#define ELFOSABI_FREEBSD                9
#define ELFOSABI_STANDALONE             255

#define ET_NONE                         0
#define ET_REL                          1
#define ET_EXEC                         2
#define ET_DYN                          3
#define ET_CORE                         4
#define ET_OSSPEC_LO                    0xFE00
#define ET_OSSPEC_HI                    0xFEFF
#define ET_PROCSPEC_LO                  0xFF00
#define ET_PROCSPEC_HI                  0xFFFF

#define EF_A32_ABIMASK                  0xFF000000
#define EF_A32_ABIVER                   0x05000000

#define EM_NONE                         0
#define EM_X32                          3
#define EM_X64                          62
#define EM_A32                          40
#define EM_A64                          183

#define ELF_LIBREC_LENGTH               60

//
// section headers
//

#define SHN_UNDEF                       0
#define SHN_LORESERVE                   0xFF00
#define SHN_LOPROC                      0xFF00
#define SHN_HIPROC                      0xFF1F
#define SHN_LOOS                        0xFF20
#define SHN_HIOS                        0xFF3F
#define SHN_ABS                         0xFFF1
#define SHN_COMMON                      0xFFF2
#define SHN_XINDEX                      0xFFFF
#define SHN_HIRESERVE                   0xFFFF

#define SHT_NULL                        0
#define SHT_PROGBITS                    1
#define SHT_SYMTAB                      2
#define SHT_STRTAB                      3
#define SHT_RELA                        4
#define SHT_HASH                        5
#define SHT_DYNAMIC                     6
#define SHT_NOTE                        7
#define SHT_NOBITS                      8
#define SHT_REL                         9
#define SHT_SHLIB                       10
#define SHT_DYNSYM                      11
#define SHT_UNK_12                      12
#define SHT_UNK_13                      13
#define SHT_INIT_ARRAY                  14
#define SHT_FINI_ARRAY                  15
#define SHT_PREINIT_ARRAY               16
#define SHT_GROUP                       17
#define SHT_SYMTAB_SHNDX                18
#define SHT_LOOS                        0x60000000
#define SHT_HIOS                        0x6FFFFFFF
#define SHT_LOPROC                      0x70000000
#define SHT_A32_EXIDX                   0x70000001 
#define SHT_A32_PREEMPTMAP              0x70000002 
#define SHT_A32_ATTRIBUTES              0x70000003
#define SHT_A32_DEBUGOVERLAY            0x70000004
#define SHT_A32_OVERLAYSECTION          0x70000005
#define SHT_HIPROC                      0x7FFFFFFF
#define SHT_LOUSER                      0x80000000
#define SHT_HIUSER                      0xFFFFFFFF

#define SHF_WRITE                       0x00000001
#define SHF_ALLOC                       0x00000002
#define SHF_EXECINSTR                   0x00000004
#define SHF_RESERVED8                   0x00000008
#define SHF_MERGE                       0x00000010
#define SHF_STRINGS                     0x00000020
#define SHF_INFO_LINK                   0x00000040
#define SHF_LINK_ORDER                  0x00000080
#define SHF_OS_NONCONFORMING            0x00000100
#define SHF_GROUP                       0x00000200
#define SHF_TLS                         0x00000400
#define SHF_MASKOS                      0x0FF00000
#define SHF_MASKPROC                    0xF0000000

#define ELFCOMPRESS_ZLIB                1
#define ELFCOMPRESS_LOOS                0x60000000
#define ELFCOMPRESS_HIOS                0x6fffffff
#define ELFCOMPRESS_LOPROC              0x70000000
#define ELFCOMPRESS_HIPROC              0x7fffffff

#define STN_UNDEF                       0

//
// symbols 
//

#define STB_LOCAL                       0
#define STB_GLOBAL                      1
#define STB_WEAK                        2
#define STB_LOOS                        10
#define STB_HIOS                        12
#define STB_LOPROC                      13
#define STB_HIPROC                      15

#define STT_NONE                        0
#define STT_OBJECT                      1
#define STT_FUNC                        2
#define STT_SECTION                     3
#define STT_FILE                        4
#define STT_COMMON                      5
#define STT_TLS                         6
#define STT_LOOS                        10
#define STT_HIOS                        12
#define STT_LOPROC                      13
#define STT_HIPROC                      15

#define STV_DEFAULT                     0
#define STV_INTERNAL                    1
#define STV_HIDDEN                      2
#define STV_PROTECTED                   3

//
// program headers
//

#define PT_NULL                         0
#define PT_LOAD                         1
#define PT_DYNAMIC                      2
#define PT_INTERP                       3
#define PT_NOTE                         4
#define PT_SHLIB                        5
#define PT_PHDR                         6
#define PT_TLS                          7
#define PT_LOOS                         0x60000000
#define PT_HIOS                         0x6FFFFFFF
#define PT_LOPROC                       0x70000000
#define PT_HIPROC                       0x7FFFFFFF

#define PF_X                            0x00000001
#define PF_W                            0x00000002
#define PF_R                            0x00000004
#define PF_MASKOS                       0x00FF0000
#define PF_MASKPROC                     0xFF000000

//
// relocations
//

// A This means the addend used to compute the value of the relocatable field
// S This means the value of the symbol whose index resides in the relocation entry
// P This means the place (section offset or address) of the storage unit being relocated (computed using r_offset).
// T is 1 if the target symbol S has type STT_FUNC and the symbol addresses a Thumb instruction; it is 0 otherwise.

#define R_NONE                          0

#define R_386_32                        1         // S + A
#define R_386_PC32                      2         // S + A - P

#define R_ARM_PC24                      1         // ((S + A) | T) – P
#define R_ARM_ABS32                     2         // (S + A) | T
#define R_ARM_CALL                      28        // ((S + A) | T) – P
#define R_ARM_JUMP24                    29        // ((S + A) | T) – P
#define R_ARM_TARGET1                   38        // (S + A) | T or ((S + A) | T) – P
#define R_ARM_V4BX                      40        // ignored
#define R_ARM_PREL31                    42        // ((S + A) | T) – P
#define R_ARM_MOVW_ABS_NC               43        // (S + A) | T
#define R_ARM_MOVT_ABS                  44        // S + A

//
// --------------------------------------------------------------------------------- 
//

#endif // __SPEC_ELFDEF_INC

