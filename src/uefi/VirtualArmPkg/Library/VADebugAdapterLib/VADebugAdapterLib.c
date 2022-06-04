
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/VADebugAdapterLib.h>

void VIRTARMDEBUG_Init(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, BOOLEAN aUseAdapterBuffer, BOOLEAN aSetKitlMode)
{
    if (!apAdapter)
        return;

    /* clear all pending interrupts and mask them all */
    apAdapter->mInterrupt = VIRTARM_DEBUGADAPTER_INTR_PEND_ALL |
        VIRTARM_DEBUGADAPTER_INTR_MASKSET_ALL;

    /* toggle messagebuffer state if we need to */
    if (aUseAdapterBuffer)
    {
        /* if it is off, toggle it */
        if (!(apAdapter->mControlAndStatus & VIRTARM_DEBUGADAPTER_CTRLSTAT_MESSAGEBUFFER))
            apAdapter->mControlAndStatus = VIRTARM_DEBUGADAPTER_CTRLSTAT_MESSAGEBUFFER;
    }
    else
    {
        /* if it is on, toggle it */
        if (apAdapter->mControlAndStatus & VIRTARM_DEBUGADAPTER_CTRLSTAT_MESSAGEBUFFER)
            apAdapter->mControlAndStatus = VIRTARM_DEBUGADAPTER_CTRLSTAT_MESSAGEBUFFER;
    }

    VIRTARMDEBUG_SetKitlMode(apAdapter, aSetKitlMode);
}

void VIRTARMDEBUG_SetKitlMode(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, BOOLEAN aSetKitlMode)
{
    if (!apAdapter)
        return;

    /* toggle kitl mode if we need to */
    if (aSetKitlMode)
    {
        /* if it is off, toggle it */
        if (!(apAdapter->mControlAndStatus & VIRTARM_DEBUGADAPTER_CTRLSTAT_KITLMODE))
            apAdapter->mControlAndStatus = VIRTARM_DEBUGADAPTER_CTRLSTAT_KITLMODE;
    }
    else
    {
        /* if it is on, toggle it */
        if (apAdapter->mControlAndStatus & VIRTARM_DEBUGADAPTER_CTRLSTAT_KITLMODE)
            apAdapter->mControlAndStatus = VIRTARM_DEBUGADAPTER_CTRLSTAT_KITLMODE;
    }
}

void VIRTARMDEBUG_OutputString(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, CHAR16 const *apString, UINTN aStrLen)
{
    UINT32 ctrlStat;
    UINT8 * pTarget;

    if ((!apAdapter) || (!aStrLen))
        return;

    ctrlStat = apAdapter->mControlAndStatus;
    if (!(ctrlStat & VIRTARM_DEBUGADAPTER_CTRLSTAT_CONSOLE_PRESENT))
    {
        /* nobody is going to see what we are going to send, so don't bother sending it */
        return;
    }

    /* debug buffer ram is 4k, which is max 2048 WCHARs */
    if (aStrLen > 2047)
        aStrLen = 2047;

    if (ctrlStat & VIRTARM_DEBUGADAPTER_CTRLSTAT_MESSAGEBUFFER)
    {
        /* get a pointer to the adapter's RAM buffer */
        pTarget = (UINT8 *)apAdapter;
        pTarget -= VIRTARM_OFFSET_ADAPTER_REGS;
        pTarget += VIRTARM_OFFSET_ADAPTER_SRAM;
        pTarget += VIRTARM_DEBUGADAPTER_SRAM_OFFSET_MESSAGEBUFFER;

        CopyMem(pTarget, apString, aStrLen * sizeof(CHAR16));
        apAdapter->mDebugOut = aStrLen;
    }
    else
    {
        do {
            apAdapter->mDebugOut = *apString;
            apString++;
        } while (--aStrLen);
    }
}

void VIRTARMDEBUG_OutputStringZ(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, CHAR16 const *apStringZ)
{
    UINT32 ctrlStat;
    UINTN strLen;

    if ((!apAdapter) || (!apStringZ))
        return;

    ctrlStat = apAdapter->mControlAndStatus;
    if (!(ctrlStat & VIRTARM_DEBUGADAPTER_CTRLSTAT_CONSOLE_PRESENT))
    {
        /* nobody is going to see what we are going to send, so don't bother sending it */
        return;
    }

    strLen = StrLen(apStringZ);
    VIRTARMDEBUG_OutputString(apAdapter, apStringZ, strLen);
}

UINT32 VIRTARMDEBUG_ReadPROM(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT32 aAddress)
{
    if ((!apAdapter) || (aAddress >= VIRTARM_DEBUGADAPTER_SIZE_PROM_BYTES))
        return 0;
    apAdapter->mPROMAddr = aAddress;
    return apAdapter->mPROMData;
}

void VIRTARMDEBUG_WritePROM(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT32 aAddress, UINT32 aData)
{
    if ((!apAdapter) || (aAddress >= VIRTARM_DEBUGADAPTER_SIZE_PROM_BYTES))
        return;
    apAdapter->mPROMAddr = aAddress;
    apAdapter->mPROMData = aData;
}

