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
#include <stdint.h>
#include <string.h>
/* */
#include "http.h"
/* */
#include "client.h"
#include "common.h"
#include "debug.h"
#include "lclient.h"
#include "token.h"

#if 0
    #define DEBUG_THIS
#endif

/* */
static int http_ParseRelPath(struct client_t *client);
static int http_ParseQuery(struct client_t *client);
static int http_ParseHeaders(struct client_t *client);
static int http_LexAnalyze(struct token_info_t *tinfo, char *ch, int type);
static int http_LexPostAnalyze(struct token_info_t *tinfo, int type);
static char *http_Strnstr(char *haystack, size_t haystacklen, char *needle, size_t needlelen);
enum {
    TOKEN_TYPE_CRLF,
    TOKEN_TYPE_SP,
    TOKEN_TYPE_AND,
    TOKEN_TYPE_EQUAL,
    TOKEN_TYPE_DOT,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_SEMICOLON,
#if 0
    TOKEN_TYPE_LWS,
#endif
    TOKEN_TYPE_ALPHA,
    TOKEN_TYPE_HIALPHA,
    TOKEN_TYPE_DIGIT,
    TOKEN_TYPE_FSLASH,
    TOKEN_TYPE_QUESTION,
    TOKEN_TYPE_PCHARX,
    TOKEN_TYPE_QUERY,
    TOKEN_TYPE_ESCAPE,
    TOKEN_TYPE_OWS,
    TOKEN_TYPE_TCHARS,
    TOKEN_TYPE_BOUNDARY,
    TOKEN_TYPE_ANY_NOT_CRLF,
};
#define TOKEN_GET(type) tokenGet(token, &tlen, &eof, http_LexAnalyze, http_LexPostAnalyze, type)

/*
 * Retrieve client request.
 * Fields of lua "request" filled in this call:
 *     method    "GET" or "POST"
 *     path      abs_path
 *     query     query
 *     qtable     query table, items a split of "query" by "&".
 *     ...
 *     TODO Full documentation.
 *
 * RETURN
 *     -1 if imposible to continue processing, connection must be dropped.
 *     HTTP_200_OK if request can be passed for futher processing.
 *     Other HTTP error code otherwise.
 */
int httpProcessRequest(struct client_t *client)
{
    struct token_t *token = &client->token;
    char *tval;
    int tlen;
    int eof;
    int error;

    error = -1;
    /* Drop any previous tokens. */
    tokenDrop(token);

    /*
     * :::: RFC 2616
     * :: generic-message = start-line
     * ::             *(message-header CRLF)
     * ::             CRLF
     * ::             [ message-body ]
     * :: start-line      = Request-Line | Status-Line
     * ::
     * ::
     * :: LWS            = [CRLF] 1*( SP | HT )
     */

    /* Drop any leading CRLF. */
    while (TOKEN_GET(TOKEN_TYPE_CRLF))
        tokenDrop(token);
    if (eof)
        goto error_eof;

    /*
     * :::: RFC 2616
     * :: Request       = Request-Line              ; Section 5.1
     * ::                 *(( general-header        ; Section 4.5
     * ::                  | request-header         ; Section 5.3
     * ::                  | entity-header ) CRLF)  ; Section 7.1
     * ::                 CRLF
     * ::                 [ message-body ]          ; Section 4.3    
     * ::
     * :: Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
     */

    /* Request-Line. */
    {
        /* Method. */
        {
            tval = TOKEN_GET(TOKEN_TYPE_HIALPHA);
            if (!tval)
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) No method token");
                goto error;
            }
            /* Supported methods. */
            if (
                strncmp(tval, "GET", tlen)  == 0 ||
                strncmp(tval, "POST", tlen) == 0 ||
                strncmp(tval, "OPTIONS", tlen) == 0
            ) {

            } else {
                DEBUG_CLIENT(DLEVEL_NOISE, "(E) Unknown method %.*s", tlen, tval);
                error = HTTP_400_BAD_REQUEST;
                /* 
                 * TODO
                 * Response with HTTP_405_METHOD_NOT_ALLOWED.
                 *
                 * :::: RFC 2616
                 * :: The response MUST include an
                 * :: Allow header containing a list of valid methods for the requested
                 * :: resource.
                 */
                goto error;
            }
            lclientSetRequestFieldL(client->luaState, "method", tval, tlen);
            tokenDrop(token);
        }
        /* SP. */
        {
            if (!TOKEN_GET(TOKEN_TYPE_SP))
            {
                /* Discard LWS (started with CRLF). */
                if (TOKEN_GET(TOKEN_TYPE_CRLF))
                {
                    error = HTTP_400_BAD_REQUEST;
                    goto error;
                }
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) No SP after \"Method\"");
                goto error;
            }
            tokenDrop(token);
        }

        /*
         * :::: RFC 2616
         * :: Request-URI    = "*" | absoluteURI | abs_path | authority
         *
         * No support for "*".
         * No support for authority.
         * No support for absoluteURI.
         *
         */

        /*
         * abs_path.
         *
         * :::: RFC 1808
         * :: abs_path    = "/"  rel_path
         */
        {
            if (!TOKEN_GET(TOKEN_TYPE_FSLASH))
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) No \"/\" character in \"abs_path\"");
                goto error;
            }
            tokenDrop(token);
            lclientSetRequestFieldL(client->luaState, "path", "/", 1);
            if (!http_ParseRelPath(client))
                goto error;
        }
        /* SP. */
        {
            if (!TOKEN_GET(TOKEN_TYPE_SP))
            {
                /* Discard LWS (started with CRLF). */
                if (TOKEN_GET(TOKEN_TYPE_CRLF))
                {
                    error = HTTP_400_BAD_REQUEST;
                    goto error;
                }
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) No SP after \"Request-URI\"");
                goto error;
            }
            tokenDrop(token);
        }
        /*
         * HTTP-Version.
         *
         * :::: RFC 2616
         * :: HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
         *
         * TODO read carefuly RFC 2145
         */
        {
            int versionError;
            int64_t major;
            int64_t minor;

            versionError = 1;
            do {
                /* "HTTP" */
                if (!(tval = TOKEN_GET(TOKEN_TYPE_HIALPHA)))
                    break;
                if (strncmp(tval, "HTTP", tlen) != 0)
                    break;
                tokenDrop(token);
                /* "/" */
                if (!(tval = TOKEN_GET(TOKEN_TYPE_FSLASH)))
                    break;
                tokenDrop(token);
                /* MAJOR */
                if (!(tval = TOKEN_GET(TOKEN_TYPE_DIGIT)))
                    break;
                if (commonString2Number(tval, tlen, &major) < 0)
                    break;
                tokenDrop(token);
                if (major != 1)
                {
                    /* TODO Proper handling. */
                    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Invalid MAJOR version of HTTP, must be 1");
                    break;
                }
                /* "." */
                if (!(tval = TOKEN_GET(TOKEN_TYPE_DOT)))
                    break;
                tokenDrop(token);
                /* MINOR */
                if (!(tval = TOKEN_GET(TOKEN_TYPE_DIGIT)))
                    break;
                if (commonString2Number(tval, tlen, &minor) < 0)
                    break;
                tokenDrop(token);
                if (minor != 1)
                {
                    /* TODO Proper handling. */
                    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Invalid MINOR version of HTTP, must be 1");
                    break;
                }
                versionError = 0;
            } while (0);
            if (versionError)
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Error in \"HTTP-Version\"");
                goto error;
            }
        }
        /*
         * CRLF
         */
        {
            if (!TOKEN_GET(TOKEN_TYPE_CRLF))
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) No CRLF at end of \"Request-Line\"");
                goto error;
            }
            tokenDrop(token);
        }
    }

    error = http_ParseHeaders(client);
    if (error < 0 || error != HTTP_200_OK)
        goto error;

