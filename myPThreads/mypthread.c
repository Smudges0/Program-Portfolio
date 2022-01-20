// File:	mypthread.c

// List all group member's name: Simon Bruce
// username of iLab: ssb170
// iLab Server: crayon.cs.rutgers.edu

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

// Thread states
#define NEW_STATE 0				 // If created (Not used)
#define READY_STATE 1			 // If yield or unblocked
#define RUNNING_STATE 2		 // If active (preempted)
#define BLOCKED_STATE 3		 // If joining or waiting on mutex lock
#define TERMINATED_STATE 4 // If ended

#define STACK_SIZE SIGSTKSZ		 // SIGSTKSZ = 8192 on my PC and ilab servers.  Same as ulimit -s command output.                               
							 // 16384 - Size from sample code in man makecontext (https://man7.org/linux/man-pages/man3/swapcontext.3.html) 

#define QUANTA_TIME_USECS 5000 // 5K Microseconds = 5 milliseconds

int haveMainContext = 0;			// Flag to check if the main context has been initialized (only once)
tcb *activeThread;						// Keep track of the current thread.
ucontext_t threadExitContext; // Global context used by every thread when terminating to clean up.

list *readyQueue; // Queue of all threads which are ready to run (not running). Does not include blocked threads
									// Threads added to readyQueue will be ordered so that lowest quanta is at the tail

list *allThreads; // List of all threads, ready and blocked. Required to find tcbs by tid when joining a thread

int doNotInterrupt = 0;					// Flag to prevent scheduler from preempting the thread while in a critical section.
struct itimerval timerSettings; // Use to control the timer settings.
mypthread_t nextThreadId = 0;		// Global thread id counter
int nextMutexId = 0;						// Global mutex id counter

/*============================================================================
 *
 * Pthread functions
 * 
 *============================================================================*/

/*----------------------------------
 *  create a new thread
 *----------------------------------*/
int mypthread_create(mypthread_t *thread, pthread_attr_t *attr,
										 void *(*function)(void *), void *arg)
{
	// create Thread Control Block
	// create and initialize the context of this thread
	// allocate space of stack for this thread to run
	// after everything is all set, push this thread int

	// YOUR CODE HERE

	// NOTE: The current thread context is the caller, who is creating the new
	// thread.  The thread must be added to the readyQueue here, because the
	// scheduler cannot enqueue it because it is not be the active thread.
	//
	// NOTE 2: The function being passed in must take only one arg, of type void *.
	// When calling makeContext, you must hardcode argc = 1 before passing in the arg.

	// Check for main context
	if (!haveMainContext)
	{
		initMainContext();
	}

	doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.  Checking and update should be consistent.
	{
		// Create thread context
		ucontext_t *threadContext = (ucontext_t *)malloc(sizeof(ucontext_t));
		getcontext(threadContext);
		threadContext->uc_link = &threadExitContext; // When thread finishes its function, continue in the global exit context
		threadContext->uc_stack.ss_sp = malloc(STACK_SIZE);
		threadContext->uc_stack.ss_size = STACK_SIZE;
		threadContext->uc_stack.ss_flags = 0;
		makecontext(threadContext, (void *)function, 1, arg);

		tcb *newThread = (tcb *)malloc(sizeof(tcb));
		newThread->context = threadContext;
		newThread->tid = nextThreadId++;
		newThread->status = READY_STATE;
		newThread->quantaCount = 0;
		newThread->joinQueue = NULL;			// Tracks threads which this thread is joined on (waiting for this thread to exit).
		newThread->retValFromJoin = NULL; // Set by a joined thread on the current thread when the joined thread exits.

		*thread = newThread->tid;

		myprintf("Thread %d created new thread %d.\n", activeThread->tid, newThread->tid);

		// Queue the thread and notify scheduler.
		enqueue(&allThreads, newThread);
		enqueueByQuanta(&readyQueue, newThread); // We must enqueue the new thread before notifying the scheduler.
	}
	doNotInterrupt = 0; // Must reset before exiting
	raise(SIGALRM);			// Go to scheduler function

	return 0;
};

/*----------------------------------
 *  give CPU possession to other user-level threads voluntarily
 *----------------------------------*/
