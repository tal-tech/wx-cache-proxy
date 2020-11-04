#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif



 void init_filehand(char* log);
 void  wirte_log(char* msg);
extern void test();
#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */