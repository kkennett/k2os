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

#if 0
SD card LBA map - 512 byte sectors
-------------------------------------------
0x000000 0      Protective MBR --------------------------BEGINNING OF FD FILE
0x000200 1      GPT Header
0x000400 2      IVT offset for iMX6 
0x000600 3      spare sector
0x000800 4      GPT Partition Table Entries 0, 1, 2, 3
0x000A00 5      GPT Partition Table Entries 4, 5, 6, 7
0x000C00 6      GPT Partition Table Entries 8, 9, 10, 11
0x000E00 7      GPT Partition Table Entries 12, 13, 14, 15
0x001000 8      Start of hole (Temp ram area page 1 (starting at 0))
0x001200 9      Hole..
..
0x008E00 71     Hole 
0x009000 72     SEC FV start sector
0x009200 73     SEC FV..
...
0x030E00 391    SEC FV
0x031000 392    VarStore first sector 
0x031200 393    VarStore...
...
0x07FE00 1023   Varstore 
0x080000 1024   GPT "First Usable LBA" (512KB)
0x080200 1025   Second usable LBA
...
0x?      n-6    GPT "Last Usable LBA"
0x?      n-5    GPT Partition Table Entries 0,1,2,3
0x?      n-4    GPT Partition Table Entries 4,5,6,7
0x?      n-3    GPT Partition Table Entries 8,9,10,11
0x?      n-2    GPT Partition Table Entries 12,13,14,15
0x?      n-1    GPT Header
----     n      Size of SD Card
#endif


#include "stdafx.h"
#include "SdCardPrep.h"
#include "ProgDlg.h"

#define STATE_TURN_OFF_AUTO_MOUNT       0

// 2 -10
#define STATE_FIND_DISK                 (STATE_TURN_OFF_AUTO_MOUNT + 1)

// 10-20
#define STATE_START_PURGE_VOLUMES       (STATE_FIND_DISK + 1)
#define STATE_CONTINUE_PURGE_VOLUMES    (STATE_START_PURGE_VOLUMES + 1)

// 20-30
#define STATE_START_DESTROY_DISK        (STATE_CONTINUE_PURGE_VOLUMES + 1)
#define STATE_CONTINUE_DESTROY_DISK     (STATE_START_DESTROY_DISK + 1)

// 30-35
#define STATE_CREATE_DISK               (STATE_CONTINUE_DESTROY_DISK + 1)

// 35-40
#define STATE_CREATE_VOLUME             (STATE_CREATE_DISK + 1)

// 40-95
#define STATE_START_CREATE_FILESYS      (STATE_CREATE_VOLUME + 1)
#define STATE_CONTINUE_CREATE_FILESYS   (STATE_START_CREATE_FILESYS + 1)

// 90-95
#define STATE_MAKE_ACCESS_PATH          (STATE_CONTINUE_CREATE_FILESYS + 1)

// 10-90
#define STATE_UPDATE_EFI                (STATE_MAKE_ACCESS_PATH + 1)

// 100
#define STATE_COMPLETED_OK              (STATE_UPDATE_EFI + 1)


static const GUID EfiSystemPartitionGuid = 
{ 0xC12A7328, 0xF81F, 0x11D2, { 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B } };

static const GUID BasicDataPartitionGuid =
{ 0xebd0a0a2, 0xb9e5, 0x4433, { 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 } };

static const GUID MsReservedPartitionGuid =
{ 0xE3C9E316L, 0x0B5C, 0x4DB8, { 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE } };

static const GUID SdCardPartitionGuid = 
{ 0xecb8b3a1, 0x1b33, 0x4691, { 0xa4, 0xd9, 0xdb, 0xb5, 0x0f, 0x50, 0x3e, 0xa7 } };

static const char *sgpSig = "EFI PART";
static const UINT32 sgRev = 0x00010000;

static const UINT32 sgCRC32_tab[] = 
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419,
    0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4,
    0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07,
    0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856,
    0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4,
    0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3,
    0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A,
    0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599,
    0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190,
    0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
    0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E,
    0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED,
    0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3,
    0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A,
    0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5,
    0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010,
    0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17,
    0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6,
    0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615,
    0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344,
    0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A,
    0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1,
    0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C,
    0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF,
    0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE,
    0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31,
    0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C,
    0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B,
    0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1,
    0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278,
    0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7,
    0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66,
    0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605,
    0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8,
    0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B,
    0x2D02EF8D
};                                                                       

UINT32 
CRC_Calc32(
    UINT32          aInitialValue
,   void const *    apData
,   UINT32          aDataBytes
)
{
    if (!aDataBytes)
        return aInitialValue;
    aInitialValue ^= 0xFFFFFFFF;
    do {
        aInitialValue = sgCRC32_tab[(aInitialValue ^ (*((UINT8 const *)apData))) & 0xFF] ^ (aInitialValue >> 8);
        apData = ((UINT8 const *)apData)+1;
    } while (--aDataBytes);
    return aInitialValue ^ 0xFFFFFFFF;
}


// ProgDlg dialog

IMPLEMENT_DYNAMIC(CProgDlg, CDialogEx)

CProgDlg::CProgDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_WORKER, pParent)
{
    mStartDelay = 0;
    mState = 0;
    mAutoMountWasOff = false;

    /* working variables for state machine */
    mpWorkEnumOuter = NULL;
    mpWorkEnumInner = NULL;
    mpWorkAsync = NULL;
    mpWorkVol = NULL;

    mhWorkFile = INVALID_HANDLE_VALUE;
    mpWorkBuf = NULL;
    mWorkBytes = 0;
    mWorkLeft = 0;

    mDoneOuter = false;
    mDoneInner = false;
}

