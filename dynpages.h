#include "cfghelper.h"
#ifndef _DYNPAGES_INC
#define _DYNPAGES_INC
void gen400 (int fd);
void gen404 (int fd, char *filename);
void gen403 (int fd, char *msg);
void gen405 (int fd);
void genmkd (int wfd, char *file);
void loadhf (mucfg *cf);
void cleanupfiles (mucfg *file);
int cacheCheck(char *file);
#endif