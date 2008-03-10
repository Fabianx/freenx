/*
   nxsocksd - user specific SOCKS5 daemon

   socks.c - SOCKS5 protocol dialog handler, TCP connection handler

   Copyright 1997 Olaf Titz <olaf@bigred.inka.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the License, or (at your option) any later version.
*/
/* $Id: socks.c,v 1.19 1999/05/13 22:28:11 olaf Exp $ */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "thread.h"
#include "socks.h"
#include "log.h"
#include "lib.h"
#include "stats.h"
#include "udp.h"
#include "resolv.h"

#ifndef BUFS
#define BUFS 16384
#endif

/* Global */
struct in_addr myaddress;

typedef enum socks_state {
    st_rinit, st_rauths, st_wauths,
    st_ruser, st_rpass, st_chkpass, st_wpass,
    st_ratyp, st_raddr, st_request,
    st_copening, st_copened,
    st_bopening, st_bwaiting, st_bopened,
    st_uopening, st_uwaiting,
    st_running,  st_eof,
    st_spawn,
    st_err
} socksstate;

/* Parameter block */
typedef struct socks_parm {
    socksstate state;
    short port;                     /* used by DNS lookup */
    const char *uname, *pass;       /* Authentication */
    int me, proxy;                  /* Own/proxy fd */
    unsigned int bufpos, bufgoal;   /* Buffer tail/head position */
    struct socks_parm *peer;        /* Peer parameter block */
    struct in_addr *udpclient;      /* UDP expectance */
    int udpclientn;
    unsigned char buf[BUFS];        /* The buffer proper */
} socksparm;

/* Fields of the request/reply */
#define rq_ver(s) ((s)->buf[0])
#define rq_cmd(s) ((s)->buf[1])
#define rq_rsv(s) ((s)->buf[2])
#define rq_atyp(s) ((s)->buf[3])
#define rq_addr(s) (*(struct in_addr *)(((s)->buf)+4))
#define rq_port(s) (*(unsigned short *)(((s)->buf)+8))

#define rp_ver(s) ((s)->buf[16])
#define rp_rep(s) ((s)->buf[17])
#define rp_rsv(s) ((s)->buf[18])
#define rp_atyp(s) ((s)->buf[19])
#define rp_addr(s) (*(struct in_addr *)(((s)->buf)+20))
#define rp_port(s) (*(unsigned short *)(((s)->buf)+24))

#define reqsize 10
#define reppos 16

/* Set buffer positions: read/write from, to */
static void bufset(socksparm *sp, int pos, int len)
{
    dprintf2(DEB_SOS, "bufset %d %d", pos, len);
    sp->bufpos=pos; sp->bufgoal=pos+len;
}

/* Close this fd and its proxy. */
void closeboth(int fd, void *a)
{
    socksparm *sp=a;
    dprintf2(DEB_SO, "closeboth %d %d", fd, sp->proxy);
    if (sp->proxy>=0) {
	thread_fd_close(sp->proxy);
	thread_timer_cancel(sp->proxy);
    }
    if (sp->peer)
	free(sp->peer);
    thread_fd_close(fd);
    thread_timer_cancel(fd);
    free(sp);
}

/* Read a chunk into buffer. Return true if complete. */
static int readcompl(int fd, socksparm *sp)
{
    int n=read(fd, sp->buf+sp->bufpos, sp->bufgoal-sp->bufpos);
    if (n<=0) {
	closeboth(fd, sp);
	return 0;
    }
#ifdef DEBUG
    if (debug&DEB_CDUMP)
	hexdump(d_from, fd, sp->buf+sp->bufpos, n);
#endif
    sp->bufpos+=n;
    return (sp->bufpos>=sp->bufgoal);
}

/* Write a chunk from buffer. Return true if complete. */
static int writecompl(int fd, socksparm *sp)
{
    int n=write(fd, sp->buf+sp->bufpos, sp->bufgoal-sp->bufpos);
    if (n<=0) {
	closeboth(fd, sp);
	return 0;
    }
#ifdef DEBUG
    if (debug&DEB_CDUMP)
	hexdump(d_to, fd, sp->buf+sp->bufpos, n);
#endif
    sp->bufpos+=n;
    return (sp->bufpos>=sp->bufgoal);
}

