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

#include "k2export.h"

OUTCTX gOut;

int
TreeStringCompare(UINT_PTR aKey, K2TREE_NODE *apNode)
{
    return K2ASC_Comp((char const *)aKey, (char const *)apNode->mUserVal);
}

int main(int argc, char **argv)
{
    ArgParser   args;
    K2STAT      stat;
    
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
    K2TREE_Init(&gOut.SymbolTree, TreeStringCompare);

    //
    // parse args and load files
    //
    while (args.Left())
    {
        char const *pArg = args.Arg();
        if ((pArg[0] == '-') ||
            (pArg[0] == '/'))
        {
            args.Advance();
            if ((!args.Left()) || (0 != pArg[2]))
            {
                return K2STAT_ERROR_BAD_ARGUMENT;
            }
            switch (pArg[1])
            {
            case 'i':
            case 'I':
                if (NULL != gOut.mpMappedXdlInf)
                {
                    printf("*** more than one INF specifier on command line\n");
                    return K2STAT_ERROR_BAD_ARGUMENT;
                }
                stat = LoadXdlInfFile(args.Arg());
                if (K2STAT_IS_ERROR(stat))
                    return stat;
                break;

            case 'o':
            case 'O':
                if (NULL != gOut.mpOutputFilePath)
                {
                    printf("*** more than one output exports object file name specified on command line\n");
                    return K2STAT_ERROR_BAD_ARGUMENT;
                }
                gOut.mpOutputFilePath = args.Arg();
                break;

            default:
                printf("*** unrecognized switch on command line ('%c')\n", pArg[1]);
                return K2STAT_ERROR_BAD_ARGUMENT;
            }
        }
        else
        {
            stat = LoadInputFile(args.Arg());
            if (K2STAT_IS_ERROR(stat))
                return stat;
        }
        args.Advance();
    }

    return DoExport();
}