#ifndef _lib_h_
#define _lib_h_

#ifdef STDC_HEADERS
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
#error Neither <string.h> nor <strings.h> found
#endif
#endif

#include <netinet/in.h>
#include <signal.h>

extern int nsocket(int domain, int type, int protocol);
extern void sockaddr_init(struct sockaddr_in *sa);

#ifdef DEBUG
typedef enum {d_from, d_to} direction;
extern void hexdump(direction d, int fd, unsigned char *p, int n);
#endif

#ifdef HAVE_LINUX_LIBC4
extern int sendmsg(int s, const struct msghdr *msg, unsigned int flags);
#endif

#ifndef HAVE_MEMSET
extern void *memset(void *s, int c, size_t n);
#endif

#ifdef DO_SPAWN
extern void xexec(char *argv[], int fd);
#endif

#ifdef HAVE_SIGACTION
extern void setsig(int sig, RETSIGTYPE (*fun)(int s));
#else
#define setsig signal
#endif

#endif
