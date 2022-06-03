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

    IF  !:DEF: __VATOUCHADAPTERREGS_INC
__VATOUCHADAPTERREGS_INC EQU 1

;;/------------------------------------------------------------------------------

VATOUCHADAPTER_OFFSET_VID            EQU 0x0000
VATOUCHADAPTER_OFFSET_PID            EQU 0x0004
VATOUCHADAPTER_OFFSET_VERSION        EQU 0x0008

VATOUCHADAPTER_OFFSET_WIDTH          EQU 0x0010
VATOUCHADAPTER_OFFSET_HEIGHT         EQU 0x0014
VATOUCHADAPTER_OFFSET_CURRENT        EQU 0x0018

;;/---------------------------------------------------------------------

VIRTARM_TOUCHADAPTER_VID              EQU 0x0000045E
VIRTARM_TOUCHADAPTER_PID              EQU 0x00000404

;;/---------------------------------------------------------------------

VIRTARM_TOUCHADAPTER_REGS_CURRENT_DOWN    EQU 0x80000000
VIRTARM_TOUCHADAPTER_REGS_CURRENT_XMASK   EQU 0x00000FFF
VIRTARM_TOUCHADAPTER_REGS_CURRENT_XSHIFT  EQU 0
VIRTARM_TOUCHADAPTER_REGS_CURRENT_YMASK   EQU 0x0FFF0000
VIRTARM_TOUCHADAPTER_REGS_CURRENT_YSHIFT  EQU 16

;;/------------------------------------------------------------------------------

    ENDIF    ; __VATOUCHADAPTERREGS_INC

    END 
