#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "myfs.h"

#define MY_INODE_SIZE 128
#define MY_BLOCK_SIZE 1 K

#define BUFFER_SIZE 512

struct my_partition* my_make_partition(uint32_t size)
{
    if (size < 5 * MY_BLOCK_SIZE) return NULL;
    uint8_t* memory = (uint8_t*) malloc(size);
    struct my_partition* partition = (struct my_partition*) memory;
    partition->size = size;

    partition->inode_size = MY_INODE_SIZE;
    partition->block_size = MY_BLOCK_SIZE;

    uint32_t num_of_blocks = partition->size / partition->block_size;
    uint32_t size_of_bitmap = num_of_blocks / 8;
    if (num_of_blocks % 8) ++size_of_bitmap;
    uint32_t blocks_of_bitmap = size_of_bitmap / partition->block_size;
    if (size_of_bitmap % partition->block_size) ++blocks_of_bitmap;
    uint32_t tmp;

    partition->inode_bitmap = 1;
    partition->block_bitmap = partition->inode_bitmap + blocks_of_bitmap;

    partition->inodes = partition->block_bitmap + blocks_of_bitmap;

    partition->inode_count = (num_of_blocks - partition->inodes) *
        partition->inode_size / (partition->inode_size + partition->block_size);
    partition->block_count = num_of_blocks;
    tmp = partition->inode_count * partition->inode_size;
    partition->blocks = partition->inodes + tmp / partition->block_size;
    if (tmp % partition->block_size) ++partition->blocks;

    partition->inode_used = 0;
    partition->block_used = 0;

    memset(my_get_block_pointer(partition,
        partition->inode_bitmap), 0, partition->block_size);
    memset(my_get_block_pointer(partition,
        partition->block_bitmap), 0, partition->block_size);

    for (uint32_t i = 0; i < partition->blocks; ++i)
        my_mark_block_used(partition, i);

    my_mark_inode_used(partition, 0);
    partition->root = 0;

    struct my_inode* root = my_get_inode_pointer(partition, 0);
    root->reference_count = 1;
    root->mtime = time(NULL);
    root->size = 0;

    return partition;
}

struct my_partition* my_load_partition_from_file(FILE* file)
{
    return NULL;
}

void my_dump_partition_to_file(FILE* file)
{}

void my_free_partition(struct my_partition* partition)
{
    free(partition);
}

uint8_t* my_get_block_pointer(
    struct my_partition* partition, uint32_t block)
{
    return (uint8_t*) partition + block * partition->block_size;
}

struct my_inode* my_get_inode_pointer(
    struct my_partition* partition, uint32_t inode)
{
    return (struct my_inode*) (
        my_get_block_pointer(partition, partition->inodes) +
        partition->inode_size * inode);
}

static int32_t first_zero(uint64_t x)
{
    int32_t i;
    
    #ifndef MY_FS_BIG_ENDIAN
        uint8_t* p = ((uint8_t*) &x) - 1;
    #endif

    for (i = 63; i >= 0; --i)
    {
        #ifndef MY_FS_BIG_ENDIAN
            if ((i & 7) == 7) ++p;
            if (((*p >> (i & 7)) & 1) == 0) break;
        #else
            if ((x >> i & 1) == 0) break;
        #endif
    }

    return 63 - i;
}

uint32_t my_get_free_inode(struct my_partition* partition)
{
    uint32_t loop = partition->inode_count / 64;
    uint64_t *p = (uint64_t*) my_get_block_pointer(
        partition, partition->inode_bitmap);
    for (uint32_t i = 0; i < loop; ++i, ++p)
        if (*p != 0xffffffffffffffff)
            return first_zero(*p) + i * 64;
    uint32_t base = loop * 64;
    uint32_t z = first_zero(*p);
    if (base + z >= partition->inode_count) return -1;
    return base + z;
}

void my_mark_inode_used(struct my_partition* partition, uint32_t inode)
{
    uint8_t* bitmap = my_get_block_pointer(
        partition, partition->inode_bitmap) + (inode / 8);
    uint8_t bit = 0x80 >> (inode & 7);
    if (!(*bitmap & bit))
    {
        ++partition->inode_used;
        *bitmap |= bit;
    }
}

