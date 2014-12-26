#include <stdio.h>
#include <readline/readline.h> 
#include <readline/history.h>
#include <stdlib.h>
#define WRITE 0
#define APPEND 1
#define STDIN 0
#define STDOUT 1
#define STDERR 2


//command1 [[param1_1] ...[param1_n]] [<</< file] [>>/> file] | command2 [[param2_1] ...[param2_n]]... [&]
typedef struct commande {
	char* cmd_initiale;				
	char **cmd_membres;			
	unsigned int nb_cmd_membres;
	char ***cmd_args;			
	unsigned int *nb_args;		
	char ***redirection;	
	int **type_redirection;
    char **server_ip;     
    char **server_port;
} cmd;

void trim(char* str);
int is_empty_str(char* str);
/**
  * @brief Initialise cmd par une chaine de caractere 
  * @param chaine donnee pour initilaliser la cmd
  * @param c cmd
  */
void create_cmd(char* chaine, cmd *c);

void parse_membres(char *chaine,cmd *ma_cmd); 
void aff_membres(cmd *ma_cmd);
void free_membres(cmd *ma_cmd);

void parse_ip_port(cmd *c);
void free_ip_port(cmd *c);
void parse_ip_port(cmd *c);

void aff_args(cmd *c);
void free_args(cmd *c);
void parse_args(cmd *c);  

void parse_all_redirection(cmd* c);
int parse_redirection(unsigned int i,cmd *c);  
void free_redirection(cmd *c);
void aff_all_redirection(cmd *c);
void aff_redirection(cmd *c, int i);

void destroy_cmd(cmd *c);
