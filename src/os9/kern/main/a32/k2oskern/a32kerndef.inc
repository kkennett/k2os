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

#ifndef __A32KERNDEF_INC
#define __A32KERNDEF_INC

#include "../../kerndef.inc"

/* --------------------------------------------------------------------------------- */

#define A32KERN_CORE_IRQ_STACK_BYTES                        1152
#define A32KERN_CORE_ABT_STACK_BYTES                        256
#define A32KERN_CORE_UND_STACK_BYTES                        256
#define A32KERN_CORE_SVC_STACK_BYTES                        512

#define A32KERN_EXCEPTION_REASON_UNDEFINED_INSTRUCTION      1
#define A32KERN_EXCEPTION_REASON_SYSTEM_CALL                2
#define A32KERN_EXCEPTION_REASON_PREFETCH_ABORT             3
#define A32KERN_EXCEPTION_REASON_DATA_ABORT                 4
#define A32KERN_EXCEPTION_REASON_IRQ                        5
#define A32KERN_EXCEPTION_REASON_RAISE_EXCEPTION            6

#define A32KERN_MAX_IRQ                                     256

//
// TTB must be 16KB aligned physically. best to do it virtually too
//
// 0xFFFF0000 Vector Page
//       1000 PeriphBase (SCU, INTC, Global and private timers)
//       2000 GICD
//       3000 AP start Transition Page
//       4000 AP start TTB page 0
//       5000 AP start TTB page 1
//       6000 AP start TTB page 2
//       7000 AP start TTB page 3
//       8000 AP Start pagetable
//
#define A32KERN_APSTART_TRANSIT_PAGE_VIRT                   K2OS_KVA_A32_AP_TRANSIT
#define A32KERN_APSTART_TTB_VIRT                            K2OS_KVA_A32_AP_TTB
#define A32KERN_APSTART_PAGETABLE_VIRT                      K2OS_KVA_A32_AP_PAGETABLE
#define A32KERN_APSTART_PARKPAGE_VIRT                       K2OS_KVA_A32_AP_COREPARK_PAGE

#define A32KERN_MP_CONFIGBASE_MAP_VIRT                      K2OS_KVA_A32_PERIPHBASE
#define A32KERN_MP_SCU_VIRT                                 (A32KERN_MP_CONFIGBASE_MAP_VIRT + A32_PERIPH_OFFSET_SCU)
#define A32KERN_MP_GICC_VIRT                                (A32KERN_MP_CONFIGBASE_MAP_VIRT + A32_PERIPH_OFFSET_GICC)
#define A32KERN_MP_GLOBAL_TIMER_VIRT                        (A32KERN_MP_CONFIGBASE_MAP_VIRT + A32_PERIPH_OFFSET_GLOBAL_TIMER)
#define A32KERN_MP_PRIVATE_TIMERS_VIRT                      (A32KERN_MP_CONFIGBASE_MAP_VIRT + A32_PERIPH_OFFSET_PRIVATE_TIMERS)
#define A32KERN_MP_GICD_VIRT                                (A32KERN_MP_CONFIGBASE_MAP_VIRT + A32_PERIPH_OFFSET_GICD)

#define A32_MP_GTIMER_COUNTLOW_VIRT                         (A32KERN_MP_GLOBAL_TIMER_VIRT + A32_PERIF_GTIMER_OFFSET_COUNTLOW)
#define A32_MP_GTIMER_COUNTHIGH_VIRT                        (A32KERN_MP_GLOBAL_TIMER_VIRT + A32_PERIF_GTIMER_OFFSET_COUNTHIGH)
#define A32_MP_GTIMER_CONTROL_VIRT                          (A32KERN_MP_GLOBAL_TIMER_VIRT + A32_PERIF_GTIMER_OFFSET_CONTROL)
#define A32_MP_GTIMER_INTSTATUS_VIRT                        (A32KERN_MP_GLOBAL_TIMER_VIRT + A32_PERIF_GTIMER_OFFSET_INTSTATUS)
#define A32_MP_GTIMER_COMPVALLOW_VIRT                       (A32KERN_MP_GLOBAL_TIMER_VIRT + A32_PERIF_GTIMER_OFFSET_COMPVALLOW)
#define A32_MP_GTIMER_COMPVALHIGH_VIRT                      (A32KERN_MP_GLOBAL_TIMER_VIRT + A32_PERIF_GTIMER_OFFSET_COMPVALHIGH)
#define A32_MP_GTIMER_AUTOINC_VIRT                          (A32KERN_MP_GLOBAL_TIMER_VIRT + A32_PERIF_GTIMER_OFFSET_AUTOINC)

#define A32_MP_PTIMERS_LOAD_VIRT                            (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_LOAD)
#define A32_MP_PTIMERS_COUNTER_VIRT                         (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_COUNTER)
#define A32_MP_PTIMERS_CONTROL_VIRT                         (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_CONTROL)
#define A32_MP_PTIMERS_INTSTATUS_VIRT                       (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_INTSTATUS)
#define A32_MP_PTIMERS_WATCHDOGLOAD_VIRT                    (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_WATCHDOGLOAD)
#define A32_MP_PTIMERS_WATCHDOGCOUNTER_VIRT                 (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_WATCHDOGCOUNTER)
#define A32_MP_PTIMERS_WATCHDOGCONTROL_VIRT                 (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_WATCHDOGCONTROL)
#define A32_MP_PTIMERS_WATCHDOGINTSTATUS_VIRT               (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_WATCHDOGINTSTATUS)
#define A32_MP_PTIMERS_WATCHDOGRESETSTATUS_VIRT             (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_WATCHDOGRESETSTATUS)
#define A32_MP_PTIMERS_WATCHDOGDISABLE_VIRT                 (A32KERN_MP_PRIVATE_TIMERS_VIRT + A32_PERIF_PTIMERS_OFFSET_WATCHDOGDISABLE)

/* --------------------------------------------------------------------------------- */

#endif // __A32KERNDEF_INC
