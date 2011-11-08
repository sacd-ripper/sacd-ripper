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

#ifndef SOCKET_H
#define SOCKET_H

/*=========================================================================*\
* Platform specific compatibilization
\*=========================================================================*/
#ifdef _WIN32
#include "wsocket.h"
#else
#include "usocket.h"
#endif

/*=========================================================================*\
* The connect and accept functions accept a timeout and their
* implementations are somewhat complicated. We chose to move
* the timeout control into this module for these functions in
* order to simplify the modules that use them. 
\*=========================================================================*/
#include "timeout.h"

/* IO error codes */
enum {
    IO_DONE = 0,        /* operation completed successfully */
    IO_TIMEOUT = -1,    /* operation timed out */
    IO_CLOSED = -2,     /* the connection has been closed */
    IO_UNKNOWN = -3
}; 

#ifdef _WIN32
#define INET_ATON
#endif 

const char *io_strerror(int err); 

const char *inet_trycreate(p_socket ps, int type);
const char *inet_tryconnect(p_socket ps, const char *address, 
                            unsigned short port, p_timeout tm);
const char *inet_trybind(p_socket ps, const char *address, 
                         unsigned short port);

#ifdef INET_ATON
int inet_aton(const char *cp, struct in_addr *inp);
#endif 

/* we are lazy... */
typedef struct sockaddr SA;

/*=========================================================================*\
* Functions bellow implement a comfortable platform independent 
* interface to sockets
\*=========================================================================*/
int socket_open(void);
int socket_close(void);
void socket_destroy(p_socket ps);
void socket_shutdown(p_socket ps, int how); 
int socket_sendto(p_socket ps, const char *data, size_t count, 
        size_t *sent, SA *addr, socklen_t addr_len, p_timeout tm);
int socket_recvfrom(p_socket ps, char *data, size_t count, 
        size_t *got, SA *addr, socklen_t *addr_len, p_timeout tm);

void socket_setnonblocking(p_socket ps);
void socket_setblocking(p_socket ps);

int socket_waitfd(p_socket ps, int sw, p_timeout tm);
int socket_select(t_socket n, fd_set *rfds, fd_set *wfds, fd_set *efds, 
        p_timeout tm);

int socket_connect(p_socket ps, SA *addr, socklen_t addr_len, p_timeout tm); 
int socket_create(p_socket ps, int domain, int type, int protocol);
int socket_bind(p_socket ps, SA *addr, socklen_t addr_len); 
int socket_listen(p_socket ps, int backlog);
int socket_accept(p_socket ps, p_socket pa, SA *addr, 
        socklen_t *addr_len, p_timeout tm);

const char *socket_hoststrerror(int err);
const char *socket_strerror(int err);

/* these are perfect to use with the io abstraction module 
   and the buffered input module */
int socket_send(p_socket ps, const char *data, size_t count, 
        size_t *sent, int flags, p_timeout tm);
int socket_recv(p_socket ps, char *data, size_t count, size_t *got, int flags, p_timeout tm);
const char *socket_ioerror(p_socket ps, int err);

int socket_gethostbyaddr(const char *addr, socklen_t len, struct hostent **hp);
int socket_gethostbyname(const char *addr, struct hostent **hp);

#endif /* SOCKET_H */
