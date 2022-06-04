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

    IF  !:DEF: __VARAW32DISPLAYADAPTERREGS_INC
__VARAW32DISPLAYADAPTERREGS_INC EQU 1

;;/------------------------------------------------------------------------------

VARAW32DISPLAYADAPTER_OFFSET_VID            EQU 0x0000
VARAW32DISPLAYADAPTER_OFFSET_PID            EQU 0x0004
VARAW32DISPLAYADAPTER_OFFSET_VERSION        EQU 0x0008

VARAW32DISPLAYADAPTER_OFFSET_WIDTH          EQU 0x0010
VARAW32DISPLAYADAPTER_OFFSET_HEIGHT         EQU 0x0014
VARAW32DISPLAYADAPTER_OFFSET_CONFIG         EQU 0x0018
VARAW32DISPLAYADAPTER_OFFSET_PHYSPIXADDR    EQU 0x001C

;;/---------------------------------------------------------------------

VIRTARM_RAW32DISPLAYADAPTER_VID             EQU 0x0000045E
VIRTARM_RAW32DISPLAYADAPTER_PID             EQU 0x00000808

;;/---------------------------------------------------------------------

VIRTARM_RAW32DISPLAYADAPTER_REGS_CONFIG_ENABLE        EQU   0x00000001

;;/------------------------------------------------------------------------------

    ENDIF    ; __VARAW32DISPLAYADAPTERREGS_INC

    END 
