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

#include "virtarm_diskadapter.h"

void VIRTARMDISK_Init(volatile VIRTARM_DISKADAPTER_REGS *apAdapter)
{
    if (!apAdapter)
        return;

    /* if it is enabled disable it */
    if (apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_ENABLE_TOGGLE)
        apAdapter->mControlStatus = VIRTARM_DISKADAPTER_CTRLSTAT_ENABLE_TOGGLE;

    /* now if it is disabled enable it */
    if (!(apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_ENABLE_TOGGLE))
        apAdapter->mControlStatus = VIRTARM_DISKADAPTER_CTRLSTAT_ENABLE_TOGGLE;

    /* mask all ints */
    apAdapter->mIntSetMask = VIRTARM_DISKADAPTER_INTR_ALL;

    /* clear pending ints */
    apAdapter->mIntPend = VIRTARM_DISKADAPTER_INTR_ALL;
}

UINT32 VIRTARMDISK_GetMaxNumSectors(volatile VIRTARM_DISKADAPTER_REGS *apAdapter)
{
    if (!apAdapter)
        return 0;

    if (!(apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_PRESENCE))
        return 0;

    return apAdapter->mDiskTotalSectors;
}

static 
HRESULT diskIo(volatile VIRTARM_DISKADAPTER_REGS *apAdapter, BOOL aIsWrite, UINT32 aStartSector, UINT aNumSectors, UINT8 *apBuffer, BOOL aDoFlushAtEnd)
{
    HRESULT hr;
    UINT8 * pSectorBuffer;
    UINT8 * pCurSector;
    UINT8 * pEndOfWindowSectors;
    UINT32  max;
    UINT32  sectorsLeftInWindowFromCurPos;
    UINT32  bytesTransferredNow;
    UINT32  sessionId;
    UINT32  curChunkOfSectors;

    if (!apAdapter)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (!(apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_PRESENCE))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (!aNumSectors)
        return 0;

    /* store the session id so we know if the disk changes (is removed/reinserted) */
    sessionId = apAdapter->mSessionId;

    /* just use window 0 */
    max = apAdapter->mDiskTotalSectors;
    if (!max)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if ((aStartSector >= max) ||
        ((max - aStartSector) < aNumSectors))
        return VIRTARM_DISKADAPTER_IOERR_OUTOFRANGE;

    /* get a pointer to the first byte of the adapter's RAM */
    pSectorBuffer = (UINT8 *)apAdapter;
    pSectorBuffer -= VIRTARM_OFFSET_ADAPTER_REGS;
    pSectorBuffer += VIRTARM_OFFSET_ADAPTER_SRAM;
    pSectorBuffer += VIRTARM_DISKADAPTER_SRAM_OFFSET_SECTORBUFFER;

    do {
        /* command - frame the next sector somewhere into window 0 */
        apAdapter->mWindowArg = aStartSector;
        apAdapter->mWindowCmd = VIRTARM_DISKADAPTER_WINDOWCMD_FRAME | 0;

        /* get result of the command */
        hr = apAdapter->mWindowCmd;
        if (FAILED(hr))
            return hr;

        /* window 0 registers updated */
        /* as well, windowarg now points to absolute offset of sector in SRAM */
        pCurSector = pSectorBuffer + apAdapter->mWindowArg;

        /* calc address after last sector in window - this will be somewhere after pCurSector */
        pEndOfWindowSectors = pSectorBuffer + apAdapter->mWindow0BufferOffset;
        pEndOfWindowSectors += (VIRTARM_DISKADAPTER_SECTOR_BYTES * apAdapter->mWindow0SectorRun);

        /* sanity check */
        if (pEndOfWindowSectors < pCurSector)
            return E_FAIL;

        /* make sure disk has not changed since read started */
        if (apAdapter->mSessionId != sessionId)
            return E_FAIL; 

        /* now we can find out how much is left in the window from the sector we asked for */
        sectorsLeftInWindowFromCurPos = ((UINT)(pEndOfWindowSectors - pCurSector)) / VIRTARM_DISKADAPTER_SECTOR_BYTES;

        /* sanity check */
        if (!sectorsLeftInWindowFromCurPos)
            return E_FAIL;

        /* in this set of the window position, how much can we transfer to the user buffer? */
        if (aNumSectors > sectorsLeftInWindowFromCurPos)
            curChunkOfSectors = sectorsLeftInWindowFromCurPos;
        else
            curChunkOfSectors = aNumSectors;

        /* copy from adapter ram to user buffer */
        bytesTransferredNow = VIRTARM_DISKADAPTER_SECTOR_BYTES * curChunkOfSectors;
        if (aIsWrite)
            CopyMemory(pCurSector, apBuffer, bytesTransferredNow);
        else
            CopyMemory(apBuffer, pCurSector, bytesTransferredNow);
        apBuffer += bytesTransferredNow;

        /* update sector position for the next window position set */
        aNumSectors -= curChunkOfSectors;
        aStartSector += curChunkOfSectors;

    } while (aNumSectors);

    if (!FAILED(hr))
    {
        if ((aIsWrite) && (aDoFlushAtEnd))
        {
            apAdapter->mWindowArg = 0;
            apAdapter->mWindowCmd = VIRTARM_DISKADAPTER_WINDOWCMD_FLUSH_WRITE;
            hr = apAdapter->mWindowCmd;
        }
    }

    return hr;
}

HRESULT VIRTARMDISK_ReadSectors(volatile VIRTARM_DISKADAPTER_REGS *apAdapter, UINT32 aStartSector, UINT aNumSectors, UINT8 *apBuffer)
{
    return diskIo(apAdapter, FALSE, aStartSector, aNumSectors, apBuffer, FALSE);
}

HRESULT VIRTARMDISK_WriteSectors(volatile VIRTARM_DISKADAPTER_REGS *apAdapter, UINT32 aStartSector, UINT aNumSectors, UINT8 *apBuffer, BOOL aDoFlushAtEnd)
{
    if (!apAdapter)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (!(apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_PRESENCE))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_READONLY)
        return STG_E_DISKISWRITEPROTECTED;

    return diskIo(apAdapter, TRUE, aStartSector, aNumSectors, apBuffer, aDoFlushAtEnd);
}

