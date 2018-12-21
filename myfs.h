#ifndef __H_MY_FS__
#define __H_MY_FS__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define K *(1024  )
#define M *(1024 K)
#define G *(1024 M)

#define NUM_OF_DIRECT_BLOCKS 12

#define MY_TYPE_DIR  1
#define MY_TYPE_SYM  2
#define MY_TYPE_FILE 3

struct my_inode
{
    uint32_t reference_count;
    uint64_t mtime;
    uint32_t size;
    uint32_t direct_block[NUM_OF_DIRECT_BLOCKS];
    uint32_t indirect_block;
    uint32_t double_indirect_block;
    uint32_t trible_indirect_block;
};

struct my_partition
{
    uint32_t size;
    uint32_t inode_size;
    uint32_t block_size;
    uint32_t inode_count;
    uint32_t block_count;
    uint32_t inode_used;
    uint32_t block_used;
    uint32_t root;
    uint32_t inode_bitmap;
    uint32_t block_bitmap;
    uint32_t inodes;
    uint32_t blocks;
};

struct my_file
{
    struct my_inode* inode;
    uint32_t position;
    uint32_t block;
    uint32_t block_position;
};

struct my_dir_list
{
    uint8_t type;
    char filename[512];
    uint32_t inode;
    struct my_dir_list* next;
};

struct my_partition* my_make_partition(uint32_t size);
struct my_partition* my_load_partition_from_file(FILE* file);
void my_dump_partition_to_file(FILE* file);
void my_free_partition(
    struct my_partition* partition);

uint8_t* my_get_block_pointer(
    struct my_partition* partition, uint32_t block);
struct my_inode* my_get_inode_pointer(
    struct my_partition* partition, uint32_t inode);

uint32_t my_get_free_inode(
    struct my_partition* partition);
void my_mark_inode_used(
    struct my_partition* partition, uint32_t inode);
void my_mark_inode_unused(
    struct my_partition* partition, uint32_t inode);

uint32_t my_get_free_block(
    struct my_partition*);
void my_mark_block_used(
    struct my_partition* partition, uint32_t block);
void my_mark_block_unused(
    struct my_partition* partition, uint32_t block);

struct my_dir_list* my_ls_dir(
    struct my_partition* partition, uint32_t dir);
struct my_dir_list* my_get_file(
    struct my_partition* partition,
    struct my_dir_list* file_list, const char* filename);
void my_free_directory_file_list(
    struct my_partition* partition,
    struct my_dir_list* list);

uint32_t my_touch(
    struct my_partition* partition);
bool my_dir_reference_file(
    struct my_partition* partition,
    uint32_t dir, uint32_t file, uint8_t type, const char* filename);
void my_dir_unreference_file(
    struct my_partition* partition,
    uint32_t dir, const char* filename);
void my_delete_file(
    struct my_partition* partition, uint32_t inode);
void my_erase_file(
    struct my_partition* partition, uint32_t inode);

struct my_file* my_file_open(
    struct my_partition* partition, uint32_t file_inode);
struct my_file* my_file_open_end(
    struct my_partition* partition, uint32_t file_inode);
void my_file_close(
    struct my_partition* partition, struct my_file* file);
uint32_t my_file_read(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size);
uint32_t my_file_read_line(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size);
uint32_t my_file_write(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size);

#endif
