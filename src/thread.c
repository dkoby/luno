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
    #include <windows.h>
#else
    #include <pthread.h>
    #include <sys/select.h>
    #include <sys/time.h>
    #include <sys/socket.h>
#endif
/* */
#include "thread.h"
/* */
#include "debug.h"

#ifdef WINDOWS
    #define THREAD_RUN_WRAP(name, argName) DWORD WINAPI name(LPVOID argName)
#else
    #define THREAD_RUN_WRAP(name, argName) void *name(void *argName)

#endif

static THREAD_RUN_WRAP(thread_Run, arg);
static void thread_Stop(void *arg);

/*
 *
 */
void threadInit(struct thread_t *thread)
{
#ifdef WINDOWS
    thread->cancelEvent = NULL;
    thread->stopEvent   = NULL;
    thread->wsaSockReadEvent = WSA_INVALID_EVENT;
    thread->wsaSockWriteEvent = WSA_INVALID_EVENT;
#endif
}
/*
 * RETURN
 *     0 on success, -1 on error.
 */
int threadCreate(struct thread_t *thread, ThreadRun run, ThreadStop stop, void *arg)
{
    thread->stop = stop;
    thread->run  = run;
    thread->arg  = arg;
#ifdef WINDOWS
    thread->cancelEvent = CreateEvent(
        NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes, */
        FALSE, /* BOOL                  bManualReset, */
        FALSE, /* BOOL                  bInitialState, */
        NULL   /* LPCSTR                lpName */
    );
    thread->stopEvent = CreateEvent(
        NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes, */
        FALSE, /* BOOL                  bManualReset, */
        FALSE, /* BOOL                  bInitialState, */
        NULL   /* LPCSTR                lpName */
    );
    if (thread->cancelEvent == NULL || thread->stopEvent == NULL)
    {
        debugPrint(DLEVEL_ERROR, "Events creation failed");
        goto error;
    }
    thread->thread = CreateThread(
        NULL,        /* LPSECURITY_ATTRIBUTES   lpThreadAttributes, */
        0,           /* SIZE_T                  dwStackSize, */
        thread_Run,  /* LPTHREAD_START_ROUTINE  lpStartAddress, */
        thread,      /* __drv_aliasesMem LPVOID lpParameter, */
        0,           /* DWORD                   dwCreationFlags, */
        NULL         /* LPDWORD                 lpThreadId */
    );
    if (thread->thread == NULL)
    {
        debugPrint(DLEVEL_ERROR, "Thread creation failed");
        goto error;
    }
    return 0;
error:
    if (thread->stopEvent)
        CloseHandle(thread->stopEvent);
    if (thread->cancelEvent)
        CloseHandle(thread->cancelEvent);
    return -1;
#else
    if (pthread_create(&thread->thread, NULL, thread_Run, thread) != 0)
        return -1;
    else
        return 0;
#endif
}
/*
 * RETURN
 *     0 on success, -1 on error.
 */
int threadInitSock(struct thread_t *thread, int sockfd)
{
#ifdef WINDOWS
    thread->wsaSockReadEvent = WSACreateEvent();
    thread->wsaSockWriteEvent = WSACreateEvent();
    if (thread->wsaSockReadEvent  == WSA_INVALID_EVENT ||
        thread->wsaSockWriteEvent == WSA_INVALID_EVENT)
    {
        debugPrint(DLEVEL_ERROR, "WSACreateEvent failed");
        goto error;
    }
    return 0;
error:
    if (thread->wsaSockReadEvent != WSA_INVALID_EVENT)
        WSACloseEvent(thread->wsaSockReadEvent);
    if (thread->wsaSockWriteEvent != WSA_INVALID_EVENT)
        WSACloseEvent(thread->wsaSockWriteEvent);
    return -1;
#else
    return 0;
#endif
}
/*
 *
 */
