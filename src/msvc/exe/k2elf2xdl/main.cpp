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

#include "k2elf2xdl.h"

OUTCTX gOut;

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
    ArgParser       args;
    UINT8 const *   pData;
    char            ch;
    char const *    pPath;
    char const *    pScan;
    char *          pConv;

    K2MEM_Zero(&gOut, sizeof(gOut));

    if (!args.Init(argc, argv))
    {
        printf("*** Could not set up argument parser.\n");
        return -1;
    }

    if (!args.Left())
    {
        printf("*** Error on command line, could not parse.\n");
        return -2;
    }

    //
    // eat program name (arg[0])
    //
    args.Advance();

    if (!args.Left())
    {
        printf("*** No arguments found\n");
        return -2;
    }

    K2MEM_Zero(&gOut, sizeof(gOut));

    //	@k2elf2xdl $(K2_SPEC_KERNEL) -s $(XDL_STACK) -i $(K2_TARGET_ELFFULL_SPEC) -o $(K2_TARGET_FULL_SPEC) -l $(K2_TARGET_EXPORTLIB)

        //
        // parse args and load files
        //
    while (args.Left())
    {
        char const *pArg = args.Arg();
        if ((pArg[0] == '-') ||
            (pArg[0] == '/'))
        {
            if ((!args.Left()) || (0 != pArg[2]))
            {
                return K2STAT_ERROR_BAD_ARGUMENT;
            }
            switch (pArg[1])
            {
            case 'k':
            case 'K':
                gOut.mSpecKernel = true;
                break;

            case 's':
            case 'S':
                args.Advance();
                gOut.mSpecStack = K2ASC_NumValue32(args.Arg());
                break;

            case 'i':
            case 'I':
                args.Advance();
                if (NULL != gOut.mpElfFile)
                {
                    printf("*** Double specification of input ELF file (-I)\n");
                    return K2STAT_ERROR_ALREADY_EXISTS;
                }
                gOut.mpElfFile = K2ReadOnlyMappedFile::Create(args.Arg());
                if (NULL == gOut.mpElfFile)
                {
                    printf("*** Could not load input ELF (%s)\n", args.Arg());
                    return K2STAT_ERROR_BAD_ARGUMENT;
                }
                break;

            case 'o':
            case 'O':
                args.Advance();
                if (NULL != gOut.mpOutputFilePath)
                {
                    printf("*** Double specification of output file path (-O)\n");
                    return K2STAT_ERROR_ALREADY_EXISTS;
                }
                gOut.mpOutputFilePath = args.Arg();
                break;

            case 'l':
            case 'L':
                args.Advance();
                if (NULL != gOut.mpImportLibFilePath)
                {
                    printf("*** Double specification of import library file path (-L)\n");
                    return K2STAT_ERROR_ALREADY_EXISTS;
                }
                gOut.mpImportLibFilePath = args.Arg();
                break;

            case 'p':
            case 'P':
                args.Advance();
                if (gOut.mUsePlacement)
                {
                    printf("*** More than one placement specified on command line\n");
                    return K2STAT_ERROR_ALREADY_EXISTS;
                }
                gOut.mPlacement = K2ASC_NumValue32(args.Arg());
                gOut.mUsePlacement = true;
                break;

            default:
                printf("*** unrecognized switch on command line ('%c')\n", pArg[1]);
                return K2STAT_ERROR_BAD_ARGUMENT;
            }
        }
        else
        {
            printf("*** unexpected argument on command line (%s)\n", args.Arg());
            return K2STAT_ERROR_BAD_ARGUMENT;
        }
        args.Advance();
    }

    if (NULL == gOut.mpOutputFilePath)
    {
        printf("*** no output file path specified on command line\n");
        return K2STAT_ERROR_BAD_ARGUMENT;
    }
    if (NULL == gOut.mpImportLibFilePath)
    {
        printf("*** no import library file path specified on command line\n");
        return K2STAT_ERROR_BAD_ARGUMENT;
    }
    if (NULL == gOut.mpElfFile)
    {
        printf("*** no input elf file path specified on command line\n");
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (gOut.mpElfFile->FileBytes() < K2ELF32_MIN_FILE_SIZE)
    {
        printf("*** input elf file is too small to be valid\n");
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    // get the target name (extensionless and lower case)
    pPath = pScan = gOut.mpElfFile->FileName();
    while (*pScan)
        pScan++;
    do
    {
        pScan--;
        ch = *pScan;
        if ((ch == '/') || (ch == '\\'))
        {
            pScan++;
            break;
        }
    } while (pScan != pPath);
    if (pScan == pPath)
    {
        printf("*** Could not parse target XDL name from input ELF path\n");
        return K2STAT_ERROR_BAD_ARGUMENT;
    }
    pPath = pScan;
    ch = *pScan;
    do {
        if ('.' == ch)
            break;
        pScan++;
        ch = *pScan;
    } while (0 != ch);
    gOut.mTargetNameLen = (UINT_PTR)(pScan - pPath);
    if (0 == gOut.mTargetNameLen)
    {
        printf("*** No filename part in input ELF file paht\n");
        return K2STAT_ERROR_BAD_ARGUMENT;
    }
    if (XDL_NAME_MAX_LEN < gOut.mTargetNameLen)
    {
        printf("*** input ELF filename part is too long for XDL\n");
    }
    pConv = &gOut.mTargetName[0];
    while (pPath != pScan)
    {
        *pConv = K2ASC_ToLower(*pPath);
        pConv++;
        pPath++;
    }
    *pConv = 0;
//    printf("  TARGET NAME = \"%s\" (%d)\n", gOut.mTargetName, gOut.mTargetNameLen);

//    gOut.mPlacement = 0x400000;
//    gOut.mUsePlacement = true;

    pData = (UINT8 const *)gOut.mpElfFile->DataPtr(); 
    if (pData[EI_CLASS] == ELFCLASS32)
    {
        return Convert32();
    }
    if (pData[EI_CLASS] == ELFCLASS64)
    {
        return Convert64();
    }

    printf("*** input file is eitehr not an ELF file or has unknown class (%d)\n", pData[EI_CLASS]);

    return K2STAT_ERROR_BAD_ARGUMENT;
}