void my_mark_inode_unused(struct my_partition* partition, uint32_t inode)
{
    uint8_t* bitmap = my_get_block_pointer(
        partition, partition->inode_bitmap) + (inode / 8);
    uint8_t bit = 0x80 >> (inode & 7);
    if (*bitmap & bit)
    {
        --partition->inode_used;
        *bitmap &= ~bit;
    }
}

uint32_t my_get_free_block(struct my_partition* partition)
{
    uint32_t loop = partition->block_count / 64;
    uint64_t *p = (uint64_t*) my_get_block_pointer(
        partition, partition->block_bitmap);
    for (uint32_t i = 0; i < loop; ++i, ++p)
        if (*p != 0xffffffffffffffff)
            return first_zero(*p) + i * 64;
    uint32_t base = loop * 64;
    uint32_t z = first_zero(*p);
    if (base + z >= partition->block_count) return 0;
    return base + z;
}

void my_mark_block_used(struct my_partition* partition, uint32_t block)
{
    uint8_t* bitmap = my_get_block_pointer(
        partition, partition->block_bitmap) + (block / 8);
    uint8_t bit = 0x80 >> (block & 7);
    if (!(*bitmap & bit))
    {
        ++partition->block_used;
        *bitmap |= bit;
    }
}

void my_mark_block_unused(struct my_partition* partition, uint32_t block)
{
    uint8_t* bitmap = my_get_block_pointer(
        partition, partition->block_bitmap) + (block / 8);
    uint8_t bit = 0x80 >> (block & 7);
    if (*bitmap & bit)
    {
        --partition->block_used;
        *bitmap &= ~bit;
    }
}

struct my_dir_list* my_ls_dir(
    struct my_partition* partition, uint32_t dir)
{
    struct my_file* directory = my_file_open(partition, dir);
    struct my_dir_list *head = NULL, *node;
    uint32_t len = 1;
    uint32_t inode, type;
    int r;
    char* buffer = (char*) malloc(BUFFER_SIZE);
    char *p, *q;
    while (len != 0)
    {
        len = my_file_read_line(partition, directory, (uint8_t*) buffer, BUFFER_SIZE);
        if (len == 0 || len < 6) continue;
        q = p = buffer;

        // inode
        while (*p != '|' && *p != '\n' && (buffer + len - p) > 0) ++p;
        if (p == q) continue;
        *p = '\0';
        r = sscanf(q, "%x", &inode);
        if (r < 1) continue;

        // type
        q = ++p;
        while (*p != '|' && *p != '\n' && (buffer + len - p) > 0) ++p;
        if (p == q) continue;
        *p = '\0';
        r = sscanf(q, "%x", &type);
        if (r < 1) continue;

        // filename
        q = ++p;
        while (*p != '\n' && (buffer + len - p) > 0) ++p;
        if (p == q) continue;
        *p = '\0';

        // linked list
        node = (struct my_dir_list*) malloc(sizeof(struct my_dir_list));
        node->inode = inode;
        node->type = type;
        strcpy(node->filename, q);
        node->next = head;
        head = node;
    }
    my_file_close(partition, directory);
    free(buffer);
    return head;
}

void my_free_dir_list(
    struct my_partition* partition, struct my_dir_list* list)
{
    struct my_dir_list* next;
    while (list)
    {
        next = list->next;
        free(list);
        list = next;
    }
}

uint32_t my_touch(struct my_partition* partition)
{
    uint32_t inode = my_get_free_inode(partition);
    if (inode == -1) return -1;
    my_mark_inode_used(partition, inode);
    return inode;
}

struct my_dir_list* my_get_file(
    struct my_partition* partition,
    struct my_dir_list* file_list, const char* filename)
{
    if (filename == NULL) return NULL;
    while (file_list)
        if (strcmp(file_list->filename, filename) == 0) break;
        else file_list = file_list->next;
    return file_list;
}

