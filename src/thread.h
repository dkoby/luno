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
#ifndef _THREAD_H
#define _THREAD_H

#ifdef WINDOWS
    #include <windows.h>
#else
    #include <pthread.h>
#endif
#include <sys/types.h>

struct threadMutex_t {
#ifdef WINDOWS
    HANDLE mutex;
#else
    pthread_mutex_t mutex;
    pthread_mutex_t *pmutex;
#endif
};

typedef void(*ThreadRun)(void *);
#define THREAD_RUN(name, argName) void name(void *argName)
#ifdef WINDOWS
typedef void (*ThreadStop)(LPVOID);
#define THREAD_STOP(name, argName) void name(LPVOID argName)
#define THREAD_RETURN(value) return (LPVOID)(value)
#else
typedef void (*ThreadStop)(void *);
#define THREAD_STOP(name, argName) void name(void *argName)
#define THREAD_RETURN(value) return (void *)(value)
#endif

struct thread_t {
#ifdef WINDOWS
    HANDLE thread;
    HANDLE cancelEvent;
    HANDLE stopEvent;
    LPVOID arg;

    WSAEVENT wsaSockReadEvent;
    WSAEVENT wsaSockWriteEvent;
#else
    pthread_t thread;
    void *arg;
#endif
    ThreadStop stop;
    ThreadRun run;
};

void threadInit(struct thread_t *thread);
int threadCreate(struct thread_t *thread, ThreadRun run, ThreadStop stop, void *arg);
void threadCancel(struct thread_t *thread);
void threadSleepMs(struct thread_t *thread, int ms);
int threadInitSock(struct thread_t *thread, int sockfd);
ssize_t threadRecv(struct thread_t *thread, int sockfd, void *buf, size_t len, int flags);
ssize_t threadSend(struct thread_t *thread, int sockfd, const void *buf, size_t len, int flags);

void threadMutexFill(struct threadMutex_t *mutex);
int threadMutexInit(struct threadMutex_t *mutex);
void threadMutexDestroy(struct threadMutex_t *mutex);
void threadMutexLock(struct threadMutex_t *mutex);
void threadMutexUnlock(struct threadMutex_t *mutex);

#endif

