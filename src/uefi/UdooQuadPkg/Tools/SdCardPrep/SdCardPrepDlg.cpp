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

// SdCardPrepDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SdCardPrep.h"
#include "SdCardPrepDlg.h"
#include "afxdialogex.h"
#include "ProgDlg.h"

#include <mmsystem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// -----------------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

static TCHAR *sgNoDriveText = TEXT("<No Usable Disks Found>");

// CSdCardPrepDlg dialog
#define WM_VDS_NOTIFY    WM_USER + 103

struct VdsCallback : public IVdsAdviseSink
{
    VdsCallback(CSdCardPrepDlg *pParent)
    {
        mpParent = pParent;
        mRefs = 1;
        mCookie = 0;
    }
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject)
    {
        if ((riid==IID_IUnknown) ||
            (riid==IID_IVdsAdviseSink))
        {
            AddRef();
            *ppvObject = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    {
        return InterlockedIncrement(&mRefs);
    }

    virtual ULONG STDMETHODCALLTYPE Release( void)
    {
        ULONG ret = InterlockedDecrement(&mRefs);
        if (!ret)
            delete this;
        return ret;
    }

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnNotify( 
        /* [range][in] */ LONG lNumberOfNotifications,
        /* [size_is][in] */ __RPC__in_ecount_full(lNumberOfNotifications) VDS_NOTIFICATION *pNotificationArray)
    {
        if (lNumberOfNotifications<=0)
            return E_FAIL;

        /* allocate and copy a tracking buffer */
        UINT actBytes = (sizeof(VDS_NOTIFICATION)*(lNumberOfNotifications-1)) + sizeof(NotifyEntry);
        NotifyEntry *pNewNot = (NotifyEntry *)LocalAlloc(LPTR,actBytes);
        if (!pNewNot)
            return E_OUTOFMEMORY;

        pNewNot->mpNext = NULL;
        pNewNot->mNum = lNumberOfNotifications;

        CopyMemory(pNewNot->mEntries,pNotificationArray,sizeof(VDS_NOTIFICATION)*lNumberOfNotifications);

        EnterCriticalSection(&mpParent->mNotifySec);
        NotifyEntry *pPrev = NULL;
        NotifyEntry *pEnd = mpParent->mpNotify;
        while (pEnd)
        {
            pPrev = pEnd;
            pEnd = pEnd->mpNext;
        }
        if (!pPrev)
            mpParent->mpNotify = pNewNot;
        else
            pPrev->mpNext = pNewNot;
        LeaveCriticalSection(&mpParent->mNotifySec);

        PostMessage(mpParent->m_hWnd, WM_VDS_NOTIFY,0,0);

        return S_OK;
    }

    CSdCardPrepDlg *mpParent;
    LONG mRefs;
    DWORD mCookie;
};

CSdCardPrepDlg::CSdCardPrepDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SDCARDPREP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    InitializeCriticalSection(&mSec);
    mpDisks = NULL;
    mDoRefresh = true;
    InitializeCriticalSection(&mNotifySec);
    mpNotify = NULL;
    mOrigWidth = 0;
    mLockoutRefresh = false;
}

CSdCardPrepDlg::~CSdCardPrepDlg(void)
{
    DeleteCriticalSection(&mSec);
    EnterCriticalSection(&mNotifySec);
    while (mpNotify)
    {
        NotifyEntry *pKill = mpNotify;
        mpNotify = mpNotify->mpNext;
        LocalFree((HLOCAL)pKill);
    }
    DeleteCriticalSection(&mNotifySec);
}

void CSdCardPrepDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DROPLIST_DISKSEL, mDiskSel);
    DDX_Control(pDX, IDCANCEL, mButCancel);
    DDX_Control(pDX, IDOK, mButOK);
    DDX_Control(pDX, IDC_EDIT_PATH, mEditPath);
    DDX_Control(pDX, IDC_BUT_BROWSE, mButBrowse);
    DDX_Control(pDX, IDC_CHECK_FORCE_CLEAN, mCheckForceClean);
    DDX_Control(pDX, IDC_BUTEJECT, mButEject);
}

