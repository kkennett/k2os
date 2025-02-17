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

#include <lib/k2fatfs.h>
#include <lib/k2asc.h>
#include <lib/k2mem.h>

#define FF_FSIFLAG_DIRTY        0x01
#define FF_FSIFLAG_DISABLED     0x80

/* Limits and boundaries */
#define MAX_DIR                 0x200000        /* Max size of FAT directory */
#define MAX_FAT12               0xFF5            /* Max FAT12 clusters (differs from specs, but right for real DOS/Windows behavior) */
#define MAX_FAT16               0xFFF5            /* Max FAT16 clusters (differs from specs, but right for real DOS/Windows behavior) */
#define MAX_FAT32               0x0FFFFFF5        /* Max FAT32 clusters (not specified, practical limit) */

/* Character code support macros */
#define IsUpper(c)              ((c) >= 'A' && (c) <= 'Z')
#define IsLower(c)              ((c) >= 'a' && (c) <= 'z')
#define IsDigit(c)              ((c) >= '0' && (c) <= '9')
#define IsSeparator(c)          ((c) == '/' || (c) == '\\')
#define IsTerminator(c)         ((UINT32)(c) < ' ')
#define IsSurrogate(c)          ((c) >= 0xD800 && (c) <= 0xDFFF)
#define IsSurrogateH(c)         ((c) >= 0xD800 && (c) <= 0xDBFF)
#define IsSurrogateL(c)         ((c) >= 0xDC00 && (c) <= 0xDFFF)

/* Additional file access control and file status flags for internal use */
#define FA_SEEKEND              0x20    /* Seek to end of the file on file open */
#define FA_MODIFIED             0x40    /* File has been modified */
#define FA_DIRTY                0x80    /* K2FATFS_OBJ_FILE.buf[] needs to be written-back */

/* Additional file attribute bits for internal use */
#define AM_VOL                  0x08    /* Volume label */
#define AM_LFN                  0x0F    /* LFN entry */
#define AM_MASK                 0x3F    /* Mask of defined bits in FAT */

/* Name status flags in mSfn[11] */
#define NSFLAG                  11      /* Index of the name status byte */
#define NS_LOSS                 0x01    /* Out of 8.3 format */
#define NS_LFN                  0x02    /* Force to create LFN entry */
#define NS_LAST                 0x04    /* Last segment */
#define NS_BODY                 0x08    /* Lower case flag (body) */
#define NS_EXT                  0x10    /* Lower case flag (ext) */
#define NS_DOT                  0x20    /* Dot entry */
#define NS_NOLFN                0x40    /* Do not find LFN */
#define NS_NONAME               0x80    /* Not followed */

/* FatFs refers the FAT structure as simple byte array instead of structure member
/ because the C structure is not binary compatible between different platforms */

#define BS_JmpBoot              0       /* x86 jump instruction (3-byte) */
#define BS_OEMName              3       /* OEM name (8-byte) */
#define BPB_BytsPerSec          11      /* Sector size [byte] (UINT16) */
#define BPB_SecPerClus          13      /* Cluster size [sector] (UINT8) */
#define BPB_RsvdSecCnt          14      /* Size of reserved area [sector] (UINT16) */
#define BPB_NumFATs             16      /* Number of FATs (UINT8) */
#define BPB_RootEntCnt          17      /* Size of root directory area for FAT [entry] (UINT16) */
#define BPB_TotSec16            19      /* Volume size (16-bit) [sector] (UINT16) */
#define BPB_Media               21      /* Media descriptor byte (UINT8) */
#define BPB_FATSz16             22      /* FAT size (16-bit) [sector] (UINT16) */
#define BPB_SecPerTrk           24      /* Number of sectors per track for int13h [sector] (UINT16) */
#define BPB_NumHeads            26      /* Number of heads for int13h (UINT16) */
#define BPB_HiddSec             28      /* Volume offset from top of the drive (UINT32) */
#define BPB_TotSec32            32      /* Volume size (32-bit) [sector] (UINT32) */
#define BS_DrvNum               36      /* Physical drive number for int13h (UINT8) */
#define BS_NTres                37      /* WindowsNT error flag (UINT8) */
#define BS_BootSig              38      /* Extended boot signature (UINT8) */
#define BS_VolID                39      /* Volume serial number (UINT32) */
#define BS_VolLab               43      /* Volume label string (8-byte) */
#define BS_FilSysType           54      /* Filesystem type string (8-byte) */
#define BS_BootCode             62      /* Boot code (448-byte) */
#define BS_55AA                 510     /* Signature word (UINT16) */

#define BPB_FATSz32             36      /* FAT32: FAT size [sector] (UINT32) */
#define BPB_ExtFlags32          40      /* FAT32: Extended flags (UINT16) */
#define BPB_FSVer32             42      /* FAT32: Filesystem version (UINT16) */
#define BPB_RootClus32          44      /* FAT32: Root directory cluster (UINT32) */
#define BPB_FSInfo32            48      /* FAT32: Offset of FSINFO sector (UINT16) */
#define BPB_BkBootSec32         50      /* FAT32: Offset of backup boot sector (UINT16) */
#define BS_DrvNum32             64      /* FAT32: Physical drive number for int13h (UINT8) */
#define BS_NTres32              65      /* FAT32: Error flag (UINT8) */
#define BS_BootSig32            66      /* FAT32: Extended boot signature (UINT8) */
#define BS_VolID32              67      /* FAT32: Volume serial number (UINT32) */
#define BS_VolLab32             71      /* FAT32: Volume label string (8-byte) */
#define BS_FilSysType32         82      /* FAT32: Filesystem type string (8-byte) */
#define BS_BootCode32           90      /* FAT32: Boot code (420-byte) */

#define DIR_Name                0       /* Short file name (11-byte) */
#define DIR_Attr                11      /* Attribute (UINT8) */
#define DIR_NTres               12      /* Lower case flag (UINT8) */
#define DIR_CrtTime10           13      /* Created time sub-second (UINT8) */
#define DIR_CrtTime             14      /* Created time (UINT32) */
#define DIR_LstAccDate          18      /* Last accessed date (UINT16) */
#define DIR_FstClusHI           20      /* Higher 16-bit of first cluster (UINT16) */
#define DIR_ModTime             22      /* Modified time (UINT32) */
#define DIR_FstClusLO           26      /* Lower 16-bit of first cluster (UINT16) */
#define DIR_FileSize            28      /* File size (UINT32) */
#define LDIR_Ord                0       /* LFN: LFN order and LLE flag (UINT8) */
#define LDIR_Attr               11      /* LFN: LFN attribute (UINT8) */
#define LDIR_Type               12      /* LFN: Entry type (UINT8) */
#define LDIR_Chksum             13      /* LFN: Checksum of the SFN (UINT8) */
#define LDIR_FstClusLO          26      /* LFN: MBZ field (UINT16) */

#define SZDIRE                  32      /* Size of a directory entry */
#define DDEM                    0xE5    /* Deleted directory entry mark set to DIR_Name[0] */
#define RDDEM                   0x05    /* Replacement of the character collides with DDEM */
#define LLEF                    0x40    /* Last long entry flag in LDIR_Ord */

#define FSI_LeadSig             0       /* FAT32 FSI: Leading signature (UINT32) */
#define FSI_StrucSig            484     /* FAT32 FSI: Structure signature (UINT32) */
#define FSI_Free_Count          488     /* FAT32 FSI: Number of free clusters (UINT32) */
#define FSI_Nxt_Free            492     /* FAT32 FSI: Last allocated cluster (UINT32) */

#define MAX_MALLOC              0x8000    /* Must be >=K2FATFS_SECTOR_BYTES */

#define FF_CLUSTER_EMPTY        0
#define FF_CLUSTER_INT_ERROR    1
#define FF_CLUSTER_FIRST_VALID  2
#define FF_CLUSTER_DISK_ERROR   0xFFFFFFFF

#define FIND_RECURS             4    /* Maximum number of wildcard terms in the pattern to limit recursion */

#define FF_CHECKFS_FAT_VBR              0
#define FF_CHECKFS_EXFAT_VBR            1
#define FF_CHECKFS_NOT_FAT_VALID_BS     2
#define FF_CHECKFS_NOT_FAT_INVALID      3
#define FF_CHECKFS_DISK_ERROR           4

