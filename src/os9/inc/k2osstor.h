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

// {28564880-106B-4D3A-9566-CDA8619CAF82}
#define K2OS_RAMDISK_GUID   { 0x28564880, 0x106b, 0x4d3a, { 0x95, 0x66, 0xcd, 0xa8, 0x61, 0x9c, 0xaf, 0x82 } }

//
//------------------------------------------------------------------------
//

// {0D287A2A-9032-47CE-A072-FF4191158E87}
#define K2OS_IFACE_STORAGE_PARTITION    { 0xd287a2a, 0x9032, 0x47ce, { 0xa0, 0x72, 0xff, 0x41, 0x91, 0x15, 0x8e, 0x87 } }

typedef enum _K2OS_StorePart_Method K2OS_StorePart_Method;
enum _K2OS_StorePart_Method
{
    K2OS_StorePart_Method_Invalid = 0,

    K2OS_StorePart_Method_GetMedia,
    K2OS_StorePart_Method_GetInfo,

    K2OS_StorePart_Method_Count
};

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

typedef struct _K2OS_STORAGE_PARTITION_GET_MEDIA_OUT K2OS_STORAGE_PARTITION_GET_MEDIA_OUT;
struct _K2OS_STORAGE_PARTITION_GET_MEDIA_OUT
{
    K2OS_STORAGE_MEDIA  Media;
    BOOL                mIsGpt;
    UINT32              mPartitionCount;
    K2OS_IFINST_ID      mDevIfInstId;
};

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

// {62525869-523E-41A6-8831-1BC244F5AC78}
#define K2OS_IFACE_STORAGE_VOLMGR       { 0x62525869, 0x523e, 0x41a6, { 0x88, 0x31, 0x1b, 0xc2, 0x44, 0xf5, 0xac, 0x78 } }

// {3EA53843-5D12-4D13-A24B-9DE87D575A78}
#define K2OS_IFACE_STORAGE_VOLUME       { 0x3ea53843, 0x5d12, 0x4d13, { 0xa2, 0x4b, 0x9d, 0xe8, 0x7d, 0x57, 0x5a, 0x78 } }

typedef struct _K2OSSTOR_VOLUME_OPAQUE K2OSSTOR_VOLUME_OPAQUE;
typedef K2OSSTOR_VOLUME_OPAQUE *       K2OSSTOR_VOLUME;

typedef enum _K2OS_StoreVolMgr_Method K2OS_StoreVolMgr_Method;
enum _K2OS_StoreVolMgr_Method
{
    K2OS_StoreVolMgr_Method_Invalid = 0,
    
    K2OS_StoreVolMgr_Method_Create,

    K2OS_StoreVolMgr_Method_Count
};

typedef enum _K2OS_StoreVol_Method K2OS_StoreVol_Method;
enum _K2OS_StoreVol_Method
{
    K2OS_StoreVol_Method_Invalid = 0,

    K2OS_StoreVol_Method_Config,
    K2OS_StoreVol_Method_GetInfo,
    K2OS_StoreVol_Method_AddPart,
    K2OS_StoreVol_Method_RemPart,
    K2OS_StoreVol_Method_GetPart,
    K2OS_StoreVol_Method_Make,
    K2OS_StoreVol_Method_GetState,
    K2OS_StoreVol_Method_Break,
    K2OS_StoreVol_Method_Transfer,

    K2OS_StoreVol_Method_Count
};

typedef struct _K2OSSTOR_VOLUME_PART K2OSSTOR_VOLUME_PART;
struct _K2OSSTOR_VOLUME_PART
{
    K2OS_IFINST_ID  mBlockIoDevIfInstId;
    K2OS_IFINST_ID  mPartitionIfInstId;
    UINT64          mVolumeOffset;
};

