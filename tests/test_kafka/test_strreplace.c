#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
gcc -o test_strreplace test_strreplace.c

./test_strreplace

*/

char * str_replace(char const * const original, char const * const pattern, char const * const replacement) 
{
    size_t const replen = strlen(replacement);
    size_t const patlen = strlen(pattern);
    size_t const orilen = strlen(original);
 
    size_t patcnt = 0;
    const char * oriptr;
    const char * patloc;
 
  // find how many times the pattern occurs in the original string
    for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
    {
        patcnt++;
    }
 
 
    // allocate memory for the new string
    size_t const retlen = orilen + patcnt * (replen - patlen);
    char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );
 
    if (returned != NULL)
    {
        // copy the original string, 
        // replacing all the instances of the pattern
        char * retptr = returned;
        for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
        {
            size_t const skplen = patloc - oriptr;
            // copy the section until the occurence of the pattern
            strncpy(retptr, oriptr, skplen);
            retptr += skplen;
            // copy the replacement 
            strncpy(retptr, replacement, replen);
            retptr += replen;
        }
        // copy the rest of the string.
        strcpy(retptr, oriptr);
    }

    return returned;
}


int main() {
    char* str = "abc\r\ndef\r\n";
    char *replaced = str_replace(str, "\r\n", " ");
    printf("%s\n", replaced);

    return 0;
}