/*
 * Copyright ¬ 2004-2007 Diego Nehab
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE. 
 */
 
#include <stdio.h>
#include <string.h> 

#include "socket.h"

#define _SOCKET_INCLUDE_C

#ifdef _WIN32
#include "wsocket.c"
#else
#include "usocket.c"
#endif

#undef _SOCKET_INCLUDE_C

/*-------------------------------------------------------------------------*\
* I/O error strings
\*-------------------------------------------------------------------------*/
const char *io_strerror(int err) {
    switch (err) {
        case IO_DONE: return NULL;
        case IO_CLOSED: return "closed";
        case IO_TIMEOUT: return "timeout";
        default: return "unknown error"; 
    }
}

/*-------------------------------------------------------------------------*\
* Tries to create a new inet socket
\*-------------------------------------------------------------------------*/
const char *inet_trycreate(p_socket ps, int type) {
    return socket_strerror(socket_create(ps, AF_INET, type, 0));
}

/*-------------------------------------------------------------------------*\
* Tries to connect to remote address (address, port)
\*-------------------------------------------------------------------------*/
const char *inet_tryconnect(p_socket ps, const char *address, 
                            unsigned short port, p_timeout tm)
{
    struct sockaddr_in remote;
    int err;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    if (strcmp(address, "*")) {
        if (!inet_aton(address, &remote.sin_addr)) {
            struct hostent *hp = NULL;
            struct in_addr **addr;
            err = socket_gethostbyname(address, &hp);
            if (err != IO_DONE) return socket_hoststrerror(err);
            addr = (struct in_addr **) hp->h_addr_list;
            memcpy(&remote.sin_addr, *addr, sizeof(struct in_addr));
        }
    } else remote.sin_family = AF_UNSPEC;
    err = socket_connect(ps, (SA *) &remote, sizeof(remote), tm);
    return socket_strerror(err);
}

/*-------------------------------------------------------------------------*\
* Tries to bind socket to (address, port)
\*-------------------------------------------------------------------------*/
const char *inet_trybind(p_socket ps, const char *address, unsigned short port)
{
    struct sockaddr_in local;
    int err;
    memset(&local, 0, sizeof(local));
    /* address is either wildcard or a valid ip address */
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    local.sin_family = AF_INET;
    if (strcmp(address, "*") && !inet_aton(address, &local.sin_addr)) {
        struct hostent *hp = NULL;
        struct in_addr **addr;
        err = socket_gethostbyname(address, &hp);
        if (err != IO_DONE) return socket_hoststrerror(err);
        addr = (struct in_addr **) hp->h_addr_list;
        memcpy(&local.sin_addr, *addr, sizeof(struct in_addr));
    }
    err = socket_bind(ps, (SA *) &local, sizeof(local));
    if (err != IO_DONE) socket_destroy(ps);
    return socket_strerror(err); 
}

/*-------------------------------------------------------------------------*\
* Some systems do not provide this so that we provide our own. It's not
* marvelously fast, but it works just fine.
\*-------------------------------------------------------------------------*/
#ifdef INET_ATON
int inet_aton(const char *cp, struct in_addr *inp)
{
    unsigned int a = 0, b = 0, c = 0, d = 0;
    int n = 0, r;
    unsigned long int addr = 0;
    r = sscanf(cp, "%u.%u.%u.%u%n", &a, &b, &c, &d, &n);
    if (r == 0 || n == 0) return 0;
    cp += n;
    if (*cp) return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255) return 0;
    if (inp) {
        addr += a; addr <<= 8;
        addr += b; addr <<= 8;
        addr += c; addr <<= 8;
        addr += d;
        inp->s_addr = htonl(addr);
    }
    return 1;
}
#endif