CProgDlg::~CProgDlg()
{
}

void CProgDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROG_TEXT, mProgText);
    DDX_Control(pDX, IDC_PROG_BAR, mProgBar);
}


BEGIN_MESSAGE_MAP(CProgDlg, CDialogEx)
    ON_COMMAND(IDOK, DoNothing)
    ON_WM_TIMER()
    ON_COMMAND(IDCANCEL, DoNothing)
END_MESSAGE_MAP()

BOOL CProgDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    mProgBar.SetRange(0,100);
    mProgBar.SetPos(2);

    mStartDelay = 0;
    SetTimer(100,30,NULL);

    return TRUE; // return TRUE  unless you set the focus to a control
}

void CProgDlg::DoNothing(void) {}

bool CProgDlg::AbortMessage(TCHAR *pMsg, ...)
{
    va_list argList;
    TCHAR outs[256];
    va_start(argList, pMsg);
    _vsnwprintf_s(outs,255,255,pMsg,argList);
    outs[255] = 0;
    UINT flags = MB_ICONERROR;
    if (IsDebuggerPresent())
        flags |= MB_OKCANCEL;
    else
        flags |= MB_OK;
    if (IDCANCEL==MessageBox(outs,TEXT("Disk Preparation Error"), flags))
        DebugBreak();
    OnCancel();
    return false;
}

void CProgDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (++mStartDelay>4)
    {
        KillTimer(100);
        if (Iterate())
        {
            /* keep going - set for next timer event */
            SetTimer(100,30,NULL);
        }
    }
}

bool CProgDlg::Iterate(void)
{
    bool ret = true; /* keep going */

    switch (mState)
    {
    case STATE_TURN_OFF_AUTO_MOUNT:
        {
            mProgText.SetWindowTextW(TEXT("Turning off Service Auto-mount..."));
            VDS_SERVICE_PROP servProp;
            HRESULT hr = theApp.mpServ->GetProperties(&servProp);
            if (FAILED(hr))
            {
                ret = AbortMessage(TEXT("Could not get Virtual Disk Service properties.\r\n"));
                break;
            }
            if (!(servProp.ulFlags & VDS_SVF_AUTO_MOUNT_OFF))
            {
                mAutoMountWasOff = false;
                hr = theApp.mpServ->SetFlags(VDS_SVF_AUTO_MOUNT_OFF);
                if (FAILED(hr))
                {
                    ret = AbortMessage(TEXT("Could not turn off Virtual Disk Service auto-mount.\r\n"));
                    break;
                }
            }
            else
                mAutoMountWasOff = true;
        }
        if (ret)
            mState = STATE_FIND_DISK;
        break;

    case STATE_FIND_DISK:
        mProgBar.SetPos(5);
        mProgText.SetWindowTextW(TEXT("Acquiring Target Disk..."));
        ret = State_FindDisk();
        if (ret)
        {
            if (!theApp.mDiskPartOk)
            {
                if (theApp.mpVdsPack)
                    mState = STATE_START_PURGE_VOLUMES;
                else
                    mState = STATE_CREATE_DISK;
            }
            else
                mState = STATE_UPDATE_EFI;
        }
        break;

    case STATE_START_PURGE_VOLUMES:
        mProgBar.SetPos(10);
        mProgText.SetWindowTextW(TEXT("Purging any existing volumes..."));
        mDoneOuter = false;
        ret = State_StartPurgeVolumes();
        if (ret)
        {
            if (mDoneOuter)
                mState = STATE_START_DESTROY_DISK;
            else
                mState = STATE_CONTINUE_PURGE_VOLUMES;
        }
        break;

    case STATE_CONTINUE_PURGE_VOLUMES:
        mProgBar.SetPos(15);
        ret = State_ContinuePurgeVolumes();
        if (ret)
        {
            if (mDoneOuter)
                mState = STATE_START_DESTROY_DISK;
        }
        break;

    case STATE_START_DESTROY_DISK:
        mProgBar.SetPos(20);
        mProgText.SetWindowTextW(TEXT("Destroying Target Disk..."));
        mDoneOuter = false;
        ret = State_StartDestroyDisk();
        if (ret)
        {
            if (mDoneOuter)
                mState = STATE_CREATE_DISK;
            else
                mState = STATE_CONTINUE_DESTROY_DISK;
        }
        break;

    case STATE_CONTINUE_DESTROY_DISK:
        mProgBar.SetPos(25);
        ret = State_ContinueDestroyDisk();
        if (ret)
        {
            if (mDoneOuter)
                mState = STATE_CREATE_DISK;
        }
        break;

    case STATE_CREATE_DISK:
        mProgBar.SetPos(30);
        mProgText.SetWindowTextW(TEXT("Creating Target Disk..."));
        mDoneOuter = false;
        ret = State_CreateDisk();
        if (ret)
            mState = STATE_CREATE_VOLUME;
        break;

    case STATE_CREATE_VOLUME:
        mProgBar.SetPos(30);
        mProgText.SetWindowTextW(TEXT("Creating Target Volume..."));
        mDoneOuter = false;
        ret = State_CreateVolume();
        if (ret)
            mState = STATE_START_CREATE_FILESYS;
        break;


    case STATE_START_CREATE_FILESYS:
        mProgBar.SetPos(40);
        mProgText.SetWindowTextW(TEXT("Creating Target File System..."));
        mDoneOuter = false;
        ret = State_StartCreateFileSys();
        if (ret)
        {
            if (mDoneOuter)
                mState = STATE_MAKE_ACCESS_PATH;
            else
                mState = STATE_CONTINUE_CREATE_FILESYS;
        }
        break;

    case STATE_CONTINUE_CREATE_FILESYS:
        ret = State_ContinueCreateFileSys();
        if (ret)
        {
            if (mDoneOuter)
                mState = STATE_MAKE_ACCESS_PATH;
        }
        break;

    case STATE_UPDATE_EFI:
        ret = State_UpdateEFI();
        if (ret)
            mState = STATE_COMPLETED_OK;
        break;

    case STATE_MAKE_ACCESS_PATH:
        mProgBar.SetPos(90);
        mProgText.SetWindowTextW(TEXT("Trying to create disk access path..."));
        State_MakeAccessPath();
        mState = STATE_COMPLETED_OK;
        break;

    case STATE_COMPLETED_OK:
        mProgBar.SetPos(100);
        MessageBox(TEXT("Disk Preparation Completed ok."),TEXT("Success"),MB_OK | MB_ICONINFORMATION);
        OnOK();
        ret = false;
        break;

    default:
        ret = AbortMessage(TEXT("Program is in an unknown state!"));
    };

    if (!ret)
    {
        /* we are stopping */
        if (mpWorkVol)
        {
            mpWorkVol->Release();
            mpWorkVol = NULL;
        }

        if (mState>STATE_FIND_DISK)
        {
            if (theApp.mpVdsDisk)
            {
                theApp.mpVdsDisk->Release();
                theApp.mpVdsDisk = NULL;
            }
            if (theApp.mpVdsPack)
            {
                theApp.mpVdsPack->Release();
                theApp.mpVdsPack = NULL;
            }
        }
        
        /* turn auto-mount back on if it was off */
        if (mState>STATE_TURN_OFF_AUTO_MOUNT)
        {
            if (!mAutoMountWasOff)
            {
                /* turn auto-mount back on */
                theApp.mpServ->ClearFlags(VDS_SVF_AUTO_MOUNT_OFF);
            }
        }
        /* always refresh before you leave for the last time */
        theApp.mpServ->Refresh();
    }

    return ret;
}

