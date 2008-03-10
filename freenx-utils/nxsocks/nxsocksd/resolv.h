#ifndef _resolv_h_
#define _resolv_h_

#include <netdb.h>

/* The callback function which is called when a resolver operation is
   finished. "h" holds the result in static memory, only valid when 
   "state" is NOERROR. "fd" and "parm" are copied from the invoker */
typedef void (*rescall)(struct hostent *h, int state, int fd, void *parm);

/* "state" is a response code as of <arpa/nameser.h> or one of
   the following, in both cases taken negative */
#define NOREVERSE  95
#define NOANSWER   96
#define OUTOFMEM   97
#define TIMEOUT    98
#define GENFAIL    99

/* Init the module (returns -1 on error) */
extern int resolv_init(void);

/* Translate state to description */
extern const char *resolv_strerr(int s);

/* Lookup by name (query for A record) */
extern void lookup(const char *c, rescall rch, int fd, void *parm);

/* Lookup by address (query for PTR record) */
extern void rlookup(struct in_addr a, rescall rch, int fd, void *parm);

#endif
