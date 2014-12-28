#include<stdio.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"shell_fct.h"
#define MAX_BUFF_SIZE 32768
#define MAX_SECOND 30

void exec_cd(char* filename) {
    struct stat sb;
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

void time_over(int signo) {
    //perror("Ce processus depasse plein de temps, on s'arrete\n");
    //exit(1);
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

int exec_commande(cmd* ma_cmd) {
	pid_t* child_procs;
	int** fd;
	int* server_fd;
	int nb_cmd_membres = ma_cmd->nb_cmd_membres;

    signal(SIGALRM, time_over);
    //set garde de chien
    alarm(MAX_SECOND);
    
	//initial fd et child_procs et server_fd
	fd = (int**)malloc(nb_cmd_membres * sizeof(int*) );
	int i;
	for ( i = 0; i < nb_cmd_membres; i ++ ) {
		fd[i] = (int*)malloc(2 * sizeof(int));
		pipe(fd[i]);
	}
	child_procs = (pid_t*)malloc(nb_cmd_membres * sizeof(pid_t));

	server_fd = (int*)malloc(nb_cmd_membres * sizeof(int));
	for ( i = 0; i < nb_cmd_membres; i ++ ) {
		server_fd[i] = -1;
	}

	//creer child_procs processus et executer les commandes
	int cmd_i;
	for ( cmd_i = 0; cmd_i < nb_cmd_membres; cmd_i ++ ) {
		child_procs[cmd_i]=fork();
		if(child_procs[cmd_i]==0) {
			int should_send_to_server = ma_cmd->server_ip[cmd_i] == NULL ? 0 : 1;
			if ( should_send_to_server ) {
				server_fd[cmd_i] = get_server_fd(ma_cmd, cmd_i);	

				send(server_fd[cmd_i], ma_cmd->cmd_membres[cmd_i], strlen(ma_cmd->cmd_membres[cmd_i]), 0);
				send(server_fd[cmd_i], "\n", 1, 0);
				char recvbuff[MAX_BUFF_SIZE];
				int stdin_fd = dup(0);
				int changed_in_fd = dup2(server_fd[cmd_i], 0);
				char recvbuff[MAX_BUFF_SIZE];
				int nbytes = read(0, recvbuff, sizeof(recvbuff)); 
				dup2(stdin_fd, changed_in_fd);
				if (nbytes < 0) {
					perror("recv error\n");	
				} else {
					recvbuff[nbytes] = '\0';
					close(fd[cmd_i][0]);
					dup2(fd[cmd_i][1],1);
					close(fd[cmd_i][1]);
					printf("%s", recvbuff);
				}
			} else {
				if ( cmd_i != 0 ) {
					close(fd[cmd_i - 1][1]);
					dup2(fd[cmd_i - 1][0],0);
					close(fd[cmd_i - 1][0]);
				}

				//s'il y a de 'in redirection' 
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
						//s'il y a de 'out redirection' 
						int out_fd = get_redirect_fd(ma_cmd, cmd_i, STDOUT);
						dup2(out_fd, 1);
						close(out_fd);
					} 
					if ( have_err_redirect ) {
						//s'il y a de 'err redirection'
						int out_fd = get_redirect_fd(ma_cmd, cmd_i, STDERR);
						dup2(out_fd, 2);
						close(out_fd);
					}
				} else {
					close(fd[cmd_i][0]);
					dup2(fd[cmd_i][1],1);
					close(fd[cmd_i][1]);
				}
				execvp(ma_cmd->cmd_args[cmd_i][0], ma_cmd->cmd_args[cmd_i]);
			}
		}
		if ( cmd_i != 0 && cmd_i != nb_cmd_membres ) {
			close(fd[cmd_i - 1][1]);
			close(fd[cmd_i - 1][0]);
		}
	}

	//redirect stdin du dernier child_proc
    int stdin_fd= dup(0);
	int last_child_proc_i = nb_cmd_membres - 1;
	int changed_fd;

	close(fd[last_child_proc_i][1]);
	changed_fd = dup2(fd[last_child_proc_i][0],0);
	close(fd[last_child_proc_i][0]);

    //read the result of the command
	int nbytes;
	char result[MAX_BUFF_SIZE];
	nbytes = read(0,result,sizeof(result));
	result[nbytes] = '\0';

	//out redirection
	int have_out_redirect = ma_cmd->redirection[last_child_proc_i][STDOUT] == NULL ? 0 : 1;
	int stdout_fd = dup(1);
	int changed_out_fd;
	if ( have_out_redirect ) {
		int out_fd = get_redirect_fd(ma_cmd, last_child_proc_i, STDOUT);
		changed_out_fd = dup2(out_fd, 1);
		close(out_fd);
	}

    //err redirection
    int stderr_fd = dup(2);
    int changed_err_fd; 
	int have_err_redirect = ma_cmd->redirection[last_child_proc_i][STDERR] == NULL ? 0 : 1;
    if ( have_err_redirect ) {
        int out_fd = get_redirect_fd(ma_cmd, cmd_i, STDERR);
        changed_err_fd = dup2(out_fd, 2);
        close(out_fd);
    }

	printf("%s",result);

	//reback redirection
    dup2(stderr_fd, changed_err_fd);
	dup2(stdout_fd, changed_out_fd);
	dup2(stdin_fd, changed_fd);

	//free
	free(child_procs);

	for ( i = 0; i < nb_cmd_membres; i ++ ) {
		free(fd[i]);
	}
	free(fd);

	for ( i = 0; i < nb_cmd_membres; i ++ ) {
		if ( server_fd[i] != -1 ) {
			close(server_fd[i]);
		}
	}
	free(server_fd);

	return 0;
}