#if 0
    /*
     * XXX Debug, disable this block.
     */
    return -1;
#endif
    return HTTP_200_OK;
error_eof:
    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(W) EOF");
error:
    return error;
}
/*
 * RETURN
 *     0 on error, 1 on success.
 */
static int http_ParseRelPath(struct client_t *client)
{
    struct token_t *token = &client->token;
    char *tval;
    int tlen;
    int eof;

    /*
     * :::: RFC 1808
     * :: rel_path    = [ path ] [ ";" params ] [ "?" query ]
     * ::
     * :: path        = fsegment *( "/" segment )
     * :: fsegment    = 1*pchar
     * :: segment     =  *pchar
     * ::
     * :: pchar       = uchar | ":" | "@" | "&" | "="
     * :: uchar       = unreserved | escape
     * :: unreserved  = alpha | digit | safe | extra
     * ::
     * :: escape      = "%" hex hex
     * :: hex         = digit | "A" | "B" | "C" | "D" | "E" | "F" |
     * ::                       "a" | "b" | "c" | "d" | "e" | "f"
     * ::
     * :: alpha       = lowalpha | hialpha
     * :: lowalpha    = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" |
     * ::               "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" |
     * ::               "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z"
     * :: hialpha     = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" |
     * ::               "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" |
     * ::               "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
     * ::
     * :: digit       = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" |
     * ::               "8" | "9"
     * ::
     * :: safe        = "$" | "-" | "_" | "." | "+"
     * :: extra       = "!" | "*" | "'" | "(" | ")" | ","
     * :: national    = "{" | "}" | "|" | "\" | "^" | "~" | "[" | "]" | "`"
     * :: reserved    = ";" | "/" | "?" | ":" | "@" | "&" | "="
     * :: punctuation = "<" | ">" | "#" | "%" | <">
     *
     * Does "query" have same rules as "path"?
     */

    /* 
     * XXX
     * Is "//.." path token allowed? Firefox 45.0 permit to user to enter
     * such request. Rules in RFC 1808 does not permit.
     */

    /* fsegment. */
    while (1) {
        if ((tval = TOKEN_GET(TOKEN_TYPE_PCHARX)))
        {
            lclientAppendRequestFieldL(client->luaState, "path", tval, tlen);
            tokenDrop(token);
        } else if ((tval = TOKEN_GET(TOKEN_TYPE_ESCAPE))) {
            lclientAppendRequestFieldL(client->luaState, "path", tval, tlen);
            tokenDrop(token);
        } else {
            break;
        }
    }
    /* :: *( "/" segment) */
    while (1)
    {
        /* "/" */
        if (!TOKEN_GET(TOKEN_TYPE_FSLASH))
            break;
        tokenDrop(token);
        lclientAppendRequestFieldL(client->luaState, "path", "/", 1);
        /*
         * segment.
         */
        while (1) {
            if ((tval = TOKEN_GET(TOKEN_TYPE_PCHARX)))
            {
                lclientAppendRequestFieldL(client->luaState, "path", tval, tlen);
                tokenDrop(token);
            } else if ((tval = TOKEN_GET(TOKEN_TYPE_ESCAPE))) {
                lclientAppendRequestFieldL(client->luaState, "path", tval, tlen);
                tokenDrop(token);
            } else {
                break;
            }
        }
    }
    /* No support for [";" params ] */
    return http_ParseQuery(client);
}
/*
 * RETURN
 *     0 on error, 1 on success.
 */