/* Translate errno to SOCKS error indication */
static int transerr(const char *n, int c)
{
    int e=1, er=errno;
    switch (er) {
    case EINPROGRESS:
	e = (c==0) ? -1 : 1; break;
#ifdef ENONET
    case ENONET:
#endif
    case ENETUNREACH:
	e=3; break;		/* network unreachable */
#ifdef ENOPROTOOPT
    case ENOPROTOOPT:
#endif
    case ECONNREFUSED:
	e=5; break;		/* connection refused */
    default:
	e=4; break;		/* host unreachable */
    }
    dprintf3(DEB_SO, "transerr: %s: %s (%d)", n, strerror(er), e);
    errno=er;
    return e;
}

/* Do a connect() call, parameters in buffer */
static int doconnect(socksparm *sp, int c)
{
    int e;
    struct sockaddr_in sa;
    size_t sal=sizeof(sa);
    sockaddr_init(&sa);
    sa.sin_addr=rq_addr(sp);
    sa.sin_port=rq_port(sp);
    dprintf3(DEB_SO, "doconnect req %d %s %d", c, inet_ntoa(sa.sin_addr),
	     ntohs(sa.sin_port));
#ifdef NONBLOCK_ONCE
    if (c==1) {
	size_t el=sizeof(e);
	if (getsockopt(sp->proxy, SOL_SOCKET, SO_ERROR, &e, &el)<0) {
	    dprintf1(DEB_SO, "doconnect getsockopt: %s", strerror(errno));
	} else {
	    errno=e;
	    e = (errno==0) ? 0 : -1;
	}
    } else
#endif
	e=connect(sp->proxy, (struct sockaddr *)&sa, sal);
    if (e<0)
	return transerr("connect", c);
    if (getsockname(sp->proxy, (struct sockaddr *)&sa, &sal)<0)
	return transerr("getsockname", c);
    dprintf3(DEB_SO, "doconnect rsp %d %s %d", c, inet_ntoa(sa.sin_addr),
	     ntohs(sa.sin_port));
    rp_addr(sp)=sa.sin_addr;
    rp_port(sp)=sa.sin_port;
    return 0;
}

/* Do a bind() call, parameters in buffer */
static int dobind(socksparm *sp, int c)
{
    struct sockaddr_in sa;
    size_t sal=sizeof(sa);
    sockaddr_init(&sa);
    sa.sin_addr.s_addr=htonl(INADDR_ANY);
#ifdef USECPORT_BIND
    if (ntohl(rq_port(sp))>1024)
	/* Try to bind to port requested by client.
	   RFC 1928 is unclear about whether this is correct. */
	sa.sin_port=rq_port(sp);
    else
	/* don't bother at all for reserved ports */
#endif
	sa.sin_port=htons(0);
    dprintf3(DEB_SO, "dobind req %d %s %d", c, inet_ntoa(sa.sin_addr),
	     ntohs(sa.sin_port));
    if (bind(sp->proxy, (struct sockaddr *)&sa, sal)<0)
	return transerr("bind", c);
    if (listen(sp->proxy, 1)<0)
	return transerr("listen", c);
    if (getsockname(sp->proxy, (struct sockaddr *)&sa, &sal)<0)
	return transerr("getsockname", c);
    dprintf3(DEB_SO, "dobind rsp %d %s %d", c, inet_ntoa(myaddress),
	     ntohs(sa.sin_port));
    rp_addr(sp)=myaddress;
    rp_port(sp)=sa.sin_port;
    return 0;
}

/* Do an accept() call (bind second reply), parameters in buffer */
static int doaccept(socksparm *sp, int c)
{
    int n;
    struct sockaddr_in sa;
    size_t sal=sizeof(sa);
    if ((n=accept(sp->proxy, (struct sockaddr *)&sa, &sal))<0)
	return transerr("accept", c);
    dprintf3(DEB_SO, "doaccept rsp %d %s %d", c, inet_ntoa(sa.sin_addr),
	     ntohs(sa.sin_port));
    rp_addr(sp)=sa.sin_addr;
    rp_port(sp)=sa.sin_port;
    thread_fd_close(sp->proxy);
    sp->proxy=n;
    return 0;
}

