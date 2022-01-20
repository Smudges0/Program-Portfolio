#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>

#include "doRollback.h"
#include "initGetFromSocket.h"
#include "getFromSocket.h"
#include "directoryFunctions.h"
#include "mutexHandler.h"

void doRollback(void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;

  char *projectName = getProjectName(socketPointer); // get project name from socket
  char *projectDir = getProjectDir(projectName);     // get project directory name
  printf("Project name: %s. Project directory: %s. Getting lock...\n", projectName, projectDir);

  pthread_mutex_t *rollbackLock = getLock(projectDir);
  pthread_mutex_lock(rollbackLock);
  printf("Locked.\n");
  char *versionNumber = getVersionFromSocket(socketPointer);

  if (!checkDirectory(projectDir))
  {
    // Fail! Project does not exist
    printf("Project does not exist.\n");
    char failureMessage[24] = "01Project doesn't exist";
    send(clientSocket, failureMessage, sizeof(failureMessage), 0);
    free(projectName);
    free(projectDir);
    free(versionNumber);
    pthread_mutex_unlock(rollbackLock);
    return;
  }

  if (checkVersionDirectory(projectDir, versionNumber))
  {
    printf("Version exists. Rolling back...");
    rollback(projectDir, versionNumber);
  }
  else
  {
    // Fail! Version does not exist
    printf("Version does not exist.\n");
    char failureMessage[24] = "01Version doesn't exist";
    send(clientSocket, failureMessage, sizeof(failureMessage), 0);
  }

  // Update history file with rollback
  char *historyPath = getFilePath(projectDir, ".history");
  int historyFD = open(historyPath, O_WRONLY | O_APPEND, S_IRWXU);
  char *rollbackMessage = malloc(sizeof(char) * (strlen("Rollback to version: ") + strlen(versionNumber)) + 2);
  sprintf(rollbackMessage, "Rollback to version: %s\n", versionNumber);
  write(historyFD, rollbackMessage, strlen(rollbackMessage));
  close(historyFD);
  free(rollbackMessage);

  free(projectName);
  free(projectDir);
  free(versionNumber);
  pthread_mutex_unlock(rollbackLock);
  printf("Unlocked.\n");

  printf("Sending two success bytes...\n");
  char successMessage[3] = "00";
  send(clientSocket, successMessage, sizeof(successMessage) + 1, 0);
}