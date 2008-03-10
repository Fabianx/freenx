/*
   nxsocksd - user specific SOCKS5 daemon

   resolv.c - asynchronous domain name resolver

   Copyright 1997 Olaf Titz <olaf@bigred.inka.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the License, or (at your option) any later version.
*/
/* $Id: resolv.c,v 1.9 1998/11/29 22:51:57 olaf Exp $ */

#include "config.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#ifdef RESOLV_NEEDS_STDIO
#include <stdio.h>
#endif
#include <resolv.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

extern struct RES_STATE _res;

#include "thread.h"
#include "resolv.h"
#include "log.h"
#include "lib.h"

#ifndef MAXTTL
/* upper bound on rrset TTLs */
#define MAXTTL 86400
#endif

#ifndef BADTTL
/* TTL for errors */
#define BADTTL 60
#endif

const char *resolv_strerr(int s)
{
    switch(abs(s)) {
	/* my errors */
    case NOREVERSE:  return "No reverse mapping";
    case NOANSWER:   return "No name of this type";
    case OUTOFMEM:   return "Out of memory";
    case TIMEOUT:    return "Timed out";
    case GENFAIL:    return "General resolver failure";
	/* DNS errors */
    case FORMERR:    return "Format error";
    case SERVFAIL:   return "Server failure";
    case NXDOMAIN:   return "Nonexistant domain";
    case NOTIMP:     return "Query not implemented";
    case REFUSED:    return "Query refused";
	/* Etc. */
    case NOERROR:    return "No error";
    default:         return "Unknown error";
    }
}

/*** Caching stuff ***/

#define HASHSIZE 257 /* best primes: (151) 257 (331) 641 1039 */

typedef struct hash_t {
    struct hash_t *next;      /* next link */
    char *name;               /* the name */
    int typ;                  /* query type */
    struct hostent hent;      /* answer; only valid if status==0 */
    int status;               /* status */
    time_t attl;              /* TTL timeout, absolute value */
} hash;

static hash **ht=NULL;


/* Free a null-terminated list */
#define lfree(typ,p) { typ **x=p; while (*x) { free(*x); ++x; }; free(p); }

/* Free the indirect components in an hostent */
static void hefree(struct hostent *h)
{
    if (h->h_name)
	/* cast spares us compiler warnings, depending on header pickiness */
	free((void *)(h->h_name));
    if (h->h_aliases)
	lfree(char, h->h_aliases);
    if (h->h_addr_list)
	/* Don't tell me assuming sizeof(struct in_addr*)==sizeof(char*) is
	   portable. If it isn't, probably <netdb.h> falls to pieces anyway */
	lfree(struct in_addr, (struct in_addr **)(h->h_addr_list));
}

/* Hash function on a domain name.
   Experiments have shown that this primitive rotate-xor type hash does not
   distribute much worse than taking a CRC, which is what dbz uses.
   BIND uses an even simpler rotate-xor hash than this one.
   So I decided that this is good enough. */
static int hn(const char *n, int t)
{
    register unsigned long i=t;
    register int s;
    while (*n) {
	s=(((*n+i)&7)<<1)+3;
	i^=*n++;
	i=(i<<s)^(i>>(32-s));
    }
    return i%HASHSIZE;
}

/* Look for "name" in the cache. 0=success, 1=needs lookup.
   Leave in "*ret" pointer to the found hash structure if 0,
   pointer to previous link (only "next" valid), else. */
static int hashlook(const char *name, int t, hash **ret)
{
    hash *p0=(hash *)&ht[hn(name,t)];
    hash *p=p0->next;
    while (p) {
	if ((t==p->typ) && (!strcmp(name, p->name))) {
	    *ret=p;
	    return 0;
	}
	p0=p; p=p->next;
    }
    *ret=p0;
    return 1;
}

#ifdef DEBUG
/* Show contents of cache. */
void dumpcache(void)
{
    int i;
    hash *p;
    time_t t0=time(0);
    for (i=0; i<HASHSIZE; ++i) {
	if (ht[i]) {
	    printf("dumpcache: %d ", i);
	    for (p=ht[i]; p; p=p->next)
		printf("%s(%d):%d:%ld ", p->name, p->typ,
		       p->status, p->attl-t0);
	    printf("\n");
	}
    }
}
#endif


