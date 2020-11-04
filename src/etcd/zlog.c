#include <stdio.h> 

#include "zlog.h"

/*
 cc -c -o test_hello.o test_hello.c -I/usr/local/include
$ cc -o test_hello test_hello.o -L/usr/local/lib -lzlog -lpthread

gcc -o zlog zlog.c  -lzlog -lpthread
*/
int main(int argc, char** argv)
{
	int rc;
	zlog_category_t *c;

	rc = zlog_init("./zlog.conf");
	if (rc) {
		printf("init failed\n");
		return -1;
	}

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