BEGIN_MESSAGE_MAP(CSdCardPrepDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
    ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_VDS_NOTIFY, OnAdviseNotify)
    ON_BN_CLICKED(IDOK, &CSdCardPrepDlg::OnBnClickedOk)
    ON_CBN_SELCHANGE(IDC_DROPLIST_DISKSEL, &CSdCardPrepDlg::OnCbnSelchangeDroplistDisksel)
    ON_BN_CLICKED(IDC_BUT_BROWSE, &CSdCardPrepDlg::OnBnClickedButBrowse)
    ON_BN_CLICKED(IDC_BUTEJECT, &CSdCardPrepDlg::OnClickedButEject)
END_MESSAGE_MAP()


// CSdCardPrepDlg message handlers

BOOL CSdCardPrepDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    mButCancel.EnableWindow(TRUE);
    mButOK.EnableWindow(FALSE);
    mButEject.EnableWindow(FALSE);

    if (theApp.mpEFIPath != NULL)
        mEditPath.SetWindowTextW(theApp.mpEFIPath);

    mDiskSel.ResetContent();
    int ix = mDiskSel.InsertString(0,sgNoDriveText);
    mDiskSel.SetItemDataPtr(ix,NULL);
    mDiskSel.SetCurSel(ix);

    mButCancel.SetFocus();

    mpCallback = new VdsCallback(this);
    if (!mpCallback)
    {
        MessageBox(TEXT("Could not create callback interface for Virtual Disk Service."),TEXT("System Error"),MB_OK | MB_ICONERROR);
        OnCancel();
        return FALSE;
    }

    EnterCriticalSection(&mSec);
    if (FAILED(theApp.mpServ->Advise(mpCallback,&mpCallback->mCookie)))
    {
        mpCallback->Release();
        mpCallback = NULL;
    }
    else
        Locked_Refresh();
    LeaveCriticalSection(&mSec);

    if (!mpCallback)
    {
        MessageBox(TEXT("Could not register callback interface with Virtual Disk Service."),TEXT("System Error"),MB_OK | MB_ICONERROR);
        OnCancel();
        return FALSE;
    }

    SetTimer(1,1500,NULL);

	return FALSE; 
}

void CSdCardPrepDlg::EndModalLoop(int nResult)
{
    CDialog::EndModalLoop(nResult);
    KillTimer(1);
    EnterCriticalSection(&mSec);
    if (mpCallback)
    {
        theApp.mpServ->Unadvise(mpCallback->mCookie);
        mpCallback->Release();
        mpCallback = NULL;
    }
    while (mpDisks)
    {
        DiskEntry *pEnt = mpDisks;
        mpDisks = mpDisks->mpNext;
        delete pEnt;
    }
    LeaveCriticalSection(&mSec);
}

void CSdCardPrepDlg::OnRefresh(void)
{
    EnterCriticalSection(&mSec);
    Locked_Refresh();
    LeaveCriticalSection(&mSec);
}

void CSdCardPrepDlg::OnTimer(UINT_PTR nIDEvent)
{
    if ((mDoRefresh) && (!mLockoutRefresh))
        OnRefresh();
}

LRESULT CSdCardPrepDlg::OnAdviseNotify(WPARAM wParam, LPARAM lParam)
{
    EnterCriticalSection(&mNotifySec);
    while (mpNotify)
    {
        NotifyEntry *pNot = mpNotify;
        mpNotify = mpNotify->mpNext;
        LONG ix;
        for(ix=0;ix<pNot->mNum;ix++)
        {
            if (pNot->mEntries[ix].objectType==VDS_NTT_DISK)
            {
                mDoRefresh = true;
                break;
            }
        }
        LocalFree((HLOCAL)pNot);
    }
    LeaveCriticalSection(&mNotifySec);
    return 0;
}

void CSdCardPrepDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSdCardPrepDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSdCardPrepDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static DiskEntry * sMakeNewDisk(IVdsDisk *pDisk, VDS_DISK_PROP *pDiskProp)
{
    IVdsAdvancedDisk *pAdvDisk = NULL;
    HRESULT hr = pDisk->QueryInterface(IID_IVdsAdvancedDisk,(void **)&pAdvDisk);
    if (FAILED(hr))
        return NULL;    /* could not get advanced disk interface - disk is not usable */

    bool bIsRemovable = (pDiskProp->dwMediaType==RemovableMedia);
    
    LONG partType = 0;
    LONG lNumProps;
    VDS_PARTITION_PROP *pProps;
    hr = pAdvDisk->QueryPartitions(&pProps,&lNumProps);
    if (!FAILED(hr))
    {
        LONG ix;
        ULONGLONG bigSize = 0;
        for(ix=0;ix<lNumProps;ix++)
        {
            if (pProps[ix].ullSize>bigSize)
            {
                partType = pProps[ix].Mbr.partitionType;
                bigSize = pProps[ix].ullSize;
            }
        }
        CoTaskMemFree(pProps);
    }

    DiskEntry *pRet = new DiskEntry;
    pRet->mDiskId = pDiskProp->id;
    pRet->mIsRemovable = bIsRemovable;
    ULONG megs = (ULONG)(pDiskProp->ullSize/(1024*1024));  /* liars */
    if (megs<1000)
    {
        _snwprintf_s(pRet->mFriendlyString,MAX_PATH-1,MAX_PATH-1,
                        TEXT("%dMB - %s%s\r\n"),
                        megs,
                        bIsRemovable?TEXT("Removable "):TEXT(""),
                        pDiskProp->pwszFriendlyName);
    }
    else
    {
        _snwprintf_s(pRet->mFriendlyString,MAX_PATH-1,MAX_PATH-1,
                        TEXT("%d.%dGB - %s%s\r\n"),
                        megs/1000,megs%1000,
                        bIsRemovable?TEXT("Removable "):TEXT(""),
                        pDiskProp->pwszFriendlyName);
    }
    pRet->mPartType = partType;
    pRet->mBytes = pDiskProp->ullSize;
    pRet->mpNext = NULL;

    pAdvDisk->Release();
    return pRet;
}

DiskEntry * CSdCardPrepDlg::Locked_BuildDiskList(void)
{
    DiskEntry *pList = NULL;

    IEnumVdsObject *pEnumPacks = NULL;
    HRESULT hr = theApp.mpBasic->QueryPacks(&pEnumPacks);
    if (!FAILED(hr))
    {
        pEnumPacks->Reset();
        do {
            ULONG fetched = 0;
            IUnknown *pUnk = NULL;
            hr = pEnumPacks->Next(1,&pUnk,&fetched);
            if ((FAILED(hr)) || (!fetched))
                break;
            IVdsPack *pPack = NULL;
            hr = pUnk->QueryInterface(IID_IVdsPack,(void **)&pPack);
            pUnk->Release();
            if (!FAILED(hr))
            {
                /* build disks in this pack */
                VDS_PACK_PROP packProp;
                hr = pPack->GetProperties(&packProp);
                if (!FAILED(hr))
                {
                    if (packProp.status==VDS_PS_ONLINE)
                    {
                        IEnumVdsObject *pEnumDisks = NULL;
                        hr = pPack->QueryDisks(&pEnumDisks);
                        if (!FAILED(hr))
                        {
                            pEnumDisks->Reset();
                            do {
                                ULONG fetched = 0;
                                IUnknown *pUnk = NULL;
                                hr = pEnumDisks->Next(1,&pUnk,&fetched);
                                if ((FAILED(hr)) || (!fetched))
                                    break;
                                IVdsDisk *pDisk = NULL;
                                hr = pUnk->QueryInterface(IID_IVdsDisk, (void **)&pDisk);
                                pUnk->Release();
                                if (!FAILED(hr))
                                {
                                    VDS_DISK_PROP diskProp;
                                    hr = pDisk->GetProperties(&diskProp);
                                    if (!FAILED(hr))
                                    {
                                        if ((diskProp.dwMediaType == RemovableMedia) && 
                                            (diskProp.status == VDS_DS_ONLINE) &&
                                            ((diskProp.ulFlags & 0xAFC0) == 0))
                                        {
                                            /* create disk here */
                                            DiskEntry *pNewDisk = sMakeNewDisk(pDisk, &diskProp);
                                            if (pNewDisk)
                                            {
                                                pNewDisk->mpNext = pList;
                                                pList = pNewDisk;
                                            }
                                        }
                                    }
                                    pDisk->Release();
                                }
                            } while (true);
                            pEnumDisks->Release();
                        }
                    }
                }
                pPack->Release();
            }
        } while (true);
        pEnumPacks->Release();
    }

    return pList;
}