#ifdef DO_SPAWN
/* Run a subprocess. */
static void dospawn(socksparm *sp)
{
    char *argv[4]={NULL};
    register int i;

    if (vfork()!=0)
        return;
    switch(rq_cmd(sp)) {
#ifdef PATH_PING
    case 128:
        argv[0]=PATH_PING;
        break;
#endif
#ifdef PATH_TRACE
    case 129:
        argv[0]=PATH_TRACE;
        break;
#endif
    }
    i=1;
    if (rq_rsv(sp)&1)
        argv[i++]="-n";
    if (rq_rsv(sp)&2)
        argv[i++]="-v";

    argv[i]=inet_ntoa(rq_addr(sp));
    if (argv[0])
        xexec(argv, sp->me);
    else
        write(sp->me, "Program not available\n", 21);
    exit(-1);
}
#endif


/*** Handlers for symmetric shuffling. Parameter block is own ***/

/* Read into buffer... */
void shuffle_rd(int fd, void *a)
{
    int n;
    socksparm *sp=a;
    n=BUFS-sp->bufgoal;
    if (n<=0) {
	if (sp->bufpos>=BUFS) {
	    bufset(sp, 0, 0);
	    n=BUFS;
	} else {
	    /* buffer full */
	    thread_fd_rd_off(fd);
	    return;
	}
    }
    n=read(fd, sp->buf+sp->bufgoal, n);
    if (n<=0) {
        /* EOF or read error */
        dprintf2(DEB_CONN, "shuffle_rd %d %s", fd, n<0?"error":"eof");
        sp->state=st_eof;
        shutdown(fd, 0);
        thread_fd_rd_off(fd);
        thread_fd_wr_on(sp->proxy);
        return;
    }
#ifdef DEBUG
    if (debug&DEB_DDUMP)
	hexdump(d_from, fd, sp->buf+sp->bufpos, n);
#endif
    sp->bufgoal+=n;
    thread_fd_wr_on(sp->proxy);
}

/* Write from buffer... */
void shuffle_wr(int fd, void *a)
{
    socksparm *sp=a;
    int n;
    sp=sp->peer; /* write from peer buffer */
    n=sp->bufgoal-sp->bufpos;
    if (n==0) {
        /* buffer empty */
        if (sp->state==st_eof) {
            dprintf1(DEB_CONN, "shuffle_wr eof %d", fd);
            if (sp->peer->state==st_eof) {
                closeboth(fd, sp->peer);
                return;
            }
            shutdown(fd, 1);
        }
        bufset(sp, 0, 0);
	thread_fd_wr_off(fd);
	return;
    }
    n=write(fd, sp->buf+sp->bufpos, n);
    if (n<=0) {
	/* broken pipe */
	dprintf1(DEB_CONN, "shuffle_wr err close %d", fd);
	closeboth(fd, sp->peer);
	return;
    }
    sp->bufpos+=n;
    thread_fd_rd_on(sp->me);
}


/*** Handlers for new proxy. The parameter block is that of the master ***/

/* Newly opened proxy is ready for reading, i.e. has just been connected to */
void proxy_rd(int fd, void *a)
{
    socksparm *sp=a;
    int n=doaccept(sp, 1);
    rp_rep(sp)=n;
    if (n>0)
	++s_fbind;
    sp->state=(n>0) ? st_err : st_bopened;
    thread_fd_wr_on(sp->me);
    bufset(sp, reppos, reqsize);
    thread_fd_rd_off(fd);
}

/* Newly opened proxy is ready for writing, i.e. has just connected */
void proxy_wr(int fd, void *a)
{
    socksparm *sp=a;
    int n=doconnect(sp, 1);
    rp_rep(sp)=n;
    if (n>0)
	++s_fconn;
    sp->state=(n>0) ? st_err : st_copened;
    thread_fd_wr_on(sp->me);
    bufset(sp, reppos, reqsize);
    thread_fd_wr_off(fd);
}


/*** Handlers for the socks control connection ***/

void socks_rd(int fd, void *a);

