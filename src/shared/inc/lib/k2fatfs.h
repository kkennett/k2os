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

#ifndef __K2FATFS_H
#define __K2FATFS_H

#include <k2systype.h>

#define K2FATFS_SECTOR_BYTES    512
#define FAT32_LFN_MAX_LENGTH    255
#define FAT32_SFN_MAX_LENGTH    12

#define K2FATFS_FATTYPE_NONE    0
#define K2FATFS_FATTYPE_FAT12   1
#define K2FATFS_FATTYPE_FAT16   2
#define K2FATFS_FATTYPE_FAT32   3

#define K2FATFS_ACCESS_READ     0x01
#define K2FATFS_ACCESS_WRITE    0x02
#define K2FATFS_OPEN_EXISTING   0x00    // file or directory must exist already, is opened according to access request, file pointer set to 0
#define K2FATFS_CREATE_NEW      0x04    // file must not exist and be creatable, is opened R/W, file pointer set to 0
#define K2FATFS_CREATE_ALWAYS   0x08    // file is opened R/W and truncated or created, file pointer set to 0
#define K2FATFS_OPEN_ALWAYS     0x10    // file is opened R/W if it exists. if it does not exist it is created, file pointer set to 0
#define K2FATFS_OPEN_APPEND     0x30    // file must exist already, is opened R/W and file pointer set to the end of the file.

#define K2FATFS_FORMAT_FAT      0x01
#define K2FATFS_FORMAT_FAT32    0x02
#define K2FATFS_FORMAT_ANY      0x03

typedef struct _K2FATFS_OPS K2FATFS_OPS;

typedef K2STAT  (*K2FATFS_pf_DiskStatus)(K2FATFS_OPS const *apOps);
typedef K2STAT  (*K2FATFS_pf_DiskRead)(K2FATFS_OPS const *apOps, UINT8 * apBuffer, UINT32 aStartSector, UINT32 aSectorCount);
typedef K2STAT  (*K2FATFS_pf_DiskWrite)(K2FATFS_OPS const *apOps, const UINT8 * apBuffer, UINT32 aStartSector, UINT32 aSectorCount);
typedef K2STAT  (*K2FATFS_pf_DiskSync)(K2FATFS_OPS const *apOps);
typedef UINT32  (*K2FATFS_pf_GetFatTime)(K2FATFS_OPS const *apOps);
typedef void *  (*K2FATFS_pf_MemAlloc)(K2FATFS_OPS const *apOps, UINT32 aSizeBytes);
typedef void    (*K2FATFS_pf_MemFree)(K2FATFS_OPS const *apOps, void * aPtr);
typedef UINT32  (*K2FATFS_pf_MutexCreate)(K2FATFS_OPS const *apOps);
typedef void    (*K2FATFS_pf_MutexDelete)(K2FATFS_OPS const *apOps, UINT32 aMutex);
typedef K2STAT  (*K2FATFS_pf_MutexTake)(K2FATFS_OPS const *apOps, UINT32 aMutex);
typedef void    (*K2FATFS_pf_MutexGive)(K2FATFS_OPS const *apOps, UINT32 aMutex);

struct _K2FATFS_OPS
{
    K2FATFS_pf_DiskStatus     DiskStatus;
    K2FATFS_pf_DiskRead       DiskRead;
    K2FATFS_pf_DiskWrite      DiskWrite;
    K2FATFS_pf_DiskSync       DiskSync;
    K2FATFS_pf_GetFatTime     GetFatTime;
    K2FATFS_pf_MemAlloc       MemAlloc;
    K2FATFS_pf_MemFree        MemFree;
    K2FATFS_pf_MutexCreate    MutexCreate;
    K2FATFS_pf_MutexDelete    MutexDelete;
    K2FATFS_pf_MutexTake      MutexTake;
    K2FATFS_pf_MutexGive      MutexGive;
};

