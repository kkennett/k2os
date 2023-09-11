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
#ifndef __SPEC_FATDEF_INC
#define __SPEC_FATDEF_INC

//
// --------------------------------------------------------------------------------- 
//

#define FAT_DIRENTRY_NAME0_AVAIL            0x00
#define FAT_DIRENTRY_NAME0_E5               0x05
#define FAT_DIRENTRY_NAME0_DOT              0x2E
#define FAT_DIRENTRY_NAME0_ERASED           0xE5

#define FAT_DIRENTRY_CASE_LOWEXT            0x10
#define FAT_DIRENTRY_CASE_LOWBASE           0x08
#define FAT_DIRENTRY_CASE_NO83NAME          0x04

#define FAT_DIRENTRY_ATTR_READONLY          0x01
#define FAT_DIRENTRY_ATTR_HIDDEN            0x02
#define FAT_DIRENTRY_ATTR_SYSTEM            0x04
#define FAT_DIRENTRY_ATTR_LABEL             0x08
#define FAT_DIRENTRY_ATTR_DIR               0x10
#define FAT_DIRENTRY_ATTR_ARCHIVE           0x20
#define FAT_DIRENTRY_ATTR_LONGEND           0x40
#define FAT_DIRENTRY_ATTR_LONGDEL           0x80

#define FAT_DIRENTRY_IS_LONGNAME(x)         (((x)&0xFF)==0x0F)

#define FAT_CLUSTER12_IS_UNUSED(x)          (((x)&0xFFF)==0)
#define FAT_CLUSTER12_IS_NEXT_PTR(x)        ((((x)&0xFFF)>0) && (((x)&0xFFF)<0xFF0))
#define FAT_CLUSTER12_IS_RESERVED(x)        ((((x)&0xFFF)>=0xFF0) && (((x)&0xFFF)<=0xFF6))
#define FAT_CLUSTER12_IS_BAD(x)             (((x)&0xFFF)==0xFF7)
#define FAT_CLUSTER12_IS_CHAIN_END(x)       ((((x)&0xFFF)>=0xFF8) && (((x)&0xFFF)<=0xFFF))

#define FAT_CLUSTER16_IS_UNUSED(x)          (((x)&0xFFFF)==0)
#define FAT_CLUSTER16_IS_NEXT_PTR(x)        ((((x)&0xFFFF)>0) && (((x)&0xFFFF)<0xFFF0))
#define FAT_CLUSTER16_IS_RESERVED(x)        ((((x)&0xFFFF)>=0xFFF0) && (((x)&0xFFFF)<=0xFFF6))
#define FAT_CLUSTER16_IS_BAD(x)             (((x)&0xFFFF)==0xFFF7)
#define FAT_CLUSTER16_IS_CHAIN_END(x)       ((((x)&0xFFFF)>=0xFFF8) && (((x)&0xFFFF)<=0xFFFF))

#define FAT_CLUSTER32_IS_UNUSED(x)          (((x)&0x0FFFFFFF)==0)
#define FAT_CLUSTER32_IS_NEXT_PTR(x)        ((((x)&0x0FFFFFFF)>0) && (((x)&0x0FFFFFFF)<0x0FFFFFF0))
#define FAT_CLUSTER32_IS_RESERVED(x)        ((((x)&0x0FFFFFFF)>=0x0FFFFFF0) && (((x)&0x0FFFFFFF)<=0x0FFFFFF6))
#define FAT_CLUSTER32_IS_BAD(x)             (((x)&0x0FFFFFFF)==0x0FFFFFF7)
#define FAT_CLUSTER32_IS_CHAIN_END(x)       ((((x)&0x0FFFFFFF)>=0x0FFFFFF8) && (((x)&0x0FFFFFFF)<=0x0FFFFFFF))

#define FAT_CLUSTER_TO_SECTOR(firstDataSector, sectorsPerCluster, cluster)     ((((cluster) - 2) * (sectorsPerCluster)) + (firstDataSector))
#define FAT_SECTOR_TO_CLUSTER(firstDataSector, sectorsPerCluster, sector)      ((((sector) - (firstDataSector)) / (sectorsPerCluster)) + 2)

//
// --------------------------------------------------------------------------------- 
//

#endif // __SPEC_FATDEF_INC