void socks_rd_gotaddr(struct hostent *h, int s, int fd, void *a)
{
    socksparm *sp=a;
    dprintf2(DEB_SOS, "socks_rd_gotaddr fd %d status %d", fd, s);
    if (s!=0) {
	eprintf1("DNS lookup failed: `%s'", sp->buf+5);
	++s_addrfail;
	sp->buf[1]=1;
	bufset(sp, 0, 10);
	sp->state=st_err;
	thread_fd_rd_off(fd);
	thread_fd_wr_on(fd);
	thread_timer_register(10, closeboth, fd, sp);
	return;
    }
    /* Fill in the request with the address */
    rq_atyp(sp)=1;
    memcpy(&(rq_addr(sp)), h->h_addr_list[0], sizeof(struct in_addr));
    rq_port(sp)=htons(sp->port);
    sp->state=st_request;
    socks_rd(fd, a); /* restart */
}

void socks_rd(int fd, void *a)
{
    socksparm *sp=a;
    struct sockaddr_in pa;
    unsigned char *p;
    int i, n;
#ifdef USECLIENTSPORT_TCP
    size_t sal;
#endif

#define trans(x) do{sp->state=(x); return;}while(0)
#define waitcompl() if (!readcompl(fd,sp)) return;
#define flagerr(n) do{sp->buf[1]=(n); bufset(sp,0,10); goto err0;}while(0)

    dprintf2(DEB_SOS, "socks_rd fd %d state %d", fd, sp->state);
    switch (sp->state) {

    case st_rinit:
	waitcompl();
	bufset(sp, 2, sp->buf[1]);
	trans(st_rauths);

    case st_rauths:
	waitcompl();
	thread_fd_wr_on(fd);
	if (rq_ver(sp)==5) {
	    n=(sp->pass) ? 2 : 0;
	    for (i=2; i<sp->buf[1]+2; ++i)
		if (sp->buf[i]==n) {
		    sp->buf[1]=n;
		    bufset(sp, 0, 2);
		    trans(st_wauths);
		}
	}
	eprintf1("%d not socks5", fd);
	++s_protfail;
	goto err255;

    case st_ruser:
	waitcompl();
	if (rq_ver(sp)!=1) {
	    eprintf1("%d not passwd auth", fd);
	    ++s_protfail;
	    goto err255;
	}
	bufset(sp, 2, sp->buf[1]+1);
	trans(st_rpass);

    case st_rpass:
	waitcompl();
	if (sp->buf[sp->buf[1]+2]==0)
	    goto havepasswd;
	bufset(sp, sp->buf[1]+3, sp->buf[sp->buf[1]+2]);
	trans(st_chkpass);

    case st_chkpass:
	waitcompl();
    havepasswd:
	p=sp->buf+sp->buf[1]+2;
	p[*p+1]=0; *p=0;
	/* Cast needed by some compilers - signed vs. unsigned */
	if (strcmp(sp->uname, (char *)(sp->buf+2)) ||
	    strcmp(sp->pass, (char *)(p+1))) {
	    eprintf1("%d passwd auth failed", fd);
	    ++s_authfail;
	    goto err255;
	}
	sp->buf[1]=0;
	bufset(sp, 0, 2);
	thread_fd_wr_on(fd);
	trans(st_wpass);

    case st_ratyp:
	waitcompl();
	switch(rq_atyp(sp)) {
	case 1:
	    bufset(sp, 5, 5); break;
	case 3:
	    bufset(sp, 5, sp->buf[4]+2); break;
	default:
	    ++s_addrfail;
	    flagerr(8);
	}
	trans(st_raddr);

    case st_raddr:
	waitcompl();
	/* Now we have the request */
	switch (rq_atyp(sp)) {
	case 1: /* IP address */
	    break;
	case 3: /* domain name */
	    sockaddr_init(&pa);
	    p=sp->buf+5+sp->buf[4];
	    sp->port=(p[0]<<8)+p[1]; /* save the port */
	    *p=0;
	    dprintf1(DEB_SO, "socks_rd lookup %s", sp->buf+5);
	    lookup((char *)(sp->buf+5), socks_rd_gotaddr, fd, sp);
	    return;
	default: /* invalid/unsupported */
	    thread_fd_wr_on(fd);
	    ++s_addrfail;
	    flagerr(1);
	}
	/*fallthru*/
    case st_request:
	thread_fd_wr_on(fd);
	dprintf3(DEB_SO, "socks_rd request cmd=%d addr=%s port=%d", rq_cmd(sp),
		 inet_ntoa(rq_addr(sp)), ntohs(rq_port(sp)));
	memcpy(sp->buf+reppos, sp->buf, reqsize);
        /* rp_rsv(sp)=0; NEC extension using that as flags */
	if (rq_ver(sp)!=5) {
	    ++s_protfail;
	    flagerr(7);
	}
	
	if (strncmp(sp->uname, "nxsocksd", 8)==0)
	{
	    /* FF HACK - Just allow CONNECT method, 127.0.0.1:CUPS/NXFISH now.*/

	    if (rq_cmd(sp) != 1 || (strcmp(inet_ntoa(rq_addr(sp)),"127.0.0.1")!=0) || (ntohs(rq_port(sp)) != 631 && ntohs(rq_port(sp)) != 6201))
	    {
		++s_refused;
		flagerr(2);
            }
	}

	switch(rq_cmd(sp)) {
	case 1: /* CONNECT */
	    ++s_rconn;
	    if ((n=nsocket(AF_INET, SOCK_STREAM, 0))<0) {
		++s_fconn;
		flagerr(1);
	    }
            i=2*BUFS;
            if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i))<0)
                perror("CONNECT: warning: SO_SNDBUF"); /* not fatal */
            i=2*BUFS;
            if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i))<0)
                perror("CONNECT: warning: SO_RCVBUF"); /* not fatal */
