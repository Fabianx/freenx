/* An "nsquery"-like resolver client to test and show usage of resolv.c. */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <stdlib.h>
#include <unistd.h>

extern int optind;
extern char *optarg;

#include "thread.h"
#include "resolv.h"
#include "log.h"

#ifdef DEBUG
int debug=DEB_RES;
static int ghbn=0;
static int rpt=1;
extern void dumpcache(void);
#endif

static int rcnt=0;
static int args_1, args_2;
static char **args_v;
void querythem(int fd, void *parm);

void cb(struct hostent *he, int state, int fd, void *parm)
{
    char *c=parm;
    char **n;
    struct in_addr **a;

    printf("Looking for %s:\n", c);
    if (state==0) {
	printf    ("  name:    %s\n", he->h_name);
	n=he->h_aliases;
	while(*n)
	    printf("  alias:   %s\n", *n++);
	a=(struct in_addr **)he->h_addr_list;
	while(*a)
	    printf("  address: %s\n", inet_ntoa(*(*a++)));
    } else {
	printf    ("  error: %s\n", resolv_strerr(state));
    }
    if (--rcnt<=0) {
#ifdef DEBUG
	if (--rpt>0) {
	    dumpcache();
	    thread_timer_register(5, querythem, 0, NULL);
	} else
#endif
	    exit(0);
    }
}

int reverse(const char *c, int n)
{
    struct in_addr a;
    if (inet_aton(c, &a)==0)
	return 0; /* no reverse lookup */
    rlookup(a, cb, n, (void *)c);
    return 1;
}

void querythem(int fd, void *parm) /* dummy args */
{
    int i;
    for (i=args_1; i<args_2; ++i) {
	++rcnt;
	if (!reverse(args_v[i], i)) {
#ifdef DEBUG
	    if (ghbn) {
		/* debug: call gethostbyname(), to compare the results */
		struct hostent *he=gethostbyname(args_v[i]);
		++rcnt;
		printf("gethostbyname result:\n");
		cb(he, (he) ? 0 : -h_errno, i, args_v[i]);
	    }
#endif
	    lookup(args_v[i], cb, i, args_v[i]);
	}
#ifdef DEBUG
	else if (ghbn) {
	    unsigned long a=inet_addr(args_v[i]);
	    struct hostent *he=gethostbyaddr((char *)&a, sizeof(a), AF_INET);
	    ++rcnt;
	    printf("gethostbyaddr result:\n");
	    cb(he, (he) ? 0 : -h_errno, i, args_v[i]);
	}
#endif
    }
}

int main(int argc, char *argv[])
{
    int i;


    setunbuf(stdout);
    setunbuf(stderr);
    while ((i=getopt(argc, argv, "d:r:g"))!=EOF) {
	switch(i) {
#ifdef DEBUG
	case 'd': debug=atoi(optarg); break;
	case 'r': rpt=atoi(optarg); break;
	case 'g': ++ghbn; break;
#endif
	default: fprintf(stderr, "unknown option: %c\n", i);
	}
    }
    if ((thread_init()<0) || (resolv_init()<0)) {
	eprintf0("main: out of memory, a hopeless case");
	exit(1);
    }
    args_1=optind; args_2=argc; args_v=argv;
    querythem(0, NULL);
    return thread_mainloop();
}
