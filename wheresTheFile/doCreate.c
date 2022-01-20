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

#include "getFromSocket.h"
#include "sendToSocket.h"
#include "fileNode.h"
#include "directoryFunctions.h"
#include "mutexHandler.h"

void doCreate(void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;
  char *projectName = getProjectName(socketPointer); // get project name from socket
  // char *projectDirName = malloc(sizeof(char) * strlen(projectName) + 3); // malloc space for directory
  // sprintf(projectDirName, "./%s", projectName);                          // create project directory name
  char *projectDirName = getProjectDir(projectName);
  clearRecvBuffer(socketPointer);

  //check if directory exists
  // struct stat pathStat;
  // stat(projectDirName, &pathStat);
  // if (S_ISDIR(pathStat.st_mode))
  // {
  //   // Fail! Project must not exist.
  //   printf("Project already exists\n.");
  //   char failureMessage[24] = "01Project already exists";
  //   send(clientSocket, failureMessage, sizeof(failureMessage), 0);
  // }
  if (checkDirectory(projectDirName))
  {
    // Fail! Project must not exist.
    printf("Project already exists\n.");
    char failureMessage[25] = "01Project already exists";
    send(clientSocket, failureMessage, sizeof(failureMessage), 0);
  }
  else
  {
    // If no directory is found, create new directory and manifest.
    // char *versionDirName = malloc(sizeof(char) * (strlen(projectDirName) + strlen(projectName)) + 4); // malloc space for version subdirectory
    // sprintf(versionDirName, "%s/%sv1", projectDirName, projectName);                                  // create name of version subdirectory

    pthread_mutex_t *doCreateLock = getLock(projectDirName);
    pthread_mutex_lock(doCreateLock);

    mkdir(projectDirName, S_IRWXU); // create project directory
    opendir(projectDirName);        // move into project directory
    char *versionDirName = getVersionDir(projectDirName, projectName);
    mkdir(versionDirName, S_IRWXU); // create version subdirectory
    opendir(versionDirName);        // move into version subdirectory
    // char *manifestPath = malloc(sizeof(char) * strlen(versionDirName) + 11);                          // malloc space for .manifest file path
    // sprintf(manifestPath, "%s/.manifest", versionDirName);                                            // create .manifest file path
    char *manifestPath = getFilePath(versionDirName, ".manifest");
    // create manifest
    int manifest = open(manifestPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    if (manifest == -1)
    {
      perror("File write error");
      close(manifest);
      pthread_mutex_unlock(doCreateLock);
      exit(EXIT_FAILURE);
    }

    char *versionNumber = "0\n";                           // version data for new .manifest file
    write(manifest, versionNumber, strlen(versionNumber)); // write default data to .manifest
    close(manifest);

    FileNode *head = NULL;                                                         // create head of list
    FileNode *manifestNode = newNode(getFilePathClient(projectName, ".manifest")); // create new node for .manifest file
    manifestNode->filePathServer = manifestPath;
    manifestNode->fileVersion = 0;    // add file version
    manifestNode->projectVersion = 0; // add project version
    appendNode(&head, manifestNode);  // add node to list

    // send manifest to client
    printf("Sending two success bytes...\n");
    char successMessage[2] = "00";
    send(clientSocket, successMessage, sizeof(successMessage), 0);
    printf("Sending .manifest...\n");
    sendSpecialFile(manifestNode, socketPointer);
    char nullTerminator[1] = "\0";
    send(clientSocket, nullTerminator, sizeof(nullTerminator), 0);

    pthread_mutex_unlock(doCreateLock);

    free(versionDirName);
    freeAllNodes(head);
  }
  // create history file for project
  char *historyPath = getFilePath(projectDirName, ".history");
  int history = open(historyPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  char *initHistory = "0\n";
  write(history, initHistory, strlen(initHistory));
  close(history);

  free(projectName);
  free(projectDirName);
}