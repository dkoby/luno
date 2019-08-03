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
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
/* */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
/* */
#include "debug.h"
#include "thread.h"
#include "lserver.h"
#include "server.h"

#define DEBUG_SERVER(level, fmt, ...) \
    debugPrint(level, "[LSERVER]: " fmt, __VA_ARGS__)

/*
 *
 */
int lserverInit()
{
    lua_State *L;

    L = luaL_newstate();
    if (!L)
    {
        DEBUG_SERVER(DLEVEL_ERROR, "%s", "Lua state init failed");
        goto error;
    }
    luaL_openlibs(L);
    server.luaState = L;

    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        srand(tv.tv_usec);
    }

    if (threadMutexInit(&server.lmutex) < 0)
    {
        DEBUG_SERVER(DLEVEL_ERROR, "%s", "Mutex init failed");
        goto error;
    }
    return 0;
error:
    return -1;
}
/*
 *
 */
void lserverDestroy()
{
    lua_State *L = server.luaState;

    if (L)
        lua_close(L);
    threadMutexDestroy(&server.lmutex);
}
/*
 * Create new entry in session table.
 *
 * RETURN
 *     id of entry
 */
uint32_t lserverNewSession()
{
    lua_State *L = server.luaState;
    uint32_t id;
    int t;

    threadMutexLock(&server.lmutex);

#define _SESSIONS_TABLE "sessions"
    t = lua_getglobal(L, _SESSIONS_TABLE); /* [sessions]->TOS */
    if (t == LUA_TNIL)
    {
         /* [nil]->TOS */
        lua_pop(L, 1);        /* ->TOS */
        lua_newtable(L);      /* [newtable]->TOS */
        lua_pushvalue(L, -1); /* [newtable][newtable]->TOS */
        lua_setglobal(L, _SESSIONS_TABLE); /* [newtable]->TOS */
    }
    /* [sessions]->TOS */
    while (1)
    {
        id = rand();
        t = lua_rawgeti(L, -1, id); /* [sessions][session]->TOS */
        if (t == LUA_TNIL)
        {
             /* [sessions][nil]->TOS */
            lua_pop(L, 1);   /* [sessions]->TOS */
            lua_newtable(L); /* [sessions][newtable]->TOS */
            lua_rawseti(L, -2, id); /* [sessions]->TOS */
            DEBUG_SERVER(DLEVEL_NOISE, "New session ID, %d", id);
            break;
        }
    }
    /* [sessions]->TOS */
    lua_pop(L, 1); /* ->TOS */

#if 0
    /* Stack MUST be empty (gettop return 0). */
    debugPrint(DLEVEL_NOISE, "%s stack \"%d\".",
            __FUNCTION__, lua_gettop(L));
#endif

    threadMutexUnlock(&server.lmutex);
    return id;
}
/*
 *
 */
static int _pushSessionTable(lua_State *L, uint32_t id)
{
    int ret;
    int t;

    ret = -1;

    t = lua_getglobal(L, _SESSIONS_TABLE); /* [sessions]->TOS */
    if (t == LUA_TNIL)
    {
        /* [nil]->TOS */
        DEBUG_SERVER(DLEVEL_NOISE, "(E) %s, No sessions table.", __FUNCTION__);
        goto done;
    }
    t = lua_rawgeti(L, -1, id); /* [sessions][sessions[id]]->TOS */
    if (t == LUA_TNIL)
    {
         /* [sessions][nil]->TOS */
        DEBUG_SERVER(DLEVEL_NOISE, "(E) %s, No session id, id %d.", __FUNCTION__, id);
        goto done;
    }

    ret = 0;
done:
    return ret;
}
/*
 * Check if session table with ID exists.
 *
 * RETURN
 *     0 on success, -1 if no such id exists.
 */
int lserverHasSession(uint32_t id)
{
    int ret;
    int top;
    lua_State *L = server.luaState;

    ret = -1;
    top = lua_gettop(L);
    threadMutexLock(&server.lmutex);

    if (_pushSessionTable(L, id) < 0)
        goto done;
    ret = 0;
done:
    threadMutexUnlock(&server.lmutex);
    lua_settop(L, top);
    return ret;
}
/*
 * Store string value in session table.
 *
 * RETURN
 *     0 on success, -1 if no such id exists.
 */
int lserverSetSessionString(uint32_t id, const char *name, const char *value)
{
    int ret;
    int top;
    lua_State *L = server.luaState;

    ret = -1;
    top = lua_gettop(L);
    threadMutexLock(&server.lmutex);

    if (_pushSessionTable(L, id) < 0)
        goto done;
    /* [sessions][sessions[id]]->TOS */
    lua_pushstring(L, name);  /* [sessions][sessions[id]][name]->TOS */
    lua_pushstring(L, value); /* [sessions][sessions[id]][name][value]->TOS */
    lua_rawset(L, -3);        /* [sessions][sessions[id]]->TOS */

    ret = 0;
done:
    threadMutexUnlock(&server.lmutex);
    lua_settop(L, top);
    return ret;
}
/*
 * Get string value from session table.
 *
 * NOTE
 *     Value pushed on client's lua state stack on success.
 *
 * RETURN
 *     0 on success, -1 if no such id exists or string exists.
 */
int lserverGetSessionString(lua_State *clientL, uint32_t id, const char *name)
{
    int ret;
    int top;
    int t;
    lua_State *L = server.luaState;

    ret = -1;
    top = lua_gettop(L);
    threadMutexLock(&server.lmutex);

    if (_pushSessionTable(L, id) < 0)
        goto done;

    /* [sessions][sessions[id]]->TOS */
    lua_pushstring(L, name);  /* [sessions][sessions[id]][name]->TOS */
    t = lua_rawget(L, -2);    /* [sessions][sessions[id]][value]->TOS */
    if (t != LUA_TSTRING)
    {
        DEBUG_SERVER(DLEVEL_NOISE, "(E) %s, Not a string, id %d, \"%s\".", __FUNCTION__, id, name);
        goto done;
    }
    /* [sessions][sessions[id]][value]->TOS */

    lua_pushstring(clientL, lua_tostring(L, -1));

    ret = 0;
done:
    threadMutexUnlock(&server.lmutex);
    lua_settop(L, top);
    return ret;
}

