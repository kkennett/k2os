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

#include <k2osstor.h>
#include <lib/k2rofshelp.h>

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
typedef K2OS_TOKEN           (*K2OSKERN_pf_UserToken_Clone)(UINT32 aProcessId, K2OS_TOKEN aUserToken);
typedef K2OS_VIRTMAP_TOKEN   (*K2OSKERN_pf_UserVirtMap_Create)(UINT32 aProcessId, UINT32 aVirtResBase, UINT32 aVirtResPageCount, K2OS_PAGEARRAY_TOKEN aTokPageArray);
typedef K2STAT               (*K2OSKERN_pf_UserMap)(UINT32 aProcessId, K2OS_PAGEARRAY_TOKEN aKernTokPageArray, UINT32 aPageCount, UINT32 *apRetUserVirtAddr, K2OS_VIRTMAP_TOKEN *apRetTokUserVirtMap);

typedef struct _K2OSKERN_DDK K2OSKERN_DDK;
struct _K2OSKERN_DDK
{
    K2OSKERN_pf_PageArray_CreateAt      PageArray_CreateAt;
    K2OSKERN_pf_PageArray_CreateIo      PageArray_CreateIo;
    K2OSKERN_pf_PageArray_GetPagePhys   PageArray_GetPagePhys;
    K2OSKERN_pf_UserToken_Clone         UserToken_Clone;
    K2OSKERN_pf_UserVirtMap_Create      UserVirtMap_Create;
    K2OSKERN_pf_UserMap                 UserMap;
};

typedef void (*K2OSKERN_pf_WaitSysProcReady)(void);

typedef struct _K2OSEXEC_INIT K2OSEXEC_INIT;
struct _K2OSEXEC_INIT
{
    K2OSACPI_INIT                   AcpiInit;
    K2OSKERN_DDK                    DdkInit;
    K2OSKERN_pf_WaitSysProcReady    WaitSysProcReady;
    K2ROFS const *                  mpRofs;
};

/* --------------------------------------------------------------------------------- */

#endif // __KERNEXEC_H