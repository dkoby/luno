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
#ifndef _CLIENT_H
#define _CLIENT_H

#include <lua.h>
/* */
#include "debug.h"
#include "thread.h"
#include "token.h"

struct client_t {
    int lease;

    int sock;
    struct {
#define CLIENT_ADDR_INFO_ADDR_LENGTH    40
        char addr[CLIENT_ADDR_INFO_ADDR_LENGTH + 1];
        int port;
    } addrInfo;
    struct thread_t thread;

    int serverPort;

    int (*getChars)(char *, int, void *);

    struct token_t token;
    lua_State *luaState;
    /*
     * Fields from HTTP request header.
     */
    struct {
        int keepAlive; /* "Connection" field, 0 for "close", 1 for "keep-alive" */
    } request;
};

int clientStart(struct client_t *client);
void clientStop(struct client_t *client);

int clientSendChars(struct client_t *client, const void *buf, size_t len);

#define DEBUG_CLIENT(level, fmt, ...) \
        debugPrint(level, "[Client (%p) %s:%d]: " fmt,    \
                            client, client->addrInfo.addr, client->addrInfo.port, \
                            __VA_ARGS__)

#endif

