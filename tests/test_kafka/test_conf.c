#include <stdio.h>
#include <stdlib.h>

#include "mq_conf.h"

/*
gcc -g -O2 -fPIC -Wall -Wsign-compare -Wfloat-equal -Wpointer-arith -Wcast-align   -o test_conf test_conf.c  -I../../src/mq  ../../src/mq/mq_conf.c ../../src/mq/cJSON.c  

./test_conf 
*/

int main() {
    const char* filename = "test.conf";
   
    struct conf_mq_data *conf_mq = NULL;

    conf_mq = parse_mq_conf(filename);
    if (NULL == conf_mq)
    {
        free_mq_conf(conf_mq);
        return 1;
    }

    return 0;
}