static int http_ParseQuery(struct client_t *client)
{
    struct token_t *token = &client->token;
    lua_State *L = client->luaState;
    char *tval;
    int tlen;
    int eof;

    lua_getglobal(L, "request");   /* [request]->TOS */
    lua_newtable(L);               /* [request][qtable]->TOS */
    lua_setfield(L, -2, "qtable"); /* [request]->TOS */
    lua_pop(L, 1);                 /* ->TOS */

    if (!TOKEN_GET(TOKEN_TYPE_QUESTION))
        return 1;
    tokenDrop(token);

    lclientSetRequestFieldL(client->luaState, "query", "?", 1);

    lua_newtable(L); /* [qtable]->TOS */

    while (1) {
        /*
         * Get name of parameter.
         */
        lua_pushstring(L, ""); /* [qtable][name]->TOS */
        while (1) {
            if ((tval = TOKEN_GET(TOKEN_TYPE_QUERY)))
            {
                lclientAppendRequestFieldL(client->luaState, "query", tval, tlen);
                lua_pushlstring(L, tval, tlen); /* [qtable][name][name1]->TOS */
                lua_concat(L, 2);               /* [qtable][name..name1]->TOS */
                tokenDrop(token);
            } else if ((tval = TOKEN_GET(TOKEN_TYPE_ESCAPE))) {
                lclientAppendRequestFieldL(client->luaState, "query", tval, tlen);
                lua_pushlstring(L, tval, tlen);  /* [qtable][name][name1]->TOS */
                lua_concat(L, 2);                 /* [qtable][name..name1]->TOS */
                tokenDrop(token);
            } else {
                break;
            }
        }
        lua_pushstring(L, ""); /* [qtable][name][value]->TOS*/
        /* = */
        if ((tval = TOKEN_GET(TOKEN_TYPE_EQUAL)))
        {
            lclientAppendRequestFieldL(client->luaState, "query", tval, tlen);
            tokenDrop(token);
            /*
             * Get value of parameter.
             */
            while (1) {
                if ((tval = TOKEN_GET(TOKEN_TYPE_QUERY)))
                {
                    lclientAppendRequestFieldL(client->luaState, "query", tval, tlen);
                    lua_pushlstring(L, tval, tlen);  /* [qtable][name][value][value1]->TOS */
                    lua_concat(L, 2);                 /* [qtable][name][value .. value1]->TOS */
                    tokenDrop(token);
                } else if ((tval = TOKEN_GET(TOKEN_TYPE_ESCAPE))) {
                    lclientAppendRequestFieldL(client->luaState, "query", tval, tlen);
                    lua_pushlstring(L, tval, tlen);  /* [qtable][name][value][value1]->TOS */
                    lua_concat(L, 2);                 /* [qtable][name][value .. value1]->TOS */
                    tokenDrop(token);
                } else {
                    break;
                }
            }
            /* Set only non-empty name values, drop value and name otherwise. */
            if (lua_rawlen(L, -2))
                lua_settable(L, -3); /* [qtable]->TOS */
            else
                lua_pop(L, 2); /* [qtable]->TOS */
        } else if ((tval = TOKEN_GET(TOKEN_TYPE_AND))) {
            lclientAppendRequestFieldL(client->luaState, "query", tval, tlen);
            tokenDrop(token);
            /* Set only non-empty name values, drop value and name otherwise. */
            if (lua_rawlen(L, -2))
                lua_settable(L, -3); /* [qtable]->TOS */
            else
                lua_pop(L, 2); /* [qtable]->TOS */
        } else {
            /* Set only non-empty name values, drop value and name otherwise. */
            if (lua_rawlen(L, -2))
                lua_settable(L, -3); /* [qtable]->TOS */
            else
                lua_pop(L, 2); /* [qtable]->TOS */
            break;
        }
    }

    {
        lua_getglobal(L, "request"); /* [qtable][request]->TOS */
#if 1
        lua_insert(L, -2);           /* [request][qtable]->TOS */
        lua_pushstring(L, "qtable");  /* [request][qtable]["qtable"]->TOS */
        lua_insert(L, -2);           /* [request]["qtable"][qtable]->TOS */
#else
        lua_pushstring(L, "qtable");               /* [qtable][request]["qtable"]->TOS */
        lua_rotate(L, -3 /* idx */, -1 /* n */);  /* [request]["qtable"][qtable]->TOS */
#endif
        lua_settable(L, -3);                      /* [request]->TOS */
        lua_pop(L, 1);                            /* ->TOS */
    }

#if (0 && (defined DEBUG_THIS))
        /* Stack MUST be empty (gettop return 0). */
        debugPrint(DLEVEL_NOISE, "%s stack \"%d\" [sock %d].",
                __FUNCTION__, lua_gettop(L), client->sock);
#endif
    return 1;
}
/*
 *
 * RETURN
 *     see clientreqRead()
 */
static int _parseHeaderValue(struct client_t *client);

