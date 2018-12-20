#include <stdlib.h>
#include <string.h>

#include "utils.h"

char* newstr(char* old, uint32_t size)
{
    char* new;
    if (size != 0)
    {
        new = (char*) malloc(sizeof(char) * size);
        if (old != NULL) strcpy(new, old);
        else new[0] = '\0';
    }
    else new = NULL;
    free(old);
    return new;
}

char* strappend(char* str, uint32_t* len, uint32_t* size, char ch)
{
    if (*len + 1 >= *size)
        str = newstr(str, *size += 8);
    str[(*len)++] = ch;
    str[*len] = '\0';
    return str;
}