int mypthread_yield()
{
	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// switch from thread context to scheduler context

	// YOUR CODE HERE

	myprintf("Thread %d yielding.\n", activeThread->tid);

	activeThread->status = READY_STATE;
	raise(SIGALRM); // Go to scheduler function

	return 0;
};

/*----------------------------------
 *  terminate a thread
 *----------------------------------*/
void mypthread_exit(void *value_ptr)
{
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE

	// NOTE: This function can be called manually, but is also as part of the cleanup for all threads
	// which complete their functions before they stop running.  (All thread contexts' ulink value = threadExitContext)
	// If called manually, the thread does not complete it's function, so the ulink context is not invoked. (Exit is not called twice.)

	myprintf("Thread %d exiting.\n", activeThread->tid);

	doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.  Checking and update should be consistent.
	{
		// Handle joined threads.  Remove them from the exiting thread's join queue and enqueue them in the readyQueue
		// so the scheduler can manage them again.
		tcb *tcb;
		while (activeThread->joinQueue != NULL)
		{
			tcb = dequeue(&activeThread->joinQueue);
			tcb->status = READY_STATE;
			enqueueByQuanta(&readyQueue, tcb);

			// Return the exit value in the location specified in the join request, if specified.
			if (tcb->retValFromJoin)
			{
				*(tcb->retValFromJoin) = value_ptr;
			}
		}

		removeTcb(&allThreads, activeThread->tid); // Clear from allThreads list.

		activeThread->status = TERMINATED_STATE;
	}
	doNotInterrupt = 0; // Must reset before exiting

	raise(SIGALRM); // Go to scheduler function
};

/*----------------------------------
 *  Wait for thread termination
 *----------------------------------*/
int mypthread_join(mypthread_t thread, void **value_ptr)
{
	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE

	myprintf("Thread %d joining on %d.\n", activeThread->tid, thread);

	doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.  Checking and update should be consistent.
	{
		if (activeThread->tid == thread)
		{											// Don't allow join on self
			doNotInterrupt = 0; // Must reset before exiting
			return -1;
		}

		// Find the thread to join by searching for the thread id (thread) in allThreads list.
		tcb *targetThread = searchTcb(&allThreads, thread);
		if (!targetThread)
		{
			doNotInterrupt = 0; // Must reset before exiting
			return -1;
		}

		// Add the current thread to the target thread's join queue.
		enqueue(&(targetThread->joinQueue), activeThread);
		activeThread->retValFromJoin = value_ptr; // Save the address where the ret val should be written when the joined thread exits.
		activeThread->status = BLOCKED_STATE;
	}
	doNotInterrupt = 0; // Must reset before exiting
	raise(SIGALRM);			// Go to scheduler function

	return 0;
};

/*----------------------------------
 *  initialize the mutex lock
 *----------------------------------*/
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	//initialize data structures for this mutex

	// YOUR CODE HERE

	// Check for main context
	if (!haveMainContext)
	{
		initMainContext();
	}

	doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.  Checking and update should be consistent.
	{
		// Check that mutex is not initialized and not locked.
		if (mutex->initialized || mutex->locked)
		{
			doNotInterrupt = 0; // Must reset before exiting
			return -1;
		}

		myprintf("Thread %d initialized mutex %d.\n", activeThread->tid, mutex->mid);

		mutex->mid = nextMutexId++;
		mutex->initialized = 1; // Must set this flag on init.  Don't allow lock/unlock if not initialized.
		mutex->locked = 0;
		mutex->threadId = -1; //holder represents the tid of the thread that is currently holding the mutex
		mutex->blockedThreads = NULL;
	}
	doNotInterrupt = 0; // Must reset before exiting

	// No need to signal the scheduler for this.
	return 0;
};

