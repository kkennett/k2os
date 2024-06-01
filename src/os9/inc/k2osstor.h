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
#ifndef __K2OSSTOR_H
#define __K2OSSTOR_H

#include <k2osdev_blockio.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

// {72D3F8AD-D08E-45F0-ACD1-DF42C9F4EF3F}
#define K2OS_IFACE_FSMGR                { 0x72d3f8ad, 0xd08e, 0x45f0, { 0xac, 0xd1, 0xdf, 0x42, 0xc9, 0xf4, 0xef, 0x3f } }

// {DFCE881F-24CE-4EF3-9AEC-1524D07F2C1D}
#define K2OS_IFACE_FILESYS              { 0xdfce881f, 0x24ce, 0x4ef3, { 0x9a, 0xec, 0x15, 0x24, 0xd0, 0x7f, 0x2c, 0x1d } }

//
//------------------------------------------------------------------------
//

// {62525869-523E-41A6-8831-1BC244F5AC78}
#define K2OS_IFACE_STORAGE_VOLMGR       { 0x62525869, 0x523e, 0x41a6, { 0x88, 0x31, 0x1b, 0xc2, 0x44, 0xf5, 0xac, 0x78 } }

// {3EA53843-5D12-4D13-A24B-9DE87D575A78}
#define K2OS_IFACE_STORAGE_VOLUME       { 0x3ea53843, 0x5d12, 0x4d13, { 0xa2, 0x4b, 0x9d, 0xe8, 0x7d, 0x57, 0x5a, 0x78 } }

//
//------------------------------------------------------------------------
//

    // {0D287A2A-9032-47CE-A072-FF4191158E87}
#define K2OS_IFACE_STORAGE_PARTITION    { 0xd287a2a, 0x9032, 0x47ce, { 0xa0, 0x72, 0xff, 0x41, 0x91, 0x15, 0x8e, 0x87 } }

// {E088781B-D814-459A-8B44-131A4EF0DDB3}
#define K2OS_GPT_PARTITION_OBJECT_CLASSID       { 0xe088781b, 0xd814, 0x459a, { 0x8b, 0x44, 0x13, 0x1a, 0x4e, 0xf0, 0xdd, 0xb3 } }

// {1B207783-814C-4741-B142-B3828CA2CD8A}
#define K2OS_MBR_PARTITION_OBJECT_CLASSID       { 0x1b207783, 0x814c, 0x4741, { 0xb1, 0x42, 0xb3, 0x82, 0x8c, 0xa2, 0xcd, 0x8a } }


//
//------------------------------------------------------------------------
//

typedef struct _K2OS_STORAGE_PARTITION  K2OS_STORAGE_PARTITION;
struct _K2OS_STORAGE_PARTITION
{
    K2_GUID128  mTypeGuid;
    K2_GUID128  mIdGuid;
    UINT64      mAttributes;
    UINT64      mStartBlock;
    UINT64      mBlockCount;
    UINT32      mBlockSizeBytes;
    UINT32      mUserContext;
    UINT8       mPartTypeByte;
    UINT8       mFlagReadOnly;
    UINT8       mFlagActive;
    UINT8       mFlagEFI;
};

#define K2OS_STORAGE_PARTITION_METHOD_GET_MEDIA 1

typedef struct _K2OS_STORAGE_PARTITION_GET_MEDIA_OUT K2OS_STORAGE_PARTITION_GET_MEDIA_OUT;
struct _K2OS_STORAGE_PARTITION_GET_MEDIA_OUT
{
    K2OS_STORAGE_MEDIA  Media;
    BOOL                mIsGpt;
    UINT32              mPartitionCount;
    K2OS_IFINST_ID      mDevIfInstId;
};

#define K2OS_STORAGE_PARTITION_METHOD_GET_INFO  2

typedef struct _K2OS_STORAGE_PARTITION_GET_INFO_OUT K2OS_STORAGE_PARTITION_GET_INFO_OUT;
struct _K2OS_STORAGE_PARTITION_GET_INFO_OUT
{
    K2OS_STORAGE_PARTITION  Info;
    UINT32                  mIndex;
    UINT32                  mTableIndex;
    K2OS_IFINST_ID          mDevIfInstId;
};

//
//------------------------------------------------------------------------
//

