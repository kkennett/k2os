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

#include <lib/k2xml.h>
#include <lib/k2asc.h>
#include <lib/k2mem.h>

typedef struct _K2XML_CONTEXT K2XML_CONTEXT;
struct _K2XML_CONTEXT
{
    K2XML_pf_Allocator      mfAllocator;
    K2XML_pf_Deallocator    mfDeallocator;
    K2XML_PARSE *           mpParse;
    K2STAT                  mStatus;
};

void
K2XML_PurgeNode(
    K2XML_pf_Deallocator    afDealloc,
    K2XML_NODE *            apNode
)
{
    UINT_PTR ix;

    if (0 < apNode->mNumSubNodes)
    {
        for (ix = 0; ix < apNode->mNumSubNodes; ix++)
        {
            K2XML_PurgeNode(afDealloc, apNode->mppSubNode[ix]);
            apNode->mppSubNode[ix] = NULL;
        }
        afDealloc(apNode->mppSubNode);
    }

    if (0 < apNode->mNumAttrib)
    {
        for (ix = 0; ix < apNode->mNumAttrib; ix++)
        {
            afDealloc(apNode->mppAttrib[ix]);
            apNode->mppAttrib[ix] = NULL;
        }
        afDealloc(apNode->mppAttrib);
    }

    afDealloc(apNode);
}