/*** DNS resolver ***/

typedef struct res_parm {
    time_t retrans;           /* Query timeout */
    int retry;                /* Query retry count (goes down) */
    int smask;                /* server status bitmap */
    int thens;                /* counter */
    hash *hashp;              /* Cache slot for this query */
    rescall callback;         /* Callback */
    int callfd;               /* Parameter for callback */
    void *callpar;            /* dito */
    int qlen, alen;           /* Query/answer length */
    char sndbuf[PACKETSZ];    /* Send buffer */
    char rcvbuf[PACKETSZ];    /* Receive buffer */
} resparm;


/* Answer parsing */

/* Append "name" at the end of the "names" list, if not already in list */
static int insname(char **names, const char *name)
{
    char **p=names;
    while (*p) {
	if (!strcmp(*p, name))
	    return 0;
	++p;
    }
    if (!(*p++=strdup(name)))
	return -OUTOFMEM;
#if 0
    *p=NULL;
#endif
    return 0;
}

/* Process resource record starting at "p" in "rp"'s receive buffer,
   store addresses in the "addrs" list of "naddrs" current length,
   and names in the "names" list */
static int getrr(resparm *rp, char **p, char **names,
		 struct in_addr **addrs, int *naddrs)
{
    char buf[MAXDNAME];
    int i, typ, ttl;
    struct in_addr *ap;

    /* This RR's name */
    if ((i=dn_expand(rp->rcvbuf, rp->rcvbuf+rp->alen, *p, buf, MAXDNAME))<0)
	return -FORMERR;
    (*p)+=i;
    if ((i=insname(names, buf))<0)
	return i;

    GETSHORT(typ, *p); /* type */
    GETSHORT(i, *p);   /* class */
    if (i!=C_IN)
	return 0; /* eh? */
    GETLONG(ttl, *p);  /* TTL */
    GETSHORT(i, *p);   /* length (unused) */
    switch (typ) {
    case T_A:
	if (!(ap=malloc(sizeof(struct in_addr))))
	    return -OUTOFMEM;
	/* no direct assignment from *p because of alignment problems */
        GETLONG(i, *p);
        ap->s_addr=htonl(i);
	addrs[(*naddrs)++]=ap;
#if 0
	addrs[*naddrs]=NULL;
#endif
	break;
    case T_PTR:
    case T_CNAME:
	if ((i=dn_expand(rp->rcvbuf, rp->rcvbuf+rp->alen,
			 *p, buf, MAXDNAME))<0)
	    return -FORMERR;
	(*p)+=i;
	if ((i=insname(names, buf))<0)
	    return i;
	/* Note: depending on the order of CNAME records in the answer,
	   the "true" canonical name may be the wrong one. BIND apparently
	   sorts them right in its answers, though. */
	break;
    default:
	return 0; /* ignore */
    }
    return ttl;
}

/* Maximum RRs to fit in one packet (rough upper bounds) */
#define MAX_NAMES ((PACKETSZ-sizeof(HEADER)-4)/12)
#define MAX_ADDRS ((PACKETSZ-sizeof(HEADER)-4)/14)

