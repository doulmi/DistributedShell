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
void* handle_connect(void* client_fd) {
	cmd mycmd;
	char readlineptr[2048];
	int nbytes = recv((int)client_fd, readlineptr, sizeof(readlineptr), 0);
	if ( nbytes < 0 ) {
		printf("recv error\n");
		pthread_exit(NULL);
	}
	printf("%d bytes read: %s\n", nbytes, readlineptr);
	trim(readlineptr);
	if (!is_empty_str(readlineptr)) {
		create_cmd(readlineptr, &mycmd);	

		int stdout_fd = dup(1);
		dup2((int)client_fd, 1);
		int stderr_fd = dup(2);
		dup2((int)client_fd, 2);

		exec_commande(&mycmd);
		destroy_cmd(&mycmd);


		dup2(stdout_fd, 1);
		dup2(stderr_fd, 2);
	}
	printf("client %d quit\n", (int)client_fd);
	pthread_exit(NULL);
}

int main(int argc, char** argv)
{
	int id_socket = socket(AF_INET, SOCK_STREAM, 0);
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

	taille = sizeof(struct sockaddr_in);
	while(1) {
		int fd_socket = accept(id_socket, (struct sockaddr*)&servaddr, &taille);
		pthread_t thread;
		printf("a client connect :%d\n", fd_socket);
		int rc = pthread_create(&thread, NULL, handle_connect, (void*)fd_socket);
		if (rc) {
			perror("create thread error\n");
			continue;
		}
	}

	pthread_exit(NULL);
	return 0;
}
