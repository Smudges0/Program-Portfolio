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

#include "doDestroy.h"
#include "getFromSocket.h"
#include "directoryFunctions.h"
#include "mutexHandler.h"

void doDestroy(void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;

  char *projectName = getProjectName(socketPointer);
  char *projectDir = getProjectDir(projectName);
  clearRecvBuffer(socketPointer);

  if (!checkDirectory(projectDir))
  {
    // Fail! Project does not exist
    printf("Project does not exist.\n");
    char failureMessage[24] = "01Project doesn't exist";
    send(clientSocket, failureMessage, sizeof(failureMessage), 0);
    free(projectName);
    free(projectDir);
    return;
  }

  pthread_mutex_t *destroyLock = getLock(projectDir);
  pthread_mutex_lock(destroyLock);
  deleteDirectory(projectDir);
  pthread_mutex_unlock(destroyLock);

  printf("Sending two success bytes...\n");
  char successMessage[3] = "00";
  send(clientSocket, successMessage, sizeof(successMessage), 0);

  free(projectName);
  free(projectDir);
}