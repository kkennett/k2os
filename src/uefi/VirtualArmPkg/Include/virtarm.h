//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED 'AS IS', WITH NO WARRANTIES OR INDEMNITIES.
//
// Code Originator:  Kurt Kennett
// Last Updated By:  Kurt Kennett
//

#ifndef _VIRTARM_H_
#define _VIRTARM_H_

/*--------------------------------------------*
         RAW MULTI-PLATFORM INCLUDE FILE
         DO NOT ASSUME ANY PARTICULAR OS
           OR ANY PARTICULAR COMPILER
  --------------------------------------------*/

#include "virtarm.inc"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------

#ifndef _REG32_DEFINED
#define _REG32  volatile unsigned long
#define _REG32_DEFINED
#endif

#ifndef _REG16_DEFINED
#define _REG16  volatile unsigned short
#define _REG16_DEFINED
#endif

// ---------------------------------------------------------------------

typedef struct _CORTEXA9MP_SCU_REGS
{
/* 0x000 */ _REG32 mControl;  
/* 0x004 */ _REG32 mConfig;
/* 0x008 */ _REG32 mCPUPowerStatus;
/* 0x00C */ _REG32 mInvalidateSecure;

/* 0x010-
   0x03C */ _REG32 mReserved1[12];

/* 0x040 */ _REG32 mFilterStart;
/* 0x044 */ _REG32 mFilterEnd;

/* 0x048-
   0x04C */ _REG32 mReserved2[2];

/* 0x050 */ _REG32 mAccessControl;
/* 0x054 */ _REG32 mNonSecureAccessControl;
} CORTEXA9MP_SCU_REGS;

// ---------------------------------------------------------------------

/* this is an aliased per-cpu structure */

typedef struct _CORTEXA9MP_PERCPUINT_REGS
{
/* 0x000 */ _REG32 ICCICR;
/* 0x004 */ _REG32 ICCPMR;
/* 0x008 */ _REG32 ICCBPR;
/* 0x00C */ _REG32 ICCIAR;
/* 0x010 */ _REG32 ICCEOIR;
/* 0x014 */ _REG32 ICCRPR;
/* 0x018 */ _REG32 ICCHPIR;
/* 0x01C */ _REG32 ICCABPR;
/* 0x020-
   0x0F8 */ _REG32 mReserved[55];
/* 0x0FC */ _REG32 ICPIIR;
} CORTEXA9MP_PERCPUINT_REGS;

// ---------------------------------------------------------------------

typedef struct _CORTEXA9MP_GTIMER_REGS
{
/* 0x000 */ _REG32 mCountLow;
/* 0x004 */ _REG32 mCountHigh;
/* 0x008 */ _REG32 mControl;
/* 0x00C */ _REG32 mIntStatus;
/* 0x010 */ _REG32 mCompValLow;
/* 0x014 */ _REG32 mCompValHigh;
/* 0x018 */ _REG32 mAutoInc;
} CORTEXA9MP_GTIMER_REGS;

// ---------------------------------------------------------------------

typedef struct _CORTEXA9MP_PRIVTIMERS_REGS
{
/* 0x000 */ _REG32 mLoad;
/* 0x004 */ _REG32 mCounter;
/* 0x008 */ _REG32 mControl;
/* 0x00C */ _REG32 mIntStatus;

/* 0x010-
   0x01C */ _REG32 mReserved[4];

/* 0x020 */ _REG32 mWatchdogLoad;
/* 0x024 */ _REG32 mWatchdogCounter;
/* 0x028 */ _REG32 mWatchdogControl;
/* 0x02C */ _REG32 mWatchdogIntStatus;
/* 0x030 */ _REG32 mWatchdogResetStatus;
/* 0x034 */ _REG32 mWatchdogDisable;
} CORTEXA9MP_PRIVTIMERS_REGS;

// ---------------------------------------------------------------------

typedef struct _CORTEXA9MP_INTDIST_REGS
{
/* 0x000 */ _REG32 ICDDCR;
/* 0x004 */ _REG32 ICDICTR;
/* 0x008 */ _REG32 ICDDIR;

/* 0x00C-
   0x07C */ _REG32 mReserved1[29];

/* 0x080-
   0x09C */ _REG32 ICDISR[8];

/* 0x0A0-
   0x0FC */ _REG32 mReserved2[24];

/* 0x100-
   0x11C */ _REG32 ICDISER[8];

/* 0x120-
   0x17C */ _REG32 mReserved3[24];

/* 0x180-
   0x19C */ _REG32 ICDICER[8];

/* 0x1A0-
   0x1FC */ _REG32 mReserved4[24];

/* 0x200-
   0x21C */ _REG32 IDISPR[8];

/* 0x220-
   0x27C */ _REG32 mReserved5[24];

/* 0x280-
   0x29C */ _REG32 ICDICPR[8];

/* 0x2A0-
   0x2FC */ _REG32 mReserved6[24];

/* 0x300-
   0x31C */ _REG32 ICDABR[8];

/* 0x320-
   0x3FC */ _REG32 mReserved7[56];

/* 0x400-
   0x4FC */ _REG32 ICDIPR[64];

/* 0x500-
   0x7FC */ _REG32 mReserved8[192];

/* 0x800-
   0x8FC */ _REG32 ICDIPTR[64];

/* 0x900-
   0xBFC */ _REG32 mReserved9[192];

/* 0xC00-
   0xC3C */ _REG32 ICDICR[16];

/* 0xC40-
   0xCFC */ _REG32 mReserved10[48];

/* 0xD00 */ _REG32 ppi_status;
/* 0xD04-
   0xD1C */ _REG32 spi_status[7];

/* 0xD20-
   0xEFC */ _REG32 mReserved11[120];

/* 0xF00 */ _REG32 ICDSGIR;

/* 0xF04-
   0xFCC */ _REG32 mReserved12[51];

/* 0xFD0-
   0xFEC */ _REG32 periph_id[8];

/* 0xFF0-
   0xFFC */ _REG32 component_id[4];

} CORTEXA9MP_INTDIST_REGS;

