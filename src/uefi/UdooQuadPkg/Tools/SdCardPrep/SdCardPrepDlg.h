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

// SdCardPrepDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CSdCardPrepDlg dialog
class CSdCardPrepDlg : public CDialogEx
{
// Construction
public:
	CSdCardPrepDlg(CWnd* pParent = NULL);	// standard constructor
    ~CSdCardPrepDlg(void);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SDCARDPREP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
    void EndModalLoop(int nResult);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnAdviseNotify(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
    CComboBox mDiskSel;
    CButton mButCancel;
    CButton mButOK;

    struct VdsCallback *mpCallback;
    CRITICAL_SECTION mSec;
    DiskEntry *mpDisks;
    bool mDoRefresh;

    CRITICAL_SECTION mNotifySec;
    struct NotifyEntry *mpNotify;
    UINT mOrigWidth;
    bool mLockoutRefresh;

private:
    struct DiskEntry * Locked_BuildDiskList(void);
    void Locked_Refresh(void);
    void Locked_SelectSomething(void);
    void Locked_SelectNothing(void);
    void OnRefresh(void);

public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnCbnSelchangeDroplistDisksel();
    CEdit mEditPath;
    CButton mButBrowse;
    afx_msg void OnBnClickedButBrowse();
    CButton mCheckForceClean;
    CButton mButEject;
    afx_msg void OnClickedButEject();
};