typedef struct _K2OSSTOR_VOLUME_OPAQUE K2OSSTOR_VOLUME_OPAQUE;
typedef K2OSSTOR_VOLUME_OPAQUE *       K2OSSTOR_VOLUME;

typedef struct _K2OSSTOR_VOLUME_PART K2OSSTOR_VOLUME_PART;
struct _K2OSSTOR_VOLUME_PART
{
    K2OS_IFINST_ID  mPartitionIfInstId;
    UINT64          mOffset;
};

K2OSSTOR_VOLUME K2OS_Vol_Attach(K2OS_IFINST_ID aVolIfInstId, UINT32 aAccess, UINT32 aShare, K2OS_MAILBOX_TOKEN aTokNotifyMailbox);
K2OSSTOR_VOLUME K2OS_Vol_Create(K2_GUID128 const *apVolumeId, UINT32 aShare, K2OS_IFINST_ID *apRetIfInstId, K2OS_MAILBOX_TOKEN aTokNotifyMailbox);
BOOL            K2OS_Vol_GetInfo(K2OSSTOR_VOLUME aStorVol, K2OS_STORAGE_VOLUME *apRetVolumeInfo);
BOOL            K2OS_Vol_AddPartition(K2OSSTOR_VOLUME aStorVol, K2OSSTOR_VOLUME_PART const *apVolPart);
BOOL            K2OS_Vol_RemovePartition(K2OSSTOR_VOLUME aStorVol, K2OS_IFINST_ID aPartitionIfInstId);
BOOL            K2OS_Vol_GetPartition(K2OSSTOR_VOLUME aStorVol, UINT32 aIxPart, K2OSSTOR_VOLUME_PART *apRetPart);
BOOL            K2OS_Vol_Make(K2OSSTOR_VOLUME aStorVol);
K2STAT          K2OS_Vol_GetState(K2OSSTOR_VOLUME aStorVol, BOOL *apRetState);
BOOL            K2OS_Vol_Break(K2OSSTOR_VOLUME aStorVol);
BOOL            K2OS_Vol_Read(K2OSSTOR_VOLUME aStorVol, UINT64 const *apBytesOffset, void *apBuffer, UINT32 aByteCount);
BOOL            K2OS_Vol_Write(K2OSSTOR_VOLUME aStorVol, UINT64 const *apBytesOffset, void const *apBuffer, UINT32 aByteCount);
BOOL            K2OS_Vol_Detach(K2OSSTOR_VOLUME aStorVol);

//
//------------------------------------------------------------------------
//

#define K2OS_STORVOLMGR_METHOD_CREATE       1

typedef struct _K2OS_STORVOLMGR_CREATE_IN K2OS_STORVOLMGR_CREATE_IN;
struct _K2OS_STORVOLMGR_CREATE_IN
{
    K2_GUID128      mId;
    UINT32          mShare;
};

typedef struct _K2OS_STORVOLMGR_CREATE_OUT K2OS_STORVOLMGR_CREATE_OUT;
struct _K2OS_STORVOLMGR_CREATE_OUT
{
    K2OS_IFINST_ID  mIfInstId;
};

//
//------------------------------------------------------------------------
//

#define K2OS_STORVOL_METHOD_CONFIG          1
typedef struct _K2OS_STORVOL_CONFIG_IN K2OS_STORVOL_CONFIG_IN;
struct _K2OS_STORVOL_CONFIG_IN
{
    UINT32  mAccess;
    UINT32  mShare;
};

#define K2OS_STORVOL_METHOD_GETINFO         2

#define K2OS_STORVOL_METHOD_ADDPART         3

#define K2OS_STORVOL_METHOD_REMPART         4
typedef struct _K2OS_STORVOL_REMPART_IN K2OS_STORVOL_REMPART_IN;
struct _K2OS_STORVOL_REMPART_IN
{
    K2OS_IFINST_ID  mPartIfInstId;
};

#define K2OS_STORVOL_METHOD_GETPART         5
typedef struct _K2OS_STORVOL_GETPART_IN K2OS_STORVOL_GETPART_IN;
struct _K2OS_STORVOL_GETPART_IN
{
    UINT32  mIxPart;
};

#define K2OS_STORVOL_METHOD_MAKE            6

