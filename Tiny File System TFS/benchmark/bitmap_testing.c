#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* --------------------------------------------
 * Testing the way the bitmap is allocated and updated.
 * I think that the bitmap is allocated in BYTES (8 bits) but then is set/unset
 * in BITS.
 * 
 * Turns out, the bitmap is allocated using bytes (unsigned char and unsigned uint8 are both 1 bytes = 8 bit)
 * But the bits are actually stored as bits, so the first 8 bits are stored in the first byte, etc.
 * The possible number of inodes or data blocks can be lower or higher than the blocksize. (Max = 8 * 4096 = 32768 bits)
 * For this project, it is 1024 max inodes and 16384 data blocks.
 * 
 * If you run this, it allocates a bitmap of BLOCKSIZE 10, which is 10 bytes.
 * But if you set 10 bits, only the first two bytes get set to non-zero values.
 * 
 * 1111111111
 * 255.3.0.0.0.0.0.0.0.0.
 *
 * 0000000000
 * 0.0.0.0.0.0.0.0.0.0.
 *
 * Debug logging shows that the first 8 bits are all stored in the first byte, and next 8 bits stored in the seconnd byte, etc.
 *---------------------------------------------*/

#define BLOCKSIZE 10 // Standard blocksize

// What is diff between bitmap_t (unsigned char) and uint8_t (unsigned int)?  
// Both are 1 byte long (8 bits).  (See https://docs.microsoft.com/en-us/cpp/cpp/data-type-ranges?view=msvc-170)
//
// https://stackoverflow.com/questions/653336/should-a-buffer-of-bytes-be-signed-or-unsigned-char-buffer
// If you intend to store arbitrary binary data, you should use unsigned char. It is the only data type 
// that is guaranteed to have no padding bits by the C Standard. Each other data type may contain 
// padding bits in its object representation.
//
// NOTE: I think that the bitmap is not actually stored in bits, but bytes, which is confusing.  But that is why the typdefs
// are unsigned chars and unsigned uint8, which are both 1 byte/8 bits.  
// If we want to store the bitmap using actual bits, it can be done: (but we don't want to change the set/unset/get methods in the project)
// https://stackoverflow.com/questions/2525310/how-to-define-and-work-with-an-array-of-bits-in-c
typedef unsigned char* bitmap_t;

void set_bitmap(bitmap_t b, int i) {
	b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i) {
	b[i / 8] &= ~(1 << (i & 7));
}

// uint8_t = unsigned int = 1 byte
uint8_t get_bitmap(bitmap_t b, int i) {
    printf("i / 8 = %d\n", i / 8); // If i < 1, i/8 rounds to 0, so first 8 bits all come from b[0].  Next 8 from b[1], etc.
    printf("b[i / 8] = %d\n", b[i / 8]);
	return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
}

bitmap_t bitmap;

int main()
{
    
	bitmap = malloc(BLOCKSIZE);
	
// 	set_bitmap(bitmap,0);
// 	set_bitmap(bitmap,2);
// 	set_bitmap(bitmap,4);
// 	set_bitmap(bitmap,6);
// 	set_bitmap(bitmap,8);
	for (int i = 0; i < BLOCKSIZE; ++i) {  
		set_bitmap(bitmap, i);
	}

	// Print as bits
	for (int i = 0; i < BLOCKSIZE; ++i) {  
		printf("%d",get_bitmap(bitmap, i));
	}
    printf("\n\n");
    
    // Print bytes
	for (int i = 0; i < BLOCKSIZE; ++i) {  
		printf("%d.",bitmap[i]);
	}
    printf("\n\n");
    
//  unset_bitmap(bitmap,0);
// 	unset_bitmap(bitmap,2);
// 	unset_bitmap(bitmap,4);
// 	unset_bitmap(bitmap,6);
// 	unset_bitmap(bitmap,8);
	for (int i = 0; i < BLOCKSIZE; ++i) {  
		unset_bitmap(bitmap, i);
	}	

	// Print as bits
	for (int i = 0; i < BLOCKSIZE; ++i) {  
		printf("%d",get_bitmap(bitmap, i));
	}
    printf("\n\n");
    
    // Print bytes
	for (int i = 0; i < BLOCKSIZE; ++i) {  
		printf("%d.",bitmap[i]);
	}
    printf("\n\n");
}
