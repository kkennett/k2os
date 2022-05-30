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
#include <lib/k2win32.h>
#include <lib/k2mem.h>
#include <lib/k2parse.h>

struct ArgTrack
{
    int     mCount;
    int     mIx;
    char ** mppArg;

    ArgTrack(void)
    {
        mCount = 0;
        mIx = 0;
        mppArg = NULL;
    }

    ~ArgTrack(void)
    {
        int ix;
        for(ix=0;ix<mCount;ix++)
        {
            if (mppArg[ix])
            {
                delete [] mppArg[ix];
                mppArg[ix] = NULL;
            }
        }
        if (mppArg)
        {
            delete [] mppArg;
            mppArg = NULL;
        }
    }

    bool Load(char const *apFileName);
};

ArgParser::ArgParser(void)
{
    mCmdArgc = -1;
    mpCmdArgv = NULL;
    mCmdIx = -1;
    mppFiles = NULL;
}

bool ArgParser::Init(int cmdArgc, char **cmdArgv)
{
    bool ok;
    int ix;

    if ((cmdArgc<1) || (!cmdArgv) || (!cmdArgv[0]))
        return false;

    mCmdArgc = cmdArgc;
    mpCmdArgv = cmdArgv;

    mppFiles = (void **)(new ArgTrack * [mCmdArgc]);
    if (!mppFiles)
        return false;
    K2MEM_Zero(mppFiles, sizeof(void *) * mCmdArgc);

    for(ix=0;ix<mCmdArgc;ix++)
    {
        if (*(cmdArgv[ix]) == '@')
        {
            /* argument is a file */
            ArgTrack *pTrack = new ArgTrack;
            if (!pTrack)
                return false;

            ok = pTrack->Load((cmdArgv[ix])+1);
            if (!ok)
            {
                delete pTrack;
                return false;
            }

            mppFiles[ix] = pTrack;
        }
    }

    Reset();

    return true;
}

ArgParser::~ArgParser(void)
{
    int ix;
    if (!mppFiles)
        return;

    for(ix=0;ix<mCmdArgc;ix++)
    {
        if (mppFiles[ix])
        {
            ArgTrack *pTrack = (ArgTrack *)mppFiles[ix];
            delete pTrack;
            mppFiles[ix] = NULL;
        }
    }

    delete [] mppFiles;
}

void ArgParser::Reset(void)
{
    int ix;

    if (!mppFiles)
        return;

    for(ix=0;ix<mCmdArgc;ix++)
    {
        if (mppFiles[ix])
        {
            ArgTrack *pTrack = (ArgTrack *)mppFiles[ix];
            pTrack->mIx = 0;
        }
    }

    mCmdIx = 0;

    /* advance to first argument */
    do {
        if (!mppFiles[mCmdIx])
            break;
        ArgTrack *pTrack = (ArgTrack *)mppFiles[mCmdIx];
        if (pTrack->mCount)
            break;
        mCmdIx++;
    } while (mCmdIx < mCmdArgc);
}

void ArgParser::Advance(void)
{
    if (!mppFiles)
        return;

    if (mCmdIx == mCmdArgc)
        return;

    if (!mppFiles[mCmdIx])
    {
        /* not in a file - advance to first valid argument from here */
        mCmdIx++;
        if (mCmdIx == mCmdArgc)
            return;
        do {
            if (!mppFiles[mCmdIx])
                break;
            ArgTrack *pTrack = (ArgTrack *)mppFiles[mCmdIx];
            if (pTrack->mCount)
                break;
            mCmdIx++;
        } while (mCmdIx < mCmdArgc);
        return;
    }

    /* in a file - advance to next valid argument from here */
    while (mppFiles[mCmdIx])
    {
        ArgTrack *pTrack = (ArgTrack *)mppFiles[mCmdIx];
        if (pTrack->mCount)
            pTrack->mIx++;
        if (pTrack->mIx < pTrack->mCount)
            return;
        if (++mCmdIx == mCmdArgc)
            break;
    }
}

