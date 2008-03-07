/*
   nxsocksd - user specific SOCKS5 daemon

   main.c - initialization, listening socket, ident lookup

   Copyright 1997 Olaf Titz <olaf@bigred.inka.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the License, or (at your option) any later version.
*/
/* $Id: main.c,v 1.20 1999/05/13 22:28:11 olaf Exp $ */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

extern int optind;
extern char *optarg;

#include "thread.h"
#include "socks.h"
#include "log.h"
#include "lib.h"
#include "stats.h"
#include "resolv.h"

#ifdef DEBUG
int debug=0;
#endif

#ifndef BUFS
#define BUFS 16384
#endif

unsigned short port=1080;
int maxfail=0;

/* statistics */
int s_connects=0;
int s_refused=0;
int s_protfail=0;
int s_authfail=0;
int s_addrfail=0;
int s_rconn=0;
int s_rbind=0;
int s_rudp=0;
#ifdef DO_SPAWN
int s_rspawn=0;
#endif
int s_fconn=0;
int s_fbind=0;
int s_fudp=0;

#ifdef __GNUC__
/* Shut up warnings */
static void usage(const char *p) __attribute__((noreturn));
#endif

typedef enum {
    st_connecting, st_connected, st_resp
} ident_state;

/* Acceptor parameter block. */

typedef struct {
    struct in_addr *acc;        /* Addresses to accept from */
    int nacc;                   /* length of acc array */
    struct in_addr *udp;        /* Addresses for UDP relayer */
    int nudp;                   /* length of udp array */
    char *id;                   /* Ident user to accept from */
    char *uname, *pass;         /* auth user/password */
    struct sockaddr_in identa;  /* identd socket address */
    struct sockaddr_in caddr;   /* connecting address */
    int pendfd, idfd;           /* connecting/ident fd*/
    ident_state state;          /* ident state */
} perm;

/* Look up host names/IP addresses. */
static int blookup(const char *c, struct in_addr *sa)
{
    struct hostent *h;
    if ((sa->s_addr=inet_addr(c))==0xFFFFFFFF) {
	if ((h=gethostbyname(c))) {
	    memcpy(&sa->s_addr, h->h_addr_list[0],
		   sizeof(sa->s_addr));
	} else {
	    eprintf1("host `%s' invalid/not found", c);
	    return -1;
	}
    }
    return 0;
}

static int blookupn(const char *c, struct in_addr **sa, int *n)
{
    char buf[256];
    const char *p, *p0;
    char *p1;
    struct in_addr *sa0;

    for (p=c, *n=1; *p; ++p)
        if (*p==',')
            ++(*n);
    if (!(*sa=sa0=malloc((*n)*sizeof(struct in_addr)))) {
        eprintf0("blookupn: out of memory!");
        return -1;
    }
    for (p0=p=c; *p0&&*p; p=p0+1, ++sa0) {
        for (p0=p; *p0 && (*p0!=','); ++p0);
        if (p0-p>0 && p0-p<256) {
            for (p1=buf; p<p0;)
                *p1++=*p++;
            *p1='\0';
            if (blookup(buf, sa0)==0)
                continue;
        }
        --(*n); --sa0; /* lookup error */
    }
    return 0;
}

/* Open a listening socket on specified port. */
static int opensock(short p)
{
    struct sockaddr_in sa;
    int o=1;
    int s=nsocket(AF_INET, SOCK_STREAM, 0);
    if (s<0) {
	perror("socket"); return -1;
    }
    sa.sin_family=AF_INET;
    sa.sin_port=htons(p);
    sa.sin_addr.s_addr=htonl(INADDR_ANY);
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o))<0) {
	perror("warning: SO_REUSEADDR"); /* not fatal */
    }
    if (bind(s, (struct sockaddr *)&sa, sizeof(sa))<0) {
	perror("bind"); return -1;
    }
    if (listen(s, 128)<0) {
	perror("listen"); return -1;
    }
    return s;
}