/*----------------------------------
 *  aquire the mutex lock
 *----------------------------------*/
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
	// use the built-in test-and-set atomic function to test the mutex
	// if the mutex is acquired successfully, enter the critical section
	// if acquiring mutex fails, push current thread into block list and //
	// context switch to the scheduler thread

	// YOUR CODE HERE

	// Check for main context
	if (!haveMainContext)
	{
		initMainContext();
	}

	doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.  Checking and update should be consistent.
	{
		// Check that mutex is initialized already.  Check that the lock holder is not trying to lock it again.
		if (!mutex->initialized || mutex->threadId == activeThread->tid)
		{
			doNotInterrupt = 0; // Must reset before exiting
			return -1;
		}

		// test_and_set sets the locked flag to 1 before checking its previous value.
		// If it was 0, this thread was the one who updated it.
		// Do this check in a while loop so that if this thread gets blocked, when it finally wakes up, it will resume
		// execution in the loop and try again.  (This is not a spin loop.)
		while (__atomic_test_and_set((volatile void *)&mutex->locked, __ATOMIC_RELAXED))
		{
			// Prev locked flag was already 1, so we didn't lock it.

			myprintf("Thread %d blocked on mutex %d.\n", activeThread->tid, mutex->mid);

			// Block and signal scheduler
			enqueue(&mutex->blockedThreads, activeThread);
			activeThread->status = BLOCKED_STATE;

			doNotInterrupt = 0; // Must reset before exiting
			raise(SIGALRM);

			// Blocked thread will resume at this point, and will continue running the test_and_set check until it
			// is able to lock.
			doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.
		}

		// Got the lock!  The locked flag is already set to 1 by the active thread from the test_and_set.

		myprintf("Thread %d locked mutex %d.\n", activeThread->tid, mutex->mid);

		// Need to double-check that the mutex was not destroyed by the previous locking thread.
		if (!mutex->initialized)
		{
			// It was destroyed.  Unlock and exit with error.
			mutex->locked = 0;

			doNotInterrupt = 0; // Must reset before exiting
			return -1;
		}

		// The lock is good.  Update the locking thread id and continue.
		mutex->threadId = activeThread->tid;
	}
	doNotInterrupt = 0; // Must reset before exiting

	return 0;
}

/*----------------------------------
 *  release the mutex lock
 *----------------------------------*/
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE

	// Check for main context
	if (!haveMainContext)
	{
		initMainContext();
	}

	doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.  Checking and update should be consistent.
	{

		// Check that this thread is locked, and not destroyed, and the activeThread has the lock.
		if (!mutex->locked || !mutex->initialized || mutex->threadId != activeThread->tid)
		{
			doNotInterrupt = 0; // Must reset before exiting
			return -1;
		}

		// Unlock mutex and unset threadId
		mutex->locked = 0;
		mutex->threadId = -1; // Must not match any valid thread id.

		// If there are any threads blocked, then unblock the oldest one and enqueue it on the runQueue with READY status and notify sequencer.
		// Only notify scheduler if any threads were enqueued to the readyQueue.
		if (mutex->blockedThreads != NULL)
		{
			tcb *tcb;
			tcb = dequeue(&mutex->blockedThreads);
			tcb->status = READY_STATE;
			enqueueByQuanta(&readyQueue, tcb);

			myprintf("Thread %d unlocked mutex %d and unblocked thread %d.\n", activeThread->tid, mutex->mid, tcb->tid);

			doNotInterrupt = 0; // Must reset before exiting
			raise(SIGALRM);
		}
		else
		{
			myprintf("Thread %d unlocked mutex %d.  No blocked threads.\n", activeThread->tid, mutex->mid);
		}
	}
	doNotInterrupt = 0; // Must reset before exiting

	return 0;
};

/*----------------------------------
 *  destroy the mutex
 *----------------------------------*/
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	// Deallocate dynamic memory created in mypthread_mutex_init

	// Check for main context
	if (!haveMainContext)
	{
		initMainContext();
	}

	doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.  Checking and update should be consistent.
	{

		// Check that this thread is unlocked by locking it so other threads can't use it while we are destroying it.
		// If test_and_set returns 1, it was locked by some other thread.  Exit with error.
		// Otherwise, it was unlocked, and now we have the lock.
		if (__atomic_test_and_set((volatile void *)&mutex->locked, __ATOMIC_RELAXED))
		{
			doNotInterrupt = 0; // Must reset before exiting
			return -1;
		}

		// Mark as not initialized
		mutex->initialized = 0;
		// Unlock mutex and unset threadId
		mutex->locked = 0;
		mutex->threadId = -1; // Must not match any valid thread id.

		myprintf("Thread %d destroyed mutex %d.\n", activeThread->tid, mutex->mid);

		// Handle any blocked threads.  Only notify scheduler if any threads were enqueued to the readyQueue.
		if (mutex->blockedThreads != NULL)
		{
			// Remove them from the mutex's queue and enqueue them in the readyQueue so the scheduler can manage them again.
			// They will all get an error because they will try to lock the mutext but it is destroyed now.
			tcb *tcb;
			while (mutex->blockedThreads != NULL)
			{
				tcb = dequeue(&mutex->blockedThreads);
				tcb->status = READY_STATE;
				enqueueByQuanta(&readyQueue, tcb);
			}

			doNotInterrupt = 0; // Must reset before exiting
			raise(SIGALRM);
		}
	}
	doNotInterrupt = 0; // Must reset before exiting

	return 0;
};

