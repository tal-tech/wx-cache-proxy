#include <hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
gcc -g -O2 -fPIC -Wall -Wsign-compare -Wfloat-equal -Wpointer-arith -Wcast-align   -I/home/www/hiredis  -o test_redis test_redis.c    /home/www/hiredis/libhiredis.a 
*/

redisContext * conn_redis(const char* host, int port, const char* auth, const char* cmd)
{
    redisContext *c;
    redisReply *reply;

    if(NULL == host || port <= 0 || NULL == auth)
    {
        return NULL;
    }
   
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(host, port, timeout);
    if (c == NULL || c->err)
    {
        if (c)
        {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        return NULL;
    }

    //auth密码验证
    //char cmd[100];
    //sprintf(cmd, "*2\r\n$4\r\nauth\r\n$3\r\nabc\r\n");
    //printf("CCCCCCCCCCCCCCCCCCCCC %s\n", cmd);
   // reply = redisCommandResp(c, cmd);

    reply = redisCommand(c, "auth abc");
    //reply = redisCommandResp(c, "AUTH %s", auth);
    printf("aaaaaaaaaaaa AUTH: %s\n", reply->str);
    freeReplyObject(reply);


    reply = redisCommandResp(c, cmd);
    //reply = redisCommandResp(c, "AUTH %s", auth);
    printf("res %s\n", reply->str);
    freeReplyObject(reply);

    return c;
}

int main(int   argc,   char*   argv[])
{
    char cmd[100] = {'\0'};
    int i;
    
    for (i=1; i<argc; i++)
    {
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }
   
    printf("cmd %s\n", cmd);
    conn_redis("10.90.100.13", 22121, "abc", cmd);

    return 0;
}