bool CProgDlg::State_FindDisk(void)
{
    bool ret = false;
    HRESULT hr = E_FAIL;

    ret = theApp.FindDisk(theApp.mpDisk, &theApp.mpVdsDisk, &theApp.mpVdsPack);
    if (!ret)
        AbortMessage(TEXT("Unknown error finding the target disk.  Please check the dialog and then re-try."));

    // 
    // found the disk
    // now see if the partitioning is already ok
    //
    theApp.mDiskPartOk = false;
    do
    {
        if (theApp.mForceClean)
            break;

        if (theApp.mDiskProp.PartitionStyle != VDS_PST_GPT)
            break;

        HANDLE hDisk;

        hDisk = CreateFile(theApp.mDiskProp.pwszDevicePath,
            GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
        if (hDisk == INVALID_HANDLE_VALUE)
        {
            AbortMessage(TEXT("Could not open target disk for processing."));
            ret = false;
            break;
        }

        // aligned buffer for sector 1
        UINT8 DiskSector[DISK_SECTOR_BYTES *2];
        UINT8 *pBuffer;
        pBuffer = DiskSector;
        pBuffer = (UINT8 *)(((((UINT_PTR)pBuffer) + (DISK_SECTOR_BYTES-1)) / DISK_SECTOR_BYTES) * DISK_SECTOR_BYTES);

        do
        {
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hDisk, DISK_SECTOR_BYTES, NULL, FILE_BEGIN))
            {
                AbortMessage(TEXT("Could not seek to sector 1 on disk."));
                ret = false;
                break;
            }

            DWORD red = 0;
            if (!ReadFile(hDisk, pBuffer, DISK_SECTOR_BYTES, &red, NULL))
            {
                AbortMessage(TEXT("Could not read target disk."));
                ret = false;
                break;
            }

            // check required partitioning parameters
            GPT_HEADER *pHdr;
            pHdr = (GPT_HEADER *)pBuffer;

            if (pHdr->MyLBA != 1)
                break;
            if (pHdr->PartitionEntryLBA != PARTITION_TABLE_START_LBA)
                break;
            if (pHdr->NumberOfPartitionEntries != PARTITION_TABLE_ENTRY_COUNT)
                break;
            if (pHdr->FirstUsableLBA != (DISK_HEADER_SIZE / DISK_SECTOR_BYTES))
                break;

            theApp.mDiskPartOk = true;

        } while (false);

        CloseHandle(hDisk);

    } while (false);

    if (!theApp.mDiskPartOk)
    {
        if (IDCANCEL == MessageBox(TEXT("WARNING!\n\nContinuing will destroy all data in all volumes on the target disk.\n\nAre you sure you want to continue?"),
            TEXT("Confirm Dangerous Action"), MB_OKCANCEL | MB_ICONWARNING))
        {
            ret = false;
            OnCancel();
        }
    }

    return ret;
}

