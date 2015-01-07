#include<stdio.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<fcntl.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"shell_fct.h"
#define MAX_BUFF_SIZE 1024
#define MAX_SECOND 10

void exec_cd(char* filename) {
    struct stat sb;

	int last_char_index = strlen(filename) - 1;
	if ( filename[last_char_index] == '/' ) {
		filename[last_char_index] = '\0';
	}
    stat(filename, &sb);
    if ( (sb.st_mode & S_IFMT) == S_IFDIR ) {
        chdir(filename);
    } else {
        fprintf(stderr, "%s: No such file or directory\n", filename);
    }
}

int exec_inside_command(char* cmd_str) { 
	trim(cmd_str);
    if( ! strncmp(cmd_str, "cd ", 3) ) { // "cd " not "cd"
        exec_cd(cmd_str+3); 
        return 1;
    }
    return 0;
}

int get_redirect_fd( cmd* ma_cmd, int cmd_i, int type_redirec ) {
    int out_fd;
    if ( ma_cmd->type_redirection[cmd_i][type_redirec] == APPEND ) {
        out_fd = open(ma_cmd->redirection[cmd_i][type_redirec], O_APPEND | O_CREAT | O_RDWR, 0666);
    } else {
        out_fd = open(ma_cmd->redirection[cmd_i][type_redirec], O_CREAT | O_RDWR, 0666);
    }
    return out_fd;
}


int get_server_fd(cmd* ma_cmd, int cmd_i) {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	//server port
	servaddr.sin_port = htons(atoi(ma_cmd->server_port[cmd_i]));
	//server ip
	servaddr.sin_addr.s_addr = inet_addr(ma_cmd->server_ip[cmd_i]);

	if (connect(server_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect");
		exit(1);
	}

	return server_fd;
}

void time_over(int signo) {
    printf("Ce processus %d depasse plein de temps, on s'arrete\n", getpid());
	char pidStr[25];
	sprintf(pidStr, "%d", getpid());
	execlp("pkill", "pkill", "-TERM", "-P", pidStr, NULL);
}

void freeFds( int** fd, int n ) {
	int i;
	for ( i = 0; i < n; i ++ ) {
		free(fd[i]);
	}
	free( fd );
}
int exec_commande(cmd* ma_cmd) {
	pid_t pid = fork();
	if ( pid == 0 ) {
		int** fd;
		int nb_cmd_membres = ma_cmd->nb_cmd_membres;

		//set garde de chien
		signal(SIGALRM, time_over);
		alarm(MAX_SECOND);
		
		fd = (int**)malloc(nb_cmd_membres * sizeof(int*) );
		int i;
		for ( i = 0; i < nb_cmd_membres; i ++ ) {
			fd[i] = (int*)malloc(2 * sizeof(int));
			pipe(fd[i]);
		}

		//creer child_procs processus et executer les commandes
		int cmd_i;
		for ( cmd_i = 0; cmd_i < nb_cmd_membres; cmd_i ++ ) {
			if(fork()==0) {
				int k;
				for ( k = 0; k < nb_cmd_membres && k != cmd_i && k != cmd_i - 1; k ++ ) {
					close(fd[k][0] );
					close(fd[k][1] );
				}
				int should_send_to_server = ma_cmd->server_ip[cmd_i] == NULL ? 0 : 1;
				if ( should_send_to_server ) {
					int server_fd = get_server_fd(ma_cmd, cmd_i);	
					write(server_fd, ma_cmd->cmd_membres[cmd_i], 256);
					if ( cmd_i > 0 ) {
						char buff[MAX_BUFF_SIZE];
						int nbytes2;
						close(fd[cmd_i-1][1]);
						do {
							nbytes2 = read(fd[cmd_i-1][0], buff, sizeof(buff) );
							if ( nbytes2 < 0 ) {
								perror("recv error\n");	
								exit(1);
							} else {
								buff[nbytes2] = '\0';
								if ( write(server_fd, buff, nbytes2) < 0 ) {
									perror("write to server error\n");
									exit(1);
								}
							}
						} while ( nbytes2 > 0 );
						close(fd[cmd_i-1][0]);

						
						//pour dire qu'il n'y a plus de msg a envoyer au serveur, le serveur va commencer a traiter les messages
						//mais en meme temps, le client peut recevoir les messages viennent du serveur
						shutdown(server_fd, SHUT_WR);
					}

					close(fd[cmd_i][0]);
					char recvbuff[MAX_BUFF_SIZE];
					int nbytes;
					do {
						nbytes = read(server_fd, recvbuff, sizeof(recvbuff));
						if (nbytes < 0) {
							perror("recv error\n");	
							exit(1);
						} else {
							recvbuff[nbytes] = '\0';
							if ( write(fd[cmd_i][1], recvbuff, nbytes) < 0 ) {
								perror("write pipe error");
								exit(1);
							}
						}
					} while ( nbytes > 0 );
					close(fd[cmd_i][1]);
					close(server_fd);
					exit(0);
				} else {
					if ( cmd_i > 0 ) {
						close(fd[cmd_i - 1][1]);
						dup2(fd[cmd_i - 1][0],0);
						close(fd[cmd_i - 1][0]);
					}

					int have_in_redirect = ma_cmd->redirection[cmd_i][STDIN] == NULL ? 0 : 1;
					if ( have_in_redirect ) {
						int in_fd = open(ma_cmd->redirection[cmd_i][STDIN], O_RDONLY, 0666);
						dup2(in_fd,0);
						close(in_fd);
					}

					int have_out_redirect = ma_cmd->redirection[cmd_i][STDOUT] == NULL ? 0 : 1;
					int have_err_redirect = ma_cmd->redirection[cmd_i][STDERR] == NULL ? 0 : 1;
					if ( have_out_redirect || have_err_redirect ) {
						if ( have_out_redirect ) {
							int out_fd = get_redirect_fd(ma_cmd, cmd_i, STDOUT);
							dup2(out_fd, 1);
							close(out_fd);
						} 
						if ( have_err_redirect ) {
							int out_fd = get_redirect_fd(ma_cmd, cmd_i, STDERR);
							dup2(out_fd, 2);
							close(out_fd);
						}
					} else {
						close(fd[cmd_i][0]);
						dup2(fd[cmd_i][1],1);
						close(fd[cmd_i][1]);
					}

					//freeFds(fd, nb_cmd_membres);
					if ( execvp(ma_cmd->cmd_args[cmd_i][0], ma_cmd->cmd_args[cmd_i]) == -1 ) {
						perror("Commande inconnue\n");
						exit(1);
					}
				}
			}
			if ( cmd_i > 0 ) {
				wait(NULL);
				close(fd[cmd_i - 1][1]);
				close(fd[cmd_i - 1][0]);
			}
		}

		int last_child_proc_i = nb_cmd_membres - 1;
		//read the result of the command
		int nbytes;
		char result[MAX_BUFF_SIZE];

		close(fd[last_child_proc_i][1]);
		do {
			nbytes = read(fd[last_child_proc_i][0],result,sizeof(result));
			if ( nbytes < 0 ) {
				perror("read error\n");
				exit(1);
			} else {
				result[nbytes] = '\0';
				printf("%s",result);
			}
		} while (nbytes > 0);
		close(fd[last_child_proc_i][0]);

		for ( i = 0; i < nb_cmd_membres; i ++ ) {
			free(fd[i]);
		}

		free( fd );
		exit(0);
	} else {
		wait(NULL);
	}
	return 0;
}
