#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "myfs.h"
#include "cmds.h"

char* cmds[] = {
    "cd",
    "ls",
    "put",
    "get",
    "cat",
    "help",
};

void (*cmd_ptrs[])(struct cwd*, struct cmd_args*) = {
    cmd_cd,
    cmd_ls,
    cmd_put,
    cmd_get,
    cmd_cat,
    cmd_help,
};

void cwd_append(struct cwd* cwd, char* dir_name, uint32_t inode)
{
    struct cwd_node* new = (struct cwd_node*) malloc(sizeof(struct cwd_node));
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

void print_dir(struct cwd* cwd)
{
    uint32_t len = 0, size = 0;
    char* p;
    char* buffer = NULL;
    struct cwd_node* node = cwd->next;

    buffer = strappend(buffer, &len, &size, '/');
    while (node)
    {
        for (p = node->dir_name; *p; ++p)
            buffer = strappend(buffer, &len, &size, *p);
        buffer = strappend(buffer, &len, &size, '/');
        node = node->next;
    }
    printf("%s", buffer);
    free(buffer);
}

int my_sh(struct my_partition* partition)
{
    const int num_of_cmds = sizeof(cmds) / sizeof(char**);
    bool cont = true;
    struct cmd_args* args;
    struct cwd* cwd = (struct cwd*) malloc(sizeof(struct cwd));
    cwd->partition = partition;
    cwd->next = NULL;

    cmd_help(NULL, NULL);

    while (cont)
    {
        print_dir(cwd);
        printf(" $ ");
        args = get_args_from_stdin();

        if (args == NULL) break;

        if (strlen(args->arg) > 0)
        {
            bool found = false;
            for (int i = 0; i < num_of_cmds; ++i)
                if (strcmp(args->arg, cmds[i]) == 0)
                {
                    found = true;
                    cmd_ptrs[i](cwd, args);
                }
            if (!found)
                printf("command '%s' not found\ntry 'help'?\n", args->arg);
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
{
    bool long_list = false;
    bool ls_cwd = true;
    struct cmd_args* a = args->next;
    uint32_t inode, len;
    struct cwd_node* node = cwd->next;
    struct my_directory_file_list *list, *tmp;

    while (a)
    {
        len = strlen(a->arg);
        if (len > 2 && a->arg[0] == '-' && a->arg[1] == '-')
        {
            if (strcmp(a->arg + 2, "help") == 0)
            {
                puts(
                    "yo""\n"
                    "do ya need help?""\n"
                    "\t""-l""\t""use a long listing format""\n"
                    "\n"
                    "'ls' v1 by Yz :D"
                );
                return;
            }
        }
        else if (len > 1 && a->arg[0] == '-')
            for (char* p = a->arg + 1; *p; ++p)
                switch (*p)
                {
                    case 'l': long_list = true; break;
                    default: printf("ls: unknown option %c\n", *p); return;
                }
        else ls_cwd = false;
        a = a->next;
    }

    if (ls_cwd)
    {
        if (node == NULL) inode = cwd->partition->root;
        else
        {
            while (node->next) node = node->next;
            inode = node->inode;
        }
        tmp = list = my_list_directory(cwd->partition, inode);

        while (tmp)
        {
            if (long_list) {}
            else printf("%s ", tmp->filename);
            tmp = tmp->next;
        }

        my_free_directory_file_list(cwd->partition, list);
    }
}

void cmd_put(
    struct cwd* cwd,
    struct cmd_args* args)
{}

void cmd_get(
    struct cwd* cwd,
    struct cmd_args* args)
{}

void cmd_cat(
    struct cwd* cwd,
    struct cmd_args* args)
{}

void cmd_help(
    struct cwd* cwd,
    struct cmd_args* args)
{
    puts(
        "hum...""\n"
        "it seems like ya need a help""\n"
        "should I call 911 for ya?""\n"
    );
    puts(
        "btw, you can try the following commands""\n"
        "'ls' list directory""\n"
        "'cd' change directory""\n"
        "'put' put file into this shit""\n"
        "'get' get file from this shit""\n"
        "'cat' meow?""\n"
        "'help' call 911""\n"
    );
}