void CSdCardPrepDlg::Locked_Refresh(void)
{
    mDoRefresh = false;

    /* save the current selection */
    DiskEntry *pOldDisk = (DiskEntry *)mDiskSel.GetItemDataPtr(mDiskSel.GetCurSel());

    int newSel = -1;
    int i = 0;

    mDiskSel.SetRedraw(FALSE);
    mDiskSel.ResetContent();

    DiskEntry *pNewList = Locked_BuildDiskList();
    DiskEntry *pWork = pNewList;
    while (pWork)
    {
        int ix = mDiskSel.InsertString(i,pWork->mFriendlyString);
        mDiskSel.SetItemDataPtr(ix,pWork);
        i++;
        /* is this the same one that was selected before? */
        if (pOldDisk)
        {
            if (!memcmp(&pOldDisk->mDiskId,&pWork->mDiskId,sizeof(pOldDisk->mDiskId)))
                newSel = ix;
        }
        pWork = pWork->mpNext;
    }

    pOldDisk = mpDisks;
    mpDisks = pNewList;
    while (pOldDisk)
    {
        pWork = pOldDisk;
        pOldDisk = pOldDisk->mpNext;
        delete pWork;
    }

    if (!pNewList)
    {
        /* nothing to select */
        int ix = mDiskSel.InsertString(0,sgNoDriveText);
        mDiskSel.SetItemDataPtr(ix,NULL);
        mDiskSel.SetCurSel(0);
        Locked_SelectNothing();
    }
    else
    {
        if (newSel<0)
            newSel = 0;
        mDiskSel.SetCurSel(newSel);
        Locked_SelectSomething();
    }

    mDiskSel.SetRedraw(TRUE);
    mDiskSel.InvalidateRect(NULL);
}

void CSdCardPrepDlg::Locked_SelectNothing(void)
{
    mDiskSel.EnableWindow(FALSE);
    mButOK.EnableWindow(FALSE);
    mButEject.EnableWindow(FALSE);
}

void CSdCardPrepDlg::Locked_SelectSomething(void)
{
    mDiskSel.EnableWindow(TRUE);
    mButOK.EnableWindow(TRUE);
    mButEject.EnableWindow(TRUE);
}

