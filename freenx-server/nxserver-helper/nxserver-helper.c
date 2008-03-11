/*
 * Copyright (c) 2006 by Fabian Franz.
 *
 * License: GPL, v2
 *
 * SVN: $Id$
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	int fds[2];
	char buf[1024];

	socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
	
	// This fd can be used by the server for spawning the node
	snprintf(buf, 1022, "%d", fds[1]);
	setenv("NX_SERVERFD", buf, 1);
	
	// This fd can be used for communication with the node
	snprintf(buf, 1022, "%d", fds[0]);
	setenv("NX_COMMFD", buf, 1);

	// We do not trust this user, he still has to login
	unsetenv("NX_TRUSTED_USER");

	argv++;
	execv(argv[0], argv);
}
