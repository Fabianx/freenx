/*
   nxsocksd - user specific SOCKS5 daemon

   udp.c - UDP proxy

   Copyright 1997 Olaf Titz <olaf@bigred.inka.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the License, or (at your option) any later version.
*/
/* $Id: udp.c,v 1.10 1999/05/13 22:28:11 olaf Exp $ */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "thread.h"
#include "log.h"
#include "udp.h"
#include "lib.h"
#include "socks.h"
#include "resolv.h"

#ifndef BUFS
#define BUFS 32768
#endif

typedef struct udp_parm {
    struct in_addr *allow;
    int allown;
    struct sockaddr_in client;
    struct sockaddr_in fromto;
    int pos, len;
    unsigned char buf[BUFS];
} udpparm;

#define u_rsv(b) (*(unsigned short *)(b))
#define u_frag(b) ((b)[2])
#define u_atyp(b) ((b)[3])
#define u_addr(b) (*(struct in_addr *)((b)+4))
#define u_port(b) (*(unsigned short *)((b)+8))

void udp_rd_gotaddr(struct hostent *h, int s, int fd, void *a)
{
    udpparm *up=a;
    dprintf2(DEB_UDPS, "udp_rd_gotaddr fd %d status %d", fd, s);
    if (s!=0) {
	eprintf1("DNS lookup failed: `%s'", up->buf+5);
	return; /* error - don't do anything */
    }
    /* Now we have the address, make us ready for writing */
    memcpy(&(up->fromto.sin_addr), h->h_addr_list[0], sizeof(struct in_addr));
    thread_fd_wr_on(fd);
}

void udp_rd(int fd, void *a)
{
    udpparm *up=a;
    struct sockaddr_in sa;
    size_t sal=sizeof(sa);
    unsigned char *p;

    dprintf1(DEB_UDPS, "udp_rd fd %d", fd);
    if ((up->len=recvfrom(fd, up->buf, BUFS, 0, (struct sockaddr *)&sa,
			  &sal))<0) {
	dprintf1(DEB_UDP, "udp_rd: recvfrom: %s", strerror(errno));
	return;
    }
    if (up->client.sin_addr.s_addr==htonl(INADDR_ANY)) {
        /* assume first packet from allowed clients is real client */
        int i;
        for (i=0; i<up->allown; ++i)
            if (sa.sin_addr.s_addr==up->allow[i].s_addr) {
                dprintf1(DEB_UDP, "udp_rd: got client address %s",
                         inet_ntoa(sa.sin_addr));
                up->client=sa;
                break;
            }
    }
    if (sa.sin_addr.s_addr==up->client.sin_addr.s_addr) {
	if (up->client.sin_port==0) {
	    /* assume first packet from this machine is real client */
	    up->client.sin_port=sa.sin_port;
            dprintf1(DEB_UDP, "udp_rd: got client port %d",
                     ntohs(sa.sin_port));
        }
	if (sa.sin_port!=up->client.sin_port)
	    goto toclient;

	/* from client */
	dprintf0(DEB_UDPS, "udp_rd from client");
	if ((u_rsv(up->buf)!=0) || (u_frag(up->buf)!=0))
	    return;
	sockaddr_init(&(up->fromto));
	switch (u_atyp(up->buf)) {
	case 1:
	    up->fromto.sin_addr=u_addr(up->buf);
	    up->fromto.sin_port=u_port(up->buf);
	    up->pos=10;
	    up->len-=10;
	    break;
	case 3:
	    p=up->buf+5+up->buf[4];
	    up->fromto.sin_port=(p[0]<<8)+p[1];
	    *p=0;
	    up->pos=7+up->buf[4];
	    up->len-=up->pos;
	    dprintf1(DEB_SO, "udp_rd lookup %s", up->buf+5);
	    thread_fd_rd_off(fd);
	    lookup((char *)(up->buf+5), udp_rd_gotaddr, fd, up);
	    return;
	default:
	    return; /* unsupported */
	}
#ifdef DEBUG
	if (debug&DEB_DDUMP)
	    hexdump(d_from, fd, up->buf+up->pos, up->len);
#endif
    } else {
    toclient:
	/* to client */
	dprintf0(DEB_UDPS, "udp_rd to client");
	up->pos=0;
	up->fromto=sa;
#ifdef DEBUG
	if (debug&DEB_DDUMP)
	    hexdump(d_to, fd, up->buf, up->len);
#endif
    }
    thread_fd_rd_off(fd);
    thread_fd_wr_on(fd);
}

