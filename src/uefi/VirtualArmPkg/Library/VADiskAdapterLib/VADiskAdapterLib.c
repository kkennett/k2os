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

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/VADiskAdapterLib.h>

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
EFI_STATUS
diskIo(volatile VIRTARM_DISKADAPTER_REGS *apAdapter, BOOLEAN aIsWrite, UINT32 aStartSector, UINTN aNumSectors, UINT8 *apBuffer, BOOLEAN aDoFlushAtEnd)
{
    EFI_STATUS status;
    UINT8 * pSectorBuffer;
    UINT8 * pCurSector;
    UINT8 * pEndOfWindowSectors;
    UINT32  max;
    UINT32  sectorsLeftInWindowFromCurPos;
    UINT32  bytesTransferredNow;
    UINT32  sessionId;
    UINT32  curChunkOfSectors;

    if (!aNumSectors)
        return 0;

    /* store the session id so we know if the disk changes (is removed/reinserted) */
    sessionId = apAdapter->mSessionId;

    /* just use window 0 */
    max = apAdapter->mDiskTotalSectors;
    if (!max)
        return EFI_NO_RESPONSE;

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
        status = apAdapter->mWindowCmd;
        if (0 != status)
            return status;

        /* window 0 registers updated */
        /* as well, windowarg now points to absolute offset of sector in SRAM */
        pCurSector = pSectorBuffer + apAdapter->mWindowArg;

        /* calc address after last sector in window - this will be somewhere after pCurSector */
        pEndOfWindowSectors = pSectorBuffer + apAdapter->mWindow0BufferOffset;
        pEndOfWindowSectors += (VIRTARM_DISKADAPTER_SECTOR_BYTES * apAdapter->mWindow0SectorRun);

        /* sanity check */
        if (pEndOfWindowSectors < pCurSector)
            return EFI_DEVICE_ERROR;

        /* make sure disk has not changed since read started */
        if (apAdapter->mSessionId != sessionId)
            return EFI_DEVICE_ERROR;

        /* now we can find out how much is left in the window from the sector we asked for */
        sectorsLeftInWindowFromCurPos = ((UINTN)(pEndOfWindowSectors - pCurSector)) / VIRTARM_DISKADAPTER_SECTOR_BYTES;

        /* sanity check */
        if (!sectorsLeftInWindowFromCurPos)
            return EFI_DEVICE_ERROR;

        /* in this set of the window position, how much can we transfer to the user buffer? */
        if (aNumSectors > sectorsLeftInWindowFromCurPos)
            curChunkOfSectors = sectorsLeftInWindowFromCurPos;
        else
            curChunkOfSectors = aNumSectors;

        /* copy from adapter ram to user buffer */
        bytesTransferredNow = VIRTARM_DISKADAPTER_SECTOR_BYTES * curChunkOfSectors;
        if (aIsWrite)
            CopyMem(pCurSector, apBuffer, bytesTransferredNow);
        else
            CopyMem(apBuffer, pCurSector, bytesTransferredNow);
        apBuffer += bytesTransferredNow;

        /* update sector position for the next window position set */
        aNumSectors -= curChunkOfSectors;
        aStartSector += curChunkOfSectors;

    } while (aNumSectors);

    if (0 == status)
    {
        if ((aIsWrite) && (aDoFlushAtEnd))
        {
            apAdapter->mWindowArg = 0;
            apAdapter->mWindowCmd = VIRTARM_DISKADAPTER_WINDOWCMD_FLUSH_WRITE;
            status = apAdapter->mWindowCmd;
        }
    }

    return status;
}

EFI_STATUS VIRTARMDISK_ReadSectors(volatile VIRTARM_DISKADAPTER_REGS *apAdapter, UINT32 aStartSector, UINTN aNumSectors, UINT8 *apBuffer)
{
    if (!apAdapter)
        return EFI_NOT_FOUND;

    if (!(apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_PRESENCE))
        return EFI_NO_MEDIA;

    return diskIo(apAdapter, FALSE, aStartSector, aNumSectors, apBuffer, FALSE);
}

EFI_STATUS VIRTARMDISK_WriteSectors(volatile VIRTARM_DISKADAPTER_REGS *apAdapter, UINT32 aStartSector, UINTN aNumSectors, UINT8 *apBuffer, BOOLEAN aDoFlushAtEnd)
{
    if (!apAdapter)
        return EFI_NOT_FOUND;

    if (!(apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_PRESENCE))
        return EFI_NO_MEDIA;

    if (apAdapter->mControlStatus & VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_READONLY)
        return EFI_WRITE_PROTECTED;

    return diskIo(apAdapter, TRUE, aStartSector, aNumSectors, apBuffer, aDoFlushAtEnd);
}

