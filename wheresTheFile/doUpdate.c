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

#include "doUpdate.h"
#include "getFromSocket.h"
#include "sendToSocket.h"
#include "directoryFunctions.h"
#include "mutexHandler.h"

void doUpdate(void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;

  char *projectName = getProjectName(socketPointer); // get project name from socket
  char *projectDir = getProjectDir(projectName);     // get project directory name

  printf("Checking if project exists...\n");
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

  pthread_mutex_t *updateLock = getLock(projectDir);
  pthread_mutex_lock(updateLock);

  printf("Project exists!\n");
  char *versionDir = getVersionDir(projectDir, projectName); // Get latest version directory name
  printf("Latest project version: %s\n", versionDir);
  char *manifestPath = getFilePath(versionDir, ".manifest");                     // Get path of .manifest file
  FileNode *manifestNode = newNode(getFilePathClient(projectName, ".manifest")); // Create file node for .manifest
  manifestNode->filePathServer = manifestPath;

  printf("Success... Sending success bytes.\n");
  send(clientSocket, "00", sizeof(char) * 2, 0);
  printf("Sending manifest...\n");
  sendSpecialFile(manifestNode, socketPointer); // Send .manifest using manifest fileNode

  pthread_mutex_unlock(updateLock);

  freeAllNodes(manifestNode);
  free(projectName);
  free(projectDir);
  free(versionDir);
}