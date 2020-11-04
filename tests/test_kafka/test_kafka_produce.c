#include <stdio.h>
#include <stdlib.h>

#include "mq.h"
/*
gcc -g -O2 -fPIC -Wall -Wsign-compare -Wfloat-equal -Wpointer-arith -Wcast-align -I../../src/mq -I/home/www/librdkafka-1.2.0/src  -o test_kafka_produce test_kafka_produce.c  ../../src/mq/mq.c  ../../src/mq/conf.c ../../src/mq/cJSON.c /home/www/librdkafka-1.2.0/src/librdkafka.a -llz4  -lm -lssl -lcrypto  -lz -ldl -lpthread -lrt 







gcc -g -O2 -fPIC -Wall -Wsign-compare -Wfloat-equal -Wpointer-arith -Wcast-align -I../../src/mq -I../../src/zlog -I/home/www/librdkafka-1.2.0/src -I/home/www/hiredis   -o test_kafka_produce test_kafka_produce.c  ../../src/mq/mq.c  ../../src/mq/mq_conf.c ../../src/mq/cJSON.c  /home/www/hiredis/libhiredis.a  -llz4  -lm -lssl -lcrypto  -lz -ldl -lpthread -lrt -lrdkafka -L../../src/zlog -lzlog

https://www.cnblogs.com/GnibChen/p/8604544.html



=========================================================

gcc -g -O2 -fPIC -Wall -Wsign-compare -Wfloat-equal -Wpointer-arith -Wcast-align -I../../src/mq -I../../src/mq/include  -I../../src/mq/include/kafka  -I../../src/mq/include/hiredis  -o test_kafka_produce test_kafka_produce.c  ../../src/mq/mq.c  ../../src/mq/mq_conf.c ../../src/mq/cJSON.c  -L../../src/mq/lib -lhiredis -lzlog -llz4  -lm -lssl -lcrypto  -lz -ldl -lpthread -lrt -lrdkafka 

./test_kafka_produce

*/

int main() {
    int res = init_produce("test.conf");
    if (0 != res)
    {
        printf("init mq error\n");
        return 1;
    }

    char buf[512]; 

   // while (fgets(buf, sizeof(buf), stdin))
    fgets(buf, sizeof(buf), stdin);

    {  
        size_t len = strlen(buf);

        /* Remove newline */
        if (buf[len-1] == '\n') 
        {
            buf[--len] = '\0';
        }

        if (len == 0)
        {
            /* Empty line: only serve delivery reports */
            //rd_kafka_poll(rk, 0/*non-blocking */);
           // continue;
        }

        produce(buf);
    }

    return 0;
}