static const UINT16 uc437[] = {    /*  CP437(U.S.) to Unicode conversion table */
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

static const UINT8 LfnOfs[] = { 1,3,5,7,9,14,16,18,20,22,24,28,30 };    /* FAT: Offset of LFN characters in the directory entry */

static const UINT8 ExCvt[] =
    { 0x80,0x9A,0x45,0x41,0x8E,0x41,0x8F,0x80,0x45,0x45,0x45,0x49,0x49,0x49,0x8E,0x8F, 
      0x90,0x92,0x92,0x4F,0x99,0x4F,0x55,0x55,0x59,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, 
      0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF, 
      0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, 
      0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF, 
      0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, 
      0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF, 
      0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF };

UINT16 sK2FATFS_UniCharToOem(UINT32 aUniChar)
{
    UINT16 c = 0;
    const UINT16* p = uc437;

    if (aUniChar < 0x80)
    {
        c = (UINT16)aUniChar;
    }
    else
    {
        if (aUniChar < 0x10000)
        {
            for (c = 0; c < 0x80 && aUniChar != p[c]; c++)
            {
            }
            c = (c + 0x80) & 0xFF;
        }
    }

    return c;
}

UINT16 sK2FATFS_OemCharToUni(UINT16 aOemChar)
{
    UINT16 c = 0;
    const UINT16* p = uc437;

    if (aOemChar < 0x80)
    {
        c = aOemChar;
    }
    else
    {
        if (aOemChar < 0x100)
            c = p[aOemChar - 0x80];
    }

    return c;
}

UINT32 sK2FATFS_UniCharUpCase(UINT32 aUniChar)
{
    const UINT16* p;
    UINT16 uc, bc, nc, cmd;
    static const UINT16 cvt1[] = {    /* Compressed up conversion table for U+0000 - U+0FFF */
        /* Basic Latin */
        0x0061,0x031A,
        /* Latin-1 Supplement */
        0x00E0,0x0317,
        0x00F8,0x0307,
        0x00FF,0x0001,0x0178,
        /* Latin Extended-A */
        0x0100,0x0130,
        0x0132,0x0106,
        0x0139,0x0110,
        0x014A,0x012E,
        0x0179,0x0106,
        /* Latin Extended-B */
        0x0180,0x004D,0x0243,0x0181,0x0182,0x0182,0x0184,0x0184,0x0186,0x0187,0x0187,0x0189,0x018A,0x018B,0x018B,0x018D,0x018E,0x018F,0x0190,0x0191,0x0191,0x0193,0x0194,0x01F6,0x0196,0x0197,0x0198,0x0198,0x023D,0x019B,0x019C,0x019D,0x0220,0x019F,0x01A0,0x01A0,0x01A2,0x01A2,0x01A4,0x01A4,0x01A6,0x01A7,0x01A7,0x01A9,0x01AA,0x01AB,0x01AC,0x01AC,0x01AE,0x01AF,0x01AF,0x01B1,0x01B2,0x01B3,0x01B3,0x01B5,0x01B5,0x01B7,0x01B8,0x01B8,0x01BA,0x01BB,0x01BC,0x01BC,0x01BE,0x01F7,0x01C0,0x01C1,0x01C2,0x01C3,0x01C4,0x01C5,0x01C4,0x01C7,0x01C8,0x01C7,0x01CA,0x01CB,0x01CA,
        0x01CD,0x0110,
        0x01DD,0x0001,0x018E,
        0x01DE,0x0112,
        0x01F3,0x0003,0x01F1,0x01F4,0x01F4,
        0x01F8,0x0128,
        0x0222,0x0112,
        0x023A,0x0009,0x2C65,0x023B,0x023B,0x023D,0x2C66,0x023F,0x0240,0x0241,0x0241,
        0x0246,0x010A,
        /* IPA Extensions */
        0x0253,0x0040,0x0181,0x0186,0x0255,0x0189,0x018A,0x0258,0x018F,0x025A,0x0190,0x025C,0x025D,0x025E,0x025F,0x0193,0x0261,0x0262,0x0194,0x0264,0x0265,0x0266,0x0267,0x0197,0x0196,0x026A,0x2C62,0x026C,0x026D,0x026E,0x019C,0x0270,0x0271,0x019D,0x0273,0x0274,0x019F,0x0276,0x0277,0x0278,0x0279,0x027A,0x027B,0x027C,0x2C64,0x027E,0x027F,0x01A6,0x0281,0x0282,0x01A9,0x0284,0x0285,0x0286,0x0287,0x01AE,0x0244,0x01B1,0x01B2,0x0245,0x028D,0x028E,0x028F,0x0290,0x0291,0x01B7,
        /* Greek, Coptic */
        0x037B,0x0003,0x03FD,0x03FE,0x03FF,
        0x03AC,0x0004,0x0386,0x0388,0x0389,0x038A,
        0x03B1,0x0311,
        0x03C2,0x0002,0x03A3,0x03A3,
        0x03C4,0x0308,
        0x03CC,0x0003,0x038C,0x038E,0x038F,
        0x03D8,0x0118,
        0x03F2,0x000A,0x03F9,0x03F3,0x03F4,0x03F5,0x03F6,0x03F7,0x03F7,0x03F9,0x03FA,0x03FA,
        /* Cyrillic */
        0x0430,0x0320,
        0x0450,0x0710,
        0x0460,0x0122,
        0x048A,0x0136,
        0x04C1,0x010E,
        0x04CF,0x0001,0x04C0,
        0x04D0,0x0144,
        /* Armenian */
        0x0561,0x0426,

        0x0000    /* EOT */
    };
    static const UINT16 cvt2[] = {    /* Compressed up conversion table for U+1000 - U+FFFF */
        /* Phonetic Extensions */
        0x1D7D,0x0001,0x2C63,
        /* Latin Extended Additional */
        0x1E00,0x0196,
        0x1EA0,0x015A,
        /* Greek Extended */
        0x1F00,0x0608,
        0x1F10,0x0606,
        0x1F20,0x0608,
        0x1F30,0x0608,
        0x1F40,0x0606,
        0x1F51,0x0007,0x1F59,0x1F52,0x1F5B,0x1F54,0x1F5D,0x1F56,0x1F5F,
        0x1F60,0x0608,
        0x1F70,0x000E,0x1FBA,0x1FBB,0x1FC8,0x1FC9,0x1FCA,0x1FCB,0x1FDA,0x1FDB,0x1FF8,0x1FF9,0x1FEA,0x1FEB,0x1FFA,0x1FFB,
        0x1F80,0x0608,
        0x1F90,0x0608,
        0x1FA0,0x0608,
        0x1FB0,0x0004,0x1FB8,0x1FB9,0x1FB2,0x1FBC,
        0x1FCC,0x0001,0x1FC3,
        0x1FD0,0x0602,
        0x1FE0,0x0602,
        0x1FE5,0x0001,0x1FEC,
        0x1FF3,0x0001,0x1FFC,
        /* Letterlike Symbols */
        0x214E,0x0001,0x2132,
        /* Number forms */
        0x2170,0x0210,
        0x2184,0x0001,0x2183,
        /* Enclosed Alphanumerics */
        0x24D0,0x051A,
        0x2C30,0x042F,
        /* Latin Extended-C */
        0x2C60,0x0102,
        0x2C67,0x0106, 0x2C75,0x0102,
        /* Coptic */
        0x2C80,0x0164,
        /* Georgian Supplement */
        0x2D00,0x0826,
        /* Full-width */
        0xFF41,0x031A,

        0x0000    /* EOT */
    };

    if (aUniChar < 0x10000)
    {    /* Is it in BMP? */
        uc = (UINT16)aUniChar;
        p = uc < 0x1000 ? cvt1 : cvt2;
        for (;;)
        {
            bc = *p++;                                /* Get the block base */
            if (bc == 0 || uc < bc) 
                break;            /* Not matched? */
            nc = *p++; cmd = nc >> 8; nc &= 0xFF;    /* Get processing command and block size */
            if (uc < bc + nc)
            {    /* In the block? */
                switch (cmd)
                {
                    case 0: uc = p[uc - bc]; break;        /* Table conversion */
                    case 1: uc -= (uc - bc) & 1; break;    /* Case pairs */
                    case 2: uc -= 16; break;            /* Shift -16 */
                    case 3: uc -= 32; break;            /* Shift -32 */
                    case 4: uc -= 48; break;            /* Shift -48 */
                    case 5: uc -= 26; break;            /* Shift -26 */
                    case 6: uc += 8; break;                /* Shift +8 */
                    case 7: uc -= 80; break;            /* Shift -80 */
                    case 8: uc -= 0x1C60; break;        /* Shift -0x1C60 */
                }
                break;
            }
            if (cmd == 0) 
                p += nc;    /* Skip table if needed */
        }
        aUniChar = uc;
    }

    return aUniChar;
}

static UINT16 sK2FATFS_LoadFromUnalignedLe16(const UINT8* apByte)    /*     Load a 2-byte little-endian word */
{
    UINT16 result;

    result = apByte[1];
    result = result << 8 | apByte[0];
    return result;
}

static UINT32 sK2FATFS_LoadFromUnalignedLe32(const UINT8* apByte)    /* Load a 4-byte little-endian word */
{
    UINT32 result;

    result = apByte[3];
    result = result << 8 | apByte[2];
    result = result << 8 | apByte[1];
    result = result << 8 | apByte[0];
    return result;
}

static void sK2FATFS_StoreToUnalignedLe16(UINT8* apByte, UINT16 aVal16)    /* Store a 2-byte word in little-endian */
{
    *apByte++ = (UINT8)aVal16; aVal16 >>= 8;
    *apByte++ = (UINT8)aVal16;
}

static void sK2FATFS_StoreToUnalignedLe32(UINT8* apByte, UINT32 aVal32)    /* Store a 4-byte word in little-endian */
{
    *apByte++ = (UINT8)aVal32; aVal32 >>= 8;
    *apByte++ = (UINT8)aVal32; aVal32 >>= 8;
    *apByte++ = (UINT8)aVal32; aVal32 >>= 8;
    *apByte++ = (UINT8)aVal32;
}

static UINT32 sK2FATFS_EatCharToUnicode(const char** appStr)
{
    UINT32 uc;
    const char *p = *appStr;

    UINT16 wc;

    wc = (UINT8)*p++;            /* Get a byte */
    if (wc != 0)
    {
        wc = sK2FATFS_OemCharToUni(wc);    /* ANSI/OEM ==> Unicode */
        if (wc == 0) 
            return 0xFFFFFFFF;    /* Invalid code? */
    }
    uc = wc;

    *appStr = p;    /* Next read pointer */
    return uc;
}

static UINT32 sK2FATFS_StorUniChar(UINT32 aUniChar, char* apOutBuf, UINT32 aOutBufLeft)
{
    UINT16 wc;

    // returns # of bytes written to apOutBuf

    wc = sK2FATFS_UniCharToOem(aUniChar);
    if (wc >= 0x100)
    {    /* Is this a DBC? */
        if (aOutBufLeft < 2) 
            return 0;
        *apOutBuf++ = (char)(wc >> 8);    /* Store DBC 1st byte */
        *apOutBuf++ = (char)wc;            /* Store DBC 2nd byte */
        return 2;
    }
    if (wc == 0 || aOutBufLeft < 1) 
        return 0;    /* Invalid char or buffer overflow? */
    *apOutBuf++ = (char)wc;                    /* Store the character */
    return 1;
}

static K2STAT sK2FATFS_LockVolume(K2FATFS *apFatFs)
{
    return apFatFs->mpOps->MutexTake(apFatFs->mpOps, apFatFs->mMutex);
}

static void sK2FATFS_Locked_UnlockVolume(K2FATFS *apFatFs)
{
    apFatFs->mpOps->MutexGive(apFatFs->mpOps, apFatFs->mMutex);
}

static K2STAT sK2FATFS_Locked_SyncWindow(K2FATFS* apFatFs)
{
    K2STAT k2stat = K2STAT_NO_ERROR;

    if (apFatFs->mWindowIsDirtyFlag)
    {    /* Is the disk access window dirty? */
        if (apFatFs->mpOps->DiskWrite(apFatFs->mpOps, apFatFs->mpAlignedWindow, apFatFs->mCurWinSector, 1) == K2STAT_NO_ERROR)
        {    /* Write it back into the volume */
            apFatFs->mWindowIsDirtyFlag = 0;    /* Clear window dirty flag */
            if (apFatFs->mCurWinSector - apFatFs->mFatBaseSector < apFatFs->mSectorsPerFat)
            {    /* Is it in the 1st FAT? */
                if (apFatFs->mFatCount == 2) 
                    apFatFs->mpOps->DiskWrite(apFatFs->mpOps, apFatFs->mpAlignedWindow, apFatFs->mCurWinSector + apFatFs->mSectorsPerFat, 1);    /* Reflect it to 2nd FAT if needed */
            }
        }
        else
        {
            k2stat = K2STAT_ERROR_HARDWARE;
        }
    }
    return k2stat;
}

static K2STAT sK2FATFS_Locked_MoveWindow(K2FATFS* apFatFs, UINT32 aSectorNumber)
{
    K2STAT k2stat = K2STAT_NO_ERROR;

    if (aSectorNumber != apFatFs->mCurWinSector)
    {    /* Window offset changed? */
        k2stat = sK2FATFS_Locked_SyncWindow(apFatFs);        /* Flush the window */
        if (k2stat == K2STAT_NO_ERROR)
        {            /* Fill sector window with new data */
            if (apFatFs->mpOps->DiskRead(apFatFs->mpOps, apFatFs->mpAlignedWindow, aSectorNumber, 1) != K2STAT_NO_ERROR)
            {
                aSectorNumber = (UINT32)0 - 1;    /* Invalidate window if read data is not valid */
                k2stat = K2STAT_ERROR_HARDWARE;
            }
            apFatFs->mCurWinSector = aSectorNumber;
        }
    }
    return k2stat;
}

static K2STAT sK2FATFS_Locked_Sync(K2FATFS* apFatFs)
{
    K2STAT k2stat;

    k2stat = sK2FATFS_Locked_SyncWindow(apFatFs);
    if (k2stat == K2STAT_NO_ERROR)
    {
        if (apFatFs->mFatType == K2FATFS_FATTYPE_FAT32 && apFatFs->mFsiFlags == FF_FSIFLAG_DIRTY)
        {
            /* FAT32: Update FSInfo sector if needed */
            /* Create FSInfo structure */
            K2MEM_Zero(apFatFs->mpAlignedWindow, K2FATFS_SECTOR_BYTES);
            sK2FATFS_StoreToUnalignedLe16(apFatFs->mpAlignedWindow + BS_55AA, 0xAA55);                    /* Boot signature */
            sK2FATFS_StoreToUnalignedLe32(apFatFs->mpAlignedWindow + FSI_LeadSig, 0x41615252);        /* Leading signature */
            sK2FATFS_StoreToUnalignedLe32(apFatFs->mpAlignedWindow + FSI_StrucSig, 0x61417272);        /* Structure signature */
            sK2FATFS_StoreToUnalignedLe32(apFatFs->mpAlignedWindow + FSI_Free_Count, apFatFs->mFreeClusterCount);    /* Number of free clusters */
            sK2FATFS_StoreToUnalignedLe32(apFatFs->mpAlignedWindow + FSI_Nxt_Free, apFatFs->mLastAllocCluster);    /* Last allocated culuster */
            apFatFs->mCurWinSector = 0;                        /* Write it into the FSInfo sector (Next to VBR) */
            apFatFs->mpOps->DiskWrite(apFatFs->mpOps, apFatFs->mpAlignedWindow, apFatFs->mCurWinSector, 1);
            apFatFs->mFsiFlags = 0;
        }
        /* Make sure that no pending write process in the lower layer */
        if (K2STAT_NO_ERROR != apFatFs->mpOps->DiskSync(apFatFs->mpOps))
            k2stat = K2STAT_ERROR_HARDWARE;
    }

    return k2stat;
}

static UINT32 sK2FATFS_LockDC_ClusterToSector(K2FATFS* apFatFs, UINT32 aClusterNumber)
{
    aClusterNumber -= 2;        /* Cluster number is origin from 2 */
    if (aClusterNumber >= (apFatFs->mFatEntryCount - 2))
    {
        return 0;        /* Is it invalid cluster number? */
    }
    return apFatFs->mDataBaseSector + ((UINT32)apFatFs->mSectorsPerCluster * aClusterNumber);
}

static UINT32 sK2FATFS_Locked_GetObjCluster(K2FATFS_OBJ_HDR* obj, UINT32 aClusterNumber)
{
    UINT32 wc, bc;
    UINT32 val;
    K2FATFS *pFatFs = obj->mpParentFs;

    if ((aClusterNumber < 2) || 
        (aClusterNumber >= pFatFs->mFatEntryCount))
    {
        val = FF_CLUSTER_INT_ERROR;
    }
    else
    {
        val = FF_CLUSTER_DISK_ERROR;

        switch (pFatFs->mFatType)
        {
        case K2FATFS_FATTYPE_FAT12:
            bc = (UINT32)aClusterNumber; bc += bc / 2;
            if (sK2FATFS_Locked_MoveWindow(pFatFs, pFatFs->mFatBaseSector + (bc / K2FATFS_SECTOR_BYTES)) != K2STAT_NO_ERROR) 
                break;
            wc = pFatFs->mpAlignedWindow[bc++ % K2FATFS_SECTOR_BYTES];        /* Get 1st byte of the entry */
            if (sK2FATFS_Locked_MoveWindow(pFatFs, pFatFs->mFatBaseSector + (bc / K2FATFS_SECTOR_BYTES)) != K2STAT_NO_ERROR) 
                break;
            wc |= pFatFs->mpAlignedWindow[bc % K2FATFS_SECTOR_BYTES] << 8;    /* Merge 2nd byte of the entry */
            val = (aClusterNumber & 1) ? (wc >> 4) : (wc & 0xFFF);    /* Adjust bit position */
            break;

        case K2FATFS_FATTYPE_FAT16:
            if (sK2FATFS_Locked_MoveWindow(pFatFs, pFatFs->mFatBaseSector + (aClusterNumber / (K2FATFS_SECTOR_BYTES / 2))) != K2STAT_NO_ERROR) 
                break;
            val = sK2FATFS_LoadFromUnalignedLe16(pFatFs->mpAlignedWindow + aClusterNumber * 2 % K2FATFS_SECTOR_BYTES);        /* Simple UINT16 array */
            break;

        case K2FATFS_FATTYPE_FAT32:
            if (sK2FATFS_Locked_MoveWindow(pFatFs, pFatFs->mFatBaseSector + (aClusterNumber / (K2FATFS_SECTOR_BYTES / 4))) != K2STAT_NO_ERROR) 
                break;
            val = sK2FATFS_LoadFromUnalignedLe32(pFatFs->mpAlignedWindow + aClusterNumber * 4 % K2FATFS_SECTOR_BYTES) & 0x0FFFFFFF;    /* Simple UINT32 array but mask out upper 4 bits */
            break;
        default:
            val = FF_CLUSTER_INT_ERROR;    /* Internal error */
        }
    }

    return val;
}

static K2STAT sK2FATFS_Locked_SetFatCluster(K2FATFS* apFatFs, UINT32 aClusterNumber, UINT32 aNewValue)
{
    UINT32 bc;
    UINT8 *p;
    K2STAT k2stat = K2STAT_ERROR_UNKNOWN;

    if (aClusterNumber >= 2 && aClusterNumber < apFatFs->mFatEntryCount)
    {    /* Check if in valid range */
        switch (apFatFs->mFatType)
        {
        case K2FATFS_FATTYPE_FAT12:
            bc = (UINT32)aClusterNumber; bc += bc / 2;    /* bc: byte offset of the entry */
            k2stat = sK2FATFS_Locked_MoveWindow(apFatFs, apFatFs->mFatBaseSector + (bc / K2FATFS_SECTOR_BYTES));
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            p = apFatFs->mpAlignedWindow + bc++ % K2FATFS_SECTOR_BYTES;
            *p = (aClusterNumber & 1) ? ((*p & 0x0F) | ((UINT8)aNewValue << 4)) : (UINT8)aNewValue;    /* Update 1st byte */
            apFatFs->mWindowIsDirtyFlag = 1;
            k2stat = sK2FATFS_Locked_MoveWindow(apFatFs, apFatFs->mFatBaseSector + (bc / K2FATFS_SECTOR_BYTES));
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            p = apFatFs->mpAlignedWindow + bc % K2FATFS_SECTOR_BYTES;
            *p = (aClusterNumber & 1) ? (UINT8)(aNewValue >> 4) : ((*p & 0xF0) | ((UINT8)(aNewValue >> 8) & 0x0F));    /* Update 2nd byte */
            apFatFs->mWindowIsDirtyFlag = 1;
            break;

        case K2FATFS_FATTYPE_FAT16:
            k2stat = sK2FATFS_Locked_MoveWindow(apFatFs, apFatFs->mFatBaseSector + (aClusterNumber / (K2FATFS_SECTOR_BYTES / 2)));
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            sK2FATFS_StoreToUnalignedLe16(apFatFs->mpAlignedWindow + aClusterNumber * 2 % K2FATFS_SECTOR_BYTES, (UINT16)aNewValue);    /* Simple UINT16 array */
            apFatFs->mWindowIsDirtyFlag = 1;
            break;

        case K2FATFS_FATTYPE_FAT32:
            k2stat = sK2FATFS_Locked_MoveWindow(apFatFs, apFatFs->mFatBaseSector + (aClusterNumber / (K2FATFS_SECTOR_BYTES / 4)));
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            aNewValue = (aNewValue & 0x0FFFFFFF) | (sK2FATFS_LoadFromUnalignedLe32(apFatFs->mpAlignedWindow + aClusterNumber * 4 % K2FATFS_SECTOR_BYTES) & 0xF0000000);
            sK2FATFS_StoreToUnalignedLe32(apFatFs->mpAlignedWindow + aClusterNumber * 4 % K2FATFS_SECTOR_BYTES, aNewValue);
            apFatFs->mWindowIsDirtyFlag = 1;
            break;
        }
    }
    return k2stat;
}

static K2STAT sK2FATFS_Locked_RemoveObjChain(K2FATFS_OBJ_HDR* apObj, UINT32 aClusterNumber, UINT32 aPrevClusterNumber)
{
    K2STAT k2stat = K2STAT_NO_ERROR;
    UINT32 nxt;
    K2FATFS *pFatFs = apObj->mpParentFs;

    if (aClusterNumber < 2 || aClusterNumber >= pFatFs->mFatEntryCount) 
        return K2STAT_ERROR_UNKNOWN;    /* Check if in valid range */

    /* Mark the previous cluster 'EOC' on the FAT if it exists */
    if (aPrevClusterNumber != 0)
    {
        k2stat = sK2FATFS_Locked_SetFatCluster(pFatFs, aPrevClusterNumber, 0xFFFFFFFF);
        if (k2stat != K2STAT_NO_ERROR) 
            return k2stat;
    }

    /* Remove the chain */
    do
    {
        nxt = sK2FATFS_Locked_GetObjCluster(apObj, aClusterNumber);            /* Get cluster status */
        if (nxt == FF_CLUSTER_EMPTY) 
            break;                /* Empty cluster? */
        if (nxt == FF_CLUSTER_INT_ERROR) 
            return K2STAT_ERROR_UNKNOWN;    /* Internal error? */
        if (nxt == FF_CLUSTER_DISK_ERROR) 
            return K2STAT_ERROR_HARDWARE;    /* Disk error? */
        k2stat = sK2FATFS_Locked_SetFatCluster(pFatFs, aClusterNumber, 0);        /* Mark the cluster 'free' on the FAT */
        if (k2stat != K2STAT_NO_ERROR) 
            return k2stat;
        if (pFatFs->mFreeClusterCount < pFatFs->mFatEntryCount - 2)
        {    /* Update FSINFO */
            pFatFs->mFreeClusterCount++;
            pFatFs->mFsiFlags |= FF_FSIFLAG_DIRTY;
        }
        aClusterNumber = nxt;                    /* Next cluster */
    } while (aClusterNumber < pFatFs->mFatEntryCount);    /* Repeat while not the last link */

    return K2STAT_NO_ERROR;
}

static UINT32 sK2FATFS_Locked_AddClusterToObjChain(K2FATFS_OBJ_HDR* apObj, UINT32 aClusterNumber)
{
    UINT32 cs, ncl, scl;
    K2STAT k2stat;
    K2FATFS *pFatFs = apObj->mpParentFs;

    if (aClusterNumber == 0)
    {    /* Create a new chain */
        scl = pFatFs->mLastAllocCluster;                /* Suggested cluster to start to find */
        if (scl == 0 || scl >= pFatFs->mFatEntryCount) 
            scl = 1;
    }
    else
    {                /* Stretch a chain */
        cs = sK2FATFS_Locked_GetObjCluster(apObj, aClusterNumber);            /* Check the cluster status */
        if (cs < FF_CLUSTER_FIRST_VALID) 
            return FF_CLUSTER_INT_ERROR;                /* Test for insanity */
        if (cs == FF_CLUSTER_DISK_ERROR) 
            return cs;    /* Test for disk error */
        if (cs < pFatFs->mFatEntryCount) 
            return cs;    /* It is already followed by next cluster */
        scl = aClusterNumber;                            /* Cluster to start to find */
    }
    if (pFatFs->mFreeClusterCount == 0) 
        return 0;        /* No free cluster */

    {    /* On the FAT/FAT32 volume */
        ncl = 0;
        if (scl == aClusterNumber)
        {                        /* Stretching an existing chain? */
            ncl = scl + 1;                        /* Test if next cluster is free */
            if (ncl >= pFatFs->mFatEntryCount) 
                ncl = 2;
            cs = sK2FATFS_Locked_GetObjCluster(apObj, ncl);                /* Get next cluster status */
            if (cs == FF_CLUSTER_INT_ERROR || cs == FF_CLUSTER_DISK_ERROR) 
                return cs;    /* Test for error */
            if (cs != FF_CLUSTER_EMPTY)
            {                        /* Not free? */
                cs = pFatFs->mLastAllocCluster;                /* Start at suggested cluster if it is valid */
                if (cs >= 2 && cs < pFatFs->mFatEntryCount) 
                    scl = cs;
                ncl = 0;
            }
        }
        if (ncl == 0)
        {    /* The new cluster cannot be contiguous and find another fragment */
            ncl = scl;    /* Start cluster */
            for (;;)
            {
                ncl++;                            /* Next cluster */
                if (ncl >= pFatFs->mFatEntryCount)
                {        /* Check wrap-around */
                    ncl = 2;
                    if (ncl > scl) 
                        return 0;    /* No free cluster found? */
                }
                cs = sK2FATFS_Locked_GetObjCluster(apObj, ncl);            /* Get the cluster status */
                if (cs == FF_CLUSTER_EMPTY) 
                    break;                /* Found a free cluster? */
                if (cs == FF_CLUSTER_INT_ERROR || cs == FF_CLUSTER_DISK_ERROR) 
                    return cs;    /* Test for error */
                if (ncl == scl) 
                    return 0;        /* No free cluster found? */
            }
        }
        k2stat = sK2FATFS_Locked_SetFatCluster(pFatFs, ncl, 0xFFFFFFFF);        /* Mark the new cluster 'EOC' */
        if (k2stat == K2STAT_NO_ERROR && aClusterNumber != 0)
        {
            k2stat = sK2FATFS_Locked_SetFatCluster(pFatFs, aClusterNumber, ncl);        /* Link it from the previous one if needed */
        }
    }

    if (k2stat == K2STAT_NO_ERROR)
    {            /* Update FSINFO if function succeeded. */
        pFatFs->mLastAllocCluster = ncl;
        if (pFatFs->mFreeClusterCount <= pFatFs->mFatEntryCount - 2) 
            pFatFs->mFreeClusterCount--;
        pFatFs->mFsiFlags |= FF_FSIFLAG_DIRTY;
    }
    else
    {
        ncl = (k2stat == K2STAT_ERROR_HARDWARE) ? 0xFFFFFFFF : 1;    /* Failed. Generate error status */
    }

    return ncl;        /* Return new cluster number or error status */
}

static K2STAT sK2FATFS_Locked_ClearDir(K2FATFS *apFatFs, UINT32 aDirCluster)
{
    UINT32 sect;
    UINT32 n, szb;
    UINT8 *ibuf;

    if (sK2FATFS_Locked_SyncWindow(apFatFs) != K2STAT_NO_ERROR) 
        return K2STAT_ERROR_HARDWARE;    /* Flush disk access window */
    sect = sK2FATFS_LockDC_ClusterToSector(apFatFs, aDirCluster);        /* Top of the cluster */
    apFatFs->mCurWinSector = sect;                /* Set window to top of the cluster */
    K2MEM_Zero(apFatFs->mpAlignedWindow, K2FATFS_SECTOR_BYTES);    /* Clear window buffer */

    /* Allocate a temporary buffer */
    for (szb = ((UINT32)apFatFs->mSectorsPerCluster * K2FATFS_SECTOR_BYTES >= MAX_MALLOC) ? MAX_MALLOC : apFatFs->mSectorsPerCluster * K2FATFS_SECTOR_BYTES,  ibuf = 0; 
        szb > K2FATFS_SECTOR_BYTES && (ibuf = apFatFs->mpOps->MemAlloc(apFatFs->mpOps, szb)) == 0; 
        szb /= 2);
    if (szb > K2FATFS_SECTOR_BYTES)
    {        /* Buffer allocated? */
        K2MEM_Zero(ibuf, szb);
        szb /= K2FATFS_SECTOR_BYTES;        /* Bytes -> Sectors */
        for (n = 0; 
            n < apFatFs->mSectorsPerCluster && apFatFs->mpOps->DiskWrite(apFatFs->mpOps, ibuf, sect + n, szb) == K2STAT_NO_ERROR; 
            n += szb);    /* Fill the cluster with 0 */
        apFatFs->mpOps->MemFree(apFatFs->mpOps, ibuf);
    }
    else
    {
        ibuf = apFatFs->mpAlignedWindow; 
        szb = 1;    /* Use window buffer (many single-sector writes may take a time) */
        for (n = 0; 
            n < apFatFs->mSectorsPerCluster && apFatFs->mpOps->DiskWrite(apFatFs->mpOps, ibuf, sect + n, szb) == K2STAT_NO_ERROR; 
            n += szb);    /* Fill the cluster with 0 */
    }
    return (n == apFatFs->mSectorsPerCluster) ? K2STAT_NO_ERROR : K2STAT_ERROR_HARDWARE;
}

static K2STAT sK2FATFS_Locked_SetDirIndex(K2FATFS_OBJ_DIR* apDirObj, UINT32 aOffset)
{
    UINT32 csz, clst;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;

    if (aOffset >= (UINT32)(MAX_DIR) || aOffset % SZDIRE)
    {    /* Check range of offset and alignment */
        return K2STAT_ERROR_UNKNOWN;
    }
    apDirObj->mPointer = aOffset;                /* Set current offset */
    clst = apDirObj->Hdr.mStartCluster;        /* Table start cluster (0:root) */
    if (clst == 0 && pFatFs->mFatType >= K2FATFS_FATTYPE_FAT32)
    {    /* Replace cluster# 0 with root cluster# */
        clst = (UINT32)pFatFs->mRootDirBase;
    }

    if (clst == 0)
    {    /* Static table (root-directory on the FAT volume) */
        if (aOffset / SZDIRE >= pFatFs->mRootDirEntCount) 
            return K2STAT_ERROR_UNKNOWN;    /* Is index out of range? */
        apDirObj->mCurrentSector = pFatFs->mRootDirBase;

    }
    else
    {            /* Dynamic table (sub-directory or root-directory on the FAT32/exFAT volume) */
        csz = (UINT32)pFatFs->mSectorsPerCluster * K2FATFS_SECTOR_BYTES;    /* Bytes per cluster */
        while (aOffset >= csz)
        {                /* Follow cluster chain */
            clst = sK2FATFS_Locked_GetObjCluster(&apDirObj->Hdr, clst);                /* Get next cluster */
            if (clst == FF_CLUSTER_DISK_ERROR) 
                return K2STAT_ERROR_HARDWARE;    /* Disk error */
            if (clst < FF_CLUSTER_FIRST_VALID || clst >= pFatFs->mFatEntryCount) 
                return K2STAT_ERROR_UNKNOWN;    /* Reached to end of table or internal error */
            aOffset -= csz;
        }
        apDirObj->mCurrentSector = sK2FATFS_LockDC_ClusterToSector(pFatFs, clst);
    }
    apDirObj->mCurrentCluster = clst;                    /* Current cluster# */
    if (apDirObj->mCurrentSector == 0) 
        return K2STAT_ERROR_UNKNOWN;
    apDirObj->mCurrentSector += aOffset / K2FATFS_SECTOR_BYTES;            /* Sector# of the directory entry */
    apDirObj->mpRawData = pFatFs->mpAlignedWindow + (aOffset % K2FATFS_SECTOR_BYTES);    /* Pointer to the entry in the win[] */

    return K2STAT_NO_ERROR;
}

static K2STAT sK2FATFS_Locked_DirMoveNext(K2FATFS_OBJ_DIR* apDirObj, int aDoStretch)
{
    UINT32 ofs, clst;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;

    ofs = apDirObj->mPointer + SZDIRE;    /* Next entry */
    if (ofs >= (UINT32)(MAX_DIR)) 
        apDirObj->mCurrentSector = 0;    /* Disable it if the offset reached the max value */
    if (apDirObj->mCurrentSector == 0) 
        return K2STAT_ERROR_NOT_FOUND;    /* Report EOT if it has been disabled */

    if (ofs % K2FATFS_SECTOR_BYTES == 0)
    {    /* Sector changed? */
        apDirObj->mCurrentSector++;                /* Next sector */

        if (apDirObj->mCurrentCluster == 0)
        {    /* Static table */
            if (ofs / SZDIRE >= pFatFs->mRootDirEntCount)
            {    /* Report EOT if it reached end of static table */
                apDirObj->mCurrentSector = 0; 
                return K2STAT_ERROR_NOT_FOUND;
            }
        }
        else
        {                    /* Dynamic table */
            if ((ofs / K2FATFS_SECTOR_BYTES & (pFatFs->mSectorsPerCluster - 1)) == 0)
            {    /* Cluster changed? */
                clst = sK2FATFS_Locked_GetObjCluster(&apDirObj->Hdr, apDirObj->mCurrentCluster);        /* Get next cluster */
                if (clst <= FF_CLUSTER_INT_ERROR) 
                    return K2STAT_ERROR_UNKNOWN;            /* Internal error */
                if (clst == FF_CLUSTER_DISK_ERROR) 
                    return K2STAT_ERROR_HARDWARE;    /* Disk error */
                if (clst >= pFatFs->mFatEntryCount)
                {                    /* It reached end of dynamic table */
                    if (!aDoStretch)
                    {                                /* If no aDoStretch, report EOT */
                        apDirObj->mCurrentSector = 0; 
                        return K2STAT_ERROR_NOT_FOUND;
                    }
                    clst = sK2FATFS_Locked_AddClusterToObjChain(&apDirObj->Hdr, apDirObj->mCurrentCluster);    /* Allocate a cluster */
                    if (clst == FF_CLUSTER_EMPTY) 
                        return K2STAT_ERROR_NOT_ALLOWED;            /* No free cluster */
                    if (clst == FF_CLUSTER_INT_ERROR) 
                        return K2STAT_ERROR_UNKNOWN;            /* Internal error */
                    if (clst == FF_CLUSTER_DISK_ERROR) 
                        return K2STAT_ERROR_HARDWARE;    /* Disk error */
                    if (sK2FATFS_Locked_ClearDir(pFatFs, clst) != K2STAT_NO_ERROR) 
                        return K2STAT_ERROR_HARDWARE;    /* Clean up the stretched table */
                }
                apDirObj->mCurrentCluster = clst;        /* Initialize data for new cluster */
                apDirObj->mCurrentSector = sK2FATFS_LockDC_ClusterToSector(pFatFs, clst);
            }
        }
    }
    apDirObj->mPointer = ofs;                        /* Current entry */
    apDirObj->mpRawData = pFatFs->mpAlignedWindow + ofs % K2FATFS_SECTOR_BYTES;    /* Pointer to the entry in the win[] */

    return K2STAT_NO_ERROR;
}

static K2STAT sK2FATFS_Locked_AllocDirEntries(K2FATFS_OBJ_DIR* apDirObj, UINT32 aEntryCount)
{
    K2STAT k2stat;
    UINT32 n;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;

    k2stat = sK2FATFS_Locked_SetDirIndex(apDirObj, 0);
    if (k2stat == K2STAT_NO_ERROR)
    {
        n = 0;
        do
        {
            k2stat = sK2FATFS_Locked_MoveWindow(pFatFs, apDirObj->mCurrentSector);
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            if (apDirObj->mpRawData[DIR_Name] == DDEM || apDirObj->mpRawData[DIR_Name] == 0)
            {    /* Is the entry free? */
                if (++n == aEntryCount) 
                    break;    /* Is a block of contiguous free entries found? */
            }
            else
            {
                n = 0;                /* Not a free entry, restart to search */
            }
            k2stat = sK2FATFS_Locked_DirMoveNext(apDirObj, 1);    /* Next entry with table stretch enabled */
        } while (k2stat == K2STAT_NO_ERROR);
    }

    if (k2stat == K2STAT_ERROR_NOT_FOUND) 
        k2stat = K2STAT_ERROR_NOT_ALLOWED;    /* No directory entry to allocate */
    return k2stat;
}

static UINT32 sK2FATFS_Locked_GetStartCluster(K2FATFS * apFatFs, const UINT8 * apDirEntObjEnt)
{
    UINT32 cl;

    cl = sK2FATFS_LoadFromUnalignedLe16(apDirEntObjEnt + DIR_FstClusLO);
    if (apFatFs->mFatType == K2FATFS_FATTYPE_FAT32)
    {
        cl |= (UINT32)sK2FATFS_LoadFromUnalignedLe16(apDirEntObjEnt + DIR_FstClusHI) << 16;
    }

    return cl;
}

static void sK2FATFS_Locked_PutStartCluster(K2FATFS * apFatFs, UINT8 * apDirEntObjEnt, UINT32 aClusterNumber)
{
    sK2FATFS_StoreToUnalignedLe16(apDirEntObjEnt + DIR_FstClusLO, (UINT16)aClusterNumber);
    if (apFatFs->mFatType == K2FATFS_FATTYPE_FAT32)
    {
        sK2FATFS_StoreToUnalignedLe16(apDirEntObjEnt + DIR_FstClusHI, (UINT16)(aClusterNumber >> 16));
    }
}

static int sK2FATFS_Locked_CompareLfn(const UINT16 * apLfnBuf, UINT8 * apDirEntObjEnt)
{
    UINT32 i, s;
    UINT16 wc, uc;

    if (sK2FATFS_LoadFromUnalignedLe16(apDirEntObjEnt + LDIR_FstClusLO) != 0) 
        return 0;    /* Check LDIR_FstClusLO */

    i = ((apDirEntObjEnt[LDIR_Ord] & 0x3F) - 1) * 13;    /* Offset in the LFN buffer */

    for (wc = 1, s = 0; s < 13; s++)
    {        /* Process all characters in the entry */
        uc = sK2FATFS_LoadFromUnalignedLe16(apDirEntObjEnt + LfnOfs[s]);        /* Pick an LFN character */
        if (wc != 0)
        {
            if (i >= FAT32_LFN_MAX_LENGTH + 1 || sK2FATFS_UniCharUpCase(uc) != sK2FATFS_UniCharUpCase(apLfnBuf[i++]))
            {    /* Compare it */
                return 0;                    /* Not matched */
            }
            wc = uc;
        }
        else
        {
            if (uc != 0xFFFF) 
                return 0;        /* Check filler */
        }
    }

    if ((apDirEntObjEnt[LDIR_Ord] & LLEF) && wc && apLfnBuf[i]) 
        return 0;    /* Last segment matched but different length */

    return 1;        /* The part of LFN matched */
}

static BOOL sK2FATFS_Locked_GetLfn(UINT16 * apLfnBuf, UINT8 * apDirEntObjEnt)
{
    UINT32 i, s;
    UINT16 wc, uc;

    if (sK2FATFS_LoadFromUnalignedLe16(apDirEntObjEnt + LDIR_FstClusLO) != 0) 
        return FALSE;    /* Check LDIR_FstClusLO is 0 */

    i = ((apDirEntObjEnt[LDIR_Ord] & ~LLEF) - 1) * 13;    /* Offset in the LFN buffer */

    for (wc = 1, s = 0; s < 13; s++)
    {        /* Process all characters in the entry */
        uc = sK2FATFS_LoadFromUnalignedLe16(apDirEntObjEnt + LfnOfs[s]);        /* Pick an LFN character */
        if (wc != 0)
        {
            if (i >= FAT32_LFN_MAX_LENGTH + 1) 
                return 0;    /* Buffer overflow? */
            apLfnBuf[i++] = wc = uc;            /* Store it */
        }
        else
        {
            if (uc != 0xFFFF) 
                return FALSE;        /* Check filler */
        }
    }

    if (apDirEntObjEnt[LDIR_Ord] & LLEF && wc != 0)
    {    /* Put terminator if it is the last LFN part and not terminated */
        if (i >= FAT32_LFN_MAX_LENGTH + 1) 
            return FALSE;    /* Buffer overflow? */
        apLfnBuf[i] = 0;
    }

    return TRUE;        /* The part of LFN is valid */
}

static void sK2FATFS_Locked_PutLfn(const UINT16 * apLfn, UINT8 * apDirEntObjEnt,  UINT8 aOrder, UINT8 aChecksum)
{
    UINT32 i, s;
    UINT16 wc;

    apDirEntObjEnt[LDIR_Chksum] = aChecksum;            /* Set checksum */
    apDirEntObjEnt[LDIR_Attr] = AM_LFN;        /* Set attribute. LFN entry */
    apDirEntObjEnt[LDIR_Type] = 0;
    sK2FATFS_StoreToUnalignedLe16(apDirEntObjEnt + LDIR_FstClusLO, 0);

    i = (aOrder - 1) * 13;                /* Get offset in the LFN working buffer */
    s = wc = 0;
    do
    {
        if (wc != 0xFFFF) 
            wc = apLfn[i++];    /* Get an effective character */
        sK2FATFS_StoreToUnalignedLe16(apDirEntObjEnt + LfnOfs[s], wc);        /* Put it */
        if (wc == 0) 
            wc = 0xFFFF;            /* Padding characters for following items */
    } while (++s < 13);
    if (wc == 0xFFFF || !apLfn[i]) 
        aOrder |= LLEF;    /* Last LFN part is the start of LFN sequence */
    apDirEntObjEnt[LDIR_Ord] = aOrder;            /* Set the LFN order */
}

static void sK2FATFS_LockDC_GetNumericName(UINT8 * apNameBuf, const UINT8 * apSfnEntry, const UINT16 * apLfnEntry, UINT32 aSeqNo )
{
    UINT8 ns[8], c;
    UINT32 i, j;
    UINT16 wc;
    UINT32 sreg;

    K2MEM_Copy(apNameBuf, apSfnEntry, 11);    /* Prepare the SFN to be modified */

    if (aSeqNo > 5)
    {    /* In case of many collisions, generate a hash number instead of sequential number */
        sreg = aSeqNo;
        while (*apLfnEntry)
        {    /* Create a CRC as hash value */
            wc = *apLfnEntry++;
            for (i = 0; i < 16; i++)
            {
                sreg = (sreg << 1) + (wc & 1);
                wc >>= 1;
                if (sreg & 0x10000) 
                    sreg ^= 0x11021;
            }
        }
        aSeqNo = (UINT32)sreg;
    }

    /* Make suffix (~ + hexadecimal) */
    i = 7;
    do
    {
        c = (UINT8)((aSeqNo % 16) + '0'); aSeqNo /= 16;
        if (c > '9') 
            c += 7;
        ns[i--] = c;
    } while (i && aSeqNo);
    ns[i] = '~';

    /* Append the suffix to the SFN body */
    for (j = 0; j < i && apNameBuf[j] != ' '; j++)
    {    /* Find the offset to append */
    }
    do
    {    /* Append the suffix */
        apNameBuf[j++] = (i < 8) ? ns[i++] : ' ';
    } while (j < 8);
}

static UINT8 sK2FATFS_LockDC_MakeSfnChecksum(const UINT8 * apSfnEntry)
{
    UINT8 sum = 0;
    UINT32 n = 11;

    do
    {
        sum = (sum >> 1) + (sum << 7) + *apSfnEntry++;
    } while (--n);
    return sum;
}

static K2STAT sK2FATFS_Locked_AdvanceDir(K2FATFS_OBJ_DIR * apDirObj)
{
    K2STAT k2stat = K2STAT_ERROR_NOT_FOUND;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;
    UINT8 attr, b;
    UINT8 ord = 0xFF, sum = 0xFF;

    while (apDirObj->mCurrentSector)
    {
        k2stat = sK2FATFS_Locked_MoveWindow(pFatFs, apDirObj->mCurrentSector);
        if (k2stat != K2STAT_NO_ERROR) 
            break;
        b = apDirObj->mpRawData[DIR_Name];    /* Test for the entry type */
        if (b == 0)
        {
            k2stat = K2STAT_ERROR_NOT_FOUND; 
            break; /* Reached to end of the directory */
        }
        {    /* On the FAT/FAT32 volume */
            apDirObj->Hdr.mObjAttr = attr = apDirObj->mpRawData[DIR_Attr] & AM_MASK;    /* Get attribute */
            if ((b == DDEM) || (b == '.') || ((int)((attr & ~K2_FSATTRIB_ARCHIVE) == AM_VOL)) != 0)
            {    /* An entry without valid data */
                ord = 0xFF;
            }
            else
            {
                if (attr == AM_LFN)
                {    /* An LFN entry is found */
                    if (b & LLEF)
                    {        /* Is it start of an LFN sequence? */
                        sum = apDirObj->mpRawData[LDIR_Chksum];
                        b &= (UINT8)~LLEF; ord = b;
                        apDirObj->mCurBlockOffset = apDirObj->mPointer;
                    }
                    /* Check LFN validity and capture it */
                    ord = (b == ord && sum == apDirObj->mpRawData[LDIR_Chksum] && sK2FATFS_Locked_GetLfn(pFatFs->mLfnWork, apDirObj->mpRawData)) ? ord - 1 : 0xFF;
                }
                else
                {                /* An SFN entry is found */
                    if (ord != 0 || sum != sK2FATFS_LockDC_MakeSfnChecksum(apDirObj->mpRawData))
                    {    /* Is there a valid LFN? */
                        apDirObj->mCurBlockOffset = 0xFFFFFFFF;    /* It has no LFN. */
                    }
                    break;
                }
            }
        }
        k2stat = sK2FATFS_Locked_DirMoveNext(apDirObj, 0);        /* Next entry */
        if (k2stat != K2STAT_NO_ERROR) 
            break;
    }

    if (k2stat != K2STAT_NO_ERROR) 
        apDirObj->mCurrentSector = 0;        /* Terminate the read operation on error or EOT */

    return k2stat;
}

static K2STAT sK2FATFS_Locked_FindLfnWorkInDir(K2FATFS_OBJ_DIR * apDirObj)
{
    K2STAT k2stat;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;
    UINT8 c;
    UINT8 a, ord, sum;

    k2stat = sK2FATFS_Locked_SetDirIndex(apDirObj, 0);            /* Rewind directory object */
    if (k2stat != K2STAT_NO_ERROR) 
        return k2stat;
    /* On the FAT/FAT32 volume */
    ord = sum = 0xFF; apDirObj->mCurBlockOffset = 0xFFFFFFFF;    /* Reset LFN sequence */
    do
    {
        k2stat = sK2FATFS_Locked_MoveWindow(pFatFs, apDirObj->mCurrentSector);
        if (k2stat != K2STAT_NO_ERROR) 
            break;
        c = apDirObj->mpRawData[DIR_Name];
        if (c == 0)
        {
            k2stat = K2STAT_ERROR_NOT_FOUND; 
            break;
        }    /* Reached to end of table */
        apDirObj->Hdr.mObjAttr = a = apDirObj->mpRawData[DIR_Attr] & AM_MASK;
        if (c == DDEM || ((a & AM_VOL) && a != AM_LFN))
        {    /* An entry without valid data */
            ord = 0xFF; apDirObj->mCurBlockOffset = 0xFFFFFFFF;    /* Reset LFN sequence */
        }
        else
        {
            if (a == AM_LFN)
            {            /* An LFN entry is found */
                if (!(apDirObj->mSfn[NSFLAG] & NS_NOLFN))
                {
                    if (c & LLEF)
                    {        /* Is it start of LFN sequence? */
                        sum = apDirObj->mpRawData[LDIR_Chksum];
                        c &= (UINT8)~LLEF; ord = c;    /* LFN start order */
                        apDirObj->mCurBlockOffset = apDirObj->mPointer;    /* Start offset of LFN */
                    }
                    /* Check validity of the LFN entry and compare it with given name */
                    ord = (c == ord && sum == apDirObj->mpRawData[LDIR_Chksum] && sK2FATFS_Locked_CompareLfn(pFatFs->mLfnWork, apDirObj->mpRawData)) ? ord - 1 : 0xFF;
                }
            }
            else
            {                    /* An SFN entry is found */
                if (ord == 0 && sum == sK2FATFS_LockDC_MakeSfnChecksum(apDirObj->mpRawData)) 
                    break;    /* LFN matched? */
                if (!(apDirObj->mSfn[NSFLAG] & NS_LOSS) && (0 == K2MEM_Compare(apDirObj->mpRawData, apDirObj->mSfn, 11))) 
                    break;    /* SFN matched? */
                ord = 0xFF; 
                apDirObj->mCurBlockOffset = 0xFFFFFFFF;    /* Reset LFN sequence */
            }
        }
        k2stat = sK2FATFS_Locked_DirMoveNext(apDirObj, 0);    /* Next entry */
    } while (k2stat == K2STAT_NO_ERROR);
   
    return k2stat;
}

static K2STAT sK2FATFS_Locked_CreateDirEntry(K2FATFS_OBJ_DIR * apDirObj)
{
    K2STAT k2stat;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;
    UINT32 n, len, n_ent;
    UINT8 sn[12], sum;

    if (apDirObj->mSfn[NSFLAG] & (NS_DOT | NS_NONAME)) 
        return K2STAT_ERROR_BAD_NAME;    /* Check name validity */
    for (len = 0; pFatFs->mLfnWork[len]; len++);    /* Get lfn length */

    /* On the FAT/FAT32 volume */
    K2MEM_Copy(sn, apDirObj->mSfn, 12);
    if (sn[NSFLAG] & NS_LOSS)
    {            /* When LFN is out of 8.3 format, generate a numbered name */
        apDirObj->mSfn[NSFLAG] = NS_NOLFN;        /* Find only SFN */
        for (n = 1; n < 100; n++)
        {
            sK2FATFS_LockDC_GetNumericName(apDirObj->mSfn, sn, pFatFs->mLfnWork, n);    /* Generate a numbered name */
            k2stat = sK2FATFS_Locked_FindLfnWorkInDir(apDirObj);                /* Check if the name collides with existing SFN */
            if (k2stat != K2STAT_NO_ERROR) 
                break;
        }
        if (n == 100) 
            return K2STAT_ERROR_NOT_ALLOWED;        /* Abort if too many collisions */
        if (k2stat != K2STAT_ERROR_NOT_FOUND) 
            return k2stat;    /* Abort if the result is other than 'not collided' */
        apDirObj->mSfn[NSFLAG] = sn[NSFLAG];
    }

    /* Create an SFN with/without LFNs. */
    n_ent = (sn[NSFLAG] & NS_LFN) ? (len + 12) / 13 + 1 : 1;    /* Number of entries to allocate */
    k2stat = sK2FATFS_Locked_AllocDirEntries(apDirObj, n_ent);        /* Allocate entries */
    if (k2stat == K2STAT_NO_ERROR && --n_ent)
    {    /* Set LFN entry if needed */
        k2stat = sK2FATFS_Locked_SetDirIndex(apDirObj, apDirObj->mPointer - n_ent * SZDIRE);
        if (k2stat == K2STAT_NO_ERROR)
        {
            sum = sK2FATFS_LockDC_MakeSfnChecksum(apDirObj->mSfn);    /* Checksum value of the SFN tied to the LFN */
            do
            {                    /* Store LFN entries in bottom first */
                k2stat = sK2FATFS_Locked_MoveWindow(pFatFs, apDirObj->mCurrentSector);
                if (k2stat != K2STAT_NO_ERROR) 
                    break;
                sK2FATFS_Locked_PutLfn(pFatFs->mLfnWork, apDirObj->mpRawData, (UINT8)n_ent, sum);
                pFatFs->mWindowIsDirtyFlag = 1;
                k2stat = sK2FATFS_Locked_DirMoveNext(apDirObj, 0);    /* Next entry */
            } while (k2stat == K2STAT_NO_ERROR && --n_ent);
        }
    }

    /* Set SFN entry */
    if (k2stat == K2STAT_NO_ERROR)
    {
        k2stat = sK2FATFS_Locked_MoveWindow(pFatFs, apDirObj->mCurrentSector);
        if (k2stat == K2STAT_NO_ERROR)
        {
            K2MEM_Zero(apDirObj->mpRawData, SZDIRE);    /* Clean the entry */
            K2MEM_Copy(apDirObj->mpRawData + DIR_Name, apDirObj->mSfn, 11);    /* Put SFN */
            apDirObj->mpRawData[DIR_NTres] = apDirObj->mSfn[NSFLAG] & (NS_BODY | NS_EXT);    /* Put NT flag */
            pFatFs->mWindowIsDirtyFlag = 1;
        }
    }

    return k2stat;
}

static K2STAT sK2FATFS_Locked_RemoveDirEntry(K2FATFS_OBJ_DIR * apDirObj)
{
    K2STAT k2stat;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;
    UINT32 last = apDirObj->mPointer;

    k2stat = (apDirObj->mCurBlockOffset == 0xFFFFFFFF) ? K2STAT_NO_ERROR : sK2FATFS_Locked_SetDirIndex(apDirObj, apDirObj->mCurBlockOffset);    /* Goto top of the entry block if LFN is exist */
    if (k2stat == K2STAT_NO_ERROR)
    {
        do
        {
            k2stat = sK2FATFS_Locked_MoveWindow(pFatFs, apDirObj->mCurrentSector);
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            apDirObj->mpRawData[DIR_Name] = DDEM;    /* Mark the entry 'deleted'. */
            pFatFs->mWindowIsDirtyFlag = 1;
            if (apDirObj->mPointer >= last) 
                break;    /* If reached last entry then all entries of the object has been deleted. */
            k2stat = sK2FATFS_Locked_DirMoveNext(apDirObj, 0);    /* Next entry */
        } while (k2stat == K2STAT_NO_ERROR);
        if (k2stat == K2STAT_ERROR_NOT_FOUND) 
            k2stat = K2STAT_ERROR_UNKNOWN;
    }

    return k2stat;
}

static void sK2FATFS_Locked_GetFileInfo(K2FATFS_OBJ_DIR * apDirObj, K2FATFS_FILEINFO * apRetFileInfo)
{
    UINT32 si, di;
    UINT8 lcf;
    UINT16 wc, hs;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;
    UINT32 nw;

    apRetFileInfo->mName[0] = 0;            /* Invaidate file info */
    if (apDirObj->mCurrentSector == 0) 
        return;    /* Exit if read pointer has reached end of directory */


        if (apDirObj->mCurBlockOffset != 0xFFFFFFFF)
        {    /* Get LFN if available */
            si = di = 0;
            hs = 0;
            while (pFatFs->mLfnWork[si] != 0)
            {
                wc = pFatFs->mLfnWork[si++];        /* Get an LFN character (UTF-16) */
                if (hs == 0 && IsSurrogate(wc))
                {    /* Is it a surrogate? */
                    hs = wc; 
                    continue;        /* Get low surrogate */
                }
                nw = sK2FATFS_StorUniChar((UINT32)hs << 16 | wc, &apRetFileInfo->mName[di], FAT32_LFN_MAX_LENGTH - di);    /* Store it in API encoding */
                if (nw == 0)
                {                /* Buffer overflow or wrong char? */
                    di = 0; 
                    break;
                }
                di += nw;
                hs = 0;
            }
            if (hs != 0) 
                di = 0;    /* Broken surrogate pair? */
            apRetFileInfo->mName[di] = 0;        /* Terminate the LFN (null string means LFN is invalid) */
        }

    si = di = 0;
    while (si < 11)
    {        /* Get SFN from SFN entry */
        wc = apDirObj->mpRawData[si++];            /* Get a char */
        if (wc == ' ') 
            continue;    /* Skip padding spaces */
        if (wc == RDDEM) 
            wc = DDEM;    /* Restore replaced DDEM character */
        if (si == 9 && di < FAT32_SFN_MAX_LENGTH) 
            apRetFileInfo->mAltName[di++] = '.';    /* Insert a . if extension is exist */
        apRetFileInfo->mAltName[di++] = (char)wc;    /* Store it without any conversion */
    }
    apRetFileInfo->mAltName[di] = 0;    /* Terminate the SFN  (null string means SFN is invalid) */

    if (apRetFileInfo->mName[0] == 0)
    {    /* If LFN is invalid, mAltName[] needs to be copied to fname[] */
        if (di == 0)
        {    /* If LFN and SFN both are invalid, this object is inaccessible */
            apRetFileInfo->mName[di++] = '\?';
        }
        else
        {
            for (si = di = 0, lcf = NS_BODY; apRetFileInfo->mAltName[si]; si++, di++)
            {    /* Copy mAltName[] to fname[] with case information */
                wc = (UINT16)apRetFileInfo->mAltName[si];
                if (wc == '.') 
                    lcf = NS_EXT;
                if (IsUpper(wc) && (apDirObj->mpRawData[DIR_NTres] & lcf)) 
                    wc += 0x20;
                apRetFileInfo->mName[di] = (char)wc;
            }
        }
        apRetFileInfo->mName[di] = 0;    /* Terminate the LFN */
        if (!apDirObj->mpRawData[DIR_NTres]) 
            apRetFileInfo->mAltName[0] = 0;    /* Altname is not needed if neither LFN nor case info is exist. */
    }

    apRetFileInfo->mAttrib = apDirObj->mpRawData[DIR_Attr] & AM_MASK;            /* Attribute */
    apRetFileInfo->mSizeBytes = sK2FATFS_LoadFromUnalignedLe32(apDirObj->mpRawData + DIR_FileSize);        /* Size */
    apRetFileInfo->mTime16 = sK2FATFS_LoadFromUnalignedLe16(apDirObj->mpRawData + DIR_ModTime + 0);    /* Time */
    apRetFileInfo->mDate16 = sK2FATFS_LoadFromUnalignedLe16(apDirObj->mpRawData + DIR_ModTime + 2);    /* Date */
}

static UINT32 sK2FATFS_LockDC_EatChar(const char * *appChar)
{
    UINT32 chr;

    chr = (UINT8) *(*appChar)++;                /* Get a byte */
    if (IsLower(chr)) 
        chr -= 0x20;        /* To upper ASCII char */
    if (chr >= 0x80) 
        chr = ExCvt[chr - 0x80];    /* To upper SBCS extended char */

    return chr;
}

static BOOL sK2FATFS_LockDC_PatternMatch(const char * apPattern, const char * apName, UINT32 aSkipCount, UINT32 aRecursionDepth)
{
    const char *pptr;
    const char *nptr;
    UINT32 pchr, nchr;
    UINT32 sk;

    while ((aSkipCount & 0xFF) != 0)
    {        /* Pre-aSkipCount name chars */
        if (!sK2FATFS_LockDC_EatChar(&apName)) 
            return FALSE;    /* Branch mismatched if less name chars */
        aSkipCount--;
    }
    if (*apPattern == 0 && aSkipCount) 
        return TRUE;

    do
    {
        pptr = apPattern; nptr = apName;            /* Top of pattern and name to match */
        for (;;)
        {
            if (*pptr == '\?' || *pptr == '*')
            {    /* Wildcard term? */
                if (aRecursionDepth == 0) 
                    return FALSE;    /* Too many wildcard terms? */
                sk = 0;
                do
                {    /* Analyze the wildcard term */
                    if (*pptr++ == '\?')
                    {
                        sk++;
                    }
                    else
                    {
                        sk |= 0x100;
                    }
                } while (*pptr == '\?' || *pptr == '*');
                if (sK2FATFS_LockDC_PatternMatch(pptr, nptr, sk, aRecursionDepth - 1)) 
                    return TRUE;    /* Test new branch (recursive call) */
                nchr = *nptr; 
                break;    /* Branch mismatched */
            }
            pchr = sK2FATFS_LockDC_EatChar(&pptr);    /* Get a pattern char */
            nchr = sK2FATFS_LockDC_EatChar(&nptr);    /* Get a name char */
            if (pchr != nchr) 
                break;    /* Branch mismatched? */
            if (pchr == 0) 
                return TRUE;    /* Branch matched? (matched at end of both strings) */
        }
        sK2FATFS_LockDC_EatChar(&apName);            /* apName++ */
    } while (aSkipCount && nchr);        /* Retry until end of name if infinite search is specified */

    return FALSE;
}

static K2STAT sK2FATFS_LockDC_CreateName(K2FATFS_OBJ_DIR * apDirObj, const char * *appPath)
{
    UINT8 b, cf;
    UINT16 wc;
    UINT16 *lfn;
    const char* p;
    UINT32 uc;
    UINT32 i, ni, si, di;

    /* Create LFN into LFN working buffer */
    p = *appPath; lfn = apDirObj->Hdr.mpParentFs->mLfnWork; di = 0;
    for (;;)
    {
        uc = sK2FATFS_EatCharToUnicode(&p);            /* Get a character */
        if (uc == 0xFFFFFFFF) 
            return K2STAT_ERROR_BAD_NAME;        /* Invalid code or UTF decode error */
        if (uc >= 0x10000) 
            lfn[di++] = (UINT16)(uc >> 16);    /* Store high surrogate if needed */
        wc = (UINT16)uc;
        if (wc < ' ' || IsSeparator(wc)) 
            break;    /* Break if end of the appPath or a separator is found */
        if ((wc < 0x80) &&
            (NULL != K2ASC_FindCharConstLen((char)wc, "*:<>|\"\?\x7F", (UINT_PTR)-1)))
            return K2STAT_ERROR_BAD_NAME;    /* Reject illegal characters for LFN */
        if (di >= FAT32_LFN_MAX_LENGTH) 
            return K2STAT_ERROR_BAD_NAME;    /* Reject too long name */
        lfn[di++] = wc;                /* Store the Unicode character */
    }
    if (wc < ' ')
    {                /* Stopped at end of the appPath? */
        cf = NS_LAST;            /* Last segment */
    }
    else
    {                    /* Stopped at a separator */
        while (IsSeparator(*p)) 
            p++;    /* Skip duplicated separators if exist */
        cf = 0;                    /* Next segment may follow */
        if (IsTerminator(*p)) 
            cf = NS_LAST;    /* Ignore terminating separator */
    }
    *appPath = p;                    /* Return pointer to the next segment */

    if ((di == 1 && lfn[di - 1] == '.') ||
        (di == 2 && lfn[di - 1] == '.' && lfn[di - 2] == '.'))
    {    /* Is this segment a dot name? */
        lfn[di] = 0;
        for (i = 0; i < 11; i++)
        {    /* Create dot name for SFN entry */
            apDirObj->mSfn[i] = (i < di) ? '.' : ' ';
        }
        apDirObj->mSfn[i] = cf | NS_DOT;    /* This is a dot entry */
        return K2STAT_NO_ERROR;
    }
    while (di)
    {                    /* Snip off trailing spaces and dots if exist */
        wc = lfn[di - 1];
        if (wc != ' ' && wc != '.') 
            break;
        di--;
    }
    lfn[di] = 0;                            /* LFN is created into the working buffer */
    if (di == 0) 
        return K2STAT_ERROR_BAD_NAME;    /* Reject null name */

    /* Create SFN in directory form */
    for (si = 0; lfn[si] == ' '; si++);    /* Remove leading spaces */
    if (si > 0 || lfn[si] == '.') 
        cf |= NS_LOSS | NS_LFN;    /* Is there any leading space or dot? */
    while (di > 0 && lfn[di - 1] != '.') 
        di--;    /* Find last dot (di<=si: no extension) */

    K2MEM_Set(apDirObj->mSfn, ' ', 11);
    i = b = 0; ni = 8;
    for (;;)
    {
        wc = lfn[si++];                    /* Get an LFN character */
        if (wc == 0) 
            break;                /* Break on end of the LFN */
        if (wc == ' ' || (wc == '.' && si != di))
        {    /* Remove embedded spaces and dots */
            cf |= NS_LOSS | NS_LFN;
            continue;
        }

        if (i >= ni || si == di)
        {        /* End of field? */
            if (ni == 11)
            {                /* Name extension overflow? */
                cf |= NS_LOSS | NS_LFN;
                break;
            }
            if (si != di) 
                cf |= NS_LOSS | NS_LFN;    /* Name body overflow? */
            if (si > di) 
                break;                        /* No name extension? */
            si = di; i = 8; ni = 11; b <<= 2;        /* Enter name extension */
            continue;
        }

        if (wc >= 0x80)
        {    /* Is this an extended character? */
            cf |= NS_LFN;    /* LFN entry needs to be created */
            wc = sK2FATFS_UniCharToOem(wc);            /* Unicode ==> ANSI/OEM code */
            if (wc & 0x80) 
                wc = ExCvt[wc & 0x7F];    /* Convert extended character to upper (SBCS) */
        }

        if (wc >= 0x100)
        {                /* Is this a DBC? */
            if (i >= ni - 1)
            {            /* Field overflow? */
                cf |= NS_LOSS | NS_LFN;
                i = ni; 
                continue;        /* Next field */
            }
            apDirObj->mSfn[i++] = (UINT8)(wc >> 8);    /* Put 1st byte */
        }
        else
        {                        /* SBC */
            if ((wc == 0) ||
                (NULL != K2ASC_FindCharConstLen((char)wc, "+,;=[]", (UINT_PTR)-1)))
            {    /* Replace illegal characters for SFN */
                wc = '_'; cf |= NS_LOSS | NS_LFN;/* Lossy conversion */
            }
            else
            {
                if (IsUpper(wc))
                {        /* ASCII upper case? */
                    b |= 2;
                }
                if (IsLower(wc))
                {        /* ASCII lower case? */
                    b |= 1; wc -= 0x20;
                }
            }
        }
        apDirObj->mSfn[i++] = (UINT8)wc;
    }

    if (apDirObj->mSfn[0] == DDEM) 
        apDirObj->mSfn[0] = RDDEM;    /* If the first character collides with DDEM, replace it with RDDEM */

    if (ni == 8) 
        b <<= 2;                /* Shift capital flags if no extension */
    if ((b & 0x0C) == 0x0C || (b & 0x03) == 0x03) 
        cf |= NS_LFN;    /* LFN entry needs to be created if composite capitals */
    if (!(cf & NS_LFN))
    {                /* When LFN is in 8.3 format without extended character, NT flags are created */
        if (b & 0x01)
            cf |= NS_EXT;        /* NT flag (Extension has small capital letters only) */
        if (b & 0x04) 
            cf |= NS_BODY;    /* NT flag (Body has small capital letters only) */
    }

    apDirObj->mSfn[NSFLAG] = cf;    /* SFN is created into apDirObj->mSfn[] */

    return K2STAT_NO_ERROR;
}

static K2STAT sK2FATFS_Locked_FollowPath(K2FATFS_OBJ_DIR * apDirObj, const char * apPath)
{
    K2STAT k2stat;
    UINT8 ns;
    K2FATFS *pFatFs = apDirObj->Hdr.mpParentFs;

    if (!IsSeparator(*apPath) && (!IsTerminator(*apPath)))
    {    /* Without heading separator */
        apDirObj->Hdr.mStartCluster = pFatFs->mCurrentDirStartCluster;            /* Start at the current directory */
    }
    else
    {                                        /* With heading separator */
        while (IsSeparator(*apPath)) 
            apPath++;    /* Strip separators */
        apDirObj->Hdr.mStartCluster = 0;                    /* Start from the root directory */
    }

    if ((UINT32)*apPath < ' ')
    {                /* Null apPath name is the origin directory itself */
        apDirObj->mSfn[NSFLAG] = NS_NONAME;
        k2stat = sK2FATFS_Locked_SetDirIndex(apDirObj, 0);

    }
    else
    {                                /* Follow apPath */
        for (;;)
        {
            k2stat = sK2FATFS_LockDC_CreateName(apDirObj, &apPath);    /* Get a segment name of the path */
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            k2stat = sK2FATFS_Locked_FindLfnWorkInDir(apDirObj);                /* Find an object with the segment name */
            ns = apDirObj->mSfn[NSFLAG];
            if (k2stat != K2STAT_NO_ERROR)
            {                /* Failed to find the object */
                if (k2stat == K2STAT_ERROR_NOT_FOUND)
                {    /* Object is not found */
                    if (ns & NS_DOT)
                    {    /* If dot entry is not exist, stay there */
                        if (!(ns & NS_LAST)) 
                            continue;    /* Continue to follow if not last segment */
                        apDirObj->mSfn[NSFLAG] = NS_NONAME;
                        k2stat = K2STAT_NO_ERROR;
                    }
                    else
                    {                            /* Could not find the object */
                        if (!(ns & NS_LAST)) 
                            k2stat = K2STAT_ERROR_NO_PATH;    /* Adjust error code if not last segment */
                    }
                }
                break;
            }
            if (ns & NS_LAST) 
                break;        /* Last segment matched. Function completed. */
            /* Get into the sub-directory */
            if (!(apDirObj->Hdr.mObjAttr & K2_FSATTRIB_DIR))
            {    /* It is not a sub-directory and cannot follow */
                k2stat = K2STAT_ERROR_NO_PATH; 
                break;
            }
            {
                apDirObj->Hdr.mStartCluster = sK2FATFS_Locked_GetStartCluster(pFatFs, pFatFs->mpAlignedWindow + apDirObj->mPointer % K2FATFS_SECTOR_BYTES);    /* Open next directory */
            }
        }
    }

    return k2stat;
}

static UINT32 sK2FATFS_Locked_CheckFatSector(K2FATFS * apFatFs, UINT32 aSectorNumber)
{
    UINT16 w, sign;
    UINT8 b;

    apFatFs->mWindowIsDirtyFlag = 0; apFatFs->mCurWinSector = (UINT32)0 - 1;        /* Invaidate window */
    if (sK2FATFS_Locked_MoveWindow(apFatFs, aSectorNumber) != K2STAT_NO_ERROR) 
        return FF_CHECKFS_DISK_ERROR;
    sign = sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BS_55AA);
    b = apFatFs->mpAlignedWindow[BS_JmpBoot];
    if (b == 0xEB || b == 0xE9 || b == 0xE8)
    {    /* Valid JumpBoot code? (short jump, near jump or near call) */
        if (sign == 0xAA55 && (0 == K2MEM_Compare(apFatFs->mpAlignedWindow + BS_FilSysType32, "FAT32   ", 8)))
        {
            return FF_CHECKFS_FAT_VBR;    /* It is an FAT32 VBR */
        }
        /* FAT volumes formatted with early MS-DOS lack BS_55AA and BS_FilSysType, so FAT VBR needs to be identified without them. */
        w = sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_BytsPerSec);
        b = apFatFs->mpAlignedWindow[BPB_SecPerClus];
        if ((w == K2FATFS_SECTOR_BYTES)
            && b != 0 && (b & (b - 1)) == 0                /* Properness of cluster size (2^n) */
            && sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_RsvdSecCnt) != 0    /* Properness of reserved sectors (MNBZ) */
            && (UINT32)apFatFs->mpAlignedWindow[BPB_NumFATs] - 1 <= 1        /* Properness of FATs (1 or 2) */
            && sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_RootEntCnt) != 0    /* Properness of root dir entries (MNBZ) */
            && (sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_TotSec16) >= 128 || sK2FATFS_LoadFromUnalignedLe32(apFatFs->mpAlignedWindow + BPB_TotSec32) >= 0x10000)    /* Properness of volume sectors (>=128) */
            && sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_FATSz16) != 0)
        {    /* Properness of FAT size (MNBZ) */
            return FF_CHECKFS_FAT_VBR;    /* It can be presumed an FAT VBR */
        }
    }
    return sign == 0xAA55 ? FF_CHECKFS_NOT_FAT_VALID_BS : FF_CHECKFS_NOT_FAT_INVALID;    /* Not an FAT VBR (valid or invalid BS) */
}