#define K2OS_STORVOL_METHOD_GETSTATE        7
typedef struct _K2OS_STORVOL_GETSTATE_OUT K2OS_STORVOL_GETSTATE_OUT;
struct _K2OS_STORVOL_GETSTATE_OUT
{
    BOOL    mIsMade;
};

#define K2OS_STORVOL_METHOD_BREAK           8

#define K2OS_STORVOL_METHOD_TRANSFER        9
typedef struct _K2OS_STORVOL_TRANSFER_IN K2OS_STORVOL_TRANSFER_IN;
struct _K2OS_STORVOL_TRANSFER_IN
{
    UINT32  mMemAddr;
    UINT32  mByteCount;
    UINT64  mBytesOffset;
    BOOL    mIsWrite;
};

//
//------------------------------------------------------------------------
//

typedef K2STAT(*K2OS_pf_Ifs_Start)(K2OS_IFINST_ID aVolIfInstId, K2OS_STORAGE_VOLUME const *apVolInfo, UINT8 const *apSector0);
typedef K2STAT(*K2OS_pf_Ifs_Stop)(K2OS_IFINST_ID aVolIfInstId);

//
//------------------------------------------------------------------------
//

typedef struct _K2OSSTOR_FILESYS_OPAQUE K2OSSTOR_FILESYS_OPAQUE;
typedef K2OSSTOR_FILESYS_OPAQUE *       K2OSSTOR_FILESYS;

BOOL K2OS_FsMgr_FormatVolume(K2OS_IFINST_ID aVolIfInstId, K2_GUID128 const *apFsGuid, K2_GUID128 const *apUniqueId, UINT32 aParam1, UINT32 aParam2);
BOOL K2OS_FsMgr_CleanVolume(K2OS_IFINST_ID aVolIfInstId);
BOOL K2OS_FsMgr_Mount(K2OSSTOR_FILESYS aFileSys, char const *apPath);
BOOL K2OS_FsMgr_Dismount(char const *apPath);

#define K2OS_FSMGR_METHOD_FORMAT_VOLUME     1
typedef struct _K2OS_FSMGR_FORMAT_VOLUME_IN K2OS_FSMGR_FORMAT_VOLUME_IN;
struct _K2OS_FSMGR_FORMAT_VOLUME_IN
{
    K2OS_IFINST_ID  mVolIfInstId;
    K2_GUID128      mFsGuid;
    K2_GUID128      mUniqueId;
    UINT32          mParam1;
    UINT32          mParam2;
};

#define K2OS_FSMGR_METHOD_CLEAN_VOLUME      2
typedef struct _K2OS_FSMGR_CLEAN_VOLUME_IN K2OS_FSMGR_CLEAN_VOLUME_IN;
struct _K2OS_FSMGR_CLEAN_VOLUME_IN
{
    K2OS_IFINST_ID  mVolIfInstId;
};

#define K2OS_FSMGR_METHOD_MOUNT             3
typedef struct _K2OS_FSMGR_MOUNT_IN K2OS_FSMGR_MOUNT_IN;
struct _K2OS_FSMGR_MOUNT_IN
{
    K2OSSTOR_FILESYS    mFileSys;
    UINT32              mPathLen;
    char                mPath[4];
};

#define K2OS_FSMGR_METHOD_DISMOUNT          4
typedef struct _K2OS_FSMGR_MOUNT_IN K2OS_FSMGR_DISMOUNT_IN;
struct _K2OS_FSMGR_DISMOUNT_IN
{
    char mPath[4];
};

//
//------------------------------------------------------------------------
//

typedef struct _K2OS_FSFOLDER_OPAQUE    K2OS_FSFOLDER_OPAQUE;
typedef K2OS_FSFOLDER_OPAQUE *          K2OS_FSFOLDER;

K2OSSTOR_FILESYS    K2OS_FileSys_Attach(K2OS_IFINST_ID aIfInstId);
K2OS_FSFOLDER       K2OS_FileSys_OpenRoot(K2OSSTOR_FILESYS aFileSys);
BOOL                K2OS_FileSys_Detach(K2OSSTOR_FILESYS aFileSys);


