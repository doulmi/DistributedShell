#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#define PORT 1234 
#define BUFFER_SIZE 32768

int main() {
	//define sockfd
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	const char* const serverIP = "127.0.0.1";

	//define sockaddr_in
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	//server port
	servaddr.sin_port = htons(PORT);
	//server ip
	servaddr.sin_addr.s_addr = inet_addr(serverIP);

	if (connect(server_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect");
		exit(1);
	}
	char sendbuf[BUFFER_SIZE];
	while ( fgets(sendbuf, sizeof(sendbuf), stdin) != NULL ) {
		send(server_fd, sendbuf, strlen(sendbuf), 0);
		if (strcmp(sendbuf, "exit\n") == 0) break;
		memset(sendbuf, 0, sizeof(sendbuf));

		int stdin_fd = dup(0);
		int changed_out_fd = dup2(server_fd, 0);
		char recvbuff[BUFFER_SIZE];
		int nbytes;
		if ((nbytes = read(0, recvbuff, sizeof(recvbuff))) < 0 ) {
			puts("recv failed\n");
			break;
		}
		recvbuff[nbytes] = '\0';
		puts(recvbuff);

		dup2(stdin_fd, changed_out_fd);
	}
	close(server_fd);
	return 0;
}
