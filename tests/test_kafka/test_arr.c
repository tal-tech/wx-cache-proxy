#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char  ab[100] = {0};
    sprintf(ab, "abc");
    printf("%d\n", strlen(ab));

    return 0;
}