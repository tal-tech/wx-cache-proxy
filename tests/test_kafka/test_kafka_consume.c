#include <stdio.h>
#include <stdlib.h>

#include "mq.h"
/*
no-used
gcc -g -O2 -fPIC -Wall -Wsign-compare -Wfloat-equal -Wpointer-arith -Wcast-align -I../../src/mq -I/home/www/librdkafka-1.2.0/src   -o test_kafka_consume test_kafka_consume.c  ../../src/mq/mq.c  ../../src/mq/mq_conf.c ../../src/mq/cJSON.c  ../../src/mq/libhiredis.a ../../src/mq/libzlog.a  -llz4  -lm -lssl -lcrypto  -lz -ldl -lpthread -lrt -lrdkafka


编译
gcc -g -O2 -fPIC -Wall -Wsign-compare -Wfloat-equal -Wpointer-arith -Wcast-align -I../../src/mq -I../../src/kafka -I../../src/zlog  -o test_kafka_consume test_kafka_consume.c  ../../src/mq/mq.c  ../../src/mq/mq_conf.c ../../src/mq/cJSON.c  -L../../src/mq -lhiredis -L../../src/zlog -lzlog -lrdkafka -llz4  -lm -lssl -lcrypto  -lz -ldl -lpthread -lrt 


gcc -g -O2 -fPIC -Wall -Wsign-compare -Wfloat-equal -Wpointer-arith -Wcast-align -I../../src/mq -I../../src/mq/include -I../../src/mq/include/kafka -I../../src/mq/include/hiredis   -o test_kafka_consume test_kafka_consume.c  ../../src/mq/mq.c  ../../src/mq/mq_conf.c ../../src/mq/cJSON.c  -L../../src/mq/lib -lhiredis -lzlog -lrdkafka -llz4  -lm -lssl -lcrypto  -lz -ldl -lpthread -lrt 


调用
./test_kafka_consume


https://www.cnblogs.com/GnibChen/p/8604544.html

https://www.cnblogs.com/GnibChen/p/8604585.html

*/

int main() {
    consume("test.conf", 2);
//    if (0 != res)
//    {
//        printf("init mq error\n");
//        return 1;
//    }

    return 0;
}