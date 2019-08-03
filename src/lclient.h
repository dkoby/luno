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
#ifndef _LCLIENT_H
#define _LCLIENT_H

#include <lua.h>
#include <stdint.h>
/* */
#include "client.h"

int lclientInit0(struct client_t *client);
int lclientInit1(struct client_t *client);
void lclientDestroy(struct client_t *client);
int lclientProcessRequest(struct client_t *client, int httpError);
void lclientSetRequestField(lua_State *L, char *field, char *value);
void lclientSetRequestFieldL(lua_State *L, char *field, char *value, int vlen);
void lclientAppendRequestField(lua_State *L, char *field, char *value);
void lclientAppendRequestFieldL(lua_State *L, char *field, char *value, int vlen);
void lclientSetRequestFieldNum(lua_State *L, char *field, int64_t num);
void lclientSetGlobalVal(lua_State *L, char *name, char *value);
void lclientSetGlobalValL(lua_State *L, char *name, char *value, int vlen);
enum lclient_compare_type_t {
    LSTATE_COMPARE_CASE,      /* Case-sensitive. */
    LSTATE_COMPARE_NOCASE,    /* Ignoring case. */
    LSTATE_COMPARE_LUA_EQUAL, /* Using lua_compare. */
};
int lclientCompareGlobalVals(lua_State *L,
        char *name1, char *name2, enum lclient_compare_type_t type);
void lclientHeadersAdd(lua_State *L);

#define LSTATE_GLOBAL_VAR_FIELD_NAME      "fieldName"
#define LSTATE_GLOBAL_VAR_FIELD_NAME_CMP  "fieldNameCmp"
#define LSTATE_GLOBAL_VAR_FIELD_VALUE     "fieldValue"
#define LSTATE_GLOBAL_VAR_FIELD_VALUE_CMP "fieldValueCmp"

#endif

