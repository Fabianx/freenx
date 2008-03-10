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
 * Author: alriddoch@google.com (Alistair Riddoch)
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <assert.h>

char * prgname = NULL;

#define STRING_BUFLEN 512
#define NXNODE_COMMAND "nxnode"
#define NXSERVER_COMMAND "nxserver"

int launch_nxnode(uid_t user, int comm_fd)
{
    // This program is being run setuid, so we can now drop back to
    // the ruid which should be the original user.
    if (seteuid(user) != 0) {
        syslog(LOG_ERR, "ERROR: Unable to drop back to calling userid: %m\n");
        return 1;
    }

    // Disassociate from the terminal connected to the client that we were
    // invoked from.
    setsid();

    // Close stdio, and reconnected it to a socket via which we can
    // communicate with nxserver.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    dup2(comm_fd, STDIN_FILENO);
    dup2(comm_fd, STDOUT_FILENO);
    dup2(comm_fd, STDERR_FILENO);

    return execlp(NXNODE_COMMAND, NXNODE_COMMAND, NULL);
}

int launch_nxserver(const char * username, int comm_fd, int argc, char ** argv)
{
    size_t env_string_length;
    char * env_string;
    char ** new_argv;
    int i;

    env_string_length = snprintf(0, 0, "%s=%s",
                                 "NX_TRUSTED_USER", username);
    env_string = malloc(env_string_length + 1);
    assert(env_string != NULL);
    sprintf(env_string, "%s=%s", "NX_TRUSTED_USER", username);
    putenv(env_string);

    env_string_length = snprintf(0, 0, "%s=%d",
                                 "NX_COMMFD", comm_fd);
    env_string = malloc(env_string_length + 1);
    assert(env_string != NULL);
    sprintf(env_string, "%s=%d", "NX_COMMFD", comm_fd);
    putenv(env_string);

    new_argv = calloc(argc + 1, sizeof(char *));
    new_argv[0] = NXSERVER_COMMAND;

    for (i = 1; i < argc; ++i) {
        new_argv[i] = argv[i];
    }

    return execvp(NXSERVER_COMMAND, new_argv);
}

int main(int argc, char ** argv)
{
    prgname = basename(strdup(argv[0]));

    openlog(prgname, LOG_PID, LOG_USER);

    uid_t calling_uid = getuid();

    if (geteuid() == calling_uid) {
        syslog(LOG_WARNING, "WARNING: Not running suid.\n");
    }

    struct passwd calling_user;
    struct passwd * ret;
    char user_string_buffer[STRING_BUFLEN];
    errno = 0;

    if (getpwuid_r(calling_uid, &calling_user, &user_string_buffer[0],
                   STRING_BUFLEN, &ret) != 0) {
        syslog(LOG_ERR, "ERROR: Unable to get passwd entry for calling user %d: %m\n", calling_uid);
        return 1;
    }

    int sockets[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        syslog(LOG_ERR, "FATAL: Socket error: %m");
        return 1;
    }

    pid_t child = fork();

    if (child < 0) {
        syslog(LOG_ERR, "FATAL: Fork error: %m");
        return 1;
    }

    if (child == 0) {
        close(sockets[1]);
        return launch_nxnode(calling_uid, sockets[0]);
    }

    close(sockets[0]);
    return launch_nxserver(calling_user.pw_name, sockets[1], argc, argv);
}
