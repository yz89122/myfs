#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "myfs.h"
#include "commands.h"

char* commands[] = {
    "ls",
    "cd"
};

struct command_args* get_args_from_stdin()
{
    char ch, quote = '\0';
    struct command_args *head = NULL, *node = NULL;
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
                    head = node = (struct command_args*) malloc(
                        sizeof(struct command_args));
                else if (!in_arg)
                    node = node->next = (struct command_args*) malloc(
                        sizeof(struct command_args));
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
        head = (struct command_args*) malloc(sizeof(struct command_args));
        head->arg = NULL;
        head->next = NULL;
    }
    return head;
}

void free_args(struct command_args* args)
{
    struct command_args* next;
    while (args)
    {
        next = args->next;
        free(args->arg);
        free(args);
        args = next;
    }
}

void print_args(struct command_args* args)
{
    for (int i = 0; args; ++i, (args = args->next))
        printf("%d\t[%s]\n", i, args->arg);
}

int my_sh(struct my_partition* partition)
{
    bool cont = true;
    struct command_args* args;

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