static UINT32 sK2FATFS_Locked_FindFatVolume(K2FATFS * apFatFs)
{
    UINT32 fmt;
    fmt = sK2FATFS_Locked_CheckFatSector(apFatFs, 0);
    if (fmt != FF_CHECKFS_NOT_FAT_VALID_BS)
        return fmt;    
    return FF_CHECKFS_NOT_FAT_INVALID;
}

static K2STAT sK2FATFS_CheckAndLockVolume(K2FATFS * apFatFs, UINT8 aMode)
{
    K2STAT k2stat;
    UINT32 tsect, sysect, fasize, nclst, szbfat;
    UINT16 nrsv;
    UINT32 fmt;

    K2_ASSERT(NULL != apFatFs);

    k2stat = sK2FATFS_LockVolume(apFatFs);
    if (K2STAT_IS_ERROR(k2stat))
        return K2STAT_ERROR_TIMEOUT;

    do
    {
        k2stat = K2STAT_NO_ERROR;

        aMode &= (UINT8)~K2FATFS_ACCESS_READ;                /* Desired access mode, write access or not */

        if (apFatFs->mFatType != 0)
        {
            k2stat = apFatFs->mpOps->DiskStatus(apFatFs->mpOps);
            if (K2STAT_IS_ERROR(k2stat))
            {
                k2stat = K2STAT_ERROR_HARDWARE;
            }
            else
            {
                if ((0 != (apFatFs->VolInfo.mAttributes & K2_STORAGE_VOLUME_ATTRIB_READ_ONLY)) &&
                    (0 != (aMode & K2FATFS_ACCESS_WRITE)))
                {
                    k2stat = K2STAT_ERROR_READ_ONLY;
                }
            }
            break;
        }

        //
        // detect
        //
        fmt = sK2FATFS_Locked_FindFatVolume(apFatFs);
        if (fmt == FF_CHECKFS_DISK_ERROR)
        {
            k2stat = K2STAT_ERROR_HARDWARE;        /* An error occurred in the disk I/O layer */
            break;
        }
        if (fmt >= FF_CHECKFS_NOT_FAT_VALID_BS)
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* No FAT volume is found */
            break;
        }

        if (sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_BytsPerSec) != K2FATFS_SECTOR_BYTES)
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (BPB_BytsPerSec must be equal to the physical sector size) */
            break;
        }

        fasize = sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_FATSz16);        /* Number of sectors per FAT */
        if (fasize == 0)
            fasize = sK2FATFS_LoadFromUnalignedLe32(apFatFs->mpAlignedWindow + BPB_FATSz32);
        apFatFs->mSectorsPerFat = fasize;

        apFatFs->mFatCount = apFatFs->mpAlignedWindow[BPB_NumFATs];                /* Number of FATs */
        if (apFatFs->mFatCount != 1 && apFatFs->mFatCount != 2)
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (Must be 1 or 2) */
            break;
        }
        fasize *= apFatFs->mFatCount;                            /* Number of sectors for FAT area */

        apFatFs->mSectorsPerCluster = apFatFs->mpAlignedWindow[BPB_SecPerClus];            /* Cluster size */
        if (apFatFs->mSectorsPerCluster == 0 || (apFatFs->mSectorsPerCluster & (apFatFs->mSectorsPerCluster - 1)))
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (Must be power of 2) */
            break;
        }

        apFatFs->mRootDirEntCount = sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_RootEntCnt);    /* Number of root directory entries */
        if (apFatFs->mRootDirEntCount % (K2FATFS_SECTOR_BYTES / SZDIRE))
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (Must be sector aligned) */
            break;
        }

        tsect = sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_TotSec16);        /* Number of sectors on the volume */
        if (tsect == 0) 
            tsect = sK2FATFS_LoadFromUnalignedLe32(apFatFs->mpAlignedWindow + BPB_TotSec32);

        nrsv = sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_RsvdSecCnt);        /* Number of reserved sectors */
        if (nrsv == 0)
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;            /* (Must not be 0) */
            break;
        }

        /* Determine the FAT sub type */
        sysect = nrsv + fasize + apFatFs->mRootDirEntCount / (K2FATFS_SECTOR_BYTES / SZDIRE);    /* RSV + FAT + K2FATFS_OBJ_DIR */
        if (tsect < sysect)
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (Invalid volume size) */
            break;
        }

        nclst = (tsect - sysect) / apFatFs->mSectorsPerCluster;            /* Number of clusters */
        if (nclst == 0)
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;        /* (Invalid volume size) */
            break;
        }

        fmt = 0;
        if (nclst <= MAX_FAT32) 
            fmt = K2FATFS_FATTYPE_FAT32;
        if (nclst <= MAX_FAT16) 
            fmt = K2FATFS_FATTYPE_FAT16;
        if (nclst <= MAX_FAT12) 
            fmt = K2FATFS_FATTYPE_FAT12;
        if (fmt == 0)
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;
            break;
        }

        /* Boundaries and Limits */
        apFatFs->mFatEntryCount = nclst + 2;                        /* Number of FAT entries */
        apFatFs->mFatBaseSector = nrsv;                     /* FAT start sector */
        apFatFs->mDataBaseSector = sysect;                    /* Data start sector */
        if (fmt == K2FATFS_FATTYPE_FAT32)
        {
            if (sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + BPB_FSVer32) != 0)
            {
                k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (Must be FAT32 revision 0.0) */
                break;
            }
            if (apFatFs->mRootDirEntCount != 0)
            {
                k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (BPB_RootEntCnt must be 0) */
                break;
            }
            apFatFs->mRootDirBase = sK2FATFS_LoadFromUnalignedLe32(apFatFs->mpAlignedWindow + BPB_RootClus32);    /* Root directory start cluster */
            szbfat = apFatFs->mFatEntryCount * 4;                    /* (Needed FAT size) */
        }
        else
        {
            if (apFatFs->mRootDirEntCount == 0)
            {
                k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (BPB_RootEntCnt must not be 0) */
                break;
            }
            apFatFs->mRootDirBase = apFatFs->mFatBaseSector + fasize;            /* Root directory start sector */
            szbfat = (fmt == K2FATFS_FATTYPE_FAT16) ?                /* (Needed FAT size) */
                apFatFs->mFatEntryCount * 2 : apFatFs->mFatEntryCount * 3 / 2 + (apFatFs->mFatEntryCount & 1);
        }
        if (apFatFs->mSectorsPerFat < (szbfat + (K2FATFS_SECTOR_BYTES - 1)) / K2FATFS_SECTOR_BYTES)
        {
            k2stat = K2STAT_ERROR_NOT_CONFIGURED;    /* (BPB_FATSz must not be less than the size needed) */
            break;
        }

        /* Get FSInfo if available */
        apFatFs->mLastAllocCluster = apFatFs->mFreeClusterCount = 0xFFFFFFFF;        /* Initialize cluster allocation information */
        apFatFs->mFsiFlags = FF_FSIFLAG_DISABLED;

        apFatFs->mFatType = (UINT8)fmt;/* FAT sub-type (the filesystem object gets valid) */
        apFatFs->mCurrentDirStartCluster = 0;            /* Initialize current directory */

    } while (0);

    if (k2stat != K2STAT_NO_ERROR)
    {
        sK2FATFS_Locked_UnlockVolume(apFatFs);
    }

    return k2stat;
}