bool ArgParser::Left(void)
{
    if (!mppFiles)
        return false;
    return (mCmdIx != mCmdArgc);
}

char const * ArgParser::Arg(void)
{
    if ((!mppFiles) || (mCmdIx == mCmdArgc))
        return "";

    if (mppFiles[mCmdIx])
    {
        ArgTrack *pTrack = (ArgTrack *)mppFiles[mCmdIx];
        return pTrack->mppArg[pTrack->mIx];
    }

    return mpCmdArgv[mCmdIx];
}

bool ArgTrack::Load(char const *apFileName)
{
    bool ok;
    UINT_PTR pathLen = GetFullPathName(apFileName, 0, NULL, NULL);
    char *pActPath = new char[(pathLen+4)&~3];
    if (!pActPath)
        return false;
    GetFullPathName(apFileName, (DWORD)(pathLen+1), pActPath, NULL);

    K2ReadOnlyMappedFile *pFile;
    pFile = K2ReadOnlyMappedFile::Create(pActPath);

    delete [] pActPath;

    if (pFile == NULL)
        return false;

    char const *pPars;
    pPars = (char const *)pFile->DataPtr();
    UINT_PTR left = (UINT_PTR)pFile->FileBytes();

    ok = true;
    do {
        char const *pLine;
        UINT_PTR lineLen;
		bool balancedQuotes;
		bool quotes;

        K2PARSE_EatLine(&pPars, &left, &pLine, &lineLen);
        if (lineLen)
        {
            K2PARSE_EatWhitespace(&pLine, &lineLen);
            if (lineLen)
            {
                K2PARSE_EatWhitespaceAtEnd(pLine,&lineLen);
                if (lineLen)
                {
                    /* parse args on this nonempty line */
                    do {
                        char const *pTok = pLine;

						balancedQuotes = false;
						quotes = false;
                        do {
                            if (((*pLine==' ') && !balancedQuotes) || (*pLine=='\t') || (*pLine=='\r') || (*pLine=='\n'))
                                break;
							// preserve quotes now, strip them later
                            if (*pLine=='"')
							{
								balancedQuotes = !balancedQuotes;
								quotes = true;
							}
                            pLine++;
                            lineLen--;
                        } while (lineLen);
                        int tokLen = (int)(pLine-pTok);

                        /* convert token to unicode */
                        char *pConv = new char[tokLen+4];
                        if (!pConv)
                        {
                            ok = false;
                            break;
                        }
                        K2MEM_Copy(pConv, pTok, tokLen+1);
						/* Quotes are necessary when there is a space in a
						 * command.  They are no longer necessary once they are 
						 * read into the program and passed to Windows file system
						 * calls; that is, when they become a 'string'.
						 */
						if (quotes)
						{
							char *apStr = pConv;
							char *pTmp;
							char tmpCh;
							
							// strip all quotes
							pTmp = apStr;
							tmpCh = *pTmp;
							while (tmpCh != 0)
							{
								if (tmpCh != '"')
								{
									*apStr = tmpCh;
									apStr++;
								}
								pTmp++;
								tmpCh = *pTmp;
							}
							// copy NULL char
							*apStr = tmpCh;
						}
						
                        char **pNewList = new char *[mCount+1];
                        if (!pNewList)
                        {
                            delete [] pConv;
                            ok = false;
                            break;
                        }
                        if (mCount)
                        {
                            K2MEM_Copy(pNewList, mppArg, sizeof(char *) * mCount);
                            delete [] mppArg;
                        }
                        mppArg = pNewList;

                        mppArg[mCount] = pConv;
                        mCount++;

                        K2PARSE_EatWhitespace(&pLine,&lineLen);
                    } while (lineLen);
                    if (!ok)
                        break;
                }
            }
        }
    } while (left);

    pFile->Release();
    pFile = NULL;

    return ok;
}