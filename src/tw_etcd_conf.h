#ifndef TW_ETCD_CONF_H
#define TW_ETCD_CONF_H
#include <stdlib.h>
typedef struct 
{
	char *host;
	char *key;
	char *value;
	int ttl;
    int interval;
}ConfData;


ConfData *parseConf(char* fileName);

#endif