static K2STAT sK2FATFS_ValidatObjAndLockVolume(K2FATFS_OBJ_HDR * apObj, K2FATFS **appRetFs)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;

    k2stat = K2STAT_ERROR_CORRUPTED;

    if (apObj && apObj->mpParentFs && apObj->mpParentFs->mFatType)
    {    /* Test if the object is valid */
        pFatFs = apObj->mpParentFs;
        k2stat = sK2FATFS_LockVolume(pFatFs);
        if (!K2STAT_IS_ERROR(k2stat))
        {
            *appRetFs = pFatFs;
        }
        else
        {    /* Could not take */
            k2stat = K2STAT_ERROR_TIMEOUT;
        }
    }

    return k2stat;
}

//
// ----------------------------------------------------------------------------------
//

BOOL K2FATFS_iseof(K2FATFS_OBJ_FILE *apFileObj)
{
    if (apFileObj->mPointer == apFileObj->Hdr.mObjSize)
        return TRUE;
    return FALSE;
}

UINT8 K2FATFS_haserror(K2FATFS_OBJ_FILE *apFileObj)
{
    return apFileObj->mErrorCode;
}

UINT32 K2FATFS_tell(K2FATFS_OBJ_FILE *apFileObj)
{
    return apFileObj->mPointer;
}

