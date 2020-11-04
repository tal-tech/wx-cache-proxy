#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
gcc -o test_sprintf  test_sprintf.c

./test_sprintf

*/
int main()
{
  char buf[4] = {0};
    //  char* buf = (char*)calloc(10, sizeof(char));
    char a[3] = "abc";
   int b =  snprintf(buf, sizeof(a)+2  , "%se", a);
    //memcpy(buf, a, sizeof(a));

   // buf[0] = 'e';
    printf("%d %s\n", b, buf);

    return 0;
}
