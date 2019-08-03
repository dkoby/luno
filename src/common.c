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
#include <string.h>
#include "common.h"
/*
 * Convert string to number.
 *
 * NOTE Only positive decimal numbers supported.
 * TODO
 *     * Negative decimal numbers.
 *     * Hex numbers (with prefix of "0x").
 *
 * ARGS
 *     sdata    Pointer to string to convert.
 *     slen     Length of string. If string is 0-terminated can be 0.
 *     value    Pointer to number where converted value will be placed.
 *
 * RETURN
 *     Zero on success, -1 on error.
 */
int commonString2Number(char *sdata, int slen, int64_t *value)
{
    uint64_t result;
    uint64_t weight;
    int len;
    char *p;

    result = 0;

    if (!sdata || *sdata == 0)
        goto error;

    /* TODO Identify number format by prefix. */

    if (slen)
        len = slen;
    else
        len = strlen(sdata);

    if (len < 1)
        goto error;

    result = 0;
    weight = 1;
    p      = sdata + len - 1;
    while (len--)
    {
        if (*p >= '0' && *p <= '9')
            result += (*p - '0') * weight;
        else
            goto error;
        p--;
        weight *= 10;
    }

    *value = result;
    return 0;
error:
    return -1;
}

