//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2023, Kurt Kennett
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
    apRing->State.Fields.mEmpty = 1;
    K2_CpuWriteBarrier();
}

UINT32
K2RING_Reader_Internal(
    K2RING *    apRing,
    UINT32 *    apRetOffset,
    UINT32 *    apRetState
)
{
    K2RING_STATE    state;
    UINT32          spaceToEnd;
    UINT32          gapPos;

    state.mAsUINT32 = apRing->State.mAsUINT32;
    K2_CpuReadBarrier();
    if (NULL != apRetState)
    {
        *apRetState = state.mAsUINT32;
    }

    if (state.Fields.mEmpty)
    {
        return 0;
    }

    *apRetOffset = state.Fields.mReadIx;

    if (state.Fields.mReadIx < state.Fields.mWriteIx)
    {
        //
        // simplest case when not empty
        //
        return (UINT32)(state.Fields.mWriteIx - state.Fields.mReadIx);
    }

    //
    // read is equal to or greater than write
    //
    spaceToEnd = apRing->mSize - state.Fields.mReadIx;

    if (0 == state.Fields.mHasGap)
    {
        //
        // no gap, so available block goes exactly to the end
        //
        return spaceToEnd;
    }

    //
    // gap must be nonzero
    //
    K2_ASSERT(0 != apRing->mGapSize);

    // 
    // calc where the gap starts
    //
    gapPos = apRing->mSize - apRing->mGapSize;

    //
    // if read is at the gap start, then write pos
    // must not be at zero, and the size of data
    // available at zero must be greater than the
    // size of the gap (or the gap wouldn't exist)
    //
    if (state.Fields.mReadIx == gapPos)
    {
        K2_ASSERT(state.Fields.mWriteIx > 0);
        K2_ASSERT(state.Fields.mWriteIx > apRing->mGapSize);
        *apRetOffset = 0;
        return state.Fields.mWriteIx;
    }

    return gapPos - state.Fields.mReadIx;
}

UINT32 
K2RING_Reader_GetAvail(
    K2RING *    apRing,
    UINT32 *    apRetOffset
)
{
    return K2RING_Reader_Internal(apRing, apRetOffset, NULL);
}

