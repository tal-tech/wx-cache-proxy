#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
gcc -o test_cmd test_cmd.c 
*/

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

    int cnt = 0;
    while(pNext != NULL) 
    {
        cnt ++;
        if(NULL == strstr(pNext, "*") && NULL == strstr(pNext, "$"))
        {
            //*dest++ = pNext;
           // ++count;
           
            
           if (cnt >=7 && cnt %2 ==1)
           {
                if(NULL != strstr(pNext, "{"))
                {
                    strcat(dest, "'");
                    strcat(dest, pNext);
                    strcat(dest, "'");
                    strcat(dest, " ");
                }
           }
           else
           {
                strcat(dest, pNext);
                strcat(dest, " ");
           }
        }
        pNext = strtok(NULL, separator);  
        
    }  
} 

int main()
{  
    char cmd[200]={0};

    char* buf=(char*)calloc(200, sizeof(char));

    char* a = "*3\r\n$3\r\nset\r\n$4\r\nname\r\n$2\r\nab\r\n";
    // char* a = "*3\r\n$3\r\nset\r\n$4\r\nname\r\n$2\r\n{\"name\":\"zm\"}\r\n";
    memcpy(buf, a, strlen(a));
    gen_cmd(buf, "\r\n", cmd);
    printf("%s\n", cmd);

    return 0;
}