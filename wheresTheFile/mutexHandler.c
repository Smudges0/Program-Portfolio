#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "mutexHandler.h"

extern MutexNode *mutexHead;
extern pthread_mutex_t mutexHandlerLock;

MutexNode *newMutexNode(char *directoryPath)
{
  MutexNode *aMutexNode = (MutexNode *)malloc(sizeof(MutexNode));

  aMutexNode->directoryPath = malloc(strlen(directoryPath) + 1);
  strcpy(aMutexNode->directoryPath, directoryPath);
  aMutexNode->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(aMutexNode->lock, NULL);

  printf("Created mutex.\n");
  return aMutexNode;
}

pthread_mutex_t *getLock(char *directoryPath)
{
  pthread_mutex_lock(&mutexHandlerLock);
  printf("Locked mutex handler.\n");

  MutexNode *aNode = mutexHead;
  MutexNode *prevNode = NULL;

  while (aNode)
  {
    printf("Node path: %s. Given path: %s\n", aNode->directoryPath, directoryPath);
    if (!strcmp(aNode->directoryPath, directoryPath))
    {
      printf("Found mutex for: %s\n", directoryPath);
      break;
    }
    else
    {
      printf("Not found, moving to next node...\n");
      prevNode = aNode;
      aNode = aNode->next;
    }
  }
  if (!aNode)
  {
    printf("Node is null.\n");
    if (prevNode)
    {
      printf("List is populated. Appending node to list...\n");
      prevNode->next = newMutexNode(directoryPath);
    }
    else
    {
      printf("List is empty. Making new node head of list...\n");
      aNode = newMutexNode(directoryPath);
      mutexHead = aNode;
    }
  }

  pthread_mutex_unlock(&mutexHandlerLock);
  printf("Unlocked mutex handler.\n");
  return aNode->lock;
}