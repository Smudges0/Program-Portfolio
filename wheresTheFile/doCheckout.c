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

#include "doCheckout.h"
#include "getFromSocket.h"
#include "sendToSocket.h"
#include "directoryFunctions.h"
#include "readSpecialFile.h"
#include "mutexHandler.h"

void doCheckout(void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;

  char *projectName = getProjectName(socketPointer); // Get project name from client message
  char *projectDir = getProjectDir(projectName);     // Get project directory from project name
  clearRecvBuffer(socketPointer);

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

  pthread_mutex_t *checkoutLock = getLock(projectDir);
  pthread_mutex_lock(checkoutLock);

  printf("Project exists!\n");
  char *versionDir = getVersionDir(projectDir, projectName); // Get latest version directory name
  printf("Latest project version: %s\n", versionDir);
  char *manifestPath = getFilePath(versionDir, ".manifest");                     // Get path of .manifest file
  FileNode *manifestNode = newNode(getFilePathClient(projectName, ".manifest")); // Create file node for .manifest
  manifestNode->filePathServer = manifestPath;

  FileNode *head = NULL;             // Pointer to head of list of fileNodes
  head = readManifest(manifestNode); // Read manifest file and create list of FileNodes for each file listed inside
  if (!head)
  {
    printf("Head is null, No source files to send.\n");
    freeAllNodes(manifestNode);
    free(projectName);
    free(projectDir);
    free(versionDir);
    pthread_mutex_unlock(checkoutLock);
    return;
  }
  if (head->error == 1)
  {
    printf("%s\n", head->filePathClient);
    send(clientSocket, head->filePathClient, strlen(head->filePathClient) + 1, 0);
    freeAllNodes(manifestNode);
    freeAllNodes(head);
    free(projectName);
    free(projectDir);
    free(versionDir);
    pthread_mutex_unlock(checkoutLock);
    return;
  }

  // Edit filepath
  setFilePathServer(head, versionDir);

  printf("Success... Sending success bytes.\n");
  send(clientSocket, "00", sizeof(char) * 2, 0);
  printf("Sending manifest...\n");
  sendSpecialFile(manifestNode, socketPointer); // Send .manifest using manifest fileNode
  printf("Sent manifest. Creating FileNode list...\n");
  printf("Sending source files...\n");
  sendSourceFiles(head, socketPointer); // Send all source files in the list of FileNodes
  char nullTerminator[1] = "\0";
  send(clientSocket, nullTerminator, sizeof(nullTerminator), 0);

  pthread_mutex_unlock(checkoutLock);
  freeAllNodes(manifestNode);
  freeAllNodes(head);
  free(projectName);
  free(projectDir);
  free(versionDir);
}