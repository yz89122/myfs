#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "myfs.h"
#include "cmds.h"

struct my_partition* get_partition();

int main(int argc, char const *argv[])
{
    struct my_partition* partition = get_partition();

    if (partition == NULL) return 1;

    return my_sh(partition);
}

#define BUFFER_SIZE 512

int32_t read_line(char* dest, uint32_t size)
{
    uint32_t count = 0;
    char ch = getchar();
    if (ch != EOF) ungetc(ch, stdin);
    else return -1;
    while ((count + 1) < size)
        if ((ch = getchar()) != EOF && ch != '\r' && ch != '\n') dest[count++] = ch;
        else break;
    dest[count] = '\0';
    return count;
}

struct my_partition* get_partition()
{
    uint32_t op;
    int32_t len;
    char* line = (char*) malloc(BUFFER_SIZE);
    bool first_time = true;

    puts("\toptions:");
    puts("\t1, load from file");
    puts("\t2. create new partition in memory");
    scanf("%d", &op);
    getchar();
    puts(""); // new line

    while (1)
    {
        char* p;
        long num, unit = 1;
        
        if (first_time) first_time = false;
        else puts("\n\tWTF?\n");
        
        if (op == 1)
        {
            printf("Enter the file name: ");
            len = read_line(line, BUFFER_SIZE);
            FILE* fp = fopen(line, "rb");
            if (fp == NULL)
            {
                printf("failed to open %s\n", line);
                continue;
            }
            struct my_partition* partition = my_load_partition_from_file(fp);
            fclose(fp);
            printf("partition size: %d\n", partition->size);
            free(line);
            return partition;
        }
        else
        {
            puts("create new partition\n");
            puts("input size of partition");
            puts("(at least 5KB)");
            puts("(example '8192', '512KB', '20MB')");
            len = read_line(line, BUFFER_SIZE);
            if (len == -1) return NULL;
            if (len == 0) continue;
            num = strtol(line, &p, 0);
            while (*p == ' ' && (p - line) < len) ++p;
            if (*p != '\0')
            {
                switch (*p)
                {
                    case 'b':
                    case 'B': unit = 1  ; break;
                    case 'k':
                    case 'K': unit = 1 K; break;
                    case 'm':
                    case 'M': unit = 1 M; break;
                    case 'g':
                    case 'G': unit = 1 G; break;
                    default: continue;
                }
            }
            if (num * unit < 5 K) continue;
            free(line);
            printf("partition size = %ld\n", num * unit);
            return my_make_partition(num * unit);
        }
    }
    return NULL;
}