bool CProgDlg::StartPurgeOne(void)
{
    IVdsVolumeMF *pMF = NULL;
    HRESULT hr = mpWorkVol->QueryInterface(IID_IVdsVolumeMF,(void **)&pMF);
    if (FAILED(hr))
        return AbortMessage(TEXT("Error acquiring necessary interface from an existing volume on the disk."));

    LONG lNumPaths = 0;
    WCHAR **ppPaths = NULL;
    hr = pMF->QueryAccessPaths(&ppPaths,&lNumPaths);
    if (!FAILED(hr))
    {
        LONG i;
        for(i=0;i<lNumPaths;i++)
        {
            hr = pMF->DeleteAccessPath(ppPaths[i],TRUE);
            if (FAILED(hr))
                break;
        }
        if (ppPaths)
            CoTaskMemFree(ppPaths);
    }

    if (!FAILED(hr))
        hr = pMF->Dismount(TRUE,TRUE);

    pMF->Release();
    pMF = NULL;

    if (FAILED(hr))
        return AbortMessage(TEXT("Could not dismount an existing access path or volume on the disk."));

    /* destroy the volume's plexes and then the volume itself - only proceed if volume is offline */
    VDS_VOLUME_PROP volProp;
    hr = mpWorkVol->GetProperties(&volProp);
    if (FAILED(hr))
        return AbortMessage(TEXT("Could not access properties of an existing volume on the disk."));

    if (!(volProp.ulFlags & VDS_VF_PERMANENTLY_DISMOUNTED))
        return AbortMessage(TEXT("An existing volumet on the disk did not dismount properly."));

    /* start to destroy the plexes on the disk */
    bool ret = true;
    do {
        mpWorkEnumInner = NULL;
        hr = mpWorkVol->QueryPlexes(&mpWorkEnumInner);
        if (FAILED(hr))
        {
            mDoneInner = true;
            break;
        }
        mpWorkEnumInner->Reset();

        do {
            ULONG fetched = 0;
            IUnknown *pUnk = NULL;
            hr = mpWorkEnumInner->Next(1,&pUnk,&fetched);
            if ((FAILED(hr)) || (!fetched))
            {
                mDoneInner = true;
                break;
            }

            IVdsVolumePlex *pPlex = NULL;
            hr = pUnk->QueryInterface(IID_IVdsVolumePlex,(void **)&pPlex);
            pUnk->Release();
            pUnk = NULL;
            if (FAILED(hr))
            {
                ret = AbortMessage(TEXT("An existing volume's plex could not be accessed."));
                break;
            }

            VDS_VOLUME_PLEX_PROP plexProp;
            hr = pPlex->GetProperties(&plexProp);
            pPlex->Release();
            pPlex = NULL;
            if (FAILED(hr))
            {
                ret = AbortMessage(TEXT("The properties of an existing volume's plex could not be retreived."));
                break;
            }

            mpWorkAsync = NULL;
            hr = mpWorkVol->RemovePlex(plexProp.id,&mpWorkAsync);
            if (!FAILED(hr))
                break;
            if (hr!=VDS_E_NOT_SUPPORTED)
            {
                ret = AbortMessage(TEXT("The command to delete an existing volume's plex failed."));
                break;
            }
            /* otherwise go try to do the next one */

        } while (true);

        if ((!ret) || (mDoneInner))
        {
            mpWorkEnumInner->Release();
            mpWorkEnumInner = NULL;
        }

    } while (false);

    if ((ret) && (mDoneInner))
    {
        /* we cannot delete a volume on a removable disk.  the disk-destroy below should take care of the volume */
        if (!theApp.mpDisk->mIsRemovable)
            mpWorkVol->Delete(FALSE);   /* we don't care if this fails */
    }

    return ret;
}

bool CProgDlg::State_StartPurgeVolumes(void)
{
    mpWorkEnumOuter = NULL;
    HRESULT hr = theApp.mpVdsPack->QueryVolumes(&mpWorkEnumOuter);
    if (FAILED(hr))
        return AbortMessage(TEXT("Could not start enumeration of volumes on the disk."));
    mpWorkEnumOuter->Reset();

    bool ret = true;
    do {
        /* get the first volume to do */
        ULONG fetched = 0;
        IUnknown *pUnk = NULL;
        hr = mpWorkEnumOuter->Next(1,&pUnk,&fetched);
        if ((FAILED(hr)) || (!fetched))
        {
            mDoneOuter = true;
            break;
        }

        mpWorkVol = NULL;
        hr = pUnk->QueryInterface(IID_IVdsVolume,(void **)&mpWorkVol);
        pUnk->Release();
        pUnk = NULL;
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Error accessing an existing volume on the disk."));
            break;
        }

        mDoneInner = false;
        ret = StartPurgeOne();
        if ((!ret) || (mDoneInner))
        {
            mpWorkVol->Release();
            mpWorkVol = NULL;
        }

    } while (ret && (!mDoneOuter) && mDoneInner);
        
    if ((!ret) || (mDoneOuter))
    {
        /* stop outer enumeration */
        mpWorkEnumOuter->Release();
        mpWorkEnumOuter = NULL;
    }

    return ret;
}

