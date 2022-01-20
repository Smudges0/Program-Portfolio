#ifndef MUTEX_HANDLER
#define MUTEX_HANDLER

#include <pthread.h>

typedef struct mutexNode
{
  char *directoryPath;
  pthread_mutex_t *lock;
  struct mutexNode *next;
} MutexNode;

MutexNode *newMutexNode(char *directoryPath);
pthread_mutex_t *getLock(char *directoryPath);
#endif