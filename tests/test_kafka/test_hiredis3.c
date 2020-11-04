#include "hiredis.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
 
/*
gcc -o test_hiredis3 test_hiredis3.c -I../../src/mq/include/hiredis -L/usr/local/lib -lhiredis
*/

redisContext * conn_redis(const char* host, int port, const char* auth)
{
    redisContext *c;
    redisReply *reply;

    if(NULL == host || port <= 0 || NULL == auth)
    {
        return NULL;
    }
   
    struct timeval timeout = { 5, 500000 }; // 1.5 seconds
    int i;
    for(i=0; i< 3; i++)
    {
        c = redisConnectWithTimeout(host, port, timeout);
        if (c == NULL || c->err)
        {
            if (c)
            {
                printf("connection Tw error: %s, auth:%s, port:%d, auth:%s\n", c->errstr, host, port, auth);
                redisFree(c);
            } else {
                printf("connection Tw error: can't allocate redis context, auth:%s, port:%d, auth:%s\n", host, port, auth);
            }
            sleep(1);

            if (i == 2)
            {
                return NULL;
            }
        }
        else
        {
            printf("connection Tw success\n");
            break;
        }
    }
    
    //auth密码验证
    char cmd[200] = {0};
    if ( 0 != strcmp(auth, ""))
    {
        sprintf(cmd, "*2\r\n$4\r\nauth\r\n$6\r\n%s\r\n", auth);
        reply = redisCommandOriginResp(c, cmd);
        
        if(strstr(reply->str, "invalid"))
        {
            printf("connect %s:%d failed, auth error %s\n", host, port, reply->str);
            freeReplyObject(reply);
            return NULL;
        }
        else 
        {
            printf("connect %s:%d success, auth res %s\n", host, port, reply->str);
        }

        freeReplyObject(reply);
    }
    
    return c;
}

int exec(redisContext *conn, const char* cmd)
{
    redisReply *reply;
    reply = redisCommandOriginResp(conn, cmd);

    printf("write to tw %s\n",  reply->str);
    freeReplyObject(reply);

    return 0;
}

int main(int argc, char** argv)
{
    const char* host = "10.90.100.13";
    int port = 6379;
    const char* auth = "123456";

    redisContext * conn = conn_redis(host, port, auth);
    
    const char* cmd = "*3\r\n$3\r\nset\r\n$4\r\nname\r\n$2\r\ncd\r\n";
    exec(conn, cmd);

    return 0;
}