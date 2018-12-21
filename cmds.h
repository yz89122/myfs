#ifndef __H_COMMANDS__
#define __H_COMMANDS__

#include "myfs.h"

struct cmd_args
{
    char* arg;
    struct cmd_args* next;
};

struct cwd
{
    struct my_partition* partition;
    struct cwd_node* next;
};

struct cwd_node
{
    char* dir_name;
    uint32_t inode;
    struct cwd_node* next;
};

void cwd_append(
    struct cwd* cwd,
    char* dir_name, uint32_t inode);
void cwd_free(struct cwd* cwd);

struct cmd_args* get_args_from_stdin();
void free_args(struct cmd_args* args);

void print_dir(struct cwd* cwd);
int my_sh(struct my_partition* partition);

void cmd_cd(
    struct cwd* cwd,
    struct cmd_args* args);
void cmd_ls(
    struct cwd* cwd,
    struct cmd_args* args);
void cmd_rm(
    struct cwd* cwd,
    struct cmd_args* args);
void cmd_mkdir(
    struct cwd* cwd,
    struct cmd_args* args);
void cmd_put(
    struct cwd* cwd,
    struct cmd_args* args);
void cmd_get(
    struct cwd* cwd,
    struct cmd_args* args);
void cmd_cat(
    struct cwd* cwd,
    struct cmd_args* args);
void cmd_help(
    struct cwd* cwd,
    struct cmd_args* args);

#endif
