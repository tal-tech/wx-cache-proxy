#include "tlog.h"
#include <stdio.h>
#include <time.h>

/*
gcc -o test_tlog test_tlog.c -I../../src/mq/ ../../src/mq/tlog.c  -lpthread 
*/

int main(int argc, char *argv[])
{
//    tlog_log *log = NULL;

    /* init and output log message */
    tlog_init("/tmp/example.log", 1024 * 1024, 8, 0, 0);

    int i;
    for(i=0; i< 10; i++) {
        tlog(TLOG_INFO, "This is a log message. %d\n", i);
    }

//    sleep(50);
//    /* open another log file, and output message*/
//    log = tlog_open("another.log", 1024 * 1024, 8, 0, TLOG_SEGMENT);
//    tlog_printf(log, "This is a separate log stream\n");
//    /* close log stream */
//    tlog_close(log);

    /* flush pending message, and exit tlog */
    tlog_exit();
    return 0;
}
