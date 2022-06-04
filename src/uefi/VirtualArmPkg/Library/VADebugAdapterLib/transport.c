
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/VADebugAdapterLib.h>

static volatile VIRTARM_DEBUGADAPTER_REGS *sgpAdapter = NULL;

void VIRTARMDEBUG_TRANSPORT_SetAdapter(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter)
{
    sgpAdapter = apAdapter;
}

BOOLEAN VIRTARMDEBUG_TRANSPORT_Init(BOOLEAN fSetKitlMode)
{
    if (!sgpAdapter)
        return FALSE;
    VIRTARMDEBUG_SetKitlMode(sgpAdapter, fSetKitlMode);
    return TRUE;
}

void VIRTARMDEBUG_TRANSPORT_DeInit(void)
{
    sgpAdapter = NULL;
}

UINT16 VIRTARMDEBUG_TRANSPORT_SendFrame(UINT8 *apBuffer, UINT16 aLength)
{
    UINT8 *pBuf;
    UINT32 err;

    if (!sgpAdapter)
        return 0;

    pBuf = VIRTARMDEBUG_GetXmitPacketBuffer(sgpAdapter, aLength);
    if (!pBuf)
        return 0;
    
    CopyMem(pBuf,apBuffer,aLength);
    
    err = VIRTARMDEBUG_SendPacket(sgpAdapter, pBuf);

    return err ? 0 : aLength;
}

UINT16 VIRTARMDEBUG_TRANSPORT_GetFrame(UINT8 *apBuffer, UINT16 aMaxLength)
{
    UINT32  dataLen;
    UINT8 * pBuf;

    if (!sgpAdapter)
        return 0;

    if (!VIRTARMDEBUG_IsRecvPacketAvail(sgpAdapter))
        return 0;

    dataLen = 0;
    pBuf = VIRTARMDEBUG_GetRecvPacketBuffer(sgpAdapter, &dataLen);
    if (!pBuf)
        return 0;

    if (dataLen > aMaxLength)
        dataLen = aMaxLength;

    CopyMem(apBuffer, pBuf, dataLen);

    VIRTARMDEBUG_ReleaseBuffer(sgpAdapter,pBuf);

    /* if this was the last packet in the receive queue, then
       the raw interrupt will have been deasserted.  we need to ack pending
       here so that the adapter will deassert its interrupt line */
    sgpAdapter->mInterrupt = VIRTARM_DEBUGADAPTER_INTR_PEND_PACKET_RECV;

    return dataLen;
}

void VIRTARMDEBUG_TRANSPORT_CurrentPacketFilter(UINTN aFilter)
{

}

