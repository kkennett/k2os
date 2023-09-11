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

#include <lib/k2spin.h>

#define N_MASK              0xFFFFFFFC
#define N_ONE               0x00000004
#define READER_FLAG         0x00000002
#define WRITER_FLAG         0x00000001
#define WRITER_END_OF_LIST  0x00000003
#define FLAG_MASK           0x00000003

void
K2SPIN_ReadLock(
    UINT_PTR volatile *apLock
)
{
    UINT_PTR volatile me;
    UINT_PTR v;

    do {
        v = *apLock;
        K2_CpuReadBarrier();
        if (0 == v)
        {
            // lock is free
            if (v == K2ATOMIC_CompareExchange(apLock, N_ONE | READER_FLAG, v))
                return;
        }
        else if (READER_FLAG == (v & FLAG_MASK))
        {
            // readers only
            if (v == K2ATOMIC_CompareExchange(apLock, (v + N_ONE), v))
                return;
        }
        else
        {
            // writers active or waiting - put us on the chain
            me = (v & N_MASK) | READER_FLAG;  // i am a reader
            K2_CpuWriteBarrier();
            if (v == K2ATOMIC_CompareExchange(apLock, ((UINT_PTR)&me) | (READER_FLAG | WRITER_FLAG), v))
            {
                // on chain. wait for somebody to release me
                while (me != 0)
                {
                    K2_CpuYield();
                }
                return;
            }
        }
    } while (1);
}

void
K2SPIN_WriteLock(
    UINT_PTR volatile *apLock
)
{
    UINT_PTR volatile me;
    UINT_PTR v;

    do {
        v = *apLock;
        K2_CpuReadBarrier();
        switch (v & FLAG_MASK)
        {
        case 0:
            // lock is free 
            if (0 == K2ATOMIC_CompareExchange(apLock, WRITER_FLAG, 0))
                return;
            break;

        case WRITER_FLAG:
            // no readers, writer(s) active. add to chain
            me = (v & N_MASK);  // v is 0 or pointer to first waiting writer
            if (0 == me)
                me |= WRITER_END_OF_LIST;    // 'end' of the chain (no readers holding lock)
            else
                me |= WRITER_FLAG;    // not end of the chain (some other writer is waiting)
            K2_CpuWriteBarrier();
            if (v == K2ATOMIC_CompareExchange(apLock, ((UINT_PTR)&me) | WRITER_FLAG, v))
            {
                // on chain. wait for somebody to release me
                while (me != 0)
                {
                    K2_CpuYield();
                }
                return;
            }
            break;

        case READER_FLAG:
            // readers active, no writers, add to chain
            me = (v & N_MASK) | WRITER_END_OF_LIST;  // v is count of readers, set 'end' of the chain
            K2_CpuWriteBarrier();
            if (v == K2ATOMIC_CompareExchange(apLock, ((UINT_PTR)&me) | (READER_FLAG | WRITER_FLAG), v))
            {
                // on chain. wait for somebody to release me
                while (me != 0)
                {
                    K2_CpuYield();
                }
                return;
            }
            break;

        case (READER_FLAG | WRITER_FLAG):
            me = (v & N_MASK) | WRITER_FLAG; // v is chain, there is something already there
            K2_CpuWriteBarrier();
            if (v == K2ATOMIC_CompareExchange(apLock, ((UINT_PTR)&me) | (READER_FLAG | WRITER_FLAG), v))
            {
                // on chain. wait for somebody to release me
                while (me != 0)
                {
                    K2_CpuYield();
                }
                return;
            }
            break;

        default:
            K2_ASSERT(0);
            return;
        }
    } while (1);
}

