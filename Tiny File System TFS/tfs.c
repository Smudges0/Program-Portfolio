/*
 *  Copyright (C) 2019 CS416 Spring 2019
 *	
 *	Tiny File System
 *
 *	File:	tfs.c
 *  Author: Yujie REN
 *	Date:	April 2019
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here
const int INODES_PER_BLOCK = BLOCK_SIZE / sizeof(struct inode);
const int DIRENT_PER_BLOCK = BLOCK_SIZE / sizeof(struct dirent);

struct superblock *newSuperBlock;
bitmap_t inodeBitmap,
		dataBlockBitmap;

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino()
{

	printf("get_avail_ino\n");

	// Step 1: Read inode bitmap from disk
	bio_read(newSuperBlock->i_bitmap_blk, inodeBitmap);

	// Step 2: Traverse inode bitmap to find an available slot
	for (int i = 0; i < MAX_INUM; i++)
	{
		if (get_bitmap(inodeBitmap, i) == 0)
		{

			// Step 3: Update inode bitmap and write to disk
			set_bitmap(inodeBitmap, i);
			bio_write(newSuperBlock->i_bitmap_blk, inodeBitmap);
			return i;
		}
	}

	return -1;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno()
{

	printf("get_avail_blkno\n");

	// Step 1: Read data block bitmap from disk
	bio_read(newSuperBlock->d_bitmap_blk, dataBlockBitmap);
	// Step 2: Traverse data block bitmap to find an available slot
	for (int i = 0; i < MAX_INUM; i++)
	{
		if (get_bitmap(dataBlockBitmap, i) == 0)
		{

			// Step 3: Update data block bitmap and write to disk
			set_bitmap(dataBlockBitmap, i);
			bio_write(newSuperBlock->d_bitmap_blk, dataBlockBitmap);
			return i;
		}
	}

	return -1;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode)
{

	printf("readi\n");

	// Step 1: Get the inode's on-disk block number
	// blocknum = ino / INODES_PER_BLOCK   <-- Calculate these as constants

	// Step 2: Get offset of the inode in the inode on-disk block
	// offset = (ino % INODES_PER_BLOCK) * SIZE_OF_INODE <-- Calculate these as constants

	// Step 3: Read the block from disk and then copy into inode structure
	// Start from superblock->i_bitmap_blk, skip (blocknum-1)*BLOCK_SIZE to get to start of the block, skip offset to get to start of inode.

	return 0;
}

int writei(uint16_t ino, struct inode *inode)
{

	printf("writei\n");

	// Step 1: Get the block number where this inode resides on disk

	// Step 2: Get the offset in the block where this inode resides on disk

	// Step 3: Write inode to disk

	return 0;
}

/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent)
{
	printf("dir_find\n");

	// NOTE: Do NOT need to support large directories.  No indirect pointer support required for directories, only files.
	// Still need to support 16 blocks of directory entries per inode.

	// Step 1: Call readi() to get the inode using ino (inode number of current directory)

	// Step 2: Get data block of current directory from inode

	// Step 3: Read directory's data block and check each directory entry.
	//If the name matches, then copy directory entry to dirent structure

	return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len)
{

	printf("dir_add\n");

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode

	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len)
{
	printf("dir_remove\n");

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode

	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode)
{
	printf("get_node_by_path\n");

	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way

	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs()
{
	printf("mkfs\n");

	// Call dev_init() to initialize (Create) Diskfile	// dev_init() is in block.c
	dev_init(diskfile_path);

	// Calculate the number of inode blocks needed for MAX_INUM inodes.  Must allow for all the blocks,
	// so we can locate the datablocks at the end.  Round up a block if there is any remainder.
	// Better way to round up: Divide as double, with decimal, and use ceil() function to round up.
	// But somehow math.h does not seem to include ceil() function.
	//int inumBlockCount = ceil((double)MAX_INUM / INODES_PER_BLOCK);
	int inumBlockCount = MAX_INUM / INODES_PER_BLOCK;
	if ((MAX_INUM % INODES_PER_BLOCK) != 0)
	{
		inumBlockCount++;
	}

	// write superblock information
	newSuperBlock = malloc(sizeof(struct superblock));
	newSuperBlock->magic_num = MAGIC_NUM;
	newSuperBlock->max_inum = MAX_INUM;
	newSuperBlock->max_dnum = MAX_DNUM;
	newSuperBlock->i_bitmap_blk = 1;
	newSuperBlock->d_bitmap_blk = 2;
	newSuperBlock->i_start_blk = 3;
	newSuperBlock->d_start_blk = 3 + inumBlockCount;

	// NOTE: This actually writes an entire BLOCK_SIZE of data to the disk, not just the superblock data.  Should be OK though...
	bio_write(0, newSuperBlock);

	// initialize inode bitmap
	inodeBitmap = malloc(BLOCK_SIZE);
	memset(inodeBitmap, '\0', MAX_INUM);
	bio_write(1, inodeBitmap);

	// initialize data block bitmap
	dataBlockBitmap = malloc(BLOCK_SIZE);
	memset(dataBlockBitmap, '\0', MAX_INUM);
	bio_write(2, dataBlockBitmap);

	// update inode for root directory
	void *block = malloc(BLOCK_SIZE);
	struct inode *newInode = (struct inode *)block;
	formatInodeBlock(3, newInode);

	newInode->ino = get_avail_ino();
	newInode->valid = VALID;
	newInode->size = BLOCK_SIZE;
	newInode->type = TYPE_DIR;
	newInode->link = 2; // All Directories have 2 links (/, .)
	newInode->direct_ptr[0] = get_avail_blkno();
	// vstat stuff

	bio_write(3, newInode);

	struct dirent *rootDirEnt = malloc(BLOCK_SIZE);
	formatDirEntBlock(newSuperBlock->d_start_blk, rootDirEnt);

	rootDirEnt[0].ino = newInode->ino;
	rootDirEnt[0].valid = VALID;
	strcpy(rootDirEnt[0].name, "/");

	rootDirEnt[1].ino = newInode->ino;
	rootDirEnt[1].valid = VALID;
	strcpy(rootDirEnt[1].name, ".");

	rootDirEnt[2].ino = newInode->ino;
	rootDirEnt[2].valid = VALID;
	strcpy(rootDirEnt[2].name, "..");

	int dirEntOffset = newSuperBlock->d_start_blk + newInode->direct_ptr[0];

	bio_write(dirEntOffset, rootDirEnt);

	free(newSuperBlock);
	return 0;
}

/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn)
{

	printf("init\n");

	// Step 1a: If disk file is not found, call mkfs
	// Disk file is stored in diskfile_path var.
	// Use dev_open() in block.c to try and open it.  -1 means it was not found.

	if (dev_open(diskfile_path) == -1)
	{
		tfs_mkfs();
	}

	// Step 1b: If disk file is found, just initialize in-memory data structures
	// and read superblock from disk

	// WARNING - Can't do a bio_read (block) into a superBlock pointer!  It will be too big and overwrite memory!
	// Must allocate a block-sized buffer, and then cast the buffer to the superblock.
	//bio_read(0, superBlock); // This was causing segfaults!
	void *buf = malloc(BLOCK_SIZE);
	bio_read(0, buf);
	struct superblock *superBlock = (struct superblock *)buf;
	printf("MAGIC_NUM: %#x\n", superBlock->magic_num);

	if (superBlock->magic_num != MAGIC_NUM)
	{
		perror("Magic number is wrong!");
		exit(EXIT_FAILURE);
	}

	int inode_block_num = superBlock->i_start_blk;

	bio_read(inode_block_num, buf);
	struct inode *readInode = (struct inode *)buf;
	if (readInode->valid)
	{
		printf("Valid! Inode->ino = %d\n", readInode->ino);
	}

	free(buf);

	return NULL;
}

static void tfs_destroy(void *userdata)
{
	printf("destroy\n");

	// Step 1: De-allocate in-memory data structures

	// Step 2: Close diskfile
}

static int tfs_getattr(const char *path, struct stat *stbuf)
{
	printf("getattr\n");

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode

	stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 2;
	time(&stbuf->st_mtime);

	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi)
{

	printf("opendir\n");

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{

	printf("readdir\n");

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	return 0;
}

static int tfs_mkdir(const char *path, mode_t mode)
{

	printf("mkdir\n");

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory

	// Step 5: Update inode for target directory

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_rmdir(const char *path)
{

	printf("rmdir\n");

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi)
{
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{

	printf("Created some stuff\n");

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi)
{

	printf("Opened some stuff\n");

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{

	printf("Read some stuff\n");

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("Write some stuff\n");

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path)
{

	printf("Unlink some stuff\n");

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of target file

	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

	return 0;
}

static int tfs_truncate(const char *path, off_t size)
{
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi)
{
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char *path, struct fuse_file_info *fi)
{
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2])
{
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static struct fuse_operations tfs_ope = {
		.init = tfs_init,
		.destroy = tfs_destroy,

		.getattr = tfs_getattr,
		.readdir = tfs_readdir,
		.opendir = tfs_opendir,
		.releasedir = tfs_releasedir,
		.mkdir = tfs_mkdir,
		.rmdir = tfs_rmdir,

		.create = tfs_create,
		.open = tfs_open,
		.read = tfs_read,
		.write = tfs_write,
		.unlink = tfs_unlink,

		.truncate = tfs_truncate,
		.flush = tfs_flush,
		.utimens = tfs_utimens,
		.release = tfs_release};

int main(int argc, char *argv[])
{
	int fuse_stat;
	// argv contains the path to the mount point.  Passed to fuse_main.
	getcwd(diskfile_path, PATH_MAX);		// Current directory where tfs is started
	strcat(diskfile_path, "/DISKFILE"); // diskfile_path is the full path and name of the actual file where our disk will be saved.

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

	return fuse_stat;
}

// New inode should point to malloc'd memory of BLOCK_SIZE
void formatInodeBlock(uint32_t blockNum, struct inode *newInode)
{

	// void *block = malloc(BLOCK_SIZE);
	// struct inode *newInode = (struct inode *)block;
	memset(newInode, '\0', BLOCK_SIZE);

	for (int i = 0; i < INODES_PER_BLOCK; i++)
	{
		newInode[i].ino = (blockNum * INODES_PER_BLOCK) + i;
		newInode[i].valid = INVALID;

		for (int j = 0; j < NUM_DIRECT_PTRS; j++)
		{
			newInode[i].direct_ptr[j] = -1;
		}

		for (int j = 0; j < NUM_INDIRECT_PTRS; j++)
		{
			newInode[i].indirect_ptr[j] = -1;
		}
	}

	bio_write(blockNum, newInode);
}

void formatDirEntBlock(uint32_t blockNum, struct dirent *newDirEnt)
{
	memset(newDirEnt, '\0', BLOCK_SIZE);

	for (int i = 0; i < DIRENT_PER_BLOCK; i++)
	{
		newDirEnt[i].ino = -1;
		newDirEnt[i].valid = INVALID;
	}

	bio_write(blockNum, newDirEnt);
}