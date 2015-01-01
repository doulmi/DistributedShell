#include <stdio.h>
#include <string.h>
 #include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include "shell_fct.h"
  
#define MYSHELL_CMD_OK 1
#define MYSHELL_FCT_EXIT 0
void handle_connect(int client_fd) {
	cmd mycmd;
	char readlineptr[256];
	int nbytes = read(client_fd, readlineptr, sizeof(readlineptr));
	if ( nbytes < 0 ) {
		printf("recv error\n");
	}
	printf("%d bytes read: %s\n", nbytes, readlineptr);
	trim(readlineptr);
	if (!is_empty_str(readlineptr)) {
		create_cmd(readlineptr, &mycmd);	

		int stdin_fd = dup(0);
		dup2(client_fd, 0);
//		int stdout_fd = dup(1);
//		dup2(client_fd, 1);
//		int stderr_fd = dup(2);
//		dup2(client_fd, 2);

		exec_commande(&mycmd);

		dup2(stdin_fd, 0);
//		dup2(stdout_fd, 1);
//		dup2(stderr_fd, 2);

		destroy_cmd(&mycmd);
	}

	printf("client %d quit\n", (int)client_fd);
}

int main(int argc, char** argv)
{
	int id_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servaddr;
	socklen_t taille;

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1234);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	printf("%d\n", getpid());

	//bind, succes return 0, error return -1
	if (bind(id_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		perror("bind error\n");
		exit(1);
	}

	if (listen(id_socket, 10) == -1) {
		perror("listen error\n");
		exit(1);
	}

	taille = sizeof(struct sockaddr_in);
	while(1) {
		int fd_socket = accept(id_socket, (struct sockaddr*)&servaddr, &taille);

		printf("a client connect :%d\n", fd_socket);
		pid_t pid = fork();
		if ( pid == 0 ) {
			handle_connect(fd_socket);
			break;
		}
		close(fd_socket);
	}

	return 0;
}
