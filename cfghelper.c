#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfghelper.h"

void loadcfg(char *filename, mucfg *file) {
	FILE *fp;
	int port, length;
	char *headerfile, *footerfile;
	char *del, *parser, *prev, *buffer;
	
	fp=fopen(filename, "rb");
	printf("Using %s as configuration file!\n", filename);
	if(fp==NULL) {
		fprintf(stderr, "Configuration could not be loaded!\n");
		exit(3);
	}
	
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buffer = malloc(length); /* char is 1 */
	if (buffer) {
		fread(buffer, 1, length, fp);
	} else {
		fprintf(stderr, "Configuration could not be loaded!\n");
		exit(3);
	}
	fclose(fp);
	parser = buffer;
	del = strchr(parser, '\n');
	prev = parser;
	while(del != NULL) {
		*del = '\0';
		if((*(prev)=='#')||(*(prev)=='\n')||(*(prev)==' ')||(*(prev)=='\r')) {
			//starts with comment
		} else if (*(prev)=='P'){
			printf("Loaded port info from line: %s\n", prev);
			prev+=5;
			port = atoi(prev);
			if(port < 0 || port >60000) {
				fprintf(stderr, "The port number specified is invalid! It must be 1->60000\n");
				exit(3);
			}
			file->port = port;
		} else if (*(prev)=='H'){ /* HEADER_FILE */
			printf("Loaded header file from line: %s\n", prev);
			prev+=12;
			file->head = strdup(prev);
		} else if (*(prev)=='F'){ /* FOOTER_FILE */
			printf("Loaded footer file from line: %s\n", prev);
			prev+=12;
			file->foot = strdup(prev);
		} else if (*(prev)=='S'){ /* SERVE */
			printf("Loaded serving dir from line: %s\n", prev);
			prev+=6;
			file->srvdir = strdup(prev);
		} else if (*(prev)=='C'){  /* CACHE */
			printf("Loaded cache from line: %s\n", prev);
			prev+=6;
			if((*(prev)=='Y')||(*(prev)=='y')||(*(prev)=='1')) {
				file->cache = 1;
			} else if((*(prev)=='N')||(*(prev)=='n')||(*(prev)=='0')) {
				file->cache = 0;
			} else {
				fprintf(stderr, "Invalid option for CACHE! Only Y/N, y/n, or 1/0 allowed!\n");
				exit(3);
			}
		} else if (*(prev)=='T'){ /* TIME_CACHE */
			printf("Cache timing loaded from line: %s\n", prev);
			prev+=11;
			file->cachetime = atoi(prev)*60;
		}/* else {
			fprintf(stderr, "You have an invalid configuration file!\n");
			exit(3);
		}*/
		parser = del+1;
		del = strchr(parser, '\n');
		prev=parser;
	}
	free(buffer);
}

void cleanupcfg(mucfg *file) {
	free(file->head);
	free(file->foot);
	free(file->srvdir);
	free(file);
}