void udp_wr(int fd, void *a)
{
    udpparm *up=a;
    struct msghdr m;
    struct iovec iv[2];
    unsigned char shb[10];
    int i;

    memset(&m, 0, sizeof(m));
    m.msg_namelen=sizeof(struct sockaddr_in);
    m.msg_iov=iv;
    dprintf2(DEB_UDPS, "udp_wr fd %d fromto=%s", fd,
	     inet_ntoa(up->fromto.sin_addr));

    if (up->pos) {
	/* from client */
	dprintf0(DEB_UDPS, "udp_wr from client");
	m.msg_name=&(up->fromto);
	iv[0].iov_base=up->buf+up->pos;
	iv[0].iov_len=up->len;
	m.msg_iovlen=1;
    } else {
	/* to client - encode address */
	dprintf0(DEB_UDPS, "udp_wr to client");
	m.msg_name=&(up->client);
	u_rsv(shb)=0;
	u_frag(shb)=0;
	u_atyp(shb)=1;
	u_addr(shb)=up->fromto.sin_addr;
	u_port(shb)=up->fromto.sin_port;
	iv[0].iov_base=shb;
	iv[0].iov_len=sizeof(shb);
	iv[1].iov_base=up->buf;
	iv[1].iov_len=up->len;
	m.msg_iovlen=2;
    }
#ifdef DEBUG
    {
	struct sockaddr_in *sa=(struct sockaddr_in *)m.msg_name;
	dprintf2(DEB_UDPS, "udp_wr to %s:%d",
		 inet_ntoa(sa->sin_addr), ntohs(sa->sin_port));
    }
#endif
    if ((i=sendmsg(fd, &m, 0))<0)
	dprintf1(DEB_UDP, "udp_wr: sendmsg: %s", strerror(errno));
    thread_fd_wr_off(fd);
    thread_fd_rd_on(fd);
}

void *udp_init(int fd, struct sockaddr_in *pa, struct in_addr *allow,
               int allown)
{
    struct sockaddr_in sa;
    size_t sal=sizeof(sa);
    int i;
    udpparm *up=malloc(sizeof(udpparm));
    if (!up) {
	eprintf0("udp_init: out of memory");
	close(fd);
	return NULL;
    }

    dprintf3(DEB_UDP, "udp_init %d %s:%d", fd,
	     inet_ntoa(pa->sin_addr), ntohs(pa->sin_port));
    assert(allown>0);
    if (pa->sin_addr.s_addr != htonl(INADDR_ANY)) {
        for (i=0; i<allown; ++i)
            if (pa->sin_addr.s_addr==allow[i].s_addr)
                goto okay;
	eprintf0("udp_init: disallowed client address");
	goto err;
    }
 okay:

    up->allow=allow;
    up->allown=allown;
    sockaddr_init(&(up->client));
    if ((allown==1) && (pa->sin_addr.s_addr==htonl(INADDR_ANY)))
        up->client.sin_addr=allow[0]; /* it's unique */
    else
        up->client.sin_addr=pa->sin_addr;
    up->client.sin_port=pa->sin_port;

    sockaddr_init(&sa);
#ifdef USECLIENTSPORT
    sa.sin_port=pa->sin_port;
#else
    sa.sin_port=0;
#endif
    sa.sin_addr=myaddress;
    dprintf2(DEB_UDPS, "udp_init want %s:%d", inet_ntoa(sa.sin_addr),
	     ntohs(sa.sin_port));
#ifdef USECLIENTSPORT
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa))<0) {
	eprintf1("udp_init: could not bind %d, trying different port",
		 ntohs(sa.sin_port));
	sa.sin_port=0;
#endif
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa))<0) {
	    perror("udp_init: bind");
	    goto err;
	}
#ifdef USECLIENTSPORT
    }
#endif
    if (getsockname(fd, (struct sockaddr *)&sa, &sal)<0) {
	perror("udp_init: getsockname");
	goto err;
    }
    dprintf2(DEB_UDPS, "udp_init got %s:%d", inet_ntoa(sa.sin_addr),
	     ntohs(sa.sin_port));
    i=BUFS;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i))<0)
        perror("UDP: warning: SO_SNDBUF"); /* not fatal */
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i))<0)
        perror("UDP: warning: SO_RCVBUF"); /* not fatal */

    *pa=sa;
    thread_fd_register(fd, udp_rd, udp_wr, NULL, up);
    thread_fd_wr_off(fd);
    return up;

 err:
    close(fd);
    free(up);
    return NULL;
}