bool my_dir_reference_file(
    struct my_partition* partition,
    uint32_t dir, uint32_t file, uint8_t type, const char* filename)
{
    struct my_dir_list* list = my_ls_dir(partition, dir);
    if (strlen(filename) == 0 || my_get_file(partition, list, filename) != NULL)
    {
        // if filename already exist or filename with length of 0
        my_free_dir_list(partition, list);
        return false;
    }
    my_free_dir_list(partition, list);

    char* buffer = (char*) malloc(BUFFER_SIZE);

    uint32_t line_len = snprintf(buffer, BUFFER_SIZE, "%x|%x|%s\n", file, type, filename);
    if (line_len == 511) buffer[line_len++] = '\n';
    struct my_file* directory = my_file_open_end(partition, dir);
    my_file_write(partition, directory, (uint8_t*) buffer, line_len);
    my_file_close(partition, directory);
    ++(my_get_inode_pointer(partition, file)->reference_count);

    free(buffer);

    return true;
}

void my_dir_unreference_file(
    struct my_partition* partition,
    uint32_t dir, const char* filename)
{
    struct my_dir_list* list = my_ls_dir(partition, dir);
    struct my_dir_list* file = my_get_file(partition, list, filename);
    if (file == NULL)
    {
        my_free_dir_list(partition, list);
        return;
    }
    struct my_dir_list* iter = list;
    my_erase_file(partition, dir);
    struct my_file* fp = my_file_open(partition, dir);
    uint8_t* buffer = (uint8_t*) malloc(BUFFER_SIZE);
    while (iter)
    {
        if (iter != file)
        {
            uint32_t line_len = snprintf(
                (char*) buffer, 512, "%x|%x|%s\n",
                iter->inode,
                iter->type,
                iter->filename);
            if (line_len == 511) buffer[line_len++ - 1] = '\n';
            my_file_write(partition, fp, buffer, line_len);
        }
        iter = iter->next;
    }
    my_file_close(partition, fp);
    free(buffer);
    struct my_inode* inode = my_get_inode_pointer(partition, file->inode);
    --(inode->reference_count);
    if (inode->reference_count == 0)
        my_delete_file(partition, file->inode);
    my_free_dir_list(partition, list);
}

void my_delete_file(struct my_partition* partition, uint32_t inode)
{
    my_erase_file(partition, inode);
    my_mark_inode_unused(partition, inode);
}

void my_erase_file(struct my_partition* partition, uint32_t inode)
{
    struct my_inode* s_inode = my_get_inode_pointer(partition, inode);
    if (s_inode->size == 0) return;
    const uint32_t ind = partition->block_size / sizeof(uint32_t);
    const uint32_t d_ind = ind * ind;
    uint32_t blocks = s_inode->size / partition->block_size, tmp;
    if (s_inode->size % partition->block_size) ++blocks;
    s_inode->size = 0;

    puts("erase");

    // direct block
    tmp = (blocks > NUM_OF_DIRECT_BLOCKS) ? NUM_OF_DIRECT_BLOCKS : blocks;
    for (int i = 0; i < tmp; ++i)
        my_mark_block_unused(partition, s_inode->direct_block[i]);
    blocks -= tmp;

    // indirect block
    if (blocks == 0) return;
    tmp = (blocks < ind) ? blocks : ind;
    uint32_t in = tmp;

    for (uint32_t i = 0; i < in; ++i)
        my_mark_block_unused(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                s_inode->indirect_block
            ))[i]
        );
    my_mark_block_unused(partition, s_inode->indirect_block);
    blocks -= tmp;

    // double indirect block
    if (blocks == 0) return;
    tmp = (blocks < d_ind) ? blocks : d_ind;
    uint32_t d = tmp / ind;
    in = tmp % ind;

    for (uint32_t i = 0; i < d; ++i)
    {
        for (uint32_t j = 0; j < ind; ++j)
            my_mark_block_unused(
                partition,
                ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        s_inode->double_indirect_block
                    ))[i]
                ))[j]
            );
        my_mark_block_unused(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                s_inode->double_indirect_block
            ))[i]
        );
    }
    for (uint32_t j = 0; j < in; ++j)
        my_mark_block_unused(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                ((uint32_t*) my_get_block_pointer(
                    partition,
                    s_inode->double_indirect_block
                ))[d]
            ))[j]
        );
    my_mark_block_unused(
        partition,
        ((uint32_t*) my_get_block_pointer(
            partition,
            s_inode->double_indirect_block
        ))[d]
    );
    my_mark_block_unused(partition, s_inode->double_indirect_block);
    blocks -= tmp;

    // trible indirect block
    if (blocks == 0) return;
    uint32_t t = blocks / d_ind;
    d = (blocks % d_ind) / ind;
    in = blocks % ind;

    for (uint32_t i = 0; i < t; ++i)
    {
        for (uint32_t j = 0; j < ind; ++j)
        {
            for (uint32_t k = 0; k < ind; ++k)
                my_mark_block_unused(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        ((uint32_t*) my_get_block_pointer(
                            partition,
                            ((uint32_t*) my_get_block_pointer(
                                partition,
                                s_inode->trible_indirect_block
                            ))[i]
                        ))[j]
                    ))[k]
                );
            my_mark_block_unused(
                partition,
                ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        s_inode->trible_indirect_block
                    ))[i]
                ))[j]
            );
        }
        my_mark_block_unused(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                s_inode->trible_indirect_block
            ))[i]
        );
    }
    for (uint32_t j = 0; j < d; ++j)
    {
        for (uint32_t k = 0; k < ind; ++k)
            my_mark_block_unused(
                partition,
                ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        ((uint32_t*) my_get_block_pointer(
                            partition,
                            s_inode->trible_indirect_block
                        ))[t]
                    ))[j]
                ))[k]
            );
        my_mark_block_unused(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                ((uint32_t*) my_get_block_pointer(
                    partition,
                    s_inode->trible_indirect_block
                ))[t]
            ))[j]
        );
    }
    for (uint32_t k = 0; k < in; ++k)
        my_mark_block_unused(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        s_inode->trible_indirect_block
                    ))[t]
                ))[d]
            ))[k]
        );
    my_mark_block_unused(
        partition,
        ((uint32_t*) my_get_block_pointer(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                s_inode->trible_indirect_block
            ))[t]
        ))[d]
    );
    my_mark_block_unused(
        partition,
        ((uint32_t*) my_get_block_pointer(
            partition,
            s_inode->trible_indirect_block
        ))[t]
    );
    my_mark_block_unused(
        partition,
        s_inode->trible_indirect_block
    );
}