#ifdef USECPORT_CONNECT
	    sal=sizeof(pa);
	    if (getpeername(fd, (struct sockaddr *)&pa, &sal)<0) {
		perror("(warning) socks_rd request connect");
	    } else {
		pa.sin_addr.s_addr=htonl(INADDR_ANY);
		if (bind(n, (struct sockaddr *)&pa, sal)<0)
		    perror("(warning) socks_rd request connect");
	    }
#endif
	    sp->proxy=n;
	    if ((i=doconnect(sp, 0))>0) {
		close(n);
		sp->proxy=-1;
		++s_fconn;
		flagerr(i);
	    }
	    thread_fd_rd_off(fd);
	    if (i<0) { /* waiting */
		thread_fd_register(n, NULL, proxy_wr, closeboth, sp);
		thread_fd_wr_off(fd);
		trans(st_copening);
	    }
	    /* non-waiting */
	    rp_rep(sp)=0;
	    bufset(sp, reppos, reqsize);
	    trans(st_copened);

	case 2: /* BIND */
	    ++s_rbind;
	    if ((n=nsocket(AF_INET, SOCK_STREAM, 0))<0) {
		++s_fbind;
		flagerr(1);
	    }
            i=2*BUFS;
            if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i))<0)
                perror("BIND: warning: SO_SNDBUF"); /* not fatal */
            i=2*BUFS;
            if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i))<0)
                perror("BIND: warning: SO_RCVBUF"); /* not fatal */
	    sp->proxy=n;
	    if ((i=dobind(sp, 0))>0) {
		close(n);
		sp->proxy=-1;
		++s_fbind;
		flagerr(i);
	    }
	    rp_rep(sp)=0;
	    bufset(sp, reppos, reqsize);
	    trans(st_bopening);

	case 3: /* UDP */
	    ++s_rudp;
	    if ((n=nsocket(AF_INET, SOCK_DGRAM, 0))<0) {
		++s_fudp;
		flagerr(1);
	    }
	    sockaddr_init(&pa);
	    pa.sin_addr=rq_addr(sp);
	    pa.sin_port=rq_port(sp);
	    if (!(a=udp_init(n, &pa, sp->udpclient, sp->udpclientn))) {
		close(n);
		++s_fudp;
		flagerr(1);
	    }
	    sp->proxy=n;
	    /* this is actually a pointer to the wrong type but the only
	       operation used on it is free() anyway */
	    sp->peer=a;
	    rp_rep(sp)=0;
	    rp_addr(sp)=pa.sin_addr;
	    rp_port(sp)=pa.sin_port;
	    bufset(sp, reppos, reqsize);
	    trans(st_uopening);