K2OSSTOR_VOLUME K2OS_Vol_Attach(K2OS_IFINST_ID aVolIfInstId, UINT32 aAccess, UINT32 aShare, K2OS_MAILBOX_TOKEN aTokNotifyMailbox);
K2OSSTOR_VOLUME K2OS_Vol_Create(K2_GUID128 const *apVolumeId, UINT32 aShare, K2OS_IFINST_ID *apRetIfInstId, K2OS_MAILBOX_TOKEN aTokNotifyMailbox);
BOOL            K2OS_Vol_GetInfo(K2OSSTOR_VOLUME aStorVol, K2_STORAGE_VOLUME *apRetVolumeInfo);
BOOL            K2OS_Vol_AddPartition(K2OSSTOR_VOLUME aStorVol, K2OSSTOR_VOLUME_PART const *apVolPart);
BOOL            K2OS_Vol_RemovePartition(K2OSSTOR_VOLUME aStorVol, K2OS_IFINST_ID aPartitionIfInstId);
BOOL            K2OS_Vol_GetPartition(K2OSSTOR_VOLUME aStorVol, UINT32 aIxPart, K2OSSTOR_VOLUME_PART *apRetPart);
BOOL            K2OS_Vol_Make(K2OSSTOR_VOLUME aStorVol);
K2STAT          K2OS_Vol_GetState(K2OSSTOR_VOLUME aStorVol, BOOL *apRetState);
BOOL            K2OS_Vol_Break(K2OSSTOR_VOLUME aStorVol);
BOOL            K2OS_Vol_Read(K2OSSTOR_VOLUME aStorVol, UINT64 const *apBytesOffset, void *apBuffer, UINT32 aByteCount);
BOOL            K2OS_Vol_Write(K2OSSTOR_VOLUME aStorVol, UINT64 const *apBytesOffset, void const *apBuffer, UINT32 aByteCount);
BOOL            K2OS_Vol_Detach(K2OSSTOR_VOLUME aStorVol);

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

typedef struct _K2OS_STORVOL_CONFIG_IN K2OS_STORVOL_CONFIG_IN;
struct _K2OS_STORVOL_CONFIG_IN
{
    UINT32  mAccess;
    UINT32  mShare;
};

typedef struct _K2OS_STORVOL_REMPART_IN K2OS_STORVOL_REMPART_IN;
struct _K2OS_STORVOL_REMPART_IN
{
    K2OS_IFINST_ID  mPartIfInstId;
};

typedef struct _K2OS_STORVOL_GETPART_IN K2OS_STORVOL_GETPART_IN;
struct _K2OS_STORVOL_GETPART_IN
{
    UINT32  mIxPart;
};

typedef struct _K2OS_STORVOL_GETSTATE_OUT K2OS_STORVOL_GETSTATE_OUT;
struct _K2OS_STORVOL_GETSTATE_OUT
{
    BOOL    mIsMade;
};

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

// {72D3F8AD-D08E-45F0-ACD1-DF42C9F4EF3F}
#define K2OS_IFACE_FSMGR                { 0x72d3f8ad, 0xd08e, 0x45f0, { 0xac, 0xd1, 0xdf, 0x42, 0xc9, 0xf4, 0xef, 0x3f } }

typedef enum _K2OS_FsMgr_Method K2OS_FsMgr_Method;
enum _K2OS_FsMgr_Method
{
    K2OS_FsMgr_Method_Invalid = 0,

    K2OS_FsMgr_Method_FormatVolume,
    K2OS_FsMgr_Method_CleanVolume,
    K2OS_FsMgr_Method_Mount,
    K2OS_FsMgr_Method_Dismount,

    K2OS_FsMgr_Method_Count
};

typedef enum _K2OS_FsMgr_Notify K2OS_FsMgr_Notify;
enum _K2OS_FsMgr_Notify
{
    K2OS_FsMgr_Notify_Invalid = 0,

    K2OS_FsMgr_Notify_MountChange,
    K2OS_FsMgr_Notify_FsArrived,

    K2OS_FsMgr_Notify_Count
};

typedef struct _K2OS_FSMGR_FORMAT_VOLUME_IN K2OS_FSMGR_FORMAT_VOLUME_IN;
struct _K2OS_FSMGR_FORMAT_VOLUME_IN
{
    K2OS_IFINST_ID  mVolIfInstId;
    K2_GUID128      mFsGuid;
    K2_GUID128      mUniqueId;
    UINT32          mParam1;
    UINT32          mParam2;
};

typedef struct _K2OS_FSMGR_CLEAN_VOLUME_IN K2OS_FSMGR_CLEAN_VOLUME_IN;
struct _K2OS_FSMGR_CLEAN_VOLUME_IN
{
    K2OS_IFINST_ID  mVolIfInstId;
};

typedef struct _K2OS_FSMGR_MOUNT_IN K2OS_FSMGR_MOUNT_IN;
struct _K2OS_FSMGR_MOUNT_IN
{
    K2OS_IFINST_ID      mFileSysIfInstId;
    UINT32              mAccess;
    UINT32              mShare;
    UINT32              mPathLen;
    char                mPath[4];
};

