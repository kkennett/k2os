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
#ifndef __K2WIN32_H
#define __K2WIN32_H

#include <stdio.h>
#include <Windows.h>
#include <k2systype.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus

class K2MappedFile
{
public:
    INT_PTR AddRef(void);
    INT_PTR Release(void);

    char const * const FileName(void) const
    {
        return mpFileName; 
    }

    UINT64 const & FileBytes(void) const
    {
        return mFileBytes;
    }

    FILETIME const & FileTime(void) const
    {
        return mFileTime;
    }

protected:
    K2MappedFile(char const *apFileName, HANDLE ahFile, HANDLE ahMap, UINT64 aFileBytes, FILETIME const &aFileTime, void *apMapped) :
        mhFile(ahFile),
        mhMap(ahMap),
        mFileBytes(aFileBytes),
        mpMapped(apMapped),
        mFileTime(aFileTime)
    {
        int copyLen = (int)strlen(apFileName);
        mpFileName = new char[copyLen+4];
        strcpy_s(mpFileName,copyLen+1,apFileName);
        mRefs = 1;
    }

    virtual ~K2MappedFile(void)
    {
        UnmapViewOfFile(mpMapped);
        CloseHandle(mhMap);
        CloseHandle(mhFile);
        delete [] mpFileName;
    }

    volatile INT_PTR    mRefs;
    char *              mpFileName;
    HANDLE const        mhFile;
    HANDLE const        mhMap;
    UINT64 const        mFileBytes;
    FILETIME const      mFileTime;
    void * const        mpMapped;
};

class K2ReadOnlyMappedFile : public K2MappedFile
{
public:
    static K2ReadOnlyMappedFile * Create(char const *apFileName);

    void const * DataPtr(void) const
    {
        return mpMapped;
    }
private:
    K2ReadOnlyMappedFile(char const *apFileName, HANDLE ahFile, HANDLE ahMap, UINT64 aFileBytes, FILETIME const &aFileTime, void const *apMapped) :
        K2MappedFile(apFileName, ahFile, ahMap, aFileBytes, aFileTime, (void *)apMapped)
    {
    }
};

class K2ReadWriteMappedFile : public K2MappedFile
{
public:
    static K2ReadWriteMappedFile * Create(char const *apFileName);

    void * DataPtr(void) const
    {
        return mpMapped;
    }
private:
    K2ReadWriteMappedFile(char const *apFileName, HANDLE ahFile, HANDLE ahMap, UINT64 aFileBytes, FILETIME const &aFileTime, void *apMapped) :
        K2MappedFile(apFileName, ahFile, ahMap, aFileBytes, aFileTime, apMapped)
    {
    }
};

//
//------------------------------------------------------------------------
//

class ArgParser
{
public:
    ArgParser(void);
    ~ArgParser(void);

    bool Init(int cmdArgc, char **cmdArgv);
    void Reset(void);
    void Advance(void);
    bool Left(void);
    char const * Arg(void);

private:
    int         mCmdArgc;
    char **     mpCmdArgv;
    int         mCmdIx;
    void **     mppFiles;
};

//
//------------------------------------------------------------------------
//

class DataPortBuffer
{
public:
    DataPortBuffer(void)
    {
        mpBuffer = NULL;
        mSizeBytes = 0;
        mpNext = NULL;
        mOnChain = false;
        mFullBytes = 0;
    }

    DataPortBuffer(UINT8 *apBuffer, UINT_PTR aByteCount) : DataPortBuffer()
    {
        Set(apBuffer, aByteCount);
    }

    static DataPortBuffer * Alloc(UINT_PTR aByteCount)
    {
        UINT8 *             pBuffer;
        DataPortBuffer *    pResult;

        if (0 == aByteCount)
            return NULL;

        pBuffer = new UINT8[aByteCount];
        if (NULL == pBuffer)
            return NULL;

        //
        // always at least one extra byte in the buffer longer than the specified size
        //
        pResult = new DataPortBuffer(pBuffer, (aByteCount + 4) & ~3);
        if (NULL == pResult)
        {
            delete[] pBuffer;
            return NULL;
        }

        return pResult;
    }