bool CProgDlg::State_ContinuePurgeVolumes(void)
{
    ULONG percentComplete = 0;
    HRESULT hrAsync;
    HRESULT hr = mpWorkAsync->QueryStatus(&hrAsync,&percentComplete);

    if ((!FAILED(hr)) && (hrAsync==VDS_E_OPERATION_PENDING))
        return true;

    mpWorkAsync->Release();
    mpWorkAsync = NULL;

    bool ret = true;
    do  {
        if ((FAILED(hr)) || (FAILED(hrAsync)))
        {
            ret = AbortMessage(TEXT("Destroying the plex of an existing volume failed (hr=0x%08X)"),hrAsync);
            break;
        }

        /* all done destruction of volume plex - go get the next one to destroy */
        ULONG fetched = 0;
        IUnknown *pUnk = NULL;
        hr = mpWorkEnumInner->Next(1,&pUnk,&fetched);
        if ((FAILED(hr)) || (!fetched))
        {
            mDoneInner = true;
            break;
        }

        /* go start to nuke the next plex */

        IVdsVolumePlex *pPlex = NULL;
        hr = pUnk->QueryInterface(IID_IVdsVolumePlex,(void **)&pPlex);
        pUnk->Release();
        pUnk = NULL;
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("An existing volume's plex could not be accessed."));
            break;
        }

        VDS_VOLUME_PLEX_PROP plexProp;
        hr = pPlex->GetProperties(&plexProp);
        pPlex->Release();
        pPlex = NULL;
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("The properties of an existing volume's plex could not be retreived."));
            break;
        }

        mpWorkAsync = NULL;
        hr = mpWorkVol->RemovePlex(plexProp.id,&mpWorkAsync);
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("The command to delete an existing volume's plex failed."));
            break;
        }

    } while (false);

    if ((!ret) || (mDoneInner))
    {
        mpWorkEnumInner->Release();
        mpWorkEnumInner = NULL;

        if (ret)
        {
            /* we cannot delete a volume on a removable disk.  the disk-destroy below should take care of the volume */
            if (!theApp.mpDisk->mIsRemovable)
                mpWorkVol->Delete(FALSE);   /* we don't care if this fails */
        }

        mpWorkVol->Release();
        mpWorkVol = NULL;

        if (ret)
        {
            do {
                /* try to get the next volume */
                ULONG fetched = 0;
                IUnknown *pUnk = NULL;
                hr = mpWorkEnumOuter->Next(1,&pUnk,&fetched);
                if ((FAILED(hr)) || (!fetched))
                {
                    mDoneOuter = true;
                    break;
                }

                mpWorkVol = NULL;
                hr = pUnk->QueryInterface(IID_IVdsVolume,(void **)&mpWorkVol);
                pUnk->Release();
                pUnk = NULL;
                if (FAILED(hr))
                {
                    ret = AbortMessage(TEXT("Error accessing an existing volume on the disk."));
                    break;
                }

                mDoneInner = false;
                ret = StartPurgeOne();
                if ((!ret) || (mDoneInner))
                {
                    mpWorkVol->Release();
                    mpWorkVol = NULL;
                }
            } while (ret && (!mDoneOuter) && mDoneInner);
        }
    }
        
    if ((!ret) || (mDoneOuter))
    {
        /* stop outer enumeration */
        mpWorkEnumOuter->Release();
        mpWorkEnumOuter = NULL;
    }

    return ret;
}

bool CProgDlg::State_StartDestroyDisk(void)
{
    IVdsAdvancedDisk *pAdvDisk;
    HRESULT hr = theApp.mpVdsDisk->QueryInterface(IID_IVdsAdvancedDisk, (void **)&pAdvDisk);
    if (FAILED(hr))
        return AbortMessage(TEXT("Could not get interface used to destroy the existing disk structures (hr=0x%08X)."),hr);
    bool ret = true;
    do {
        mpWorkAsync = NULL;
        hr = pAdvDisk->Clean(TRUE,TRUE,FALSE,&mpWorkAsync);
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Could not clean existing disk."));
            break;
        }

        /* async is running - check it in continuedestroy */
 
    } while (false);
    pAdvDisk->Release();
    return ret;
}

bool CProgDlg::State_ContinueDestroyDisk(void)
{
    ULONG percentComplete = 0;
    HRESULT hrAsync;
    
    HRESULT hr = mpWorkAsync->QueryStatus(&hrAsync,&percentComplete);

    if ((!FAILED(hr)) && (hrAsync==VDS_E_OPERATION_PENDING))
        return true;

    if ((!FAILED(hr)) && (hrAsync==__HRESULT_FROM_WIN32(ERROR_NOT_READY)))
    {
        VDS_ASYNC_OUTPUT asyncOut;
        hr = mpWorkAsync->Wait(&hrAsync,&asyncOut);
        if (!FAILED(hr))
        {
            if (hrAsync==__HRESULT_FROM_WIN32(ERROR_NOT_READY))
                hrAsync = S_OK;
        }
    }

    mpWorkAsync->Release();
    mpWorkAsync = NULL;

    bool ret = true;
    do  {
        if ((FAILED(hr)) || (FAILED(hrAsync)))
        {
            ret = AbortMessage(TEXT("Destroying the structures on the existing disk failed (hr=0x%08X)"),hrAsync);
            break;
        }

        hr = theApp.mpVdsDisk->GetProperties(&theApp.mDiskProp);
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Existing disk disappeared! (hr=0x%08X)"),hr);
            break;
        }

        mDoneOuter = true;
    
    } while (false);

    return ret;
}

