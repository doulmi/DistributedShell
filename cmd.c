#include "cmd.h"
#include <memory.h>
#include <string.h>
#include <ctype.h>
#define MAX_MENBRES_NB 20
#define MAX_ARGS_NB 20

void create_cmd(char* chaine, cmd *c) {
	c->cmd_initiale = strdup(chaine);
	parse_membres(chaine, c); 
	aff_membres(c);
	parse_ip_port(c);
	aff_ip_port(c);
	parse_args(c);
	aff_args(c);
	parse_all_redirection(c);
	aff_all_redirection(c);
}

void destroy_cmd(cmd *c) {
	free_redirection(c);
	free_args(c);
	free_membres(c);
}

void trim(char* str) {
	if (str== NULL) {
		return;
	}
	int pre_blanc_num = 0;
	int post_blanc_num = 0;
	int str_len = strlen(str);
	while ( *(str+ pre_blanc_num) != '\0' && *(str + pre_blanc_num) == ' ' ) {
		pre_blanc_num ++;
	}

	int last_not_blanc = str_len - 1;
	if ( pre_blanc_num != str_len ) {
		while ( last_not_blanc >= 0 && 
				*(str + last_not_blanc) == ' ' ) {
			post_blanc_num ++;
			last_not_blanc --;
		}
	}

	int i;
	int real_len = str_len - pre_blanc_num - post_blanc_num;
	for (i = 0; i < real_len; i++ ) {
		str[i] = str[pre_blanc_num + i];
	}
	str[i] = '\0';
}

int is_empty_str(char* str) {
	while( *str != '\0') {
		if (!isblank(*str)) {
			return 0;
		}
		str ++;
	}
	return 1;
}

void parse_membres(char* chaine, cmd *ma_cmd ) {
	const char delim[] = "|";
	char* token;
	int i;
	char* tmp = strdup(chaine);
	ma_cmd->nb_cmd_membres = 0;
	ma_cmd->cmd_membres = (char**) malloc( MAX_MENBRES_NB * sizeof(char*) );
	for (i = 0, token = strtok(tmp, delim); token != NULL; token = strtok(NULL, delim), i ++) {
		ma_cmd->nb_cmd_membres ++;
		ma_cmd->cmd_membres[i] = strdup(token);
	}
	ma_cmd->cmd_membres = (char**)realloc( ma_cmd->cmd_membres, 
			ma_cmd->nb_cmd_membres * sizeof( char* ) );
	free(tmp);
}

void aff_membres(cmd *ma_cmd) {
	int i;
	for ( i = 0; i < ma_cmd->nb_cmd_membres; i ++ ) {
		printf("cmd_membres[%d] = \"%s\"\n", i, ma_cmd->cmd_membres[i] );
	}
}

void free_membres(cmd *ma_cmd) {
	int i;
	for ( i = 0; i < ma_cmd->nb_cmd_membres; i ++ ) {
		free( ma_cmd->cmd_membres[i] );
	}
	free( ma_cmd->cmd_membres );
	ma_cmd->nb_cmd_membres = 0;
}

void parse_ip_port(cmd* c) {
	int i;
	c->server_ip = (char**)malloc(c->nb_cmd_membres * sizeof(char*));
	c->server_port = (char**)malloc(c->nb_cmd_membres * sizeof(char*));
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		c->server_ip[i] = NULL;
		c->server_port[i] = NULL;

		trim(c->cmd_membres[i]);
		//s:iii.iii.iii.iii:pppp
		if ( strncmp(c->cmd_membres[i], "s:", 2 ) == 0 ) {
			int start_i= 2;
			int end_i = 2;
			int point_count = 0;
			for ( end_i  = start_i; c->cmd_membres[i][end_i] != '\0'; 
					end_i ++ ) {
				if (c->cmd_membres[i][end_i] != '.' && c->cmd_membres[i][end_i] != ':' &&
						c->cmd_membres[i][end_i] != ' ' && !isdigit(c->cmd_membres[i][end_i]) ) {
					perror("Faux format de IP ou Port\n");
					break;
				}
				if (c->cmd_membres[i][end_i] == '.') {
					point_count ++;
				} 
				//get iii.iii.iii.iii
				if (c->cmd_membres[i][end_i] == ':' && point_count == 3) {
					c->server_ip[i]= strndup(c->cmd_membres[i] + start_i, end_i - start_i);
					start_i = end_i;
				}
				//get :pppp
				if (c->cmd_membres[i][end_i] == ' ' && point_count == 3) {
					c->server_port[i] = strndup( c->cmd_membres[i] + start_i + 1, end_i - start_i - 1);

					//change c->cmd_membres[i]
					int new_membre_len = strlen(c->cmd_membres[i]) - end_i - 1; 
					int j;
					for ( j = 0; j < new_membre_len; j ++ ) {
						c->cmd_membres[i][j] = c->cmd_membres[i][end_i+j+1];
					}
					c->cmd_membres[i][j] = '\0';
					break;
				}
			}
			if ( c->server_ip[i] == NULL || c->server_port[i] == NULL ) {
				perror("Faux format de IP ou Port\n");
				break;
			}
		}
	}
}

