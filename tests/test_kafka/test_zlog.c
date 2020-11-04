#include <stdio.h> 
#include <unistd.h>
#include "zlog.h"

/*
gcc -o test_zlog test_zlog.c -I../../src/mq/include -L../../src/mq/lib -lzlog -lpthread 
*/

int main(int argc, char** argv)
{
	int rc;
	zlog_category_t *c;

    pid_t pid = fork();
    if (pid> 0)
    {
        rc = zlog_init("../../src/zlog.conf");
        if (rc) {
            printf("init failed\n");
            return -1;
        }
    } else if (pid ==0)
    {
        rc = zlog_init("../../src/zlog.conf");
        if (rc) {
            printf("init failed2\n");
            return -1;
        }
    }
	

//    rc = zlog_init("../../src/zlog.conf");
//	if (rc) {
//		printf("init failed222\n");
//		return -1;
//	}


	c = zlog_get_category("my_cat");
	if (!c) {
		printf("get cat fail\n");
		zlog_fini();
		return -2;
	}

	zlog_info(c, "hello, zlog");

	zlog_fini();

	return 0;
} 