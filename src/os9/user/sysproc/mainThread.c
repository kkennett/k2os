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
#include "sysproc.h"
#include "../../kern/main/kerniface.h"

K2OS_NOTIFY_TOKEN   gTokKernNotify;
K2OS_SYSPROC_PAGE * gpNotifyPage;
K2OS_MSG const *    gpNotifyMsgBuf;

void
SysProc_SetupNotify(
    void
)
{
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));

    gTokKernNotify = (K2OS_NOTIFY_TOKEN)K2OS_Kern_SysCall1(K2OS_SYSCALL_ID_SYSPROC_REGISTER, 0);
    K2_ASSERT(NULL != gTokKernNotify);

    gpNotifyPage = (K2OS_SYSPROC_PAGE *)pThreadPage->mSysCall_Arg7_Result0;
    gpNotifyMsgBuf = (K2OS_MSG const *)pThreadPage->mSysCall_Arg6_Result1;
}

void
SysProc_ProcessOneMsgFromKernel(
    K2OS_MSG const * apMsg
)
{
    Debug_Printf("SYSPROC: Recv <%d.%d> Msg from Kernel\n", apMsg->mType, apMsg->mShort);
}

void
SysProc_ProcessKernelNotifies(
    void
)
{
    UINT32      ixCons;
    UINT32      ixProd;
    UINT32      wordIndex;
    UINT32      bitIndex;
    UINT32      ixNext;
    K2OS_MSG    recvMsg;
    UINT32      bitField;
    BOOL        kernelClearedBit;

    do {
        ixCons = gpNotifyPage->IxConsumer.mVal;
        K2_CpuReadBarrier();

        if (0 != (ixCons & K2OS_SYSPROC_NOTIFY_EMPTY_BIT))
        {
            //
            // nothing to consume
            // 
            return;
        }

        //
        // sleep flag was clear when we checked it so there is 
        // a message available to consume
        //
        wordIndex = ixCons >> 5;
        bitIndex = ixCons & 0x1F;
        ixNext = (ixCons + 1) & K2OS_SYSPROC_NOTIFY_MSG_IX_MASK;

        //
        // should be a message available as this thread is the only receiver
        // in the whole system
        //
        bitField = gpNotifyPage->OwnerMask[wordIndex].mVal;
        K2_CpuReadBarrier();
        if (0 != (bitField & (1 << bitIndex)))
        {
            kernelClearedBit = FALSE;

            //
            // if this is the last message try to set the sleeping bit
            //
            ixProd = gpNotifyPage->IxProducer.mVal;
            K2_CpuReadBarrier();

            if (ixProd == ixNext)
            {
                //
                // from what we can see, buffer will go empty at update. so set the 
                // sleep flag as we update
                // ** this is the only place in the system where this bit can be set
                //
                K2ATOMIC_Exchange(&gpNotifyPage->IxConsumer.mVal, ixNext | K2OS_SYSPROC_NOTIFY_EMPTY_BIT);

                //
                // if somebody produced then we have to make sure that the sleeping bit is clear
                //
                ixProd = gpNotifyPage->IxProducer.mVal;
                K2_CpuReadBarrier();

                if (ixProd != ixNext)
                {
                    //
                    // somebody produced while we were setting the sleeping bit so make sure it is clear
                    // 
                    do {
                        ixCons = gpNotifyPage->IxConsumer.mVal;
                        K2_CpuReadBarrier();
                        if (0 == (ixCons & K2OS_SYSPROC_NOTIFY_EMPTY_BIT))
                        {
                            kernelClearedBit = TRUE;
                            break;
                        }
                    } while (ixCons != K2ATOMIC_CompareExchange(&gpNotifyPage->IxConsumer.mVal, ixCons & ~K2OS_SYSPROC_NOTIFY_EMPTY_BIT, ixCons));

                    //
                    // sleeping bit is definitely clear here.  either we cleared it
                    // or the kernel did when a message was produced (kernelClearedBit case)
                    //
                }
            }
            else
            {
                //
                // buffer will not go empty at update. eat it. yummy
                //
                K2ATOMIC_Exchange(&gpNotifyPage->IxConsumer.mVal, ixNext);
            }

            //
            // we received a message from the ixCons slot
            // so copy it out and clear the ownership bit
            // so it can be produced into again
            //
            K2MEM_Copy(&recvMsg, &gpNotifyMsgBuf[ixCons], sizeof(K2OS_MSG));
            K2ATOMIC_And(&gpNotifyPage->OwnerMask[wordIndex].mVal, ~(1 << bitIndex));

            SysProc_ProcessOneMsgFromKernel(&recvMsg);

            if (kernelClearedBit)
            {
                //
                // kernel cleared the sleeping bit when a message was produced
                // but it also set the notify.  we exit here because we need 
                // to eat the notify using a wait before we consume the next
                // message
                //
                return;
            }
        }
        // go around
    } while (1);
}

UINT32
MainThread(
    char const *apArgs
)
{
    K2OS_WaitResult waitResult;

    Debug_Printf("SysProc::MainThread()\n");

    SysProc_SetupNotify();

    do {
        if (K2OS_Thread_WaitOne(&waitResult, gTokKernNotify, 1000))
        {
            SysProc_ProcessKernelNotifies();
        }
        Debug_Printf("Tick\n");

    } while (1);

    K2OS_RaiseException(K2STAT_EX_LOGIC);

    return 0;
}

