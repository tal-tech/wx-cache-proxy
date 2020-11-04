#include<stdio.h>
#include<time.h>
#include<sys/time.h>

/*
https://www.cnblogs.com/book-gary/p/3716790.html

二、struct timeval 定义：

struct timeval {
time_t tv_sec;	// seconds 
long tv_usec;	// microseconds 
};
struct timezone{ 
int tz_minuteswest; //miniutes west of Greenwich 
int tz_dsttime; //type of DST correction 
};

struct timeval有两个成员，一个是秒，一个是微秒, 所以最高精确度是微秒。
一般由函数int gettimeofday(struct timeval *tv, struct timezone *tz)获取系统的时间 

gcc -o test_time test_time.c
*/
void nowtime_ns()
{
    printf("---------------------------struct timespec---------------------------------------\n"); 
    printf("[time(NULL)]     :     %ld\n", time(NULL)); 
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("clock_gettime : tv_sec=%ld, tv_nsec=%ld\n", ts.tv_sec, ts.tv_nsec);
    
    struct tm t;
    char date_time[64];
    strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", localtime_r(&ts.tv_sec, &t));
    printf("clock_gettime : date_time=%s, tv_nsec=%ld\n", date_time, ts.tv_nsec);
}
void nowtime_us()
{
    printf("---------------------------struct timeval----------------------------------------\n"); 
    printf("[time(NULL)]    :    %ld\n", time(NULL)); 
    struct timeval us;
    gettimeofday(&us,NULL);
    printf("gettimeofday: tv_sec=%ld, tv_usec=%ld\n", us.tv_sec, us.tv_usec);
    
    struct tm t;
    char date_time[64];
    strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", localtime_r(&us.tv_sec, &t));
    printf("gettimeofday: date_time=%s, tv_usec=%ld\n", date_time, us.tv_usec);
    printf("aa us.tv_sec=%ld, us.tv_usec=%ld, total=%ld\n", us.tv_sec, us.tv_usec, us.tv_sec* 1000000 + us.tv_usec);
}

int main(int argc, char* argv[])
{
    nowtime_ns();
    printf("\n");
    nowtime_us();
    printf("\n");
    printf("===========================\n");
     nowtime_us();
    printf("\n");
    return 0;
}
