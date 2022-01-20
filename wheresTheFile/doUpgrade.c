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

#include "doUpgrade.h"
#include "getFromSocket.h"
#include "sendToSocket.h"
#include "directoryFunctions.h"
#include "initGetFromSocket.h"
#include "mutexHandler.h"

void doUpgrade(void *socketPointer)
{
  INIT_GET(socketPointer); // Initialize read from client
  free(tokenBuf);          // Free unused buffer

  FileNode *head = NULL; // Create pointer to empty list

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

  pthread_mutex_t *upgradeLock = getLock(projectDir);
  pthread_mutex_lock(upgradeLock);

  char *versionDir = getVersionDir(projectDir, projectName); // get version subdirectory name
  while (1)                                                  // loop
  {
    bytes_read = recv(clientSocket, &byteBuffer, sizeof(byteBuffer), MSG_PEEK); // peek receive from socket
    if (bytes_read < 0)
    {
      perror("File Read Error\n");
      // close(clientSocket);
      // exit(EXIT_FAILURE);
    }
    if (byteBuffer == '\0') // if peeked null terminator, then there are no more files to read
    {
      // printf("No more files to read.\n");
      clearRecvBuffer(socketPointer);
      break;
    }

    int numBytes = getBytesToRead(socketPointer);                // Get number of bytes in filename from client
    char *filePathClient = getFileName(numBytes, socketPointer); // get filename from client

    char *fileName = getBaseFileName(filePathClient);
    char *filePathServer = getFilePath(versionDir, fileName); // build full filepath from versionDir and filename

    FileNode *aNode = newNode(filePathClient); // Create a new node for that file
    aNode->filePathServer = filePathServer;
    appendNode(&head, aNode); // append that node to the list
  }
  printf("Success! Sending success bytes.\n");
  send(clientSocket, "00", sizeof(char) * 2, 0);
  printf("Sending requested files...\n");
  sendSourceFiles(head, socketPointer); // send all files in list to client
  char nullTerminator[1] = "\0";
  send(clientSocket, nullTerminator, sizeof(nullTerminator), 0);

  pthread_mutex_unlock(upgradeLock);

  // Freeing memory
  freeAllNodes(head);
  free(projectName);
  free(projectDir);
  free(versionDir);
}