    virtual ~DataPortBuffer(void)
    {
        if (mOnChain)
        {
            DebugBreak();
        }

        if (NULL != mpBuffer)
        {
            delete[] mpBuffer;
            mpBuffer = NULL;
        }
        mSizeBytes = 0;
        mFullBytes = 0;
    }

    UINT8 * Ptr(void) const 
    { 
        return mpBuffer; 
    }
    
    UINT_PTR const & SizeBytes(void) const 
    { 
        return mSizeBytes; 
    }
    
    UINT_PTR const & FullBytes(void) const 
    { 
        return mFullBytes; 
    }

    void Set(UINT8 * apBuffer, UINT_PTR aByteCount)
    {
        if (NULL != mpBuffer)
        {
            delete[] mpBuffer;
        }
        mpBuffer = apBuffer;
        mSizeBytes = aByteCount;
        mFullBytes = 0;
    }

    void SetFull(UINT_PTR aAmount)
    {
        if (aAmount > mSizeBytes)
        {
            DebugBreak();
        }
        mFullBytes = aAmount;
    }

private:
    friend class DataPortBufferChain;
    UINT8 *             mpBuffer;
    UINT_PTR            mSizeBytes;
    DataPortBuffer *    mpNext;
    UINT_PTR            mFullBytes;
    bool                mOnChain;
};

class DataPortBufferChain
{
public:
    DataPortBufferChain(void)
    {
        mInitialized = false;
        mpHead = NULL;
        mpTail = NULL;
        mhEmptyEvent = NULL;
        mhNotEmptyEvent = NULL;
        InitializeCriticalSection(&mSec);
    }
    virtual ~DataPortBufferChain(void)
    {
        EnterCriticalSection(&mSec);
        Purge();
        DeleteCriticalSection(&mSec);
    }

    HANDLE NotEmptyEvent(void)  
    {
        if (!Init())
            return NULL;
        return mhNotEmptyEvent; 
    }

    HANDLE EmptyEvent(void)  
    {
        if (!Init())
            return NULL;
        return mhEmptyEvent;
    }

    void Purge(void)
    {
        DataPortBuffer *pBuf;
        EnterCriticalSection(&mSec);
        do {
            pBuf = Dequeue();
            if (NULL == pBuf)
                break;
            delete pBuf;
        } while (true);
        LeaveCriticalSection(&mSec);
    }

    bool Init(void)
    {
        EnterCriticalSection(&mSec);
        
        if (!mInitialized)
        {
            mpHead = NULL;
            mhEmptyEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
            if (NULL == mhEmptyEvent)
            {
                LeaveCriticalSection(&mSec);
                return false;
            }
            mhNotEmptyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (NULL == mhNotEmptyEvent)
            {
                CloseHandle(mhEmptyEvent);
                mhEmptyEvent = NULL;
                LeaveCriticalSection(&mSec);
                return false;
            }
            mInitialized = true;
        }

        LeaveCriticalSection(&mSec);

        return mInitialized;
    }

    bool Queue(DataPortBuffer *apBuffer)
    {
        if (apBuffer->mOnChain)
        {
            DebugBreak();
        }

        if (0 == apBuffer->FullBytes())
            return false;

        EnterCriticalSection(&mSec);
        
        if (!Init())
        {
            LeaveCriticalSection(&mSec);
            return false;
        }

        if (NULL != mpTail)
        {
            if (NULL == mpHead)
            {
                DebugBreak();
            }
            mpTail->mpNext = apBuffer;
            mpTail = apBuffer;
        }
        else
        {
            if (NULL != mpHead)
            {
                DebugBreak();
            }
            mpHead = mpTail = apBuffer;
        }
        SetEvent(mhNotEmptyEvent);

        LeaveCriticalSection(&mSec);
        
        return true;
    }

