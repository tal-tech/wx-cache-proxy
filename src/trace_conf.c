#include "trace_conf.h"

void free_trace_conf(struct conf_trace_data* conf)
{
	if(NULL != conf)	
	{
		if(NULL != conf->app_name)
		{
			free(conf->app_name);
			conf->app_name = NULL;
		}

		if(NULL != conf->log_filename)
		{
			free(conf->log_filename);
			conf->log_filename = NULL;
		}

        if(NULL != conf->room)
		{
			free(conf->room);
			conf->room = NULL;
		}
	}
}

struct conf_trace_data* malloc_trace_conf()
{
    struct conf_trace_data* conf = (struct conf_trace_data* )calloc(1, sizeof(struct conf_trace_data));
    if (NULL == conf)
    {
        return NULL;
    }
    
    conf->dept_id=0;

    conf->app_name = (char*)calloc(100, sizeof(char));
    if (NULL == conf->app_name)
    {
        return NULL;
    }

    conf->log_filename = (char*)calloc(100, sizeof(char));
    if (NULL == conf->log_filename)
    {
        return NULL;
    }

    conf->room = (char*)calloc(100, sizeof(char));
    if (NULL == conf->room)
    {
        return NULL;
    }

    return conf;
}

struct conf_trace_data* parse_trace_conf(const char* filename) {
	FILE *fp = fopen(filename, "r");
    fseek(fp, 0, SEEK_END);

    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = (char*)calloc(len + 1, (unsigned int)sizeof(char));
    fread(data, 1, len, fp);
    fclose(fp);
    printf("%s\n", data);

	cJSON *json =cJSON_Parse(data); 
	if (NULL == json )
	{
		printf("error:%s\n", cJSON_GetErrorPtr());
		cJSON_Delete(json);
		return NULL;
	}

	cJSON * item = NULL;
    
	char *app_name = NULL;
    char *log_filename = NULL;
    char *room = NULL;

    struct conf_trace_data* conf = malloc_trace_conf();
    if (NULL == conf)
    {
        printf("UUUUUUUUUUUUUUUU error\n");
        return NULL;
    }
    
    item = cJSON_GetObjectItem(json, "dept_id");
    if (NULL == item)
    {
        log_error("need dept_id");
        return NULL;
    }
    conf->dept_id = item->valueint;

    item = cJSON_GetObjectItem(json, "app_name");
    if (NULL == item)
    {  
        log_error("need app_name");
        return NULL;
    } 
    app_name = item->valuestring;
	memcpy(conf->app_name, app_name, strlen(app_name));
	
    item = cJSON_GetObjectItem(json, "log_filename");
    if (NULL == item)
    {
        log_error("need log_filename");
        return NULL;
    }
    log_filename = item->valuestring;
    memcpy(conf->log_filename, log_filename, strlen(log_filename));
    
    item = cJSON_GetObjectItem(json, "room");
    if (NULL == item)
    {
        log_error("need room");
        return NULL;
    }
    room = item->valuestring;
	memcpy(conf->room, room, strlen(room));
    
    item = cJSON_GetObjectItem(json, "slow_threshold");
    if (NULL == item)
    {
        log_error("need slow_threshold");
        return NULL;
    }
    conf->slow_threshold = item->valueint;

	if (NULL != data)
	{
		free(data);
		data = NULL;
	}

    cJSON_Delete(json);
    
    printf("fff %p\n", conf);
	return conf;
}