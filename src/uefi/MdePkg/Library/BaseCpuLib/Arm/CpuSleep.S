#------------------------------------------------------------------------------
#
# CpuSleep() for ARMv7
#
# ARMv6 versions was:
#    MOV r0,#0
#    MCR p15,0,r0,c7,c0,4   ;Wait for Interrupt instruction
#
# But this is a no-op on ARMv7
#
# Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
# Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

.text
.p2align 2
GCC_ASM_EXPORT(CpuSleep)

#/**
#  Places the CPU in a sleep state until an interrupt is received.
#
#  Places the CPU in a sleep state until an interrupt is received. If interrupts
#  are disabled prior to calling this function, then the CPU will be placed in a
#  sleep state indefinitely.
#
#**/
#VOID
#EFIAPI
#CpuSleep (
#  VOID
#  );
#
ASM_PFX(CpuSleep):
    wfi
    bx  lr
