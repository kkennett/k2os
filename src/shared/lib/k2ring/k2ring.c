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
#include <lib/k2ring.h>
#include <lib/k2mem.h>
#include <lib/k2atomic.h>

void
K2RING_Init(
    K2RING *    apRing,
    UINT32      aSize
)
{
    K2_ASSERT(aSize <= 0x7FFF);
    K2MEM_Zero(apRing, sizeof(K2RING));
    apRing->mSize = aSize;
}

UINT32 
K2RING_Reader_GetAvail(
    K2RING *    apRing,
    UINT32 *    apRetOffset,
    BOOL        aSetSpareOnEmpty
)
{
    K2RING_STATE    state;
    K2RING_STATE    newVal;
    UINT32          v;

    state.mAsUINT32 = apRing->State.mAsUINT32;
    *apRetOffset = state.Fields.mReadIx;

    if (state.mAsUINT32 == 0)
    {
        // buffer is empty
        if (!aSetSpareOnEmpty)
        {
            return 0;
        }

        newVal.mAsUINT32 = 0;
        newVal.Fields.mSpare = 1;
        v = K2ATOMIC_CompareExchange32(&apRing->State.mAsUINT32, newVal.mAsUINT32, 0);
        if (0 == v)
        {
            // empty and spare bit set successfully
            return 0;
        }

        // changed between read of empty and trying to set spare bit - no longer empty. 
        state.mAsUINT32 = v;
        *apRetOffset = state.Fields.mReadIx;
    }

    if (state.Fields.mReadIx <= state.Fields.mWriteIx)
    {
        return state.Fields.mWriteIx - state.Fields.mReadIx;
    }

    v = (apRing->mSize - state.Fields.mReadIx);

    if (state.Fields.mHasGap)
    {
        K2_ASSERT(state.Fields.mHasGap < v);
        v -= apRing->mWriterGap;
    }

    return v;
}

K2STAT 
K2RING_Reader_Consumed(
    K2RING *    apRing,
    UINT32      aCount
)
{
    K2RING_STATE    state;
    K2RING_STATE    newVal;
    UINT32          avail;
    BOOL            clearGap;
    UINT32          saveGap;

    if (aCount == 0)
        return K2STAT_ERROR_BAD_ARGUMENT;
    clearGap = FALSE;
    do
    {
        state.mAsUINT32 = apRing->State.mAsUINT32;
        newVal.mAsUINT32 = state.mAsUINT32;
        if (state.Fields.mReadIx <= state.Fields.mWriteIx)
        {
            //
            // read is behind write.  gap must be empty
            //
            K2_ASSERT(state.Fields.mHasGap == 0);
            K2_ASSERT(apRing->mWriterGap == 0);
            avail = state.Fields.mWriteIx - state.Fields.mReadIx;
            if (aCount > avail)
                return K2STAT_ERROR_TOO_BIG;
            if (aCount == avail)
            {
                // ate everything that was left.  clear to empty
                newVal.mAsUINT32 = 0;
            }
            else
            {
                // just move the read state
                newVal.Fields.mReadIx = state.Fields.mReadIx + aCount;
            }
        }
        else
        {
            //
            // write is behind read
            //
            avail = (apRing->mSize - state.Fields.mReadIx);
            if (state.Fields.mHasGap)
            {
                K2_ASSERT(state.Fields.mHasGap < avail);
                avail -= apRing->mWriterGap;
                if (aCount > avail)
                    return K2STAT_ERROR_TOO_BIG;
                if (avail == aCount)
                {
                    // wrap to start and clear gap if read successful
                    newVal.Fields.mReadIx = 0;
                    newVal.Fields.mHasGap = 0;
                    clearGap = TRUE;
                }
                else
                {
                    // just move the read state
                    newVal.Fields.mReadIx = state.Fields.mReadIx + aCount;
                }
            }
            else
            {
                // write is behind read and no gap
                K2_ASSERT(apRing->mWriterGap == 0);
                if (aCount > avail)
                    return K2STAT_ERROR_TOO_BIG;
                if (aCount == avail)
                {
                    // eating to end of buffer. wrap the read state
                    newVal.Fields.mReadIx = 0;
                }
                else
                {
                    // just move the read state
                    newVal.Fields.mReadIx = state.Fields.mReadIx + aCount;
                }
            }
        }

        if (clearGap)
        {
            saveGap = apRing->mWriterGap;
            apRing->mWriterGap = 0;
        }

        if (state.mAsUINT32 == K2ATOMIC_CompareExchange32(&apRing->State.mAsUINT32, newVal.mAsUINT32, state.mAsUINT32))
        {
            break;
        }

        if (clearGap)
        {
            apRing->mWriterGap = saveGap;
        }
    } while (1);

    return K2STAT_NO_ERROR;
}