void CSdCardPrepDlg::OnBnClickedOk()
{
    /* stop live updates */
    KillTimer(1);
    if (mpCallback)
        theApp.mpServ->Unadvise(mpCallback->mCookie);
    bool bDoOk = false;
    do {
        /* validate parameters */
        theApp.mpDisk = (DiskEntry *)mDiskSel.GetItemDataPtr(mDiskSel.GetCurSel());
        if (!theApp.mpDisk)
        {
            MessageBox(TEXT("Disk was not found.  Try again."),TEXT("Disk Error"),MB_OK | MB_ICONERROR);
            break;
        }

        if (theApp.mpDisk->mBytes<(33*1024*1024))
        {
            MessageBox(TEXT("The disk is not big enough."), TEXT("Disk Too Small"), MB_OK | MB_ICONERROR);
            break;
        }
        
        if (theApp.mpDisk->mBytes>=(32ll*1024*1024*1024))
        {
            MessageBox(TEXT("The disk is too big."), TEXT("Disk Too Big"), MB_OK | MB_ICONERROR);
            break;
        }

        TCHAR *pPath;
        int len = mEditPath.GetWindowTextLength();
        if (len == 0)
        {
            MessageBox(TEXT("No EFI image specified"), TEXT("Need EFI path"), MB_OK | MB_ICONERROR);
            break;
        }

        pPath = new TCHAR[len + 4];
        if (pPath == NULL)
        {
            MessageBox(TEXT("Memory allocation error"), TEXT("Memory failure"), MB_OK | MB_ICONERROR);
            break;
        }

        do
        {
            mEditPath.GetWindowText(pPath, len+1);

            theApp.mhEFIFile = CreateFile(pPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (theApp.mhEFIFile == INVALID_HANDLE_VALUE)
            {
                MessageBox(TEXT("Could not open EFI file"), TEXT("Check EFI file"), MB_OK | MB_ICONERROR);
                break;
            }

            do
            {
                LARGE_INTEGER liFileSize;
                if (!GetFileSizeEx(theApp.mhEFIFile, &liFileSize))
                {
                    MessageBox(TEXT("Could not get size of EFI file"), TEXT("Check EFI file"), MB_OK | MB_ICONERROR);
                    break;
                }

                if ((liFileSize.HighPart != 0) ||
                    (liFileSize.LowPart != EFI_FILE_SIZE))
                {
                    MessageBox(TEXT("EFI file is not the right size"), TEXT("Check EFI file"), MB_OK | MB_ICONERROR);
                    break;
                }

                // read into align-able buffer
                theApp.mpEFIAlloc = new UINT8[DISK_HEADER_SIZE + DISK_SECTOR_BYTES];
                if (theApp.mpEFIAlloc == NULL)
                {
                    MessageBox(TEXT("Memory allocation failed"), TEXT("Memory error"), MB_OK | MB_ICONERROR);
                    break;
                }
                ZeroMemory(theApp.mpEFIAlloc, DISK_HEADER_SIZE + DISK_SECTOR_BYTES);

                do
                {
                    // align data location in buffer
                    theApp.mpEFIData = (UINT8 *)(((((UINT_PTR)theApp.mpEFIAlloc) + (DISK_SECTOR_BYTES-1)) / DISK_SECTOR_BYTES) * DISK_SECTOR_BYTES);

                    DWORD red = 0;
                    if (!ReadFile(theApp.mhEFIFile, theApp.mpEFIData, EFI_FILE_SIZE, &red, NULL))
                    {
                        MessageBox(TEXT("File read failure"), TEXT("Check EFI File"), MB_OK | MB_ICONERROR);
                        break;
                    }

                    if (theApp.mpEFIPath != NULL)
                        delete[] theApp.mpEFIPath;
                    theApp.mpEFIPath = pPath;
                    pPath = NULL;
                    bDoOk = true;

                    // we are going to try to use this path
                    // so try to save it for next time
                    HKEY hKey;
                    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\SdCardPrep"),
                        NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL))
                    {
                        RegSetValueEx(hKey, 
                            TEXT("Path"), 0, REG_SZ, 
                            (BYTE const *)theApp.mpEFIPath, 
                            (((DWORD)_tcslen(theApp.mpEFIPath)) + 1) * sizeof(TCHAR));
                        RegCloseKey(hKey);
                    }


                } while (false);

                if (!bDoOk)
                {
                    delete[] theApp.mpEFIAlloc;
                    theApp.mpEFIAlloc = NULL;
                    theApp.mpEFIData = NULL;
                }

            } while (false);

            if (!bDoOk)
            {
                CloseHandle(theApp.mhEFIFile);
                theApp.mhEFIFile = INVALID_HANDLE_VALUE;
            }

        } while (false);

        if (pPath != NULL)
            delete[] pPath;

    } while (false);

    if (bDoOk)
    {
        /* good to go - we're all set */
        theApp.mDiskPartOk = false;
        theApp.mpVdsDisk = NULL;
        theApp.mpVdsPack = NULL;
        ZeroMemory(&theApp.mDiskProp,sizeof(theApp.mDiskProp));
        ZeroMemory(&theApp.mNewVolumeId,sizeof(theApp.mNewVolumeId));
        theApp.mForceClean = (mCheckForceClean.GetCheck() != 0);

        CProgDlg prog;
        bDoOk = (IDOK==prog.DoModal());

        CloseHandle(theApp.mhEFIFile);
        theApp.mhEFIFile = INVALID_HANDLE_VALUE;
        delete[] theApp.mpEFIAlloc;
        theApp.mpEFIAlloc = NULL;
        theApp.mpEFIData = NULL;
    }

    if ((bDoOk) && (!theApp.mDiskPartOk))
    {
        // auto-exit if we partitioned the disk
        // otherwise if we just updated then leave it alone
        OnOK();
    }
    else
    {
        /* start up monitoring again */
        EnterCriticalSection(&mSec);
        if (mpCallback)
        {
            if (FAILED(theApp.mpServ->Advise(mpCallback,&mpCallback->mCookie)))
            {
                mpCallback->Release();
                mpCallback = NULL;
            }
        }
        Locked_Refresh();
        LeaveCriticalSection(&mSec);
        SetTimer(1,1500,NULL);
    }
}