static int http_ParseHeaders(struct client_t *client)
{
    struct token_t *token = &client->token;
    int error             = -1;
    char *tval;
    int tlen;
    int eof;
    /*
     * :::: RFC 7230
     * :: Each header field consists of a case-insensitive field name followed
     * :: by a colon (":"), optional leading whitespace, the field value, and
     * :: optional trailing whitespace.    
     *
     * :: header-field   = field-name ":" OWS field-value OWS
     * :: field-name     = token
     * :: field-value    = *( field-content / obs-fold )
     * :: field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
     * :: field-vchar    = VCHAR / obs-text
     * ::
     * :: obs-fold       = CRLF 1*( SP / HTAB )
     * ::                ; obsolete line folding
     * ::                ; see Section 3.2.4
     * ::
     * :: BWS = OWS             ; "bad" whitespace
     * :: OWS = *( SP / HTAB )  ; optional whitespace
     * :: RWS = 1*( SP / HTAB ) ; required whitespace
     * ::
     * :: No whitespace is allowed between the header field-name and colon.  In
     * :: the past, differences in the handling of such whitespace have led to
     * :: security vulnerabilities in request routing and response handling.  A
     * :: server MUST reject any received request message that contains
     * :: whitespace between a header field-name and colon with a response code
     * :: of 400 (Bad Request).  A proxy MUST remove any such whitespace from a
     * :: response message before forwarding the message downstream.
     * ::
     * :: A field value might be preceded and/or followed by optional
     * :: whitespace (OWS); a single SP preceding the field-value is preferred
     * :: for consistent readability by humans.  The field value does not
     * :: include any leading or trailing whitespace: OWS occurring before the
     * :: first non-whitespace octet of the field value or after the last
     * :: non-whitespace octet of the field value ought to be excluded by
     * :: parsers when extracting the field value from a header field.
     * ::
     * :: Historically, HTTP header field values could be extended over
     * :: multiple lines by preceding each extra line with at least one space
     * :: or horizontal tab (obs-fold).  This specification deprecates such
     * :: line folding except within the message/http media type
     * :: (Section 8.3.1).  A sender MUST NOT generate a message that includes
     * :: line folding (i.e., that has any field-value that contains a match to
     * :: the obs-fold rule) unless the message is intended for packaging
     * :: within the message/http media type.
     * ::
     * :: A server that receives an obs-fold in a request message that is not
     * :: within a message/http container MUST either reject the message by
     * :: sending a 400 (Bad Request), preferably with a representation
     * :: explaining that obsolete line folding is unacceptable, or replace
     * :: each received obs-fold with one or more SP octets prior to
     * :: interpreting the field value or forwarding the message downstream.
     * ::
     * ::
     * :: Most HTTP header field values are defined using common syntax
     * :: components (token, quoted-string, and comment) separated by
     * :: whitespace or specific delimiting characters.  Delimiters are chosen
     * :: from the set of US-ASCII visual characters not allowed in a token
     * :: (DQUOTE and "(),/:;<=>?@[\]{}").
     * :: 
     * ::   token          = 1*tchar
     * :: 
     * ::   tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
     * ::                  / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
     * ::                  / DIGIT / ALPHA
     * ::                  ; any VCHAR, except delimiters
     * :: 
     * :: A string of text is parsed as a single value if it is quoted using
     * :: double-quote marks.
     * :: 
     * ::   quoted-string  = DQUOTE *( qdtext / quoted-pair ) DQUOTE
     * ::   qdtext         = HTAB / SP /%x21 / %x23-5B / %x5D-7E / obs-text
     * ::   obs-text       = %x80-FF
     * :: 
     * :: Comments can be included in some HTTP header fields by surrounding
     * :: the comment text with parentheses.  Comments are only allowed in
     * :: fields containing "comment" as part of their field value definition.
     * :: 
     * ::   comment        = "(" *( ctext / quoted-pair / comment ) ")"
     * ::   ctext          = HTAB / SP / %x21-27 / %x2A-5B / %x5D-7E / obs-text
     * :: 
     * :: The backslash octet ("\") can be used as a single-octet quoting
     * :: mechanism within quoted-string and comment constructs.  Recipients
     * :: that process the value of a quoted-string MUST handle a quoted-pair
     * :: as if it were replaced by the octet following the backslash.
     * :: 
     * ::   quoted-pair    = "\" ( HTAB / SP / VCHAR / obs-text )
     * :: 
     * :: A sender SHOULD NOT generate a quoted-pair in a quoted-string except
     * :: where necessary to quote DQUOTE and backslash octets occurring within
     * :: that string.  A sender SHOULD NOT generate a quoted-pair in a comment
     * :: except where necessary to quote parentheses ["(" and ")"] and
     * :: backslash octets occurring within that comment.
     * ::
     * ::
     * :: HTTP does not place a predefined limit on the length of each header
     * :: field or on the length of the header section as a whole, as described
     * :: in Section 2.5.  Various ad hoc limitations on individual header
     * :: field length are found in practice, often depending on the specific
     * :: field semantics.
     * ::
     * :: A server that receives a request header field, or set of fields,
     * :: larger than it wishes to process MUST respond with an appropriate 4xx
     * :: (Client Error) status code.  Ignoring such header fields would
     * :: increase the server's vulnerability to request smuggling attacks
     * :: (Section 9.5).
     *
     * NOTE
     * Are VCHAR characters lying in range 0x21 - 0x7E? Look at RFC 5234.
     * Except (DQUOTE and "(),/:;<=>?@[\]{}")
     *
     * :::: RFC 2616
     * :: Request       = Request-Line              ; Section 5.1
     * ::                 *(( general-header        ; Section 4.5
     * ::                  | request-header         ; Section 5.3
     * ::                  | entity-header ) CRLF)  ; Section 7.1
     * ::
     * :: general-header = Cache-Control     ; Section 14.9
     * ::         | Connection               ; Section 14.10
     * ::         | Date                     ; Section 14.18
     * ::         | Pragma                   ; Section 14.32
     * ::         | Trailer                  ; Section 14.40
     * ::         | Transfer-Encoding        ; Section 14.41
     * ::         | Upgrade                  ; Section 14.42
     * ::         | Via                      ; Section 14.45
     * ::         | Warning                  ; Section 14.46
     * ::
     * ::
     * :: request-header = Accept                   ; Section 14.1
     * ::                | Accept-Charset           ; Section 14.2
     * ::                | Accept-Encoding          ; Section 14.3
     * ::                | Accept-Language          ; Section 14.4
     * ::                | Authorization            ; Section 14.8
     * ::                | Expect                   ; Section 14.20
     * ::                | From                     ; Section 14.22
     * ::                | Host                     ; Section 14.23
     * ::                | If-Match                 ; Section 14.24
     * ::                | If-Modified-Since        ; Section 14.25
     * ::                | If-None-Match            ; Section 14.26
     * ::                | If-Range                 ; Section 14.27
     * ::                | If-Unmodified-Since      ; Section 14.28
     * ::                | Max-Forwards             ; Section 14.31
     * ::                | Proxy-Authorization      ; Section 14.34
     * ::                | Range                    ; Section 14.35
     * ::                | Referer                  ; Section 14.36
     * ::                | TE                       ; Section 14.39
     * ::                | User-Agent               ; Section 14.43
     * ::
     * ::
     * :: entity-header  = Allow                    ; Section 14.7
     * ::                | Content-Encoding         ; Section 14.11
     * ::                | Content-Language         ; Section 14.12
     * ::                | Content-Length           ; Section 14.13
     * ::                | Content-Location         ; Section 14.14
     * ::                | Content-MD5              ; Section 14.15
     * ::                | Content-Range            ; Section 14.16
     * ::                | Content-Type             ; Section 14.17
     * ::                | Expires                  ; Section 14.21
     * ::                | Last-Modified            ; Section 14.29
     * ::                | extension-header
     * ::
     * :: extension-header = message-header
     * ::
     * :: The extension-header mechanism allows additional entity-header fields
     * :: to be defined without changing the protocol, but these fields cannot
     * :: be assumed to be recognizable by the recipient. Unrecognized header
     * :: fields SHOULD be ignored by the recipient and MUST be forwarded by
     * :: transparent proxies.
     */

    /* Get header fields. */
    while (1)
    {
        /*
         * Field-name.
         */
        if (!(tval = TOKEN_GET(TOKEN_TYPE_TCHARS)))
        {
            DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) \"Field-name\" missing");
            error = HTTP_400_BAD_REQUEST;
            goto error;
        }