    DataPortBuffer * Dequeue(void)
    {
        DataPortBuffer * pResult;

        EnterCriticalSection(&mSec);

        if (!Init())
        {
            LeaveCriticalSection(&mSec);
            return NULL;
        }

        pResult = mpHead;
        if (NULL != pResult)
        {
            mpHead = pResult->mpNext;
            if (NULL == mpHead)
            {
                ResetEvent(mhNotEmptyEvent);
                mpTail = NULL;
                SetEvent(mhEmptyEvent);
            }
        }

        LeaveCriticalSection(&mSec);

        return pResult;
    }

private:
    bool                mInitialized;
    CRITICAL_SECTION    mSec;
    HANDLE              mhEmptyEvent;
    HANDLE              mhNotEmptyEvent;
    DataPortBuffer *    mpHead;
    DataPortBuffer *    mpTail;
};

class DataPort
{
public:
    virtual ~DataPort(void)
    {
        mInQueue.Purge();
        mOutQueue.Purge();
        if (NULL != mhConnectedEvent)
        {
            CloseHandle(mhConnectedEvent);
            mhConnectedEvent = NULL;
        }
        if (NULL != mhDisconnectedEvent)
        {
            CloseHandle(mhDisconnectedEvent);
            mhDisconnectedEvent = NULL;
        }
    }

    HANDLE ConnectedEvent(void) 
    { 
        return mhConnectedEvent; 
    }
    
    HANDLE DisconnectedEvent(void) 
    { 
        return mhDisconnectedEvent; 
    }
    
    HANDLE ReadAvailEvent(void) 
    { 
        return mInQueue.NotEmptyEvent(); 
    }
    
    DataPortBuffer * Read(void)  
    { 
        return mInQueue.Dequeue(); 
    }

    bool Write(DataPortBuffer *apBuffer) 
    { 
        return mOutQueue.Queue(apBuffer); 
    }

    HANDLE WriteBufferEmptyEvent(void)
    {
        return mOutQueue.EmptyEvent();
    }

protected:
    DataPort(
        HANDLE ahConnectedEvent,
        HANDLE ahDisconnectedEvent
    )
    {
        mhConnectedEvent = ahConnectedEvent;
        mhDisconnectedEvent = ahDisconnectedEvent;
        ZeroMemory(&mOvRecv, sizeof(mOvSend));
        ZeroMemory(&mOvSend, sizeof(mOvSend));
    }

    HANDLE              mhConnectedEvent;
    HANDLE              mhDisconnectedEvent;
    DataPortBufferChain mInQueue;
    DataPortBufferChain mOutQueue;
    OVERLAPPED          mOvRecv;
    OVERLAPPED          mOvSend;
};

class PipeServerDataPort : public DataPort
{
public:
    static PipeServerDataPort * Open(char const *apPipePath);

    void Disconnect(void);

    ~PipeServerDataPort(void)
    {
        if (NULL != mhThread)
        {
            SetEvent(mhStopEvent);
            WaitForSingleObject(mhThread, INFINITE);
            CloseHandle(mhStopEvent);
            mhStopEvent = NULL;
            CloseHandle(mhThread);
            mhThread = NULL;
        }
        if (NULL != mhManualDisconnectEvent)
        {
            CloseHandle(mhManualDisconnectEvent);
            mhManualDisconnectEvent = NULL;
        }
        if (NULL != mhReadCompletedEvent)
        {
            CloseHandle(mhReadCompletedEvent);
            mhReadCompletedEvent = NULL;
        }
        if (NULL != mhWriteCompletedEvent)
        {
            CloseHandle(mhWriteCompletedEvent);
            mhWriteCompletedEvent = NULL;
        }
        if (NULL != mhPipe)
        {
            CloseHandle(mhPipe);
            mhPipe = NULL;
        }
    }

private:
    PipeServerDataPort(
        HANDLE ahPipe,
        HANDLE ahConnectedEvent,
        HANDLE ahDisconnectedEvent,
        HANDLE ahReadCompletedEvent,
        HANDLE ahWriteCompletedEvent,
        HANDLE ahManualDisconnectEvent,
        HANDLE ahStopEvent
    ) : DataPort(ahConnectedEvent, ahDisconnectedEvent)
    {
        mhPipe = ahPipe;
        mhReadCompletedEvent = ahReadCompletedEvent;
        mhWriteCompletedEvent = ahWriteCompletedEvent;
        mhManualDisconnectEvent = ahManualDisconnectEvent;
        mhStopEvent = ahStopEvent;
        mhThread = NULL;
        ZeroMemory(&mOvConnect, sizeof(mOvConnect));
    }