UINT32 K2FATFS_size(K2FATFS_OBJ_FILE *apFileObj)
{
    return apFileObj->Hdr.mObjSize;
}

K2STAT K2FATFS_mount(K2FATFS * apFatFs, K2FATFS_OBJ_DIR *apFillRootDir)
{
    K2STAT k2stat;

    if (0 != apFatFs->mFatType)
    {
        return K2STAT_ERROR_ALREADY_OPEN;
    }

    if ((NULL == apFatFs->mpOps) ||
        (apFatFs->VolInfo.mBlockSizeBytes != K2FATFS_SECTOR_BYTES))
        return K2STAT_ERROR_BAD_ARGUMENT;

    apFatFs->mMutex = apFatFs->mpOps->MutexCreate(apFatFs->mpOps);
    if (0 == apFatFs->mMutex)
        return K2STAT_ERROR_UNKNOWN;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, 0);
    if (K2STAT_NO_ERROR == k2stat)
    {
        K2MEM_Zero(apFillRootDir, sizeof(K2FATFS_OBJ_DIR));

        apFillRootDir->Hdr.mpParentFs = apFatFs;
        apFillRootDir->mSfn[NSFLAG] = NS_NONAME;
        k2stat = sK2FATFS_Locked_SetDirIndex(apFillRootDir, 0);
        if (K2STAT_IS_ERROR(k2stat))
        {
            apFillRootDir->Hdr.mpParentFs = NULL;
        }

        sK2FATFS_Locked_UnlockVolume(apFatFs);
    }

    if (K2STAT_IS_ERROR(k2stat))
    {
        apFatFs->mpOps->MutexDelete(apFatFs->mpOps, apFatFs->mMutex);
        apFatFs->mMutex = 0;
        apFatFs->mFatType = 0;
    }

    return k2stat;
}

K2STAT K2FATFS_unmount(K2FATFS_OBJ_DIR *apRootDir)
{
    K2FATFS *pFatFs;

    if (NULL == apRootDir)
        return K2STAT_ERROR_BAD_ARGUMENT;

    pFatFs = apRootDir->Hdr.mpParentFs;
    if ((NULL == pFatFs) || (0 == pFatFs->mFatType))
    {
        return K2STAT_ERROR_NOT_OPEN;
    }

    pFatFs->mpOps->DiskSync(pFatFs->mpOps);

    pFatFs->mpOps->MutexDelete(pFatFs->mpOps, pFatFs->mMutex);

    pFatFs->mMutex = 0;

    pFatFs->mFatType = 0;

    return K2STAT_NO_ERROR;
}

//
// ----------------------------------------------------------------------------------
//

K2STAT K2FATFS_open(K2FATFS * apFatFs, K2FATFS_OBJ_DIR const *apDir, const char * apPath, UINT8 aMode, K2FATFS_OBJ_FILE * apFileObj)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR dj;
    UINT32 cl, bcs, clst, tm;
    UINT32 sc;
    UINT32 ofs;

    if (!apFileObj)
        return K2STAT_ERROR_CORRUPTED;

    K2MEM_Copy(&dj, apDir, sizeof(dj));

    if (0 != (aMode & (K2FATFS_CREATE_ALWAYS | K2FATFS_CREATE_NEW | K2FATFS_OPEN_APPEND)))
        aMode |= K2FATFS_ACCESS_WRITE;

    aMode &= K2FATFS_ACCESS_READ | K2FATFS_ACCESS_WRITE | K2FATFS_CREATE_ALWAYS | K2FATFS_CREATE_NEW | K2FATFS_OPEN_ALWAYS | K2FATFS_OPEN_APPEND;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, aMode);
    if (K2STAT_NO_ERROR != k2stat)
        return k2stat;

    dj.Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(&dj, apPath);    /* Follow the file path */
    if (k2stat == K2STAT_NO_ERROR)
    {
        if (dj.mSfn[NSFLAG] & NS_NONAME)
        {    /* Origin directory itself? */
            k2stat = K2STAT_ERROR_BAD_NAME;
        }
    }

    if (aMode & (K2FATFS_CREATE_ALWAYS | K2FATFS_OPEN_ALWAYS | K2FATFS_CREATE_NEW))
    {
        if (k2stat != K2STAT_NO_ERROR)
        {                    /* No file, create new */
            if (k2stat == K2STAT_ERROR_NOT_FOUND)
            {        /* There is no file to open, create a new entry */
                k2stat = sK2FATFS_Locked_CreateDirEntry(&dj);
            }
            aMode |= K2FATFS_CREATE_ALWAYS;        /* File is created */
        }
        else
        {                                /* Any object with the same name is already existing */
            if (dj.Hdr.mObjAttr & (K2_FSATTRIB_READONLY | K2_FSATTRIB_DIR))
            {    /* Cannot overwrite it (R/O or K2FATFS_OBJ_DIR) */
                k2stat = K2STAT_ERROR_NOT_ALLOWED;
            }
            else
            {
                if (aMode & K2FATFS_CREATE_NEW) k2stat = K2STAT_ERROR_EXISTS;    /* Cannot create as new file */
            }
        }
        if (k2stat == K2STAT_NO_ERROR && (aMode & K2FATFS_CREATE_ALWAYS))
        {
            {
                tm = apFatFs->mpOps->GetFatTime(apFatFs->mpOps);                    /* Set created time */
                sK2FATFS_StoreToUnalignedLe32(dj.mpRawData + DIR_CrtTime, tm);
                sK2FATFS_StoreToUnalignedLe32(dj.mpRawData + DIR_ModTime, tm);
                cl = sK2FATFS_Locked_GetStartCluster(apFatFs, dj.mpRawData);            /* Get current cluster chain */
                dj.mpRawData[DIR_Attr] = K2_FSATTRIB_ARCHIVE;            /* Reset attribute */
                sK2FATFS_Locked_PutStartCluster(apFatFs, dj.mpRawData, 0);            /* Reset file allocation info */
                sK2FATFS_StoreToUnalignedLe32(dj.mpRawData + DIR_FileSize, 0);
                apFatFs->mWindowIsDirtyFlag = 1;
                if (cl != 0)
                {                        /* Remove the cluster chain if exist */
                    sc = apFatFs->mCurWinSector;
                    k2stat = sK2FATFS_Locked_RemoveObjChain(&dj.Hdr, cl, 0);
                    if (k2stat == K2STAT_NO_ERROR)
                    {
                        k2stat = sK2FATFS_Locked_MoveWindow(apFatFs, sc);
                        apFatFs->mLastAllocCluster = cl - 1;        /* Reuse the cluster hole */
                    }
                }
            }
        }
    }
    else
    {
        if (k2stat == K2STAT_NO_ERROR)
        {
            if (dj.Hdr.mObjAttr & K2_FSATTRIB_DIR)
            {
                k2stat = K2STAT_ERROR_NOT_FOUND;
            }
            else
            {
                if ((aMode & K2FATFS_ACCESS_WRITE) && (dj.Hdr.mObjAttr & K2_FSATTRIB_READONLY))
                {
                    k2stat = K2STAT_ERROR_NOT_ALLOWED;
                }
            }
        }
    }
    if (k2stat == K2STAT_NO_ERROR)
    {
        if (aMode & K2FATFS_CREATE_ALWAYS) 
            aMode |= FA_MODIFIED;    /* Set file change flag if created or overwritten */
        apFileObj->mDirEntrySectorNum = apFatFs->mCurWinSector;            /* Pointer to the directory entry */
        apFileObj->mpRawData = dj.mpRawData;
    }

    if (k2stat == K2STAT_NO_ERROR)
    {
        apFileObj->Hdr.mStartCluster = sK2FATFS_Locked_GetStartCluster(apFatFs, dj.mpRawData);                    /* Get object allocation info */
        apFileObj->Hdr.mObjSize = sK2FATFS_LoadFromUnalignedLe32(dj.mpRawData + DIR_FileSize);
        apFileObj->Hdr.mpParentFs = apFatFs;    /* Validate the file object */
        apFileObj->mStatusFlags = aMode;    /* Set file access mode */
        apFileObj->mErrorCode = 0;        /* Clear error flag */
        apFileObj->mSectorInBuffer = 0;        /* Invalidate current data sector */
        apFileObj->mPointer = 0;        /* Set file pointer top of the file */
        K2MEM_Zero(apFileObj->mpAlignedSectorBuffer, K2FATFS_SECTOR_BYTES);    /* Clear sector buffer */
        if ((aMode & FA_SEEKEND) && apFileObj->Hdr.mObjSize > 0)
        {    /* Seek to end of file if K2FATFS_OPEN_APPEND is specified */
            apFileObj->mPointer = apFileObj->Hdr.mObjSize;            /* Offset to seek */
            bcs = (UINT32)apFatFs->mSectorsPerCluster * K2FATFS_SECTOR_BYTES;    /* Cluster size in byte */
            clst = apFileObj->Hdr.mStartCluster;                /* Follow the cluster chain */
            for (ofs = apFileObj->Hdr.mObjSize; k2stat == K2STAT_NO_ERROR && ofs > bcs; ofs -= bcs)
            {
                clst = sK2FATFS_Locked_GetObjCluster(&apFileObj->Hdr, clst);
                if (clst <= FF_CLUSTER_INT_ERROR) 
                    k2stat = K2STAT_ERROR_UNKNOWN;
                if (clst == FF_CLUSTER_DISK_ERROR) 
                    k2stat = K2STAT_ERROR_HARDWARE;
            }
            apFileObj->mCurrentCluster = clst;
            if (k2stat == K2STAT_NO_ERROR && ofs % K2FATFS_SECTOR_BYTES)
            {    /* Fill sector buffer if not on the sector boundary */
                sc = sK2FATFS_LockDC_ClusterToSector(apFatFs, clst);
                if (sc == 0)
                {
                    k2stat = K2STAT_ERROR_UNKNOWN;
                }
                else
                {
                    apFileObj->mSectorInBuffer = sc + (UINT32)(ofs / K2FATFS_SECTOR_BYTES);
                    if (apFatFs->mpOps->DiskRead(apFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, apFileObj->mSectorInBuffer, 1) != K2STAT_NO_ERROR)
                        k2stat = K2STAT_ERROR_HARDWARE;
                }
            }
        }
    }

    if (k2stat != K2STAT_NO_ERROR)
        apFileObj->Hdr.mpParentFs = 0;    /* Invalidate file object on error */

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_read(K2FATFS_OBJ_FILE * apFileObj, void* apBuffer, UINT32 aBytesToRead, UINT32 * apRetBytesRead)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;
    UINT32 clst;
    UINT32 sect;
    UINT32 remain;
    UINT32 rcnt, cc, csect;
    UINT8 *rbuff = (UINT8*)apBuffer;

    *apRetBytesRead = 0;    /* Clear read byte counter */
    k2stat = sK2FATFS_ValidatObjAndLockVolume(&apFileObj->Hdr, &pFatFs);                /* Check validity of the file object */
    if (K2STAT_NO_ERROR != k2stat)
        return k2stat;

    do
    {
        if (K2STAT_NO_ERROR != (k2stat = (K2STAT)apFileObj->mErrorCode))
            break;

        if (!(apFileObj->mStatusFlags & K2FATFS_ACCESS_READ))
        {
            k2stat = K2STAT_ERROR_NOT_ALLOWED;
            break;
        }

        remain = apFileObj->Hdr.mObjSize - apFileObj->mPointer;
        if (aBytesToRead > remain) 
            aBytesToRead = (UINT32)remain;        /* Truncate aBytesToRead by remaining bytes */

        for (; aBytesToRead > 0; aBytesToRead -= rcnt, *apRetBytesRead += rcnt, rbuff += rcnt, apFileObj->mPointer += rcnt)
        {    /* Repeat until aBytesToRead bytes read */
            if (apFileObj->mPointer % K2FATFS_SECTOR_BYTES == 0)
            {            /* On the sector boundary? */
                csect = (UINT32)(apFileObj->mPointer / K2FATFS_SECTOR_BYTES & (pFatFs->mSectorsPerCluster - 1));    /* Sector offset in the cluster */
                if (csect == 0)
                {                    /* On the cluster boundary? */
                    if (apFileObj->mPointer == 0)
                    {            /* On the top of the file? */
                        clst = apFileObj->Hdr.mStartCluster;        /* Follow cluster chain from the origin */
                    }
                    else
                    {                        /* Middle or end of the file */
                        clst = sK2FATFS_Locked_GetObjCluster(&apFileObj->Hdr, apFileObj->mCurrentCluster);    /* Follow cluster chain on the FAT */
                    }
                    if (clst < FF_CLUSTER_FIRST_VALID)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_UNKNOWN);
                        break;
                    }
                    if (clst == FF_CLUSTER_DISK_ERROR)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                        break;
                    }
                    apFileObj->mCurrentCluster = clst;                /* Update current cluster */
                }
                sect = sK2FATFS_LockDC_ClusterToSector(pFatFs, apFileObj->mCurrentCluster);    /* Get current sector */
                if (sect == 0)
                {
                    apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_UNKNOWN);
                    break;
                }
                sect += csect;
                cc = aBytesToRead / K2FATFS_SECTOR_BYTES;                    /* When remaining bytes >= sector size, */
                if (cc > 0)
                {                        /* Read maximum contiguous sectors directly */
                    if (csect + cc > pFatFs->mSectorsPerCluster)
                    {    /* Clip at cluster boundary */
                        cc = pFatFs->mSectorsPerCluster - csect;
                    }
                    if (pFatFs->mpOps->DiskRead(pFatFs->mpOps, rbuff, sect, cc) != K2STAT_NO_ERROR)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                        break;
                    }
                    if ((apFileObj->mStatusFlags & FA_DIRTY) && apFileObj->mSectorInBuffer - sect < cc)
                    {
                        K2MEM_Copy(rbuff + ((apFileObj->mSectorInBuffer - sect) * K2FATFS_SECTOR_BYTES), apFileObj->mpAlignedSectorBuffer, K2FATFS_SECTOR_BYTES);
                    }
                    rcnt = K2FATFS_SECTOR_BYTES * cc;                /* Number of bytes transferred */
                    continue;
                }
                if (apFileObj->mSectorInBuffer != sect)
                {            /* Load data sector if not in cache */
                    if (apFileObj->mStatusFlags & FA_DIRTY)
                    {        /* Write-back dirty sector cache */
                        if (pFatFs->mpOps->DiskWrite(pFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, apFileObj->mSectorInBuffer, 1) != K2STAT_NO_ERROR)
                        {
                            apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                            break;
                        }
                        apFileObj->mStatusFlags &= (UINT8)~FA_DIRTY;
                    }
                    if (pFatFs->mpOps->DiskRead(pFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, sect, 1) != K2STAT_NO_ERROR)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                        break;
                    }
                }
                apFileObj->mSectorInBuffer = sect;
            }
            rcnt = K2FATFS_SECTOR_BYTES - (UINT32)apFileObj->mPointer % K2FATFS_SECTOR_BYTES;    /* Number of bytes remains in the sector */
            if (rcnt > aBytesToRead) 
                rcnt = aBytesToRead;                    /* Clip it by aBytesToRead if needed */
            K2MEM_Copy(rbuff, apFileObj->mpAlignedSectorBuffer + apFileObj->mPointer % K2FATFS_SECTOR_BYTES, rcnt);    /* Extract partial sector */
        }

    } while (0);

    sK2FATFS_Locked_UnlockVolume(pFatFs);
    return k2stat;
}

