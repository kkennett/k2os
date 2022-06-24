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
#include "k2elf2dlx.h"

int
TreeStrCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
    )
{
    return K2ASC_Comp((char const *)aKey, (char const *)apNode->mUserVal);
}

int main(int argc, char **argv)
{
    ArgParser   parse;
    char        ch;
    int         ret;

    if (!parse.Init(argc, argv))
    {
        printf("*** Could not parse command line arguments\n");
        return -1;
    }

    K2MEM_Zero(&gOut, sizeof(gOut));

    // bypass program name
    parse.Advance();

    while (parse.Left())
    {
        char const *pArg = parse.Arg();
        parse.Advance();

        if (*pArg == '-')
        {
            if ((pArg[1] == 0) ||
                (pArg[2] != 0))
            {
                printf("*** Argument error near \"%s\"\n", pArg);
                return -2;
            }
            if (!parse.Left())
            {
                printf("*** Switch '%c' with no argument\n", pArg[1]);
                return -3;
            }
            if (K2ASC_ToUpper(pArg[1])=='K')
            {
                gOut.mIsKernelDLX = true;
            }
            else
            {
                switch (pArg[1])
                {
                case 'o':
                case 'O':
                    if (!SetupFilePath(parse.Arg(), &gOut.mpOutputFilePath))
                        return -4;
                    break;

                case 'i':
                case 'I':
                    if (!SetupFilePath(parse.Arg(), &gOut.mpInputFilePath))
                        return -5;
                    break;

                case 'l':
                case 'L':
                    if (!SetupFilePath(parse.Arg(), &gOut.mpImportLibFilePath))
                        return -6;
                    break;

                case 's':
                case 'S':
                    if (!SetupEntryStackSize(parse.Arg(), &gOut.mEntryStackReq))
                        return -7;
                    break;

                default:
                    printf("*** Unknown switch '%c' found\n", pArg[1]);
                    return -7;
                }
                parse.Advance();
            }
        }
    }

    // if we get here we are ready to convert the sucker
    gOut.mpInput = K2ReadWriteMappedFile::Create(gOut.mpInputFilePath);
    if (gOut.mpInput == NULL)
    {
        printf("*** Could not open input file\n");
        return -8;
    }

    // get the target name (extensionless and lower case)
    if (gOut.mpInputFilePath == NULL)
    {
        printf("*** No input file specified\n");
        return -23;
    }
    gOut.mpTargetName = gOut.mpInputFilePath;
    while (*gOut.mpTargetName)
        gOut.mpTargetName++;
    do
    {
        gOut.mpTargetName--;
        ch = *gOut.mpTargetName;
        if ((ch == '/') || (ch == '\\'))
        {
            gOut.mpTargetName++;
            break;
        }
        if (ch == '.')
            gOut.mTargetNameLen = 0;
        else
        {
            *gOut.mpTargetName = K2ASC_ToLower(ch);
            gOut.mTargetNameLen++;
        }
    } while (gOut.mpTargetName != gOut.mpInputFilePath);
    if (gOut.mTargetNameLen == 0)
    {
        printf("*** Could not parse target DLX name from input path\n");
        return -9;
    }
//    printf("Target DLX \"%.*s\"\n", gOut.mTargetNameLen, gOut.mpTargetName);

    if (!ValidateLoad())
    {
        printf("*** Input file is invalid\n");
        return -10;
    }

    if (gOut.mpOutputFilePath == NULL)
    {
        printf("*** No output file specified\n");
        return -23;
    }
    gOut.mhOutput = CreateFile(gOut.mpOutputFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (gOut.mhOutput == INVALID_HANDLE_VALUE)
    {
        printf("*** Could not create output file\n");
        return -11;
    }

    if (gOut.mTotalExportCount > 0)
    {
        if (gOut.mpImportLibFilePath == NULL)
        {
            printf("*** No import library file path specified, and there are exports.\n");
            return -22;
        }
        gOut.mhOutLib = CreateFile(gOut.mpImportLibFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (gOut.mhOutLib == INVALID_HANDLE_VALUE)
        {
            printf("*** Could not create target import library file\n");
            return -12;
        }

        ret = CreateImportLib();

        if (gOut.mhOutLib != INVALID_HANDLE_VALUE)
        {
            CloseHandle(gOut.mhOutLib);
            if (ret != 0)
                DeleteFile(gOut.mpImportLibFilePath);
        }
    }
    else
    {
        if (gOut.mpImportLibFilePath != NULL)
            DeleteFile(gOut.mpImportLibFilePath);
        ret = 0;
    }

    if (ret == 0)
    {
        ret = Convert();
        if (ret == 0)
            ret = CreateTargetDLX();

        CloseHandle(gOut.mhOutput);
        if (ret != 0)
        {
            DeleteFile(gOut.mpOutputFilePath);
            if (gOut.mTotalExportCount > 0)
                DeleteFile(gOut.mpImportLibFilePath);
        }
        else
            VerifyAndDump();
    }

    return ret;
}

