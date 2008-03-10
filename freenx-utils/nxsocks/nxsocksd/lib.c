/*
   nxsocksd - user specific SOCKS5 daemon

   lib.c - convenience functions

   Copyright 1997 Olaf Titz <olaf@bigred.inka.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the License, or (at your option) any later version.
*/
/* $Id: lib.c,v 1.12 1999/05/13 22:28:11 olaf Exp $ */

#include "config.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "lib.h"

#ifdef NONBLOCK_NONE
#ifdef O_NONBLOCK
#undef O_NONBLOCK
#endif
#endif

/* Open a nonblocking socket */
int nsocket(int domain, int type, int protocol)
{
    int n=socket(domain, type, protocol);
#ifdef O_NONBLOCK
    if (n>=0) {
	register int i=fcntl(n, F_GETFL);
	if ((i<0) || (fcntl(n, F_SETFL, i|O_NONBLOCK)<0)) {
	    close(n);
	    return -1;
	}
    }
#endif
    return n;
}

/* Init a struct sockaddr_in */

void sockaddr_init(struct sockaddr_in *sa)
{
    memset(sa, 0, sizeof(struct sockaddr_in));
    sa->sin_family=AF_INET;
}

#ifdef HAVE_LINUX_LIBC4

/* Implement the sendmsg() call which is missing from the library. */

#include <linux/net.h>
#include <linux/unistd.h>
static inline _syscall2(int,socketcall, int,call, unsigned long *,args);

int sendmsg(int s, const struct msghdr *msg, unsigned int flags)
{
    unsigned long args[3] = {
	(unsigned long) s,
	(unsigned long) msg,
	(unsigned long) flags
    };
    return socketcall(SYS_SENDMSG, args);
}

#endif

#ifndef HAVE_MEMSET

void *memset(void *s, int c, size_t n)
{
    register char *p=s;
    while (n-->0)
	*p++=c;
    return s;
}

#endif

#ifdef DEBUG

void hexdump(direction d, int fd, unsigned char *p, int n)
{
    register int l=0;
    char buf[78];
    unsigned char c;
#define hpos(x) (3*(x)+(((x)>7)?2:1))
#define apos(x) ((x)+60) /* (x+((x>7)?60:59)) */
#define hdigit(x) (((x)>9)?((x)-10+'a'):((x)+'0'))

    memset(buf, ' ', 77);
    buf[77]='\0';
    printf("%s %d:\n", (d==d_from) ? "From" : "To", fd);
    while (n-->0) {
	c=(unsigned char)*p++;
	buf[hpos(l)]   = hdigit(c>>4);
	buf[hpos(l)+1] = hdigit(c&15);
	buf[apos(l)] = (c<32||c>126) ? '.' : c;
	if (++l>15) {
	    puts(buf);
	    memset(buf, ' ', 77);
	    l=0;
	}
    }
    puts(buf);

#undef hpos
#undef apos
#undef hdigit
}

#endif

#ifdef DO_SPAWN
void xexec(char *argv[], int fd)
{
    register int i;
    char *pr=argv[0];
#ifdef O_NONBLOCK
    i=fcntl(fd, F_GETFL);
    (void) fcntl(fd, F_SETFL, i&(~O_NONBLOCK));
#endif
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    for (i=getdtablesize()-1; i>2; --i)
        (void) close(i);
    setsig(SIGPIPE, SIG_DFL);
#if defined(HAVE_STRDUP) && defined(HAVE_STRRCHR)
    {
        char *p=strrchr(argv[0], '/');
        if (p && (p=strdup(p+1)))
            argv[0]=p;
    }
#endif
    execv(pr, argv);
    write(2, "exec error\n", 11);
}
#endif

/* Signal handling. */
#ifdef HAVE_SIGACTION
void setsig(int sig, RETSIGTYPE (*fun)(int s))
{
  struct sigaction sa;
  sa.sa_handler = fun;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(sig, &sa, NULL);
}
#endif
