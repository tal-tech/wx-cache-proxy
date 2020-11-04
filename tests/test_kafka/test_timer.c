#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
gcc -o test_timer test_timer.c
*/

static inline unsigned long zend_inline_hash_func(char *arKey, uint nKeyLength)
{
    register ulong hash = 5381;
 
    /* variant with the hash unrolled eight times */
    for (; nKeyLength >= 8; nKeyLength -= 8) {
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
    }
    switch (nKeyLength) {
        case 7: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 6: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 5: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 4: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 3: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 2: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 1: hash = ((hash << 5) + hash) + *arKey++; break;
        case 0: break;
    }
    return hash;
}

int main(){
    char* key ="abc";

   unsigned long res;
    res = zend_inline_hash_func(key, strlen(key));
    printf("%ld", res);
}