UINT8 * VIRTARMDEBUG_GetXmitPacketBuffer(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT32 aDataBytesToSend)
{
    UINT8 * pTarget;
    UINT32  sramOffset;

    if ((!apAdapter) || (!aDataBytesToSend))
        return NULL;

    apAdapter->mIOArgument = aDataBytesToSend;
    apAdapter->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_GETXMITBUF;
    if (apAdapter->mIOCommand)
        return NULL;    /* error retrieving buffer for xmit */

    sramOffset = apAdapter->mIOArgument;
    if ((sramOffset < VIRTARM_DEBUGADAPTER_SRAM_OFFSET_PACKETBUFFER) ||
        (sramOffset >= VIRTARM_PHYSSIZE_ADAPTER_SRAM))
    {
        /* retrieved buffer offset is not valid */
        apAdapter->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_RELEASE;
        return NULL;
    }

    /* return a pointer to the adapter SRAM packet */
    pTarget = (UINT8 *)apAdapter;
    pTarget -= VIRTARM_OFFSET_ADAPTER_REGS;
    pTarget += VIRTARM_OFFSET_ADAPTER_SRAM;

    return pTarget + sramOffset;
}

UINT32 VIRTARMDEBUG_SendPacket(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT8 *apPacketBuffer)
{
    UINT32 sramOffset;

    if ((!apAdapter) || (!apPacketBuffer))
        return VIRTARM_DEBUGADAPTER_IOERR_BADFRAMEPTR;

    /* convert adapter ptr to adapter sram ptr */
    sramOffset = (UINT32)apAdapter;
    sramOffset -= VIRTARM_OFFSET_ADAPTER_REGS;
    sramOffset += VIRTARM_OFFSET_ADAPTER_SRAM;

    sramOffset = ((UINT32)apPacketBuffer) - sramOffset;

    if ((sramOffset < VIRTARM_DEBUGADAPTER_SRAM_OFFSET_PACKETBUFFER) ||
        (sramOffset >= VIRTARM_PHYSSIZE_ADAPTER_SRAM))
    {
        /* buffer pointer is not inside SRAM packet buffer area for this adapter */
        return VIRTARM_DEBUGADAPTER_IOERR_BADFRAMEPTR;
    }

    apAdapter->mIOArgument = sramOffset;
    apAdapter->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_DOXMIT;

    return apAdapter->mIOCommand;
}

BOOLEAN VIRTARMDEBUG_IsRecvPacketAvail(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter)
{
    if (!apAdapter)
        return FALSE;
    return (apAdapter->mInterrupt & VIRTARM_DEBUGADAPTER_INTR_STAT_PACKET_RECV);
}

UINT8 * VIRTARMDEBUG_GetRecvPacketBuffer(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT32 *apRetDataBytesRecv)
{
    UINT32 bytesRecv;
    UINT8 *pTarget;
    UINT32 sramOffset;

    if ((!apAdapter) || (!apRetDataBytesRecv))
        return NULL;

    if (!VIRTARMDEBUG_IsRecvPacketAvail(apAdapter))
        return NULL;

    apAdapter->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_GETRECVBUF;
    bytesRecv = apAdapter->mIOCommand;
    if (bytesRecv & 0x80000000)
        return NULL;
    if (!bytesRecv)
    {
        /* apAdapter->mIOArgument is sram offset to (empty) received frame.
           this should not have been delivered.  just release it */
        apAdapter->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_RELEASE;
        return NULL;
    }

    *apRetDataBytesRecv = bytesRecv;

    sramOffset = apAdapter->mIOArgument;
    if ((sramOffset < VIRTARM_DEBUGADAPTER_SRAM_OFFSET_PACKETBUFFER) ||
        (sramOffset >= VIRTARM_PHYSSIZE_ADAPTER_SRAM))
    {
        /* retrieved buffer offset is not valid */
        apAdapter->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_RELEASE;
        return NULL;
    }

    /* return a pointer to the adapter SRAM packet */
    pTarget = (UINT8 *)apAdapter;
    pTarget -= VIRTARM_OFFSET_ADAPTER_REGS;
    pTarget += VIRTARM_OFFSET_ADAPTER_SRAM;

    return pTarget + sramOffset;
}

UINT32 VIRTARMDEBUG_ReleaseBuffer(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT8 *apPacketBuffer)
{
    UINT32 sramOffset;

    if ((!apAdapter) || (!apPacketBuffer))
        return VIRTARM_DEBUGADAPTER_IOERR_BADFRAMEPTR;

    /* convert adapter ptr to adapter sram ptr */
    sramOffset = (UINT32)apAdapter;
    sramOffset -= VIRTARM_OFFSET_ADAPTER_REGS;
    sramOffset += VIRTARM_OFFSET_ADAPTER_SRAM;

    sramOffset = ((UINT32)apPacketBuffer) - sramOffset;
    if ((sramOffset < VIRTARM_DEBUGADAPTER_SRAM_OFFSET_PACKETBUFFER) ||
        (sramOffset >= VIRTARM_PHYSSIZE_ADAPTER_SRAM))
    {
        /* buffer pointer is not inside SRAM packet buffer area for this adapter */
        return VIRTARM_DEBUGADAPTER_IOERR_BADFRAMEPTR;
    }

    apAdapter->mIOArgument = sramOffset;
    apAdapter->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_RELEASE;

    return apAdapter->mIOCommand;
}

void VIRTARMDEBUG_ClearPacketBuffers(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter)
{
    if (!apAdapter)
        return;
    apAdapter->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_CLEARBUFFERS;
}

