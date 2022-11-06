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

#include <lib/k2mem.h>
#include <lib/k2sort.h>

/* This implementation is taken from Paul Edward's PDPCLIB.
    Original code is credited to Raymond Gardner, Englewood CO.
    Minor mods are credited to Paul Edwards.
    Some reformatting and simplification done by Martin Baute.
    All code is still Public Domain.
*/

typedef int (*K2SORT_pf_Compare)(void const *aPtr1, void const *aPtr2);

/* For small sets, insertion sort is faster than quicksort.
   METHOD_THRESHOLD is the threshold below which insertion sort will be used.
   Must be 3 or larger.
*/
#define METHOD_THRESHOLD    7
#define STACKSIZE           64

/* Macros for handling the QSort stack */
void
K2SORT_Quick(
    void *              apArray,
    UINT_PTR            aCount,
    UINT_PTR            aEntryBytes,
    K2SORT_pf_Compare   afCompare
)
{
    UINT8 *     pEntryI;
    UINT8 *     pEntryJ;
    UINT_PTR    thresh;
    UINT8 *     workBase;
    UINT8 *     workLimit;
    UINT8 * *   pStackTop;
    UINT8 *     stackBuffer[STACKSIZE];

#define PUSH( base, limit ) pStackTop[0] = (base); pStackTop[1] = (limit); pStackTop += 2
#define POP( base, limit )  pStackTop -= 2; (base) = pStackTop[0]; (limit) = pStackTop[1]

    thresh = METHOD_THRESHOLD * aEntryBytes;
    workBase = (UINT8 *)apArray;
    workLimit = workBase + aCount * aEntryBytes;
    pStackTop = &stackBuffer[0];

    do {
        if ((UINT_PTR)(workLimit - workBase) > thresh) /* QSort for more than METHOD_THRESHOLD elements. */
        {
            /* We work from second to last - first will be pivot element. */
            pEntryI = workBase + aEntryBytes;
            pEntryJ = workLimit - aEntryBytes;
            /* We swap first with middle element, then sort that with second
               and last element so that eventually first element is the median
               of the three - avoiding pathological pivots.
               TODO: Instead of middle element, chose one randomly.
            */
            K2MEM_Swap(((((UINT64)(workLimit - workBase)) / aEntryBytes) / 2) * aEntryBytes + workBase, workBase, aEntryBytes);
            if (afCompare(pEntryI, pEntryJ) > 0) 
                K2MEM_Swap(pEntryI, pEntryJ, aEntryBytes);
            if (afCompare(workBase, pEntryJ) > 0) 
                K2MEM_Swap(workBase, pEntryJ, aEntryBytes);
            if (afCompare(pEntryI, workBase) > 0) 
                K2MEM_Swap(pEntryI, workBase, aEntryBytes);
            /* Now we have the median for pivot element, entering main Quicksort. */
            for (;; )
            {
                do
                {
                    /* move pEntryI right until *pEntryI >= pivot */
                    pEntryI += aEntryBytes;
                } while (afCompare(pEntryI, workBase) < 0);
                do
                {
                    /* move pEntryJ left until *pEntryJ <= pivot */
                    pEntryJ -= aEntryBytes;
                } while (afCompare(pEntryJ, workBase) > 0);
                if (pEntryI > pEntryJ)
                {
                    /* break loop if pointers crossed */
                    break;
                }
                /* else swap elements, keep scanning */
                K2MEM_Swap(pEntryI, pEntryJ, aEntryBytes);
            }
            /* move pivot into correct place */
            K2MEM_Swap(workBase, pEntryJ, aEntryBytes);
            /* larger subfile base / workLimit to stackBuffer, sort smaller */
            if (pEntryJ - workBase > workLimit - pEntryI)
            {
                /* left is larger */
                PUSH(workBase, pEntryJ);
                workBase = pEntryI;
            }
            else
            {
                /* right is larger */
                PUSH(pEntryI, workLimit);
                workLimit = pEntryJ;
            }
        }
        else 
        {
            //
            // insertion sort for less than METHOD_THRESHOLD elements
            //
            for (pEntryJ = workBase, pEntryI = pEntryJ + aEntryBytes; pEntryI < workLimit; pEntryJ = pEntryI, pEntryI += aEntryBytes)
            {
                for (; afCompare(pEntryJ, pEntryJ + aEntryBytes) > 0; pEntryJ -= aEntryBytes)
                {
                    K2MEM_Swap(pEntryJ, pEntryJ + aEntryBytes, aEntryBytes);
                    if (pEntryJ == workBase)
                    {
                        break;
                    }
                }
            }
            if (pStackTop != stackBuffer)
            {
                POP(workBase, workLimit);
            }
            else
            {
                break;
            }
        }
    } while (1);
}
