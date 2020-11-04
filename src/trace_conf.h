#ifndef TRACE_CONF_H
#define TRACE_CONF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include <nc_log.h>

struct conf_trace_data
{  
    int dept_id;
    char* app_name;
    char* log_filename;
    char* room;
    int slow_threshold;
};

struct conf_trace_data* malloc_trace_conf(void);
struct conf_trace_data* parse_trace_conf(const char* file_name);
void free_trace_conf(struct conf_trace_data* conf);

#endif