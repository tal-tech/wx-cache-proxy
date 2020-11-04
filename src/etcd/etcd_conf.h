#ifndef ETCD_CONF_H
#define ETCD_CONF_H
#include <stdlib.h>
struct conf_data
{
	char *etcdHost;
	char *key;
	char *value;
	char* logFilename;
	int ttl;
    int interval;
    char* user;
    char* pwd;
};

struct conf_data* parseConf(char* fileName);

#endif