/* Filesystem object structure (K2FATFS) */
typedef struct _K2FATFS K2FATFS;
struct _K2FATFS
{
    K2FATFS_OPS const *     mpOps;
    K2_STORAGE_VOLUME       VolInfo;
    UINT32                  mMutex;
    UINT8                   mFatType;                   /* Filesystem type (0:not mounted) */
    UINT8                   mFatCount;                  /* Number of FATs (1 or 2) */
    UINT8                   mWindowIsDirtyFlag;         /* win[] status (b0:dirty) */
    UINT8                   mFsiFlags;                  /* FSINFO status (b7:disabled, b0:dirty) */
    UINT16                  mRootDirEntCount;           /* Number of root directory entries (FAT12/16) */
    UINT16                  mSectorsPerCluster;         /* Cluster size [sectors] */
    UINT32                  mLastAllocCluster;          /* Last allocated cluster */
    UINT32                  mFreeClusterCount;          /* Number of free clusters */
    UINT32                  mCurrentDirStartCluster;    /* Current directory start cluster (0:root) */
    UINT32                  mFatEntryCount;             /* Number of FAT entries (number of clusters + 2) */
    UINT32                  mSectorsPerFat;             /* Number of sectors per FAT */
    UINT32                  mFatBaseSector;             /* FAT base sector */
    UINT32                  mRootDirBase;               /* Root directory base sector (FAT12/16) or cluster (FAT32) */
    UINT32                  mDataBaseSector;            /* Data base sector */
    UINT32                  mCurWinSector;              /* Current sector appearing in the win[] */
    UINT16                  mLfnWork[(FAT32_LFN_MAX_LENGTH + 1) * 2];   /* LFN working buffer */
    UINT8 *                 mpAlignedWindow;
    UINT8                   mUnalignedWindow[K2FATFS_SECTOR_BYTES * 2];                             /* Disk access window for Directory, FAT (and file data at tiny cfg) */
};

typedef struct _K2FATFS_FILEINFO K2FATFS_FILEINFO;
struct _K2FATFS_FILEINFO
{
    UINT32      mSizeBytes;                         /* File size */
    UINT16      mDate16;                            /* Modified date */
    UINT16      mTime16;                            /* Modified time */
    UINT32      mAttrib;                            /* File attribute */
    char        mAltName[FAT32_SFN_MAX_LENGTH + 1]; /* Alternative file name */
    char        mName[FAT32_LFN_MAX_LENGTH + 1];    /* Primary file name */
};

typedef struct _K2FATFS_FORMAT_PARAM K2FATFS_FORMAT_PARAM;
struct _K2FATFS_FORMAT_PARAM
{
    UINT16      mFormatType;        /* Format option (K2FATFS_FORMAT_FAT, K2FATFS_FORMAT_FAT32, and FM_SFD) */
    UINT16      mFatCount;          /* Number of FATs */
    UINT32      mDataAreaAlign;     /* Data area alignment (sector) */
    UINT32      mRootDirEntCount;   /* Number of root directory entries */
    UINT32      mSectorsPerCluster; /* Cluster size (byte) */
};

typedef struct _K2FATFS_OBJ_HDR K2FATFS_OBJ_HDR;
struct _K2FATFS_OBJ_HDR
{
    K2FATFS *   mpParentFs;         /* Pointer to the hosting volume of this object */
    UINT32      mObjAttr;           /* Object attribute */
    UINT32      mStartCluster;      /* Object data start cluster (0:no cluster or root directory) */
    UINT32      mObjSize;           /* Object size (valid when mStartCluster != 0) */
};

typedef struct _K2FATFS_OBJ_FILE K2FATFS_OBJ_FILE;
struct _K2FATFS_OBJ_FILE
{
    K2FATFS_OBJ_HDR Hdr;                    /* Object identifier (must be the 1st member to detect invalid object pointer) */
    UINT32          mStatusFlags;           /* File status flags */
    K2STAT          mErrorCode;             /* Abort flag (error code) */
    UINT32          mPointer;               /* File read/write pointer (Zeroed on file open) */
    UINT32          mCurrentCluster;        /* Current cluster of fpter (invalid when fptr is 0) */
    UINT32          mSectorInBuffer;        /* Sector number appearing in buf[] (0:invalid) */
    UINT32          mDirEntrySectorNum;     /* Sector number containing the directory entry */
    UINT8 *         mpRawData;              /* Pointer to the directory entry in the win[]  */
    UINT8 *         mpAlignedSectorBuffer;
    UINT8           mUnalignedSectorBuffer[K2FATFS_SECTOR_BYTES * 2];   /* File private data read/write window */
};

typedef struct _K2FATFS_OBJ_DIRENT K2FATFS_OBJ_DIR;
struct _K2FATFS_OBJ_DIRENT
{
    K2FATFS_OBJ_HDR Hdr;                /* Object identifier */
    UINT32          mPointer;           /* Current read/write offset */
    UINT32          mCurrentCluster;    /* Current cluster */
    UINT32          mCurrentSector;     /* Current sector (0:Read operation has terminated) */
    UINT8 *         mpRawData;          /* Pointer to the directory item in the win[] */
    UINT8           mSfn[12];           /* SFN (in/out) {body[8],ext[3],status[1]} */
    UINT32          mCurBlockOffset;    /* Offset of current entry block being processed (0xFFFFFFFF:Invalid) */
    const char*     mpMatchPattern;     /* Pointer to the name matching pattern */
};

