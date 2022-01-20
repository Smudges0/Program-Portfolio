#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "./mypthread.h"

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

/* Global variables */
pthread_mutex_t mutex;
pthread_t *thread_array;

void doLongLoop(int x)
{
	// Change this to loop longer.
	// Approx x seconds.

	long T1, T3, T2;
	T1 = time(&T2);
	T3 = (time(&T2)) + x;
	while (time(&T2) < T3)
		;
}

void *testSleep(void *id)
{
	// If using ITIMER_REAL, sleep will get interrupted by the alarm and not sleep the whole period.
	// If using ITIMER_VIRTUAL, sleep will not get interrupted, but scheduler will not run.
	printf("sleep %d start\n", *(int *)id);
	sleep(60);
	printf("sleep %d done!\n", *(int *)id);
}

void *testLoop(void *id)
{
	// Simple thread
	printf("loop %d start\n", *(int *)id);
	doLongLoop(5);
	printf("loop %d done!\n", *(int *)id);
}

void *testLoopAndExit(void *id)
{
	// Use to test join and retvals
	printf("loopAndExit %d start\n", *(int *)id);
	doLongLoop(5);
	printf("loopAndExit %d done!\n", *(int *)id);
	// Return a return value to the joining thread.
	mypthread_exit(id);
}

void *testYieldLoop(void *id)
{
	// Use to test yield
	printf("testYieldLoop %d start\n", *(int *)id);
	doLongLoop(2);
	mypthread_yield();
	doLongLoop(2);
	mypthread_yield();
	printf("testYieldLoop %d done!\n", *(int *)id);
}

void *blockingThreadTest(void *id)
{
	// Lock mutex first
	mypthread_mutex_lock(&mutex);

	// Do work - wait for thread 2 to block
	doLongLoop(1);

	// Unlock
	mypthread_mutex_unlock(&mutex);

	// Do work
	doLongLoop(1);

	// Lock mutex again
	mypthread_mutex_lock(&mutex);

	// Do work
	doLongLoop(1);

	// Unlock
	mypthread_mutex_unlock(&mutex);
}