void free_ip_port(cmd* c) {
	int i;
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		free(c->server_ip[i]);
		free(c->server_port[i]);
	}
	free(c->server_ip);
	free(c->server_port);
}

void aff_ip_port(cmd* c) {
	int i;
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		if ( c->server_ip[i] == NULL ) {
			printf("SERVER IP: NULL\n");
		} else {
			printf("SERVER IP: \"%s\"\n", c->server_ip[i]);
		}
		if ( c->server_port[i] == NULL ) {
			printf("SERVER PORT: NULL\n");
		} else {
			printf("SERVER PORT: \"%s\"\n", c->server_port[i]);
		}
	}
}
void parse_args(cmd* c) {
	char* str_no_redir;
	char* token;
	char* tmp;
	int i, j;
	const char delim[] = " ";
	const char delim_redir[] = "<>";
	c->nb_args = (unsigned int*)malloc(c->nb_cmd_membres * sizeof(unsigned int));
	c->cmd_args = (char***)malloc(c->nb_cmd_membres * sizeof(char**));
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		c->nb_args[i] = 0;
		c->cmd_args[i] = (char**)malloc(MAX_ARGS_NB* sizeof(char*));

		tmp = strdup( c->cmd_membres[i] );
		str_no_redir = strtok(tmp, delim_redir);
		for ( j = 0, token = strtok(str_no_redir, delim); token != NULL; j++, token = strtok(NULL, delim)) {
			c->nb_args[i] ++;
			c->cmd_args[i][j] = strdup(token);
		}
		c->cmd_args[i][j] = NULL;
		//nb_args[i]+1 pour garder un NULL
		c->cmd_args[i] = (char**)realloc(c->cmd_args[i], (c->nb_args[i]+1) * sizeof(char*));

		free(tmp);
	}
}

void aff_args(cmd* c) {
	int i, j;
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		for ( j = 0; j < c->nb_args[i]; j ++ ) {
			printf("cmd_args[%d][%d] = \"%s\"\n", i, j, c->cmd_args[i][j]);
		}
		printf("cmd_args[%d][%d] = NULL\n", i, j);
	}
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		printf("nb_args[%d] = %d\n", i, c->nb_args[i] );
	}
}

void free_args(cmd *c) {
	int i, j;
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		for ( j = 0; j < c->nb_args[i]; j ++ ) {
			free(c->cmd_args[i][j]);
		}
		free(c->cmd_args[i]);
	}
	free(c->cmd_args);
}

void parse_all_redirection(cmd* c) {
	unsigned int i;
	c->redirection = (char***)malloc(c->nb_cmd_membres * sizeof(char**));
	c->type_redirection = (int**)malloc(c->nb_cmd_membres * sizeof(int*));
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		parse_redirection( i, c );
	}
}


