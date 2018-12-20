#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "myfs.h"

struct my_partition* get_partition();

int main(int argc, char const *argv[])
{
    struct my_partition* partition = get_partition();

    return my_sh(partition);
}

#define BUFFER_SIZE 512

static int32_t read_line(char* dest, uint32_t size)
{
    uint32_t count = 0;
    char ch;
    while ((count + 1) < size)
        if ((ch = getchar()) != EOF && ch != '\r' && ch != '\n') dest[count++] = ch;
        else break;
    dest[count] = '\0';
    return count;
}

struct my_partition* get_partition()
{
    uint32_t op;
    puts("\toptions:");
    puts("\t1, loads from file");
    puts("\t2. create new partition in memory");
    scanf("%d", &op);
    puts(""); // new line
    if (op == 1)
    {
        puts("not implemented yet");
        return NULL;
    }
    else
    {
        char* line = (char*) malloc(BUFFER_SIZE);
        char *p, *q;
        int32_t len;
        long num, unit = 1;
        bool virgin = true;
        while (1)
        {
            if (virgin) virgin = false;
            else puts("\n\tWTF?\n");
            puts("create new partition\n");
            puts("input size of partition");
            puts("(at least 5KB)");
            puts("(example '8192', '512KB', '20MB')");
            len = read_line(line, BUFFER_SIZE);
            if (len == 0) continue;
            num = strtol(line, &p, 0);
            while (*p == ' ' && (p - line) < len) ++p;
            q = p;
            if (*p != '\0')
            {
                switch (*p)
                {
                    case 'k':
                    case 'K': unit = 1024; break;
                    case 'm':
                    case 'M': unit = 1024 * 1024; break;
                    case 'g':
                    case 'G': unit = 1024 * 1024 * 1024; break;
                }
            }
            if (num * unit < 5 K) continue;
            break;
        }
        free(line);
        printf("partition size = %d\n", num * unit);
        return my_make_partition(num * unit);
    }
}
