#ifndef __H_COMMANDS__
#define __H_COMMANDS__

#include "myfs.h"

struct command_args
{
    char* arg;
    struct command_args* next;
};

struct cwd
{
    char* dir;
    struct cwd* next;
};

extern char* commands[];

struct command_args* get_args_from_stdin();
void free_args(struct command_args* args);

int my_sh(struct my_partition* partition);

void command_ls(
    struct my_partition* partition,
    struct command_args* args);

#endif
