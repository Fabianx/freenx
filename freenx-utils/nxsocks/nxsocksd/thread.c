/*
   nxsocksd - user specific SOCKS5 daemon

   thread.c - generic I/O multiplexing module

   Copyright 1997 Olaf Titz <olaf@bigred.inka.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the License, or (at your option) any later version.
*/
/* $Id: thread.c,v 1.10 1999/05/13 22:28:11 olaf Exp $ */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "thread.h"
#include "log.h"
#include "lib.h"

static fd_set mfd_ma, mfd_rd, mfd_wr, mfd_ex;

static struct registry {
    handler rdhand, wrhand, exhand;
    void *parm;
} *regs=NULL;

static struct timer {
    struct timer *next;
    time_t tim;
    handler doit;
    int id;
    void *parm;
} *tpending=NULL;

static int nregs, maxfd;

/* Initialize the module. */
int thread_init(void)
{
    dprintf0(DEB_THR, "thread_init");
    if (((nregs=getdtablesize())>0) &&
	((regs=calloc(nregs, sizeof(struct registry))))) {
	FD_ZERO(&mfd_ma);
	FD_ZERO(&mfd_rd);
	FD_ZERO(&mfd_wr);
	FD_ZERO(&mfd_ex);
	maxfd=0;
	return 0;
    }
    return -1;
}

/* Check which fds are valid. Theoretically, a call of this is caused
   by an internal error of the program (not this module) */
static void checkfds(void)
{
    fd_set fds;
    struct timeval tv;
    int i=getdtablesize();

    eprintf0("WARNING: select signalling inconsistency. Checking fdsets");
    while (i>0) {
	if (!FD_ISSET(i-1, &mfd_ma))
	    continue;
	FD_ZERO(&fds);
	FD_SET(i-1, &fds);
	tv.tv_sec=0;
	tv.tv_usec=1;
	if ((select(i, &fds, NULL, NULL, &tv)<0) && (errno==EBADF)) {
	    eprintf1(" closing %d", i-1);
	    FD_CLR(i-1, &mfd_ma);
	    FD_CLR(i-1, &mfd_rd);
	    FD_CLR(i-1, &mfd_wr);
	    FD_CLR(i-1, &mfd_ex);
	}
	--i;
    }
    eprintf0("Check complete.");
}

#ifdef DEBUG
void printfd(int m, fd_set *f)
{
    int i;
    for (i=0; i<m; ++i)
	if (FD_ISSET(i, f))
	    printf("%d ", i);
}
#endif

sig_atomic_t thread_stop;

/* The main loop.
   For each run, look what is active, call select,
   then call the handlers for the active fds,
   then call expired timers. */
int thread_mainloop(void)
{
    fd_set fd_rd, fd_wr, fd_ex;
    int i, n;
    struct timeval t;
    struct timer *p;

    if (!regs)
	return -1; /* initialized? - sanity */
    thread_stop=0;
    while (!thread_stop) {
	fd_rd=mfd_rd;
	fd_wr=mfd_wr;
	fd_ex=mfd_ex;
	if (tpending) {
	    t.tv_sec=tpending->tim-time(0);
	    t.tv_usec=0;
	}
#ifdef DEBUG
	if (debug&DEB_THRTR) {
	    printf("thread_mainloop: maxfd=%d", maxfd);
	    printf("  rd( "); printfd(maxfd, &fd_rd);
	    printf(")  wr( "); printfd(maxfd, &fd_wr);
	    printf(")  ex( "); printfd(maxfd, &fd_ex);
	    printf(")  to=%ld\n", (tpending) ? t.tv_sec : -1L);
	}
#endif
	if ((n=select(maxfd, &fd_rd, &fd_wr, &fd_ex,
		      (tpending) ? &t : NULL))<0) {
	    switch (errno) {
	    case EBADF: checkfds(); break;
            case EINTR: break;
	    default: perror("select");
	    }
	    continue;
	}
#ifdef DEBUG
	if (debug&DEB_THRTR) {
	    printf("select->  rd( "); printfd(maxfd, &fd_rd);
	    printf(")  wr( "); printfd(maxfd, &fd_wr);
	    printf(")  ex( "); printfd(maxfd, &fd_ex);
	    printf(")\n");
	}
#endif
	for (i=0; i<maxfd; ++i) {
	    if (FD_ISSET(i, &fd_rd)&&FD_ISSET(i, &mfd_rd)) {
		dprintf1(DEB_THRTR, "rdhand %d", i);
		regs[i].rdhand(i, regs[i].parm);
	    }
	    if (FD_ISSET(i, &fd_wr)&&FD_ISSET(i, &mfd_wr)) {
		dprintf1(DEB_THRTR, "wrhand %d", i);
		regs[i].wrhand(i, regs[i].parm);
	    }
	    if (FD_ISSET(i, &fd_ex)&&FD_ISSET(i, &mfd_ex)) {
		dprintf1(DEB_THRTR, "exhand %d", i);
		regs[i].exhand(i, regs[i].parm);
	    }
	}
	while (tpending) {
	    if (time(0)<tpending->tim)
		break;
	    /* dequeue a timer event */
	    p=tpending;
	    tpending=p->next;
	    dprintf1(DEB_THRTR, "timhand %d", p->id);
	    p->doit(p->id, p->parm);
	    free(p);
	}
    }
    return 0;
}

