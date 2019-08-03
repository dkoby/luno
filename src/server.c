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

#ifdef WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
/* */
#include "server.h"
/* */
#include "client.h"
#include "debug.h"
#include "lserver.h"
#include "mromfs.h"
#include "mromfsimage.h"

#define SERVER_LISTEN_QUEUE_LENGTH    100
#define SERVER_MAX_CLIENTS            1024

static int nclients = 0;

struct server_t server;
static struct client_t clients[SERVER_MAX_CLIENTS];

static int _openServerSock(int port);
static void _acceptClient();
static int _initSignals();
#ifdef WINDOWS
static void _sigAction(int sig);
#else
static void _sigAction(int sig, siginfo_t *siginfo, void *context);
#endif
static struct client_t *_getClient();
static void _stopClients();

/*
 *
 */
void serverInit()
{
    server.portNumber  = DEFAULT_PORT;
    server.resourceDir = NULL;
    server.sock        = -1;
    server.luaState    = NULL;
    server.run         = 1;
    server.caching     = 1; /* NOTE Not implemented */
    threadMutexFill(&server.lmutex);
    threadMutexFill(&server.cmutex);
    memset(clients, 0, sizeof(struct client_t) * SERVER_MAX_CLIENTS);

}
/*
 *
 */
int serverRun()
{
#ifdef WINDOWS
    WSADATA wsaData;
#endif
    int ret;

    ret = -1;

#ifdef WINDOWS
    if (WSAStartup(0x0101, &wsaData) != 0)
    {
        debugPrint(DLEVEL_ERROR, "WSAStartup error");
        goto done;
    }
#endif
    if (threadMutexInit(&server.cmutex) < 0)
    {
        debugPrint(DLEVEL_ERROR, "%s", "Mutex init failed");
        goto done;
    }

    if (_initSignals() < 0)
        goto done;
    if (mromfsInit(&server.mromfs, mromfsimageData, sizeof(mromfsimageData)) < 0)
    {
        debugPrint(DLEVEL_ERROR, "%s", "Mromfs init failed");
        goto done;
    }

    server.sock = _openServerSock(server.portNumber);
    if (server.sock < 0)
        goto done;
    if (lserverInit() < 0)
        goto done;

    while (server.run)
    {
        fd_set rfds;
        int sel;
        struct timeval timeout;

        FD_ZERO(&rfds);
        FD_SET(server.sock, &rfds);

        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        sel = select(server.sock + 1, &rfds, NULL, NULL, &timeout);
        if (!server.run)
            break;
        if (sel < 0)
        {
            if (errno == EINTR)
            {
                /* NOTREACHED _sigAction must terminate program */
#if 1
                debugPrint(DLEVEL_ERROR, "Select EINTR %s", strerror(errno));
#endif
                continue;
            }

            debugPrint(DLEVEL_ERROR, "Select failed %s", strerror(errno));
            goto done;
        }
        if (sel == 0)
        {
            /* Timeout. */
            continue;
        }
        if (FD_ISSET(server.sock, &rfds))
            _acceptClient();
    }

    _stopClients();
#ifdef WINDOWS
    WSACleanup();
#endif
    ret = 0;
done:
    if (server.sock >= 0)
        close(server.sock);
    lserverDestroy();

    return ret;
}

/*
 * RETURN
 *     socket on success, -1 on error.
 */
static int _openServerSock(int port)
{
    int sock;
    struct sockaddr_in serverAddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        debugPrint(DLEVEL_ERROR, "Failed to open server socket, %s", strerror(errno));
        goto error;
    }

#if 1
    {
        int enable = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
#ifdef WINDOWS
            (const char*)&enable,
#else
            &enable,
#endif
            sizeof(int)) < 0
        ) {
            debugPrint(DLEVEL_ERROR, "setsockopt(SO_REUSEADDR) failed");
        }
    }
#endif

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((unsigned short)port);

    if (bind(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        debugPrint(DLEVEL_ERROR, "Failed to bind server to port %d, %s", port, strerror(errno));
        goto error;
    }

    debugPrint(DLEVEL_INFO, "Binding to port %d success", port);

    if (listen(sock, SERVER_LISTEN_QUEUE_LENGTH) < 0)
    {
        debugPrint(DLEVEL_ERROR, "Failed to listen port %d, %s", port, strerror(errno));
        goto error;
    }

    return sock;
error:
    if (sock >= 0)
        close(sock);
    return -1;
}
/*
 *
 */
