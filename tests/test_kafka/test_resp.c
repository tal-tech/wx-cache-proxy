#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
https://www.cnblogs.com/piaoyang/p/9271879.html
*/

//void split(char *src,const char *separator,char **dest,int *num) {
//    char *pNext;
//    int count = 0;
//    if (src == NULL || strlen(src) == 0)
//    {
//        return;
//    }
//
//    if (separator == NULL || strlen(separator) == 0)
//    {
//        return; 
//    }
//
//    pNext = strtok(src,separator);
//    while(pNext != NULL) 
//    {
//        if(NULL == strstr(pNext, "*") && NULL == strstr(pNext, "$"))
//        {
//            //*dest++ = pNext;
//           // ++count;
//           strcat(dest, src);
//        }
//        pNext = strtok(NULL,separator);  
//        
//    }  
//
//    *num = count;
//} 


void split(char *src,const char *separator,char *dest) {
    char *pNext;
    int count = 0;
    if (src == NULL || strlen(src) == 0)
    {
        return;
    }

    if (separator == NULL || strlen(separator) == 0)
    {
        return; 
    }

    pNext = strtok(src,separator);
    while(pNext != NULL) 
    {
        if(NULL == strstr(pNext, "*") && NULL == strstr(pNext, "$"))
        {
            //*dest++ = pNext;
           // ++count;
           strcat(dest, pNext);
           strcat(dest, " ");
        }
        pNext = strtok(NULL,separator);  
        
    }  
    
} 



void gen_cmd(char *src, const char *separator, char *dest) {
    char *pNext;
    //int count = 0;
    if (src == NULL || strlen(src) == 0)
    {
        return;
    }

    if (separator == NULL || strlen(separator) == 0)
    {
        return; 
    }

    pNext = strtok(src, separator);
    printf("lllllllll %s\n", pNext);
    while(pNext != NULL) 
    {
        printf("== %s\n", pNext);
        if(NULL == strstr(pNext, "*") && NULL == strstr(pNext, "$"))
        {
            //*dest++ = pNext;
           // ++count;
           strcat(dest, pNext);
           strcat(dest, " ");
        }
        pNext = strtok(NULL,separator);  
        
    }  
} 

int main() {
    char cmd[200];
   // sprintf(cmd, "*2\r\n$4\r\nauth\r\n$6\r\n123456\r\n");
    sprintf(cmd, "*2\r\n$4\r\nset\r\n$4\r\nname\r\n$3\r\nabc\r\n");
   // printf("%s\n", cmd);
     //用来接收返回数据的数组。这里的数组元素只要设置的比分割后的子字符串个数大就好了。
    char *revbuf[8] = {0};

    char final_cmd[200];

// 
//     //分割后子字符串的个数
    int num = 0;
// 
     gen_cmd(cmd,"\r\n",final_cmd);
     printf("final %s\n", final_cmd);
// 
//    int i = 0;
//    for(i = 0;i < num; i ++) {
//        printf("%s\n",revbuf[i]);
//    }
 
//    char* a = "1abc";
//    printf("====== %c\n", a[0]);
     return 0;

}

//int main() {
//    char cmd[200];
//   // sprintf(cmd, "*2\r\n$4\r\nauth\r\n$6\r\n123456\r\n");
//    sprintf(cmd, "*2\r\n$4\r\nset\r\n$4\r\nname\r\n$3\r\nabc\r\n");
//   // printf("%s\n", cmd);
//     //用来接收返回数据的数组。这里的数组元素只要设置的比分割后的子字符串个数大就好了。
//    char *revbuf[8] = {0};
//
//    char final_cmd[200];
//
//// 
////     //分割后子字符串的个数
//    int num = 0;
//// 
//     split(cmd,"\r\n",final_cmd);
//     printf("final %s\n", final_cmd);
//// 
////    int i = 0;
////    for(i = 0;i < num; i ++) {
////        printf("%s\n",revbuf[i]);
////    }
// 
//    char* a = "1abc";
//    printf("====== %c\n", a[0]);
//     return 0;

//}