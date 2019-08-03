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
/* */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <lsqlite3.h>
/* */
/* */
#include "lclient.h"
/* */
#include "client.h"
#include "debug.h"
#include "debug.h"
#include "http.h"
#include "lmromfs.h"
#include "lserver.h"
#include "lua/init0.h"
#include "lua/init1.h"
#include "lua/ljson.h"
#include "lua/process.h"
#include "lua/util.h"
#include "server.h"
#include "version.h"

#if 0
    #define DEBUG_THIS
#endif

#define MFS_PREFIX "mfs/"

static int lclient_CloseConnection(lua_State *L);
static int lclient_ServerDebugPrint(lua_State *L);
static int lclient_ServerCaching(lua_State *L);
static int lclient_ServerNewSession(lua_State *L);
static int lclient_ServerHasSession(lua_State *L);
static int lclient_ServerSetSessionString(lua_State *L);
static int lclient_ServerGetSessionString(lua_State *L);
static int lclient_requestGetContent(lua_State *L);
static int lclient_responseWriteSock(lua_State *L);

static struct script_t {
    const char *data;
    int size;
    const char *name;
} scripts[] = {
    {utilScript, sizeof(utilScript), "utilScript"},
    {init0Script, sizeof(init0Script), "init0Script"},
    {ljsonScript, sizeof(ljsonScript), "ljsonScript"},
//    {websocketScript, sizeof(websocketScript), "websocketScript"},
    {NULL, 0, NULL},
};

/*
 * Initialize lua state on client creation.
 *
 * Global variables:
 *     client
 *
 *     MFS_PREFIX
 *
 *     RESOURCE_DIR
 */