void
K2SPIN_ReadUnlock(
    UINT_PTR volatile *apLock
)
{
    UINT_PTR v;
    UINT_PTR t;
    UINT_PTR p;
    UINT_PTR rCount;
    UINT_PTR volatile * pScan;
    UINT_PTR volatile * pPrev;

    do {
        v = *apLock;
        K2_CpuReadBarrier();
        t = (v & FLAG_MASK);

        K2_ASSERT(0 != (t & READER_FLAG));    // reader must be active

        if (0 == (t & WRITER_FLAG))
        {
            // 1 0 - only readers active.  N is count
            K2_ASSERT(v >= (N_ONE | READER_FLAG));
            t = v - N_ONE;
            if (0 == (t & N_MASK))
            {
                t = 0;
            }
            if (v == K2ATOMIC_CompareExchange(apLock, t, v))
                return;
        }
        else
        {
            // writers are waiting on the chain
            // last thing on chain must be a writer.
            // it has its low bits set to 3.  
            // it's N holds count of readers still holding lock
            pScan = (UINT_PTR volatile *)(v & N_MASK);
            pPrev = NULL;
            rCount = 0;
            do {
                t = *pScan;
                K2_CpuReadBarrier();
                if (READER_FLAG == (t & FLAG_MASK))
                {
                    // this is a reader that is waiting
                    rCount++;
                }
                if (WRITER_END_OF_LIST == (t & FLAG_MASK))
                {
                    //
                    // this is the writer at the end of the list
                    //
                    break;
                }
                pPrev = pScan;
                p = t;
                pScan = (UINT_PTR volatile *)(t & N_MASK);
            } while (1);

            K2_ASSERT(t > FLAG_MASK);   // last writer must have a readers count > 0

            if ((t - N_ONE) != FLAG_MASK)
            {
                // still more readers that need to unlock
                if (t == K2ATOMIC_CompareExchange(pScan, t - N_ONE, t))
                    return;
            }
            else
            {
                // last reader is unlocking and there is a writer waiting
                if (NULL == pPrev)
                {
                    // list only holds one writer
                    // so to update we reset the chain pointer and set only writer active
                    // then release the writer that is waiting
                    K2_ASSERT(rCount == 0);
                    if (v == K2ATOMIC_CompareExchange(apLock, WRITER_FLAG, v))
                    {
                        *pScan = 0;
                        K2_CpuWriteBarrier();
                        return;
                    }
                }
                else
                {
                    // pPrev points to the value p, n of which is the pointer pScan
                    // pScan points to the value t, the end of the list which is a writer
                    // that holds the value 7
                    if (0 != (p & WRITER_FLAG))
                    {
                        // prev is another writer - set it as the new end of the list and cleave off the writer
                        // and then release it
                        if (p == K2ATOMIC_CompareExchange(pPrev, WRITER_END_OF_LIST, p))
                        {
                            *pScan = 0;
                            K2_CpuWriteBarrier();
                            return;
                        }
                    }
                    else
                    {
                        // prev is a reader
                        if (p == K2ATOMIC_CompareExchange(pPrev, READER_FLAG, p))
                        {
                            *pScan = 0;
                            K2_CpuWriteBarrier();
                            return;
                        }
                    }
                }
            }
        }
    } while (1);
}

