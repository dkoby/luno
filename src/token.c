/*
 * Luno - the web server.
 *
 * Copyright (c) 2016-2019, Dmitry Kobylin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* */
#include "token.h"

#if 0
    #define DEBUG_THIS
#endif
/*
 * Initialize token.
 *
 * ARGS
 *     token         Pointer to preallocated token structure.
 *     getChar       Function for read character from input stream.
 *     getCharArg    Argument for character read function.
 *
 * RETURN
 *     Zero on success, -1 on error.
 *
 */
int tokenInit(struct token_t *token,
        int (*getChars)(char *, int, void *), void *getCharsArg)
{
    token->getChars    = getChars;
    token->getCharsArg = getCharsArg;

    tokenReset(token);

#if 0
    token->bufSize = TOKEN_BUF_SIZE;
    token->buf     = malloc(token->bufSize);
    if (!token->buf)
        return -1;
#endif

    return 0;
}
/*
 * Destroy token.
 *
 * ARGS
 *     token    Pointer to preallocated token structure.
 */
void tokenDestroy(struct token_t *token)
{
#if 0
    if (token->buf)
        free(token->buf);
#endif
}
/*
 *
 */
void tokenReset(struct token_t *token)
{
    token->nTokens       = 0;
    token->nBufChars     = 0;
    token->nDroppedChars = 0;
    token->nRead         = 0;
}

/*
 * Get token from input stream.
 *
 * ARGS
 *     token          Pointer to token structure.
 *     tlength        Pointer to length of returned token payload, can be NULL.
 *     eof            Pointer where EOF condition will be placed, can be NULL.
 *     analyze        Function for analize input characters.
 *
 *                    ARGS
 *                        tinfo    Pointer to token info structure.
 *                        ch       Input character.
 *                        type     Type of token to get.
 *                    RETURN
 *                    If token is complete or does not appear in input stream
 *                    value of 1 is returned, zero otherwise.
 *     postAnalyze    Function for post analize input characters, can be NULL.
 *                    This function does not called if token payload length
 *                    is zero.
 *
 *                    ARGS
 *                        tinfo    Pointer to token info structure.
 *                        type     Type of token to analyze.
 *                    RETURN
 *                        If token is valid return value of 1, 0 otherwise.
 *
 *     type       Type of token to get.
 *
 * RETURN
 *     Pointer to token payload, length of payload is stored in tlength argument. Zero if token
 *     does not appear in input stream.
 */