//command1 [[param1_1] ...[param1_n]] [<</< file] [>>/> file]
//on suppose '<' est tourjours avant '>'
int parse_redirection(unsigned int i, cmd *c) {
	char* pi;
	char* pj = NULL; //l'index de '<'
	char* pk = NULL; //l'index de '>'
	int j;
	//initialise type_redirection[i] et redirection[i]
	c->redirection[i] = (char**)malloc(3 * sizeof(char*));
	c->type_redirection[i] = (int*)malloc( 3 * sizeof(int) );
	for ( j = 0; j < 3; j ++ ) {
		c->redirection[i][j] = NULL;
		c->type_redirection[i][j] = WRITE;
	}

	//pour trouver l'indix de '<' et '>'
	for ( pi = c->cmd_membres[i]; *pi != '\0'; pi ++ ) {
		if ( *pi == '<' ) {
			pj = pi;
		} else if ( *pi == '>' ) {
			pk = pi - 1;
			if ( *(pi-1) == '2' || *(pi-1) == '1' || *(pi-1) == '&' ) {
				pk = pi - 2;
			}
			if ( *(pi-1) == '2' ) {	// 2>
				if ( *(pi+1) == '>' ) {	//il existe '2>>'
					c->type_redirection[i][STDERR] = APPEND;
					c->redirection[i][STDERR] = strdup( pi + 2 );
				} else {
					c->redirection[i][STDERR] = strdup( pi + 1 );
				}
			} else if ( *(pi-1) == '&' ) { // &>
				if ( *(pi+1) == '>' ) {	//il existe '&>>'
					c->redirection[i][STDERR] = strdup( pi + 1 );
					c->type_redirection[i][STDERR] = APPEND;
					c->type_redirection[i][STDOUT] = APPEND;
					c->redirection[i][STDERR] = strdup( pi + 2 );
					c->redirection[i][STDOUT] = strdup( pi + 2 );
				} else {
					c->redirection[i][STDERR] = strdup( pi + 1 );
					c->redirection[i][STDOUT] = strdup( pi + 1 );
				}
			} else { // > || 1>
				if ( *(pi+1) == '>' ) {	//il existe '>>'
					c->redirection[i][STDOUT] = strdup( pi + 2 );
					c->type_redirection[i][STDOUT] = APPEND;
				} else {
					c->redirection[i][STDOUT] = strdup( pi + 1 );
				}
			}
			trim(c->redirection[i][STDOUT]);
			trim(c->redirection[i][STDIN]);
			trim(c->redirection[i][STDERR]);
			break;
		}
	}

	//il existe un '<'
	if ( pj != NULL ) {
		if ( pk == NULL ) {
			c->redirection[i][STDIN] = strdup( pj + 1 );
		} else {
			c->redirection[i][STDIN] = strndup( pj + 1, pk - pj );
		}
		trim(c->redirection[i][STDIN]);
	}
	return 0;
}

void aff_all_redirection(cmd* c) {
	int i;
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		aff_redirection(c, i);
	}
}

void aff_redirection(cmd* c, int i) {
	if ( c->redirection[i][STDIN] != NULL ) {
		printf("Redirection[%d][STDIN] = \"%s\"\n", i, c->redirection[i][STDIN] );
	} else {
		printf("Redirection[%d][STDIN] = NULL\n", i );
	}
	if ( c->redirection[i][STDOUT] != NULL ) {
		printf("Redirection[%d][STDOUT] = \"%s\"\n", i, c->redirection[i][STDOUT] );
	} else {
		printf("Redirection[%d][STDOUT] = NULL\n", i );
	}
	if ( c->redirection[i][STDERR] != NULL ) {
		printf("Redirection[%d][STDERR] = \"%s\"\n", i, c->redirection[i][STDERR] );
	} else {
		printf("Redirection[%d][STDERR] = NULL\n", i );
	}

	if ( c->type_redirection[i][STDIN] == APPEND ) {
		printf("Type_redirection[%d][STDIN] = APPEND\n", i);
	}
	if ( c->type_redirection[i][STDOUT] == APPEND ) {
		printf("Type_redirection[%d][STDOUT] = APPEND\n", i);
	}
	if ( c->type_redirection[i][STDERR] == APPEND ) {
		printf("Type_redirection[%d][STDERR] = APPEND\n", i);
	}
}
void free_redirection(cmd* c) {
	int i;
	for ( i = 0; i < c->nb_cmd_membres; i ++ ) {
		free(c->redirection[i][STDIN]);	
		free(c->redirection[i][STDOUT]);	
		free(c->redirection[i][STDERR]);	
		free(c->redirection[i]);
	}
	free(c->redirection);
}
