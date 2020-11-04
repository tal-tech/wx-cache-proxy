#ifndef NC_TIME_H
#define NC_TIME_H

#include <sys/uio.h>
#include <glib.h>
#include <nc_core.h>
#include <nc_server.h>
#include <proto/nc_proto.h>
#include <time.h>

//获取毫秒
int64_t get_current_time(void);
void get_current_time_str(GString *str);
void get_time_str(GString *date, uint64_t timestamp);
#endif