    static DWORD WINAPI sThread(void *apParam);
    DWORD Thread(void);
    void RunConnected(bool *apStopNow);
    void RunWaitConnect(bool *apStopNow);

    HANDLE      mhPipe;
    HANDLE      mhThread;
    DWORD       mThreadId;
    HANDLE      mhManualDisconnectEvent;
    HANDLE      mhReadCompletedEvent;
    HANDLE      mhWriteCompletedEvent;
    HANDLE      mhStopEvent;
    OVERLAPPED  mOvConnect;
};

class ComDataPort : public DataPort
{
public:
    static ComDataPort * Open(UINT_PTR aPortNumber);
    void Close(void);

    ~ComDataPort(void)
    {
        if (NULL != mhComPort)
        {
            CloseHandle(mhComPort);
            mhComPort = NULL;
        }
    }

private:
    ComDataPort(void) : DataPort(NULL, NULL)
    {
        mhComPort = NULL;
    }

    HANDLE  mhComPort;
};

//
//------------------------------------------------------------------------
//

typedef struct _EnvironEntry EnvironEntry;
struct _EnvironEntry
{
    char *      mpName;
    UINT_PTR    mNameLen;
    char *      mpValue;
    UINT_PTR    mValueLen;
};

class Environ
{
public:
    Environ(bool aInit);

    Environ(Environ const &aCopyFrom);

    ~Environ(void)
    {
        UINT_PTR ix;

        if (NULL != mpBlob)
        {
            delete[] mpBlob;
            mpBlob = NULL;
        }
        if (0 != mNumEntries)
        {
            K2_ASSERT(NULL != mppEntry);
            for (ix = 0; ix < mNumEntries; ix++)
            {
                delete mppEntry[ix];
                mppEntry[ix] = NULL;
            }
            delete[] mppEntry;
            mppEntry = NULL;
            mNumEntries = 0;
        }
    }

    UINT_PTR const &        NumEntries(void) const { return mNumEntries; }
    EnvironEntry const &    Entry(UINT_PTR aIndex) const { if (aIndex >= mNumEntries) return smBlankEntry; return *(mppEntry[aIndex]); }
    UINT_PTR                Find(char const *apName) const;

    bool                    Set(char const *apName, char const *apValue);
    char *                  CopyBlob(UINT_PTR *apRetLen);

private:
    static EnvironEntry const smBlankEntry;

    char *          mpBlob;
    UINT_PTR        mBlobLen;
    bool            mChanged;
    UINT_PTR        mNumEntries;
    EnvironEntry ** mppEntry;
};

//
//------------------------------------------------------------------------
//

char *
MakeTempFileName(
    char const *    apExt,
    UINT_PTR *      apRetLen
);

//
//------------------------------------------------------------------------
//

int 
RunProgram(
    char const *    apWorkDir,
    char const *    apCmdLine,
    char const *    apEnviron,
    char **         appRetOutput,
    UINT_PTR *      apRetOutputLen
);

//
//------------------------------------------------------------------------
//

extern "C" {
#endif

// c-only defs go here

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif // __K2WIN32_H