bool CProgDlg::State_CreateDisk(void)
{
    HANDLE  hDisk;
    bool    ret;

    /* disk destroyed - old pack should be gone */
    if (theApp.mpVdsPack)
    {
        theApp.mpVdsPack->Release();
        theApp.mpVdsPack = NULL;
    }

    HRESULT hr = theApp.mpVdsDisk->GetProperties(&theApp.mDiskProp);
    if (FAILED(hr))
        return AbortMessage(TEXT("Failed to get properties of new disk."));

    theApp.mpVdsDisk->Release();
    theApp.mpVdsDisk = NULL;

    GPT_SECTOR *pGPT = (GPT_SECTOR *)(theApp.mpEFIData + DISK_SECTOR_BYTES);
    GPT_ENTRY *pEntry = (GPT_ENTRY *)(theApp.mpEFIData + (PARTITION_TABLE_START_LBA * DISK_SECTOR_BYTES));

    hDisk = CreateFile(theApp.mDiskProp.pwszDevicePath,
        GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
    if (hDisk == INVALID_HANDLE_VALUE)
        return AbortMessage(TEXT("Could not open target disk for exclusive access"));

    ret = false;
    do
    {
        DISK_GEOMETRY_EX    geo;
        LARGE_INTEGER       diskEnd;
        DWORD               result;

        ZeroMemory(&geo, sizeof(geo));
        SetLastError(0);
        if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            NULL, 0,
            &geo, sizeof(geo), &result, NULL))
        {
            AbortMessage(TEXT("Could not get geometry of target disk"));
            break;
        }

        ZeroMemory(pGPT, sizeof(GPT_HEADER));
        CopyMemory(pGPT->Header.Signature, sgpSig, 8);
        pGPT->Header.Revision = sgRev;
        pGPT->Header.HeaderSize = sizeof(GPT_HEADER);
        pGPT->Header.MyLBA = 1;
        pGPT->Header.FirstUsableLBA = (DISK_HEADER_SIZE / DISK_SECTOR_BYTES);
        pGPT->Header.PartitionEntryLBA = PARTITION_TABLE_START_LBA;
        pGPT->Header.NumberOfPartitionEntries = PARTITION_TABLE_ENTRY_COUNT; // at 128 bytes each, this is  512-byte sectors
        pGPT->Header.SizeOfPartitionEntry = sizeof(GPT_ENTRY);

        diskEnd.QuadPart = geo.DiskSize.QuadPart / DISK_SECTOR_BYTES;
        diskEnd.QuadPart--;

        UuidCreate(&pGPT->Header.DiskGuid);

        pGPT->Header.AlternateLBA = diskEnd.QuadPart;
        pGPT->Header.LastUsableLBA = diskEnd.QuadPart - 5;

        ZeroMemory(pEntry, sizeof(GPT_ENTRY) * pGPT->Header.NumberOfPartitionEntries);

        //
        // partitions definition for the disk
        //
//        CopyMemory(&pEntry[0].PartitionTypeGuid, &BasicDataPartitionGuid, sizeof(GUID));
        CopyMemory(&pEntry[0].PartitionTypeGuid, &EfiSystemPartitionGuid, sizeof(GUID));
        CopyMemory(&pEntry[0].UniquePartitionGuid, &SdCardPartitionGuid, sizeof(GUID));
        pEntry[0].StartingLBA = pGPT->Header.FirstUsableLBA;
        pEntry[0].EndingLBA = pEntry[0].StartingLBA + ((32 * 1024 * 1024) / DISK_SECTOR_BYTES) - 1;
        CopyMemory(pEntry[0].PartitionName, L"EFI", 8);

        CopyMemory(&pEntry[1].PartitionTypeGuid, &MsReservedPartitionGuid, sizeof(GUID));
        UuidCreate(&pEntry[1].UniquePartitionGuid);
        pEntry[1].StartingLBA = pEntry[0].EndingLBA + 1;
        if (geo.DiskSize.QuadPart >= (16ull * 1024 * 1024 * 1024))
            pEntry[1].EndingLBA = pEntry[1].StartingLBA + ((128 * 1024 * 1024) / DISK_SECTOR_BYTES) - 1;
        else
            pEntry[1].EndingLBA = pEntry[1].StartingLBA + ((32 * 1024 * 1024) / DISK_SECTOR_BYTES) - 1;
        CopyMemory(pEntry[1].PartitionName, L"MSFT", 10);

        CopyMemory(&pEntry[2].PartitionTypeGuid, &BasicDataPartitionGuid, sizeof(GUID));
        UuidCreate(&pEntry[2].UniquePartitionGuid);
        pEntry[2].StartingLBA = pEntry[1].EndingLBA + 1;
        pEntry[2].EndingLBA = pGPT->Header.LastUsableLBA;
        CopyMemory(pEntry[2].PartitionName, L"DATA", 10);

        pGPT->Header.PartitionEntryArrayCRC32 =
            CRC_Calc32(0, pEntry, pGPT->Header.SizeOfPartitionEntry * pGPT->Header.NumberOfPartitionEntries);

        pGPT->Header.HeaderCRC32 = 0;

        pGPT->Header.HeaderCRC32 =
            CRC_Calc32(0, pGPT, pGPT->Header.HeaderSize);

        if (0 != SetFilePointer(hDisk, 0, NULL, FILE_BEGIN))
        {
            AbortMessage(TEXT("Could not seek to beginning of disk for write"));
            break;
        }

        if (!WriteFile(hDisk, theApp.mpEFIData, DISK_HEADER_SIZE, &result, NULL))
        {
            AbortMessage(TEXT("Disk Write Failed"));
            break;
        }

        pGPT->Header.MyLBA = pGPT->Header.AlternateLBA;
        pGPT->Header.AlternateLBA = 1;

        pGPT->Header.PartitionEntryLBA = diskEnd.QuadPart - PARTITION_TABLE_LBA_COUNT;

        pGPT->Header.HeaderCRC32 = 0;

        pGPT->Header.HeaderCRC32 =
            CRC_Calc32(0, pGPT, pGPT->Header.HeaderSize);

        diskEnd.QuadPart *= DISK_SECTOR_BYTES;

        if (!SetFilePointerEx(hDisk, diskEnd, &diskEnd, FILE_BEGIN))
        {
            AbortMessage(TEXT("Could not seek to end of disk"));
            break;
        }

        if (!WriteFile(hDisk, pGPT, DISK_SECTOR_BYTES, &result, NULL))
        {
            AbortMessage(TEXT("Could not write alternate GPT"));
            break;
        }

        diskEnd.QuadPart -= (PARTITION_TABLE_LBA_COUNT * DISK_SECTOR_BYTES);

        if (!SetFilePointerEx(hDisk, diskEnd, &diskEnd, FILE_BEGIN))
        {
            AbortMessage(TEXT("Could not seek to alternate partition table"));
            break;
        }

        if (!WriteFile(hDisk, pEntry, PARTITION_TABLE_LBA_COUNT * DISK_SECTOR_BYTES, &result, NULL))
        {
            AbortMessage(TEXT("Could not write alternate partition table"));
            break;
        }

        ret = true;

    } while (false);

    CloseHandle(hDisk);

    if (!ret)
        return false;

    /* resync */
    theApp.mpServ->Refresh();

    /* find the disk now that the service can see it as new */

    ret = false;

    IEnumVdsObject *pEnumObj = NULL;

    hr = theApp.mpBasic->QueryPacks(&pEnumObj);
    if (FAILED(hr))
        return AbortMessage(TEXT("Error 0x%08X enumerating local Disk Packs."),hr);

    pEnumObj->Reset();
    do {
        ULONG fetched = 0;
        IUnknown *pUnk = NULL;
        hr = pEnumObj->Next(1,&pUnk,&fetched);
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
                                if (!memcmp(&theApp.mDiskProp.id,&theApp.mpDisk->mDiskId,sizeof(theApp.mDiskProp.id)))
                                {
                                    theApp.mpVdsDisk = pDisk;
                                    pDisk->AddRef();
                                    theApp.mpVdsPack = pPack;
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

    pEnumObj->Release();
    pEnumObj = NULL;

    if (!ret)
    {
        AbortMessage(TEXT("Unknown error finding the target disk.  Please check the dialog and then re-try."));
        return false;
    }

    return ret;
}

bool CProgDlg::State_CreateVolume(void)
{
    bool ret;
    HRESULT hr;
    IEnumVdsObject *pEnumObj;

    //
    // need volume id of partition on the new disk
    //
    ret = false;
    do
    {
        hr = theApp.mpVdsPack->QueryVolumes(&pEnumObj);

        if (FAILED(hr))
        {
            AbortMessage(TEXT("Could not enumerate volumes on new disk"));
            ret = false;
            break;
        }
        do
        {
            ULONG fetched = 0;
            IUnknown *pUnk = NULL;
            hr = pEnumObj->Next(1, &pUnk, &fetched);
            if ((FAILED(hr)) || (!fetched))
                break;
            IVdsVolume *pVol = NULL;
            hr = pUnk->QueryInterface(IID_IVdsVolume, (void **)&pVol);
            pUnk->Release();
            pUnk = NULL;
            if (!FAILED(hr))
            {
                VDS_VOLUME_PROP volProp;

                hr = pVol->GetProperties(&volProp);
                if (!FAILED(hr))
                {
                    CopyMemory(&theApp.mNewVolumeId, &volProp.id, sizeof(GUID));
                    ret = true;
                }

                pVol->Release();
            }
        } while (true);

        pEnumObj->Release();

    } while (false);

    if (!ret)
    {
        AbortMessage(TEXT("Could not find raw volume on newly partitioned disk"));
        return false;
    }

    return true;
}

bool CProgDlg::State_StartCreateFileSys(void)
{
    /* auto-pick cluster size based on partition size.
       for more info, see Fat technical reference @ http://technet.microsoft.com/en-us/library/cc776720.aspx */
    IUnknown *pUnk;
    HRESULT hr = theApp.mpServ->GetObjectW(theApp.mNewVolumeId,VDS_OT_VOLUME,&pUnk);
    if (FAILED(hr))
        return AbortMessage(TEXT("Could not acquire new volume on new disk.(hr=0x%08X)."),hr);
    IVdsVolumeMF *pVolMF = NULL;
    hr = pUnk->QueryInterface(IID_IVdsVolumeMF,(void **)&pVolMF);
    pUnk->Release();
    if (FAILED(hr))
        return AbortMessage(TEXT("Could not acquire volume manipulation interface on new disk volume.(hr=0x%08X)."),hr);

    bool ret = true;
    do {
        /* bring the volume online and format it */
        hr = pVolMF->Mount();
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Could not mount new disk volume.(hr=0x%08X)."),hr);
            break;
        }
        VDS_FILE_SYSTEM_PROP fsProp;
        hr = pVolMF->GetFileSystemProperties(&fsProp);
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Could not get properties of the new disk volume (hr=0x%08X)."),hr);
            break;
        }
        mpWorkAsync = NULL;
        VDS_FILE_SYSTEM_TYPE targetType;
        targetType = VDS_FST_FAT32;
        hr = pVolMF->Format(targetType,TEXT("BOOTCARD"),0,TRUE,TRUE,FALSE,&mpWorkAsync);
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Could not format the new disk volume (hr=0x%08X)."),hr);
            break;
        }

        /* async is running here */

    } while (false);

    pVolMF->Release();

    return ret;
}