#if (1 && (defined DEBUG_THIS))
        DEBUG_CLIENT(DLEVEL_NOISE, "Field-name: %.*s", tlen, tval);
#endif
        lclientSetGlobalValL(client->luaState, LSTATE_GLOBAL_VAR_FIELD_NAME, tval, tlen);
        tokenDrop(token);

        /* Colon. */
        if (!TOKEN_GET(TOKEN_TYPE_COLON))
        {
            DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Colon missing after \"Field-name\"");
            error = HTTP_400_BAD_REQUEST;
            goto error;
        }
        tokenDrop(token);
        /* OWS. */
        if (TOKEN_GET(TOKEN_TYPE_OWS))
            tokenDrop(token);

        /* field-value. */
        error = _parseHeaderValue(client);
        if (error < 0 || error != HTTP_200_OK)
        {
            error = HTTP_400_BAD_REQUEST;
            goto error;
        }

        /* OWS. */
        if (TOKEN_GET(TOKEN_TYPE_OWS))
            tokenDrop(token);
        /* CRLF. */
        if (!TOKEN_GET(TOKEN_TYPE_CRLF))
        {
            DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) CRLF missing at end of field-value");
            error = HTTP_400_BAD_REQUEST;
            goto error;
        }
        tokenDrop(token);
        /* End of headers. */
        if (TOKEN_GET(TOKEN_TYPE_CRLF))
        {
            tokenDrop(token);
            break;
        }
    }

    return HTTP_200_OK;
error:
    return error;
}
/*
 *
 */
