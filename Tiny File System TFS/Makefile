CC=gcc
CFLAGS=-g -Wall -D_FILE_OFFSET_BITS=64
LDFLAGS=-lfuse

OBJ=tfs.o block.o

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

tfs: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o tfs

test: test.o
	$(CC) -o test test.o

.PHONY: clean
clean:
	rm -f *.o tfs