#define K2OS_FILESYS_METHOD_OPEN_ROOT       100
typedef struct _K2OS_FILESYS_OPENROOT_OUT K2OS_FILESYS_OPENROOT_OUT;
struct _K2OS_FILESYS_OPENROOT_OUT
{
    K2OS_FSFOLDER   mFsFolder;
};

//
//------------------------------------------------------------------------
//

typedef struct _K2OS_FSENUM_OPAQUE  K2OS_FSENUM_OPAQUE;
typedef K2OS_FSENUM_OPAQUE *        K2OS_FSENUM;

typedef struct _K2OS_FILE_OPAQUE    K2OS_FILE_OPAQUE;
typedef K2OS_FILE_OPAQUE *          K2OS_FILE;

typedef enum _K2OS_FilePosRelType K2OS_FilePosRelType;
enum _K2OS_FilePosRelType
{
    K2OS_FilePosRel_Begin = 0,
    K2OS_FilePosRel_End,
    K2OS_FilePosRel_Current,

    K2OS_FilePosRelType_Count
};

K2OS_FSFOLDER   K2OS_FsFolder_Open(K2OS_FSFOLDER aParentFolder, char const *apSubPath, UINT32 aAccess, UINT32 aShare);
BOOL            K2OS_FsFolder_Create(K2OS_FSFOLDER aParentFolder, char const *apFolderName, UINT32 aAccess, UINT32 aShare);
BOOL            K2OS_FsFolder_Delete(K2OS_FSFOLDER aParentFolder, char const *apFolderName);
K2OS_FILE       K2OS_FsFolder_OpenFile(K2OS_FSFOLDER aParentFolder, char const *apSubPath, UINT32 aAccess, UINT32 aShare);
K2OS_FILE       K2OS_FsFolder_CreateFile(K2OS_FSFOLDER aParentFolder, char const *apFileName, UINT32 aAccess, UINT32 aShare, UINT32 aFileAttrib, BOOL aFailIfExists);
BOOL            K2OS_FsFolder_DeleteFile(K2OS_FSFOLDER aParentFolder, char const *apFileName);
BOOL            K2OS_FsFolder_GetFileInfo(K2OS_FSFOLDER aParentFolder, char const *apFileName, K2OS_FILE_INFO *apRetFileInfo);
K2OS_FSFOLDER   K2OS_FsFolder_OpenParentFolder(K2OS_FSFOLDER aParentFolder);
BOOL            K2OS_FsFolder_Close(K2OS_FSFOLDER aFolder);
K2OS_FSENUM     K2OS_FsFolder_CreateEnum(K2OS_FSFOLDER aParentFolder, char const *apPathSpec);

BOOL            K2OS_FsEnum_First(K2OS_FSENUM aFsEnum, K2OS_FILE_INFO *apRetFileInfo);
BOOL            K2OS_FsEnum_Next(K2OS_FSENUM aFsEnum, K2OS_FILE_INFO *apRetFileInfo);
BOOL            K2OS_FsEnum_Delete(K2OS_FSENUM aFsEnum);

K2OS_FSFOLDER   K2OS_File_OpenParentFolder(K2OS_FILE aFile);
K2OS_FSENUM     K2OS_File_CreateEnum(char const *apPathSpec);
K2OS_FILE       K2OS_File_Create(char const *apPath, UINT32 aAccess, UINT32 aShare, UINT32 aFileAttrib, BOOL aFailIfExists);
K2OS_FILE       K2OS_File_Open(char const *apPath, UINT32 aAccess, UINT32 aShare);
BOOL            K2OS_File_GetInfo(char const *apPath, K2OS_FILE_INFO *apRetInfo);
BOOL            K2OS_File_GetPos(K2OS_FILE aFile, UINT64 *apRetBytePos);
BOOL            K2OS_File_SetPos(K2OS_FILE aFile, K2OS_FilePosRelType aFilePosRel, UINT64 const *apSetPos);
UINT32          K2OS_File_Read(K2OS_FILE aFile, void *apBuffer, UINT32 aByteCount);
UINT32          K2OS_File_Write(K2OS_FILE aFile, void const *apBuffer, UINT32 aByteCount);
BOOL            K2OS_File_Close(K2OS_FILE aFile);
BOOL            K2OS_File_Delete(char const *apPath);

//
//------------------------------------------------------------------------
//


#if __cplusplus
}f
#endif

#endif // __K2OSSTOR_H