CC = gcc
CFLAGS = -g

all: simple_test test_case bitmap_check bitmap_testing

simple_test: simple_test.c
	$(CC) $(CFLAGS) -o simple_test simple_test.c

test_case: test_case.c
	$(CC) $(CFLAGS) -o test_case test_case.c

bitmap_check: bitmap_check.c
	$(CC) $(CFLAGS) -o bitmap_check bitmap_check.c

bitmap_testing: bitmap_testing.c
	$(CC) $(CFLAGS) -o bitmap_testing bitmap_testing.c

clean:
	rm -rf simple_test test_case bitmap_check bitmap_testing
