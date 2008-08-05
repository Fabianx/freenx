/*
 * Copyright 2007 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Authors: alriddoch@google.com (Alistair Riddoch)
 * 	    freenx@fabian-franz.de (Fabian Franz)
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef NXSERVER_COMMAND
#define NXSERVER_COMMAND "/usr/bin/nx-session-launcher"
#endif

#define CK_LAUNCH_SESSION_COMMAND "/usr/bin/ck-launch-session"

int main(int argc, char ** argv)
{
    char ** new_argv;
    new_argv = calloc(argc + 1, sizeof(char *));
    int i;

    for (i = 1; i < argc; ++i) {
        new_argv[i] = argv[i];
    }

    uid_t calling_uid = getuid();

    if (geteuid() == calling_uid) {
        printf("Not running suid. Executing ck-launch-session.\n");

        new_argv[0] = CK_LAUNCH_SESSION_COMMAND;

    }else{
        new_argv[0] = NXSERVER_COMMAND;
    }

    return execv(new_argv[0], new_argv);
}