#ifdef DO_SPAWN
        case 128: /* ping */
        case 129: /* traceroute */
            ++s_rspawn;
            rp_rep(sp)=0;
            bufset(sp, reppos, reqsize);
            thread_fd_wr_on(fd);
            thread_fd_rd_off(fd);
            trans(st_spawn);
#endif

	default:
	    flagerr(7);
	}

    case st_uwaiting:
	/* UDP has closed */
	dprintf1(DEB_CONN, "socks_rd uwaiting close %d", fd);
	closeboth(fd, sp);
	return;

    default:
	eprintf1("socks_rd: warning: listener in state %d", sp->state);
	thread_fd_rd_off(fd);
	return;
    }

 err255:
    memcpy(sp->buf, "\005\377", 2);
    bufset(sp, 0, 2);
 err0:
    thread_fd_rd_off(fd);
    thread_fd_wr_on(fd);
    thread_timer_register(10, closeboth, fd, sp);
    trans(st_err); /* error */

#undef trans
#undef waitcompl
#undef flagerr
}

void socks_wr(int fd, void *a)
{
    socksparm *sp=a;
    socksparm *sp2;

#define trans(x) do{sp->state=(x); return;}while(0)
#define waitcompl() if (!writecompl(fd,sp)) return;

    dprintf2(DEB_SOS, "socks_wr fd %d state %d", fd, sp->state);
    switch(sp->state) {

    case st_wauths:
	waitcompl();
	thread_fd_wr_off(fd);
	if (sp->pass) {
	    bufset(sp, 0, 2);
	    trans(st_ruser);
	}
	bufset(sp, 0, 5);
	trans(st_ratyp);

    case st_wpass:
	waitcompl();
	thread_fd_wr_off(fd);
	bufset(sp, 0, 5);
	trans(st_ratyp);

    case st_copened: /* connect response */
    case st_bopened: /* accept response */
	waitcompl();
	if (!(sp2=malloc(sizeof(socksparm)))) {
	    eprintf0("socks_wr 12: out of memory");
	    closeboth(fd, sp);
	    return;
	}
	dprintf2(DEB_CONN, "socks_wr proxy for %d -> %d", fd, sp->proxy);
	sp->peer=sp2;
	sp2->peer=sp;
	sp2->me=sp->proxy;
	sp2->proxy=fd;
	sp2->state=sp->state=st_running;
	bufset(sp, 0, 0);
	bufset(sp2, 0, 0);
	thread_fd_register(fd, shuffle_rd, shuffle_wr, NULL, sp);
	thread_fd_register(sp->proxy, shuffle_rd, shuffle_wr, NULL, sp2);
	return;

    case st_bopening: /* first bind response */
	waitcompl();
	thread_fd_register(sp->proxy, proxy_rd, NULL, closeboth, sp);
	thread_fd_rd_off(fd);
	thread_fd_wr_off(fd);
	trans(st_bwaiting);

    case st_uopening: /* udp response */
	waitcompl();
	thread_fd_wr_off(fd);
	trans(st_uwaiting);

#ifdef DO_SPAWN
    case st_spawn: /* spawning external process */
        waitcompl();
        dospawn(sp);
        closeboth(fd, sp);
        return;
#endif

    case st_err:
	waitcompl();
	closeboth(fd, sp);
	return;

    default:
	eprintf1("socks_wr: warning: writer in state %d", sp->state);
	thread_fd_wr_off(fd);
	return;
    }

#undef trans
#undef waitcompl
}

/* Set up a SOCKS connection */
void socks_init(int fd, const char *uname, const char *pass,
		struct in_addr *udpclient, int udpclientn)
{
    socksparm *sp=malloc(sizeof(socksparm));
    if (!sp) {
	close(fd);
	return;
    }
    sp->state=st_rinit;
    sp->me=fd;
    sp->proxy=-1;
    sp->peer=NULL;
    sp->uname=uname;
    sp->pass=pass;
    sp->udpclient=udpclient;
    sp->udpclientn=udpclientn;
    bufset(sp, 0, 2);
    thread_fd_register(fd, socks_rd, socks_wr, closeboth, sp);
    thread_fd_wr_off(fd);
}
