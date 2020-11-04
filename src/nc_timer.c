#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define CLOCKID CLOCK_REALTIME
void timer_thread(union sigval v);
void set_timmer();
 
/*
gcc -o tt tt.c  -pthread -lrt
*/
void timer_thread(union sigval v)
{
	printf("timer_thread function! %d\n", v.sival_int);
}

void set_timmer() {
    // XXX int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);
	// clockid--值：CLOCK_REALTIME,CLOCK_MONOTONIC,CLOCK_PROCESS_CPUTIME_ID,CLOCK_THREAD_CPUTIME_ID
	// evp--存放环境值的地址,结构成员说明了定时器到期的通知方式和处理方式等
	// timerid--定时器标识符
	timer_t timerid;
	struct sigevent evp;
	memset(&evp, 0, sizeof(struct sigevent));    //清零初始化
 
	evp.sigev_value.sival_int = 111;        //也是标识定时器的，这和timerid有什么区别？回调函数可以获得
	evp.sigev_notify = SIGEV_THREAD;        //线程通知的方式，派驻新线程
	evp.sigev_notify_function = timer_thread;    //线程函数地址
 
	if (timer_create(CLOCKID, &evp, &timerid) == -1)
	{
		perror("fail to timer_create");
		exit(-1);
	}

    // XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);
	// timerid--定时器标识
	// flags--0表示相对时间，1表示绝对时间，通常使用相对时间
	// new_value--定时器的新初始值和间隔，如下面的it
	// old_value--取值通常为0，即第四个参数常为NULL,若不为NULL，则返回定时器的前一个值
	
	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长,就是说it.it_value变0的时候会装载it.it_interval的值
	//it.it_interval可以理解为周期
	struct itimerspec it;
	it.it_interval.tv_sec = 4;    //间隔1s
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = 1;        
	it.it_value.tv_nsec = 0;
 
	if (timer_settime(timerid, 0, &it, NULL) == -1)
	{
		perror("fail to timer_settime");
		exit(-1);
	}
}
     
//int main()
//{
//    set_timmer();
//
//	pid_t pid;
//	int core_is_master;
//	int i;
//	for (i = 0; i < 5; i++) {
//		pid = fork();
//		if (pid > 0) {
//			core_is_master = 1;
//			continue;
//		} else if (pid == 0) {
//			core_is_master = 0;
//			break;
//		} else {
//			printf("fork error");
//			exit(1);
//		}
//	}
//	
//	int child_status;
//	pid_t new_pid;
//	if (core_is_master) 
//	{
//		while(1) 
//		{
//			printf("test\n");
//			pid = wait(&child_status);	
//			if (pid > 0) {
//				printf("fork, 1111111111111111111	\n");
//			}
//		}
//	} else {
//		while(1)
//		{
//			sleep(2);
//		}
//	}
// 
//	return 0;
//}