typedef struct _K2OS_FSMGR_MOUNT_IN K2OS_FSMGR_DISMOUNT_IN;
struct _K2OS_FSMGR_DISMOUNT_IN
{
    char mPath[4];
};

//
//------------------------------------------------------------------------
//

// {06331AD3-78B2-4B4B-B9E8-283582CA5849}
#define K2OS_IFACE_FILESYS { 0x6331ad3, 0x78b2, 0x4b4b, { 0xb9, 0xe8, 0x28, 0x35, 0x82, 0xca, 0x58, 0x49 } }

typedef enum _K2OS_FileSys_Method K2OS_FileSys_Method;
enum _K2OS_FileSys_Method
{
    K2OS_FileSys_Method_Invalid = 0,

    K2OS_FileSys_Method_GetDetail,

    K2OS_FileSys_Method_Count
};

typedef struct _K2OS_FILESYS_GETDETAIL_OUT K2OS_FILESYS_GETDETAIL_OUT;
struct _K2OS_FILESYS_GETDETAIL_OUT
{
    UINT32  mFsProv_Flags;
    UINT32  mAttachContext;
    UINT32  mProviderContext;
    UINT32  mFsNumber;
    BOOL    mAttachedReadOnly;
    BOOL    mIsCaseSensitive;
    BOOL    mCanDoPaging;
};

typedef struct _K2OS_FILESYS_GETINDEX_OUT K2OS_FILESYS_GETINDEX_OUT;
struct _K2OS_FILESYS_GETINDEX_OUT
{
    UINT32  mFsNumber;
};

//
//------------------------------------------------------------------------
//

// {BF9AC158-6F60-4015-A856-E87BBB33F842}
#define K2OS_RPCCLASS_FSCLIENT { 0xbf9ac158, 0x6f60, 0x4015, { 0xa8, 0x56, 0xe8, 0x7b, 0xbb, 0x33, 0xf8, 0x42 } }

typedef enum _K2OS_FsClient_Method K2OS_FsClient_Method;
enum _K2OS_FsClient_Method
{
    K2OS_FsClient_Method_Invalid = 0,

    K2OS_FsClient_Method_GetBaseDir,
    K2OS_FsClient_Method_SetBaseDir,
    K2OS_FsClient_Method_CreateDir,
    K2OS_FsClient_Method_RemoveDir,
    K2OS_FsClient_Method_GetAttrib,
    K2OS_FsClient_Method_SetAttrib,
    K2OS_FsClient_Method_DeleteFile,
    K2OS_FsClient_Method_GetIoBlockAlign,

    K2OS_FsClient_Method_Count
};

typedef struct _K2OS_FSCLIENT_GETBASEDIR_IN K2OS_FSCLIENT_GETBASEDIR_IN;
struct _K2OS_FSCLIENT_GETBASEDIR_IN
{
    K2OS_BUFDESC TargetBufDesc;
};

typedef struct _K2OS_FSCLIENT_SETBASEDIR_IN K2OS_FSCLIENT_SETBASEDIR_IN;
struct _K2OS_FSCLIENT_SETBASEDIR_IN
{
    K2OS_BUFDESC SourceBufDesc;
};

typedef struct _K2OS_FSCLIENT_CREATEDIR_IN K2OS_FSCLIENT_CREATEDIR_IN;
struct _K2OS_FSCLIENT_CREATEDIR_IN
{
    K2OS_BUFDESC SourceBufDesc;
};

typedef struct _K2OS_FSCLIENT_REMOVEDIR_IN K2OS_FSCLIENT_REMOVEDIR_IN;
struct _K2OS_FSCLIENT_REMOVEDIR_IN
{
    K2OS_BUFDESC SourceBufDesc;
};

typedef struct _K2OS_FSCLIENT_GETATTRIB_IN K2OS_FSCLIENT_GETATTRIB_IN;
struct _K2OS_FSCLIENT_GETATTRIB_IN
{
    K2OS_BUFDESC SourceBufDesc;
};
typedef struct _K2OS_FSCLIENT_GETATTRIB_OUT K2OS_FSCLIENT_GETATTRIB_OUT;
struct _K2OS_FSCLIENT_GETATTRIB_OUT
{
    UINT32 mFsAttrib;
};