int lclientInit0(struct client_t *client)
{
    lua_State *L;
    struct script_t *script;

    L = luaL_newstate();
    if (!L)
    {
        DEBUG_CLIENT(DLEVEL_ERROR, "%s", "Lua state init failed");
        return -1;
    }
    client->luaState = L;
    luaL_openlibs(L);

    lmromfsOpenLib(L);

    lua_pushlightuserdata(L, client); lua_setglobal(L, "client");

    lua_pushstring(L, MFS_PREFIX);
    lua_setglobal(L, "MFS_PREFIX");

    if (server.resourceDir)
        lua_pushstring(L, server.resourceDir);
    else
        lua_pushstring(L, MFS_PREFIX);
    lua_setglobal(L, "RESOURCE_DIR");

    lua_pushinteger(L, HTTP_101_SWITCHING_PROTOCOLS); lua_setglobal(L, "HTTP_101_SWITCHING_PROTOCOLS");
    lua_pushinteger(L, HTTP_200_OK                 ); lua_setglobal(L, "HTTP_200_OK");
    lua_pushinteger(L, HTTP_204_NO_CONTENT         ); lua_setglobal(L, "HTTP_204_NO_CONTENT");
    lua_pushinteger(L, HTTP_301_MOVED_PERMANENTLY  ); lua_setglobal(L, "HTTP_301_MOVED_PERMANENTLY");
    lua_pushinteger(L, HTTP_302_FOUND              ); lua_setglobal(L, "HTTP_302_FOUND");
    lua_pushinteger(L, HTTP_303_SEE_OTHER          ); lua_setglobal(L, "HTTP_303_SEE_OTHER");
    lua_pushinteger(L, HTTP_304_NOT_MODIFIED       ); lua_setglobal(L, "HTTP_304_NOT_MODIFIED");
    lua_pushinteger(L, HTTP_307_TEMPORARY_REDIRECT ); lua_setglobal(L, "HTTP_307_TEMPORARY_REDIRECT");
    lua_pushinteger(L, HTTP_400_BAD_REQUEST        ); lua_setglobal(L, "HTTP_400_BAD_REQUEST");
    lua_pushinteger(L, HTTP_403_FORBIDDEN          ); lua_setglobal(L, "HTTP_403_FORBIDDEN");
    lua_pushinteger(L, HTTP_404_NOT_FOUND          ); lua_setglobal(L, "HTTP_404_NOT_FOUND");
    lua_pushinteger(L, HTTP_405_METHOD_NOT_ALLOWED ); lua_setglobal(L, "HTTP_405_METHOD_NOT_ALLOWED");
    lua_pushinteger(L, HTTP_409_CONFLICT           ); lua_setglobal(L, "HTTP_409_CONFLICT");
    lua_pushinteger(L, HTTP_500_INTERNAL_SERVER_ERROR); lua_setglobal(L, "HTTP_500_INTERNAL_SERVER_ERROR");

    lua_pushinteger(L, DLEVEL_SYS    ); lua_setglobal(L, "DLEVEL_SYS");
    lua_pushinteger(L, DLEVEL_SILENT ); lua_setglobal(L, "DLEVEL_SILENT");
    lua_pushinteger(L, DLEVEL_ERROR  ); lua_setglobal(L, "DLEVEL_ERROR");
    lua_pushinteger(L, DLEVEL_WARNING); lua_setglobal(L, "DLEVEL_WARNING");
    lua_pushinteger(L, DLEVEL_INFO   ); lua_setglobal(L, "DLEVEL_INFO");
    lua_pushinteger(L, DLEVEL_NOISE  ); lua_setglobal(L, "DLEVEL_NOISE");

    luaopen_lsqlite3(L);
    lua_setglobal(L, "sqlite3");

//#if 1
//
//   luaopen_lcrypto(L);
//   lua_setglobal("lcrypto");
//#else
//    /*
//     * lcrypto
//     */
//    lua_getglobal(L, "package");    /* [package]->TOS */
//    lua_getfield(L, -1, "preload"); /* [package][preload]->TOS */
//
//    lua_pushcfunction(L, lclientLoadLCrypto); /* [package][preload][function]->TOS */
//    lua_setfield(L, -2, "lcrypto");  /* [package][preload]->TOS */
//    lua_pop(L, 2); /* ->TOS */
//#endif

    /*
     * Set server's table: functions, variables, etc.
     */
    {
        lua_newtable(L);               /* [newtable]->TOS */
        lua_pushvalue(L, -1);          /* [newtable][newtable]->TOS */
        lua_setglobal(L, "server");    /* [newtable]->TOS */

        lua_pushstring(L, "port");             /* [newtable][key]->TOS */
        lua_pushinteger(L, server.portNumber); /* [newtable][key][value]->TOS */
        lua_rawset(L, -3);                     /* [newtable]->TOS */

        lua_pushstring(L, "VERSION");   /* [newtable]["VERSION"]->TOS */
        lua_pushinteger(L, (MAJOR << 16) | (MINOR <<  8) | (BUILD <<  0));
        lua_rawset(L, -3);              /* [newtable]->TOS */

        lua_pushstring(L, "debugLevel"); /* [newtable][key]->TOS */
        lua_pushinteger(L, debugLevel);  /* [newtable][key][value]->TOS */
        lua_rawset(L, -3);               /* [newtable]->TOS */

        lua_pushstring(L, "debugPrint");                /* [newtable][key]->TOS */
        lua_pushcfunction(L, lclient_ServerDebugPrint); /* [newtable][key][value]->TOS */
        lua_rawset(L, -3);                              /* [newtable]->TOS */

        lua_pushstring(L, "caching");             /* [newtable][key]->TOS */
        lua_pushcfunction(L, lclient_ServerCaching); /* [newtable][key][value]->TOS */
        lua_rawset(L, -3);                           /* [newtable]->TOS */

        lua_pushstring(L, "newSession");         /* [newtable][key]->TOS */
        lua_pushcfunction(L, lclient_ServerNewSession); /* [newtable][key][value]->TOS */
        lua_rawset(L, -3);                       /* [newtable]->TOS */

        lua_pushstring(L, "hasSession");         /* [newtable][key]->TOS */
        lua_pushcfunction(L, lclient_ServerHasSession); /* [newtable][key][value]->TOS */
        lua_rawset(L, -3);                       /* [newtable]->TOS */

        lua_pushstring(L, "setSessionString");         /* [newtable][key]->TOS */
        lua_pushcfunction(L, lclient_ServerSetSessionString); /* [newtable][key][value]->TOS */
        lua_rawset(L, -3);                             /* [newtable]->TOS */

        lua_pushstring(L, "getSessionString");         /* [newtable][key]->TOS */
        lua_pushcfunction(L, lclient_ServerGetSessionString); /* [newtable][key][value]->TOS */
        lua_rawset(L, -3);                             /* [newtable]->TOS */

        lua_pop(L, 1); /* ->TOS */
    }

    for (script = scripts; script->data != NULL; script++)
    {
        if (luaL_loadbufferx(L, script->data, script->size, script->name, "b") != LUA_OK)
        {
            DEBUG_CLIENT(DLEVEL_ERROR, "(E) Failed to load init script, \"%s\": \"%s\".",
                    script->name,
                    lua_tostring(L, -1));
            lua_pop(L, 1);
            return -1;
        }
        if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK)
        {
            DEBUG_CLIENT(DLEVEL_ERROR, "(E) Failed to execute init script: \"%s\".",
                    lua_tostring(L, -1));
            lua_pop(L, 1);
            return -1;
        }     
    }

    /*
     * Request table.
     */
    lua_getglobal(L, "Request");              /* [Request]->TOS */
    lua_pushcfunction(L, lclient_requestGetContent); /* [Request][value]->TOS */
    lua_setfield(L, -2, "getContent");        /* [Request]->TOS */
    lua_pushcfunction(L, lclient_CloseConnection);   /* [Request]->TOS */
    lua_setfield(L, -2, "closeConnection");   /* [Request]->TOS */
    lua_pushinteger(L, client->serverPort);   /* [Request][value]->TOS */
    lua_setfield(L, -2, "serverPort");        /* [Request]->TOS */
    lua_pop(L, 1);                            /* ->TOS */

    /*
     * Response table.
     */
    lua_getglobal(L, "Response");             /* [response]->TOS */
    lua_pushcfunction(L, lclient_responseWriteSock); /* [response][value]->TOS */
    lua_setfield(L, -2, "writeSock");         /* [response]->TOS */
    lua_pop(L, 1);                            /* ->TOS */