K2XML_NODE *
K2XML_Construct(
    K2XML_CONTEXT * apContext,
    char const **   appWork,
    UINT_PTR *      apLeft
)
{
    char            quoteType;
    BOOL            inQuote;
    char            last;
    BOOL            minus;
    BOOL            square;
    char const*     pSave;
    UINT_PTR        saveBytes;
    UINT_PTR        len;
    char            chk;
    UINT_PTR        nameLen;
    char const *    pCont;
    char const *    pContEnd;
    K2XML_NODE **   ppSubs;
    UINT_PTR        numSubs;
    UINT_PTR        nameTagLen;
    K2XML_NODE *    pSub;
    K2XML_NODE **   ppNewNodeList;
    BOOL            ok;
    K2XML_NODE *    pRet;
    char const *    pName;
    long            parsNameLen;
    char const *    pValue;
    long            parsValLen;
    K2XML_ATTRIB *  pNewAttrib;
    K2XML_ATTRIB ** ppNewAttribList;

    quoteType = 0;

    do {
        /* find the first open tag character */
        while ((**appWork != '<') && (0 != (*apLeft)))
        {
            (*appWork)++;
            (*apLeft)--;
        }
        if (0 == (*apLeft))
            return NULL;

        /* make sure it's not a configuration command or comment block (i.e. "<?" or "<!") */
        (*appWork)++;
        if ((**appWork != '?') && (**appWork != '!'))
        {
            (*appWork)--;
            break;
        }
        (*apLeft)--;

        if (**appWork == '?')
        {
            /* config tag - this has a standard node header.
               pay attention to quoted strings. look for closing "?>" outside quoted strings */
            (*appWork)++;
            (*apLeft)--;
            if (0 == (*apLeft))
                return NULL;

            inQuote = FALSE;
            last = 0;
            do {
                while (((inQuote) || (**appWork != '?')) && (0 != (*apLeft)))
                {
                    if (inQuote)
                    {
                        if (last != '\\')
                        {
                            if (**appWork == quoteType)
                            {
                                inQuote = FALSE;
                            }
                        }
                    }
                    else
                    {
                        if ((**appWork == '\"') || (**appWork == '\''))
                        {
                            inQuote = TRUE;
                            quoteType = **appWork;
                        }
                    }
                    last = **appWork;
                    (*appWork)++;
                    (*apLeft)--;
                }
                if (0 == (*apLeft))
                    return NULL;

                (*appWork)++;
                (*apLeft)--;
                if (0 == (*apLeft))
                    return NULL;

            } while (**appWork != '>');
        }
        else
        {
            /* comment or CDATA tag */
            (*appWork)++;
            (*apLeft)--;
            if (0 == (*apLeft))
                return NULL;

            if (**appWork == '-')
            {
                (*appWork)++;
                (*apLeft)--;
                if (0 == (*apLeft))
                    return NULL;
                if (**appWork != '-')
                    return NULL; /* don't know what you're trying to do, but it's wrong */
                (*appWork)++;
                (*apLeft)--;
                if (0 == (*apLeft))
                    return NULL;
                /* now keep scanning until you get "-->" */
                do {
                    while ((**appWork != '-') && (0 != (*apLeft)))
                    {
                        (*appWork)++;
                        (*apLeft)--;
                    }
                    if (0 == (*apLeft))
                        return NULL;
                    (*appWork)++;
                    (*apLeft)--;
                    if (0 == (*apLeft))
                        return NULL;
                    minus = (**appWork == '-');
                    (*appWork)++;
                    (*apLeft)--;
                    if (0 == (*apLeft))
                        return NULL;
                    if (minus)
                    {
                        if ((**appWork) == '>')
                            break;
                    }
                } while (TRUE);
            }
            else if (**appWork == '[')
            {
                (*appWork)++;
                (*apLeft)--;
                if ((*apLeft) < 9)
                    return NULL;
                if (0 != K2ASC_CompInsLen(*appWork, "CDATA[", 6))
                    return NULL;
                (*appWork) += 6;
                (*apLeft) -= 6;
                /* now keep scanning until you get "]]>" */
                do {
                    while ((**appWork != ']') && (0 != (*apLeft)))
                    {
                        (*appWork)++;
                        (*apLeft)--;
                    }
                    if (0 == (*apLeft))
                        return NULL;
                    (*appWork)++;
                    (*apLeft)--;
                    if (0 == (*apLeft))
                        return NULL;
                    square = (**appWork == ']');
                    (*appWork)++;
                    (*apLeft)--;
                    if (0 == (*apLeft))
                        return NULL;
                    if (square)
                    {
                        if ((**appWork) == '>')
                            break;
                    }
                } while (TRUE);
            }
            else
            {
                /* some other kind of DTD or info block - simply try to scan to end of it */
                while ((**appWork != '>') && (0 != (*apLeft)))
                {
                    (*appWork)++;
                    (*apLeft)--;
                }
                if (0 == (*apLeft))
                    return NULL;
            }
        }

        /* appWork should be on trailing > of previous tag here */
        (*appWork)++;
        (*apLeft)--;

    } while (TRUE);

    pSave = *appWork;
    saveBytes = *apLeft;
    len = 0;

    /* scan to end of name of tag */
    do {
        (*appWork)++;
        (*apLeft)--;
        if (0 == (*apLeft))
            return NULL;
        len++;
        chk = **appWork;
    } while ((chk != ' ') && (chk != '\t') && (chk != '=') && (chk != '>') && (chk != '/'));
    if (len == 1)
    {
        if (chk == '/')
        {
            /* whoa, back up.  this is a closing tag */
            (*appWork)--;
            (*apLeft)++;
        }
        return NULL;    /* only open brace, or this is a closing tag */
    }

    nameLen = len - 1; /* -1 for the open brace */

    /* find closing brace for open tag - be aware of quoted strings */
    inQuote = FALSE;
    quoteType = 0;
    last = 0;
    do {
        while (((inQuote) || (chk != '>')) && (0 != (*apLeft)))
        {
            if (inQuote)
            {
                if (last != '\\')
                {
                    if (chk == quoteType)
                    {
                        inQuote = FALSE;
                    }
                }
            }
            else
            {
                if ((chk == '\"') || (chk == '\''))
                {
                    inQuote = TRUE;
                    quoteType = chk;
                }
            }
            last = chk;
            (*appWork)++;
            (*apLeft)--;
            if (0 == (*apLeft))
                return NULL;
            len++;
            chk = **appWork;
        }
        if (0 == (*apLeft))
            return NULL;
    } while (chk != '>');

    pCont = NULL;
    pContEnd = NULL;
    ppSubs = NULL;
    numSubs = 0;
    nameTagLen = (unsigned int)((*appWork) - (pSave + 1));
    if ((*((*appWork) - 1)) != '/')
    {
        /* paired xml tag (possibly has content).  do sub nodes (if there are any) */
        (*appWork)++;
        (*apLeft)--;
        if (0 == (*apLeft))
            return NULL;
        pCont = *appWork;
        do {
            pSub = K2XML_Construct(apContext, appWork, apLeft);
            if (pSub)
            {
                ppNewNodeList = apContext->mfAllocator(sizeof(K2XML_NODE *) * (numSubs + 1));
                K2_ASSERT(NULL != ppNewNodeList);
                if (numSubs)
                {
                    K2MEM_Copy(ppNewNodeList, ppSubs, sizeof(K2XML_NODE *) * numSubs);
                    apContext->mfDeallocator(ppSubs);
                }
                ppSubs = ppNewNodeList;
                ppSubs[numSubs++] = pSub;
            }
        } while (pSub);
        pContEnd = *appWork;
        /* should be at closing tag now (one that matches the open at pSave).  verify this */

        ok = ((*apLeft) >= 3 + nameLen);
        if (ok)
        {
            (*appWork) += 2;
            (*apLeft) -= 2;
            ok = (0 == K2ASC_CompInsLen((char *)pSave + 1, (char *)*appWork, nameLen));
            if (ok)
            {
                (*appWork) += nameLen;
                (*apLeft) -= nameLen;
                ok = ((**appWork) == '>');
                /* if not ok, then closing tag had extra goo after tag name */
            }
            /* else closing tag does not match open tag (case sensitive).  barf out. */
        }
        /* else no closing tag found. barf out */
        if (!ok)
        {
            /* closing tag does not match open tag (case sensitive).  barf out. */
            for (nameTagLen = 0; nameTagLen < numSubs; nameTagLen++)
            {
                K2XML_PurgeNode(apContext->mfDeallocator, ppSubs[nameTagLen]);
            }
            apContext->mfDeallocator(ppSubs);
            return NULL;
        }
    }
    else
        nameTagLen--;
    /* skip over closing brace at end of tag (both short and long cases) */
    (*appWork)++;
    (*apLeft)--;

    pRet = (K2XML_NODE *)apContext->mfAllocator(sizeof(K2XML_NODE));
    K2_ASSERT(NULL != pRet);
    pRet->mppSubNode = ppSubs;
    pRet->mNumSubNodes = numSubs;
    pRet->mNumAttrib = 0;
    pRet->mppAttrib = NULL;
    pRet->Name.mpChars = (char const *)pSave + 1;
    pRet->Name.mLen = nameLen;
    pRet->mNameTagLen = nameTagLen;
    if (NULL != pCont)
    {
        pRet->Content.mpChars = pCont;
        pRet->Content.mLen = (0 != numSubs) ?
            (unsigned int)(pRet->mppSubNode[0]->RawXml.mpChars - pCont) :
            (unsigned int)(pContEnd - pCont);
    }
    pRet->RawXml.mpChars = pSave;
    pRet->RawXml.mLen = (saveBytes - *apLeft);

    /* now parse out any attributes (if there were any) from pSave for len */
    pSave += nameLen + 1;
    len -= nameLen + 1;

    do {
        /* eat any leading whitespace */
        while ((0 != len) && ((*pSave == ' ') || (*pSave == '\t') || (*pSave == '\n')))
        {
            pSave++;
            len--;
        }
        if (len)
        {
            if ((*pSave == '/') || (*pSave == '>'))
                break;
            pName = pSave;
            parsNameLen = 0;
            while ((0 != len) &&
                (*pSave != '/') &&
                (*pSave != ' ') &&
                (*pSave != '\t') &&
                (*pSave != '\n') &&
                (*pSave != '='))
            {
                pSave++;
                parsNameLen++;
                len--;
            }
            if (*pSave == '=')
            {
                /* ok, we got an attribute an and =, so what's the value */
                pSave++;
                len--;
                /* eat any leading whitespace */
                while ((len) && ((*pSave == ' ') || (*pSave == '\t') || (*pSave == '\n')))
                {
                    pSave++;
                    len--;
                }
                if (len)
                {
                    if ((*pSave == '\"') || (*pSave == '\''))
                    {
                        quoteType = *pSave;
                        pSave++;
                        len--;
                    }
                }
                pValue = pSave;
                parsValLen = 0;

                /* we're in the quoted value - watch for backslash quotes and ignore them */
                last = 0;
                while (0 != len)
                {
                    if (*pSave == quoteType)
                    {
                        if (last != '\\')
                            break;
                    }
                    last = *pSave;
                    pSave++;
                    parsValLen++;
                    len--;
                }
                if (len)
                {
                    if (*pSave == quoteType)
                    {
                        pSave++;
                        len--;
                    }
                }

                pNewAttrib = (K2XML_ATTRIB *)apContext->mfAllocator(sizeof(K2XML_ATTRIB));
                K2_ASSERT(NULL != pNewAttrib);

                pNewAttrib->Name.mpChars = (char const *)pName;
                pNewAttrib->Name.mLen = parsNameLen;

                pNewAttrib->Value.mpChars = pValue;
                pNewAttrib->Value.mLen = parsValLen;

                ppNewAttribList = (K2XML_ATTRIB **)apContext->mfAllocator(sizeof(K2XML_ATTRIB *) * (pRet->mNumAttrib + 1));
                K2_ASSERT(NULL != ppNewAttribList);
                if (0 != pRet->mNumAttrib)
                {
                    K2MEM_Copy(ppNewAttribList, pRet->mppAttrib, sizeof(K2XML_NODE *) * pRet->mNumAttrib);
                    apContext->mfDeallocator(pRet->mppAttrib);
                }
                pRet->mppAttrib = ppNewAttribList;
                ppNewAttribList[pRet->mNumAttrib] = pNewAttrib;
                pRet->mNumAttrib++;
            }
        }
    } while (0 != len);

    return pRet;
}

