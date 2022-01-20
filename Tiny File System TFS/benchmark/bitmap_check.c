#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

/* You need to change this macro to your TFS mount point*/
#define TESTDIR "/tmp/mountdir"

#define N_FILES 100
#define BLOCKSIZE 4096 // Standard blocksize
#define FSPATHLEN 256
#define ITERS 10
#define FILEPERM 0666
#define DIRPERM 0755

#define I_BITMAP_BLOCK 4096 // Offset of bitmap for inodes in diskfile
#define D_BITMAP_BLOCK 8192 // Offset of bitmap for datablocks in diskfile

#define MAX_INUM 1024
#define MAX_DNUM 16384

#define ERR_THRESHOLD 5

// NOTE: See bitmap_testing.c for details on the bitmap typedefs.
// Turns out, the bitmap is allocated using bytes (unsigned char and unsigned uint8 are both 1 bytes = 8 bit)
// But the bits are actually stored as bits, so the first 8 bits are stored in the first byte, etc.
// The possible number of inodes or data blocks can be lower or higher than the blocksize. (Max = 8 * 4096 = 32768 bits)
// For this project, it is 1024 max inodes and 16384 data blocks.
typedef unsigned char* bitmap_t;

void set_bitmap(bitmap_t b, int i) {
	b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i) {
	b[i / 8] &= ~(1 << (i & 7));
}

// uint8_t = unsigned int = 1 byte
uint8_t get_bitmap(bitmap_t b, int i) {
	return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
}

bitmap_t bitmap;
char buf[BLOCKSIZE];

int diskfile = -1;

int main(int argc, char **argv) {

	int i, fd = 0, ret = 0;
	int icount = 0, dcount = 0, icount_after = 0, dcount_after = 0;
	struct stat st;

	bitmap = malloc(BLOCKSIZE);

	// Directly open the disk file, and read the data structure for the bitmaps and datablocks????
	//
	// See diagram of Ext2 filesystem: http://www.science.smith.edu/~nhowe/teaching/csc262/oldlabs/ext2.html#struct
	//
	// For this project, it seems like the structure is like this:
	// Superblock (1 blk) | Inode bitmap (1 blk) | Datablock bitmap (1 blk) | Inode table?    | Data blocks?    | 
	//                    | Offset = 4096        | Offset = 8192            |                 |                 |   
	//                    | 1024 bits/inodes     | 16384 bits = data blocks | N blocks        | N block         |

	diskfile = open("DISKFILE", O_RDWR, S_IRUSR | S_IWUSR); // Read & write mode, read & write by owner (600)
	if (diskfile < 0) {
		perror("disk_open failed");
		return -1;
	}

	// Read inode bitmap
	ret = pread(diskfile, bitmap, BLOCKSIZE, I_BITMAP_BLOCK); // Read <BLOCKSIZE = 4096> bytes starting from offset <I_BITMAP_BLOCK = 4096> without changing file pointer.
	if (ret <= 0) {
		perror("bitmap failed");
	}

	for (i = 0; i < MAX_INUM; ++i) { 
		if (get_bitmap(bitmap, i) == 1) // Scan from 0 to 1024 (MAX_INUM) and count number of 1s. (bitmap buffer is 4096 long.  1 block.)
			++icount;
	}

	// Read data block bitmap
	ret = pread(diskfile, bitmap, BLOCKSIZE, D_BITMAP_BLOCK); // Read <BLOCKSIZE = 4096> bytes starting from offset <D_BITMAP_BLOCK = 8192> without changing file pointer.
	if (ret <= 0) {											  // Offset is 1 block after the init bitmap.
		perror("bitmap failed");
	}

	for (i = 0; i < MAX_DNUM; ++i) {
		if (get_bitmap(bitmap, i) == 1) // Scan from 0 to 16384 (MAX_DNUM) and count number of 1s. (bitmap buffer is 4096 long. 1 block.)
			++dcount;
	}
	

	// Then do a set of operations
	if ((ret = mkdir(TESTDIR "/testdir", DIRPERM)) < 0) {
		perror("mkdir");
		exit(1);
	}

	for (i = 0; i < N_FILES; ++i) {
		char subdir_path[FSPATHLEN];
		memset(subdir_path, 0, FSPATHLEN);

		sprintf(subdir_path, "%s%d", TESTDIR "/testdir/dir", i);
		if ((ret = mkdir(subdir_path, DIRPERM)) < 0) {
			perror("mkdir");
			exit(1);
		}

		if ((ret = rmdir(subdir_path)) < 0) {
			perror("rmdir");
			exit(1);
		}
	}

	if ((fd = creat(TESTDIR "/test", FILEPERM)) < 0) {
		perror("creat");
		exit(1);
	}

	for (i = 0; i < ITERS; i++) {
		//memset with some random data
		memset(buf, 0x61 + i, BLOCKSIZE);

		if (write(fd, buf, BLOCKSIZE) != BLOCKSIZE) {
			perror("write");
			exit(1);
		}
	}

	if (close(fd) < 0) {
		perror("close");
		exit(1);
	}

	if ((ret = unlink(TESTDIR "/test")) < 0) {
		perror("unlink");
		exit(1);
	}	

	if ((ret = rmdir(TESTDIR "/testdir")) < 0) {
		perror("rmdir");
		exit(1);
	}

	// Read inode bitmap
	ret = pread(diskfile, bitmap, BLOCKSIZE, I_BITMAP_BLOCK);
	if (ret <= 0) {
		perror("bitmap failed");
	}

	for (i = 0; i < MAX_INUM; ++i) {
		if (get_bitmap(bitmap, i) == 1)
			++icount_after;
	}

	memset(bitmap, 0, BLOCKSIZE);

	// Read data block bitmap
	ret = pread(diskfile, bitmap, BLOCKSIZE, D_BITMAP_BLOCK);
	if (ret <= 0) {
		perror("bitmap failed");
	}

	for (i = 0; i < MAX_DNUM; ++i) {
		if (get_bitmap(bitmap, i) == 1)
			++dcount_after;
	}
	
	if (abs(icount - icount_after) < ERR_THRESHOLD) {
		printf("inodes reclaimed successfully %d\n", icount - icount_after);
	} else {
		printf("inodes reclaimed unsuccessfuly %d\n", icount - icount_after);
	}

	if (abs(dcount - dcount_after) < ERR_THRESHOLD) {
		printf("data blocks reclaimed successfully %d\n", dcount - dcount_after);
	} else {
		printf("data blocks reclaimed unsuccessfuly %d\n", dcount - dcount_after);
	}

	printf("Benchmark completed \n");
	return 0;
}
