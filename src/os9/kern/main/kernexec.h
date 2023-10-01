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
//   OF THIS SOFTWARE, EVEN IFK ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __KERNEXEC_H
#define __KERNEXEC_H

/* --------------------------------------------------------------------------------- */

typedef struct _K2OSACPI_INIT K2OSACPI_INIT;
struct _K2OSACPI_INIT
{
    K2OS_FWINFO FwInfo;
    UINT32      mFwBaseVirt;
    UINT32      mFacsVirt;
    UINT32      mXFacsVirt;
};

typedef K2OS_PAGEARRAY_TOKEN (*K2OSKERN_pf_PageArray_CreateAt)(UINT32 aPhysBase, UINT32 aPageCount);
typedef K2OS_PAGEARRAY_TOKEN (*K2OSKERN_pf_PageArray_CreateIo)(UINT32 aFlags, UINT32 aPageCountPow2, UINT32 *apRetPhysBase);
typedef UINT32               (*K2OSKERN_pf_PageArray_GetPagePhys)(K2OS_PAGEARRAY_TOKEN aTokPageArray, UINT32 aPageIndex);

typedef enum _K2OSExecIop_Type K2OSExecIop_Type;
enum _K2OSExecIop_Type
{
    K2OSExecIop_Invalid = 0,

    K2OSExecIop_BlockIo,

    K2OSExecIop_TypeCount 
};

typedef enum _K2OSExecIop_BlockIoOp_Type K2OSExecIop_BlockIoOp_Type;
enum _K2OSExecIop_BlockIoOp_Type
{
    K2OSExecIop_BlockIoOp_Invalid = 0,

    K2OSExecIop_BlockIoOp_Read = 1,     // same as K2OS_BlockIoTransfer_Read
    K2OSExecIop_BlockIoOp_Write = 2,    // same as K2OS_BlockIoTransfer_Write
    K2OSExecIop_BlockIoOp_Erase = 3,    // same as K2OS_BlockIoTransfer_Erase

    K2OSExecIop_BlockIoOp_GetMedia,
    K2OSExecIop_BlockIoOp_RangeCreate,
    K2OSExecIop_BlockIoOp_RangeDelete,

    K2OSExecIop_BlockIoOp_OwnerDied,

    K2OSExecIop_BlokcIoOp_TypeCount
};

typedef struct _K2OSEXEC_IOP K2OSEXEC_IOP;
struct _K2OSEXEC_IOP
{
    K2OSExecIop_Type    mType;

    UINT32              mOwner;

    union {
        struct {
            UINT32  mOp;
            UINT32  mMemAddr;
            UINT32  mRange;
            UINT32  mOffset;
            UINT32  mCount;
        } BlockIo;

        struct {
            UINT32  mMemAddr;
            UINT32  mCount;
            UINT32  mResultCount;
        } StreamIo;
    } Op;

    K2LIST_LINK         ListLink;
};

typedef struct _K2OSEXEC_IOTHREAD K2OSEXEC_IOTHREAD;

typedef BOOL   (*K2OSEXEC_pf_Io_ValidateRange)(K2OSEXEC_IOTHREAD *apIoThread, K2OS_BLOCKIO_RANGE aRange, UINT32 aBytesOffset, UINT32 aByteCount, UINT32 aMemPageOffset);
typedef void   (*K2OSEXEC_pf_Io_Submit)(K2OSEXEC_IOTHREAD *apIoThread, K2OSEXEC_IOP *apIop);
typedef K2STAT (*K2OSEXEC_pf_Io_Cancel)(K2OSEXEC_IOTHREAD *apIoThread, K2OSEXEC_IOP *apIop);

struct _K2OSEXEC_IOTHREAD
{
    BOOL                            mUsePhysAddr;
    K2OSEXEC_pf_Io_ValidateRange    ValidateRange;
    K2OSEXEC_pf_Io_Submit           Submit;
    K2OSEXEC_pf_Io_Cancel           Cancel;
};

typedef K2STAT (*K2OSKERN_pf_Io_SetThread)(K2OS_IFINST_ID aIfInstId, K2OSEXEC_IOTHREAD *apThread);
typedef void   (*K2OSKERN_pf_Io_Complete)(K2OSEXEC_IOTHREAD *apThread, K2OSEXEC_IOP *apIop, UINT32 aResult, BOOL aSetStat, K2STAT aStat);

typedef struct _K2OSKERN_DDK K2OSKERN_DDK;
struct _K2OSKERN_DDK
{
    K2OSKERN_pf_PageArray_CreateAt      PageArray_CreateAt;
    K2OSKERN_pf_PageArray_CreateIo      PageArray_CreateIo;
    K2OSKERN_pf_PageArray_GetPagePhys   PageArray_GetPagePhys;

    K2OSKERN_pf_Io_SetThread            Io_SetThread;
    K2OSKERN_pf_Io_Complete             Io_Complete;
};

typedef struct _K2OSEXEC_INIT K2OSEXEC_INIT;
struct _K2OSEXEC_INIT
{
    K2OSACPI_INIT   AcpiInit;
    K2OSKERN_DDK    DdkInit;
};

/* --------------------------------------------------------------------------------- */

#endif // __KERNEXEC_H