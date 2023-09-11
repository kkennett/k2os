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
#include <lib/k2win32.h>
#include <lib/k2atomic.h>

INT_PTR K2MappedFile::AddRef(void)
{
    return K2ATOMIC_Inc(&mRefs);
}

INT_PTR K2MappedFile::Release(void)
{
    INT_PTR ret = K2ATOMIC_Dec(&mRefs);
    if (ret == 0)
        delete this;
    return ret;
}

K2ReadOnlyMappedFile * K2ReadOnlyMappedFile::Create(char const *apFileName)
{
	void const *            pData;
	HANDLE                  hMap;
	HANDLE                  hFile;
	LARGE_INTEGER           liSize;
	K2ReadOnlyMappedFile *  pRet;
	FILETIME				fileTime;

	if ((apFileName == NULL) ||
		((*apFileName) == 0))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	hFile = CreateFile(apFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	pRet = NULL;
	hMap = NULL;

	do
	{
		if (!GetFileSizeEx(hFile, &liSize))
			break;

		if (!GetFileTime(hFile, NULL, NULL, &fileTime))
			break;

		if (liSize.HighPart != 0)
			break;  /* file is too big */

		hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, liSize.HighPart, liSize.LowPart, NULL);
		if (hMap == NULL)
			break;

		pData = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
		if (pData != NULL)
			pRet = new K2ReadOnlyMappedFile(apFileName, hFile, hMap, liSize.QuadPart, fileTime, pData);

	} while (false);

	if (pRet == NULL)
	{
		if (hMap != NULL)
			CloseHandle(hMap);
		CloseHandle(hFile);
	}

	return pRet;
}

K2ReadWriteMappedFile * K2ReadWriteMappedFile::Create(char const *apFileName)
{
	void *                  pData;
	HANDLE                  hMap;
	HANDLE                  hFile;
	LARGE_INTEGER           liSize;
	K2ReadWriteMappedFile * pRet;
	FILETIME				fileTime;

	if ((apFileName == NULL) ||
		((*apFileName) == 0))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	hFile = CreateFile(apFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	pRet = NULL;
	hMap = NULL;

	do
	{
		if (!GetFileSizeEx(hFile, &liSize))
			break;

		if (!GetFileTime(hFile, NULL, NULL, &fileTime))
			break;

		if (liSize.HighPart != 0)
			break;  /* file is too big */

		hMap = CreateFileMapping(hFile, NULL, PAGE_WRITECOPY, liSize.HighPart, liSize.LowPart, NULL);
		if (hMap == NULL)
			break;

		pData = MapViewOfFile(hMap, FILE_MAP_COPY, 0, 0, 0);
		if (pData != NULL)
			pRet = new K2ReadWriteMappedFile(apFileName, hFile, hMap, liSize.QuadPart, fileTime, pData);

	} while (false);

	if (pRet == NULL)
	{
		if (hMap != NULL)
			CloseHandle(hMap);
		CloseHandle(hFile);
	}

	return pRet;
}