BOOL   
K2RING_Writer_GetOffset(
    K2RING *    apRing,
    UINT32      aCount,
    UINT32 *    apRetOffset
)
{
    K2RING_STATE    state;
    UINT32          avail;

    state.mAsUINT32 = apRing->State.mAsUINT32;

    if (state.Fields.mReadIx <= state.Fields.mWriteIx)
    {
        avail = (apRing->mSize - 1) - state.Fields.mWriteIx;
        if (aCount <= avail)
        {
            *apRetOffset = state.Fields.mWriteIx;
            return TRUE;
        }
        if (state.Fields.mReadIx > aCount)
        {
            *apRetOffset = 0;
            return TRUE;
        }
    }
    else
    {
        avail = (state.Fields.mReadIx - 1) - state.Fields.mWriteIx;
        if (avail >= aCount)
        {
            *apRetOffset = state.Fields.mWriteIx;
            return TRUE;
        }
    }

    // write won't fit
    return FALSE;
}

K2STAT 
K2RING_Writer_Wrote(
    K2RING *    apRing,
    UINT32      aWroteAtOffset,
    UINT32      aCount
)
{
    K2RING_STATE    state;
    K2RING_STATE    newVal;
    UINT32          setGap;

    if (aCount == 0)
        return K2STAT_ERROR_BAD_ARGUMENT;
    do
    {
        state.mAsUINT32 = apRing->State.mAsUINT32;
        newVal.mAsUINT32 = state.mAsUINT32;
        setGap = 0;

        if (aWroteAtOffset == 0)
        {
            if (0 != state.Fields.mWriteIx)
            {
                // set gap
                if (aCount >= state.Fields.mReadIx)
                    return K2STAT_ERROR_CORRUPTED;
                setGap = apRing->mSize - state.Fields.mWriteIx;
                newVal.Fields.mHasGap = 1;
            }
            newVal.Fields.mWriteIx = aCount;
        }
        else
        {
            if (aWroteAtOffset != apRing->State.Fields.mWriteIx)
                return K2STAT_ERROR_UNKNOWN; //  K2STAT_ERROR_CORRUPTED;
            newVal.Fields.mWriteIx = state.Fields.mWriteIx + aCount;
        }

        if (setGap != 0)
        {
            K2_ASSERT(state.Fields.mHasGap == 0);
            K2_ASSERT(newVal.Fields.mHasGap == 1);
            apRing->mWriterGap = setGap;
        }

        if (state.mAsUINT32 == K2ATOMIC_CompareExchange32(&apRing->State.mAsUINT32, newVal.mAsUINT32, state.mAsUINT32))
        {
            break;
        }

        if (setGap != 0)
        {
            apRing->mWriterGap = 0;
        }

    } while (1);

    return K2STAT_NO_ERROR;
}

void   
K2RING_ClearSpareBit(
    K2RING *    apRing
)
{
    K2RING_STATE state;
    state.mAsUINT32 = 0;
    state.Fields.mSpare = 1;
    K2ATOMIC_And32(&apRing->State.mAsUINT32, ~state.mAsUINT32);
}
