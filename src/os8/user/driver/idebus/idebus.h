//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
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
#ifndef __IDEBUS_H
#define __IDEBUS_H

#include <k2osdrv.h>
#include <k2osrpc.h>

/* ------------------------------------------------------------------------- */

typedef struct _IDE_DEVICE IDE_DEVICE;
struct _IDE_DEVICE
{
    UINT_PTR        mLocation;
    UINT_PTR        mObjectId;
    UINT_PTR        mOwnUsageId;
    K2_DEVICE_IDENT Ident;
    UINT_PTR        mSystemDeviceInstanceId;
    BOOL            mMountOk;
};

#define IDE_CHANNEL_DEVICE_MASTER   0
#define IDE_CHANNEL_DEVICE_SLAVE    1

typedef struct _IDE_CHANNEL IDE_CHANNEL;
struct _IDE_CHANNEL
{
    BOOL            mProbed;
    K2OS_CRITSEC    Sec;
    IDE_DEVICE      Device[2];
};

#define IDE_CHANNEL_PRIMARY         0
#define IDE_CHANNEL_SECONDARY       1

extern K2OSDRV_RES_IO   gBusIo;
extern UINT_PTR         gPopMask;
extern IDE_CHANNEL      gChannel[2];

extern K2OSRPC_OBJECT_CLASS const gIdeBusChild_ObjectClass;

/* ------------------------------------------------------------------------- */

K2STAT IDEBUS_InitAndDiscover(void);

/* ------------------------------------------------------------------------- */

#endif // __IDEBUS_H