bool CProgDlg::State_ContinueCreateFileSys(void)
{
    ULONG percentComplete = 0;
    HRESULT hrAsync;
    
    HRESULT hr = mpWorkAsync->QueryStatus(&hrAsync,&percentComplete);

    if ((!FAILED(hr)) && (hrAsync==VDS_E_OPERATION_PENDING))
    {
        if (percentComplete)
        {
            percentComplete = 40 + ((percentComplete * 55)/100);
            mProgBar.SetPos(percentComplete);
        }
        return true;
    }

    mProgBar.SetPos(60);

    VDS_ASYNC_OUTPUT asyncOut;
    hr = mpWorkAsync->Wait(&hrAsync,&asyncOut);
    mpWorkAsync->Release();
    mpWorkAsync = NULL;

    bool ret = true;
    do  {
        if ((FAILED(hr)) || (FAILED(hrAsync)))
        {
            ret = AbortMessage(TEXT("Could not format the new disk (hr=0x%08X)."),hrAsync);
            break;
        }

        IUnknown *pUnk;
        HRESULT hr = theApp.mpServ->GetObjectW(theApp.mNewVolumeId,VDS_OT_VOLUME,&pUnk);
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Could not acquire new formatted volume on new disk.(hr=0x%08X)."),hr);
            break;
        }

        mpWorkVol = NULL;
        hr = pUnk->QueryInterface(IID_IVdsVolume,(void **)&mpWorkVol);
        pUnk->Release();
        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Could not acquire volume manipulation interface on formatted disk volume.(hr=0x%08X)."),hr);
            break;
        }

        VDS_VOLUME_PROP volProp;
        hr = mpWorkVol->GetProperties(&volProp);

        if (FAILED(hr))
        {
            ret = AbortMessage(TEXT("Could not get properties of new formatted volume. (hr=0x%08X)."),hr);
            break;
        }

        mDoneOuter = true;
    
    } while (false);

    return ret;
}

