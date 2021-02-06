#ifndef FS_H_
#define FS_H_

#include <stdio.h>
#include "disk.h"
#include <stddef.h> //for size_t 
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#define MAX_F_NAME 15
#define BLOCK_SIZE 4096 //4096 bytes
#define BLOCK_ESIZE 4092
//superblock is the first block in our file system, and it tells us where all our structures are.
struct super_block {
	int fat_idx; // First block of the FAT
	int fat_len; // Length of FAT in blocks
	int dir_idx; // First block of directory
	int dir_len; // Length of directory in blocks
	int data_idx; // First block of file-data
};

struct file_descriptor {
	int used; // fd in use
	int file; // the first block of the file
	// (f) to which fd refers too
	int offset; // position of fd within f
	int blocks;
	int dir_index; //directory entry that fd refers to.
};

struct dir_entry {
	int used; // Is this file-”slot” in use
	char name[MAX_F_NAME + 1]; // DOH!
	int size; // file size (in bytes)
	int head; // first data block of file
	int ref_cnt;
	int blocks; //tells us how many blocks a file consumes.
	// how many open file descriptors are there?
	// ref_cnt > 0 -> cannot delete file
};

//this will be a node of data in our file. Thus, each data block will contain its data, and a pointer to the next node.
struct data_node {
	char data[BLOCK_SIZE - sizeof(int)]; //this is the size needed for data, since we need to also store an 8 byte pointer (to next node) in our string. 4088 bytes
	int next;
};

int make_fs(char* disk_name);
int mount_fs(char* disk_name);
int umount_fs(char* disk_name);
int fs_open(char* name);
int fs_close(int fildes);
int fs_create(char* name);
int fs_delete(char* name);
int fs_read(int fildes, void* buf, size_t nbyte);
int fs_write(int fildes, void* buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_listfiles(char*** files);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);
int find_file_block(int dir_idx, int cur_block, int n);


#endif