/* Process a DNS answer */
static void gotanswer(resparm *rp)
{
    HEADER *h=(HEADER *)rp->rcvbuf;
    char buf0[MAXDNAME], buf1[MAXDNAME];
    int l0, l1, i, e;
    char *p0, *p1;
    struct hostent he;
    char **names=NULL;
    char **nn;
    struct in_addr **addrs=NULL;
    int naddrs=0;

    /* is this really our request? compare QNAME */
    e=-GENFAIL;
    if (((l0=dn_expand(rp->sndbuf, rp->sndbuf+rp->qlen,
		       rp->sndbuf+sizeof(HEADER), buf0, MAXDNAME))<0) ||
	((l1=dn_expand(rp->rcvbuf, rp->rcvbuf+rp->alen,
		       rp->rcvbuf+sizeof(HEADER), buf1, MAXDNAME))<0) ||
	(strcmp(buf0, buf1)))
	goto failed;

    p0=rp->sndbuf+sizeof(HEADER)+l0;
    p1=rp->rcvbuf+sizeof(HEADER)+l1;
    GETSHORT(l0, p0); GETSHORT(l1, p1); /* compare QTYPE */
    if (l0!=l1)
	goto failed;
    GETSHORT(l0, p0); GETSHORT(l1, p1); /* compare QCLASS */
    if (l0!=l1)
	goto failed;

    e=-OUTOFMEM;
    if ((!(names=calloc(MAX_NAMES, sizeof(char *)))) ||
	(!(addrs=calloc(MAX_ADDRS, sizeof(struct in_addr *)))))
	goto failed;

    l1=MAXTTL;
    dprintf1(DEB_RES, "gotanswer: ancount=%d", ntohs(h->ancount));
    for (i=ntohs(h->ancount); i>0; --i) {
	if ((e=getrr(rp, &p1, names, addrs, &naddrs))<0)
	    goto failed;
	if ((e>0) && (l1>e)) /* look for minimum ttl */
	    l1=e;
    }
    dprintf1(DEB_RES, "gotanswer: ttl=%d", l1);

    /* Return the answer in an hostent. */
    nn=names;
    while (nn[1])
	++nn;
    he.h_name=*nn; /* last name in list is canonical */
    *nn=NULL;
    he.h_aliases=names;
    he.h_addrtype=AF_INET;
    he.h_length=sizeof(struct in_addr);
    he.h_addr_list=(char **)addrs;
    if (rp->hashp->status!=1)
	/* another lookup has filled the cache slot. Throw away the old one. */
	hefree(&(rp->hashp->hent));
    rp->hashp->hent=he;
    rp->hashp->status=0;
    rp->hashp->attl=time(0)+l1;
    rp->callback(&he, NOERROR, rp->callfd, rp->callpar);
    return;

 failed:
    rp->hashp->status=e;
    rp->hashp->attl=time(0)+BADTTL;
    rp->callback(NULL, e, rp->callfd, rp->callpar);
    if (names)
	lfree(char, names);
    if (addrs)
	lfree(struct in_addr, addrs);
}

/* Resolver query handler routines */

/* An error occurred during the query/response process. */
void resolv_er(int fd, void *a)
{
    resparm *rp=a;
    dprintf1(DEB_RES, "resolv_er: fd=%d", fd);
    thread_fd_close(fd);
    thread_timer_cancel(fd);
    rp->hashp->status=-TIMEOUT;
    rp->hashp->attl=time(0)+BADTTL;
    rp->callback(NULL, -TIMEOUT, rp->callfd, rp->callpar);
    free(rp);
}

/* Query timeout. Restart or return error. */
void resolv_to(int fd, void *a)
{
    resparm *rp=a;
    dprintf2(DEB_RES, "resolv_to: fd=%d, r=%d", fd, rp->retry);
    if (--rp->retry<=0) {
	resolv_er(fd, a);
	return;
    }
    rp->retrans<<=1;
    thread_fd_wr_on(fd);
    thread_timer_register(rp->retrans, resolv_to, fd, a);
}

/* Resolver read handler. */
void resolv_rd(int fd, void *a)
{
    resparm *rp=a;
    HEADER *h0=(HEADER *)rp->sndbuf;
    HEADER *h=(HEADER *)rp->rcvbuf;
    struct sockaddr_in sa;
    size_t sal=sizeof(sa);
    int i, e=-GENFAIL;

    dprintf1(DEB_RES, "resolv_rd: fd=%d", fd);
    if ((rp->alen=recvfrom(fd, rp->rcvbuf, PACKETSZ, 0,
			   (struct sockaddr *)&sa, &sal))<=0) {
	resolv_er(fd, a);
	return;
    }
    /* came from known server? */
    for (i=0; i<_res.nscount; ++i) {
	if (_res.nsaddr_list[i].sin_addr.s_addr==INADDR_ANY ||
            _res.nsaddr_list[i].sin_addr.s_addr==sa.sin_addr.s_addr)
	    break;
    }
    if (i>=_res.nscount) {
	eprintf1("resolv_rd: unexpected answer from %s",
		 inet_ntoa(sa.sin_addr));
	return;
    }
    if (h->id!=h0->id)
	return; /* was a different query, wait for next answer */

    /* check for various errors */
    if ((h->tc) || (!h->qr) || (!h->ra) || (ntohs(h->qdcount)!=1))
	goto serverr;

    /* DNS detected errors? */
    if (h->rcode!=NOERROR)
	e=-h->rcode;
    else if (ntohs(h->ancount)==0)
	e=-NOANSWER;
    else {
	/* Answer seems good, process */
	gotanswer(rp);
	goto finish;
    }
    if (h->aa)
	/* Authoritative error */
	goto aserverr;

 serverr:
    /* This server has returned error or bogus answer */
    rp->smask &= ~(1<<i);
    if (rp->smask)
	return; /* still other servers left */

 aserverr:
    /* Authoritative error or no server left to query */
    rp->hashp->status=e;
    rp->hashp->attl=time(0)+BADTTL;
    rp->callback(NULL, e, rp->callfd, rp->callpar);

 finish:
    /* Now done with the request */
    thread_fd_close(fd);
    thread_timer_cancel(fd);
    free(rp);
}