void CProgDlg::State_MakeAccessPath(void)
{
    VDS_VOLUME_PROP volProp;
    if (FAILED(mpWorkVol->GetProperties(&volProp)))
        return;

    DWORD logDrives = GetLogicalDrives();
    DWORD chk = (1<<25);
    WCHAR pathName[8] = TEXT("Z:\\"); 
    while (logDrives)
    {
        if (!(logDrives & chk))
        {
            DWORD dwType = GetDriveType(pathName);
            if (dwType==DRIVE_NO_ROOT_DIR)
            {
                TCHAR actDevice[MAX_PATH];
                if (!QueryDosDevice(pathName,actDevice,MAX_PATH-1))
                    break;
                OutputDebugString(TEXT("\r\n"));
                OutputDebugString(pathName);
                OutputDebugString(TEXT(" is ["));
                OutputDebugString(actDevice);
                OutputDebugString(TEXT("]\r\n"));
                break;
            }
        }
        pathName[0]--;
        logDrives &= ~chk;
        chk >>= 1;
    }

    IVdsVolumeMF *pMF = NULL;
    if (FAILED(mpWorkVol->QueryInterface(IID_IVdsVolumeMF,(void **)&pMF)))
        return;
    pMF->Mount();
    pMF->AddAccessPath(pathName);
    pMF->Release();
}

#if 0
static void sFindMe(UINT8 const* apFind, UINT32 aLeft)
{
    do {
        if (*apFind != 'F')
        {
            apFind++;
            aLeft--;
        }
        else
        {
            if (aLeft >= 8)
            {
                if (0 == strncmp((char const*)apFind, "FINDMEEE", 8))
                    break;
            }
            apFind++;
            aLeft--;
        }
    } while (aLeft);
}
#endif

bool CProgDlg::State_UpdateEFI(void)
{
    HANDLE hDisk;
    bool   ret;

    hDisk = CreateFile(theApp.mDiskProp.pwszDevicePath,
        GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
    if (hDisk == INVALID_HANDLE_VALUE)
    {
        AbortMessage(TEXT("Could not open target disk for processing."));
        return false;
    }

    ret = false;
    do
    {
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hDisk, 0, NULL, FILE_BEGIN))
        {
            AbortMessage(TEXT("Could not seek to sector 0 on disk."));
            break;
        }

        DWORD red = 0;

//        if (!ReadFile(hDisk, theApp.mpEFIData, DISK_HEADER_SIZE, &red, NULL))
        if (!ReadFile(hDisk, theApp.mpEFIData, DISK_SECTOR_BYTES * 2, &red, NULL))
        {
            AbortMessage(TEXT("Could not read target disk."));
            break;
        }

#if 0
        HANDLE hOut = CreateFile(L"out.img", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hOut != INVALID_HANDLE_VALUE)
        {
            WriteFile(hOut, theApp.mpEFIData, DISK_HEADER_SIZE, &red, NULL);
            CloseHandle(hOut);
        }

//        sFindMe(theApp.mpEFIData, DISK_HEADER_SIZE);
#endif

        if (INVALID_SET_FILE_POINTER == SetFilePointer(hDisk, DISK_SECTOR_BYTES * PARTITION_TABLE_LBA_COUNT, NULL, FILE_BEGIN))
        {
            AbortMessage(TEXT("Could not seek to sector for partition table on disk."));
            break;
        }

        red = 0;
        if (!ReadFile(hDisk, theApp.mpEFIData + (DISK_SECTOR_BYTES * PARTITION_TABLE_START_LBA), (PARTITION_TABLE_ENTRY_COUNT *128), &red, NULL))
        {
            AbortMessage(TEXT("Could not read target disk partition table."));
            break;
        }

        //
        // loaded EFI is patched with target disk partition table now
        //
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hDisk, 0, NULL, FILE_BEGIN))
        {
            AbortMessage(TEXT("Could not seek to sector 0 on disk."));
            break;
        }

        DWORD result = 0;
        if (!WriteFile(hDisk, theApp.mpEFIData, DISK_HEADER_SIZE, &result, NULL))
        {
            AbortMessage(TEXT("Disk Write Failed"));
            break;
        }

        ret = true;

    } while (false);

    CloseHandle(hDisk);

    return ret;
}
