#ifndef __H_MY_UTILS__
#define __H_MY_UTILS__

#include <stdint.h>

#ifdef _WIN32
    #define C_RED
    #define C_GRN
    #define C_YEL
    #define C_BLU
    #define C_MAG
    #define C_CYN
    #define C_WHT
    #define C_RST
#else
    #define C_RED "\x1B[31m"
    #define C_GRN "\x1B[32m"
    #define C_YEL "\x1B[33m"
    #define C_BLU "\x1B[34m"
    #define C_MAG "\x1B[35m"
    #define C_CYN "\x1B[36m"
    #define C_WHT "\x1B[37m"
    #define C_RST "\x1B[0m"
#endif

char* newstr(char* old, uint32_t newsize);
char* strappend(
    char* str, uint32_t* len,
    uint32_t* size, char ch);

#endif