static THREAD_RUN_WRAP(thread_Run, arg)
{
#ifdef WINDOWS
    struct thread_t *thread;

    thread = arg;

    thread->run(thread->arg);
    thread_Stop(thread);

    return 0;
#else
    int r;
    int oldstate, oldtype;
    struct thread_t *thread;

    thread = arg;

    /*
     * Enable cancelation points for functions read(), select(), ... .
     */
    r = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    if (r != 0)
    {
        debugPrint(DLEVEL_ERROR, "%s, setcancelstate", __FUNCTION__);
    }
    r = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
    if (r != 0)
    {
        debugPrint(DLEVEL_ERROR, "%s, setcanceltype.", __FUNCTION__);
    }

    pthread_cleanup_push(thread_Stop, thread);
    thread->run(thread->arg);
    pthread_cleanup_pop(1);

    r = pthread_detach(thread->thread);
    if (r != 0)
        debugPrint(DLEVEL_ERROR, "Thread detach failed, (%d).", r);

    return NULL;
#endif
}
#ifdef WINDOWS
/*
 *
 */
static void thread_Clean(struct thread_t *thread)
{
    if (thread->stopEvent)
        CloseHandle(thread->stopEvent);
    if (thread->cancelEvent)
        CloseHandle(thread->cancelEvent);
    if (thread->wsaSockReadEvent != WSA_INVALID_EVENT)
        WSACloseEvent(thread->wsaSockReadEvent);
    if (thread->wsaSockWriteEvent != WSA_INVALID_EVENT)
        WSACloseEvent(thread->wsaSockWriteEvent);
}
#endif
/*
 *
 */
static void thread_Stop(void *arg)
{
    struct thread_t *thread;

    thread = arg;

    if (thread->stop)
        thread->stop(thread->arg);
#ifdef WINDOWS
    if (!SetEvent(thread->stopEvent))
    {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "SetEvent failed (stop)");
    }
    /* Was cancel, clean-up will be done by threadCancel */
    if (WaitForSingleObject(thread->cancelEvent, 0) == WAIT_OBJECT_0)
    {

    } else {
        thread_Clean(thread);
    }
#endif
}
/*
 * Cancel thread.
 */
void threadCancel(struct thread_t *thread)
{
#ifdef WINDOWS
    DWORD res;

    if (!SetEvent(thread->cancelEvent))
    {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "SetEvent failed (cancel)");
    }

    res = WaitForSingleObject(thread->stopEvent, INFINITE);
    if (res == WAIT_OBJECT_0)
    {
        thread_Clean(thread);
    } else if (res != WAIT_TIMEOUT) {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "Wait for stopEvent failed, %d", GetLastError());
    }
#else
    int r;

    r = pthread_cancel(thread->thread);
    if (r != 0)
    {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "Thread cancel failed, (%d).", r);
    }
    if (pthread_join(thread->thread, NULL) != 0)
    {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "Thread join failed, (%d).", r);
    }
#endif
}
/*
 *
 */
void threadSleepMs(struct thread_t *thread, int ms)
{
#ifdef WINDOWS
    DWORD res;

    res = WaitForSingleObject(thread->cancelEvent, ms);
    if (res == WAIT_OBJECT_0)
    {
        thread_Stop(thread);
        ExitThread(0);
        /* NOTREACHED */
    } else if (res != WAIT_TIMEOUT) {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "Wait for cancelEvent failed");
    }
#else
    int s;
    struct timeval timeout;

    timeout.tv_sec  = ms / 1000;
    timeout.tv_usec = (ms % 1000) * 1000;

    s = select(0, NULL, NULL, NULL, &timeout);
    if (s != 0)
    {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "select failed, %s", __FUNCTION__);
    }
#endif
}
/*
 *
 */