/* Resolver write handler: send a query and reset timeout */
void resolv_wr(int fd, void *a)
{
    resparm *rp=a;
    dprintf1(DEB_RES, "resolv_wr: fd=%d", fd);
    if ((rp->smask & (1<<rp->thens)) && /* send only if marked as active */
	(sendto(fd, rp->sndbuf, rp->qlen, 0,
		(struct sockaddr *)&_res.nsaddr_list[rp->thens],
		sizeof(struct sockaddr_in))!=rp->qlen)) {
	resolv_er(fd, a);
	return;
    }
    if (++rp->thens>=_res.nscount) {
	/* start next round of queries after timeout */
	rp->thens=0;
	thread_fd_wr_off(fd);
	thread_timer_cancel(fd);
	thread_timer_register(rp->retrans, resolv_to, fd, a);
    }
}

/* Start a DNS lookup. Prepare to call "rch" when complete. */
static int startlookup(const char *c, int typ, hash *hp,
		       rescall rch, int rfd, void *rpar)
{
    int fd;
    resparm *rp=malloc(sizeof(resparm));
    if (!rp)
	return -OUTOFMEM;
    if (((fd=nsocket(AF_INET, SOCK_DGRAM, 0))<0) ||
	((rp->qlen=res_mkquery(QUERY, c, C_IN, typ, NULL, 0, NULL,
			       rp->sndbuf, PACKETSZ))<0)) {
	eprintf1("startlookup: %s", strerror(errno));
	free(rp);
	close(fd);
	return -GENFAIL;
    }
    dprintf2(DEB_RES, "startlookup: `%s' typ=%d", c, typ);
    hp->status=1; /* lookup in progress */
    hp->attl=time(0);
    rp->retrans=_res.retrans;
    rp->retry=_res.retry;
    rp->smask=(1<<_res.nscount)-1;
    rp->thens=0;
    rp->hashp=hp;
    rp->callback=rch;
    rp->callfd=rfd;
    rp->callpar=rpar;
    thread_fd_register(fd, resolv_rd, resolv_wr, NULL, rp);
    thread_timer_register(_res.retrans, resolv_er, fd, rp);
    return 1; /* in progress */
}


/*** Generic lookup, using cache ***/

/* Do a lookup on the name "c" of type "typ" (T_A or T_PTR).
   Call "rch" with a hostent and the supplied parameters when complete */
static void glookup(const char *c, int typ, rescall rch, int fd, void *parm)
{
    hash *p, *p1;
    char *c1;
    int e=NOERROR;

    if (!hashlook(c, typ, &p)) {
	/* from cache */
	if (time(0)<p->attl) {
	    dprintf1(DEB_RES, "glookup: from cache: `%s'", c);
	    e=p->status;
	    goto immed;
	}
	hefree(&(p->hent));
	/* ttl expired: fall through to lookup */
    } else {
	/* insert new element into cache */
	if ((!(p1=malloc(sizeof(hash)))) ||
	    (!(c1=strdup(c)))) {
	    e=-OUTOFMEM; goto immed;
	}
	p->next=p1;
	p=p1;
	p->next=NULL;
	p->name=c1;
	p->typ=typ;
    }
    p->status=1;
    p->attl=time(0);
    e=startlookup(c, typ, p, rch, fd, parm);
    if (e>0)
	return;

 immed:
    rch((e==0) ? &(p->hent) : NULL, e, fd, parm);
}

