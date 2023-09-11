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

// SdCardPrep.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "SdCardPrep.h"
#include "SdCardPrepDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSdCardPrepApp

BEGIN_MESSAGE_MAP(CSdCardPrepApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CSdCardPrepApp construction

CSdCardPrepApp::CSdCardPrepApp()
{
    mpServ = NULL;
    mpBasic = NULL;

    mpDisk = NULL;
    mpEFIPath = NULL;
    mhEFIFile = INVALID_HANDLE_VALUE;
    mpEFIAlloc = NULL;
    mpEFIData = NULL;

    mDiskPartOk = false;
    mForceClean = false;
    mpVdsDisk = NULL;
    mpVdsPack = NULL;
    ZeroMemory(&mDiskProp,sizeof(mDiskProp));
    ZeroMemory(&mNewVolumeId,sizeof(mNewVolumeId));
}


// The one and only CSdCardPrepApp object

CSdCardPrepApp theApp;

static GUID sBasicProvierGUID = 
{ 0xCA7DE14F,
  0x5BC8,
  0x48FD,
  0x93,0xDE,0xA1,0x95,0x27,0xB0,0x45,0x9E
};

bool
CSdCardPrepApp::FindProviders(void)
{
    if (!mpServ)
        return false;
    HRESULT hr = mpServ->WaitForServiceReady();
    if (FAILED(hr))
        return NULL;
    IEnumVdsObject *pEnum = NULL;
    DWORD masks = VDS_QUERY_SOFTWARE_PROVIDERS;
    hr = mpServ->QueryProviders(masks, &pEnum);
    if (FAILED(hr))
        return NULL;
    bool bRet = false;
    pEnum->Reset();
    do {
        ULONG fetched = 0;
        IUnknown *pUnk = NULL;
        hr = pEnum->Next(1,&pUnk,&fetched);
        if ((FAILED(hr)) || (!fetched))
            break;
        IVdsProvider *pRawProv = NULL;
        hr = pUnk->QueryInterface(IID_IVdsProvider,(void **)&pRawProv);
        pUnk->Release();
        if (!FAILED(hr))
        {
            VDS_PROVIDER_PROP provProp;
            hr = pRawProv->GetProperties(&provProp);
            if (!FAILED(hr))
            {
                if (!memcmp(&provProp.id,&sBasicProvierGUID,sizeof(sBasicProvierGUID)))
                {
                    hr = pRawProv->QueryInterface(IID_IVdsSwProvider,(void **)&mpBasic);
                    if (!FAILED(hr))
                        bRet = true;
                    else
                        mpBasic = NULL;
                }
            }
            pRawProv->Release();
        }
    } while (!mpBasic);
    pEnum->Release();
    return bRet;
}

bool CSdCardPrepApp::FindDisk(DiskEntry *apEntry, IVdsDisk **appRetDisk, IVdsPack **appRetPack)
{
    bool ret = false;
    HRESULT hr = E_FAIL;

    IEnumVdsObject *pEnumPacks = NULL;

    hr = theApp.mpBasic->QueryPacks(&pEnumPacks);
    if (FAILED(hr))
        return false;

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
        pUnk = NULL;
        if (!FAILED(hr))
        {
            do {
                VDS_PACK_PROP packProp;
                hr = pPack->GetProperties(&packProp);
                if (FAILED(hr))
                    break;
                if (packProp.status!=VDS_PS_ONLINE)
                    break;

                IEnumVdsObject *pEnumDisks = NULL;
                hr = pPack->QueryDisks(&pEnumDisks);
                if (FAILED(hr))
                    break;
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
                    pUnk = NULL;
                    if (!FAILED(hr))
                    {
                        do {
                            hr = pDisk->GetProperties(&theApp.mDiskProp);
                            if (FAILED(hr))
                                break;
                            if ((theApp.mDiskProp.status==VDS_DS_ONLINE) &&
                                ((theApp.mDiskProp.ulFlags & 0xAFC0)==0))
                            {
                                /* is this our disk? */
                                if (!memcmp(&theApp.mDiskProp.id,&apEntry->mDiskId,sizeof(theApp.mDiskProp.id)))
                                {
                                    *appRetDisk = pDisk;
                                    pDisk->AddRef();
                                    *appRetPack = pPack;
                                    pPack->AddRef();
                                    ret = true;
                                }
                            }

                        } while (false);

                        pDisk->Release();
                        pDisk = NULL;
                    }

                } while (!ret);

                pEnumDisks->Release();
                pEnumDisks = NULL;

            } while (false);

            pPack->Release();
            pPack = NULL;
        }

    } while (!ret);

    pEnumPacks->Release();
    pEnumPacks = NULL;

    return ret;
}