static int _parseHeaderValue(struct client_t *client)
{
    struct token_t *token = &client->token;
    char *tval;
    int tlen;
    int eof;

#define _FNAME       LSTATE_GLOBAL_VAR_FIELD_NAME
#define _FNAME_CMP   LSTATE_GLOBAL_VAR_FIELD_NAME_CMP
#define _FVALUE      LSTATE_GLOBAL_VAR_FIELD_VALUE
#define _FVALUE_CMP  LSTATE_GLOBAL_VAR_FIELD_VALUE_CMP

#define _CMP_CASE    LSTATE_COMPARE_CASE
#define _CMP_NO_CASE LSTATE_COMPARE_NOCASE
#define _CMP_LUA     LSTATE_COMPARE_LUA_EQUAL

    do {
        lclientSetGlobalVal(client->luaState, _FNAME_CMP, "Connection");
        if (lclientCompareGlobalVals(client->luaState, _FNAME, _FNAME_CMP, _CMP_NO_CASE))
        {
            /*
             * :::: RFC 7230
             * ::   Connection        = 1#connection-option
             * ::   connection-option = token
             * ::
             * :: Connection options are case-insensitive.
             *
             * Possible values are "close", "keep-alive".
             */

            /* 
             * TODO
             * Valid parsing of field-values.
             * At this time only simple TOKEN_TYPE_ANY_NOT_CRLF parser used.
             */
            if (!(tval = TOKEN_GET(TOKEN_TYPE_ANY_NOT_CRLF)))
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Empty \"Connection\" value");
                return HTTP_400_BAD_REQUEST;
            }
#define _KEEP_ALIVE "keep-alive"
            if (http_Strnstr(tval, tlen, _KEEP_ALIVE, strlen(_KEEP_ALIVE)) != NULL)
            {
                client->request.keepAlive = 1;
                lclientSetRequestField(client->luaState, "keepAlive", "true");
            }
            tokenDrop(token);
            break;
        }
        lclientSetGlobalVal(client->luaState, _FNAME_CMP, "Content-Length");
        if (lclientCompareGlobalVals(client->luaState, _FNAME, _FNAME_CMP, _CMP_NO_CASE))
        {
            int64_t contentLength;
            /*
             * :::: RFC 2616
             * :: Content-Length    = "Content-Length" ":" 1*DIGIT
             */
            if (!(tval = TOKEN_GET(TOKEN_TYPE_DIGIT)))
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Empty \"Content-Length\" value");
                return HTTP_400_BAD_REQUEST;
            }
            if (commonString2Number(tval, tlen, &contentLength) < 0)
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Invalid \"Content-Length\" value");
                return HTTP_400_BAD_REQUEST;
            }
            tokenDrop(token);
            lclientSetRequestFieldNum(client->luaState, "contentLength", contentLength);
            break;
        }
        lclientSetGlobalVal(client->luaState, _FNAME_CMP, "Content-Type");
        if (lclientCompareGlobalVals(client->luaState, _FNAME, _FNAME_CMP, _CMP_NO_CASE))
        {
            /*
             * Content-Type value.
             */
            if (!(tval = TOKEN_GET(TOKEN_TYPE_TCHARS)))
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) \"Content-Type\" value 1 missing");
                return HTTP_400_BAD_REQUEST;
            }
            lclientSetRequestFieldL(client->luaState, "contentType", tval, tlen);
            if (strncasecmp(tval, "multipart", tlen) == 0)
            {
                tokenDrop(token);
                if (!TOKEN_GET(TOKEN_TYPE_FSLASH))
                {
                    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) No \"/\" character in \"Content-Type\" separator");
                    return HTTP_400_BAD_REQUEST;
                }
                lclientAppendRequestField(client->luaState, "contentType", "/");
                tokenDrop(token);
                if (!(tval = TOKEN_GET(TOKEN_TYPE_TCHARS)))
                {
                    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) \"Content-Type\" value 2 missing");
                    return HTTP_400_BAD_REQUEST;
                }
                if (strncasecmp(tval, "form-data", tlen) == 0)
                {
                    lclientAppendRequestFieldL(client->luaState, "contentType", tval, tlen);
                    tokenDrop(token);
                    if (!TOKEN_GET(TOKEN_TYPE_SEMICOLON))
                    {
                        DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) \";\" missing after multipart/form-data");
                        return HTTP_400_BAD_REQUEST;
                    }
                    tokenDrop(token);
                    if (TOKEN_GET(TOKEN_TYPE_OWS))
                        tokenDrop(token);
                    if (!(tval = TOKEN_GET(TOKEN_TYPE_TCHARS)) || strncmp(tval, "boundary", tlen) != 0)
                    {
                        DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) \"boundary\" missing for multipart/form-data");
                        return HTTP_400_BAD_REQUEST;
                    }
                    tokenDrop(token);
                    if (!TOKEN_GET(TOKEN_TYPE_EQUAL))
                    {
                        DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) \"=\" missing after boundary");
                        return HTTP_400_BAD_REQUEST;
                    }
                    tokenDrop(token);
                    /*
                     * boundary := 0*69<bchars> bcharsnospace
                     *
                     * bchars := bcharsnospace / " "
                     *
                     * bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" /
                     *                  "+" / "_" / "," / "-" / "." /
                     *                  "/" / ":" / "=" / "?"
                     */
                    if (!(tval = TOKEN_GET(TOKEN_TYPE_BOUNDARY)))
                    {
                        DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) invalid \"boundary\"");
                        return HTTP_400_BAD_REQUEST;
                    }
                    lclientSetRequestFieldL(client->luaState, "boundary", tval, tlen);
                    tokenDrop(token);
                    break;
                }
                break;
            } else {
                tokenDrop(token);
                if (!TOKEN_GET(TOKEN_TYPE_FSLASH))
                {
                    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) No \"/\" character in \"Content-Type\" separator");
                    return HTTP_400_BAD_REQUEST;
                }
                lclientAppendRequestField(client->luaState, "contentType", "/");
                tokenDrop(token);
                if (!(tval = TOKEN_GET(TOKEN_TYPE_TCHARS)))
                {
                    DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) \"Content-Type\" value 2 missing");
                    return HTTP_400_BAD_REQUEST;
                }
                lclientAppendRequestFieldL(client->luaState, "contentType", tval, tlen);
                tokenDrop(token);
                /* Skip any trailing data */
                if (TOKEN_GET(TOKEN_TYPE_ANY_NOT_CRLF))
                    tokenDrop(token);
                break;
            }
            tokenDrop(token);
        }
#if 0
        lclientSetGlobalVal(client->luaState, _FNAME_CMP, "Host");
        if (lclientCompareGlobalVals(client->luaState, _FNAME, _FNAME_CMP, _CMP_NO_CASE))
        {
            /*
             * :::: RFC 7230
             * ::   Host = "Host" ":" host [ ":" port ] ; Section 3.2.2
             * ::
             */

            /* 
             * TODO
             * Valid parsing of field-values.
             * At this time only simple TOKEN_TYPE_ANY_NOT_CRLF parser used.
             */
            if (!(tval = TOKEN_GET(TOKEN_TYPE_ANY_NOT_CRLF)))
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Empty \"Host\" value");
                return HTTP_400_BAD_REQUEST;
            }
            lclientSetRequestFieldL(client->luaState, "host", tval, tlen);
            tokenDrop(token);
            break;
        }