struct my_file* my_file_open(
    struct my_partition* partition, uint32_t file_inode)
{
    struct my_file* file = (struct my_file*) malloc(sizeof(struct my_file));
    file->inode = my_get_inode_pointer(partition, file_inode);
    file->position = 0;
    file->block_position = partition->block_size;
    return file;
}

struct my_file* my_file_open_end(
    struct my_partition* partition, uint32_t file_inode)
{
    uint32_t tmp;
    struct my_file* file = (struct my_file*) malloc(sizeof(struct my_file));
    const uint32_t ind = partition->block_size / sizeof(uint32_t);
    const uint32_t d_ind = ind * ind;

    file->inode = my_get_inode_pointer(partition, file_inode);
    file->position = file->inode->size;
    file->block_position = file->position % partition->block_size;

    if (file->block_position == 0) // size 0
        file->block_position = partition->block_size; // let write function handle it
    else if ((tmp = file->position / partition->block_size) <
            NUM_OF_DIRECT_BLOCKS) // direct
        file->block = file->inode->direct_block[tmp];
    else if ((tmp -= NUM_OF_DIRECT_BLOCKS) < ind) // indirect
        file->block = ((uint32_t*) my_get_block_pointer(partition,
            file->inode->indirect_block))[tmp];
    else if ((tmp -= ind) < d_ind)
        file->block = ((uint32_t*) my_get_block_pointer(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                file->inode->double_indirect_block
            ))[tmp / ind]
        ))[tmp % ind];
    else if ((tmp -= d_ind) < d_ind * ind) // trible indirect
        file->block = ((uint32_t*) my_get_block_pointer(
            partition,
            ((uint32_t*) my_get_block_pointer(
                partition,
                ((uint32_t*) my_get_block_pointer(
                    partition,
                    file->inode->trible_indirect_block
                ))[tmp / d_ind]
            ))[(tmp % d_ind) / ind]
        ))[tmp % ind];

    return file;
}

void my_file_close(struct my_partition* partition, struct my_file* file)
{
    free(file);
}

