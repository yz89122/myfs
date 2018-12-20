#ifndef __H_MY_UTILS__
#define __H_MY_UTILS__

#include <stdint.h>

char* newstr(char* old, uint32_t newsize);
char* strappend(
    char* str, uint32_t* len,
    uint32_t* size, char ch);

#endif
