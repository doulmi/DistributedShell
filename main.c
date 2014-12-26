#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

#include "shell_fct.h"
 
#define MYSHELL_CMD_OK 1
#define MYSHELL_FCT_EXIT 0

int main(int argc, char** argv)
{
	//..........
	int ret = MYSHELL_CMD_OK;
	cmd mycmd;
	char* readlineptr;
	struct passwd* infos;
	char str[1024];
	char hostname[256];
	char workingdirectory[256];

    using_history();
	while(ret != MYSHELL_FCT_EXIT)
	{
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
