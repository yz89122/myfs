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

/**
 * Make a partition for a given size.
 * If the given size is less than 5K then
 * it'll return NULL pointer.
 * 
 * The number of inodes is equal to the number
 * of blocks.
 * 
 * In default, the block size is 1K. In this
 * situation, a block of bitmap can record
 * 8*1024 of blocks or inodes.
 */
struct my_partition* my_make_partition(uint32_t size);

/**
 * Load partition from given file.(Not implement yet)
 */
struct my_partition* my_load_partition_from_file(FILE* file);

/**
 * Dump the partition to the given file pointer.
 * (not implement yet)
 */
void my_dump_partition_to_file(FILE* file);

/**
 * Free the partition in memory.
 * Actually, it just exec `free(partition);`
 */
void my_free_partition(
    struct my_partition* partition);


/**
 * Return a pointer point to the begining
 * of the given block. Keep in mind the type
 * of the pointer.
 */
uint8_t* my_get_block_pointer(
    struct my_partition* partition, uint32_t block);

/**
 * Return the pointer point to the given inode
 * number.
 */
struct my_inode* my_get_inode_pointer(
    struct my_partition* partition, uint32_t inode);


/**
 * Get a inode number that is available. If
 * there are no more available inode, `-1` will
 * returned.
 */
uint32_t my_get_free_inode(
    struct my_partition* partition);

/**
 * Mark the given inode number used(unavailable)
 * in bitmap, and increase `partition->inode_used`
 * if the inode was marked unused before calling
 * this function.
 */
void my_mark_inode_used(
    struct my_partition* partition, uint32_t inode);

/**
 * Mark the given inode number unused(available)
 * in bitmap, and decrease `partition->inode_used`
 * if the inode was marked available before calling
 * this function.
 */
void my_mark_inode_unused(
    struct my_partition* partition, uint32_t inode);


/**
 * Get a block number that is available. If
 * there are no more available block, `0` will
 * returned.
 */
uint32_t my_get_free_block(
    struct my_partition*);

/**
 * Mark the given block number used(unavailable)
 * in bitmap, and increase `partition->block_used`
 * if the block was marked unused before calling
 * this function.
 */
void my_mark_block_used(
    struct my_partition* partition, uint32_t block);

/**
 * Mark the given block number unused(available)
 * in bitmap, and decrease `partition->block_used`
 * if the block was marked available before calling
 * this function.
 */
void my_mark_block_unused(
    struct my_partition* partition, uint32_t block);

/**
 * List the given directory, and return the content
 * inside the directory, return NULL if the
 * directory is empty.
 * 
 * After calling this function, `my_free_dir_list`
 * should be called to free the list.
 */
struct my_dir_list* my_ls_dir(
    struct my_partition* partition, uint32_t dir);

/**
 * Finding the given filename in the given list
 * returned by the `my_ls_dir` function. Return
 * pointer pointed to the node of the list when
 * the filename existed in the directory. Return
 * NULL when the given filename is not exist.
 */
struct my_dir_list* my_get_file(
    struct my_partition* partition,
    struct my_dir_list* file_list, const char* filename);

/**
 * Free the list returned by the `my_ls_dir`
 * function.
 */
void my_free_dir_list(
    struct my_partition* partition,
    struct my_dir_list* list);


/**
 * Register a inode used in the partition.
 * The returned inode number should be referenced
 * immediately. If the returned inode number
 * was not referenced, it'll resulted in a zombie
 * inode.
 */
uint32_t my_touch(
    struct my_partition* partition);

/**
 * Reference the given inode to the given directory,
 * and increase reference count of the inode.
 * Return on reference seccussfully(the given
 * filename was not exist in the given directory),
 * else return false.
 */
bool my_dir_reference_file(
    struct my_partition* partition, uint32_t dir,
    uint32_t file, uint8_t type, const char* filename);

/**
 * Unreference the given filename in the directory,
 * and decrease the reference count of the file.
 * It will delete the file only if the reference
 * count is ZERO after calling this function.
 */
void my_dir_unreference_file(
    struct my_partition* partition,
    uint32_t dir, const char* filename);

/**
 * Delete the given inode of file. If you don't know
 * what that means, DO NOT CALL THIS FUNCTION.
 */
void my_delete_file(
    struct my_partition* partition, uint32_t inode);

/**
 * Release all the block that the given inode is
 * using and set size to zero.
 */
void my_erase_file(
    struct my_partition* partition,
    uint32_t inode);


/**
 * Open the given inode, and point to the begining
 * of the file.
 */
struct my_file* my_file_open(
    struct my_partition* partition,
    uint32_t file_inode);

/**
 * Same as `my_file_open`, but point the the end
 * of the file. (append mode)
 */
struct my_file* my_file_open_end(
    struct my_partition* partition,
    uint32_t file_inode);

/**
 * Move the pointer to given position. Return
 * the position it really moved to.
 */
uint32_t my_file_seek(
    struct my_partition* partition,
    struct my_file* file, uint32_t position);

/**
 * Move the pointer to the end of the file.
 * Return the position it really moved to.
 */
uint32_t my_file_seek_end(
    struct my_partition* partition,
    struct my_file* file);

/**
 * Close the file pointer. Actually it execute
 * `free(file)`.
 */
void my_file_close(
    struct my_partition* partition, struct my_file* file);

/**
 * Read from given file, read until reached buffer
 * size of occurred EOF. Return the length of data
 * written to the buffer, 0 on EOF.
 */
uint32_t my_file_read(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size);

/**
 * Same as `my_file_read`. Read until occurred `\n`.
 */
uint32_t my_file_read_line(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size);

/**
 * Write buffer to given file, return the length
 * written to the file, return 0 if there's no more
 * space.
 */
uint32_t my_file_write(
    struct my_partition* partition, struct my_file* file,
    uint8_t* buffer, uint32_t buffer_size);

#endif
