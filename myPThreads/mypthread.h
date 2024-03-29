// File:	mypthread_t.h

// List all group member's name: Simon Bruce
// username of iLab: ssb170
// iLab Server: crayon.cs.rutgers.edu

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

// Enables debug printing using myprintf().  Comment out to disable.
//define DEBUG

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdatomic.h>

typedef uint mypthread_t;

typedef struct threadControlBlock
{
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE

	mypthread_t tid;
	int status;
	ucontext_t *context;
	int quantaCount; // Count of times thread was scheduled to run.  Used as a measure to predict time to completion.  Lowest quantaCount threads run first.

	// Support for join
	struct list *joinQueue;
	void **retValFromJoin;

} tcb;

// List structure
typedef struct list
{
	tcb *tcb;
	struct list *next;
	struct list *prev;
} list;

/* mutex struct definition */
typedef struct mypthread_mutex_t
{
	/* add something here */

	// YOUR CODE HERE

	int mid;				 // Mutex id (for debugging)
	int initialized; // Set on init, cleared on destroy.  Don't allow lock/unlock on initialized mutexes.
	int locked;
	int threadId;					// Thread who has the lock
	list *blockedThreads; // List of threads blocked on the lock
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE

void enqueue(list **head, tcb *tcb);
void enqueueByQuanta(list **head, tcb *tcb);
tcb *dequeue(list **head);
tcb *removeTcb(list **head, mypthread_t tid);
tcb *searchTcb(list **head, mypthread_t tid);
void printReadyQueue(list **head);
void printThreadStatus(int status);

/* Function Declarations: */
void initMainContext();
static void schedule();
static void sched_stcf();
static void sched_mlqf();

/* create a new thread */
int mypthread_create(mypthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg);

/* give CPU pocession to other user level threads voluntarily */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initial the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t
																											 *mutexattr);

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#ifdef DEBUG
#define myprintf(...) printf(__VA_ARGS__)
#else
#define myprintf(...)
#endif

#endif
