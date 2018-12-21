#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "myfs.h"
#include "cmds.h"

#define FILE_BUFFER_SIZE 4096

const char* cmds[] = {
    "cd",
    "ls",
    "put",
    "get",
    "cat",
    "help",
};

const void (*cmd_ptrs[])(struct cwd*, struct cmd_args*) = {
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

static struct cwd_node* get_cwd(struct cwd* cwd)
{
    struct cwd_node* node = cwd->next;
    if (node) while (node->next) node = node->next;
    return node;
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
                    "\n"
                    "options:""\n"
                    "\t""-l""\t""use a long listing format""\n"
                    "\n"
                    "'ls' v1 by Yz :D""\n"
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
        printf("\n");

        my_free_directory_file_list(cwd->partition, list);
    }
}

void cmd_put(
    struct cwd* cwd,
    struct cmd_args* args)
{
    if (args->next == NULL)
    {
        puts("usage: put <file from real world> [new name in myfs]");
        return;
    }
    args = args->next;
    char* filename;
    if (args->next && strlen(args->next->arg) > 0)
    {
        // copy & paste are faster and easier than writing a loop :P
        if (strchr(args->next->arg, '\n'))
        {
            puts("doesn't support character '\\n' yet");
            return;
        }
        // copy & paste are faster and easier than writing a loop :P
        if (strchr(args->next->arg, '/'))
        {
            puts("doesn't support character '/' yet");
            return;
        }
        // copy & paste are faster and easier than writing a loop :P
        if (strchr(args->next->arg, '\\'))
        {
            puts("doesn't support character '\\' yet");
            return;
        }
        // sometimes
        filename = args->next->arg;
    }
    else filename = args->arg;
    FILE* fp = fopen(args->arg, "rb");
    if (fp == NULL)
    {
        printf("failed to read file '%s'\n", args->next->arg);
        return;
    }
    uint32_t inode = my_touch(cwd->partition);
    uint32_t dir;
    if (cwd->next) dir = get_cwd(cwd)->inode;
    else dir = cwd->partition->root;

    if (my_dir_reference_file(cwd->partition, dir, inode, MY_TYPE_FILE, filename))
    {
        struct my_file* mfp = my_file_open(cwd->partition, inode);
        uint8_t* buffer = (uint8_t*) malloc(FILE_BUFFER_SIZE);
        size_t len;
        uint32_t my_len;
        while ((len = fread((void*) buffer, sizeof(*buffer), FILE_BUFFER_SIZE, fp)))
            if (my_file_write(cwd->partition, mfp, buffer, len) == 0) break;
        my_file_close(cwd->partition, mfp);
    }

    fclose(fp);
}

void cmd_get(
    struct cwd* cwd,
    struct cmd_args* args)
{}

void cmd_cat(
    struct cwd* cwd,
    struct cmd_args* args)
{
    const char * const cat = ""
    "             *     ,MMM8&&&.            *""\n"
    "                  MMMM88&&&&&    .""\n"
    "                 MMMM88&&&&&&&""\n"
    "     *           MMM88&&&&&&&&""\n"
    "                 MMM88&&&&&&&&""\n"
    "                 'MMM88&&&&&&'""\n"
    "                   'MMM8&&&'      *""\n"
    "          |\\___/|""\n"
    "         =) ^Y^ (=            .              '""\n"
    "          \\  ^  /""\n"
    "           )=*=(       *""\n"
    "          /     \\""\n"
    "          |     |""\n"
    "         /| | | |\\""\n"
    "         \\| | |_|/\\""\n"
    "  _/\\_/\\_//_// ___/\\_/\\_/\\_/\\_/\\_/\\_/\\_/\\_/\\_""\n"
    "  |  |  |  | \\_) |  |  |  |  |  |  |  |  |  |""\n"
    "  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |""\n"
    "  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |""\n"
    "  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |""\n"
    "  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |";
    if (args->next == NULL)
    {
        puts("usage: cat <filename>");
        puts("usage: cat -n <filename>");
        puts(cat);
        return;
    }
    else if (strcmp((args = args->next)->arg, "-n") == 0)
    {
        if (args->next == NULL)
        {
            puts("usage: cat <filename>");
            puts("usage: cat -n <filename>");
            puts(cat);
            return;
        }
        args = args->next;
        uint32_t dir;
        if (cwd->next) dir = get_cwd(cwd)->inode;
        else dir = cwd->partition->root;
        struct my_directory_file_list* list = my_list_directory(cwd->partition, dir);
        struct my_directory_file_list* tmp = my_get_file(cwd->partition, list, args->arg);
        if (tmp == NULL) printf("file was eaten by this cat\n%s", cat);
        else if (tmp->type == MY_TYPE_DIR) puts(cat);
        else
        {
            struct my_file* fp = my_file_open(cwd->partition, tmp->inode);
            uint32_t len, line = 0;
            uint8_t* buffer = (uint8_t*) malloc(FILE_BUFFER_SIZE);
            while ((len = my_file_read_line(cwd->partition, fp, buffer, FILE_BUFFER_SIZE - 1)))
            {
                // buffer[len] = '\0';
                if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
                else buffer[len] = '\0';
                printf("%05d %s\n", line++, buffer);
            }
            my_file_close(cwd->partition, fp);
        }
        my_free_directory_file_list(cwd->partition, list);
    }
    else
    {
        uint32_t dir;
        if (cwd->next) dir = get_cwd(cwd)->inode;
        else dir = cwd->partition->root;
        struct my_directory_file_list* list = my_list_directory(cwd->partition, dir);
        struct my_directory_file_list* tmp = my_get_file(cwd->partition, list, args->arg);
        if (tmp == NULL) printf("file was eaten by this cat\n%s", cat);
        else if (tmp->type == MY_TYPE_DIR) puts(cat);
        else
        {
            struct my_file* fp = my_file_open(cwd->partition, tmp->inode);
            uint32_t len;
            uint8_t* buffer = (uint8_t*) malloc(FILE_BUFFER_SIZE);
            while ((len = my_file_read(cwd->partition, fp, buffer, FILE_BUFFER_SIZE - 1)))
            {
                buffer[len] = '\0';
                printf("%s", buffer);
            }
            my_file_close(cwd->partition, fp);
        }
        my_free_directory_file_list(cwd->partition, list);
    }
}

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
