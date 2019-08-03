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
#ifndef _HTTP_H
#define _HTTP_H

#include "client.h"

int httpProcessRequest(struct client_t *client);

#define HTTP_101_SWITCHING_PROTOCOLS 101
#define HTTP_200_OK                  200
#define HTTP_204_NO_CONTENT          204
#define HTTP_301_MOVED_PERMANENTLY   301
#define HTTP_302_FOUND               302
#define HTTP_303_SEE_OTHER           303
#define HTTP_304_NOT_MODIFIED        304
#define HTTP_307_TEMPORARY_REDIRECT  307
#define HTTP_400_BAD_REQUEST         400
#define HTTP_403_FORBIDDEN           403
#define HTTP_404_NOT_FOUND           404
#define HTTP_405_METHOD_NOT_ALLOWED  405
#define HTTP_409_CONFLICT            409
#define HTTP_500_INTERNAL_SERVER_ERROR 500


#endif

