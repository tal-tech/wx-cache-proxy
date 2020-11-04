#include "cetcd.h"
#include  "b64.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define CLOCKID CLOCK_REALTIME


#include "etcd_conf.h"
#include "log.h"

void timer_thread(union sigval v);
int set_timmer(struct conf_data* conf);
int first_apply_key(cetcd_client *cli, char* key, char* value, uint64_t ttl, char* lease);
int connect_etcd(char* hosts, char* key, char* value, uint64_t ttl, int flag);

int first_conn_etcd = 0;
char* lease;
//char* gConfPath;
	
void timer_thread(union sigval v)
{	
    struct conf_data* conf= (struct conf_data*)v.sival_ptr;

    if (0 == first_conn_etcd )
    {
        xes_log("第一次执行");
        xes_log("############################################################\n");

        first_conn_etcd = 1;
        lease = (char*)calloc(200, sizeof(char));

        connect_etcd(conf->etcdHost, conf->key, conf->value, conf->ttl, 0);
    } 
    else 
    {
        xes_log("续约执行");
        xes_log("=============================================================\n");

        connect_etcd(conf->etcdHost, conf->key, conf->value, conf->ttl, 1);
    }

    xes_log("timer_thread function! %d\n", v.sival_int);
}

int set_timmer(struct conf_data* conf) 
{
    int status;

    status = log_init_etcd(5, conf->logFilename);
    if (status != 0) {
        printf("log_init_etcd error %d\n", status);
        return status;
    }


	//gconf_data = initConf(gConfPath); 
    // XXX int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);
	// clockid--值：CLOCK_REALTIME,CLOCK_MONOTONIC,CLOCK_PROCESS_CPUTIME_ID,CLOCK_THREAD_CPUTIME_ID
	// evp--存放环境值的地址,结构成员说明了定时器到期的通知方式和处理方式等
	// timerid--定时器标识符
    timer_t timerid;
    struct sigevent evp;
    memset(&evp, 0, sizeof(struct sigevent));    //清零初始化

    evp.sigev_value.sival_int = 111;        //也是标识定时器的，这和timerid有什么区别？回调函数可以获得
    evp.sigev_value.sival_ptr = conf;
    evp.sigev_notify = SIGEV_THREAD;        //线程通知的方式，派驻新线程
    evp.sigev_notify_function = timer_thread;    //线程函数地址

    if (timer_create(CLOCKID, &evp, &timerid) == -1)
    {
        perror("fail to timer_create");
        return 1;
    }

    // XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);
	// timerid--定时器标识
	// flags--0表示相对时间，1表示绝对时间，通常使用相对时间
	// new_value--定时器的新初始值和间隔，如下面的it
	// old_value--取值通常为0，即第四个参数常为NULL,若不为NULL，则返回定时器的前一个值
	
	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长,就是说it.it_value变0的时候会装载it.it_interval的值
	//it.it_interval可以理解为周期
    struct itimerspec it;
    //it.it_interval.tv_sec = 2;    //间隔1s
    it.it_interval.tv_sec = conf->interval;    //间隔1s
    it.it_interval.tv_nsec = 0;
    it.it_value.tv_sec = 1;        
    it.it_value.tv_nsec = 0;
 
    if (timer_settime(timerid, 0, &it, NULL) == -1)
    {
        perror("fail to timer_settime");
        exit(-1);
    }
}

/*
参考 https://www.cnblogs.com/catgatp/p/6379955.html
*/
int first_apply_key(cetcd_client *cli, char* key, char* value, uint64_t ttl, char* lease)
{
    int res;

    size_t lenKey = strlen(key);
    char *keyBase64 = b64_encode((const unsigned char*)key, lenKey);

    size_t lenValue = strlen(value);
    char *valueBase64 = b64_encode((const unsigned char*)value, lenValue);
    xes_log("keyBase64 %s, valueBase64 %s\n", keyBase64, valueBase64);

    //获取lease
    //char* lease = (char*)calloc(200, sizeof(char));
    res = cetcd_lease(cli, ttl, lease);
    if(!res) 
    {
        xes_log("获取lease失败");
        return 1;
    }
    
    xes_log("获取lease %s\n\n", lease);

    //key绑定lease
    res = cetcd_set(cli, keyBase64, valueBase64, lease);
    if(!res) {
        xes_log("写key/value, 失败");
        goto free;
    }

    xes_log("绑定key %s成功", key);

    free:
        if(NULL != keyBase64)
        {
            free(keyBase64);
            keyBase64 = NULL;
        }

        if(NULL != valueBase64)
        {
            free(valueBase64);
            valueBase64 = NULL;
        }

        if(0 == strcmp(lease, ""))
        {
            return 1;
        }
        else
        {
            return 0;
        }
}

int connect_etcd(char* hosts, char* key, char* value, uint64_t ttl, int flag)
{
    char delim[] = ",";
    char *host = NULL;
    int res;
    long long timetolive_ttl = 0;

    xes_log("init_etcd receive host list is %s", hosts);

    int hostsLen = strlen(hosts) + 1;
    char* localHosts = (char*)malloc(sizeof(char) * hostsLen);
    memset(localHosts, 0, hostsLen);
    memcpy(localHosts, hosts, hostsLen);

    cetcd_client cli;
    cetcd_array addrs;
    cetcd_array_init(&addrs, 5);

    for(host = strtok(localHosts, delim); host != NULL; host = strtok(NULL, delim))
    {
        //printf("host is %s\n", host);
        xes_log("init_etcd host is %s", host);
        cetcd_array_append(&addrs, host);
    }

    cetcd_client_init(&cli, &addrs);

    if(0 == flag)
    {
        res = first_apply_key(&cli, key, value, ttl, lease);
        if(0 == res)
        {
            xes_log("第一次申请key成功\n");
        }
        else
        {
            xes_log("第一次申请key失败\n");
        }
    }
    else
    {
        //查看剩余时间
        timetolive_ttl = cetcd_timetolive(&cli, lease);
        xes_log("上次剩余时间ttl %ld\n\n", timetolive_ttl);

        if(timetolive_ttl < 0)
        {
            res = first_apply_key(&cli, key, value, ttl, lease);
            if(0 == res)
            {
                xes_log("续约时, 发生意外, 重新申请key成功\n");
            }
            else
            {
                xes_log("续约时, 发生意外, 重新申请key失败\n");
            }
        }

        //一直刷新 保活
        timetolive_ttl = cetcd_keepalive(&cli, lease);
        xes_log("本次刷新保活后, ttl %ld\n\n", timetolive_ttl);
    }

    cetcd_array_destroy(&addrs);
    cetcd_client_destroy(&cli);

    if(NULL != localHosts) 
    {
        free(localHosts);
        localHosts = NULL;
    }
   
    return 0;
}