/* Parse identd response. Note: read the response into buf+1. */
static const char *identparse(char *buf)
{
    char *p, *q;

    buf[strcspn(buf+1, "\r\n")+1]='\0';
    if (!(p=strchr(buf+1, ':')))
	return buf;
    while (strchr(": \t", *p)) ++p;
    if (strcasecmp(p, "USERID")) {
	if (!(p=strchr(p, ':')))
	    return "^UNKNOWN";
	while (strchr(": \t", *p)) ++p;
	if (!(q=strchr(p, ':')))
	    return "^UNKNOWN";
	/* RFC1413 says this is incorrect but practice dictates otherwise: */
	while (strchr(": \t", *q)) ++q;
	if (!strcasecmp(p, "OTHER"))
	    *--q='^';
	return q;
    }

    /* Error response */
    if (!(p=strchr(p, ':')))
	return "!UNKNOWN-ERROR";
    while (strchr(": \t", *p)) ++p;
    *--p='!';
    return p;
}

/* identd lookup reader routine */
void ident_rd(int fd, void *a)
{
    perm *p=a;
    char buf[1024];
    const char *i;

    dprintf1(DEB_IDS, "ident_rd state %d", p->state);
    switch (p->state) {

    case st_resp: /* read the answer */
	memset(buf, 0, sizeof(buf));
	read(fd, buf+1, sizeof(buf)-2);
	dprintf1(DEB_ID, "ident_rd ident: `%s'", buf+1);
	thread_fd_close(fd);
	thread_timer_cancel(fd);
	i=identparse(buf);
	if (strcmp(i, p->id)) {
	    eprintf1("refused connect from ident %s", i);
	    ++s_refused;
	    close(p->pendfd);
	    return;
	}
	dprintf1(DEB_CONN, "ident_rd socks_init %d", p->pendfd);
	/* register the control connxn */
        if (p->nudp)
            socks_init(p->pendfd, p->uname, p->pass, p->udp, p->nudp);
        else
            socks_init(p->pendfd, p->uname, p->pass, &p->caddr.sin_addr, 1);
	return;

    default:
	return;
    }
}

/* identd lookup writer routine */
void ident_wr(int fd, void *a)
{
    perm *p=a;
#ifndef NONBLOCK_ONCE
    int sal=sizeof(struct sockaddr_in);
#endif
    char buf[16];

#define trans(x) do{p->state=(x);return;}while(0)

    dprintf1(DEB_IDS, "ident_wr state %d", p->state);
    switch (p->state) {

    case st_connecting: /* connection is ready */
#ifdef NONBLOCK_ONCE
	trans(st_connected);
#else
	if (connect(fd, (struct sockaddr *)&p->identa, sal)>=0)
	    trans(st_connected);
	perror("ident_wr: connect");
	thread_fd_close(fd);
	close(p->pendfd);
	return;
#endif

    case st_connected: /* write the request */
	dprintf0(DEB_ID, "ident_wr requesting ident");
	sprintf(buf, "%d,%d\r\n", ntohs(p->caddr.sin_port), port);
	write(fd, buf, strlen(buf));
	thread_fd_wr_off(fd);
	trans(st_resp);

    default:
	return;
    }

#undef trans
}

/* identd lookup timeout routine */
void ident_to(int fd, void *a)
{
    perm *p=a;
    dprintf0(DEB_ID, "ident_to");
    thread_fd_close(fd);
    eprintf0("refused connect, no ident");
    ++s_refused;
    close(p->pendfd);
}