K2STAT 
K2RING_Reader_Consumed(
    K2RING *    apRing,
    UINT32      aCount
)
{
    K2RING_STATE    state;
    K2RING_STATE    newState;
    UINT32          offset;
    UINT32          avail;
    UINT32          gapPos;

    do {
        avail = K2RING_Reader_Internal(apRing, &offset, (UINT32 *)&state.mAsUINT32);
        K2_ASSERT(0 == state.Fields.mEmpty);
        K2_ASSERT(avail >= aCount);
        newState.mAsUINT32 = state.mAsUINT32;

        if (state.Fields.mHasGap)
        {
            gapPos = apRing->mSize - apRing->mGapSize;
            if (state.Fields.mReadIx == gapPos)
            {
                //
                // read index is at the gap, so avail must be
                // at the start of the buffer (less than write index)
                // and our update will clear the gap
                //
                K2_ASSERT(aCount <= state.Fields.mWriteIx);
                newState.Fields.mReadIx = aCount;
                newState.Fields.mHasGap = 0;
                if (state.Fields.mWriteIx == aCount)
                {
                    newState.Fields.mEmpty = 1;
                }
                apRing->mGapSize = 0;
                if (state.mAsUINT32 == K2ATOMIC_CompareExchange(&apRing->State.mAsUINT32, newState.mAsUINT32, state.mAsUINT32))
                {
                    break;
                }
                // put this back
                apRing->mGapSize = apRing->mSize - gapPos;
                // go around and try again
            }
            else
            {
                //
                // read is not at the gap, so the count must be less
                // than the space between where the read cursor is
                // and where the gap starts
                //
                K2_ASSERT((gapPos - state.Fields.mReadIx) >= aCount);
                newState.Fields.mReadIx += aCount;
                if (state.mAsUINT32 == K2ATOMIC_CompareExchange(&apRing->State.mAsUINT32, newState.mAsUINT32, state.mAsUINT32))
                {
                    break;
                }
                // go around and try again
            }
        }
        else
        {
            //
            // there is no gap
            //
            if (state.Fields.mReadIx < state.Fields.mWriteIx)
            {
                K2_ASSERT(aCount <= avail);
                if (aCount == avail)
                {
                    newState.Fields.mEmpty = 1;
                }
                newState.Fields.mReadIx += aCount;
            }
            else
            {
                //
                // read is >= write
                //
                if (aCount == (apRing->mSize - state.Fields.mReadIx))
                {
                    //
                    // exact wrap with no gap
                    //
                    newState.Fields.mReadIx = 0;
                    if (state.Fields.mWriteIx == 0)
                    {
                        newState.Fields.mEmpty = 1;
                    }
                }
                else
                {
                    //
                    // must be less than space or there would be a gap block
                    //
                    K2_ASSERT(aCount < (apRing->mSize - state.Fields.mReadIx));
                    newState.Fields.mReadIx += aCount;
                }
            }
            if (state.mAsUINT32 == K2ATOMIC_CompareExchange(&apRing->State.mAsUINT32, newState.mAsUINT32, state.mAsUINT32))
            {
                break;
            }
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
    K2_CpuReadBarrier();

    if (0 != state.Fields.mEmpty)
    {
        K2_ASSERT(aCount <= apRing->mSize);
        apRing->mWriteCount = aCount;
        apRing->mWritePos = *apRetOffset = 0;
        return TRUE;
    }

    //
    // ring is nonempty
    //
    *apRetOffset = state.Fields.mWriteIx;

    if (state.Fields.mReadIx <= state.Fields.mWriteIx)
    {
        avail = apRing->mSize - state.Fields.mWriteIx;
        if (aCount <= avail)
        {
            apRing->mWriteCount = aCount;
            apRing->mWritePos = *apRetOffset;
            return TRUE;
        }
        if (state.Fields.mReadIx >= aCount)
        {
            apRing->mWriteCount = aCount;
            apRing->mWritePos = *apRetOffset = 0;
            return TRUE;
        }
    }
    else
    {
        avail = state.Fields.mReadIx - state.Fields.mWriteIx;
        if (avail >= aCount)
        {
            apRing->mWriteCount = aCount;
            apRing->mWritePos = *apRetOffset;
            return TRUE;
        }
    }

    // write won't fit
    return FALSE;
}

K2STAT 
K2RING_Writer_Wrote(
    K2RING *    apRing
)
{
    K2RING_STATE    state;
    K2RING_STATE    newState;
    BOOL            undoGap;
    UINT32          spaceLeft;

    if (apRing->mWriteCount == 0)
        return K2STAT_ERROR_API_ORDER;
    
    undoGap = FALSE;

    do {
        if (undoGap)
        {
            apRing->mGapSize = 0;
            undoGap = FALSE;
        }
        state.mAsUINT32 = apRing->State.mAsUINT32;
        K2_CpuReadBarrier();
        newState.mAsUINT32 = state.mAsUINT32;
        newState.Fields.mEmpty = 0;

        if (state.Fields.mEmpty)
        {
            if (0 == apRing->mWritePos)
            {
                newState.Fields.mReadIx = 0;
                newState.Fields.mWriteIx = apRing->mWriteCount;
            }
            else
            {
                newState.Fields.mWriteIx += apRing->mWriteCount;
            }
            if (apRing->mSize == newState.Fields.mWriteIx)
            {
                newState.Fields.mWriteIx = 0;
            }
        }
        else
        {
            if (state.Fields.mReadIx <= state.Fields.mWriteIx)
            {
                spaceLeft = apRing->mSize - state.Fields.mWriteIx;
                if (spaceLeft < apRing->mWriteCount)
                {
                    //
                    // gap write
                    //
                    K2_ASSERT(0 == apRing->mWritePos);
                    apRing->mGapSize = spaceLeft;
                    newState.Fields.mHasGap = 1;
                    newState.Fields.mWriteIx = apRing->mWriteCount;
                    undoGap = TRUE;
                }
                else
                {
                    newState.Fields.mWriteIx += apRing->mWriteCount;
                    if (apRing->mSize == newState.Fields.mWriteIx)
                    {
                        newState.Fields.mWriteIx = 0;
                    }
                }
            }
            else
            {
                //
                // read is ahead of write, read cannot be off the end
                //
                K2_ASSERT(state.Fields.mWriteIx == apRing->mWritePos);
                spaceLeft = state.Fields.mReadIx - state.Fields.mWriteIx;
                K2_ASSERT(spaceLeft >= apRing->mWriteCount);
                newState.Fields.mWriteIx += apRing->mWriteCount;
            }
        }

    } while (state.mAsUINT32 != K2ATOMIC_CompareExchange(&apRing->State.mAsUINT32, newState.mAsUINT32, state.mAsUINT32));

    apRing->mWriteCount = 0;
    K2_CpuWriteBarrier();

    return K2STAT_NO_ERROR;
}


