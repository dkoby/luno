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
#include <string.h>
/* */
#include "debug.h"
#include "server.h"
#include "version.h"

/*
 *
 */
static void main_printHead()
{
    debugPrint(DLEVEL_SYS, "luno v" VERSION_STRING
        " (c) 2016-2019 Dmitry Kobylin."
    );
    debugPrint(DLEVEL_SYS, "");
}
/*
 *
 */
static void main_printHelp()
{
    debugPrint(DLEVEL_SYS, "Usage:");
    debugPrint(DLEVEL_SYS, "    luno <OPTIONS>");
    debugPrint(DLEVEL_SYS, "");
    debugPrint(DLEVEL_SYS, "Options:");
    debugPrint(DLEVEL_SYS, "    -h          Print this help.");
    debugPrint(DLEVEL_SYS, "    -p<Port>    Bind to port. (Default is %d)", DEFAULT_PORT);
    debugPrint(DLEVEL_SYS, "    -r=<Dir>    Use external resource directory.");
#if 0
    debugPrint(DLEVEL_SYS, "    -C          Disable caching.");
#endif
    debugPrint(DLEVEL_SYS, "    -d<L>       Debug level (0, 1, 2, 3, 4), default is 3.");
    debugPrint(DLEVEL_SYS, "                    0    SILENT ");
    debugPrint(DLEVEL_SYS, "                    1    ERROR  ");
    debugPrint(DLEVEL_SYS, "                    2    WARNING");
    debugPrint(DLEVEL_SYS, "                    3    INFO   ");
    debugPrint(DLEVEL_SYS, "                    4    NOISE  ");
    debugPrint(DLEVEL_SYS, "");

    exit(1);
}
/*
 *
 */
int main(int argc, char **argv)
{
    char **arg;

    serverInit();
    main_printHead();

    arg = argv;
    /* Skip program name. */
    arg++;
    argc--;
    while (argc-- > 0)
    {
        if (strcmp("-h", *arg) == 0 || strcmp("--help", *arg) == 0)
        {
            main_printHelp();
        } else if (strlen(*arg) == 3 && strncmp("-d", *arg, 2) == 0) {
            char *c;
            c = *arg + 2;
            if (*c < '0' || *c > '4')
            {
                debugPrint(DLEVEL_ERROR, "Invalid value of \"-d\" option.");
                return 1;
            }
            debugLevel = *c - '0';
        } else if (strlen(*arg) >= 3 && strncmp("-p", *arg, 2) == 0) {
            char *c;
            c = *arg + 2;
            server.portNumber = atoi((const char*)c);
            if (server.portNumber <= 0)
            {
                debugPrint(DLEVEL_ERROR, "Invalid value of \"-p\" option.");
                return 1;
            }
#if 0
        } else if (strcmp("-C", *arg) == 0) {
            debugPrint(DLEVEL_INFO, "Caching is disabled");
            server.caching = 0;
#endif
        } else if (strlen(*arg) >= 3 && strncmp("-r=", *arg, 3) == 0) {
            server.resourceDir = *arg + 3;
            debugPrint(DLEVEL_INFO, "Using resources from: \"%s\"", server.resourceDir);
        } else {
            debugPrint(DLEVEL_ERROR, "Unknown option \"%s\".", *arg);
            return -1;
        }
        arg++;
    }

    return serverRun();
}

