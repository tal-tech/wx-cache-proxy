#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hiredis.h"

/*
gcc -o test_hiredis test_hiredis.c -I../../src/mq/include/hiredis -L/usr/local/lib -lhiredis
*/

int main()
{
    redisContext *c;
    redisReply *reply;
    
    struct timeval timeout = { 5, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout("10.90.100.13", 6379, timeout);
    if (c == NULL || c->err)
        {
            if (c)
            {
                printf("connection Tw error %s\n", c->errstr);
                redisFree(c);
            } else {
                printf("connection Tw error: can't allocate redis context\n");
            }
           
        }

        char cmd[100];
        sprintf(cmd, "auth 123456");
        reply = redisCommand(c, cmd);
        if(strstr(reply->str, "invalid"))
        {
            printf("connect  failed,  error %s", reply->str);
            freeReplyObject(reply);
            return 1;
        }

     char* a = "*3\r\n$3\r\nset\r\n$4\r\nname\r\n$2\r\nab\r\n";

    //char* cmd = "SET b_0 '{"iddddddddd":48,"name":"\u9009\u8bfe\u4e2d\u5fc3","type":1,"level":1,"parent_id":0,"sort":0,"device":"","grade_type":1,"grade_id":"0","province_id":"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,100","is_login":2,"jump_type":0,"target":1,"url":"https:\/\/www.xueersi.com\/","type_code":"","status":2,"create_id":10417,"create_name":"zhangxianglong","create_time":"2019-01-14 18:40:49","modify_id":10692,"modify_name":"guoweilun","modify_time":"2019-08-20 13:53:50"}'";

    reply = redisCommandResp(c, a);

    printf("s write to tw %s\n",  reply->str);
    freeReplyObject(reply);
    
    return 0;
}