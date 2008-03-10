#ifndef _socks_h_
#define _socks_h_

extern struct in_addr myaddress;

extern void socks_init(int fd, const char *user, const char *pass,
		       struct in_addr *udpclient, int udpclientn);

#endif