K2STAT K2FATFS_write(K2FATFS_OBJ_FILE * apFileObj, const void* apBuffer, UINT32 aBytesToWrite, UINT32 * apRetBytesWritten)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;
    UINT32 clst;
    UINT32 sect;
    UINT32 wcnt, cc, csect;
    const UINT8 *wbuff = (const UINT8*)apBuffer;

    *apRetBytesWritten = 0;    /* Clear write byte counter */

    k2stat = sK2FATFS_ValidatObjAndLockVolume(&apFileObj->Hdr, &pFatFs);                /* Check validity of the file object */
    if (K2STAT_NO_ERROR != k2stat)
        return k2stat;

    do
    {
        if (K2STAT_NO_ERROR != (k2stat = (K2STAT)apFileObj->mErrorCode))
            break;

        if (!(apFileObj->mStatusFlags & K2FATFS_ACCESS_WRITE))
        {
            k2stat = K2STAT_ERROR_NOT_ALLOWED;
            break;
        }

        /* Check fptr wrap-around (file size cannot reach 4 GiB at FAT volume) */
        if ((UINT32)(apFileObj->mPointer + aBytesToWrite) < (UINT32)apFileObj->mPointer)
        {
            aBytesToWrite = (UINT32)(0xFFFFFFFF - (UINT32)apFileObj->mPointer);
        }

        for (; aBytesToWrite > 0; aBytesToWrite -= wcnt, *apRetBytesWritten += wcnt, wbuff += wcnt, apFileObj->mPointer += wcnt, apFileObj->Hdr.mObjSize = (apFileObj->mPointer > apFileObj->Hdr.mObjSize) ? apFileObj->mPointer : apFileObj->Hdr.mObjSize)
        {    /* Repeat until all data written */
            if (apFileObj->mPointer % K2FATFS_SECTOR_BYTES == 0)
            {        /* On the sector boundary? */
                csect = (UINT32)(apFileObj->mPointer / K2FATFS_SECTOR_BYTES) & (pFatFs->mSectorsPerCluster - 1);    /* Sector offset in the cluster */
                if (csect == 0)
                {                /* On the cluster boundary? */
                    if (apFileObj->mPointer == 0)
                    {        /* On the top of the file? */
                        clst = apFileObj->Hdr.mStartCluster;    /* Follow from the origin */
                        if (clst == 0)
                        {        /* If no cluster is allocated, */
                            clst = sK2FATFS_Locked_AddClusterToObjChain(&apFileObj->Hdr, 0);    /* create a new cluster chain */
                        }
                    }
                    else
                    {                    /* On the middle or end of the file */
                        clst = sK2FATFS_Locked_AddClusterToObjChain(&apFileObj->Hdr, apFileObj->mCurrentCluster);    /* Follow or stretch cluster chain on the FAT */
                    }
                    if (clst == FF_CLUSTER_EMPTY) 
                        break;        /* Could not allocate a new cluster (disk full) */
                    if (clst == FF_CLUSTER_INT_ERROR)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_UNKNOWN);
                        break;
                    }
                    if (clst == FF_CLUSTER_DISK_ERROR)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                        break;
                    }
                    apFileObj->mCurrentCluster = clst;            /* Update current cluster */
                    if (apFileObj->Hdr.mStartCluster == 0) 
                        apFileObj->Hdr.mStartCluster = clst;    /* Set start cluster if the first write */
                }
                if (apFileObj->mStatusFlags & FA_DIRTY)
                {       
                    if (pFatFs->mpOps->DiskWrite(pFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, apFileObj->mSectorInBuffer, 1) != K2STAT_NO_ERROR)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                        break;
                    }
                    apFileObj->mStatusFlags &= (UINT8)~FA_DIRTY;
                }
                sect = sK2FATFS_LockDC_ClusterToSector(pFatFs, apFileObj->mCurrentCluster);    /* Get current sector */
                if (sect == 0)
                {
                    apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_UNKNOWN);
                    break;
                }
                sect += csect;
                cc = aBytesToWrite / K2FATFS_SECTOR_BYTES;                /* When remaining bytes >= sector size, */
                if (cc > 0)
                {                    /* Write maximum contiguous sectors directly */
                    if (csect + cc > pFatFs->mSectorsPerCluster)
                    {    /* Clip at cluster boundary */
                        cc = pFatFs->mSectorsPerCluster - csect;
                    }
                    if (pFatFs->mpOps->DiskWrite(pFatFs->mpOps, wbuff, sect, cc) != K2STAT_NO_ERROR)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                        break;
                    }
                    if (apFileObj->mSectorInBuffer - sect < cc)
                    { /* Refill sector cache if it gets invalidated by the direct write */
                        K2MEM_Copy(apFileObj->mpAlignedSectorBuffer, wbuff + ((apFileObj->mSectorInBuffer - sect) * K2FATFS_SECTOR_BYTES), K2FATFS_SECTOR_BYTES);
                        apFileObj->mStatusFlags &= (UINT8)~FA_DIRTY;
                    }
                    wcnt = K2FATFS_SECTOR_BYTES * cc;        /* Number of bytes transferred */
                    continue;
                }
                if (apFileObj->mSectorInBuffer != sect &&         /* Fill sector cache with file data */
                    apFileObj->mPointer < apFileObj->Hdr.mObjSize &&
                    pFatFs->mpOps->DiskRead(pFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, sect, 1) != K2STAT_NO_ERROR)
                {
                    apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                    break;
                }
                apFileObj->mSectorInBuffer = sect;
            }
            wcnt = K2FATFS_SECTOR_BYTES - (UINT32)apFileObj->mPointer % K2FATFS_SECTOR_BYTES;    /* Number of bytes remains in the sector */
            if (wcnt > aBytesToWrite) 
                wcnt = aBytesToWrite;                    /* Clip it by aBytesToWrite if needed */
            K2MEM_Copy(apFileObj->mpAlignedSectorBuffer + apFileObj->mPointer % K2FATFS_SECTOR_BYTES, wbuff, wcnt);    /* Fit data to the sector */
            apFileObj->mStatusFlags |= FA_DIRTY;
        }

        apFileObj->mStatusFlags |= FA_MODIFIED;                /* Set file change flag */

    } while (0);

    sK2FATFS_Locked_UnlockVolume(pFatFs);

    return k2stat;
}

K2STAT K2FATFS_sync(K2FATFS_OBJ_FILE * apFileObj)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;
    UINT32 tm;
    UINT8 *dir;

    k2stat = sK2FATFS_ValidatObjAndLockVolume(&apFileObj->Hdr, &pFatFs);    /* Check validity of the file object */
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    do
    {
        if (apFileObj->mStatusFlags & FA_MODIFIED)
        {    /* Is there any change to the file? */
            if (apFileObj->mStatusFlags & FA_DIRTY)
            {    /* Write-back cached data if needed */
                if (pFatFs->mpOps->DiskWrite(pFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, apFileObj->mSectorInBuffer, 1) != K2STAT_NO_ERROR)
                {
                    k2stat = K2STAT_ERROR_HARDWARE;
                    break;
                }
                apFileObj->mStatusFlags &= (UINT8)~FA_DIRTY;
            }
            /* Update the directory entry */
            tm = pFatFs->mpOps->GetFatTime(pFatFs->mpOps);                /* Modified time */
            {
                k2stat = sK2FATFS_Locked_MoveWindow(pFatFs, apFileObj->mDirEntrySectorNum);
                if (k2stat == K2STAT_NO_ERROR)
                {
                    dir = apFileObj->mpRawData;
                    dir[DIR_Attr] |= K2_FSATTRIB_ARCHIVE;                        /* Set archive attribute to indicate that the file has been changed */
                    sK2FATFS_Locked_PutStartCluster(apFileObj->Hdr.mpParentFs, dir, apFileObj->Hdr.mStartCluster);        /* Update file allocation information  */
                    sK2FATFS_StoreToUnalignedLe32(dir + DIR_FileSize, (UINT32)apFileObj->Hdr.mObjSize);    /* Update file size */
                    sK2FATFS_StoreToUnalignedLe32(dir + DIR_ModTime, tm);                /* Update modified time */
                    sK2FATFS_StoreToUnalignedLe16(dir + DIR_LstAccDate, 0);
                    pFatFs->mWindowIsDirtyFlag = 1;
                    k2stat = sK2FATFS_Locked_Sync(pFatFs);                    /* Restore it to the directory */
                    apFileObj->mStatusFlags &= (UINT8)~FA_MODIFIED;
                }
            }
        }
    } while (0);

    sK2FATFS_Locked_UnlockVolume(pFatFs);

    return k2stat;
}

K2STAT K2FATFS_close(K2FATFS_OBJ_FILE * apFileObj)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;

    k2stat = K2FATFS_sync(apFileObj);                    /* Flush cached data */
    if (k2stat == K2STAT_NO_ERROR)
    {
        k2stat = sK2FATFS_ValidatObjAndLockVolume(&apFileObj->Hdr,&pFatFs);    /* Lock volume */
        if (k2stat == K2STAT_NO_ERROR)
        {
            apFileObj->Hdr.mpParentFs = 0;    /* Invalidate file object */
            sK2FATFS_Locked_UnlockVolume(pFatFs);        /* Unlock volume */
        }
    }
    return k2stat;
}

K2STAT K2FATFS_chdir(K2FATFS *apFatFs, const char * apPath)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR dj;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, 0);
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&dj, sizeof(dj));

    dj.Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(&dj, apPath);        /* Follow the path */
    if (k2stat == K2STAT_NO_ERROR)
    {                    /* Follow completed */
        if (dj.mSfn[NSFLAG] & NS_NONAME)
        {    /* Is it the start directory itself? */
            apFatFs->mCurrentDirStartCluster = dj.Hdr.mStartCluster;
        }
        else
        {
            if (dj.Hdr.mObjAttr & K2_FSATTRIB_DIR)
            {    /* It is a sub-directory */
                {
                    apFatFs->mCurrentDirStartCluster = sK2FATFS_Locked_GetStartCluster(apFatFs, dj.mpRawData);                    /* Sub-directory cluster */
                }
            }
            else
            {
                k2stat = K2STAT_ERROR_NO_PATH;        /* Reached but a file */
            }
        }
    }
    if (k2stat == K2STAT_ERROR_NOT_FOUND) 
        k2stat = K2STAT_ERROR_NO_PATH;

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_getcwd(K2FATFS *apFatFs,char * apBuffer, UINT32 aBufferBytes)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR dj;
    UINT32 i, n;
    UINT32 ccl;
    char *tp = apBuffer;
    K2FATFS_FILEINFO fno;

    apBuffer[0] = 0;    /* Set null string to get current volume */
    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, 0);    /* Get current volume */
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&dj, sizeof(dj));
    K2MEM_Zero(&fno, sizeof(fno));

    do {
        dj.Hdr.mpParentFs = apFatFs;

        /* Follow parent directories and create the path */
        i = aBufferBytes;            /* Bottom of buffer (directory stack base) */
        dj.Hdr.mStartCluster = apFatFs->mCurrentDirStartCluster;                /* Start to follow upper directory from current directory */
        while ((ccl = dj.Hdr.mStartCluster) != 0)
        {    /* Repeat while current directory is a sub-directory */
            k2stat = sK2FATFS_Locked_SetDirIndex(&dj, 1 * SZDIRE);    /* Get parent directory */
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            k2stat = sK2FATFS_Locked_MoveWindow(apFatFs, dj.mCurrentSector);
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            dj.Hdr.mStartCluster = sK2FATFS_Locked_GetStartCluster(apFatFs, dj.mpRawData);    /* Goto parent directory */
            k2stat = sK2FATFS_Locked_SetDirIndex(&dj, 0);
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            do
            {                            /* Find the entry links to the child directory */
                k2stat = sK2FATFS_Locked_AdvanceDir(&dj);
                if (k2stat != K2STAT_NO_ERROR) 
                    break;
                if (ccl == sK2FATFS_Locked_GetStartCluster(apFatFs, dj.mpRawData)) 
                    break;    /* Found the entry */
                k2stat = sK2FATFS_Locked_DirMoveNext(&dj, 0);
            } while (k2stat == K2STAT_NO_ERROR);
            if (k2stat == K2STAT_ERROR_NOT_FOUND) 
                k2stat = K2STAT_ERROR_UNKNOWN;/* It cannot be 'not found'. */
            if (k2stat != K2STAT_NO_ERROR) 
                break;
            sK2FATFS_Locked_GetFileInfo(&dj, &fno);        /* Get the directory name and push it to the buffer */
            for (n = 0; fno.mName[n]; n++);    /* Name length */
            if (i < n + 1)
            {    /* Insufficient space to store the path name? */
                k2stat = K2STAT_ERROR_OUT_OF_MEMORY; 
                break;
            }
            while (n)
            {
                apBuffer[--i] = fno.mName[--n];    /* Stack the name */
            }
            apBuffer[--i] = '/';
        }
        if (k2stat == K2STAT_NO_ERROR)
        {
            if (i == aBufferBytes) 
                apBuffer[--i] = '/';    /* Is it the root-directory? */
            /* Add current directory path */
            if (k2stat == K2STAT_NO_ERROR)
            {
                do
                {    /* Copy stacked path string */
                    *tp++ = apBuffer[i++];
                } while (i < aBufferBytes);
            }
        }
    } while (0);

    *tp = 0;

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_lseek(K2FATFS_OBJ_FILE * apFileObj, UINT32 aOffset)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;
    UINT32 clst, bcs;
    UINT32 nsect;
    UINT32 ifptr;

    k2stat = sK2FATFS_ValidatObjAndLockVolume(&apFileObj->Hdr, &pFatFs);        /* Check validity of the file object */
    if (k2stat == K2STAT_NO_ERROR) 
        k2stat = (K2STAT)apFileObj->mErrorCode;
    if (k2stat != K2STAT_NO_ERROR)
    {
        sK2FATFS_Locked_UnlockVolume(pFatFs);
        return k2stat;
    }

    do
    {
        k2stat = K2STAT_NO_ERROR;

        if (aOffset > apFileObj->Hdr.mObjSize && (!(apFileObj->mStatusFlags & K2FATFS_ACCESS_WRITE)))
        {    /* In read-only mode, clip offset with the file size */
            aOffset = apFileObj->Hdr.mObjSize;
        }
        ifptr = apFileObj->mPointer;
        apFileObj->mPointer = nsect = 0;
        if (aOffset > 0)
        {
            bcs = (UINT32)pFatFs->mSectorsPerCluster * K2FATFS_SECTOR_BYTES;    /* Cluster size (byte) */
            if (ifptr > 0 &&
                (aOffset - 1) / bcs >= (ifptr - 1) / bcs)
            {    /* When seek to same or following cluster, */
                apFileObj->mPointer = (ifptr - 1) & ~(UINT32)(bcs - 1);    /* start from the current cluster */
                aOffset -= apFileObj->mPointer;
                clst = apFileObj->mCurrentCluster;
            }
            else
            {                                    /* When seek to back cluster, */
                clst = apFileObj->Hdr.mStartCluster;                    /* start from the first cluster */
                if (clst == 0)
                {                        /* If no cluster chain, create a new chain */
                    clst = sK2FATFS_Locked_AddClusterToObjChain(&apFileObj->Hdr, 0);
                    if (clst == 1)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_UNKNOWN);
                        break;
                    }
                    if (clst == 0xFFFFFFFF)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                        break;
                    }
                    apFileObj->Hdr.mStartCluster = clst;
                }
                apFileObj->mCurrentCluster = clst;
            }
            if (clst != 0)
            {
                while (aOffset > bcs)
                {                        /* Cluster following loop */
                    aOffset -= bcs; apFileObj->mPointer += bcs;
                    if (apFileObj->mStatusFlags & K2FATFS_ACCESS_WRITE)
                    {            /* Check if in write mode or not */
                        clst = sK2FATFS_Locked_AddClusterToObjChain(&apFileObj->Hdr, clst);    /* Follow chain with forceed stretch */
                        if (clst == 0)
                        {                /* Clip file size in case of disk full */
                            aOffset = 0; 
                            break;
                        }
                    }
                    else
                    {
                        clst = sK2FATFS_Locked_GetObjCluster(&apFileObj->Hdr, clst);    /* Follow cluster chain if not in write mode */
                    }
                    if (clst == FF_CLUSTER_DISK_ERROR)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                        break;
                    }
                    if (clst <= FF_CLUSTER_INT_ERROR || clst >= pFatFs->mFatEntryCount)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_UNKNOWN);
                        break;
                    }
                    apFileObj->mCurrentCluster = clst;
                }
                if (K2STAT_NO_ERROR != k2stat)
                    break;
                apFileObj->mPointer += aOffset;
                if (aOffset % K2FATFS_SECTOR_BYTES)
                {
                    nsect = sK2FATFS_LockDC_ClusterToSector(pFatFs, clst);    /* Current sector */
                    if (nsect == 0)
                    {
                        apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_UNKNOWN);
                        break;
                    }
                    nsect += (UINT32)(aOffset / K2FATFS_SECTOR_BYTES);
                }
            }
        }
        if (apFileObj->mPointer > apFileObj->Hdr.mObjSize)
        {    /* Set file change flag if the file size is extended */
            apFileObj->Hdr.mObjSize = apFileObj->mPointer;
            apFileObj->mStatusFlags |= FA_MODIFIED;
        }
        if (apFileObj->mPointer % K2FATFS_SECTOR_BYTES && nsect != apFileObj->mSectorInBuffer)
        {    /* Fill sector cache if needed */
            if (apFileObj->mStatusFlags & FA_DIRTY)
            {            /* Write-back dirty sector cache */
                if (pFatFs->mpOps->DiskWrite(pFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, apFileObj->mSectorInBuffer, 1) != K2STAT_NO_ERROR)
                {
                    apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                    break;
                }
                apFileObj->mStatusFlags &= (UINT8)~FA_DIRTY;
            }
            if (pFatFs->mpOps->DiskRead(pFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, nsect, 1) != K2STAT_NO_ERROR)
            {
                apFileObj->mErrorCode = (UINT8)(k2stat = K2STAT_ERROR_HARDWARE);
                break;
            }
            apFileObj->mSectorInBuffer = nsect;
        }

    } while (0);

    sK2FATFS_Locked_UnlockVolume(pFatFs);

    return k2stat;
}

K2STAT K2FATFS_opendir(K2FATFS *apFatFs, K2FATFS_OBJ_DIR const * apSrcDirObj, const char* apPath, K2FATFS_OBJ_DIR *apFillDirObj)
{
    K2STAT k2stat;

    if (!apSrcDirObj) 
        return K2STAT_ERROR_CORRUPTED;

    /* Get logical drive */
    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, 0);
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Copy(apFillDirObj, apSrcDirObj, sizeof(K2FATFS_OBJ_DIR));

    // broken
//    apSrcDirObj->Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(apFillDirObj, apPath);            /* Follow the path to the directory */
    if (k2stat == K2STAT_NO_ERROR)
    {                        /* Follow completed */
        if (!(apFillDirObj->mSfn[NSFLAG] & NS_NONAME))
        {    /* It is not the origin directory itself */
            if (apFillDirObj->Hdr.mObjAttr & K2_FSATTRIB_DIR)
            {        /* This object is a sub-directory */
                {
                    apFillDirObj->Hdr.mStartCluster = sK2FATFS_Locked_GetStartCluster(apFatFs, apFillDirObj->mpRawData);    /* Get object allocation info */
                }
            }
            else
            {                        /* This object is a file */
                k2stat = K2STAT_ERROR_NO_PATH;
            }
        }
        if (k2stat == K2STAT_NO_ERROR)
        {
            k2stat = sK2FATFS_Locked_SetDirIndex(apFillDirObj, 0);            /* Rewind directory */
        }
    }
    if (k2stat == K2STAT_ERROR_NOT_FOUND) 
        k2stat = K2STAT_ERROR_NO_PATH;

    if (k2stat != K2STAT_NO_ERROR) 
        apFillDirObj->Hdr.mpParentFs = 0;        /* Invalidate the directory object if function failed */

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_closedir(K2FATFS_OBJ_DIR * apDirObj)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;

    k2stat = sK2FATFS_ValidatObjAndLockVolume(&apDirObj->Hdr,&pFatFs);    /* Check validity of the file object */
    if (k2stat == K2STAT_NO_ERROR)
    {
        apDirObj->Hdr.mpParentFs = 0;    /* Invalidate directory object */
        sK2FATFS_Locked_UnlockVolume(pFatFs);    /* Unlock volume */
    }
    return k2stat;
}