/* Listening socket reader routine (i.e. accept connection) */
void newconn(int fd, void *a)
{
    perm *p=a;
    int n, c;
    struct sockaddr_in sa;
    size_t sal=sizeof(sa);

    if (maxfail>0 && s_refused+s_protfail+s_authfail>maxfail) {
	printf("Too many failed requests. Throttling.\n");
	thread_fd_close(fd);
	return;
    }

    if ((c=accept(fd, (struct sockaddr *)&sa, &sal))<0) {
	perror("newconn: accept");
	return; /* eh? */
    }
    dprintf3(DEB_CONN, "newconn: from %s:%d -> %d", inet_ntoa(sa.sin_addr),
	     ntohs(sa.sin_port), c);

    ++s_connects;
    if (p->nacc) {
        for (n=0; n<p->nacc; ++n)
            if (sa.sin_addr.s_addr==p->acc[n].s_addr)
                goto okay;
	eprintf1("refused connect from %s", inet_ntoa(sa.sin_addr));
	++s_refused;
	close(c);
	return;
    }
 okay:

    p->caddr=sa;
    printf("connect from %s\n", inet_ntoa(sa.sin_addr));
    n=1;
    if (setsockopt(c, SOL_SOCKET, SO_OOBINLINE, &n, sizeof(n))<0)
	perror("newconn: warning: SO_OOBINLINE"); /* not fatal */
    n=2*BUFS;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n))<0)
        perror("newconn: warning: SO_SNDBUF"); /* not fatal */
    n=2*BUFS;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n))<0)
        perror("newconn: warning: SO_RCVBUF"); /* not fatal */

    if (!p->id) {
	dprintf1(DEB_CONN, "newconn: socks_init %d", c);
        if (p->nudp)
            socks_init(c, p->uname, p->pass, p->udp, p->nudp);
        else
            socks_init(c, p->uname, p->pass, &p->caddr.sin_addr, 1);
        return;
    }

    if ((n=nsocket(AF_INET, SOCK_STREAM, 0))<0) {
	perror("newconn: socket");
	close(n);
	close(c);
	return;
    }
    dprintf1(DEB_CONN, "newconn: ident lookup -> %d", n);
    sa.sin_port=htons(113);
    p->identa=sa;
    if ((connect(n, (struct sockaddr *)&sa, sal)<0)) {
	if ((errno!=EINPROGRESS)) {
	    perror("newconn: connect");
	    close(n);
	    close(c);
	    return;
	}
	p->state=st_connecting;
    } else {
	p->state=st_connected;
    }
    p->pendfd=c;
    p->idfd=n;
    thread_fd_register(n, ident_rd, ident_wr, NULL, p);
    thread_timer_register(30, ident_to, n, p);
}

/* Look up own IP address */
static void setup_myaddress(void)
{
    struct in_addr a;
    char buf[128];
    if (gethostname(buf, sizeof(buf))<0) {
	perror("setup_myaddress: gethostname"); exit(1);
    }
    if (blookup(buf, &a)<0)
	exit(1);
    myaddress=a;
}

#ifdef DO_SPAWN
static RETSIGTYPE reap(int s)
{
#ifdef HAVE_WAITPID
    while (waitpid(-1, NULL, WNOHANG)>0);
#else
#ifdef HAVE_WAIT4
    while (wait4(-1, NULL, WNOHANG, NULL)>0);
#else
#ifdef HAVE_WAIT3
    while (wait3(NULL, WNOHANG, NULL)>0);
#else
#warning "No suitable wait function available, might produce zombies"
#endif
#endif
#endif
#ifndef HAVE_SIGACTION
    setsig(SIGCHLD, reap);
#endif
}
#endif

static RETSIGTYPE finish(int s)
{
    thread_stop=s;
}

static void eofh(int fd, void *a)
{
    char buf[256];
    if (read(fd, buf, sizeof(buf))<=0)
	finish(-1);
}

static void printaddrlist(const char *f, struct in_addr *a, int n)
{
    int i;
    char buf[16*32]="";
    if (n==0) {
        printf(f, "(anywhere)");
        return;
    }
    for (i=0; i<n; ++i) {
        strcat(buf, inet_ntoa(a[i]));
        if (i>15) {
            strcat(buf, ",...");
            break;
        }
        if (i<n-1)
            strcat(buf, ",");
    }
    printf(f, buf);
}

static void usage(const char *p)
{
    eprintf1("usage: %s [-p port] [-a accepthost[,...]] [-u udphost[,...]]",
             p);
    eprintf0("       [-i identuser] [-U authuser]");
    exit(1);
}

