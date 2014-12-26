#include<stdio.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include"shell_fct.h"
#define MAX_BUFF 8192
#define MAX_SECOND 30

void exec_cd(char* filename) {
    struct stat sb;
    stat(filename, &sb);
    if ( (sb.st_mode & S_IFMT) == S_IFDIR ) {
        chdir(filename);
    } else {
        printf("%s: No such file or directory\n", filename);
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

int exec_commande(cmd* ma_cmd) {
	pid_t* child_procs;
	int** fd;
	int nb_cmd_membre = ma_cmd->nb_cmd_membres;

    signal(SIGALRM, time_over);
    //set garde de chien
    alarm(MAX_SECOND);
    
	//initial fd et child_procs
	fd = (int**)malloc(nb_cmd_membre * sizeof(int*) );
	int i;
	for ( i = 0; i < nb_cmd_membre; i ++ ) {
		fd[i] = (int*)malloc(2 * sizeof(int));
		pipe(fd[i]);
	}
	child_procs = (pid_t*)malloc(nb_cmd_membre * sizeof(pid_t));

	//creer child_procs processus et executer les commandes
	int cmd_i;
	for ( cmd_i = 0; cmd_i < nb_cmd_membre; cmd_i ++ ) {
		child_procs[cmd_i]=fork();
		if(child_procs[cmd_i]==0) {
			if ( cmd_i != 0 ) {
				close(fd[cmd_i - 1][1]);
				dup2(fd[cmd_i - 1][0],0);
				close(fd[cmd_i - 1][0]);
			}

            //s'il y a de 'in redirection' 
			int have_in_redirect = ma_cmd->redirection[cmd_i][STDIN] == NULL ? 0 : 1;
			if ( have_in_redirect ) {
				printf("child %d have in_redirect: %s\n", i, ma_cmd->redirection[cmd_i][STDIN]);
				int in_fd = open(ma_cmd->redirection[cmd_i][STDIN], O_RDONLY, 0666);
				dup2(in_fd,0);
				close(in_fd);
			}

            int have_out_redirect = ma_cmd->redirection[cmd_i][STDOUT] == NULL ? 0 : 1;
            int have_err_redirect = ma_cmd->redirection[cmd_i][STDERR] == NULL ? 0 : 1;
            if ( have_out_redirect || have_err_redirect ) {
                if ( have_out_redirect ) {
                    //s'il y a de 'out redirection' 
                    printf("child %d have out_redirect: %s\n", cmd_i, ma_cmd->redirection[cmd_i][STDOUT]);
                    int out_fd = get_redirect_fd(ma_cmd, cmd_i, STDOUT);
                    dup2(out_fd, 1);
                    close(out_fd);
                } 
                if ( have_err_redirect ) {
                    //s'il y a de 'err redirection'
                    printf("child %d have out_redirect: %s\n", cmd_i, ma_cmd->redirection[cmd_i][STDOUT]);
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
		if ( cmd_i != 0 && cmd_i != nb_cmd_membre ) {
			close(fd[cmd_i - 1][1]);
			close(fd[cmd_i - 1][0]);
		}
	}

	//redirect stdin du dernier child_proc
    int stdin_fd= dup(0);
	int last_child_proc_i = nb_cmd_membre-1;
	int changed_fd;

	close(fd[last_child_proc_i][1]);
	changed_fd = dup2(fd[last_child_proc_i][0],0);
	close(fd[last_child_proc_i][0]);

    //read the result of the command
	int nbytes;
	char result[MAX_BUFF];
	nbytes = read(0,result,sizeof(result));
	result[nbytes] = '\0';

	//out redirection
	int have_out_redirect = ma_cmd->redirection[last_child_proc_i][STDOUT] == NULL ? 0 : 1;
	int stdout_fd = dup(1);
	int changed_out_fd;
	if ( have_out_redirect ) {
		printf("have out redirect: %s\n", ma_cmd->redirection[last_child_proc_i][STDOUT]);
		int out_fd = get_redirect_fd(ma_cmd, last_child_proc_i, STDOUT);
		changed_out_fd = dup2(out_fd, 1);
		close(out_fd);
	}

    //err redirection
    int stderr_fd = dup(2);
    int changed_err_fd; 
	int have_err_redirect = ma_cmd->redirection[last_child_proc_i][STDERR] == NULL ? 0 : 1;
    if ( have_err_redirect ) {
        printf("child %d have out_redirect: %s\n", cmd_i, ma_cmd->redirection[cmd_i][STDOUT]);
        int out_fd = get_redirect_fd(ma_cmd, cmd_i, STDERR);
        changed_err_fd = dup2(out_fd, 2);
        close(out_fd);
    }

	printf("%s",result);

	//reback redirection
    dup2(stderr_fd, changed_err_fd);
	dup2(stdout_fd, changed_out_fd);
	dup2(stdin_fd, changed_fd);
	return 0;
}