#endif
#if 0
        lclientSetGlobalVal(client->luaState, _FNAME_CMP, "Origin");
        if (lclientCompareGlobalVals(client->luaState, _FNAME, _FNAME_CMP, _CMP_NO_CASE))
        {
            /* 
             * TODO
             * Valid parsing of field-values.
             * At this time only simple TOKEN_TYPE_ANY_NOT_CRLF parser used.
             */
            if (!(tval = TOKEN_GET(TOKEN_TYPE_ANY_NOT_CRLF)))
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Empty \"Origin\" value");
                return HTTP_400_BAD_REQUEST;
            }
            lclientSetRequestFieldL(client->luaState, "origin", tval, tlen);
            tokenDrop(token);
            break;
        }
        lclientSetGlobalVal(client->luaState, _FNAME_CMP, "Access-Control-Request-Method");
        if (lclientCompareGlobalVals(client->luaState, _FNAME, _FNAME_CMP, _CMP_NO_CASE))
        {
            /* 
             * TODO
             * Valid parsing of field-values.
             * At this time only simple TOKEN_TYPE_ANY_NOT_CRLF parser used.
             */
            if (!(tval = TOKEN_GET(TOKEN_TYPE_ANY_NOT_CRLF)))
            {
                DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Empty \"Origin\" value");
                return HTTP_400_BAD_REQUEST;
            }
            lclientSetRequestFieldL(client->luaState, "accessControlRequestMethod", tval, tlen);
            tokenDrop(token);
            break;
        }
        /*
         * Skip values for any unknown fields.
         */
        if (!(tval = TOKEN_GET(TOKEN_TYPE_ANY_NOT_CRLF)))
        {
            DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Empty value");
            return HTTP_400_BAD_REQUEST;
        }
#else
        if (!(tval = TOKEN_GET(TOKEN_TYPE_ANY_NOT_CRLF)))
        {
            DEBUG_CLIENT(DLEVEL_NOISE, "%s", "(E) Empty value");
            return HTTP_400_BAD_REQUEST;
        }
        lclientSetGlobalValL(client->luaState, _FVALUE, tval, tlen);
        lclientHeadersAdd(client->luaState);
        tokenDrop(token);
#endif

    } while (0);
    return HTTP_200_OK;
}
/*
 *
 * ARGS
 *     tinfo    Pointer to token info structure.
 *     ch       Input character.
 *     type     Type of token to get.
 * RETURN
 *     Return 1 if token is complete or does not appear in input stream,
 *     zero if waiting for other characters.
 */
static int http_LexAnalyze(struct token_info_t *tinfo, char *ch, int type)
{
    if (ch == NULL) /* EOF */
        return 1;

#define _LOWHEX      (*ch >= 'A' && *ch <= 'F')
#define _HIHEX       (*ch >= 'a' && *ch <= 'f')
#define _LOWALPHA    (*ch >= 'a' && *ch <= 'z')
#define _HIALPHA     (*ch >= 'A' && *ch <= 'Z')
#define _DIGIT       (*ch >= '0' && *ch <= '9')
#define _SAFE        (*ch == '$' || *ch == '-' || *ch == '_' || *ch == '.' || *ch == '+')
#define _EXTRA       (*ch == '!' || *ch == '*' || *ch == '\''|| *ch == '(' || *ch == ')' || *ch == ',')
#define _HEX         (_DIGIT || _HIHEX || _LOWHEX)

#define _ALPHA       (_LOWALPHA || _HIALPHA)
#define _UNRESERVED  (_ALPHA || _DIGIT || _SAFE || _EXTRA)
#define _UCHARX      (_UNRESERVED /* NOTE no "escape" token */)
#define _AND         (*ch == '&')
#define _EQUAL       (*ch == '=')
#define _PCHAR_      (_UCHARX || (*ch == ':' || *ch == '@'))
#define _PCHARX      (_PCHAR_ || _AND || _EQUAL)
#define _QUERY       (_PCHAR_)

#define _DELIMETERS ( \
        *ch == '(' || *ch == ')' || *ch == ',' || *ch == '/' || \
        *ch == ':' || *ch == ';' || *ch == '<' || *ch == '=' || \
        *ch == '>' || *ch == '?' || *ch == '@' || *ch == '[' || \
        *ch == '\\'|| *ch == ']' || *ch == '{' || *ch == '}'    \
)
#define _VCHAR_EXCEPT (_DELIMETERS)
#define _VCHAR_RANGE  (*ch >= 0x21 && *ch <= 0x7E)
#define _VCHAR        (_VCHAR_RANGE && !_VCHAR_EXCEPT)