char * tokenGet(struct token_t *token, int *tlength, int *eof,
        int (*analyze)(struct token_info_t *, char*, int),
        int (*postAnalyze)(struct token_info_t *, int),
        int type)
{
    struct token_info_t *tinfo; /* Current token info structure to fill. */
    int nTokenChars;                 /* Number of characters in remembered tokens. */
    char *rbuf;                 /* Current read pointer of trace buffer. */
    char *ch;
    int i, n, m;

    if (tlength)
        *tlength = 0;
    if (eof)
        *eof = 0;
    if (token->nTokens >= TOKENS_MAX)
    {
        /* NOTREACHED */
        printf("(E) TOKEN GET, TOKENS_MAX\r\n");
        /*
         * TODO
         * Dynamicaly expand number of remembered tokens.
         */
        return NULL;
    }
    
    rbuf = token->buf + token->nDroppedChars;
    /*
     * Get count of characters in remembered tokens. 
     * And adjust read buffer.
     */
    nTokenChars = 0;
    for (i = 0; i < token->nTokens; i++)
        nTokenChars += token->tinfo[i].lenw;
    rbuf += nTokenChars;
    /* 
     * Initialize token info structure for analyze function.
     */
    tinfo = &token->tinfo[token->nTokens];
    tinfo->lenw = 0;
    tinfo->lenp = 0;
    tinfo->p    = rbuf;
    /*
     * Get characters and feed them to analyze function.
     */
    while (1)
    {
        if (rbuf >= (token->buf + token->nBufChars))
        {
            n = TOKEN_BUF_SIZE - token->nBufChars; /* Number of characters to fit in trace buffer. */ 
            if (n == 0)
            {
                if (token->nDroppedChars == 0)
                {
                    printf("(E) TOKEN GET, trace buffer too short\r\n");
                    /*
                     * TODO
                     * Dynamicaly expand trace buffer.
                     * NOTE
                     * Buffer reallocated to new area. Fix current read pointer and
                     * pointers inside remembered tokens.
                     */
                    return NULL;
                } else {
                    /* 
                     * Move trace buffer down by "nDroppedChars" characters.
                     */
#if (1 && (defined DEBUG_THIS))
                    printf("Move memory, BUF_SIZE %d, nBufChars %d, nDroppedChars %d\r\n",
                            TOKEN_BUF_SIZE,
                            token->nBufChars,
                            token->nDroppedChars);
#endif
                    memmove(token->buf, token->buf + token->nDroppedChars,
                            token->nBufChars - token->nDroppedChars);
                    /*
                     * Fix pointers in remembered tokens.
                     */
                    for (i = 0; i < token->nTokens; i++)
                        token->tinfo[i].p -= token->nDroppedChars;
                    /*
                     * Fix current pointer.
                     */
                    tinfo->p -= token->nDroppedChars;
                    /*
                     * Fix other values.
                     */
                    n    += token->nDroppedChars;
                    rbuf -= token->nDroppedChars;
                    token->nBufChars -= token->nDroppedChars;
                    token->nDroppedChars = 0;
                }
            }
            if (n < 0)
            {
                printf("(E) TOKEN GET, number of characters exceed trace buffer size\r\n");
                /* NOTREACHED */
                return NULL;
            }
            /*
             * getChars function must return:
             *     Number of readed bytes on success, zero on EOF, value less then
             *     zero on error.
             */
            m = (*token->getChars)(rbuf, n, token->getCharsArg);
            if (m > n)
            {
                /* NOTREACHED */
                printf("(E) TOKEN GET, number of readed characters exceed requested characters\r\n");
                return NULL;
            }
            if (m < 0)
            {
                /* NOTREACHED */
                printf("(E) TOKEN GET, getChars error\r\n");
                return NULL;
            }
            if (m == 0)
                rbuf = NULL; /* NOTE */
            else
                token->nBufChars += m;
        }
        if (rbuf)
            ch = rbuf++;
        else
            ch = NULL;

        if (ch == NULL && eof)
            *eof = 1;
#if (0 && (defined DEBUG_THIS))
        if (ch)
        {
            if (*ch == '\r')
                printf("CH: CR");
            else if (*ch == '\n')
                printf("CH: LF");
            else if (*ch == '\t')
                printf("CH: TB");
            else if (*ch == '\t')
                printf("CH: SP");
            else if (*ch < 30)
                printf("CH: 0x%02hhX ", *ch);
            else
                printf("CH: %c ", *ch);
        } else {
            printf("CH: EOF");
        }

        {
            int res = (*analyze)(tinfo, ch, type);
            printf(" type %d, res %d, eof %d, lenw %d, lenp %d\r\n", type, res, eof ? *eof : -1, tinfo->lenw, tinfo->lenp);
            if (res)
                break;
        }
#else
        if ((*analyze)(tinfo, ch, type))
            break;
#endif
    }


    /* Check number of characters in just readed token. */
    if (tinfo->lenp > 0)
    {
        if (postAnalyze ? (*postAnalyze)(tinfo, type) : 1)
        {
            token->nTokens++;
            if (tlength)
                *tlength = tinfo->lenp;
#if (1 && (defined DEBUG_THIS))
            {
                char *p;
//                printf("Token complete \"%.*s\"\r\n", tinfo->lenp, tinfo->p);
                printf("Token: \"");
                p = tinfo->p;
                while (p < (tinfo->p + tinfo->lenp))
                {
                    if (*p < 0x21 || *p > 0x7E)
                        printf("%%%02hhX", *p);
                    else
                        printf("%c", *p);
                    p++;
                }
                printf("\"\r\n");
            }
#endif
            return tinfo->p;
        }
    }

    return NULL;
}
/*
 *
 * RETURN
 *     0 if no traced data remains or number of bytes
 *     placed in input buffer.
 *
 */
int tokenGetRemainedData(struct token_t *token, char *buf, int len)
{
    int occupied, remain;
    int nTokenChars;
    int i;

    nTokenChars = 0;
    for (i = 0; i < token->nTokens; i++)
        nTokenChars += token->tinfo[i].lenw;

    occupied = token->nDroppedChars + nTokenChars + token->nRead;
    remain   = token->nBufChars - occupied;
    if (remain <= 0)
        return 0;

    if (len > remain)
        len = remain;
    memcpy(buf, &token->buf[occupied], len);
    token->nRead += len;
    return len;
}
#if 0
/*
 * Unget last remembered tokens.
 *
 * ARGS
 *     token      Pointer to token structure.
 *     n          Number of tokens to forget. -1 to forget all tokens.
 */
void tokenUnget(struct token_t *token, int n)
{
    if (n < 0)
    {
        token->nTokens = 0;
        return;
    }
    /*
     * TODO
     * Output error if n exceed number of remembered tokens.
     */
    if (token->nTokens < n)
        token->nTokens = 0;
    else
        token->nTokens -= n;
}
#endif
/*
 * Drop all remembered tokens.
 */
void tokenDrop(struct token_t *token)
{
    int nTokenChars;
//    char *rbuf;
    int i;
    /*
     * Get count of characters in remembered tokens. 
     */
    nTokenChars = 0;
    for (i = 0; i < token->nTokens; i++)
        nTokenChars += token->tinfo[i].lenw;

    token->nDroppedChars += nTokenChars;
    token->nTokens        = 0;
#if (0 && (defined DEBUG_THIS))
    printf("Dropped chars = %d\r\n", token->nDroppedChars);
#endif
}