// CSdCardPrepApp initialization

BOOL CSdCardPrepApp::InitInstance()
{
    CString defaultPath;

    if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        MessageBox(NULL,TEXT("Could not initialize COM."),TEXT("System Error"),MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (FAILED(CoInitializeSecurity(
            NULL, 
            -1, 
            NULL, 
            NULL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_IMP_LEVEL_IMPERSONATE, 
            NULL, 
            0,
            NULL)))
    {
        MessageBox(NULL,TEXT("Could not initialize COM security."),TEXT("System Error"),MB_OK | MB_ICONERROR);
        return FALSE;
    }

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
    HKEY hKey;
    DWORD disp;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\SdCardPrep"),
        NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disp))
    {
        if (disp == REG_OPENED_EXISTING_KEY)
        {
            DWORD sizeData;
            if (ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("Path"), NULL, NULL, NULL, &sizeData))
            {
                theApp.mpEFIPath = new TCHAR[(sizeData / sizeof(TCHAR)) + 4];
                if (theApp.mpEFIPath != NULL)
                {
                    if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("Path"), NULL, NULL, (LPBYTE)theApp.mpEFIPath, &sizeData))
                    {
                        delete[] theApp.mpEFIPath;
                        theApp.mpEFIPath = NULL;
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }

    do {
        IUnknown *pUnk = NULL;
        IVdsServiceLoader *pLoader = NULL;
        HRESULT hr = CoCreateInstance(CLSID_VdsLoader, NULL, CLSCTX_SERVER, IID_IUnknown, (void **)&pUnk);
        if (FAILED(hr))
        {
            MessageBox(NULL,TEXT("Could not instantiate Virtual Disk Service Loader.\r\n"),TEXT("System Error"),MB_OK | MB_ICONERROR);
            break;
        }
        hr = pUnk->QueryInterface(IID_IVdsServiceLoader, (void **)&pLoader);
        pUnk->Release();
        if (FAILED(hr))
        {
            MessageBox(NULL,TEXT("Could not acquire interface of Virtual Disk Service Loader.\r\n"),TEXT("System Error"),MB_OK | MB_ICONERROR);
            break;
        }
        hr = pLoader->LoadService(NULL,&mpServ);
        if (FAILED(hr))
            mpServ = NULL;
        pLoader->Release();
        if (FAILED(hr))
        {
            MessageBox(NULL,TEXT("Could not load Virtual Disk Service.\r\n"),TEXT("System Error"),MB_OK | MB_ICONERROR);
            break;
        }
    } while (false);

    if (mpServ)
    {
        do {
            HRESULT hr = mpServ->WaitForServiceReady();
            if (FAILED(hr))
            {
                MessageBox(NULL,TEXT("Failed waiting for Virtual Disk Service to be ready.\r\n"),TEXT("System Error"),MB_OK | MB_ICONERROR);
                break;
            }
            if (!FindProviders())
            {
                MessageBox(NULL,TEXT("Could not find at least one required disk provider.\r\n"),TEXT("System Error"),MB_OK | MB_ICONERROR);
                break;
            }

	        CSdCardPrepDlg dlg;
	        m_pMainWnd = &dlg;
	        dlg.DoModal();

            mpBasic->Release();
            mpBasic = NULL;

        } while (false);

        mpServ->Release();
        mpServ = NULL;
    }

    if (theApp.mpEFIPath != NULL)
    {
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\SdCardPrep"),
            NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disp))
        {
            RegSetValueEx(hKey, 
                TEXT("Path"), 0, REG_SZ, 
                (BYTE const *)theApp.mpEFIPath, 
                (((DWORD)_tcslen(theApp.mpEFIPath)) + 1) * sizeof(TCHAR));
            delete[] theApp.mpEFIPath;
            RegCloseKey(hKey);
        }
    }

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
    if (pShellManager != NULL)
        delete pShellManager;

    CoUninitialize();

    return FALSE;
}