/* Register handler routines for a file descriptor. */
void thread_fd_register(int fd, handler rdh, handler wrh, handler exh,
			void *parm)
{
    dprintf1(DEB_THR, "thread_fd_register %d", fd);
    FD_SET(fd, &mfd_ma);
    if ((regs[fd].rdhand=rdh))
	FD_SET(fd, &mfd_rd);
    else
	FD_CLR(fd, &mfd_rd);
    if ((regs[fd].wrhand=wrh))
	FD_SET(fd, &mfd_wr);
    else
	FD_CLR(fd, &mfd_wr);
    if ((regs[fd].exhand=exh))
	FD_SET(fd, &mfd_ex);
    else
	FD_CLR(fd, &mfd_ex);
    regs[fd].parm=parm;
    if (fd+1>maxfd)
	maxfd=fd+1;
}

/* Activate/deactivate handlers. */

void thread_fd_rd_off(int fd)
{
    FD_CLR(fd, &mfd_rd);
}

void thread_fd_rd_on(int fd)
{
    if (regs[fd].rdhand)
	FD_SET(fd, &mfd_rd);
}

void thread_fd_wr_off(int fd)
{
    FD_CLR(fd, &mfd_wr);
}

void thread_fd_wr_on(int fd)
{
    if (regs[fd].wrhand)
	FD_SET(fd, &mfd_wr);
}

/* Close an fd and deactivate its handlers. */
int thread_fd_close(int fd)
{
    dprintf1(DEB_THR, "thread_fd_close %d", fd);
    FD_CLR(fd, &mfd_ma);
    FD_CLR(fd, &mfd_rd);
    FD_CLR(fd, &mfd_wr);
    FD_CLR(fd, &mfd_ex);
    while ((maxfd>0) && (!FD_ISSET(maxfd-1, &mfd_ma)))
	--maxfd;
    return close(fd);
}

/* Register a timer event. */
int thread_timer_register(time_t secs, handler toh, int id, void *parm)
{
#ifdef DEBUG
    int c=0;
#endif
    time_t tim=time(0)+secs;
    struct timer *p=tpending, *p0=(struct timer *)&tpending; /* XX */
    while (p && (p->tim<=tim)) {
	p0=p; p=p->next;
#ifdef DEBUG
	++c;
#endif
    }
    if (!(p=malloc(sizeof(struct timer))))
	return -1;

    dprintf3(DEB_THR, "thread_timer_register %d time=%ld pos=%d", id, tim, c);
    p->next=p0->next;
    p->tim=tim;
    p->doit=toh;
    p->id=id;
    p->parm=parm;

    p0->next=p;
    return 0;
}

/* Cancel all timer events for a given ID. */
void thread_timer_cancel(int id)
{
    struct timer *p=tpending, *p0=(struct timer *)&tpending; /* XX */
    dprintf1(DEB_THR, "thread_timer_cancel %d", id);
    while (p) {
	if (p->id==id) {
	    p0->next=p->next;
	    free(p);
	    p=p0->next;
	} else {
	    p0=p; p=p->next;
	}
    }
}
