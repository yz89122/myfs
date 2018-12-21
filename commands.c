#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "myfs.h"
#include "commands.h"

char* cmds[] = {
    "cd",
    "ls",
};

void cwd_append(struct cwd* cwd, char* dir_name, uint32_t inode)
{
    struct cwd_node* new = (char*) malloc(sizeof(struct cwd_node));
    new->dir_name = (char*) malloc(sizeof(char) * strlen(dir_name));
    strcpy(cwd->next->dir_name, dir_name);
    new->inode = inode;
    new->next = NULL;
    if (cwd->next == NULL) cwd->next = NULL;
    else
    {
        struct cwd_node* node = cwd->next;
        while (node->next) node = node->next;
        node->next = new;
    }
}

void cwd_free(struct cwd* cwd)
{
    struct cwd_node *node = cwd->next, *next;
    while (node)
    {
        next = node->next;
        free(node->dir_name);
        free(node);
        node = next;
    }
    cwd->next = NULL;
}

struct cmd_args* get_args_from_stdin()
{
    char ch, quote = '\0';
    struct cmd_args *head = NULL, *node = NULL;
    bool in_quote = false, in_arg = false;
    uint32_t size = 0, len = 0;
    bool cont = true;
    while (cont)
    {
        ch = getchar();
        switch (ch)
        {
            case EOF:
                cont = false;
                break;
            case '\r':
            case '\n':
                if (!in_quote) cont = false;
                else goto default_;
                break;
            case '\'':
                if (!in_quote)
                {
                    quote = '\'';
                    in_quote = true;
                }
                else if (quote == '\'')
                    in_quote = false;
                else goto default_;
                ch = '\0';
                if (!in_arg) goto default_;
                break;
            case '"':
                if (!in_quote)
                {
                    quote = '"';
                    in_quote = true;
                }
                else if (quote == '"')
                    in_quote = false;
                else goto default_;
                ch = '\0';
                if (!in_arg) goto default_;
                break;
            case '\\':
                if (quote != '\'' || !in_quote)
                {
                    ch = getchar();
                    if (ch == '\r' || ch == '\n') ch = '\0';
                }
                goto default_;
            case ' ':
                if (!in_quote)
                {
                    in_arg = false;
                    break;
                }
            default:
            default_:
                if (head == NULL)
                    head = node = (struct cmd_args*) malloc(
                        sizeof(struct cmd_args));
                else if (!in_arg)
                    node = node->next = (struct cmd_args*) malloc(
                        sizeof(struct cmd_args));
                if (!in_arg)
                {
                    len = size = 0;
                    node->arg = NULL;
                    node->next = NULL;
                    in_arg = true;
                }
                if (ch != '\0') node->arg = strappend(node->arg, &len, &size, ch);
        }
    }
    if (head == NULL && ch != EOF)
    {
        head = (struct cmd_args*) malloc(sizeof(struct cmd_args));
        head->arg = NULL;
        head->next = NULL;
    }
    return head;
}

void free_args(struct cmd_args* args)
{
    struct cmd_args* next;
    while (args)
    {
        next = args->next;
        free(args->arg);
        free(args);
        args = next;
    }
}

void print_args(struct cmd_args* args)
{
    // debug
    for (int i = 0; args; ++i, (args = args->next))
        printf("%d\t[%s]\n", i, args->arg);
}

int my_sh(struct my_partition* partition)
{
    bool cont = true;
    struct cmd_args* args;
    struct cwd* cwd = (struct cwd*) malloc(sizeof(struct cwd));
    cwd->partition = partition;
    cwd->next = NULL;

    while (cont)
    {
        args = get_args_from_stdin();
        print_args(args);

        if (args == NULL) break;

        if (strlen(args->arg) > 0)
        {
        }

        free_args(args);
    }

    return 0;
}

void cmd_cd(
    struct cwd* cwd,
    struct cmd_args* args)
{}

void cmd_ls(
    struct cwd* cwd,
    struct cmd_args* args)
{}