void manyThreadsJoin()
{
	printf("manyThreadsJoin starting!\n");

	int i = 0;
	int thread_max = 10; // How many threads to create for the tests.
	int thread_num = 0;
	int threads_for_test = 0;

	// // initialize counter array
	// counter = (int *)malloc(thread_max * sizeof(int));
	// for (i = 0; i < thread_max; ++i)
	// {
	// 	counter[i] = i;
	// }

	// initialize array of threads
	thread_array = (pthread_t *)malloc(thread_max * sizeof(pthread_t));

	pthread_mutex_init(&mutex, NULL);

	struct timespec start, end;
	clock_gettime(CLOCK_REALTIME, &start);

	// Execute tests

	// 1) testLoopAndExit with 50 threads
	threads_for_test = thread_max;
	for (i = thread_num; i < (thread_num + threads_for_test); ++i)
	{
		int tid = i + 1;
		pthread_create(&thread_array[i], NULL, &testLoopAndExit, &tid);
	}

	// Main thread waits for all the test threads to complete.
	for (i = 0; i < thread_max; ++i)
	{
		pthread_join(thread_array[i], NULL);
	}

	// Tests Done

	clock_gettime(CLOCK_REALTIME, &end);
	printf("running time: %lu micro-seconds\n",
				 (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);

	pthread_mutex_destroy(&mutex);
	free(thread_array);

	printf("manyThreadsJoin finished!\n");

	//free(counter);
	free(thread_array);
}

void sleepAndYieldTest()
{
	printf("sleepAndYield starting!\n");

	mypthread_t t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15;
	int i1 = 1, i2 = 2, i3 = 3, i4 = 4, i5 = 5, i6 = 6, i7 = 7, i8 = 8, i9 = 9, i10 = 10, i11 = 11, i12 = 12, i13 = 13, i14 = 14, i15 = 15;
	mypthread_create(&t1, NULL, testSleep, &i1);
	mypthread_create(&t2, NULL, testYieldLoop, &i2);
	mypthread_create(&t3, NULL, testYieldLoop, &i3);
	mypthread_create(&t4, NULL, testYieldLoop, &i4);
	mypthread_create(&t5, NULL, testYieldLoop, &i5);

	mypthread_join(t1, NULL);
	mypthread_join(t2, NULL);
	mypthread_join(t3, NULL);
	mypthread_join(t4, NULL);
	mypthread_join(t5, NULL);

	printf("sleepAndYield finished!\n");
}

void joinReturnValueTest()
{
	printf("joinReturnValueTest starting!\n");

	mypthread_t t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15;
	int i1 = 1, i2 = 2, i3 = 3, i4 = 4, i5 = 5, i6 = 6, i7 = 7, i8 = 8, i9 = 9, i10 = 10, i11 = 11, i12 = 12, i13 = 13, i14 = 14, i15 = 15;
	mypthread_create(&t1, NULL, testLoopAndExit, &i1);
	mypthread_create(&t2, NULL, testLoopAndExit, &i2);
	mypthread_create(&t3, NULL, testLoopAndExit, &i3);
	mypthread_create(&t4, NULL, testLoopAndExit, &i4);
	mypthread_create(&t5, NULL, testLoopAndExit, &i5);

	mypthread_join(t1, NULL);
	mypthread_join(t2, NULL);
	mypthread_join(t3, NULL);
	mypthread_join(t4, NULL);

	void *ret1;
	mypthread_join(t5, &ret1);
	printf("Retval = %d!\n", *(int *)ret1);

	printf("joinReturnValueTest finished!\n");
}

void blockingTest()
{
	printf("blockingTest starting!\n");

	mypthread_mutex_init(&mutex, NULL);

	mypthread_t t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15;
	int i1 = 1, i2 = 2, i3 = 3, i4 = 4, i5 = 5, i6 = 6, i7 = 7, i8 = 8, i9 = 9, i10 = 10, i11 = 11, i12 = 12, i13 = 13, i14 = 14, i15 = 15;

	mypthread_create(&t1, NULL, blockingThreadTest, &i1);
	mypthread_create(&t2, NULL, blockingThreadTest, &i2);
	mypthread_join(t1, NULL);
	mypthread_join(t2, NULL);

	printf("\n");

	printf("Normal lock: ");
	if (mypthread_mutex_lock(&mutex) == 0)
	{
		printf("OK\n\n");
	}
	else
	{
		printf("Failed\n\n");
	}

	printf("Double lock: ");
	if (mypthread_mutex_lock(&mutex) != 0)
	{
		printf("OK (rejected)\n\n");
	}
	else
	{
		printf("Failed\n\n");
	}

	printf("Destroy when locked: ");
	if (mypthread_mutex_destroy(&mutex) != 0)
	{
		printf("OK (rejected)\n\n");
	}
	else
	{
		printf("Failed\n\n");
	}

	printf("Normal unlock: ");
	if (mypthread_mutex_unlock(&mutex) == 0)
	{
		printf("OK\n\n");
	}
	else
	{
		printf("Failed\n\n");
	}

	printf("Normal destroy: ");
	if (mypthread_mutex_destroy(&mutex) == 0)
	{
		printf("OK\n\n");
	}
	else
	{
		printf("Failed\n\n");
	}

	printf("Double destroy: ");
	if (mypthread_mutex_destroy(&mutex) == 0)
	{
		printf("OK\n\n");
	}
	else
	{
		printf("Failed\n\n");
	}

	printf("Lock after destroy: ");
	if (mypthread_mutex_lock(&mutex) != 0)
	{
		printf("OK (rejected)\n\n");
	}
	else
	{
		printf("Failed\n\n");
	}

	printf("Init after destroy: ");
	if (mypthread_mutex_init(&mutex, NULL) == 0)
	{
		printf("OK\n\n");
	}
	else
	{
		printf("Failed\n\n");
	}

	printf("blockingTest finished!\n");
}

/*================================================================
 *
 * main
 * 
 *================================================================*/
int main(int argc, char **argv)
{
	/* Implement HERE */

	// manyThreadsJoin();
	// printf("\n\n\n\n\n");

	// sleepAndYieldTest();
	// printf("\n\n\n\n\n");

	// joinReturnValueTest();
	// printf("\n\n\n\n\n");

	blockingTest();
}
