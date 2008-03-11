/*
 * libnxredir - Redirect certain ports to other forwarded ports.
 *
 * Copyright (c) 2005-2008 by Fabian Franz <freenx@fabian-franz.de>.
 *
 * License: GPL, version 2
 *
 * Based on TSOCKS - Wrapper library for transparent SOCKS
 *
 * Copyright (C) 2000 Shaun Clowes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

char *progname = "libnxredir";              /* Name used in err msgs    */

#define _GNU_SOURCE

/* Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

#include <sys/un.h>

#define CONNECT_SIGNATURE int __fd, const struct sockaddr * __addr, socklen_t __len

static int (*realconnect)(CONNECT_SIGNATURE);

/* Exported Function Prototypes */
void _init(void);

int connect(CONNECT_SIGNATURE);

void _init(void) 
{
        realconnect = dlsym(RTLD_NEXT, "connect");
}

int is_local(struct in_addr *testip) {

	if (testip->s_addr == htonl(0x7f000001))
		return(1);

        return(0);
}

int connect(CONNECT_SIGNATURE) 
{
	struct sockaddr_in *connaddr;
        struct sockaddr_in peer_address;
        int sock_type = -1;
        socklen_t sock_type_len = sizeof(sock_type);
	socklen_t namelen = sizeof(peer_address);

	if (realconnect == NULL) {
                perror("Unresolved symbol: connect\n");
                return(-1);
        }

        connaddr = (struct sockaddr_in *) __addr;

        /* Get the type of the socket */
        getsockopt(__fd, SOL_SOCKET, SO_TYPE,
                   (void *) &sock_type, &sock_type_len);

        /* If this isn't an INET socket for a TCP stream we can't  */
        /* handle it, just call the real connect now               */
	if ((connaddr->sin_family != AF_INET) || (sock_type != SOCK_STREAM))
                return(realconnect(__fd, __addr, __len));

	/* If the socket is already connected, just call connect  */
	/* and get its standard reply                             */
	if (!getpeername(__fd, (struct sockaddr *) &peer_address, &namelen))
		return(realconnect(__fd, __addr, __len));

	/* If the address is not local call realconnect */
	if (!is_local(&(connaddr->sin_addr)))
		return(realconnect(__fd, __addr, __len));

	/* CUPS */
	if ((getenv("NXCUPS_PORT") != NULL) && connaddr->sin_port==htons(631))
		connaddr->sin_port=htons(atoi(getenv("NXCUPS_PORT")));

	/* SAMBA */
	if ((getenv("NXSAMBA_PORT") != NULL) && (connaddr->sin_port==htons(139) || connaddr->sin_port==htons(445)))
		connaddr->sin_port=htons(atoi(getenv("NXSAMBA_PORT")));

	return realconnect(__fd, __addr, __len);
}
