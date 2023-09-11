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
#ifndef __K2ROFSHELP_H
#define __K2ROFSHELP_H

/* --------------------------------------------------------------------------------- */

#include <k2systype.h>
#include <spec/k2rofs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K2ROFS_ROOTDIR(pRofs)               ((K2ROFS_DIR const *)(((UINT_PTR)(pRofs)) + (((K2ROFS *)(pRofs))->mRootDir)))
#define K2ROFS_NAMESTR(pRofs,nameOffset)    ((char const *)(((UINT_PTR)(pRofs)) + ((UINT_PTR)(nameOffset))))
#define K2ROFS_FILEDATA(pRofs, pFile)       (((UINT8 const *)pRofs) + ((((K2ROFS_FILE const *)(pFile))->mStartSectorOffset) * K2ROFS_SECTOR_BYTES))
#define K2ROFS_FILESECTORS(pFile)           (K2_ROUNDUP(((K2ROFS_FILE const *)(pFile))->mSizeBytes, K2ROFS_SECTOR_BYTES) / K2ROFS_SECTOR_BYTES)

K2ROFS_DIR const *
K2ROFSHELP_SubDir(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    char const *        apSubFilePath
);

K2ROFS_FILE const *
K2ROFSHELP_SubFile(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    char const *        apSubFilePath
);

K2ROFS_DIR const *
K2ROFSHELP_SubDirIx(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    UINT_PTR            aSubDirIx
);

K2ROFS_FILE const *
K2ROFSHELP_SubFileIx(
    K2ROFS const *      apBase,
    K2ROFS_DIR const *  apDir,
    UINT_PTR            aSubFileIx
);

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif  // __K2ROFSHELP_H

