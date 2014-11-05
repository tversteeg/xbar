#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int sys_output(char **buf, char *command)
{
	FILE *fp;
	char output[1035];
	int size;

	fp = popen(command, "r");
	if(fp == NULL){
		fprintf(stderr, "Failed to run command: %s\n", command);
		exit(1);
	}

	fgets(output, sizeof(output) - 1, fp);

	size = strlen(output);
	*buf = malloc(size);
	strcpy(*buf, output);
	(*buf)[size - 1] = '\0';

	fclose(fp);

	return size;
}

int main(int argc, char **argv)
{
	char config_path[256] = "~/.config/barrc";
	char *command = NULL, *output = NULL;
	int opt, len, verbose = 0;

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
		
		len = sys_output(&output, command);
		free(command);

		printf("Output: %s\n", output);
		free(output);
	}

	return 0;
}