#if (1 && (defined DEBUG_THIS))
    /* Stack MUST be empty (gettop return 0). */
    DEBUG_CLIENT(DLEVEL_NOISE, "%s, stack \"%d\"",
            __FUNCTION__, lua_gettop(L));
#endif
    return 0;
}
/*
 * Initialize lua state on new request.
 */
int lclientInit1(struct client_t *client)
{
    lua_State *L = client->luaState;

    if (luaL_loadbufferx(L, init1Script, sizeof(init1Script), "init1Script", "b") != LUA_OK)
    {
        DEBUG_CLIENT(DLEVEL_ERROR, "Failed to load init1 script: \"%s\".",
                lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }
    if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK)
    {
        DEBUG_CLIENT(DLEVEL_ERROR, "Failed to execute init1 script: \"%s\".",
                lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }

#if (1 && (defined DEBUG_THIS))
    /* Stack MUST be empty (gettop return 0). */
    debugPrint(DLEVEL_NOISE, "%s stack \"%d\" [sock %d].",
            __FUNCTION__, lua_gettop(L), client->sock);
#endif
    return 0;
}
/*
 *
 */
void lclientDestroy(struct client_t *client)
{
    if (client->luaState)
        lua_close(client->luaState);
}
/*
 * Process http request.
 *
 * RETURN
 *     0 if connection must be dropped, 1 otherwise.
 */
int lclientProcessRequest(struct client_t *client, int httpError)
{
    lua_State *L = client->luaState;

    lua_pushinteger(L, httpError);
    lua_setglobal(L, "httpError");

    if (luaL_loadbufferx(L, processScript, sizeof(processScript), "processScript", "b") != LUA_OK)
    {
        DEBUG_CLIENT(DLEVEL_ERROR, "Failed to load process script: \"%s\".",
                lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }
    if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK)
    {
        DEBUG_CLIENT(DLEVEL_ERROR, "Failed to execute process script: \"%s\".",
                lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }     

    return 1;
}
/*
 *
 */
static int lclient_CloseConnection(lua_State *L)
{
    struct client_t *client;

    lua_getglobal(L, "client");
    client = lua_touserdata(L, -1);
    lua_pop(L, 1);

    client->request.keepAlive = 0;

    return 0;
}
/*
 *
 */
static int lclient_ServerDebugPrint(lua_State *L)
{
    struct client_t *client;
    int argc;
    int isnum;
    int level;
    const char *s;
#define _LSTATE_DEBUG_PRINT_LEVEL_ARG    1
#define _LSTATE_DEBUG_PRINT_STRING_ARG   2
    argc = lua_gettop(L);
    if (argc < _LSTATE_DEBUG_PRINT_STRING_ARG)
        luaL_error(L, "not enough arguments");

    level = lua_tointegerx(L, _LSTATE_DEBUG_PRINT_LEVEL_ARG, &isnum);
    if (!isnum)
        luaL_argerror(L, _LSTATE_DEBUG_PRINT_LEVEL_ARG, "not a number");
    s = lua_tostring(L, _LSTATE_DEBUG_PRINT_STRING_ARG);
    if (!s)
        luaL_argerror(L, _LSTATE_DEBUG_PRINT_STRING_ARG, "not a string");

    lua_getglobal(L, "client");
    client = lua_touserdata(L, -1);
    lua_pop(L, 1);

#if 0
    debugPrint(level, (char*)s);
#else
    DEBUG_CLIENT(level, "%s", s);
#endif
    return 0;
}
/*
 *
 * RETURN
 *     On stack index -1 string with content. Empty string if no content remain / error.
 */
static int lclient_requestGetContent(lua_State *L)
{
    struct client_t *client;
    int len;
    int r;
    int n;
    int isnum;
    int argc;
    luaL_Buffer lbuf;
    char *data;
#define _LSTATE_GET_CONTENT_LEN_ARG    1

    argc = lua_gettop(L);
    if (argc < _LSTATE_GET_CONTENT_LEN_ARG)
        luaL_error(L, "not enough arguments");

    len = lua_tointegerx(L, _LSTATE_GET_CONTENT_LEN_ARG, &isnum);
    if (!isnum)
        luaL_argerror(L, _LSTATE_GET_CONTENT_LEN_ARG, "not a number");
    if (len < 0)
        luaL_argerror(L, _LSTATE_GET_CONTENT_LEN_ARG, "must be positive");

    lua_getglobal(L, "client");
    client = lua_touserdata(L, -1);
    lua_pop(L, 1);

    data = luaL_buffinitsize(L, &lbuf, len);
    r = tokenGetRemainedData(&client->token, data, len);
    n = r;
    if (n < len)
    {
        data += n;
        r = (*client->getChars)(data, len - n, client);
        if (r > 0)
            n += r;
    }
    luaL_pushresultsize(&lbuf, n);

    return 1;
}
/*
 *
 */
static int lclient_responseWriteSock(lua_State *L)
{
    const char *data;
    struct client_t *client;
    size_t len;

    int argc;
#define _WRITE_SOCK_DATA_ARG       1
    argc = lua_gettop(L);
    if (argc < _WRITE_SOCK_DATA_ARG)
        luaL_error(L, "no data to write specified");
    data = lua_tolstring(L, _WRITE_SOCK_DATA_ARG, &len);
    if (!data)
        luaL_argerror(L, _WRITE_SOCK_DATA_ARG, "must be string");

    lua_getglobal(L, "client");
    client = lua_touserdata(L, -1);
    lua_pop(L, 1);

    /* TODO raise error instead of boolean */
    if (clientSendChars(client, (const void*)data, len) < 0)
        luaL_error(L, "write to socket failed");

    return 0;
}
/*
 *
 */
static int lclient_ServerNewSession(lua_State *L)
{
    lua_pushinteger(L, lserverNewSession());
    return 1;
}
/*
 * RETURN
 *     -1    true if server has session with id, false otherwise.
 */
static int lclient_ServerHasSession(lua_State *L)
{
    uint32_t id;
    int isnum;
    int argc;

#define _LSERVERHASSESSION_ID_ARG    1

    argc = lua_gettop(L);
    if (argc < _LSERVERHASSESSION_ID_ARG)
        luaL_error(L, "Not enough arguments.");

    id = lua_tonumberx(L, _LSERVERHASSESSION_ID_ARG, &isnum);
    if (!isnum)
        luaL_argerror(L, _LSERVERHASSESSION_ID_ARG, "Id not a number");

    if (lserverHasSession(id) == 0)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);
    return 1;
}
/*
 * Set string of session table.
 *
 * ARGS
 *     1    ID of session.
 *     2    Key to set.
 *     3    Value to set.
 *
 * RETURN
 *     -1    true if server has session with id, false otherwise.
 */
static int lclient_ServerSetSessionString(lua_State *L)
{
    int argc;
    int isnum;
    uint32_t id;
    const char *key;
    const char *value;

#define _SERVERSETSESSIONSTRING_KEY_ARG      1
#define _SERVERSETSESSIONSTRING_VALUE_ARG    2
    argc = lua_gettop(L);
    if (argc < _SERVERSETSESSIONSTRING_VALUE_ARG)
        luaL_error(L, "Not enough arguments");

    {
        if (lua_getglobal(L, "request") != LUA_TTABLE)
            luaL_error(L, "\"request\" not a table.");

        /* [request]->TOS */
        lua_getfield(L, -1, "sessionId"); /* [request][sessionId]->TOS */

        id = lua_tonumberx(L, -1, &isnum);
        if (!isnum)
            luaL_error(L, "sessionId not a number");
        lua_pop(L, 2); /* ->TOS */
    }

    key = lua_tostring(L, _SERVERSETSESSIONSTRING_KEY_ARG);
    if (!key)
        luaL_argerror(L, _SERVERSETSESSIONSTRING_KEY_ARG, "Must be string");
    value = lua_tostring(L, _SERVERSETSESSIONSTRING_VALUE_ARG);
    if (!value)
        luaL_argerror(L, _SERVERSETSESSIONSTRING_VALUE_ARG, "Must be string");

    if (lserverSetSessionString(id, key, value) < 0)
        lua_pushboolean(L, 0);
    else
        lua_pushboolean(L, 1);
    return 1;
}
/*
 * Get string of session table.
 *
 * ARGS
 *     1    ID of session.
 *     2    Key to get.
 *
 * RETURN
 *     -1    Value of key, nil otherwise.
 */
static int lclient_ServerGetSessionString(lua_State *L)
{
    int argc;
    int isnum;
    uint32_t id;
    const char *key;

#define _SERVERGETSESSIONSTRING_KEY_ARG      1
    argc = lua_gettop(L);
    if (argc < _SERVERGETSESSIONSTRING_KEY_ARG)
        luaL_error(L, "Not enough arguments");

    key = lua_tostring(L, _SERVERGETSESSIONSTRING_KEY_ARG);
    if (!key)
        luaL_argerror(L, _SERVERGETSESSIONSTRING_KEY_ARG, "Must be string");

    {
        if (lua_getglobal(L, "request") != LUA_TTABLE)
            luaL_error(L, "\"request\" not a table.");

        /* [request]->TOS */
        lua_getfield(L, -1, "sessionId"); /* [request][sessionId]->TOS */

        id = lua_tonumberx(L, -1, &isnum);
        if (!isnum)
            luaL_error(L, "sessionId not a number");
        lua_pop(L, 2); /* ->TOS */
    }

    if (lserverGetSessionString(L, id, key) < 0)
        lua_pushnil(L);
    return 1;
}
/*
 *
 */
static void lclient_SetRequestField(lua_State *L, char *field, char *value, int *vlen)
{
    lua_getglobal(L, "request");      /* [request]->TOS */
    if (vlen)
        lua_pushlstring(L, value, *vlen);  /* [request][value]->TOS */
    else
        lua_pushstring(L, value);  /* [request][value]->TOS */
    lua_setfield(L, -2, field);       /* [request]->TOS */
    lua_pop(L, 1);                    /* ->TOS */
#if (0 && (defined DEBUG_THIS))
    /* Stack MUST be empty (gettop return 0). */
    debugPrint(DLEVEL_NOISE, "%s stack \"%d\" [sock %d].",
            __FUNCTION__, lua_gettop(L), client->sock);
#endif
}
void lclientSetRequestField(lua_State *L, char *field, char *value)
{
    lclient_SetRequestField(L, field, value, NULL);
}
void lclientSetRequestFieldL(lua_State *L, char *field, char *value, int vlen)
{
    lclient_SetRequestField(L, field, value, &vlen);
}
/*
 *
 */
void lclient_AppendRequestField(lua_State *L, char *field, char *value, int *vlen)
{
    lua_getglobal(L, "request");      /* [request]->TOS */
    lua_getfield(L, -1, field);       /* [request][oldval]->TOS */
    if (vlen)
        lua_pushlstring(L, value, *vlen);  /* [request][oldval][newval]->TOS */
    else
        lua_pushstring(L, value);  /* [request][oldval][newval]->TOS */
    lua_concat(L, 2);                 /* [request][oldval..newval]->TOS */
    lua_setfield(L, -2, field);       /* [request]->TOS */
    lua_pop(L, 1);                    /* ->TOS */
#if (0 && (defined DEBUG_THIS))
    /* Stack MUST be empty (gettop return 0). NOTE Not empty on parsing query string. */
    debugPrint(DLEVEL_NOISE, "%s stack \"%d\" [sock %d].",
            __FUNCTION__, lua_gettop(L), client->sock);
#endif
}
void lclientAppendRequestField(lua_State *L, char *field, char *value)
{
    lclient_AppendRequestField(L, field, value, NULL);
}
void lclientAppendRequestFieldL(lua_State *L, char *field, char *value, int vlen)
{
    lclient_AppendRequestField(L, field, value, &vlen);
}
/*
 *
 */
void lclientSetRequestFieldNum(lua_State *L, char *field, int64_t num)
{
    lua_getglobal(L, "request");           /* [request]->TOS */
    lua_pushinteger(L, (lua_Integer)num);  /* [request][num]->TOS */
    lua_setfield(L, -2, field);            /* [request]->TOS */
    lua_pop(L, 1);                         /* ->TOS */
#if (0 && (defined DEBUG_THIS))
    /* Stack MUST be empty (gettop return 0). */
    debugPrint(DLEVEL_NOISE, "%s stack \"%d\" [sock %d].",
            __FUNCTION__, lua_gettop(L), client->sock);
#endif
}
/*
 *
 */
void lclientHeadersAdd(lua_State *L)
{
    int t;

#define _HEADERS_NAME "headers"

    lua_getglobal(L, "request");  /* [request]->TOS */
    lua_pushstring(L, _HEADERS_NAME); /* [request]["headers"]->TOS */
    t = lua_rawget(L, -2); /* [request][headers]->TOS */
    if (t == LUA_TNIL)
    {
        /* [request][nil]->TOS */
        lua_pop(L, 1); /* [request]->TOS */
        lua_newtable(L); /* [request][newtable]->TOS */
        lua_pushstring(L, _HEADERS_NAME); /* [request][newtable]["headers"]->TOS */
        lua_pushvalue(L, -2); /* [request][newtable]["headers"][newtable]->TOS */
        lua_rawset(L, -4); /* [request][newtable]->TOS */
    }
    /* [request][headers]->TOS */

    lua_getglobal(L, LSTATE_GLOBAL_VAR_FIELD_NAME); /* [request][headers][fieldName]->TOS */
    lua_getglobal(L, LSTATE_GLOBAL_VAR_FIELD_VALUE); /* [request][headers][fieldName][fieldValue]->TOS */
    lua_rawset(L, -3); /* [request][headers]->TOS */
    lua_pop(L, 2); /* ->TOS */

#if (0 && (defined DEBUG_THIS))
    debugPrint(DLEVEL_NOISE, "%s stack \"%d\".",
            __FUNCTION__, lua_gettop(L));
#endif
}
/*
 * Set temporary value in client's lua state.
 *
 */
static void lclient_SetGlobalVal(lua_State *L, char *name, char *value, int *vlen)
{
    if (vlen)
        lua_pushlstring(L, value, *vlen);  /* [value]->TOS */
    else
        lua_pushstring(L, value);  /* [value]->TOS */
    lua_setglobal(L, name); /* ->TOS */
#if (0 && (defined DEBUG_THIS))
    /* Stack MUST be empty (gettop return 0). */
    debugPrint(DLEVEL_NOISE, "%s stack \"%d\" [sock %d].",
            __FUNCTION__, lua_gettop(L), client->sock);
#endif
}
/*
 *
 */
void lclientSetGlobalVal(lua_State *L, char *name, char *value)
{
    lclient_SetGlobalVal(L, name, value, NULL);
}
/*
 *
 */
void lclientSetGlobalValL(lua_State *L, char *name, char *value, int vlen)
{
    lclient_SetGlobalVal(L, name, value, &vlen);
}
/*
 * Compare two global values using different compare strategies.
 *
 * RETURN
 *     1 if values are matches, 0 if values does not matches or one of
 *     value is nil.
 */
int lclientCompareGlobalVals(lua_State *L,
        char *name1, char *name2, enum lclient_compare_type_t type)
{
    int r, ret;

    ret = 0;

    r = lua_getglobal(L, name1); /* [val1]->TOS */
    if (r == LUA_TNIL)
    {
        lua_pop(L, 1); /* ->TOS */
        return 0;
    }
    r = lua_getglobal(L, name2); /* [val1][val2]->TOS */
    if (r == LUA_TNIL)
    {
        lua_pop(L, 2); /* ->TOS */
        return 0;
    }

    if (type == LSTATE_COMPARE_LUA_EQUAL)
    {
        if (lua_compare(L, -1, -2, LUA_OPEQ))
            ret = 1;
    } else if (type == LSTATE_COMPARE_CASE || LSTATE_COMPARE_NOCASE) {
        const char *val1;
        const char *val2;

        val1 = lua_tostring(L, -2);
        val2 = lua_tostring(L, -1);
        if (val1 && val2)
        {
            if (type == LSTATE_COMPARE_CASE)
            {
                if (strcmp(val1, val2) == 0)
                    ret = 1;
            } else if (type == LSTATE_COMPARE_NOCASE) {
                if (strcasecmp(val1, val2) == 0)
                    ret = 1;
            }
        }
    }
    lua_pop(L, 2); /* ->TOS */
#if (0 && (defined DEBUG_THIS))
    /* Stack MUST be empty (gettop return 0). */
    debugPrint(DLEVEL_NOISE, "%s stack \"%d\" [sock %d].",
            __FUNCTION__, lua_gettop(L), client->sock);
#endif
    return ret;
}
//
//#if 0
///*
// * Print full table of client's request storead in lua state. Used for debug
// * purposes.
// */
//void lclientPrintRequest(lua_State *L)
//{
//    static char script[] = {
//        "function print_table(level, tab)" NL
//        "    if not tab then" NL
//        "        return" NL
//        "    end" NL
//        "    for k, v in pairs(tab) do" NL
//        "        if type(v) == \"string\" or type(v) == \"table\" then" NL
//        "            print(level, string.rep('  ', level) .. k, v, '# ' .. #v)" NL
//        "        else" NL
//        "            print(level, string.rep('  ', level) .. k, v)" NL
//        "        end" NL
//        "        if type(v) == \"table\" then" NL
//        "            print_table(level + 1, v)" NL
//        "        end" NL
//        "    end" NL
//        "end" NL
//        "print_table(1, request)" NL
//    };
//    if (luaL_dostring(L, script))
//    {
//        debugPrint(DLEVEL_NOISE, "Failed to execute script: \"%s\"." NL,
//                lua_tostring(L, -1));
//        lua_pop(L, 1);
//    }
//}
//#endif

/*
 *
 * ARGS
 *     1    For server caching on - true, false for off. Not changed if no
 *          argument supplied.
 *
 * RETURN
 *     1    If server allowed caching
 */
static int lclient_ServerCaching(lua_State *L)
{
#define _SERVER_CACHING_ARG    1
    if (lua_gettop(L) >= 1)
    {
        if (!lua_isboolean(L, _SERVER_CACHING_ARG))
            luaL_argerror(L, _SERVER_CACHING_ARG, "Not a boolean.");
#if 1
        /* 
         * NOTE
         *     Danger, can be modified by any client if implemented.
         * NOTE XXX
         *     No mutex.
         */
        server.caching = lua_toboolean(L, _SERVER_CACHING_ARG);
#endif
    }

    lua_pushboolean(L, server.caching);
    return 1;
}