/* Do a forward query - gethostbyname() equivalent */
void lookup(const char *c, rescall rch, int fd, void *parm)
{
    struct in_addr a;
    if (inet_aton(c, &a)) {
	/* four-numbers literal */
	struct hostent he;
	char *hx[2]={ NULL, NULL };
	hx[0]=(char*)&a; /* stupid Solaris cc can't use initializer here  */
	/* De-const-ing the pointer is technically incorrect, but
	   errors in applications that try to modify *(he.h_name)
	   _should_ be caught by the compiler anyway, if only the
	   headers were always right... else we wouldn't need the cast.
	   Remember too that this branch is absolutely nonstandard. */
	he.h_name=(char *)c;
	he.h_aliases=hx+1; /* nice place to find a NULL */
	he.h_addrtype=AF_INET;
	he.h_length=sizeof(a);
	he.h_addr_list=hx;
	rch(&he, 0, fd, parm);
    } else {
	if (!strchr(c, '.')) {
	    /* If the name contains no dot, append the default domain. */
	    char buf[MAXDNAME];
	    if ((!_res.defdname) ||
		(strlen(c)+strlen(_res.defdname)>MAXDNAME-2)) {
		/* name is too long or no default domain */
		rch(NULL, -GENFAIL, fd, parm);
		return;
	    }
	    sprintf(buf, "%s.%s", c, _res.defdname);
	    glookup(buf, T_A, rch, fd, parm);
	} else {
	    glookup(c, T_A, rch, fd, parm);
	}
    }
}


/*** The reverse lookup ***/

typedef struct rev_info {
    struct in_addr ra;        /* address searched for */
    rescall rcb;              /* original callback */
    int rfd;                  /* ...and its parameters */
    void *rparm;
} revinfo;

/* Second reverse callback, activated from the verification A lookup */
void revcb2(struct hostent *h, int state, int fd, void *parm)
{
    revinfo *ri=parm;
    struct in_addr **sa;
    dprintf1(DEB_RES, "revcb2: state=%d", state);

    if (state!=0)
	goto callit;
    sa=(struct in_addr **)(h->h_addr_list);
    while (*sa) {
	if ((*sa)->s_addr==ri->ra.s_addr)
	    goto callit;
	++sa;
    }
    /* verification failed */
    state=-NOREVERSE;

 callit:
    /* call the real callback */
    ri->rcb(h, state, ri->rfd, ri->rparm);
    free(ri);
}

/* First reverse callback, activated from the PTR lookup */
void revcb1(struct hostent *h, int state, int fd, void *parm)
{
    revinfo *ri=parm;
    dprintf1(DEB_RES, "revcb1: state=%d", state);
    if (state!=0) {
	ri->rcb(NULL, state, ri->rfd, ri->rparm);
	free(ri);
	return;
    }
    /* Start a verification A lookup */
    glookup(h->h_name, T_A, revcb2, fd, ri);
}

/* Do a reverse query - gethostbyaddr() equivalent */
void rlookup(struct in_addr a, rescall rch, int fd, void *parm)
{
    char buf[32];
    unsigned char *x=(unsigned char *)&a;
    revinfo *ri=malloc(sizeof(revinfo));
    if (!ri) {
	rch(NULL, -OUTOFMEM, fd, parm);
	return;
    }
    ri->ra=a;
    ri->rcb=rch;
    ri->rfd=fd;
    ri->rparm=parm;
    sprintf(buf, "%d.%d.%d.%d.IN-ADDR.ARPA", x[3], x[2], x[1], x[0]);
    glookup(buf, T_PTR, revcb1, fd, ri);
}


/*** Module initialization ***/

int resolv_init(void)
{
    int i;
    if (res_init()<0)
	return -1;
    _res.options|=RES_RECURSE; /* we rely on this */
    if (!(ht=malloc(HASHSIZE*sizeof(hash *))))
	return -1;
    for (i=0; i<HASHSIZE; ++i)
	ht[i]=NULL;
    return 0;
}