void
K2SPIN_WriteUnlock(
    UINT_PTR volatile *apLock
)
{
    UINT_PTR v;
    UINT_PTR t;
    UINT_PTR p;
    UINT_PTR pw;
    UINT_PTR rCount;
    UINT_PTR volatile *pScan;
    UINT_PTR volatile *pPrev;
    UINT_PTR volatile *pPrevWrite;

    do {
        v = *apLock;
        K2_CpuReadBarrier();
        K2_ASSERT(0 != (v & WRITER_FLAG));    // writer must be active

        if (v == WRITER_FLAG)
        {
            // writer held lock with no contention
            if (WRITER_FLAG == K2ATOMIC_CompareExchange(apLock, 0, WRITER_FLAG))
                return;
        }
        else
        {
            // contention occurred (chain pointer is nonzero)
            if (0 == (v & READER_FLAG))
            {
                // no readers are waiting but writer(s) are
                pScan = (UINT_PTR volatile *)(v & N_MASK);
                pPrev = NULL;
                do {
                    t = *pScan;
                    K2_CpuReadBarrier();
                    if (WRITER_END_OF_LIST == (t & FLAG_MASK))
                        break;
                    p = t;
                    pPrev = pScan;
                    pScan = (UINT_PTR volatile *)(t & N_MASK);
                } while (1);

                K2_ASSERT(t == WRITER_END_OF_LIST);
                if (NULL == pPrev)
                {
                    // only one writer was waiting on the chain
                    // so reset to just one writer holding the lock
                    // and release it
                    if (v == K2ATOMIC_CompareExchange(apLock, WRITER_FLAG, v))
                    {
                        *pScan = 0;
                        K2_CpuWriteBarrier();
                        return;
                    }
                }
                else
                {
                    // more than one writer was waiting on the chain
                    // so cut off the one we are releasing and set the new
                    // end of the chain
                    if (p == K2ATOMIC_CompareExchange(pPrev, WRITER_END_OF_LIST, p))
                    {
                        *pScan = 0;
                        K2_CpuWriteBarrier();
                        return;
                    }
                }
            }
            else
            {
                // readers waiting and possibly writers too
                // if the last thing on the list is a reader it will have a null next pointer
                // if the last thing on the list is a writer it will have both bits set (3)
                rCount = 0;
                pScan = (UINT_PTR volatile *)(v & N_MASK);
                pPrevWrite = NULL;
                pPrev = NULL;
                do {
                    t = *pScan;
                    K2_CpuReadBarrier();
                    if (READER_FLAG == (t & FLAG_MASK))
                    {
                        // this is a reader
                        rCount++;
                        if (0 == (t & N_MASK))
                        {
                            // end of the list is a reader and there are rCount of them that
                            // we will release at once
                            if (NULL == pPrevWrite)
                            {
                                // everything on the list is a reader so convert the lock
                                // value to a readers count.
                                if (v == K2ATOMIC_CompareExchange(apLock, (rCount * N_ONE) | READER_FLAG, v))
                                {
                                    // release them all 
                                    pPrev = (UINT_PTR volatile *)(v & N_MASK);
                                    do {
                                        pScan = pPrev;
                                        pPrev = (UINT_PTR volatile *)((*pPrev) & N_MASK);
                                        *pScan = 0;
                                        K2_CpuWriteBarrier();
                                    } while (NULL != pPrev);
                                    return;
                                }
                                break;
                            }

                            // not everything on the list is a reader so the previous write
                            // needs to take the read-lock count and we need to cleave the list
                            // there and release all the readers that go at once
                            if (pw == K2ATOMIC_CompareExchange(pPrevWrite, (rCount * N_ONE) | WRITER_END_OF_LIST, pw))
                            {
                                // release all the readers from *pw onwards
                                pPrev = (UINT_PTR volatile *)(pw & N_MASK);
                                do {
                                    pScan = pPrev;
                                    pPrev = (UINT_PTR volatile *)((*pPrev) & N_MASK);
                                    *pScan = 0;
                                    K2_CpuWriteBarrier();
                                } while (NULL != pPrev);
                                return;
                            }
                            break;
                        }
                    }
                    else
                    {
                        // this is a writer.
                        K2_ASSERT(NULL != pPrev);
                        if (WRITER_END_OF_LIST == (t & FLAG_MASK))
                        {
                            // this is the end of the list. pPrev cannot be NULL because otherwise
                            // bit 1 would not be set in v
                            if (0 != (p & WRITER_FLAG))
                            {
                                // prev is another writer. set it as end of the list
                                if (p == K2ATOMIC_CompareExchange(pPrev, WRITER_END_OF_LIST, p))
                                {
                                    *pScan = 0;
                                    K2_CpuWriteBarrier();
                                    return;
                                }
                            }
                            else
                            {
                                // prev is a reader 
                                if (NULL == pPrevWrite)
                                {
                                    // everything on the list is a reader so we're going to release
                                    // them all at once
                                    if (v == K2ATOMIC_CompareExchange(apLock, (rCount * N_ONE) | READER_FLAG, v))
                                    {
                                        pPrev = (UINT_PTR volatile *)(v & N_MASK);
                                        do {
                                            pScan = pPrev;
                                            pPrev = (UINT_PTR volatile *)((*pPrev) & N_MASK);
                                            *pScan = 0;
                                            K2_CpuWriteBarrier();
                                        } while (NULL != pPrev);
                                        return;
                                    }
                                }
                                else
                                {
                                    // there are writers on the list so we don't have to update v
                                    // (still both readers and writers active)
                                    if (p == K2ATOMIC_CompareExchange(pPrev, READER_FLAG, p))
                                    {
                                        *pScan = 0;
                                        K2_CpuWriteBarrier();
                                        return;
                                    }
                                }
                            }
                            break;
                        }

                        // this is a writer but it isn't the end of the list
                        // hold onto it and reset the 'reads after this write' rCount
                        pw = t;
                        pPrevWrite = pScan;
                        rCount = 0;
                    }
                    p = t;
                    pPrev = pScan;
                    pScan = (UINT_PTR volatile *)(t & N_MASK);
                } while (1);
            }
        }
    } while (1);
}