void    K2FATFS_init(K2FATFS *apFatFs, K2FATFS_OPS const *apOps, K2_STORAGE_VOLUME const *apVolInfo);

K2STAT  K2FATFS_formatvolume(K2FATFS *apFatFs, const K2FATFS_FORMAT_PARAM* apOptions, void* apWorkBuf, UINT32 aWorkBufBytes);

K2STAT  K2FATFS_mount(K2FATFS* apFatFs, K2FATFS_OBJ_DIR *apFillRootDir);
K2STAT  K2FATFS_unmount(K2FATFS_OBJ_DIR *apRootDir);

K2STAT  K2FATFS_open(K2FATFS * apFatFs, K2FATFS_OBJ_DIR const *apDir, const char * apPath, UINT8 aMode, K2FATFS_OBJ_FILE * apFillFileObj);
K2STAT  K2FATFS_opendir(K2FATFS *apFatFs, K2FATFS_OBJ_DIR const * apDirObj, const char* apPath, K2FATFS_OBJ_DIR *apFillDirObj);               
K2STAT  K2FATFS_closedir(K2FATFS_OBJ_DIR* apDirObj);                                                  
K2STAT  K2FATFS_readdir(K2FATFS_OBJ_DIR* apDirObj, K2FATFS_FILEINFO* apFileInfo);                       
K2STAT  K2FATFS_findfirst(K2FATFS *apFatFs, K2FATFS_OBJ_DIR* apDirObj, K2FATFS_FILEINFO* apFileInfo, const char* apPath, const char* pattern);  
K2STAT  K2FATFS_findnext(K2FATFS_OBJ_DIR* apDirObj, K2FATFS_FILEINFO* apFileInfo);                          
K2STAT  K2FATFS_mkdir(K2FATFS *apFatFs, const char* apPath);                                       
K2STAT  K2FATFS_unlink(K2FATFS *apFatFs, const char* apPath);                                      
K2STAT  K2FATFS_rename(K2FATFS *apFatFs, const char* path_old, const char* path_new);              
K2STAT  K2FATFS_stat(K2FATFS *apFatFs, const char* apPath, K2FATFS_FILEINFO* apFileInfo);            
K2STAT  K2FATFS_chmod(K2FATFS *apFatFs, const char* apPath, UINT8 attr, UINT8 mask);               
K2STAT  K2FATFS_utime(K2FATFS *apFatFs, const char* apPath, const K2FATFS_FILEINFO* apFileInfo);     
K2STAT  K2FATFS_chdir(K2FATFS *apFatFs, const char* apPath);                                       
K2STAT  K2FATFS_getcwd(K2FATFS *apFatFs, char* buff, UINT32 len);                                  
K2STAT  K2FATFS_getfree(K2FATFS *apFatFs, UINT32 *apRetNumClusters);                               

K2STAT  K2FATFS_close(K2FATFS_OBJ_FILE* apFileObj);
K2STAT  K2FATFS_read(K2FATFS_OBJ_FILE* apFileObj, void* buff, UINT32 btr, UINT32* br);
K2STAT  K2FATFS_write(K2FATFS_OBJ_FILE* apFileObj, const void* buff, UINT32 btw, UINT32* bw);
K2STAT  K2FATFS_lseek(K2FATFS_OBJ_FILE* apFileObj, UINT32 ofs);
K2STAT  K2FATFS_truncate(K2FATFS_OBJ_FILE* apFileObj);
K2STAT  K2FATFS_sync(K2FATFS_OBJ_FILE* apFileObj);
BOOL    K2FATFS_iseof(K2FATFS_OBJ_FILE *apFileObj);
UINT8   K2FATFS_haserror(K2FATFS_OBJ_FILE *apFileObj);
UINT32  K2FATFS_tell(K2FATFS_OBJ_FILE *apFileObj);
UINT32  K2FATFS_size(K2FATFS_OBJ_FILE *apFileObj);

#define K2FATFS_rewind(apFileObj)        K2FATFS_lseek((apFileObj), 0)
#define K2FATFS_rewinddir(apDirObj)      K2FATFS_readdir((apDirObj), 0)
#define K2FATFS_rmdir(apFatFs,apPath) K2FATFS_unlink(apFatFs,apPath)

#endif // __K2FATFS_H