/*============================================================================
 *
 * Scheduler functions
 * 
 *============================================================================*/

/*----------------------------------
 *  scheduler
 *----------------------------------*/
static void schedule()
{
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// schedule policy
#ifndef MLFQ
	// Choose STCF
	sched_stcf();
#else
	// Choose MLFQ
	sched_mlfq();
#endif
}

/*----------------------------------
 *  Preemptive SJF (STCF) scheduling algorithm
 *----------------------------------*/
static void sched_stcf()
{
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE

	// Check if current thread is in a critical block and should not be interrupted.
	if (doNotInterrupt)
	{
		return; // Returns back to where the current thread was.  See https://stackoverflow.com/questions/37063212/where-does-signal-handler-return-back-to
	}

	//myprintf("Scheduler: Checking: Active Thread %d\n", activeThread->tid);

	// Disable the timer while doing scheduler stuff.
	timerSettings.it_value.tv_sec = 0;
	timerSettings.it_value.tv_usec = 0;
	timerSettings.it_interval.tv_sec = 0;
	timerSettings.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timerSettings, NULL); // Interval and next expiration are all 0.  Disabled.

	// Increment quantaCount for current thread regardless of status.
	// Count of times thread was scheduled to run.  Used as a measure to predict time to completion.  Lowest quantaCount threads run first.
	activeThread->quantaCount++;

	// Flag for if active thread is terminated to check if setContext() should be called instead of swapContext()
	int activeThreadTerminated = 0;

	// Save for logging
	mypthread_t prevThreadId = activeThread->tid;
	int prevThreadStatus = activeThread->status;
	int prevThreadQCount = activeThread->quantaCount;

	// Check the current thread's status and deal with it first.
	switch (activeThread->status)
	{
	// NEW_STATE threads cannot be the active thread because they are not started yet.
	// They are already in the readyQueue.
	case READY_STATE: // Thread was created or yielded or became unblocked
		enqueueByQuanta(&readyQueue, activeThread);
		break;
	case RUNNING_STATE: // Thread was running and got preempted
		activeThread->status = READY_STATE;
		enqueueByQuanta(&readyQueue, activeThread);
		break;
	case BLOCKED_STATE: // Thread is blocked due to join or mutex lock wait.  (Is already in a wait queue.)
		// Don't add to readyQueue.  They are not ready and already in a different queue.
		// (joinQueue of a thread or mutexQueue of a lock.)
		break;
	case TERMINATED_STATE: // Thread was ended
		// Clean up the thread
		free(activeThread->context->uc_stack.ss_sp);
		free(activeThread->context);
		//myprintf("Scheduler: Freeing exiting thread tcb %p\n", activeThread);
		//fflush(stdout);
		free(activeThread);					// Getting seg fault or free() invalid size err here unless I added a buffer to tcb malloc in createThread
		activeThreadTerminated = 1; // Set the flag to check if setContext() should be called instead of swapContext()
		break;
	default:
		// Can't happen.
		exit(-1);
		break;
	}

	// Save current thread
	tcb *prevThread = activeThread;

	// Select the next thread and make it active.  The next thread with the lowest quantaCount will be in the tail of readyQueue.
	activeThread = dequeue(&readyQueue);

	if (activeThread == NULL)
	{
		// If there are no ready threads, the current thread must have exited, and some threads may be blocked.
		// If they are waiting on IO, that's OK.  But if waiting on a mutex, they are probably deadlocked.
		// How can we tell?

		// Don't do anything, but reset the timer to check again later
		myprintf("---Scheduler: No new active thread!");
	}
	else
	{
		activeThread->status = RUNNING_STATE;
		//myprintf("Scheduler: New active thread %d\n", activeThread->tid);
	}

	// Log details on prev thread and next thread, and the readyQueue.
	myprintf("---Scheduler: prev: %d(%d) status:", prevThreadId, prevThreadQCount);
	printThreadStatus(prevThreadStatus);
	myprintf(" -> ");
	if (activeThread)
	{
		myprintf("%d(%d)", activeThread->tid, activeThread->quantaCount);
	}
	else
	{
		myprintf(" -> ");
	}
	myprintf("\n---");
	printReadyQueue(&readyQueue);
	myprintf("\n");

	// Set the timer
	timerSettings.it_value.tv_sec = 0;
	timerSettings.it_value.tv_usec = QUANTA_TIME_USECS;
	timerSettings.it_interval.tv_sec = 0;
	timerSettings.it_interval.tv_usec = 0;
	int ret = setitimer(ITIMER_REAL, &timerSettings, NULL);

	// Here is where the scheduler actually switches from orig active thread to the next thread.
	if (activeThreadTerminated)
	{
		// Just set context to next active thread's context to start it.  Prev thread is terminated.
		setcontext(activeThread->context);

		// NOTE: Originally was calling swapcontext() for all cases, including when orig active thread was terminated.
		// swapcontext() basically calls getcontext() again for the terminated thread, which I think writes the current context
		// into the memory location which was already freed above.
		// I was seeing seg faults or "free() invalid size" err in scheduler, or "double free or corruption" error in dequeue!
		// Temporarily fixed the problem by adding extra 10-1000 bytes when mallocing the thread context on create, but it did not always work.
		// I realized the problem was due to calling swapcontext() when current thread is terminated.
	}
	else
	{
		// Prev active thread is still alive.  Save current context for prev thread (still active) and swap to new active thread.
		swapcontext(prevThread->context, activeThread->context);
	}
	// myprintf("Scheduler: Resuming from swapped thread %d\n", prevThread->tid);
}