void CSdCardPrepDlg::OnCbnSelchangeDroplistDisksel()
{
    
}

void CSdCardPrepDlg::OnBnClickedButBrowse()
{
    OPENFILENAME ofn;
    static TCHAR const *sgpFilters = TEXT("EFI Firmware (*.fd)\0*.fd\0All Files (*.*)\0*.*\0\0");
    TCHAR initDir[MAX_PATH];
    TCHAR buffer[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hInstance = theApp.m_hInstance;
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = sgpFilters;
    ofn.lpstrFile = buffer;
    buffer[0] = 0;
    ofn.nMaxFile = MAX_PATH - 1;

    TCHAR *pPath = NULL;
    int len = mEditPath.GetWindowTextLength();
    if (len > 0)
    {
        pPath = new TCHAR[len + 4];
        if (pPath != NULL)
        {
            TCHAR *pFilePart;
            mEditPath.GetWindowText(pPath, len+1);
            if (0 != GetFullPathName(pPath, MAX_PATH - 1, initDir, &pFilePart))
            {
                _tcscpy_s(buffer, MAX_PATH - 1, initDir);
                *pFilePart = 0;
                ofn.lpstrInitialDir = initDir;
            }
            delete[] pPath;
        }
    }

    ofn.lpstrTitle = TEXT("Select EFI Firmware file:");

    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileName(&ofn))
        return;

    len = (int)_tcslen(ofn.lpstrFile);
    pPath = new TCHAR[len + 4];
    if (pPath != NULL)
    {
        _tcscpy_s(pPath, len + 4, ofn.lpstrFile);
        if (theApp.mpEFIPath != NULL)
            delete[] theApp.mpEFIPath;
        theApp.mpEFIPath = pPath;
        mEditPath.SetWindowTextW(theApp.mpEFIPath);
    }
}

void CSdCardPrepDlg::OnClickedButEject()
{
    IVdsDisk *      pDisk;
    IVdsPack *      pPack;
    IVdsRemovable * pRem;

    theApp.mpDisk = (DiskEntry *)mDiskSel.GetItemDataPtr(mDiskSel.GetCurSel());
    if (!theApp.mpDisk)
    {
        MessageBox(TEXT("Disk was not found.  Try again."),TEXT("Disk Error"),MB_OK | MB_ICONERROR);
        return;
    }

    if (!theApp.FindDisk(theApp.mpDisk, &pDisk, &pPack))
    {
        MessageBox(TEXT("Disk was not found.  Try again."),TEXT("Disk Error"),MB_OK | MB_ICONERROR);
        return;
    }

    pRem = NULL;
    HRESULT hr = pDisk->QueryInterface(&pRem);
    if (!FAILED(hr))
    {
        hr = pRem->Eject();
        pRem->Release();
        if (FAILED(hr))
        {
            MessageBox(TEXT("Eject Failed"), TEXT("Disk did not eject"), MB_OK | MB_ICONERROR);
        }
        else
        {
            PlaySound(TEXT("ding.wav"), NULL, SND_FILENAME | SND_ASYNC);
            OnRefresh();
        }
    }
    else
    {
        MessageBox(TEXT("Cannot Eject"), TEXT("Disk is not ejectable"), MB_OK | MB_ICONERROR);
    }

    pDisk->Release();
    pPack->Release();
}