K2STAT K2FATFS_readdir(K2FATFS_OBJ_DIR * apDirObj, K2FATFS_FILEINFO * apFileObjInfo)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;

    k2stat = sK2FATFS_ValidatObjAndLockVolume(&apDirObj->Hdr, &pFatFs);    /* Check validity of the directory object */
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    if (!apFileObjInfo)
    {
        k2stat = sK2FATFS_Locked_SetDirIndex(apDirObj, 0);        /* Rewind the directory object */
    }
    else
    {
        k2stat = sK2FATFS_Locked_AdvanceDir(apDirObj);        /* Read an item */
        if (k2stat == K2STAT_ERROR_NOT_FOUND) 
            k2stat = K2STAT_NO_ERROR;    /* Ignore end of directory */
        if (k2stat == K2STAT_NO_ERROR)
        {                /* A valid entry is found */
            sK2FATFS_Locked_GetFileInfo(apDirObj, apFileObjInfo);        /* Get the object information */
            k2stat = sK2FATFS_Locked_DirMoveNext(apDirObj, 0);        /* Increment index for next */
            if (k2stat == K2STAT_ERROR_NOT_FOUND) 
                k2stat = K2STAT_NO_ERROR;    /* Ignore end of directory now */
        }
    }

    sK2FATFS_Locked_UnlockVolume(pFatFs);

    return k2stat;
}

K2STAT K2FATFS_findnext(K2FATFS_OBJ_DIR * apDirObj, K2FATFS_FILEINFO * apFileObjInfo)
{
    K2STAT k2stat;

    for (;;)
    {
        k2stat = K2FATFS_readdir(apDirObj, apFileObjInfo);        /* Get a directory item */
        if (k2stat != K2STAT_NO_ERROR || !apFileObjInfo || !apFileObjInfo->mName[0]) 
            break;    /* Terminate if any error or end of directory */
        if (sK2FATFS_LockDC_PatternMatch(apDirObj->mpMatchPattern, apFileObjInfo->mName, 0, FIND_RECURS)) 
            break;        /* Test for the file name */
    }
    return k2stat;
}

K2STAT K2FATFS_findfirst(K2FATFS *apFatFs,K2FATFS_OBJ_DIR * apDirObj, K2FATFS_FILEINFO * apFileObjInfo, const char * apPath, const char * apPattern)
{
    K2STAT k2stat;

    apDirObj->mpMatchPattern = apPattern;        /* Save pointer to apPattern string */
    K2_ASSERT(0); //broken
    k2stat = K2FATFS_opendir(apFatFs, apDirObj, apPath, NULL);        /* Open the target directory */
    if (k2stat == K2STAT_NO_ERROR)
    {
        k2stat = K2FATFS_findnext(apDirObj, apFileObjInfo);    /* Find the first item */
    }
    return k2stat;
}

K2STAT K2FATFS_stat(K2FATFS *apFatFs, const char * apPath, K2FATFS_FILEINFO * apFileObjInfo)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR dj;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, 0);
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&dj, sizeof(dj));

    dj.Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(&dj, apPath);    /* Follow the file path */
    if (k2stat == K2STAT_NO_ERROR)
    {                /* Follow completed */
        if (dj.mSfn[NSFLAG] & NS_NONAME)
        {    /* It is origin directory */
            k2stat = K2STAT_ERROR_BAD_NAME;
        }
        else
        {                            /* Found an object */
            if (apFileObjInfo) 
                sK2FATFS_Locked_GetFileInfo(&dj, apFileObjInfo);
        }
    }

    sK2FATFS_Locked_UnlockVolume(dj.Hdr.mpParentFs);

    return k2stat;
}

K2STAT K2FATFS_getfree(K2FATFS *apFatFs, UINT32 * apRetNumClusters)
{
    K2STAT k2stat;
    UINT32 nfree, clst, nextStat;
    UINT32 sect;
    UINT32 i;
    K2FATFS_OBJ_HDR obj;

    /* Get logical drive */
    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, 0);
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&obj, sizeof(obj));

    /* If mFreeClusterCount is valid, return it without full FAT scan */
    if (apFatFs->mFreeClusterCount <= apFatFs->mFatEntryCount - 2)
    {
        *apRetNumClusters = apFatFs->mFreeClusterCount;
    }
    else
    {
        /* Scan FAT to obtain number of free clusters */
        nfree = 0;
        if (apFatFs->mFatType == K2FATFS_FATTYPE_FAT12)
        {    /* FAT12: Scan bit field FAT entries */
            clst = 2; obj.mpParentFs = apFatFs;
            do
            {
                nextStat = sK2FATFS_Locked_GetObjCluster(&obj, clst);
                if (nextStat == FF_CLUSTER_DISK_ERROR)
                {
                    k2stat = K2STAT_ERROR_HARDWARE;
                    break;
                }
                if (nextStat == FF_CLUSTER_INT_ERROR)
                {
                    k2stat = K2STAT_ERROR_UNKNOWN;
                    break;
                }
                if (nextStat == 0) 
                    nfree++;
            } while (++clst < apFatFs->mFatEntryCount);
        }
        else
        {
            {    /* FAT16/32: Scan UINT16/UINT32 FAT entries */
                clst = apFatFs->mFatEntryCount;    /* Number of entries */
                sect = apFatFs->mFatBaseSector;        /* Top of the FAT */
                i = 0;                    /* Offset in the sector */
                do
                {    /* Counts numbuer of entries with zero in the FAT */
                    if (i == 0)
                    {    /* New sector? */
                        k2stat = sK2FATFS_Locked_MoveWindow(apFatFs, sect++);
                        if (k2stat != K2STAT_NO_ERROR)
                            break;
                    }
                    if (apFatFs->mFatType == K2FATFS_FATTYPE_FAT16)
                    {
                        if (sK2FATFS_LoadFromUnalignedLe16(apFatFs->mpAlignedWindow + i) == 0)
                            nfree++;
                        i += 2;
                    }
                    else
                    {
                        if ((sK2FATFS_LoadFromUnalignedLe32(apFatFs->mpAlignedWindow + i) & 0x0FFFFFFF) == 0)
                            nfree++;
                        i += 4;
                    }
                    i %= K2FATFS_SECTOR_BYTES;
                } while (--clst);
            }
        }
        if (k2stat == K2STAT_NO_ERROR)
        {
            *apRetNumClusters = nfree;            /* Return the free clusters */
            apFatFs->mFreeClusterCount = nfree;    /* Now mFreeClusterCount is valid */
            apFatFs->mFsiFlags |= FF_FSIFLAG_DIRTY;        /* FAT32: FSInfo is to be updated */
        }
    }

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_truncate(K2FATFS_OBJ_FILE * apFileObj)
{
    K2STAT k2stat;
    K2FATFS *pFatFs;
    UINT32 ncl;

    k2stat = sK2FATFS_ValidatObjAndLockVolume(&apFileObj->Hdr, &pFatFs);                /* Check validity of the file object */
    if (K2STAT_NO_ERROR != k2stat)
        return k2stat;
    do
    {
        if (K2STAT_NO_ERROR != (k2stat = (K2STAT)apFileObj->mErrorCode))
            break;
        if (!(apFileObj->mStatusFlags & K2FATFS_ACCESS_WRITE))
        {
            k2stat = K2STAT_ERROR_NOT_ALLOWED;
            break;
        }

        if (apFileObj->mPointer < apFileObj->Hdr.mObjSize)
        {    /* Process when fptr is not on the eof */
            if (apFileObj->mPointer == 0)
            {    /* When set file size to zero, remove entire cluster chain */
                k2stat = sK2FATFS_Locked_RemoveObjChain(&apFileObj->Hdr, apFileObj->Hdr.mStartCluster, 0);
                apFileObj->Hdr.mStartCluster = 0;
            }
            else
            {                /* When truncate a part of the file, remove remaining clusters */
                ncl = sK2FATFS_Locked_GetObjCluster(&apFileObj->Hdr, apFileObj->mCurrentCluster);
                k2stat = K2STAT_NO_ERROR;
                if (ncl == FF_CLUSTER_DISK_ERROR) 
                    k2stat = K2STAT_ERROR_HARDWARE;
                if (ncl == FF_CLUSTER_INT_ERROR) 
                    k2stat = K2STAT_ERROR_UNKNOWN;
                if (k2stat == K2STAT_NO_ERROR && ncl < pFatFs->mFatEntryCount)
                {
                    k2stat = sK2FATFS_Locked_RemoveObjChain(&apFileObj->Hdr, ncl, apFileObj->mCurrentCluster);
                }
            }
            apFileObj->Hdr.mObjSize = apFileObj->mPointer;    /* Set file size to current read/write point */
            apFileObj->mStatusFlags |= FA_MODIFIED;
            if (k2stat == K2STAT_NO_ERROR && (apFileObj->mStatusFlags & FA_DIRTY))
            {
                if (pFatFs->mpOps->DiskWrite(pFatFs->mpOps, apFileObj->mpAlignedSectorBuffer, apFileObj->mSectorInBuffer, 1) != K2STAT_NO_ERROR)
                {
                    k2stat = K2STAT_ERROR_HARDWARE;
                }
                else
                {
                    apFileObj->mStatusFlags &= (UINT8)~FA_DIRTY;
                }
            }
            if (k2stat != K2STAT_NO_ERROR)
            {
                apFileObj->mErrorCode = (UINT8)(k2stat);
            }
        }

    } while (0);

    sK2FATFS_Locked_UnlockVolume(pFatFs);

    return k2stat;
}

K2STAT K2FATFS_unlink(K2FATFS *apFatFs, const char * apPath)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR dj, sdj;
    UINT32 dclst = 0;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, K2FATFS_ACCESS_WRITE);
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&dj, sizeof(dj));
    K2MEM_Zero(&sdj, sizeof(sdj));

    dj.Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(&dj, apPath);        /* Follow the file path */
    if (k2stat == K2STAT_NO_ERROR && (dj.mSfn[NSFLAG] & NS_DOT))
    {
        k2stat = K2STAT_ERROR_BAD_NAME;            /* Cannot remove dot entry */
    }
    if (k2stat == K2STAT_NO_ERROR)
    {                    /* The object is accessible */
        if (dj.mSfn[NSFLAG] & NS_NONAME)
        {
            k2stat = K2STAT_ERROR_BAD_NAME;        /* Cannot remove the origin directory */
        }
        else
        {
            if (dj.Hdr.mObjAttr & K2_FSATTRIB_READONLY)
            {
                k2stat = K2STAT_ERROR_NOT_ALLOWED;        /* Cannot remove R/O object */
            }
        }
        if (k2stat == K2STAT_NO_ERROR)
        {
            dclst = sK2FATFS_Locked_GetStartCluster(apFatFs, dj.mpRawData);
            if (dj.Hdr.mObjAttr & K2_FSATTRIB_DIR)
            {            /* Is it a sub-directory? */
                if (dclst == apFatFs->mCurrentDirStartCluster)
                {         /* Is it the current directory? */
                    k2stat = K2STAT_ERROR_NOT_ALLOWED;
                }
                else
                {
                    sdj.Hdr.mpParentFs = apFatFs;            /* Open the sub-directory */
                    sdj.Hdr.mStartCluster = dclst;
                    k2stat = sK2FATFS_Locked_SetDirIndex(&sdj, 0);
                    if (k2stat == K2STAT_NO_ERROR)
                    {
                        k2stat = sK2FATFS_Locked_AdvanceDir(&sdj);            /* Test if the directory is empty */
                        if (k2stat == K2STAT_NO_ERROR) 
                            k2stat = K2STAT_ERROR_NOT_ALLOWED;    /* Not empty? */
                        if (k2stat == K2STAT_ERROR_NOT_FOUND) 
                            k2stat = K2STAT_NO_ERROR;    /* Empty? */
                    }
                }
            }
        }
        if (k2stat == K2STAT_NO_ERROR)
        {
            k2stat = sK2FATFS_Locked_RemoveDirEntry(&dj);            /* Remove the directory entry */
            if (k2stat == K2STAT_NO_ERROR && dclst != 0)
            {    /* Remove the cluster chain if exist */
                k2stat = sK2FATFS_Locked_RemoveObjChain(&dj.Hdr, dclst, 0);
            }
            if (k2stat == K2STAT_NO_ERROR) 
                k2stat = sK2FATFS_Locked_Sync(apFatFs);
        }
    }

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_mkdir(K2FATFS *apFatFs, const char * apPath)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR dj;
    K2FATFS_OBJ_HDR sobj;
    UINT32 dcl, pcl, tm;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, K2FATFS_ACCESS_WRITE);    /* Get logical drive */
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&dj, sizeof(dj));
    K2MEM_Zero(&sobj, sizeof(sobj));

    dj.Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(&dj, apPath);            /* Follow the file path */
    if (k2stat == K2STAT_NO_ERROR)
        k2stat = K2STAT_ERROR_EXISTS;        /* Name collision? */
    if (k2stat == K2STAT_ERROR_NOT_FOUND && (dj.mSfn[NSFLAG] & NS_DOT))
    {    /* Invalid name? */
        k2stat = K2STAT_ERROR_BAD_NAME;
    }
    if (k2stat == K2STAT_ERROR_NOT_FOUND)
    {                /* It is clear to create a new directory */
        sobj.mpParentFs = apFatFs;                        /* New object id to create a new chain */
        dcl = sK2FATFS_Locked_AddClusterToObjChain(&sobj, 0);        /* Allocate a cluster for the new directory */
        k2stat = K2STAT_NO_ERROR;
        if (dcl == 0)
            k2stat = K2STAT_ERROR_NOT_ALLOWED;        /* No space to allocate a new cluster? */
        if (dcl == 1)
            k2stat = K2STAT_ERROR_UNKNOWN;        /* Any insanity? */
        if (dcl == 0xFFFFFFFF)
            k2stat = K2STAT_ERROR_HARDWARE;    /* Disk error? */
        tm = apFatFs->mpOps->GetFatTime(apFatFs->mpOps);
        if (k2stat == K2STAT_NO_ERROR)
        {
            k2stat = sK2FATFS_Locked_ClearDir(apFatFs, dcl);        /* Clean up the new table */
            if (k2stat == K2STAT_NO_ERROR)
            {
                K2MEM_Set(apFatFs->mpAlignedWindow + DIR_Name, ' ', 11);    /* Create "." entry */
                apFatFs->mpAlignedWindow[DIR_Name] = '.';
                apFatFs->mpAlignedWindow[DIR_Attr] = K2_FSATTRIB_DIR;
                sK2FATFS_StoreToUnalignedLe32(apFatFs->mpAlignedWindow + DIR_ModTime, tm);
                sK2FATFS_Locked_PutStartCluster(apFatFs, apFatFs->mpAlignedWindow, dcl);
                K2MEM_Copy(apFatFs->mpAlignedWindow + SZDIRE, apFatFs->mpAlignedWindow, SZDIRE);    /* Create ".." entry */
                apFatFs->mpAlignedWindow[SZDIRE + 1] = '.'; pcl = dj.Hdr.mStartCluster;
                sK2FATFS_Locked_PutStartCluster(apFatFs, apFatFs->mpAlignedWindow + SZDIRE, pcl);
                apFatFs->mWindowIsDirtyFlag = 1;
                k2stat = sK2FATFS_Locked_CreateDirEntry(&dj);    /* Register the object to the parent directoy */
            }
        }
        if (k2stat == K2STAT_NO_ERROR)
        {
            sK2FATFS_StoreToUnalignedLe32(dj.mpRawData + DIR_ModTime, tm);    /* Created time */
            sK2FATFS_Locked_PutStartCluster(apFatFs, dj.mpRawData, dcl);            /* Table start cluster */
            dj.mpRawData[DIR_Attr] = K2_FSATTRIB_DIR;            /* Attribute */
            apFatFs->mWindowIsDirtyFlag = 1;
            if (k2stat == K2STAT_NO_ERROR)
            {
                k2stat = sK2FATFS_Locked_Sync(apFatFs);
            }
        }
        else
        {
            sK2FATFS_Locked_RemoveObjChain(&sobj, dcl, 0);        /* Could not register, remove the allocated cluster */
        }
    }

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_rename(K2FATFS *apFatFs, const char * apOldPath, const char * apNewPath)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR djo, djn;
    UINT8 buf[SZDIRE], *dir;
    UINT32 sect;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, K2FATFS_ACCESS_WRITE);    /* Get logical drive of the old object */
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&djo, sizeof(djo));
    K2MEM_Zero(&djn, sizeof(djn));

    djo.Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(&djo, apOldPath);            /* Check old object */
    if (k2stat == K2STAT_NO_ERROR && (djo.mSfn[NSFLAG] & (NS_DOT | NS_NONAME))) 
        k2stat = K2STAT_ERROR_BAD_NAME;    /* Check validity of name */
    if (k2stat == K2STAT_NO_ERROR)
    {                    /* Object to be renamed is found */
        {    /* At FAT/FAT32 volume */
            K2MEM_Copy(buf, djo.mpRawData, SZDIRE);            /* Save directory entry of the object */
            K2MEM_Copy(&djn, &djo, sizeof(K2FATFS_OBJ_DIR));        /* Duplicate the directory object */
            k2stat = sK2FATFS_Locked_FollowPath(&djn, apNewPath);        /* Make sure if new object name is not in use */
            if (k2stat == K2STAT_NO_ERROR)
            {                        /* Is new name already in use by any other object? */
                k2stat = (djn.Hdr.mStartCluster == djo.Hdr.mStartCluster && djn.mPointer == djo.mPointer) ? K2STAT_ERROR_NOT_FOUND : K2STAT_ERROR_EXISTS;
            }
            if (k2stat == K2STAT_ERROR_NOT_FOUND)
            {                 /* It is a valid path and no name collision */
                k2stat = sK2FATFS_Locked_CreateDirEntry(&djn);            /* Register the new entry */
                if (k2stat == K2STAT_NO_ERROR)
                {
                    dir = djn.mpRawData;                    /* Copy directory entry of the object except name */
                    K2MEM_Copy(dir + 13, buf + 13, SZDIRE - 13);
                    dir[DIR_Attr] = buf[DIR_Attr];
                    if (!(dir[DIR_Attr] & K2_FSATTRIB_DIR)) 
                        dir[DIR_Attr] |= K2_FSATTRIB_ARCHIVE;    /* Set archive attribute if it is a file */
                    apFatFs->mWindowIsDirtyFlag = 1;
                    if ((dir[DIR_Attr] & K2_FSATTRIB_DIR) && djo.Hdr.mStartCluster != djn.Hdr.mStartCluster)
                    {    /* Update .. entry in the sub-directory if needed */
                        sect = sK2FATFS_LockDC_ClusterToSector(apFatFs, sK2FATFS_Locked_GetStartCluster(apFatFs, dir));
                        if (sect == 0)
                        {
                            k2stat = K2STAT_ERROR_UNKNOWN;
                        }
                        else
                        {
                            /* Start of critical section where an interruption can cause a cross-link */
                            k2stat = sK2FATFS_Locked_MoveWindow(apFatFs, sect);
                            dir = apFatFs->mpAlignedWindow + SZDIRE * 1;    /* Ptr to .. entry */
                            if (k2stat == K2STAT_NO_ERROR && dir[1] == '.')
                            {
                                sK2FATFS_Locked_PutStartCluster(apFatFs, dir, djn.Hdr.mStartCluster);
                                apFatFs->mWindowIsDirtyFlag = 1;
                            }
                        }
                    }
                }
            }
        }
        if (k2stat == K2STAT_NO_ERROR)
        {
            k2stat = sK2FATFS_Locked_RemoveDirEntry(&djo);        /* Remove old entry */
            if (k2stat == K2STAT_NO_ERROR)
            {
                k2stat = sK2FATFS_Locked_Sync(apFatFs);
            }
        }
        /* End of the critical section */
    }

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_chmod(K2FATFS *apFatFs, const char * apPath, UINT8 aAttrib, UINT8 aMaskAttrib)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR dj;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, K2FATFS_ACCESS_WRITE);    /* Get logical drive */
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&dj, sizeof(dj));

    dj.Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(&dj, apPath);    /* Follow the file path */
    if (k2stat == K2STAT_NO_ERROR && (dj.mSfn[NSFLAG] & (NS_DOT | NS_NONAME))) 
        k2stat = K2STAT_ERROR_BAD_NAME;    /* Check object validity */
    if (k2stat == K2STAT_NO_ERROR)
    {
        aMaskAttrib &= K2_FSATTRIB_READONLY | K2_FSATTRIB_HIDDEN | K2_FSATTRIB_SYSTEM | K2_FSATTRIB_ARCHIVE;    /* Valid attribute aMaskAttrib */
        {
            dj.mpRawData[DIR_Attr] = (aAttrib & aMaskAttrib) | (dj.mpRawData[DIR_Attr] & (UINT8)~aMaskAttrib);    /* Apply attribute change */
            apFatFs->mWindowIsDirtyFlag = 1;
        }
        if (k2stat == K2STAT_NO_ERROR)
        {
            k2stat = sK2FATFS_Locked_Sync(apFatFs);
        }
    }

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_utime(K2FATFS *apFatFs, const char * apPath, const K2FATFS_FILEINFO * apFileObjInfo)
{
    K2STAT k2stat;
    K2FATFS_OBJ_DIR dj;

    k2stat = sK2FATFS_CheckAndLockVolume(apFatFs, K2FATFS_ACCESS_WRITE);    /* Get logical drive */
    if (k2stat != K2STAT_NO_ERROR)
        return k2stat;

    K2MEM_Zero(&dj, sizeof(dj));

    dj.Hdr.mpParentFs = apFatFs;
    k2stat = sK2FATFS_Locked_FollowPath(&dj, apPath);    /* Follow the file path */
    if (k2stat == K2STAT_NO_ERROR && (dj.mSfn[NSFLAG] & (NS_DOT | NS_NONAME))) 
        k2stat = K2STAT_ERROR_BAD_NAME;    /* Check object validity */
    if (k2stat == K2STAT_NO_ERROR)
    {
        sK2FATFS_StoreToUnalignedLe32(dj.mpRawData + DIR_ModTime, (UINT32)apFileObjInfo->mDate16 << 16 | apFileObjInfo->mTime16);
        apFatFs->mWindowIsDirtyFlag = 1;
        if (k2stat == K2STAT_NO_ERROR)
        {
            k2stat = sK2FATFS_Locked_Sync(apFatFs);
        }
    }

    sK2FATFS_Locked_UnlockVolume(apFatFs);

    return k2stat;
}