K2STAT
K2XML_Parse(
    K2XML_pf_Allocator      aAllocator,
    K2XML_pf_Deallocator    aDeallocator,
    char const *            apXml,
    UINT_PTR                aXmlLen,
    K2XML_PARSE *           apRetParse
)
{
    K2XML_CONTEXT context;

    if ((NULL == aAllocator) ||
        (NULL == aDeallocator) ||
        (NULL == apXml) ||
        (NULL == apRetParse))
        return K2STAT_ERROR_BAD_ARGUMENT;

    K2MEM_Zero(apRetParse, sizeof(K2XML_PARSE));

    apRetParse->mfDeallocator = aDeallocator;

    if ((0 == (*apXml)) || (0 == aXmlLen))
    {
        return K2STAT_NO_ERROR;
    }

    context.mfAllocator = aAllocator;
    context.mfDeallocator = aDeallocator;
    context.mpParse = apRetParse;
    context.mStatus = K2STAT_NO_ERROR;

    apRetParse->mpRootNode = K2XML_Construct(&context, &apXml, &aXmlLen);
    if (NULL == apRetParse->mpRootNode)
    {
        if (!K2STAT_IS_ERROR(context.mStatus))
        {
            context.mStatus = K2STAT_ERROR_BAD_FORMAT;
        }
    }

    return context.mStatus;
}

