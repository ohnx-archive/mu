#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "dynpages.h"
#include "cfghelper.h"
#define VERSION     1
#define BUFSIZE  8191
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"md","text/html"},
	{"markdown","text/html"},
	{"gif", "image/gif" },  
	{"jpg", "image/jpg" }, 
	{"jpeg","image/jpeg"},
	{"png", "image/png" },  
	{"ico", "image/ico" },  
	{"zip", "image/zip" },  
	{"gz",  "image/gz"  },  
	{"tar", "image/tar" },  
	{"htm", "text/html" },  
	{"html","text/html" },  
	{"css", "text/css" },
	{"js","text/javascript"},
	{0,0}
};

mucfg *config;

void sig_handler(int signo) {
  if (signo == SIGTERM) {
	  cleanupfiles(config);
	  cleanupcfg(config);
	  exit(0);
  }
}


/* this is a child web server process, so we can exit on errors */
void web(int fd) {
	int j, file_fd, buflen;
	long i, ret, len;
	char * fstr;
	static char buffer[BUFSIZE+1]; /* static so zero filled */

	ret =read(fd,buffer,BUFSIZE); 	/* read Web request in one go */
	if(ret == 0 || ret == -1) /* read failure stop now */
		gen400(fd);

	if(ret > 0 && ret < BUFSIZE)	/* return code is valid chars */
		buffer[ret]=0;		/* terminate the buffer */
	else buffer[0]=0;
	for(i=0;i<ret;i++)	/* remove CF and LF characters */
		if(buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i]='*';
		
	if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) )
		gen405(fd);
	
	for(i=4;i<BUFSIZE;i++) { /* null terminate after the second space to ignore extra stuff */
		if(buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
			buffer[i] = 0;
			break;
		}
	}
	for(j=0;j<i-1;j++) 	/* check for illegal parent directory use .. */
		if(buffer[j] == '.' && buffer[j+1] == '.')
			gen403(fd, "Parent directory (..) path names not allowed");
	
	else if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ) /* convert no filename to index file */
		(void)strcpy(buffer,"GET /index.html");

	/* work out the file type and check we support it */
	buflen=strlen(buffer);
	fstr = (char *)0;
	for(i=0;extensions[i].ext != 0;i++) {
		len = strlen(extensions[i].ext);
		if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			if(i==0||i==1) /* first 2 types are .md and .markdown */
				genmkd(fd, &buffer[5]);
			fstr =extensions[i].filetype;
			break;
		}
	}
	
	if(fstr == 0) gen404(fd, &buffer[5]);

	/* open the file for reading */
	if(( file_fd = open(&buffer[5],O_RDONLY)) == -1) gen404(fd, &buffer[5]);
	
	len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
	      (void)lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */
    
	(void)sprintf(buffer,"HTTP/1.1 200 OK\nServer: mu/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", VERSION, len, fstr); /* Header + a blank line */
	(void)write(fd,buffer,strlen(buffer));

	/* send file in 8KB block - last block may be smaller */
	while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
		(void)write(fd,buffer,ret);
	}
	sleep(1);	/* allow socket to drain before signalling the socket is closed */
	close(fd);
	exit(1);
}

int main(int argc, char **argv) {
	int i, port, pid, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

	config = malloc(sizeof(mucfg));
	if(argc==2) {loadcfg(argv[1], config);} else {loadcfg("simple.cfg", config);}
	loadhf(config);
	
	if(chdir(config->srvdir) == -1){ 
		fprintf(stderr, "ERROR: Can't Change to directory %s\n",config->srvdir);
		exit(4);
	}
	
	/* Become daemon + unstoppable and no zombies children (= no wait()) */
	if(fork() != 0) {
		sleep(3);
		return 0;
	}
	/* now forked */
	printf("mu version %d is loading...\n", VERSION);
	(void)signal(SIGCLD, SIG_IGN); /* ignore child death */
	(void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		fprintf(stderr, "ERROR: Can't Change to directory %s\n",config->srvdir);
		exit(3);
	}
	
	for(i=0;i<32;i++)
		(void)close(i);		/* close open files */
	
	(void)setpgrp();		/* break away from process group */

	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0) {
		fprintf(stderr, "ERROR: Couldn't create the socket!\n");
		exit(3);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(config->port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
		fprintf(stderr, "ERROR: Could not bind the socket!");
		exit(3);
	}
	
	if( listen(listenfd,64) <0) {
		fprintf(stderr, "ERROR: Could not listen on the socket!");
		exit(3);
	}
	while (1) {
		length = sizeof(cli_addr);
		
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			exit(3);
		if((pid = fork()) < 0) {
			exit(3);
		} else {
			if(pid == 0) { 	/* child */
				(void)close(listenfd);
				web(socketfd); /* never returns */
			} else { 	/* parent */
				(void)close(socketfd);
			}
		}
	}
}