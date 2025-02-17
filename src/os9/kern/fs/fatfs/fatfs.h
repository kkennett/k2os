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
#ifndef __FATFS_H
#define __FATFS_H

#include <kern/k2osddk.h>
#include <k2osstor.h>
#include <lib/k2fat.h>

/* ------------------------------------------------------------------------- */

// {80BD0D97-725C-4620-AD86-315DFB7017D1}
#define FATPROV_OBJ_CLASS_GUID  { 0x80bd0d97, 0x725c, 0x4620, { 0xad, 0x86, 0x31, 0x5d, 0xfb, 0x70, 0x17, 0xd1 } }

#define FATFS_NUM_PROV  3

typedef struct _FATPROV FATPROV;
struct _FATPROV
{
    K2OS_RPC_IFINST     mRpcIfInst;
    K2OS_RPC_OBJ        mRpcObj;
    K2OS_RPC_OBJ_HANDLE mRpcObjHandle;
    K2OS_RPC_CLASS      mRpcClass;
    K2OS_IFINST_ID      mIfInstId;

    K2OSKERN_FSPROV     KernFsProv[FATFS_NUM_PROV];

    K2OS_CRITSEC        Sec;
    K2LIST_ANCHOR       FsList;
};

typedef struct _FATFS_DIR FATFS_DIR;
struct _FATFS_DIR
{
    K2OSKERN_FSNODE     OsFsNode;
};

typedef struct _FATFS_FILE FATFS_FILE;
struct _FATFS_FILE
{
    K2OSKERN_FSNODE     OsFsNode;
};

typedef struct _FATFS_OBJ_COMMON FATFS_OBJ_COMMON;
struct _FATFS_OBJ_COMMON
{
    K2OS_CRITSEC        CritSec;

    K2OS_IFINST_ID      mVolIfInstId;
    K2OSSTOR_VOLUME     mStorVol;
    K2_STORAGE_VOLUME   VolInfo;

    UINT8 *             mpBootSecBuffer;
    UINT8 *             mpBootSector;
    K2FAT_PART          FatPart;

    K2OSKERN_FILESYS *  mpFileSys;
    K2OSKERN_FSNODE *   mpRootFsNode;
    FATFS_DIR           RootDir;
};

extern FATPROV gFatProv;

K2STAT FATFS_RpcObj_Create(K2OS_RPC_OBJ aObject, K2OS_RPC_OBJ_CREATE const *apCreate, UINT32 *apRetContext);
K2STAT FATFS_RpcObj_OnAttach(K2OS_RPC_OBJ aObject, UINT32 aObjContext, UINT32 aProcessId, UINT32 *apRetUseContext);
K2STAT FATFS_RpcObj_OnDetach(K2OS_RPC_OBJ aObject, UINT32 aObjContext, UINT32 aUseContext);
K2STAT FATFS_RpcObj_Call(K2OS_RPC_OBJ_CALL const *apCall, UINT32 *apRetUsedOutBytes);
K2STAT FATFS_RpcObj_Delete(K2OS_RPC_OBJ aObject, UINT32 aObjContext);

/* ------------------------------------------------------------------------- */

K2STAT FATCom_Probe(void * apContext, K2FAT_Type aFatType, BOOL aWantReadWrite, BOOL *apRetMatched);

K2STAT FAT12_Probe(K2OSKERN_FSPROV *apProv, void *apContext, BOOL *apRetMatched);
K2STAT FAT12_Attach(K2OSKERN_FSPROV *apProv, K2OSKERN_FILESYS *apFileSys, K2OSKERN_FSNODE *apRootFsNode);

K2STAT FAT16_Probe(K2OSKERN_FSPROV *apProv, void *apContext, BOOL *apRetMatched);
K2STAT FAT16_Attach(K2OSKERN_FSPROV *apProv, K2OSKERN_FILESYS *apFileSys, K2OSKERN_FSNODE *apRootFsNode);

K2STAT FAT32_Probe(K2OSKERN_FSPROV *apProv, void *apContext, BOOL *apRetMatched);
K2STAT FAT32_Attach(K2OSKERN_FSPROV *apProv, K2OSKERN_FILESYS *apFileSys, K2OSKERN_FSNODE *apRootFsNode);

/* ------------------------------------------------------------------------- */

#endif // __FATFS_H