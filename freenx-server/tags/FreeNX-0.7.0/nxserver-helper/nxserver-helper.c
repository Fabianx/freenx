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

int main(int argc, char* argv[])
{
	int fds[2];

	socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
	argv++;
	execv(argv[0], argv);
}