uint32_t my_file_read(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size)
{
    uint8_t* current_block = my_get_block_pointer(partition, file->block);
    uint32_t tmp, buffer_position = 0;
    if (file->position >= file->inode->size) return 0;
    const uint32_t ind = partition->block_size / sizeof(uint32_t);
    const uint32_t d_ind = ind * ind;
    
    while (buffer_position < buffer_size && file->position < file->inode->size)
    {
        if (file->block_position >= partition->block_size) // without / and %
        // if reached block ending then go to next block
        {
            if ((tmp = file->position / partition->block_size) <
                    NUM_OF_DIRECT_BLOCKS) // direct
                file->block = file->inode->direct_block[tmp];
            else if ((tmp -= NUM_OF_DIRECT_BLOCKS) < ind) // indirect
                file->block = ((uint32_t*) my_get_block_pointer(partition,
                    file->inode->indirect_block))[tmp];
            else if ((tmp -= ind) < d_ind) // double indirect
                file->block = ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        file->inode->double_indirect_block
                    ))[tmp / ind]
                ))[tmp % ind];
            else if ((tmp -= d_ind) < d_ind * ind) // trible indirect
                file->block = ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        ((uint32_t*) my_get_block_pointer(
                            partition,
                            file->inode->trible_indirect_block
                        ))[tmp / d_ind]
                    ))[(tmp % d_ind) / ind]
                ))[tmp % ind];
            else break; // you're so huge hum?

            current_block = my_get_block_pointer(partition, file->block);
            file->block_position = 0;
        }

        buffer[buffer_position++] = current_block[file->block_position++];
        ++file->position;
    }
    return buffer_position;
}

uint32_t my_file_read_line(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size)
{
    uint8_t* current_block = my_get_block_pointer(partition, file->block);
    uint32_t tmp, buffer_position = 0;
    if (file->position >= file->inode->size) return 0;
    const uint32_t ind = partition->block_size / sizeof(uint32_t);
    const uint32_t d_ind = ind * ind;
    
    while (buffer_position + 1 < buffer_size &&
        file->position < file->inode->size)
    {
        if (file->block_position >= partition->block_size) // without / and %
        // if reached block ending then go to next block
        {
            if ((tmp = file->position / partition->block_size) <
                    NUM_OF_DIRECT_BLOCKS) // direct
                file->block = file->inode->direct_block[tmp];
            else if ((tmp -= NUM_OF_DIRECT_BLOCKS) < ind) // indirect
                file->block = ((uint32_t*) my_get_block_pointer(partition,
                    file->inode->indirect_block))[tmp];
            else if ((tmp -= ind) < d_ind) // double indirect
                file->block = ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        file->inode->double_indirect_block
                    ))[tmp / ind]
                ))[tmp % ind];
            else if ((tmp -= d_ind) < d_ind * ind) // trible indirect
                file->block = ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        ((uint32_t*) my_get_block_pointer(
                            partition,
                            file->inode->trible_indirect_block
                        ))[tmp / d_ind]
                    ))[(tmp % d_ind) / ind]
                ))[tmp % ind];
            else break; // :O too large

            current_block = my_get_block_pointer(partition, file->block);
            file->block_position = 0;
        }

        buffer[buffer_position] = current_block[file->block_position++];
        ++file->position;
        if (buffer[buffer_position++] == '\n') break;
    }
    buffer[buffer_position] = '\0';
    return buffer_position;
}

