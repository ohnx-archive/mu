#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "document.h"
#include "html.h"
#include "dynpages.h"
#include "cfghelper.h"

#define DEF_IUNIT 1024
#define DEF_OUNIT   64
#define BUFSIZE    256
#define BUFSIZET   512
#define VERSION      1

char *headerc, *footerc;
int hcl, fcl, cache;

void loadhf (mucfg *cf) {
	FILE *fp;
	int length;
	fp=fopen(cf->head, "rb");
	if(fp==NULL) {
		fprintf(stderr, "Header file could not be loaded!\n");
		exit(3);
	}
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	headerc = malloc(length+1); /* char is 1 */
	if (headerc) {
		fread(headerc, 1, length, fp);
	} else {
		fprintf(stderr, "Header file could not be loaded!\n");
		exit(3);
	}
	*(headerc+length) = '\0';
	hcl = strlen(headerc);
	fclose(fp);
	
	fp=fopen(cf->foot, "rb");
	if(fp==NULL) {
		fprintf(stderr, "Footer file could not be loaded!\n");
		exit(3);
	}
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	footerc = malloc(length+1); /* char is 1 */
	if (footerc) {
		fread(footerc, 1, length, fp);
	} else {
		fprintf(stderr, "Footer file could not be loaded!\n");
		exit(3);
	}
	*(footerc+length) = '\0';
	fcl = strlen(footerc);
	fclose(fp);
	if(cf->cache) {
		cache = cf->cachetime;
	} else {
		cache = 0;
	}
}

void cleanupfiles (mucfg *file) {
	free(headerc);
	free(footerc);
}
void gen400 (int fd) {
	(void)write(fd, "HTTP/1.1 400 Bad Request\nContent-Length: 161\nConnection: close\nContent-Type: text/html\n\n<html>\n<head>\n<title>400 Bad Request</title>\n</head>\n<body>\n<h1>Bad Request</h1>\nThe web server was not able to interpret your browser's request.\n</body>\n</html>",249);
	sleep(1);
	close(fd);
	exit(1);
}

void gen404 (int fd, char *filename) {
	static char buffer[BUFSIZET];
	static char buffert[BUFSIZE];
	sprintf(buffer, "<html>\n<head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested file <i>%s</i> was not found on this server.\n</body>\n</html>\n", filename);
	sprintf(buffert, "HTTP/1.1 404 Not Found\nContent-Length: %d\nConnection: close\nContent-Type: text/html\n\n", strlen(buffer));
	
	(void)write(fd, buffert, strlen(buffert));
	(void)write(fd, buffer, strlen(buffer));
	
	sleep(1);
	close(fd);
	exit(1);
}

void gen403 (int fd, char *msg) {
	//TODO: Add message for 403
	(void)write(fd, "HTTP/1.1 403 Forbidden\nContent-Length: 167\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this web server.\n</body></html>\n",253);
	sleep(1);
	close(fd);
	exit(1);
}

void gen405 (int fd) {
	//TODO: Make this a proper 405 message
	(void)write(fd, "HTTP/1.1 400 Method Not Allowed\nContent-Length: 161\nConnection: close\nContent-Type: text/html\n\n<html>\n<head>\n<title>400 Bad Request</title>\n</head>\n<body>\n<h1>Bad Request</h1>\nThe web server was not able to interpret your browser's request.\n</body>\n</html>",249);
	sleep(1);
	close(fd);
	exit(1);
}

void gen500 (int fd) {
	(void)write(fd, "HTTP/1.1 500 Internal Server Error\nContent-Length: 175\nConnection: close\nContent-Type: text/html\n\n<html>\n<head>\n<title>500 Internal Server Error</title>\n</head>\n<body>\n<h1>Internal Server Error</h1>\nUh oh. Something bad has happened. Please try again later.\n</body>\n</html>", 274);
	sleep(1);
	close(fd);
	exit(1);
}

void genmkd (int wfd, char *file) {
	FILE *fp;
	int length;
	char *cachefile, *putout;
	static char buffer[BUFSIZE];
	hoedown_buffer *ib, *ob;
	hoedown_document *document;
	unsigned int extensions = HOEDOWN_EXT_NO_INTRA_EMPHASIS | HOEDOWN_HTML_HARD_WRAP | HOEDOWN_EXT_TABLES | HOEDOWN_EXT_UNDERLINE | HOEDOWN_EXT_HIGHLIGHT | HOEDOWN_EXT_SUPERSCRIPT | HOEDOWN_EXT_STRIKETHROUGH | HOEDOWN_EXT_FENCED_CODE | HOEDOWN_EXT_AUTOLINK;
	
	/* cache check - if not needed, then just print out cached */
	/* Using while here so I can break; out as needed */
	while(cache) {
		/* This is needed just to add a 'p' to the end of the filename... */
		cachefile = malloc(strlen(file)+2);
		strcpy(cachefile, file);
		*(cachefile+strlen(file))='p';
		*(cachefile+strlen(file)+1)='\0';
		if(cacheCheck(cachefile)) {
			//read cached file now
			fp = fopen(cachefile, "rb");
			if(fp==NULL) {
				/* There was an error opening the cached file, just re-parse it. */
				break;
			}
			fseek(fp, 0, SEEK_END);
			length = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			putout = malloc(length+1); /* char is 1 */
			if (putout) {
				fread(putout, 1, length, fp);
			} else {
				gen500(wfd);
			}
			*(putout+length) = '\0';
			sprintf(buffer, "HTTP/1.1 200 OK\nServer: mu/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: text/html\n\n", VERSION, hcl+length+fcl);
			(void)write(wfd, buffer, strlen(buffer));
			(void)write(wfd, headerc, hcl);
			(void)write(wfd, putout, length);
			(void)write(wfd, footerc, fcl);
			sleep(1);
			close(wfd);
			exit(1);
		}
		/* continue to parse file */
		break;
	}

	/* If you don't understand this code, check out https://github.com/hoedown/hoedown/wiki/Getting-Started */
	fp = fopen(file, "r");
	if(fp==NULL) {
		gen500(wfd);
	}
	ib = hoedown_buffer_new(DEF_IUNIT);
	hoedown_buffer_putf(ib, fp);
	hoedown_renderer *renderer = hoedown_html_renderer_new(0, 0);
	
	/* Perform Markdown rendering */
	ob = hoedown_buffer_new(DEF_OUNIT);
	document = hoedown_document_new(renderer, extensions, 16);

	hoedown_document_render(document, ob, ib->data, ib->size);

	hoedown_buffer_free(ib);
	hoedown_document_free(document);
	hoedown_html_renderer_free(renderer);
	
	sprintf(buffer, "HTTP/1.1 200 OK\nServer: mu/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: text/html\n\n", VERSION, hcl+ob->size+fcl);
	
	(void)write(wfd, buffer, strlen(buffer));
	(void)write(wfd, headerc, hcl);
	(void)write(wfd, ob->data, ob->size);
	(void)write(wfd, footerc, fcl);
	
	fclose(fp);
	
	if(cache) {
		/* Don't quit on errors here */
		fp = fopen(cachefile, "w");
		fwrite(ob->data, 1, ob->size, fp);
		fclose(fp);
		free(cachefile);
	}
	hoedown_buffer_free(ob);
	
	sleep(1);
	close(wfd);
	exit(1);
}

int cacheCheck(char *file) {
	struct stat attr;
	time_t curr, past;
	struct tm *timeinfo;
	
	stat(file, &attr);
	past = attr.st_mtime;
	time (&curr);
	
	if(difftime(curr, past)>cache) {
		return 0;
	} else {
		return 1;
	}
}