#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "etcd_conf.h"
#include "cJSON.h"

void freeConf(struct conf_data* conf)
{
	if(NULL != conf)	
	{
		if(NULL != conf->etcdHost)
		{
			free(conf->etcdHost);
			conf->etcdHost = NULL;
		}

		if(NULL != conf->key)
		{
			free(conf->key);
			conf->key = NULL;
		}

		if(NULL != conf->value)
		{
			free(conf->value);
			conf->value = NULL;
		}
      
		if(NULL != conf->logFilename)
		{
			free(conf->logFilename);
			conf->logFilename = NULL;
		}

//        if(NULL != conf->user)
//		{
//			free(conf->user);
//			conf->user = NULL;
//		}
//
//        if(NULL != conf->pwd)
//		{
//			free(conf->pwd);
//			conf->pwd = NULL;
//		}

		free(conf);
	}
}

struct conf_data* parseConf(char* fileName) {
	
	FILE *fp = fopen(fileName, "r");
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *data = (char*)calloc(len + 1, sizeof(char));
    fread(data, 1, len, fp);
    fclose(fp);
    printf("%s\n", data);

	cJSON *json =cJSON_Parse(data); 
	if (NULL == json )
	{
		printf("error:%s\n", cJSON_GetErrorPtr());
		cJSON_Delete(json );
		return NULL;
	}
	
	char *etcdHost = NULL;
	etcdHost = cJSON_GetObjectItem(json, "host")->valuestring;

	char *key = NULL;
	key = cJSON_GetObjectItem(json, "key")->valuestring;

    char *value = NULL;
    value = cJSON_GetObjectItem(json, "value")->valuestring;

	char *logFilename = NULL;
	logFilename = cJSON_GetObjectItem(json, "log_filename")->valuestring;
	
	int ttl = 0;
	ttl = cJSON_GetObjectItem(json, "ttl")->valueint;

    int interval = 0;
	interval = cJSON_GetObjectItem(json, "interval")->valueint;

//    char* user = NULL;
//	user = cJSON_GetObjectItem(json, "user")->valuestring;
//
//    char* pwd = NULL;
//	pwd = cJSON_GetObjectItem(json, "pwd")->valuestring;

	struct conf_data* conf = (struct conf_data* )calloc(1, sizeof(struct conf_data));

	conf->etcdHost = (char*)calloc(strlen(etcdHost)+1, sizeof(char));
	memcpy(conf->etcdHost, etcdHost, strlen(etcdHost));
	
	conf->key = (char*)calloc(strlen(key) + 1, sizeof(char));
	memcpy(conf->key, key, strlen(key));

	conf->value = (char*)calloc(strlen(value)+1, sizeof(char));
	memcpy(conf->value, value, strlen(value));
    
	conf->logFilename = (char*)calloc(strlen(logFilename)+1, sizeof(char));
	memcpy(conf->logFilename, logFilename, strlen(logFilename));
    
//    conf->user = (char*)calloc(strlen(user)+1, sizeof(char));
//	memcpy(conf->user, user, strlen(user));
//
//    conf->pwd = (char*)calloc(strlen(pwd)+1, sizeof(char));
//	memcpy(conf->pwd, pwd, strlen(pwd));

	conf->ttl = ttl;
    conf->interval = interval;
	
	if (NULL != data)
	{
		free(data);
		data = NULL;
	}
	cJSON_Delete(json);

	return conf;
}

//int main() { 
//	char *fileName = "./tw_etcd.conf";
//	ConfData *conf = parseConf(fileName);
//	printf("host %s\n", conf->host);
//	printf("key %s\n", conf->key);
//	printf("value %s\n", conf->value);
//	printf("ttl is %d\n", conf->ttl);
//	
//	if(NULL != conf) {
//		if(NULL != conf->host) {
//			free(conf->host);
//		}
//
//		if(NULL != conf->key) {
//			free(conf->key);
//		}
//
//		if(NULL != conf->value) {
//			free(conf->value);
//		}
//	}
//	return 0;
//}    