int main(int argc, char *argv[])
{
    int n;
    perm pm;
    char *acchost=NULL;
    char *udphost=NULL;
    char buf[128];

    setunbuf(stdout);
    setunbuf(stderr);
    memset(&pm, 0, sizeof(pm));
    printf("nxsocksd version " VERSION " (c) Olaf Titz 1997-1999\n");
    while((n=getopt(argc, argv, "p:a:u:i:U:d:f:"))!=EOF) {
	switch(n) {
	case 'p': port=atoi(optarg); break;
	case 'a': acchost=optarg; break;
	case 'u': udphost=optarg; break;
	case 'i': pm.id=optarg; break;
	case 'U': pm.uname=optarg; break;
#ifdef DEBUG
	case 'd': debug=atoi(optarg); break;
#endif
	case 'f': maxfail=atoi(optarg); break;
	default: usage(argv[0]);
	}
    }
#ifdef NORELAX
    if ((!acchost) || (!udphost) || (!pm.uname)) {
	eprintf1("%s: need all of the -a, -u and -U parameters", argv[0]);
	usage(argv[0]);
    }
#endif
    if ((thread_init()<0) || (resolv_init()<0)) {
	eprintf1("%s: out of memory, a hopeless case", argv[0]);
	exit(1);
    }
    pm.nacc=0;
    if (acchost && (blookupn(acchost, &pm.acc, &pm.nacc)<0))
	exit(1);
    pm.nudp=0;
    if (udphost && (blookupn(udphost, &pm.udp, &pm.nudp)<0))
	exit(1);

    setup_myaddress();
    if ((n=opensock(port))<0)
	exit(1);
    thread_fd_register(n, newconn, NULL, NULL, &pm);

    printaddrlist(" Accepting connnections from %s", pm.acc, pm.nacc);
    printf(" ident %s\n", (pm.id) ? pm.id : "(anyone)");
    printaddrlist(" Relaying UDP from %s\n", pm.udp, pm.nudp);
    if (pm.uname && getenv("NXSOCKS_PASSWORD") == NULL) {
#ifdef HAVE_TERMIOS_H
	struct termios tio;
	int l=0;
	if ((n=tcgetattr(0, &tio))>=0) {
	    l=tio.c_lflag;
	    tio.c_lflag&=(~ECHO);
	    n=tcsetattr(0, TCSANOW, &tio);
	}
#endif
	printf(" Requiring user/password authentication as user %s\n",
	       pm.uname);
#ifdef HAVE_TERMIOS_H
	if (n>=0)
#endif
	    printf(" Enter password to be used: ");
#ifdef HAVE_TERMIOS_H
	else
	    printf(" Reading password from stdin");
#endif
	pm.pass=fgets(buf, sizeof(buf), stdin);
	if (pm.pass)
	    buf[strcspn(buf, "\r\n")]='\0';
	printf("\n");
#ifdef HAVE_TERMIOS_H
	if (n>=0) {
	    tio.c_lflag=l;
	    (void)tcsetattr(0, TCSANOW, &tio);
	}
#endif
    }
    else
    {
        pm.pass=strncpy(buf, getenv("NXSOCKS_PASSWORD"), sizeof(buf));
    }
    printf("Listening on port %d.\n", port);
    /*thread_fd_register(0, eofh, NULL, NULL, NULL); */
    setsig(SIGINT, finish);
    setsig(SIGTERM, finish);
    setsig(SIGPIPE, SIG_IGN);
#ifdef DO_SPAWN
    setsig(SIGCHLD, reap);
#endif
    thread_mainloop();
    if (thread_stop>0)
	printf("Got signal %d\n", thread_stop);
    printf("Connection stats: %d connects, %d refused\n",
	   s_connects, s_refused);
    printf(" %d handshake failed, %d auth failed, %d addressing failed\n",
	   s_protfail, s_authfail, s_addrfail);
    printf(" Requests: %d connect, %d bind, %d UDP",
	   s_rconn, s_rbind, s_rudp);
#ifdef DO_SPAWN
    printf(", %d spawn", s_rspawn);
#endif
    printf("\n Requests failed: %d connect, %d bind, %d UDP\n",
	   s_fconn, s_fbind, s_fudp);
#ifdef MDEBUG
    memorymap(1);
#endif
    printf("Exiting.\n");
    exit(0);
}
