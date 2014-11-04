#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	char config_path[256] = "~/.config/barrc";
	char *command = NULL;
	int opt, verbose = 0;

	while((opt = getopt(argc, argv, "vc:p:")) != -1){
		switch(opt){
			case 'p':
				strcpy(config_path, optarg);
				break;
			case 'c':
				command = malloc(strlen(optarg));
				strcpy(command, optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case '?':
				return 1;
			default:
				break;
		}
	}

	if(verbose){
		printf("Reading configuration file: \"%s\"\n", config_path);
	}

	if(command != NULL){
		if(verbose){
			printf("Executing command: \"%s\"\n", command);
		}
		system(command);
		free(command);
	}

	return 0;
}
