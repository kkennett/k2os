#ifndef __SPEC_K2PCI_H
#define __SPEC_K2PCI_H

#include <k2basetype.h>
#include <spec/k2pcidef.inc>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------- */

typedef union _PCIADDR PCIADDR;
K2_PACKED_PUSH
union _PCIADDR
{
    UINT32  mAsUINT32;
    struct 
    {
        UINT32 mSBZ         : 2;
        UINT32 mWord        : 6;
        UINT32 mFunction    : 3;
        UINT32 mDevice      : 5;
        UINT32 mBus         : 8;
        UINT32 mReserved    : 7;
        UINT32 mSBO         : 1;
    };
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(sizeof(PCIADDR)==sizeof(UINT32));

typedef union _PCICFG_TYPEX PCICFG_TYPEX;
K2_PACKED_PUSH
union _PCICFG_TYPEX
{
    UINT32  AsUINT32[16];
    struct
    {
        UINT16  mVendorId;
        UINT16  mDeviceId;
        UINT16  mCommand;
        UINT16  mStatus;
        UINT8   mRevision;
        UINT8   mProgIF;
        UINT8   mSubClassCode;
        UINT8   mClassCode;
        UINT8   mCacheLineSize;
        UINT8   mLatencyTimer;
        UINT8   mHeaderType;
        UINT8   mBIST;
        UINT32  mBar0;
        UINT32  mBar1;

        UINT32  mStuff[9];

        UINT8   mInterruptLine;
        UINT8   mInterruptPin;
        UINT8   mMinGnt;
        UINT8   mMaxLat;
    };
} K2_PACKED_ATTRIB;
K2_PACKED_POP;
K2_STATIC_ASSERT(sizeof(PCICFG_TYPEX) == (16 * 4));

typedef union _PCICFG_TYPE0 PCICFG_TYPE0;
K2_PACKED_PUSH
union _PCICFG_TYPE0
{
    UINT32  AsUINT32[16];
    struct
    {
        UINT16  mVendorId;
        UINT16  mDeviceId;
        UINT16  mCommand;
        UINT16  mStatus;
        UINT8   mRevision;
        UINT8   mProgIF;
        UINT8   mSubClassCode;
        UINT8   mClassCode;
        UINT8   mCacheLineSize;
        UINT8   mLatencyTimer;
        UINT8   mHeaderType;
        UINT8   mBIST;
        UINT32  mBar0;
        UINT32  mBar1;

        UINT32  mBar2;
        UINT32  mBar3;
        UINT32  mBar4;
        UINT32  mBar5;
        UINT32  mCarbusCISPointer;
        UINT16  mSubsystemVendorId;
        UINT16  mSubsystemId;
        UINT32  mExpansionRomBaseAddr;
        UINT8   mCapPointer;
        UINT8   mReserved_35[3];
        UINT32  mReserved_38;

        UINT8   mInterruptLine;
        UINT8   mInterruptPin;
        UINT8   mMinGnt;
        UINT8   mMaxLat;
    };
} K2_PACKED_ATTRIB;
K2_PACKED_POP;
K2_STATIC_ASSERT(sizeof(PCICFG_TYPE0) == (16*4));

typedef union _PCICFG_TYPE0 PCICFG_TYPE1;
K2_PACKED_PUSH
union _PCICFG_TYPE1
{
    UINT32  AsUINT32[16];
    struct
    {
        UINT16  mVendorId;
        UINT16  mDeviceId;
        UINT16  mCommand;
        UINT16  mStatus;
        UINT8   mRevision;
        UINT8   mProgIF;
        UINT8   mSubClassCode;
        UINT8   mClassCode;
        UINT8   mCacheLineSize;
        UINT8   mLatencyTimer;
        UINT8   mHeaderType;
        UINT8   mBIST;
        UINT32  mBar0;
        UINT32  mBar1;

        UINT8   mPrimaryBusNumber;
        UINT8   mSecondaryBusNumber;
        UINT8   mSubordinateBusNumber;
        UINT8   mSecondaryLatencyTimer;
        UINT8   mIOBase;
        UINT8   mIOLimit;
        UINT16  mSecondaryStatus;
        UINT16  mMemoryBase;
        UINT16  mMemoryLimit;
        UINT16  mPrefMemoryBase;
        UINT16  mPrefMemoryLimit;
        UINT32  mPrefMemoryBaseUpper32;
        UINT32  mPrefMemoryLimitUpper32;
        UINT16  mIoBaseUpper16;
        UINT16  mIoLimitUpper16;
        UINT8   mCapPointer;
        UINT8   mReserved_35[3];
        UINT32  mXROMBAR;

        UINT8   mInterruptLine;
        UINT8   mInterruptPin;
        UINT8   mMinGnt;
        UINT8   mMaxLat;
    };
} K2_PACKED_ATTRIB;
K2_PACKED_POP;
K2_STATIC_ASSERT(sizeof(PCICFG_TYPE1) == (16*4));

typedef union _PCICFG PCICFG;
K2_PACKED_PUSH
union _PCICFG
{
    UINT32          mAsUINT32[16];
    PCICFG_TYPEX    AsTypeX;
    PCICFG_TYPE0    AsType0;
    PCICFG_TYPE1    AsType1;
} K2_PACKED_ATTRIB;
K2_PACKED_POP;
K2_STATIC_ASSERT(sizeof(PCICFG) == (16 * 4));

/* ------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // __SPEC_K2PCI_H

