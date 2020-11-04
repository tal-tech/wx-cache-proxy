#include "nc_time.h"

//获取微秒
int64_t get_current_time(void) 
{
    struct timeval now;
    int64_t usec;
    int status;

    status = gettimeofday(&now, NULL);
    if (status < 0) {
        log_error("gettimeofday failed: %s", strerror(errno));
        return -1;
    }

    usec = (int64_t) now.tv_sec * 1000000LL + (int64_t) now.tv_usec;

    return usec;
}

void get_current_time_str(GString *str) {
    if (!str) {
        g_critical("str is NULL when call get_current_time_str()");
        return;
    }
    struct tm *tm;
    GTimeVal tv;
    time_t  t;

    g_get_current_time(&tv);
    t = (time_t) tv.tv_sec;
    tm = localtime(&t);

    str->len = strftime(str->str, str->allocated_len, "%Y-%m-%d %H:%M:%S", tm);
    g_string_append_printf(str, ".%.3d", (int) tv.tv_usec/1000);
}

void get_time_str(GString *date, uint64_t timestamp) {
    if (!date) {
        g_critical("str is NULL when call get_current_time_str()");
        return;
    }

    time_t t;
    struct tm *p;
    t=(time_t)timestamp/1000+28800;
    p=gmtime(&t);

    date->len = strftime(date->str, date->allocated_len, "%Y-%m-%d %H:%M:%S", p);
    g_string_append_printf(date, ".%.3d", timestamp%1000);

//    time_t rawtime;
//   struct tm *info;
//   char buffer[80];
//
//   time( &rawtime );
//
//   info = localtime( &rawtime );
//
//   strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
//   printf("格式化的日期 & 时间 : |%s|\n", buffer );

}
