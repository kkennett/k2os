;;
;; Copyright (c) Microsoft Corporation.  All rights reserved.
;;
;; Use of this sample source code is subject to the terms of the Microsoft
;; license agreement under which you licensed this sample source code. If
;; you did not accept the terms of the license agreement, you are not
;; authorized to use this sample source code. For the terms of the license,
;; please see the license agreement between you and Microsoft or, if applicable,
;; see the LICENSE.RTF on your install media or the root of your tools installation.
;; THE SAMPLE SOURCE CODE IS PROVIDED 'AS IS', WITH NO WARRANTIES OR INDEMNITIES.
;;
;; Code Originator:  Kurt Kennett
;; Last Updated By:  Kurt Kennett
;;

    IF  !:DEF: __VABLITADAPTERREGS_INC
__VABLITADAPTERREGS_INC EQU 1

;;/------------------------------------------------------------------------------

VABLITADAPTER_OFFSET_VID            EQU 0x0000
VABLITADAPTER_OFFSET_PID            EQU 0x0004
VABLITADAPTER_OFFSET_VERSION        EQU 0x0008

VABLITADAPTER_OFFSET_WIDTH          EQU 0x0010
VABLITADAPTER_OFFSET_HEIGHT         EQU 0x0014
VABLITADAPTER_OFFSET_CONFIG         EQU 0x0018
VABLITADAPTER_OFFSET_COMMAND        EQU 0x001C

VABLITADAPTER_OFFSET_PHYSPIXADDR    EQU 0x0020

;;/---------------------------------------------------------------------

VIRTARM_BLITADAPTER_VID             EQU 0x0000045E
VIRTARM_BLITADAPTER_PID             EQU 0x00000303

;;/---------------------------------------------------------------------

VIRTARM_BLITADAPTER_SRAM_OFFSET_CMDBUF      EQU 0

VIRTARM_BLITADAPTER_CMDBUF_OFFSET_LEFT      EQU 0x0000
VIRTARM_BLITADAPTER_CMDBUF_OFFSET_TOP       EQU 0x0004
VIRTARM_BLITADAPTER_CMDBUF_OFFSET_RIGHT     EQU 0x0008
VIRTARM_BLITADAPTER_CMDBUF_OFFSET_BOTTOM    EQU 0x000C
VIRTARM_BLITADAPTER_CMDBUF_OFFSET_ARG0      EQU 0x0010
VIRTARM_BLITADAPTER_CMDBUF_OFFSET_ARG1      EQU 0x0014

;;/---------------------------------------------------------------------

VIRTARM_BLITADAPTER_REGS_CONFIG_ENABLE        EQU   0x00000001
VIRTARM_BLITADAPTER_REGS_COMMAND_UPDATERECT   EQU   0x00000001
VIRTARM_BLITADAPTER_REGS_COMMAND_SOLIDRECT    EQU   0x00000002

;;/------------------------------------------------------------------------------

    ENDIF    ; __VABLITADAPTERREGS_INC

    END 
