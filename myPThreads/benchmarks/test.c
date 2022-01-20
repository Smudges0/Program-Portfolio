#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

void *printString(void *something)
{
	printf("I am in the thread!\n");
	return NULL;
}

int main(int argc, char **argv)
{

	/* Implement HERE */
	// int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void *), void *arg);
	mypthread_t testThread;

	mypthread_create(&testThread, NULL, printString, NULL);
	// mypthread_join(testThread, NULL);
	printf("Outside the thread!\n");
	return 0;
}