uint32_t my_file_write(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size)
{
    uint8_t* current_block = my_get_block_pointer(partition, file->block);
    uint32_t tmp, buffer_position = 0;
    const uint32_t ind = partition->block_size / sizeof(uint32_t);
    const uint32_t d_ind = ind * ind;

    while (buffer_position < buffer_size)
    {
        if (file->block_position >= partition->block_size) // without / and %
        // if reached block ending then go to next block
        {
            uint32_t free_block = my_get_free_block(partition);
            if (free_block == 0 ||
                free_block >= partition->block_count) break; // no more blocks
            my_mark_block_used(partition, free_block);

            if ((tmp = file->position / partition->block_size) < NUM_OF_DIRECT_BLOCKS)
            {
                file->block = file->inode->direct_block[tmp] = free_block;
            }
            else if ((tmp -= NUM_OF_DIRECT_BLOCKS) < ind) // indirect
            {
                if (tmp == 0)
                {
                    uint32_t fb_i = my_get_free_block(partition);
                    if (fb_i == 0 || fb_i >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block); // free pervious
                        break;
                    }
                    my_mark_block_used(partition, fb_i);
                    file->inode->indirect_block = fb_i;
                }
                file->block = ((uint32_t*) my_get_block_pointer(partition,
                    file->inode->indirect_block))[tmp] = free_block;
            }
            else if ((tmp -= ind) < d_ind) // double indirect
            {
                uint32_t i = tmp % ind;
                uint32_t d = tmp / ind;
                if (d == 0 && i == 0)
                {
                    uint32_t fb_d = my_get_free_block(partition);
                    if (fb_d == 0 || fb_d >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        break;
                    }
                    my_mark_block_used(partition, fb_d);

                    uint32_t fb_i = my_get_free_block(partition);
                    if (fb_i == 0 || fb_i >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        my_mark_block_unused(partition, fb_d);
                        break;
                    }
                    my_mark_block_used(partition, fb_i);

                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        file->inode->double_indirect_block = fb_d
                    ))[d] = fb_i;
                }
                else if (i == 0)
                {
                    uint32_t fb_i = my_get_free_block(partition);
                    if (fb_i == 0 || fb_i >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        break;
                    }
                    my_mark_block_used(partition, fb_i);

                    ((uint32_t*) my_get_block_pointer(partition,
                        file->inode->double_indirect_block))[d] = fb_i;
                }

                file->block = ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        file->inode->double_indirect_block
                    ))[d]
                ))[i] = free_block;
            }
            else if ((tmp -= d_ind) < d_ind * ind) // trible indirect
            {
                uint32_t i = tmp % ind;
                uint32_t d = (tmp % d_ind) / ind;
                uint32_t t = tmp / d_ind;
                if (t == 0 && d == 0 && i == 0)
                {
                    uint32_t fb_t = my_get_free_block(partition);
                    if (fb_t == 0 || fb_t >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        break;
                    }
                    my_mark_block_used(partition, fb_t);

                    uint32_t fb_d = my_get_free_block(partition);
                    if (fb_d == 0 || fb_d >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        my_mark_block_unused(partition, fb_t);
                        break;
                    }
                    my_mark_block_used(partition, fb_d);

                    uint32_t fb_i = my_get_free_block(partition);
                    if (fb_i == 0 || fb_i >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        my_mark_block_unused(partition, fb_t);
                        my_mark_block_unused(partition, fb_d);
                        break;
                    }
                    my_mark_block_used(partition, fb_i);

                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        ((uint32_t*) my_get_block_pointer(
                            partition,
                            file->inode->trible_indirect_block = fb_t
                        ))[t] = fb_d
                    ))[d] = fb_i;
                }
                else if (d == 0 && i == 0)
                {
                    uint32_t fb_d = my_get_free_block(partition);
                    if (fb_d == 0 || fb_d >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        break;
                    }
                    my_mark_block_used(partition, fb_d);

                    uint32_t fb_i = my_get_free_block(partition);
                    if (fb_i == 0 || fb_i >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        my_mark_block_unused(partition, fb_d);
                        break;
                    }
                    my_mark_block_used(partition, fb_i);

                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        ((uint32_t*) my_get_block_pointer(
                            partition,
                            file->inode->trible_indirect_block
                        ))[t] = fb_d
                    ))[d] = fb_i;
                }
                else if (i == 0)
                {
                    uint32_t fb_i = my_get_free_block(partition);
                    if (fb_i == 0 || fb_i >= partition->block_count)
                    {
                        my_mark_block_unused(partition, free_block);
                        break;
                    }
                    my_mark_block_used(partition, fb_i);

                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        ((uint32_t*) my_get_block_pointer(
                            partition,
                            file->inode->trible_indirect_block
                        ))[t]
                    ))[d] = fb_i;
                }

                file->block = ((uint32_t*) my_get_block_pointer(
                    partition,
                    ((uint32_t*) my_get_block_pointer(
                        partition,
                        ((uint32_t*) my_get_block_pointer(
                            partition,
                            file->inode->trible_indirect_block
                        ))[t]
                    ))[d]
                ))[i] = free_block;
            }
            else break; // :O too large

            current_block = my_get_block_pointer(partition, file->block);
            file->block_position = 0;
        }

        current_block[file->block_position++] = buffer[buffer_position++];
        file->inode->size = ++(file->position);
    }
    return buffer_position;
}
