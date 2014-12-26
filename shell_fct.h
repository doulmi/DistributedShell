#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "cmd.h"

/**
 * @brief Executer les commandes normales
 */
int exec_commande(cmd* ma_cmd);

/**
 * @brief Executer les commandes internes comme 'exit', 'cd' ... qui ne contiennent pas aux commandes normales 
 * @param cmd_str command line
 * @return s'il execute une commande interne, retourne 1, sinon retourne 0
 */
int exec_inside_command(char* cmd_str);

int file_exist(char* filename);
void exec_cd(char* filename);
void time_over(int signo);
int get_redirect_fd(cmd* ma_cmd, int cmd_i, int type_redirec);
