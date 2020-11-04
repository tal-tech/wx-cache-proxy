#include <iostream>
#include <cstring>
#include <sstream>

 

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "spdlog.h"

#define LOG_PATH "./log/"    //存在log的子目录
 
#define USE_LOG_FILE 1
 
 /*
 https://blog.csdn.net/DeliaPu/article/details/81606630?utm_source=blogxgwz1
https://blog.csdn.net/struct_slllp_main/article/details/77896260

https://www.cnblogs.com/kmist/p/10088585.html

g++ -std=c++11 async.cpp -o async -lpthread -I/home/www/spdlog.bak/include


生成静态库
g++ -std=c++11 -c  spdlog.cpp
ar -cr libspdlog.a spdlog.o


https://segmentfault.com/a/1190000012221570

 */

void async_example();

std::shared_ptr<spdlog::logger> async_file;

void test (){
    printf("thsi is tss\n");
}
void init_filehand(const char* log)
{  
    if (NULL == log)
    {
        log = "/tmp/logs/async_log.txt";
    }

    async_file = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", log);
    async_file->set_pattern("%v");
    async_file->flush_on(spdlog::level::err);
    spdlog::flush_every(std::chrono::seconds(3));
    
}

void wirte_log(const char* msg)
{
    //printf("receive %s\n", msg);
    async_file->info("{}", msg);
    
}
//void async_example()
//{
//    // default thread pool settings can be modified *before* creating the async logger:
//    // spdlog::init_thread_pool(8192, 1); // queue with 8k items and 1 backing thread.
//    auto async_file = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "logs/async_log.txt");
//    int i;
//    for(i=0;i<10;i++){
//        async_file->info("Async message {}", "abc");
//    }
//    // alternatively:
//    // auto async_file = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger", "logs/async_log.txt");   
//}


//int main()
//{
////	async_example();
//    init_filehand();
//    wirte_log();
//	
//	return 0;
//	
//}
