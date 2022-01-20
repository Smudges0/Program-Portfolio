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
#include <math.h>
#include <pthread.h>

#include "doCommit.h"
#include "getFromSocket.h"
#include "sendToSocket.h"
#include "directoryFunctions.h"
#include "fileNode.h"
#include "mutexHandler.h"

void doCommit(void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;

  char *projectName = getProjectName(socketPointer);
  char *projectDir = getProjectDir(projectName);

  if (!checkDirectory(projectDir))
  {
    // Fail! Project must not exist.
    printf("Project doesn't exist\n.");
    char failureMessage[24] = "01Project doesn't exist";
    send(clientSocket, failureMessage, sizeof(failureMessage), 0);
    free(projectName);
    free(projectDir);
    return;
  }

  pthread_mutex_t *commitLock = getLock(projectDir);
  pthread_mutex_lock(commitLock);

  char *versionDir = getVersionDir(projectDir, projectName);
  char *manifestPath = getFilePath(versionDir, ".manifest");

  FileNode *manifestNode = newNode(getFilePathClient(projectName, ".manifest"));
  manifestNode->filePathServer = manifestPath;

  printf("Sending manifest.\n");
  sendSpecialFile(manifestNode, socketPointer);
  printf("Waiting for .commit file...\n");
  char *commitPath = getFilePath(projectDir, ".commitPending");
  printf("Commit path: %s\n", commitPath);
  int commitNum = pendingCommitNum(projectDir);
  char commitNumString[(int)log10(commitNum) + 2]; // string to hold int
  sprintf(commitNumString, "%d", commitNum);
  printf("Commit number: %s\n", commitNumString);
  char *pendingCommitPath = malloc(sizeof(char) * (strlen(commitPath) + strlen(commitNumString)) + 1);
  sprintf(pendingCommitPath, "%s%s", commitPath, commitNumString);
  printf("Full commit path: %s\n", pendingCommitPath);

  printf("Getting commit file...\n");
  getSpecialFile(pendingCommitPath, socketPointer);
  clearRecvBuffer(socketPointer);

  free(pendingCommitPath);
  pthread_mutex_unlock(commitLock);

  printf("Sending two success bytes...\n");
  char successMessage[3] = "00";
  send(clientSocket, successMessage, sizeof(successMessage) + 1, 0);
}