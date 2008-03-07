#ifndef _udp_h_
#define _udp_h_

extern void *udp_init(int fd, struct sockaddr_in *pa, struct in_addr *allow,
                      int allown);

#endif