// ---------------------------------------------------------------------

typedef struct _VIRTARM_PROGCOUNTER
{
/* 0x00 */ _REG32 LO;
/* 0x04 */ _REG32 HI;
} VIRTARM_PROGCOUNTER;

// ---------------------------------------------------------------------

#define VIRTARM_OFFSET_SP804_TIMER(x)           ((x)*VIRTARM_PHYSSIZE_ONE_SP804_TIMER)
#define VIRTARM_PHYSADDR_SP804_TIMER(x)         (VIRTARM_PHYSADDR_SP804_TIMERS + VIRTARM_OFFSET_SP804_TIMER(x))
    
typedef struct _ARM_SP804_REGS  /* ARM document DDI0271 */
{
/* 0x000 */ _REG32 Timer1Load;      /* read/write */
/* 0x004 */ _REG32 Timer1Value;     /* read-only. */
/* 0x008 */ _REG32 Timer1Control;   /* read/write. only low 8 bits valid. */
/* 0x00C */ _REG32 Timer1IntClr;    /* write-only. any write clears */
/* 0x010 */ _REG32 Timer1RIS;       /* read-only.  only low 8 bits valid */
/* 0x014 */ _REG32 Timer1MIS;       /* read-only.  only low 8 bits valid */
/* 0x018 */ _REG32 Timer1BGLoad;    /* read/write. only low 8 bits valid */
/* 0x01C */ _REG32 Reserved1;        

/* 0x020 */ _REG32 Timer2Load;      /* read/write */
/* 0x024 */ _REG32 Timer2Value;     /* read-only. */
/* 0x028 */ _REG32 Timer2Control;   /* read/write. only low 8 bits valid. */
/* 0x02C */ _REG32 Timer2IntClr;    /* write-only. any write clears */
/* 0x030 */ _REG32 Timer2RIS;       /* read-only.  only low 8 bits valid */
/* 0x034 */ _REG32 Timer2MIS;       /* read-only.  only low 8 bits valid */
/* 0x038 */ _REG32 Timer2BGLoad;    /* read/write. only low 8 bits valid */
/* 0x03C */ _REG32 Reserved2;        

/* 0x040 -- 0xEFC */ _REG32 Reserved3[944];

/* 0xF00 */ _REG32 TimerITCR;       /* read/write. only low 8 bits valid */
/* 0xF04 */ _REG32 TimerITOP;       /* write-only. only low 16 bits valid */
/* 0xF08 -- 
   0xFDC */ _REG32 Reserved4[54];
/* 0xFE0 */ _REG32 TimerPeriphID0;  /* read-only.  only low 8 bits valid */
/* 0xFE4 */ _REG32 TimerPeriphID1;  /* read-only.  only low 8 bits valid */
/* 0xFE8 */ _REG32 TimerPeriphID2;  /* read-only.  only low 8 bits valid */
/* 0xFEC */ _REG32 TimerPeriphID3;  /* read-only.  only low 8 bits valid */

/* 0xFF0 */ _REG32 TimerPCellID0;   /* read-only.  only low 8 bits valid */
/* 0xFF4 */ _REG32 TimerPCellID1;   /* read-only.  only low 8 bits valid */
/* 0xFF8 */ _REG32 TimerPCellID2;   /* read-only.  only low 8 bits valid */
/* 0xFFC */ _REG32 TimerPCellID3;   /* read-only.  only low 8 bits valid */

} ARM_SP804_REGS;

// ---------------------------------------------------------------------

#define VIRTARM_PHYSADDR_ADAPTERSLOTS       0x28000000
#define VIRTARM_OFFSET_ADAPTERSLOT(x)       ((x)*VIRTARM_PHYSSIZE_ONE_ADAPTERSLOT)
#define VIRTARM_PHYSADDR_ADAPTERSLOT(x)     (VIRTARM_PHYSADDR_ADAPTERSLOTS + VIRTARM_OFFSET_ADAPTERSLOT(x))

#define VIRTARM_PHYSADDR_ADAPTER_SRAM(x)    (VIRTARM_PHYSADDR_ADAPTERSLOT(x) + VIRTARM_OFFSET_ADAPTER_SRAM)
#define VIRTARM_PHYSADDR_ADAPTER_REGS(x)    (VIRTARM_PHYSADDR_ADAPTERSLOT(x) + VIRTARM_OFFSET_ADAPTER_REGS)

// ---------------------------------------------------------------------
#ifdef __cplusplus
};
#endif

#endif // _VIRTARM_H_
