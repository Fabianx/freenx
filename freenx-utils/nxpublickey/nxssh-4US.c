#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define NXSERVER_SUID "nxserver-suid"
#define NXSERVER_USERMODE "nxserver-usermode"
#define NXSERVER_PATH "~/bin:~/NX4U/:/usr/NX/bin:/opt/NX/bin:/opt/NX4U/bin/:/usr/NX4U/bin:/usr/local/NX4U/bin:/usr/lib/nx/bin"

#ifdef _WIN32
#define NXSSH_ORIG "mxssh.exe"
#define DIR_SEPERATOR '\\'
#define fork() -1
#define wait(x) ;
#define WEXITSTATUS(x) (x)
#else
#include <sys/types.h>
#include <sys/wait.h>

#define NXSSH_ORIG "mxssh"
#define DIR_SEPERATOR '/'
#endif

#define BUF_MAX 8192

char* dirname(char* str)
{
	char* search=strrchr(str, DIR_SEPERATOR);
	search[0]='\0';
	return str;
}

int main(int argc, char** argv)
{
	int wrapit=0;
	int i;
	int retcode, pid;
	char* hostname = NULL;
	char line[BUF_MAX+1];
	char user[BUF_MAX+1];
	char extra_command[BUF_MAX+1];
	char remote_command[BUF_MAX+1];
	char* username;

	char* mode;
	
	int pwmode=1; // Use password for authentication

	char* nxserver_suid=NXSERVER_SUID;
	char* nxserver_usermode=NXSERVER_USERMODE;
	
	// Find original nxssh
	char command[BUF_MAX+1];
	char* dname=dirname(strdup(argv[0]));
		
	snprintf(command, BUF_MAX, "%s%c%s", dname, DIR_SEPERATOR, NXSSH_ORIG);
	argv[0]=NXSSH_ORIG;
	
	if (getenv("NXWRAP") != NULL)
		wrapit=atoi(getenv("NXWRAP"));
	
	// Search for nx@ string in arguments
	for (i=0; i < argc; i++)
	{
		if (strncmp(argv[i], "nx@", 3) == 0)
		{
			// We need to shift the hostname and save it
			hostname=argv[i]+3;
			
			if (hostname[0] == '@')
			{
				hostname++;
				wrapit=1;
			}
			
			break;
		}
	}

	if (wrapit != 1)
	{
		execv(command, argv);
		perror("Error: Could not execute original renamed nxssh (default: mxssh)");
		printf("NX> 204 ERROR: wrong password or login.\n");
		printf("NX> 999 Bye\n");
		exit(1);
	}

	// Lets wrap it!
	//

	printf("NX> 203 NXSSH running with pid: %d\n", getpid());
	printf("NX> 200 Connected to address: 127.0.0.1 on port: 22\n");
	printf("NX> 202 Authenticating user: nx\n");
	printf("NX> 208 Using auth method: publickey\n");

	printf("HELLO NXSERVER - Version 3.2.0 OS (GPL Edition)\n");

	// Login stage
	while(1)
	{
		printf("NX> 105 ");
		fflush(stdout);

		if (fgets(line, BUF_MAX, stdin) == NULL)
		{
			printf("Quit\n");
			printf("NX> 999 Bye\n");
			fflush(stdout);
			exit(0);
		}  
		
		printf("%s", line);
 
		if (strncmp(line, "quit", 4) == 0 || strncmp(line, "QUIT", 4) == 0)
		{
			printf("Quit\n");
			printf("NX> 999 Bye\n");
			fflush(stdout);
			exit(0);
		}
		else if (strncmp(line, "exit", 4) == 0 || strncmp(line, "EXIT", 4) == 0)
		{
			printf("Exit\n");
			printf("NX> 999 Bye\n");
			fflush(stdout);
			exit(0);
		}
		else if (strncmp(line, "bye", 3) == 0 || strncmp(line, "BYE", 3) == 0)
		{
			printf("Bye\n");
			printf("NX> 999 Bye\n");
			fflush(stdout);
			exit(0);
		}
		else if (strncmp(line, "hello", 5) == 0 || strncmp(line, "HELLO", 5) == 0)
		{
			printf("NX> 134 Accepted protocol: 3.2.0\n");
			fflush(stdout);
		}
		else if (strncmp(line, "set auth_mode", 13) == 0 || strncmp(line, "SET AUTH_MODE", 13) == 0)
		{
			printf("%s", line);
			fflush(stdout);
		}
		else if (strncmp(line, "login", 5) == 0 || strncmp(line, "LOGIN", 5) == 0)
		{
			printf("NX> 101 User: ");
			fflush(stdout);
			fgets(user, BUF_MAX, stdin);

			// Remove newline from username
			user[strlen(user)-1]='\0';
			username=user;
			
			printf("%s\n", username);

			printf("NX> 102 Password: ");
			fflush(stdout);
			break;
		}
	}

	//
	// Check which mode to use: PW or normal ssh
	//
	// Syntax: [@]user
	
	if (username[0] == '@')
	{
		// do not use password for authentication
		username++;
		pwmode=0;

		// So read password from client and reset it again
		fgets(line, BUF_MAX, stdin);
		memset(line, 0, BUF_MAX);
	}

	//
	// Check which method (suid or usermode) to use on server
	//  
	// Syntax: user[@<U|S>:command]
	//
	
	if ((mode=strrchr(username, '@')) != NULL && strlen(mode) >= 2)
	{
		char* ucommand = NULL;

		// '@' + 'U|S' + ':' + 'Command'
		if (strlen(mode) > 3)
			ucommand=mode+3;

		if (mode[1] == 'U')
		{
			nxserver_suid="/bin/false";
			
			// Did we get a hint that another binary should be used?
			if (ucommand)
			{
				snprintf(extra_command, BUF_MAX, "%s %s",  ucommand, nxserver_usermode);
				nxserver_usermode=extra_command;
			}
		}
		else if (mode[1] == 'S')
		{
			nxserver_usermode="/bin/falsedoesnotexist";
			
			// Did we get a hint that another binary should be used?
			if (ucommand)
			{
				snprintf(extra_command, BUF_MAX, "%s %s",  ucommand, nxserver_suid);
				nxserver_suid=extra_command;
			}
		}
		mode[0]='\0';
	}

        snprintf(remote_command, BUF_MAX, "'export PATH=%s:\"$PATH\"; for i in $(which -a %s); do if [ -x \"$i\" -a -u \"$i\" ]; then unset SSH_CLIENT SSH_CLIENT2; export SSH_CLIENT=\"127.0.0.1 56404 127.0.0.1 22\"; exec \"$i\"; fi; done; for i in $(which -a %s); do if [ -x \"$i\" -a ! -u \"$i\" -a ! -L \"$i\" ]; then exec \"$i\"; fi; done; echo \"NX> 596 Service not available.\"; echo \"NX> 999 Bye\"'", NXSERVER_PATH, nxserver_suid, nxserver_usermode);

	retcode = 0;

	// Lets fork a child!
	pid = fork();
	
	// Child!
	if (pid <= 0)
	{
	
		if (pwmode == 1)
		{
			execl(command, argv[0], "-nxstdinpass", "-l", username, hostname, "-x", "-2", "-B", "-o", "NumberOfPasswordPrompts 1", "sh", "-c", remote_command, NULL);
			perror("Error: Could not execute original renamed nxssh (default: mxssh)");
			exit(1);
		}
		else
		{
			execl(command, argv[0], "-l", username, hostname, "-x", "-2", "-B", "-o", "sh", "-c", remote_command, NULL);
			perror("Error: Could not execute original renamed nxssh (default: mxssh)");
			exit(1);
		}
		
		exit(0);
	}
	// Child end

	// Lets wait for our child
	wait(&retcode);
	
	if (WEXITSTATUS(retcode) != 0)
	{
		printf("NX> 404 ERROR: wrong password or login.\n");
		printf("NX> 999 Bye\n");
		fflush(stdout);
		exit(1);
	}

	return 0;
}
