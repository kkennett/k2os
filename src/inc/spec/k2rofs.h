#ifndef __SPEC_K2ROFS_H
#define __SPEC_K2ROFS_H

#include <k2basetype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------- */

#define K2ROFS_SECTOR_BYTES 512

typedef struct _K2ROFS_FILE K2ROFS_FILE;
typedef struct _K2ROFS_DIR  K2ROFS_DIR;
typedef struct _K2ROFS      K2ROFS;

K2_PACKED_PUSH
struct _K2ROFS
{
    UINT32  mSectorCount;       // in entire ROFS
    UINT32  mRootDir;           // byte offset from K2ROFS to K2ROFS_DIR of root
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

K2_PACKED_PUSH
struct _K2ROFS_DIR
{
    UINT32  mName;              // offset in stringfield
    UINT32  mSubCount;          // number of subdirectories
    UINT32  mFileCount;         // number of files
    // K2ROFS_FILE * mFileCount
    // K2ROFS_DIR offsets from K2ROFS * mSubCount
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

K2_PACKED_PUSH
struct _K2ROFS_FILE
{
    UINT32  mName;              // offset in stringfield
    UINT32  mTime;              // dos datetime
    UINT32  mStartSectorOffset; // sector offset from ROFS
    UINT32  mSizeBytes;         // round up to 512 and divide to get sector count
} K2_PACKED_ATTRIB;
K2_PACKED_POP;

/* ------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // __SPEC_K2ROFS_H