static void _acceptClient()
{
    socklen_t addrLen;
    socklen_t clientAddrLen;
    struct sockaddr_in addr;
    struct sockaddr_in clientAddr;
    struct client_t *client;
    int clientSock;
    int success;
    char *clientHostAddr;

    debugPrint(DLEVEL_NOISE, "Incoming connection...");

    client = NULL;
    clientSock = -1;

    success = 0;
    do
    {
        addrLen = sizeof(struct sockaddr_in);
        clientSock = accept(server.sock, (struct sockaddr*)&addr, &addrLen);
        if (clientSock < 0)
        {
            debugPrint(DLEVEL_WARNING, "Accept failed %s", strerror(errno));
            break;
        }
        clientHostAddr = inet_ntoa(addr.sin_addr);
        if (!clientHostAddr)
        {
            debugPrint(DLEVEL_WARNING, "ERROR on inet_ntoa");
            break;
        }
        clientAddrLen = sizeof(clientAddr);
        if (getpeername(clientSock, (struct sockaddr*)&clientAddr, &clientAddrLen) < 0)
        {
            debugPrint(DLEVEL_WARNING, "ERROR on getpeername: %s", strerror(errno));
            break;
        }
        debugPrint(DLEVEL_INFO, "Accepted connection from %s:%d", clientHostAddr, ntohs(clientAddr.sin_port));
        client = _getClient();
        if (!client)
        {
            debugPrint(DLEVEL_WARNING, "Client limit reached");
            break;
        }
        strncpy(client->addrInfo.addr, clientHostAddr, CLIENT_ADDR_INFO_ADDR_LENGTH);
        client->addrInfo.addr[CLIENT_ADDR_INFO_ADDR_LENGTH] = 0;
        client->addrInfo.port = ntohs(clientAddr.sin_port);
        client->serverPort    = server.portNumber;
        client->sock          = clientSock;

        if (clientStart(client) != 0)
        {
            debugPrint(DLEVEL_WARNING, "Failed to start client");
            break;
        }
        success = 1;
    } while (0);

    if (!success)
    {
        if (clientSock >= 0)
            close(clientSock);
        if (client)
            serverDropClient(client);
    }
}
/*
 *
 */
static struct client_t *droppedClient = NULL;

static struct client_t *_getClient()
{
    static struct client_t *lastClient = NULL;
    int n;

    if (lastClient == NULL)
        lastClient = clients;

    threadMutexLock(&server.cmutex);
    if (droppedClient)
    {
        struct client_t *client;

        client = droppedClient;
        client->lease = 1;
        droppedClient = NULL;
        nclients++;
        debugPrint(DLEVEL_NOISE, "NCLIENTS(%d)", nclients);
        threadMutexUnlock(&server.cmutex);
        return client;
    }
    threadMutexUnlock(&server.cmutex);

    n = SERVER_MAX_CLIENTS;
    while (n--)
    {
        threadMutexLock(&server.cmutex);
        if (!lastClient->lease)
        {
            lastClient->lease = 1;
            nclients++;
            debugPrint(DLEVEL_NOISE, "NCLIENTS(%d)", nclients);
            threadMutexUnlock(&server.cmutex);
            return lastClient;
        }
        threadMutexUnlock(&server.cmutex);

        lastClient++;
        if (lastClient >= &clients[SERVER_MAX_CLIENTS])
            lastClient = clients;
    }

    return NULL;
}
/*
 * Called by client itself.
 */
void serverDropClient(struct client_t *client)
{
    threadMutexLock(&server.cmutex);
    client->lease = 0;
    droppedClient = client;
    nclients--;
    debugPrint(DLEVEL_NOISE, "NCLIENTS(%d)", nclients);
    threadMutexUnlock(&server.cmutex);
}
/*
 *
 */
static void _stopClients()
{
    int n;
    struct client_t *client;
    debugPrint(DLEVEL_INFO, "Stopping clients");

    n = SERVER_MAX_CLIENTS;
    client = clients;
    while (n--)
    {
        int lease;

        lease = 0;
        threadMutexLock(&server.cmutex);
        if (client->lease)
            lease = 1;
        threadMutexUnlock(&server.cmutex);

        if (lease)
            clientStop(client);
        client++;
    }
    debugPrint(DLEVEL_INFO, "All clients was stopped");
}
/*
 *
 */
static int _initSignals()
{
#ifdef WINDOWS
    signal(SIGINT, _sigAction);
    signal(SIGTERM, _sigAction);
#else
    {
        static struct sigaction act;

        memset(&act, 0, sizeof(act));
        act.sa_sigaction = _sigAction;
        act.sa_flags = SA_SIGINFO | SA_RESETHAND;
        if (sigaction(SIGINT, &act, NULL) == -1)
        {
            debugPrint(DLEVEL_ERROR, "Sigaction failed, %s.", strerror(errno));
            return -1;
        }
        if (sigaction(SIGTERM, &act, NULL) == -1)
        {
            debugPrint(DLEVEL_ERROR, "Sigaction failed, %s.", strerror(errno));
            return -1;
        }
    }
#endif

    /*
     * XXX Ignore SIGPIPE.
     */
#ifndef WINDOWS
    signal(SIGPIPE, SIG_IGN);    
#endif
    return 0;
}
/*
 * Signal handler.
 */
#ifdef WINDOWS
static void _sigAction(int sig)
#else
static void _sigAction(int sig, siginfo_t *siginfo, void *context)
#endif
{
    switch (sig)
    {
        case SIGINT:
        case SIGTERM:
            debugPrint(DLEVEL_INFO,
                    "Caught signal (signal %d)", sig);
            server.run = 0;
            break;
        default:
            debugPrint(DLEVEL_WARNING,
                    "Unknown signal received (%d)", sig);
            break;
    }
}

