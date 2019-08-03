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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
/* */
#include "lmromfs.h"
/* */
#include "server.h"

static int lmromfs_Open(lua_State *L);
static int lmromfs_Read(lua_State *L);
static int lmromfs_FileSize(lua_State *L);

static const struct luaL_Reg mromfsLib[] = {
    {"open", lmromfs_Open},
    {"read", lmromfs_Read},
    {"fsize", lmromfs_FileSize},
    {NULL, NULL}  /* sentinel */
};

/*
 *
 */
void lmromfsOpenLib(lua_State *L)
{
    luaL_newlib(L, mromfsLib);
    lua_setglobal(L, "mromfs");
}

/*
 * ARGS
 *     1    File name to open.
 * RETURN
 *     1    File descriptor on success. Nil on error.
 */
static int lmromfs_Open(lua_State *L)
{
    const char *name;
    struct mromfs_fd_t *fd;

#define _MROMFS_OPEN_NAME_ARG   1
    name = lua_tostring(L, _MROMFS_OPEN_NAME_ARG);
    if (!name)
        luaL_argerror(L, _MROMFS_OPEN_NAME_ARG, "Invalid value of name.");

    fd = lua_newuserdata(L, sizeof(struct mromfs_fd_t));
    /* TOS -> fd */

    if (mromfsOpen(&server.mromfs, fd, name) < 0)
    {
        lua_pop(L, 1);
        lua_pushnil(L);
    }

    return 1;
}
/*
 * ARGS
 *     1    File descriptor returned by "mromfs.open".
 *     2    Count of bytes to read.
 * RETURN
 *     1    File data on success, nil on error.
 */
static int lmromfs_Read(lua_State *L)
{
    int cnt;
    int isnum;
    struct mromfs_fd_t *fd;
    luaL_Buffer lbuf;
    char *data;
    int r;

#define _MROMFS_READ_FD_ARG      1
#define _MROMFS_READ_COUNT_ARG   2
    if (!lua_isuserdata(L, _MROMFS_READ_FD_ARG))
        luaL_argerror(L, _MROMFS_READ_FD_ARG, "Not a file descriptor.");
    fd = lua_touserdata(L, _MROMFS_READ_FD_ARG);

    cnt = lua_tointegerx(L, _MROMFS_READ_COUNT_ARG, &isnum);
    if (!isnum || cnt < 0)
        luaL_argerror(L, _MROMFS_READ_COUNT_ARG, "Must be integer, >= 0.");

    data = luaL_buffinitsize(L, &lbuf, cnt);

    r = mromfsRead(fd, (uint8_t *)data, cnt);
    if (r < 0)
    {
        luaL_pushresultsize(&lbuf, 1);
        lua_pop(L, 1);
        lua_pushnil(L);
        return 1;
    }

    luaL_pushresultsize(&lbuf, r);
    return 1;
}
/*
 * ARGS
 *     1    File descriptor returned by "mromfs.open".
 * RETURN
 *     1    File size
 */
static int lmromfs_FileSize(lua_State *L)
{
    struct mromfs_fd_t *fd;

#define _MROMFS_FSIZE_FD_ARG      1
    if (!lua_isuserdata(L, _MROMFS_READ_FD_ARG))
        luaL_argerror(L, _MROMFS_READ_FD_ARG, "Not a file descriptor.");
    fd = lua_touserdata(L, _MROMFS_READ_FD_ARG);

    lua_pushinteger(L, fd->size);
    return 1;
}