K2STAT
K2XML_Purge(
    K2XML_PARSE *   apParse
)
{
    K2XML_NODE *    pRoot;

    if (NULL == apParse)
        return K2STAT_ERROR_BAD_ARGUMENT;

    pRoot = apParse->mpRootNode;
    if (NULL != pRoot)
    {
        K2XML_PurgeNode(apParse->mfDeallocator, pRoot);
    }

    K2MEM_Zero(apParse, sizeof(K2XML_PARSE));

    return K2STAT_NO_ERROR;
}

BOOL
K2XML_FindAttrib(
    K2XML_NODE const *  apNode,
    char const *        apName,
    UINT_PTR *          apRetIx
)
{
    UINT_PTR        ix;
    UINT_PTR        len;
    K2XML_ATTRIB *  pAttrib;

    K2_ASSERT(NULL != apNode);
    K2_ASSERT(NULL != apName);
    K2_ASSERT(NULL != apRetIx);

    *apRetIx = (UINT_PTR)-1;

    if (0 == apNode->mNumAttrib)
        return FALSE;

    len = K2ASC_Len(apName);
    if (0 == len)
        return FALSE;

    for (ix = 0; ix < apNode->mNumAttrib; ix++)
    {
        pAttrib = apNode->mppAttrib[ix];
        if (pAttrib->Name.mLen == len)
        {
            if (0 == K2ASC_CompInsLen(apName, pAttrib->Name.mpChars, len))
            {
                *apRetIx = ix;
                return TRUE;
            }
        }
    }

    return FALSE;
}