#define _BOUNDARY ( \
        _DIGIT || _ALPHA || \
        *ch == '\''|| *ch == '(' || *ch == ')' || *ch == '.' || \
        *ch == '+' || *ch == '_' || *ch == ',' || *ch == '-' || \
        *ch == '/' || *ch == ':' || *ch == '=' || *ch == '?'    \
)

    if (tinfo->lenp == 0)
    {
        /*
         * For tokens with one character.
         */
        char cmp; /* Character to compare with. */
        cmp = 0; /* NOTE No possible to compare 0-terminator. */
        switch (type)
        {
            case TOKEN_TYPE_SP:        cmp = ' '; break; 
            case TOKEN_TYPE_FSLASH:    cmp = '/'; break; 
            case TOKEN_TYPE_QUESTION:  cmp = '?'; break; 
            case TOKEN_TYPE_AND:       cmp = '&'; break; 
            case TOKEN_TYPE_EQUAL:     cmp = '='; break; 
            case TOKEN_TYPE_DOT:       cmp = '.'; break; 
            case TOKEN_TYPE_COLON:     cmp = ':'; break; 
            case TOKEN_TYPE_SEMICOLON: cmp = ';'; break; 
        }
        if (cmp)
        {
            if (*ch == cmp)
            {
                tinfo->lenw++;
                tinfo->lenp++;
                return 1; /* Complete. */
            } else {
                return 1; /* Unexpected character. */
            }
        }
        /*
         * For tokens with more then one character.
         */
        switch (type)
        {
            case TOKEN_TYPE_CRLF:
                if (*ch == '\r')
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
#if 0
            case TOKEN_TYPE_LWS:
                if (*ch == '\r' || *ch == ' ' || *ch == '\t')
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
#endif
            case TOKEN_TYPE_ALPHA:
                if (_ALPHA)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_HIALPHA:
                if (_HIALPHA)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_DIGIT:
                if (_DIGIT)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_PCHARX:
                if (_PCHARX)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_QUERY:
                if (_QUERY)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_ESCAPE:
                if (*ch == '%')
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_OWS:
                if (*ch == ' ' || *ch == '\t')
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_TCHARS:
                if (_VCHAR)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_BOUNDARY:
                if (_BOUNDARY)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_ANY_NOT_CRLF:
                if (*ch != '\r')
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
        }
    } else {
        switch (type)
        {
            case TOKEN_TYPE_CRLF:
                if (*ch == '\n')
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 1; /* Complete. */
                }
                break;
#if 0
             case TOKEN_TYPE_LWS:
                if (tinfo->lenp == 1 && tinfo->p[0] == '\r')
                {
                    if (*ch == '\n')
                    {
                        tinfo->lenw++;
                        tinfo->lenp++;
                        return 0; /* Wait more. */
                    } else {
                        tinfo->lenw++;
                        tinfo->lenp++;
                        return 1; /* Unexpected character. */
                    }
                } else {
                    if (*ch == ' ' || *ch == '\t')
                    {
                        tinfo->lenw++;
                        tinfo->lenp++;
                        return 0; /* Wait more. */
                    }
                }
                break;
#endif
            case TOKEN_TYPE_ALPHA:
                if (_ALPHA)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_HIALPHA:
                if (_HIALPHA)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_DIGIT:
                if (_DIGIT)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_PCHARX:
                if (_PCHARX)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_QUERY:
                if (_QUERY)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_ESCAPE:
                if (_HEX && tinfo->lenp < 3 /* %xx */)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_OWS:
                if (*ch == ' ' || *ch == '\t')
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_TCHARS:
                if (_VCHAR)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_BOUNDARY:
                if (_BOUNDARY)
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
            case TOKEN_TYPE_ANY_NOT_CRLF:
                if (*ch != '\r')
                {
                    tinfo->lenw++;
                    tinfo->lenp++;
                    return 0; /* Wait more. */
                }
                break;
        }
    }
    return 1; /* Unexpected character or done. */
}
/*
 * Called only if token have payload length greater then 1. 
 *
 * RETURN
 *     If token is valid return value of 1, 0 otherwise.
 */
static int http_LexPostAnalyze(struct token_info_t *tinfo, int type)
{
    switch (type)
    {
        case TOKEN_TYPE_CRLF:
            if (tinfo->lenp == 2)
                return 1;
            break;
#if 0
        case TOKEN_TYPE_LWS:
            if (tinfo->p[0] == '\r' && tinfo->lenp < 2)
                return 0;
            else
                return 1;
            break;
#endif
        case TOKEN_TYPE_ESCAPE:
            if (tinfo->lenp == 3)
                return 1;
            break;
#if 1
        case TOKEN_TYPE_SP:
        case TOKEN_TYPE_FSLASH:
        case TOKEN_TYPE_QUESTION:
        case TOKEN_TYPE_AND:
        case TOKEN_TYPE_EQUAL:
        case TOKEN_TYPE_DOT:
        case TOKEN_TYPE_COLON:
        case TOKEN_TYPE_SEMICOLON:
            if (tinfo->lenp == 1)
                return 1;
            break;
        case TOKEN_TYPE_ALPHA:
        case TOKEN_TYPE_HIALPHA:
        case TOKEN_TYPE_DIGIT:
        case TOKEN_TYPE_PCHARX:
        case TOKEN_TYPE_QUERY:
        case TOKEN_TYPE_OWS:
        case TOKEN_TYPE_TCHARS:
        case TOKEN_TYPE_BOUNDARY:
        case TOKEN_TYPE_ANY_NOT_CRLF:
            return 1;
#else
        default:
            return 1;
#endif
    }

    return 0;
}
/*
 *
 */
static char *http_Strnstr(char *haystack, size_t haystacklen, char *needle, size_t needlelen)
{
    char *phaystack;

    if (haystack == NULL || needle == NULL || haystacklen == 0 || needlelen == 0)
        return NULL;

    phaystack = haystack;
    while (haystacklen >= needlelen)
    {
        if (*phaystack == *needle)
        {
            char *pneedle;
            char *pmatched;
            size_t nchars;

            pmatched = phaystack;
            pneedle  = needle;
            nchars   = needlelen;
            while (*phaystack == *pneedle)
            {
                if (--nchars == 0)
                    return pmatched;
                phaystack++;
                pneedle++;
            }
            phaystack = pmatched;
        }

        phaystack++;
        haystacklen--;
    }

    return NULL;
}

