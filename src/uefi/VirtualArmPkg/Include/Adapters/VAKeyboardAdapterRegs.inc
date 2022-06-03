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

    IF  !:DEF: __VAKEYBOARDADAPTERREGS_INC
__VAKEYBOARDADAPTERREGS_INC EQU 1

;;/------------------------------------------------------------------------------

VAKEYBOARDADAPTER_OFFSET_VID                EQU 0x0000
VAKEYBOARDADAPTER_OFFSET_PID                EQU 0x0004
VAKEYBOARDADAPTER_OFFSET_VERSION            EQU 0x0008
VAKEYBOARDADAPTER_OFFSET_KEYDATA            EQU 0x0010
VAKEYBOARDADAPTER_OFFSET_COMMAND            EQU 0x0014
VAKEYBOARDADAPTER_OFFSET_RESULT             EQU 0x001C

;;/---------------------------------------------------------------------

VIRTARM_KEYBOARDADAPTER_VID          EQU 0x0000045E
VIRTARM_KEYBOARDADAPTER_PID          EQU 0x00000707

VIRTARM_KEYBOARDADAPTER_CMD_CLEARBUFFER 0x00000001
VIRTARM_KEYBOARDADAPTER_CMD_GETNUMAVAIL 0x00000002

;;/------------------------------------------------------------------------------

    ENDIF    ; __VAKEYBOARDADAPTERREGS_INC

    END 