typedef struct _K2OS_FSCLIENT_SETATTRIB_IN K2OS_FSCLIENT_SETATTRIB_IN;
struct _K2OS_FSCLIENT_SETATTRIB_IN
{
    K2OS_BUFDESC    SourceBufDesc;
    UINT32          mNewFsAttrib;
};
typedef struct _K2OS_FSCLIENT_SETATTRIB_OUT K2OS_FSCLIENT_SETATTRIB_OUT;
struct _K2OS_FSCLIENT_SETATTRIB_OUT
{
    UINT32 mOldFsAttrib;
};

typedef struct _K2OS_FSCLIENT_DELETEFILE_IN K2OS_FSCLIENT_DELETEFILE_IN;
struct _K2OS_FSCLIENT_DELETEFILE_IN
{
    K2OS_BUFDESC SourceBufDesc;
};

typedef struct _K2OS_FSCLIENT_GETIOBLOCKALIGN_IN K2OS_FSCLIENT_GETIOBLOCKALIGN_IN;
struct _K2OS_FSCLIENT_GETIOBLOCKALIGN_IN
{
    K2OS_BUFDESC SourceBufDesc;
};
typedef struct _K2OS_FSCLIENT_GETIOBLOCKALIGN_OUT K2OS_FSCLIENT_GETIOBLOCKALIGN_OUT;
struct _K2OS_FSCLIENT_GETIOBLOCKALIGN_OUT
{
    UINT32 mIoBlockAlign;
};

//
//------------------------------------------------------------------------
//

// {EFC1D9B6-4321-4BEA-AD54-67F6444ECC0A}
#define K2OS_RPCCLASS_FSENUM { 0xefc1d9b6, 0x4321, 0x4bea, { 0xad, 0x54, 0x67, 0xf6, 0x44, 0x4e, 0xcc, 0xa } }

typedef enum _K2OS_FsEnum_Method K2OS_FsEnum_Method;
enum _K2OS_FsEnum_Method
{
    K2OS_FsEnum_Method_Invalid = 0,

    K2OS_FsEnum_Method_Exec,
    K2OS_FsEnum_Method_Next,

    K2OS_FsEnum_Method_Count
};

//
//------------------------------------------------------------------------
//

// {7E708348-82A4-4BE5-A2DE-8D29A86351CB}
#define K2OS_RPCCLASS_FSFILE { 0x7e708348, 0x82a4, 0x4be5, { 0xa2, 0xde, 0x8d, 0x29, 0xa8, 0x63, 0x51, 0xcb } }

typedef enum _K2OS_FsFile_Method K2OS_FsFile_Method;
enum _K2OS_FsFile_Method
{
    K2OS_FsFile_Method_Invalid = 0,

    K2OS_FsFile_Method_Bind,
    K2OS_FsFile_Method_GetInfo,
    K2OS_FsFile_Method_GetPos,
    K2OS_FsFile_Method_SetPos,
    K2OS_FsFile_Method_GetSize,
    K2OS_FsFile_Method_Read,
    K2OS_FsFile_Method_Write,
    K2OS_FsFile_Method_SetEnd,
    K2OS_FsFile_Method_GetAccess,
    K2OS_FsFile_Method_GetShare,
    K2OS_FsFile_Method_GetAttrib,
    K2OS_FsFile_Method_GetOpenFlags,
    K2OS_FsFile_Method_GetBlockAlign,

    K2OS_FsFile_Method_Count
};

typedef struct _K2OS_FSFILE_BIND_IN K2OS_FSFILE_BIND_IN;
struct _K2OS_FSFILE_BIND_IN
{
    UINT32              mFsClientObjId;
    K2OS_BUFDESC        SourceBufDesc;
    UINT32              mAccess;
    UINT32              mShare;
    K2OS_FileOpenType   mOpenType;
    UINT32              mOpenFlags;
    UINT32              mNewFileAttrib;
};

typedef struct _K2OS_FSFILE_READ_IN K2OS_FSFILE_READ_IN;
struct _K2OS_FSFILE_READ_IN
{
    K2OS_BUFDESC TargetBufDesc;
    UINT32       mBytesToRead;
};
typedef struct _K2OS_FSFILE_READ_OUT K2OS_FSFILE_READ_OUT;
struct _K2OS_FSFILE_READ_OUT
{
    UINT32  mBytesRed;
};

//
//------------------------------------------------------------------------
//

#if __cplusplus
}f
#endif

#endif // __K2OSSTOR_H