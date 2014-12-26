#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>

#include "shell_fct.h"
 
#define MYSHELL_CMD_OK 1
#define MYSHELL_FCT_EXIT 0

int main(int argc, char** argv)
{
	int ret = MYSHELL_CMD_OK;
	cmd mycmd;
	char* readlineptr;
	struct passwd* infos;
	char str[1024];
	char hostname[256];
	char workingdirectory[256];

	int id_socket = socket(AF_INET, SOCK_STREAM, 0);
	int fd_socket;
	struct sockaddr_in servaddr;
	socklen_t taille;

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1234);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind, succes return 0, error return -1
	if (bind(id_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		perror("bind error\n");
		exit(1);
	}

	if (listen(id_socket, 10) == -1) {
		perror("listen error\n");
		exit(1);
	}
    using_history();
	while(ret != MYSHELL_FCT_EXIT)
	{
		taille = sizeof(struct sockaddr_in);
		fd_socket = accept(id_socket, (struct sockaddr*)&servaddr, &taille);
		
		pid_t pid;
		char recvbuf[1024];
		int nbytes = recv(fd_socket, recvbuf, sizeof(recvbuf), 0);
		printf("%d bytes read: %s", nbytes, recvbuf);
		infos=getpwuid(getuid());
		gethostname(hostname, 256);
		getcwd(workingdirectory, 256);

		sprintf(str, "{myshell}%s@%s:%s$ ", infos->pw_name, hostname, workingdirectory);
		readlineptr = readline(str);
        int line_len = strlen(readlineptr);
        if ( readlineptr[line_len -1] == '\n') {
            printf("is n");
        }

		if (!is_empty_str(readlineptr)) {
			//quit command
			trim(readlineptr);
			if ( strcmp(readlineptr, "exit") == 0 ) {
				ret = MYSHELL_FCT_EXIT;
				continue;
			}
            //add cmd to history
            char* expansion;
            int result;
            result = history_expand(readlineptr, &expansion);
            if (result) {
                fprintf(stderr, "%s\n", expansion);
            }

            if (result < 0 || result == 2) {
                free(expansion);
                continue;
            }

            add_history(expansion);
            free(readlineptr);
            readlineptr = strdup(expansion);
            free(expansion);

            if (exec_inside_command(readlineptr)) {
                free(readlineptr);
                continue;
            }

			create_cmd(readlineptr, &mycmd);	
			exec_commande(&mycmd);
			destroy_cmd(&mycmd);
		}

		free(readlineptr);
	}
	return 0;
}
