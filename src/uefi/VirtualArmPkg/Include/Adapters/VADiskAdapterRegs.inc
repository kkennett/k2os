#ifndef __VADISKADAPTERREGS_INC
#define __VADISKADAPTERREGS_INC 

///------------------------------------------------------------------------------

#define VADISKADAPTER_OFFSET_VID                        0x0000
#define VADISKADAPTER_OFFSET_PID                        0x0004
#define VADISKADAPTER_OFFSET_VERSION                    0x0008

#define VADISKADAPTER_OFFSET_CTRLSTAT                   0x0010
#define VADISKADAPTER_OFFSET_INTSTAT                    0x0014
#define VADISKADAPTER_OFFSET_INTMASK                    0x0018
#define VADISKADAPTER_OFFSET_INTPEND                    0x001C

#define VADISKADAPTER_OFFSET_INTSETMASK                 0x0020
#define VADISKADAPTER_OFFSET_INTCLRMASK                 0x0024

#define VADISKADAPTER_OFFSET_DISKTOTALSECT              0x0030
#define VADISKADAPTER_OFFSET_SESSIONID                  0x0034
#define VADISKADAPTER_OFFSET_WINDOWCMD                  0x0038
#define VADISKADAPTER_OFFSET_WINDOWARG                  0x003C

#define VADISKADAPTER_OFFSET_WIN0_BUFOFFSET             0x0040
#define VADISKADAPTER_OFFSET_WIN0_STARTSECT             0x0044
#define VADISKADAPTER_OFFSET_WIN0_SECTRUN               0x0048

#define VADISKADAPTER_OFFSET_WIN1_BUFOFFSET             0x0050
#define VADISKADAPTER_OFFSET_WIN1_STARTSECT             0x0054
#define VADISKADAPTER_OFFSET_WIN1_SECTRUN               0x0058

#define VADISKADAPTER_OFFSET_WIN2_BUFOFFSET             0x0060
#define VADISKADAPTER_OFFSET_WIN2_STARTSECT             0x0064
#define VADISKADAPTER_OFFSET_WIN2_SECTRUN               0x0068

#define VADISKADAPTER_OFFSET_WIN3_BUFOFFSET             0x0070
#define VADISKADAPTER_OFFSET_WIN3_STARTSECT             0x0074
#define VADISKADAPTER_OFFSET_WIN3_SECTRUN               0x0078

#define VADISKADAPTER_OFFSET_MEDIAGUID_0                0x0080
#define VADISKADAPTER_OFFSET_MEDIAGUID_1                0x0084
#define VADISKADAPTER_OFFSET_MEDIAGUID_2                0x0088
#define VADISKADAPTER_OFFSET_MEDIAGUID_3                0x008C

///---------------------------------------------------------------------

#define VIRTARM_DISKADAPTER_VID                         0x0000045E
#define VIRTARM_DISKADAPTER_PID                         0x00000606

#define VIRTARM_DISKADAPTER_CTRLSTAT_ENABLE_TOGGLE      0x80000000
#define VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_PRESENCE     0x00000001
#define VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_READONLY     0x00000002

#define VIRTARM_DISKADAPTER_INTR_PRESENCE_CHANGE        0x00000001
#define VIRTARM_DISKADAPTER_INTR_ALL                    0x00000001

///---------------------------------------------------------------------

#define VIRTARM_DISKADAPTER_SRAM_OFFSET_SECTORBUFFER    0
#define VIRTARM_DISKADAPTER_SECTOR_BYTES                512
#define VIRTARM_DISKADAPTER_MAX_NUM_SECTORS             ((VIRTARM_PHYSSIZE_ADAPTER_SRAM - VIRTARM_DISKADAPTER_SRAM_OFFSET_SECTORBUFFER) / VIRTARM_DISKADAPTER_SECTOR_BYTES)

///---------------------------------------------------------------------

#define VIRTARM_DISKADAPTER_WINDOWCMD_IXMASK            0x0000000F
#define VIRTARM_DISKADAPTER_WINDOWCMD_CMDMASK           0x00000030

#define VIRTARM_DISKADAPTER_WINDOWCMD_FRAME             0x00000010
#define VIRTARM_DISKADAPTER_WINDOWCMD_FLUSH_WRITE       0x00000020

#define VIRTARM_DISKADAPTER_IOERR_NONE                  0
#define VIRTARM_DISKADAPTER_IOERR_OUTOFRANGE            0x807B0001
#define VIRTARM_DISKADAPTER_IOERR_BADCOMMAND            0x807B0002
#define VIRTARM_DISKADAPTER_IOERR_READONLY              0x807B0003

///---------------------------------------------------------------------

#endif // __VADISKADAPTERREGS_INC