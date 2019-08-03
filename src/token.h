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
#ifndef _TOKEN_H
#define _TOKEN_H

struct token_t {
    int (*getChars)(char *, int, void *);
    void *getCharsArg;

#define TOKEN_BUF_SIZE    1024
#if 0
    char *buf;       /* Trace buffer. */
    int bufSize;     /* Size of trace buffer. */
#else
    char buf[TOKEN_BUF_SIZE]; /* Trace buffer. */
#endif
    int nBufChars;     /* Number of characters in trace buffer */
    int nDroppedChars; /* Number of dropped characters in trace buffer */
    int nRead;

#define TOKENS_MAX   2
    struct token_info_t {
        int lenw; /* Length of whole token in trace buffer. */
        int lenp; /* Length of payload of token in trace buffer. */
        char *p;  /* Pointer to payload of token in trace buffer. */
    } tinfo[TOKENS_MAX];
    int nTokens; /* Number of remembered tokens */
};

int tokenInit(struct token_t *token,
        int (*getChars)(char *, int, void *), void *getCharsArg);
void tokenReset(struct token_t *token);
char * tokenGet(struct token_t *token, int *tlength, int *eof,
        int (*analyze)(struct token_info_t *, char*, int),
        int (*postAnalyze)(struct token_info_t *, int),
        int type);
int tokenGetRemainedData(struct token_t *token, char *buf, int len);
#if 0
void tokenUnget(struct token_t *token, int n);
#endif
void tokenDrop(struct token_t *token);
void tokenDestroy(struct token_t *token);

#endif

