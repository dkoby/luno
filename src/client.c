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

#include <unistd.h>
/* */
#include "client.h"
/* */
#include "debug.h"
#include "lclient.h"
#include "server.h"
#include "http.h"
#include "thread.h"
#include "token.h"

static THREAD_RUN(_run, arg);
static THREAD_STOP(_stop, arg);
static void client_Cleanup(struct client_t *client);
static int client_Service(struct client_t *client);
static int client_GetChars(char *buf, int len, void *arg);

/*
 *
 */
int clientStart(struct client_t *client)
{
    client->getChars = client_GetChars;
    if (tokenInit(&client->token, client_GetChars, client) != 0)
    {
        DEBUG_CLIENT(DLEVEL_ERROR, "%s", "Token init failed");
        return -1;
    }
    if (lclientInit0(client) < 0)
    {
        client_Cleanup(client);
        return -1;
    }
    threadInit(&client->thread);
    if (threadInitSock(&client->thread, client->sock) < 0)
    {
        client_Cleanup(client);
        return -1;
    }
    if (threadCreate(&client->thread, _run, _stop, client) < 0)
    {
        client_Cleanup(client);
        return -1;
    }

    return 0;
}
/*
 * Called by server.
 */
void clientStop(struct client_t *client)
{
    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "Stopping client");
    threadCancel(&client->thread);
}
/*
 *
 */
static THREAD_RUN(_run, arg)
{
    struct client_t *client;
    client = arg;

    DEBUG_CLIENT(DLEVEL_INFO, "%s", "Client started");
    while (client_Service(client))
        ;
    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "Client done");
}
/*
 *
 */
static THREAD_STOP(_stop, arg)
{
    struct client_t *client;
    client = arg;

    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "Cleanup client");
    client_Cleanup(client);
    serverDropClient(client);
    DEBUG_CLIENT(DLEVEL_INFO, "%s", "Client stopped");
}
/*
 *
 */
static int client_Service(struct client_t *client)
{
    int error;

    tokenReset(&client->token);
    /* */
    if (lclientInit1(client) < 0)
        return 0;
    /* */
    client->request.keepAlive = 0;
    /* */
    error = httpProcessRequest(client);
    if (error < 0)
        return 0;
    /* */
    if (lclientProcessRequest(client, error))
    {
        if (client->request.keepAlive)
            return 1;
        else
            return 0;
    }
    return 0;
}
/*
 *
 */
static void client_Cleanup(struct client_t *client)
{
    lclientDestroy(client);
    tokenDestroy(&client->token);
    close(client->sock);
}
/*
 * Read data from socket.
 */
static int client_GetChars(char *buf, int len, void *arg)
{
    struct client_t *client;

    client = (struct client_t *)arg;
    return threadRecv(&client->thread, client->sock, buf, len, 0);
}
/*
 *
 */
int clientSendChars(struct client_t *client, const void *buf, size_t len)
{
    return threadSend(&client->thread, client->sock, buf, len, 0);
}