K2STAT K2FATFS_formatvolume(K2FATFS *apFatFs, const K2FATFS_FORMAT_PARAM * apOptions, void* apWorkBuf, UINT32 aWorkBufLen)
{
    static const UINT16 cst[] = { 1, 4, 16, 64, 256, 512, 0 };    /* Cluster size boundary for FAT volume (4Ks unit) */
    static const UINT16 cst32[] = { 1, 2, 4, 8, 16, 32, 0 };    /* Cluster size boundary for FAT32 volume (128Ks unit) */
    static const K2FATFS_FORMAT_PARAM defopt = { K2FATFS_FORMAT_ANY, 0, 0, 0, 0 };    /* Default parameter */
    UINT8 fsopt, fsty;
    UINT8 *buf;
    UINT16 ss;    /* Sector size */
    UINT32 sz_buf, sz_blk, n_clst, pau, nsect, n, vsn;
    UINT32 sz_vol, b_vol, b_fat, b_data;        /* Size of volume, Base LBA of volume, fat, data */
    UINT32 sect;
    UINT32 sz_rsv, sz_fat, sz_dir, sz_au;    /* Size of reserved, fat, dir, data, cluster */
    UINT32 mFatCount, mRootDirEntCount, i;                    /* Index, Number of FATs and Number of roor dir entries */

    K2_ASSERT(K2FATFS_FATTYPE_NONE == apFatFs->mFatType);

    if ((NULL == apFatFs->mpOps) ||
        (apFatFs->VolInfo.mBlockSizeBytes != K2FATFS_SECTOR_BYTES))
        return K2STAT_ERROR_BAD_ARGUMENT;

    /* Get physical drive parameters (sz_drv, sz_blk and ss) */
    if (!apOptions) 
        apOptions = &defopt;    /* Use default parameter if it is not given */
    sz_blk = apOptions->mDataAreaAlign;
    if (sz_blk == 0) 
        sz_blk = apFatFs->VolInfo.mBlockSizeBytes;
    if (sz_blk == 0 || sz_blk > 0x8000 || (sz_blk & (sz_blk - 1))) 
        sz_blk = 1;    /* Use default if the block size is invalid */
    ss = K2FATFS_SECTOR_BYTES;

    /* Options for FAT sub-type and FAT parameters */
    fsopt = apOptions->mFormatType & K2FATFS_FORMAT_ANY;
    mFatCount = (apOptions->mFatCount >= 1 && apOptions->mFatCount <= 2) ? apOptions->mFatCount : 1;
    mRootDirEntCount = (apOptions->mRootDirEntCount >= 1 && apOptions->mRootDirEntCount <= 32768 && (apOptions->mRootDirEntCount % (ss / SZDIRE)) == 0) ? apOptions->mRootDirEntCount : 512;
    sz_au = (apOptions->mSectorsPerCluster <= 0x1000000 && (apOptions->mSectorsPerCluster & (apOptions->mSectorsPerCluster - 1)) == 0) ? apOptions->mSectorsPerCluster : 0;
    sz_au /= ss;    /* Byte --> Sector */

    /* Get working buffer */
    sz_buf = aWorkBufLen / ss;        /* Size of working buffer [sector] */
    if (sz_buf == 0) 
        return K2STAT_ERROR_OUT_OF_MEMORY;
    buf = (UINT8*)apWorkBuf;        /* Working buffer */
    if (!buf) 
        buf = apFatFs->mpOps->MemAlloc(apFatFs->mpOps, sz_buf * ss);    /* Use heap memory for working buffer */
    if (!buf) 
        return K2STAT_ERROR_OUT_OF_MEMORY;

    /* Determine where the volume to be located (b_vol, sz_vol) */
    b_vol = sz_vol = 0;

    sz_vol = (UINT32)apFatFs->VolInfo.mBlockCount;
    if (sz_vol < 128)
    {
        if (!apWorkBuf)
            apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
        return K2STAT_ERROR_ABORTED;
    }

    /* Now start to create an FAT volume at b_vol and sz_vol */

    do
    {    /* Pre-determine the FAT type */
        if (sz_au > 128) 
            sz_au = 128;    /* Invalid AU for FAT/FAT32? */
        if (fsopt & K2FATFS_FORMAT_FAT32)
        {    /* FAT32 possible? */
            if (!(fsopt & K2FATFS_FORMAT_FAT))
            {    /* no-FAT? */
                fsty = K2FATFS_FATTYPE_FAT32; 
                break;
            }
        }
        if (!(fsopt & K2FATFS_FORMAT_FAT))
        {
            if (!apWorkBuf)
                apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
            return K2STAT_ERROR_BAD_ARGUMENT;
        }


        fsty = K2FATFS_FATTYPE_FAT16;
    } while (0);

    vsn = (UINT32)sz_vol + apFatFs->mpOps->GetFatTime(apFatFs->mpOps);    /* VSN generated from current time and partitiion size */

    {    /* Create an FAT/FAT32 volume */
        do
        {
            pau = sz_au;
            /* Pre-determine number of clusters and FAT sub-type */
            if (fsty == K2FATFS_FATTYPE_FAT32)
            {    /* FAT32 volume */
                if (pau == 0)
                {    /* AU auto-selection */
                    n = (UINT32)sz_vol / 0x20000;    /* Volume size in unit of 128KS */
                    for (i = 0, pau = 1; cst32[i] && cst32[i] <= n; i++, pau <<= 1);    /* Get from table */
                }
                n_clst = (UINT32)sz_vol / pau;    /* Number of clusters */
                sz_fat = (n_clst * 4 + 8 + ss - 1) / ss;    /* FAT size [sector] */
                sz_rsv = 32;    /* Number of reserved sectors */
                sz_dir = 0;        /* No static directory */
                if (n_clst <= MAX_FAT16 || n_clst > MAX_FAT32)
                {
                    if (!apWorkBuf)
                        apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
                    return K2STAT_ERROR_ABORTED;
                }
            }
            else
            {                /* FAT volume */
                if (pau == 0)
                {    /* au auto-selection */
                    n = (UINT32)sz_vol / 0x1000;    /* Volume size in unit of 4KS */
                    for (i = 0, pau = 1; cst[i] && cst[i] <= n; i++, pau <<= 1);    /* Get from table */
                }
                n_clst = (UINT32)sz_vol / pau;
                if (n_clst > MAX_FAT12)
                {
                    n = n_clst * 2 + 4;        /* FAT size [byte] */
                }
                else
                {
                    fsty = K2FATFS_FATTYPE_FAT12;
                    n = (n_clst * 3 + 1) / 2 + 3;    /* FAT size [byte] */
                }
                sz_fat = (n + ss - 1) / ss;        /* FAT size [sector] */
                sz_rsv = 1;                        /* Number of reserved sectors */
                sz_dir = (UINT32)mRootDirEntCount * SZDIRE / ss;    /* Root dir size [sector] */
            }
            b_fat = b_vol + sz_rsv;                        /* FAT base */
            b_data = b_fat + sz_fat * mFatCount + sz_dir;    /* Data base */

            /* Align data area to erase block boundary (for flash memory media) */
            n = (UINT32)(((b_data + sz_blk - 1) & ~(sz_blk - 1)) - b_data);    /* Sectors to next nearest from current data base */
            if (fsty == K2FATFS_FATTYPE_FAT32)
            {        /* FAT32: Move FAT */
                sz_rsv += n; b_fat += n;
            }
            else
            {                    /* FAT: Expand FAT */
                if (n % mFatCount)
                {    /* Adjust fractional error if needed */
                    n--; sz_rsv++; b_fat++;
                }
                sz_fat += n / mFatCount;
            }

            /* Determine number of clusters and final check of validity of the FAT sub-type */
            if (sz_vol < b_data + pau * 16 - b_vol)
            {
                if (!apWorkBuf)
                    apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
                return K2STAT_ERROR_ABORTED;
            }
            n_clst = ((UINT32)sz_vol - sz_rsv - sz_fat * mFatCount - sz_dir) / pau;
            if (fsty == K2FATFS_FATTYPE_FAT32)
            {
                if (n_clst <= MAX_FAT16)
                {    /* Too few clusters for FAT32? */
                    if (sz_au == 0 && (sz_au = pau / 2) != 0) 
                        continue;    /* Adjust cluster size and retry */
                    if (!apWorkBuf)
                        apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
                    return K2STAT_ERROR_ABORTED;
                }
            }
            if (fsty == K2FATFS_FATTYPE_FAT16)
            {
                if (n_clst > MAX_FAT16)
                {    /* Too many clusters for FAT16 */
                    if (sz_au == 0 && (pau * 2) <= 64)
                    {
                        sz_au = pau * 2; 
                        continue;    /* Adjust cluster size and retry */
                    }
                    if ((fsopt & K2FATFS_FORMAT_FAT32))
                    {
                        fsty = K2FATFS_FATTYPE_FAT32; 
                        continue;    /* Switch type to FAT32 and retry */
                    }
                    if (sz_au == 0 && (sz_au = pau * 2) <= 128) 
                        continue;    /* Adjust cluster size and retry */
                    if (!apWorkBuf)
                        apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
                    return K2STAT_ERROR_ABORTED;
                }
                if (n_clst <= MAX_FAT12)
                {    /* Too few clusters for FAT16 */
                    if (sz_au == 0 && (sz_au = pau * 2) <= 128) 
                        continue;    /* Adjust cluster size and retry */
                    if (!apWorkBuf)
                        apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
                    return K2STAT_ERROR_ABORTED;
                }
            }
            if (fsty == K2FATFS_FATTYPE_FAT12 && n_clst > MAX_FAT12)
            {
                if (!apWorkBuf)
                    apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
                return K2STAT_ERROR_ABORTED;
            }

            /* Ok, it is the valid cluster configuration */
            break;
        } while (1);

        /* Create FAT VBR */
        K2MEM_Zero(buf, ss);
        K2MEM_Copy(buf + BS_JmpBoot, "\xEB\xFE\x90" "MSDOS5.0", 11);    /* Boot jump code (x86), OEM name */
        sK2FATFS_StoreToUnalignedLe16(buf + BPB_BytsPerSec, ss);                /* Sector size [byte] */
        buf[BPB_SecPerClus] = (UINT8)pau;                /* Cluster size [sector] */
        sK2FATFS_StoreToUnalignedLe16(buf + BPB_RsvdSecCnt, (UINT16)sz_rsv);    /* Size of reserved area */
        buf[BPB_NumFATs] = (UINT8)mFatCount;                    /* Number of FATs */
        sK2FATFS_StoreToUnalignedLe16(buf + BPB_RootEntCnt, (UINT16)((fsty == K2FATFS_FATTYPE_FAT32) ? 0 : mRootDirEntCount));    /* Number of root directory entries */
        if (sz_vol < 0x10000)
        {
            sK2FATFS_StoreToUnalignedLe16(buf + BPB_TotSec16, (UINT16)sz_vol);    /* Volume size in 16-bit LBA */
        }
        else
        {
            sK2FATFS_StoreToUnalignedLe32(buf + BPB_TotSec32, (UINT32)sz_vol);    /* Volume size in 32-bit LBA */
        }
        buf[BPB_Media] = 0xF8;                            /* Media descriptor byte */
        sK2FATFS_StoreToUnalignedLe16(buf + BPB_SecPerTrk, 63);                /* Number of sectors per track (for int13) */
        sK2FATFS_StoreToUnalignedLe16(buf + BPB_NumHeads, 255);                /* Number of heads (for int13) */
        sK2FATFS_StoreToUnalignedLe32(buf + BPB_HiddSec, (UINT32)b_vol);        /* Volume offset in the physical drive [sector] */
        if (fsty == K2FATFS_FATTYPE_FAT32)
        {
            sK2FATFS_StoreToUnalignedLe32(buf + BS_VolID32, vsn);            /* VSN */
            sK2FATFS_StoreToUnalignedLe32(buf + BPB_FATSz32, sz_fat);        /* FAT size [sector] */
            sK2FATFS_StoreToUnalignedLe32(buf + BPB_RootClus32, 2);            /* Root directory cluster # (2) */
            sK2FATFS_StoreToUnalignedLe16(buf + BPB_FSInfo32, 1);                /* Offset of FSINFO sector (VBR + 1) */
            sK2FATFS_StoreToUnalignedLe16(buf + BPB_BkBootSec32, 6);            /* Offset of backup VBR (VBR + 6) */
            buf[BS_DrvNum32] = 0x80;                    /* Drive number (for int13) */
            buf[BS_BootSig32] = 0x29;                    /* Extended boot signature */
            K2MEM_Copy(buf + BS_VolLab32, "NO NAME    " "FAT32   ", 19);    /* Volume label, FAT signature */
        }
        else
        {
            sK2FATFS_StoreToUnalignedLe32(buf + BS_VolID, vsn);                /* VSN */
            sK2FATFS_StoreToUnalignedLe16(buf + BPB_FATSz16, (UINT16)sz_fat);    /* FAT size [sector] */
            buf[BS_DrvNum] = 0x80;                        /* Drive number (for int13) */
            buf[BS_BootSig] = 0x29;                        /* Extended boot signature */
            K2MEM_Copy(buf + BS_VolLab, "NO NAME    " "FAT     ", 19);    /* Volume label, FAT signature */
        }
        sK2FATFS_StoreToUnalignedLe16(buf + BS_55AA, 0xAA55);                    /* Signature (offset is fixed here regardless of sector size) */
        if (apFatFs->mpOps->DiskWrite(apFatFs->mpOps, buf, b_vol, 1) != K2STAT_NO_ERROR)
        {
            if (!apWorkBuf)
                apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
            return K2STAT_ERROR_HARDWARE;
        }
        /* Create FSINFO record if needed */
        if (fsty == K2FATFS_FATTYPE_FAT32)
        {
            apFatFs->mpOps->DiskWrite(apFatFs->mpOps, buf, b_vol + 6, 1);        /* Write backup VBR (VBR + 6) */
            K2MEM_Zero(buf, ss);
            sK2FATFS_StoreToUnalignedLe32(buf + FSI_LeadSig, 0x41615252);
            sK2FATFS_StoreToUnalignedLe32(buf + FSI_StrucSig, 0x61417272);
            sK2FATFS_StoreToUnalignedLe32(buf + FSI_Free_Count, n_clst - 1);    /* Number of free clusters */
            sK2FATFS_StoreToUnalignedLe32(buf + FSI_Nxt_Free, 2);            /* Last allocated cluster# */
            sK2FATFS_StoreToUnalignedLe16(buf + BS_55AA, 0xAA55);
            apFatFs->mpOps->DiskWrite(apFatFs->mpOps, buf, b_vol + 7, 1);        /* Write backup FSINFO (VBR + 7) */
            apFatFs->mpOps->DiskWrite(apFatFs->mpOps, buf, b_vol + 1, 1);        /* Write original FSINFO (VBR + 1) */
        }

        /* Initialize FAT area */
        K2MEM_Zero(buf, sz_buf * ss);
        sect = b_fat;        /* FAT start sector */
        for (i = 0; i < mFatCount; i++)
        {            /* Initialize FATs each */
            if (fsty == K2FATFS_FATTYPE_FAT32)
            {
                sK2FATFS_StoreToUnalignedLe32(buf + 0, 0xFFFFFFF8);    /* FAT[0] */
                sK2FATFS_StoreToUnalignedLe32(buf + 4, 0xFFFFFFFF);    /* FAT[1] */
                sK2FATFS_StoreToUnalignedLe32(buf + 8, 0x0FFFFFFF);    /* FAT[2] (root directory) */
            }
            else
            {
                sK2FATFS_StoreToUnalignedLe32(buf + 0, (fsty == K2FATFS_FATTYPE_FAT12) ? 0xFFFFF8 : 0xFFFFFFF8);    /* FAT[0] and FAT[1] */
            }
            nsect = sz_fat;        /* Number of FAT sectors */
            do
            {    /* Fill FAT sectors */
                n = (nsect > sz_buf) ? sz_buf : nsect;
                if (apFatFs->mpOps->DiskWrite(apFatFs->mpOps, buf, sect, (UINT32)n) != K2STAT_NO_ERROR)
                {
                    if (!apWorkBuf)
                        apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
                    return K2STAT_ERROR_HARDWARE;
                }
                K2MEM_Zero(buf, ss);    /* Rest of FAT all are cleared */
                sect += n; nsect -= n;
            } while (nsect);
        }

        /* Initialize root directory (fill with zero) */
        nsect = (fsty == K2FATFS_FATTYPE_FAT32) ? pau : sz_dir;    /* Number of root directory sectors */
        do
        {
            n = (nsect > sz_buf) ? sz_buf : nsect;
            if (apFatFs->mpOps->DiskWrite(apFatFs->mpOps, buf, sect, (UINT32)n) != K2STAT_NO_ERROR)
            {
                if (!apWorkBuf)
                    apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
                return K2STAT_ERROR_HARDWARE;
            }
            sect += n; nsect -= n;
        } while (nsect);
    }

    if (K2STAT_NO_ERROR != apFatFs->mpOps->DiskSync(apFatFs->mpOps))
    {
        if (!apWorkBuf)
            apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);
        return K2STAT_ERROR_HARDWARE;
    }

    if (!apWorkBuf)
        apFatFs->mpOps->MemFree(apFatFs->mpOps, buf);

    return K2STAT_NO_ERROR;
}

void K2FATFS_init(K2FATFS *apFatFs, K2FATFS_OPS const *apOps, K2_STORAGE_VOLUME const *apVolInfo)
{
    K2MEM_Zero(apFatFs, sizeof(K2FATFS));
    apFatFs->mpOps = apOps;
    K2MEM_Copy(&apFatFs->VolInfo, apVolInfo, sizeof(K2_STORAGE_VOLUME));
    apFatFs->mpAlignedWindow = (UINT8 *)(((((UINT32)apFatFs->mUnalignedWindow) + (K2FATFS_SECTOR_BYTES - 1)) / K2FATFS_SECTOR_BYTES) * K2FATFS_SECTOR_BYTES);
}

void K2FATFS_initFile(K2FATFS_OBJ_FILE *apFileObj)
{
    apFileObj->mpAlignedSectorBuffer = (UINT8 *)(((((UINT32)apFileObj->mUnalignedSectorBuffer) + (K2FATFS_SECTOR_BYTES - 1)) / K2FATFS_SECTOR_BYTES) * K2FATFS_SECTOR_BYTES);
}

