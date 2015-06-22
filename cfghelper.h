#ifndef _CFGHELPER_INC
#define _CFGHELPER_INC
typedef struct cfg_loadedinfo {
	int port;
	char *head;
	char *foot;
	char *srvdir;
	int cache, cachetime;
} mucfg;
void loadcfg(char *filename, mucfg *file);
void cleanupcfg(mucfg *file);
#endif