/*----------------------------------
 *  Preemptive MLFQ scheduling algorithm
 *----------------------------------*/
static void sched_mlfq()
{
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE

/*============================================================================
 *
 * Init functions
 * 
 *============================================================================*/

/*---------------------------------
 * Get the main thread's context so the scheduler can manage the main thread too.
 *---------------------------------*/
void initMainContext()
{
	doNotInterrupt = 1; // Tell scheduler to ignore timer and not swap this thread while running this block.  Checking and update should be consistent.
	{
		ucontext_t *mainContext = (ucontext_t *)malloc(sizeof(ucontext_t));
		getcontext(mainContext);
		// Don't modify main thread context.  Use existing uc_link and stack settings.  Terminate after exiting main.  No cleanup.

		tcb *mainThread = (tcb *)malloc(sizeof(tcb));
		mainThread->context = mainContext;
		mainThread->tid = nextThreadId++;
		mainThread->status = RUNNING_STATE;
		mainThread->quantaCount = 0;
		mainThread->joinQueue = NULL;		// Tracks threads which this thread is joined on (waiting for them to exit).
		mainThread->retValFromJoin = NULL; // Set by a joined thread on the current thread when the joined thread exits.

		haveMainContext = 1;
		activeThread = mainThread;

		// Set up the schedule() function as the handler timer alarms.
		// If using ITIMER_REAL (wall clock time), threads in sleep() will get interrupted by the alarm and not sleep the whole period.
		// If using ITIMER_VIRTUAL (process time), threads in sleep() will not get interrupted, but scheduler will not run.
		struct sigaction sigSchedule; // use sigaction() instead of signal() for multithreaded environments
		memset(&sigSchedule, 0, sizeof(sigSchedule));
		sigSchedule.sa_handler = &schedule;
		sigaction(SIGALRM, &sigSchedule, NULL);

		// Set up a new global context which finished threads can execute to clean themselves up before stopping.
		// NOTE: Using a global context seems to work OK.  If any memory problems, might try creating a new threadExitContext for each
		// new thread in mypthread_create().
		getcontext(&threadExitContext);
		threadExitContext.uc_link = NULL; // After terminating threads execute mypthread_exit, don't continue.  Stop.
		threadExitContext.uc_stack.ss_sp = malloc(STACK_SIZE);
		threadExitContext.uc_stack.ss_size = STACK_SIZE;
		threadExitContext.uc_stack.ss_flags = 0;
		makecontext(&threadExitContext, (void *)&mypthread_exit, 0);
	}
	doNotInterrupt = 0; // Must reset before exiting
}

/*============================================================================
 *
 * Basic queue functions.
 * Uses circular doubly linked list.
 * 
 *============================================================================*/

/*----------------------------------
 * Queue function: Add to the head, remove from the tail.
 * (End of the list point to the head.)
 *----------------------------------*/
void enqueue(list **head, tcb *tcb)
{
	// Enqueue adds to the head of the list.
	list *currHead = *head;

	// If empty list, create the first node and make it point to itself.
	if (currHead == NULL)
	{
		currHead = (list *)malloc(sizeof(list));
		//myprintf("Enqueuing node for thread %d at %p\n", tcb->tid, currHead);
		currHead->tcb = tcb;
		currHead->next = currHead; // Head of single node list points to itself as the tail.
		currHead->prev = currHead;

		*head = currHead; // Update the external pointer to the linked list used by calling function
		return;
	}

	list *tail = currHead->next; // Get the tail, which is the head's next.
	list *newHead = (list *)malloc(sizeof(list));
	//myprintf("Enqueuing node for thread %d at %p\n", tcb->tid, newHead);
	newHead->tcb = tcb;
	newHead->next = tail; // Make the new node (the new head) point to the tail.
	newHead->prev = currHead;
	currHead->next = newHead; // Then add new head in front of the old head.
	tail->prev = newHead;

	*head = newHead; // Update the external pointer to the linked list used by calling function
	return;
}

/*----------------------------------
 * Queue function: Add to the head, remove from the tail.
 * (End of the list point to the head.)
 *----------------------------------*/
tcb *dequeue(list **head)
{
	// Dequeue removes from the end of the list (tail, which is the next of the head)
	list *currHead = *head;

	// Nothing to dequeue
	if (currHead == NULL)
	{
		return NULL;
	}

	list *tail = currHead->next; // This is the tail.  We are going to remove this node.
	tcb *tcb = tail->tcb;				 // Save the value to be returned

	if (tail == currHead)
	{
		currHead = NULL; // If tail = head, it is the last node.  Null out the list.
	}
	else
	{
		currHead->next = tail->next; // Make head point to the node after the tail.  Then tail is not pointed to at all.  Free it.
		tail->next->prev = currHead;
	}

	//myprintf("Dequeuing node for thread %d at %p\n", tail->tcb->tid, tail);
	free(tail); // Getting double free err here unless I added a buffer to tcb malloc in createThread

	*head = currHead; // Update the external pointer to the linked list used by calling function
	return tcb;
}

/*----------------------------------
 * Queue function: Add to list based on quanta so that lowest quanta are at the tail, ordered by their sequence of insertion.
 * The tail will always be the next thread to be dequeued because it has the lowest quanta and is the oldest.
 * (End of the list point to the head.)
 *----------------------------------*/
void enqueueByQuanta(list **head, tcb *tcb)
{
	// Enqueue adds to the head of the list.
	list *currNode = *head;

	// Create new node to be inserted
	list *newNode = (list *)malloc(sizeof(list));
	//myprintf("Enqueuing by quanta node for thread %d at %p\n", tcb->tid, newNode);
	newNode->tcb = tcb;

	// If empty list, make new node the head and make it point to itself.
	if (currNode == NULL)
	{
		newNode->next = newNode; // Head of single node list points to itself as the tail.
		newNode->prev = newNode;

		*head = newNode; // Update the external pointer to the linked list used by calling function
		return;
	}

	list *tail = (*head)->next; // This is the tail.

	// One element in list
	if (tail == currNode)
	{
		// Make new node and old node point to eachother
		newNode->next = tail;
		newNode->prev = tail;
		tail->next = newNode;
		tail->prev = newNode;
		
		// Decide which of the two nodes is the head based on quantaCount
		if (newNode->tcb->quantaCount >= tail->tcb->quantaCount)
		{
			*head = newNode;
		}
		else
		{
			*head = tail;
		}
		return;
	}

	// 2+ elements
	if (newNode->tcb->quantaCount >= currNode->tcb->quantaCount)
	{ // Start at the head: if newNode quantaCount is >= head quantaCount, make it the new head (Dequeued last)
		newNode->next = currNode->next;
		newNode->prev = currNode;
		currNode->next->prev = newNode;
		currNode->next = newNode;
		*head = newNode;
		return;
	}
	currNode = currNode->next;
	int checkedTail = 0;
	while (currNode != tail || (currNode == tail && !checkedTail))
	{ // Check rest of list
		if (newNode->tcb->quantaCount < currNode->tcb->quantaCount)
		{
			newNode->next = currNode;
			newNode->prev = currNode->prev;
			currNode->prev->next = newNode;
			currNode->prev = newNode;
			return;
		}
		if (currNode == tail)
		{
			checkedTail = 1;
		}
		currNode = currNode->next;
	}
}

/*----------------------------------
 * Queue function: Search list for tcb by it's threadId
 * Does not update the list.
 *----------------------------------*/
tcb *searchTcb(list **head, mypthread_t tid)
{
	list *currNode = *head;

	// Nothing to search
	if (currNode == NULL)
	{
		return NULL;
	}

	list *tail = currNode->next; // This is the tail.

	// One element in list
	if (tail == currNode)
	{
		if (currNode->tcb->tid == tid)
		{
			return currNode->tcb;
		}
		else
		{
			return NULL;
		}
	}

	// 2+ elements
	if (currNode->tcb->tid == tid)
	{ // Check head of list
		return currNode->tcb;
	}
	currNode = currNode->next;
	while (currNode != *head)
	{ // Check rest of list
		if (currNode->tcb->tid == tid)
		{
			return currNode->tcb;
		}
		currNode = currNode->next;
	}
	return NULL;
}

/*----------------------------------
 * Queue function: Search list for tcb by it's threadId and remove it
 *----------------------------------*/
tcb *removeTcb(list **head, mypthread_t tid)
{
	list *currNode = *head;
	tcb *matchingTcb;

	// Nothing to search
	if (currNode == NULL)
	{
		return NULL;
	}

	// One element in list
	if (currNode == currNode->next)
	{
		if (currNode->tcb->tid == tid)
		{
			matchingTcb = currNode->tcb;
			free(currNode);
			*head = NULL;
			return matchingTcb;
		}
		else
		{
			return NULL;
		}
	}

	// 2+ elements
	if (currNode->tcb->tid == tid)
	{ // Check head of list
		matchingTcb = currNode->tcb;
		currNode->prev->next = currNode->next;
		currNode->next->prev = currNode->prev;
		*head = currNode->next;
		free(currNode);
		return matchingTcb;
	}
	currNode = currNode->next;
	while (currNode != *head)
	{ // Check rest of list
		if (currNode->tcb->tid == tid)
		{
			matchingTcb = currNode->tcb;
			currNode->prev->next = currNode->next;
			currNode->next->prev = currNode->prev;
			free(currNode);
			return matchingTcb;
		}
		currNode = currNode->next;
	}
	return NULL;
}

/*----------------------------------
 * Queue function: Print out every tcb in the list with it's quantaCount and tid.
 *----------------------------------*/
void printReadyQueue(list **head)
{
	list *currNode = *head;

	if (currNode == NULL)
	{
		myprintf("Readyqueue: -\n");
		return;
	}

	myprintf("Readyqueue: ");
	int checkedHead = 0;
	while (currNode != *head || (currNode == *head && !checkedHead))
	{
		myprintf("%d(%d) ", currNode->tcb->tid, currNode->tcb->quantaCount);
		if (currNode == *head)
		{
			checkedHead = 1;
		}
		currNode = currNode->prev;
	}
	myprintf("\n");
	return;
}

/*----------------------------------
 * Print the name of the thread status value.
 *----------------------------------*/
void printThreadStatus(int status)
{
	switch (status)
	{
	case READY_STATE: // Thread was created or yielded or became unblocked
		myprintf("[READY  ]");
		break;
	case RUNNING_STATE: // Thread was running and got preempted
		myprintf("[RUNNING]");
		break;
	case BLOCKED_STATE: // Thread is blocked due to join or mutex lock wait.  (Is already in a wait queue.)
		myprintf("[BLOCKED]");
		break;
	case TERMINATED_STATE: // Thread was ended
		myprintf("[TERMINA]");
		break;
	default:
		break;
	}
}
