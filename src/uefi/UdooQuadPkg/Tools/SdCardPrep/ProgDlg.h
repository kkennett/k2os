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

#pragma once


// CProgDlg dialog

class CProgDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProgDlg)

public:
	CProgDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WORKER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
    afx_msg void DoNothing(void);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    bool AbortMessage(TCHAR *pMsg, ...);
    bool Iterate(void);

public:
    int mStartDelay;
    int mState;
    bool mAutoMountWasOff;
    CStatic mProgText;
    CProgressCtrl mProgBar;

private:
    bool State_FindDisk(void);

    bool State_StartPurgeVolumes(void);
    bool State_ContinuePurgeVolumes(void);

    bool State_StartDestroyDisk(void);
    bool State_ContinueDestroyDisk(void);

    bool State_CreateDisk(void);

    bool State_CreateVolume(void);

    bool State_StartCreateFileSys(void);
    bool State_ContinueCreateFileSys(void);

    void State_MakeAccessPath(void);

    bool StartPurgeOne(void);

    bool State_UpdateEFI(void);

    /* working variables for state machine */
    IEnumVdsObject *    mpWorkEnumOuter;
    IEnumVdsObject *    mpWorkEnumInner;
    IVdsAsync *         mpWorkAsync;
    IVdsVolume *        mpWorkVol;
    HANDLE              mhWorkFile;
    UINT8 *             mpWorkBuf;
    DWORD               mWorkBytes;
    DWORD               mWorkLeft;
    bool                mDoneOuter;
    bool                mDoneInner;

    void DoStuff(void);
};