ssize_t threadRecv(struct thread_t *thread, int sockfd, void *buf, size_t len, int flags)
{
#ifdef WINDOWS
    WSAEVENT events[2];
    DWORD res;

    if (WSAEventSelect(sockfd, thread->wsaSockReadEvent, FD_READ | FD_OOB) == SOCKET_ERROR)
    {
        debugPrint(DLEVEL_ERROR, "WSAEventSelect failed");
        return -1;
    }

    events[0] = thread->cancelEvent;
    events[1] = thread->wsaSockReadEvent;

    res = WSAWaitForMultipleEvents(
        2, events, FALSE, WSA_INFINITE, FALSE);
    if (res == WSA_WAIT_EVENT_0 /* cancel */)
    {
        thread_Stop(thread);
        ExitThread(0);
        /* NOTREACHED */
        return -1;
    } else if (res == (WSA_WAIT_EVENT_0 + 1) /* socket have data */) {
        WSAResetEvent(thread->wsaSockReadEvent);
        return recv(sockfd, buf, len, flags);
    } else {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "WSAWaitForMultipleEvents failed");
        return (ssize_t)-1;
    }
#else
    return recv(sockfd, buf, len, flags);
#endif
}
/*
 *
 */
ssize_t threadSend(struct thread_t *thread, int sockfd, const void *buf, size_t len, int flags)
{
#ifdef WINDOWS
    WSAEVENT events[2];
    DWORD res;

    if (WSAEventSelect(sockfd, thread->wsaSockWriteEvent, FD_WRITE) == SOCKET_ERROR)
    {
        debugPrint(DLEVEL_ERROR, "WSAEventSelect failed");
        return -1;
    }

    events[0] = thread->cancelEvent;
    events[1] = thread->wsaSockWriteEvent;

    res = WSAWaitForMultipleEvents(
        2, events, FALSE, WSA_INFINITE, FALSE);
    if (res == WSA_WAIT_EVENT_0 /* cancel */)
    {
        thread_Stop(thread);
        ExitThread(0);
        /* NOTREACHED */
        return -1;
    } else if (res == (WSA_WAIT_EVENT_0 + 1) /* socket is writable */) {
        WSAResetEvent(thread->wsaSockWriteEvent);
        return send(sockfd, buf, len, flags);
    } else {
        /* NOTREACHED */
        debugPrint(DLEVEL_ERROR, "WSAWaitForMultipleEvents failed");
        return (ssize_t)-1;
    }
#else
    return send(sockfd, buf, len, flags);
#endif
}
/*
 *
 */
void threadMutexFill(struct threadMutex_t *mutex)
{
#ifdef WINDOWS
#else
    mutex->pmutex = NULL;
#endif
}
/*
 * RETURN
 *     0 on success, -1 on error.
 */
int threadMutexInit(struct threadMutex_t *mutex)
{
#ifdef WINDOWS
    mutex->mutex = CreateMutex(NULL, FALSE, NULL);
    if (mutex->mutex != NULL)
        return 0;
    else
        return -1;
#else
    if (pthread_mutex_init(&mutex->mutex, NULL) == 0)
    {
        mutex->pmutex = &mutex->mutex;
        return 0;
    } else {
        mutex->pmutex = NULL;
        return -1;
    }
#endif
}
/*
 */
void threadMutexDestroy(struct threadMutex_t *mutex)
{
#ifdef WINDOWS
    if (mutex->mutex)
    {
        if (!CloseHandle(mutex->mutex))
            debugPrint(DLEVEL_ERROR, "Failed to destroy mutex");
    }
#else
    if (mutex->pmutex)
        pthread_mutex_destroy(mutex->pmutex);
#endif

}
/*
 *
 */
void threadMutexLock(struct threadMutex_t *mutex)
{
    int res;
#ifdef WINDOWS
    DWORD wres;

    wres = WaitForSingleObject(mutex->mutex, INFINITE);
    if (wres != WAIT_OBJECT_0)
        res = -1;
    else
        res = 0;

#else
    res = pthread_mutex_lock(mutex->pmutex);
#endif

    if (res != 0)
        debugPrint(DLEVEL_ERROR, "Failed to lock mutex");
}
/*
 *
 */
void threadMutexUnlock(struct threadMutex_t *mutex)
{
    int res;
#ifdef WINDOWS
    if (!ReleaseMutex(mutex->mutex))
        res = -1;
    else
        res = 0;
#else
    res = pthread_mutex_unlock(mutex->pmutex);
#endif

    if (res != 0)
        debugPrint(DLEVEL_ERROR, "Failed to lock mutex");
}

