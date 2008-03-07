/*
 * libnxredir - Redirect certain ports to other forwarded ports.
 *
 * Copyright (c) 2005 by Fabian Franz.
 *
 * License: GPL
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

int connect(CONNECT_SIGNATURE) 
{
	if (realconnect == NULL) {
                perror("Unresolved symbol: connect\n");
                return(-1);
        }

	/* CUPS */
	if ((getenv("NXCUPS_PORT") != NULL) && ((struct sockaddr_in*)__addr)->sin_port==htons(631))
		((struct sockaddr_in*)__addr)->sin_port=htons(atoi(getenv("NXCUPS_PORT")));

	/* SAMBA */
	if ((getenv("NXSAMBA_PORT") != NULL) && (((struct sockaddr_in*)__addr)->sin_port==htons(139) || ((struct sockaddr_in*)__addr)->sin_port==htons(445)))
		((struct sockaddr_in*)__addr)->sin_port=htons(atoi(getenv("NXSAMBA_PORT")));

	return realconnect(__fd, __addr, __len);
}
