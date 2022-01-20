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
#include <dirent.h>

#include "doHistory.h"
#include "getFromSocket.h"
#include "sendToSocket.h"
#include "directoryFunctions.h"
#include "fileNode.h"
#include "mutexHandler.h"

void doHistory(void *socketPointer)
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

  pthread_mutex_t *historyLock = getLock(projectDir);
  pthread_mutex_lock(historyLock);

  char *historyFilePath = getFilePath(projectDir, ".history");
  FileNode *historyFile = newNode(getFilePathClient(projectName, ".history"));
  historyFile->filePathServer = historyFilePath;

  printf("Success... Sending success bytes.\n");
  send(clientSocket, "00", sizeof(char) * 2, 0);
  printf("Sending manifest...\n");
  sendSpecialFile(historyFile, socketPointer); // Send .manifest using manifest fileNode

  pthread_mutex_unlock(historyLock);

  freeAllNodes(historyFile);
  free(projectName);
  free(projectDir);
}
