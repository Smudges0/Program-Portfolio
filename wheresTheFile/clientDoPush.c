#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <openssl/sha.h>
#include <math.h>

#include "clientDoPush.h"
#include "errorCheck.h"
#include "fileNode.h"
#include "directoryFunctions.h"
#include "readSpecialFile.h"
#include "getFromSocket.h"
#include "sendToSocket.h"

int doPushClient(void *socketPointer, char *projectName) // Returns 1 for success, and 0 for failure.
{
  char *projectDir = getProjectDir(projectName);

  int clientSocket = *(int *)socketPointer;            // convert void * socketPointer to int clientPointer
  char pushBytes[2] = "01";                            // Push command
  send(clientSocket, pushBytes, sizeof(pushBytes), 0); // Send push command to server

  char *commitPath = getFilePath(projectDir, ".commit"); // I assume the commit file is called .commit. Change if necessary

  struct stat stats;
  stat(commitPath, &stats);

  if (!stats.st_mode & F_OK)
  {
    printf(".commit file does not exist!\n"); // Print out an error message. You can make this whatever you want.
    return 0;
  }

  int len = strlen(projectName);
  char nameLength[(int)log10(len) + 2];
  sprintf(nameLength, "%d:", len);
  send(clientSocket, nameLength, strlen(nameLength), 0);
  send(clientSocket, projectName, strlen(projectName), 0); // Send project name to server

  FileNode *listHead = NULL;                  // Pointer to head of list
  FileNode *commitNode = newNode(commitPath); // Create a node to hold commit file data

  sendSpecialFile(commitNode, socketPointer); // Send the .commit file to the server

  listHead = readCommit(commitNode); // Fill list with file data from .commit

  FileNode *aNode = listHead; // Pointer to a node. Set it to head of list at first.
  while (aNode)               // Go through every node in the list
  {
    if (!strcmp(aNode->op, "D")) // If the file is going to be deleted
    {
      continue; // Ignore this file and don't send it
    }
    else // Otherwise, it is a Modify or Add
    {
      sendFilePath(aNode, socketPointer); // send file name size AND name
      sendNumBytes(aNode, socketPointer); // send number of bytes in file
      sendFileData(aNode, socketPointer); // send file data
    }
    aNode = aNode->next; // Move on to next node in list
  }
  send(clientSocket, "\0", sizeof(char), 0); // Send null terminator to signify end

  // Handle response from server. If you already do this somewhere else, you can delete this.
  char statusBytes[3]; // Hold 00 or 01

  int bytes_read = recv(clientSocket, statusBytes, sizeof(statusBytes) - 1, 0); // Receive two bytes from the server.
  statusBytes[2] = '\0';
  if (bytes_read < 0)
  {
    perror("Error receiving from server: ");
  }

  if (!strcmp(statusBytes, "00")) // If success
  {
    clearRecvBuffer(socketPointer);                            // Get rid of extra null terminator
    char *manifestPath = getFilePath(projectDir, ".manifest"); // Get path to manifest file
    FileNode *manifestNode = newNode(manifestPath);            // Create manifest node
    FileNode *manifestList = readManifest(manifestNode);       // Read manifest file and make list of information inside
    updateManifestPush(manifestList, listHead, projectDir);    // Update manifest file with latest information from commit file.
  }
  else // Get rest of error message
  {
    char errorBuf[256]; // Buffer to hold error message
    while (1)
    {
      bytes_read = recv(clientSocket, errorBuf, sizeof(errorBuf), 0); // Read from socket
      if (bytes_read < 0)
      {
        perror("Error receiving from server: ");
        break;
      }

      if (errorBuf[bytes_read - 1] == '\0') // If ended on null terminator, then message is over
      {
        printf("%s\n", errorBuf); // Print out error message                                               // Failure
      }
      printf("%s\n", errorBuf); // Print out partial error message
    }
  }
  unlink(commitPath); // Delete .commit file
}

void updateManifestPush(FileNode *manifestHead, FileNode *commitHead, char *versionDir)
{
  // Update manifest project version number
  int newProjectVersion = commitHead->projectVersion;

  FileNode *aManifestNode = NULL;
  FileNode *aCommitNode = commitHead;
  FileNode *prevNode;

  int foundFile;
  while (aCommitNode) // Go over each commit node
  {
    foundFile = 0;                // Set foundFile to false
    prevNode = NULL;              // Set prevNode to NULL
    aManifestNode = manifestHead; // Go to start of manifest node list
    while (aManifestNode)         // go over each manifest node
    {
      if (!strcmp(aManifestNode->filePathClient, aCommitNode->filePathClient)) // If file is in manifest and commit node
      {
        // File was added or modified, update manifest with hash and file/project version
        foundFile = 1;
        char *hash = malloc(sizeof(char) * (strlen(aCommitNode->hash)));
        strcpy(hash, aCommitNode->hash);
        aManifestNode->hash = hash;                            // change hash value in manifest
        aManifestNode->fileVersion = aCommitNode->fileVersion; // change file version in manifest
        aManifestNode->projectVersion = newProjectVersion;     // change project version in manifest
        break;                                                 // move on to next commit node
      }
      prevNode = aManifestNode;
      aManifestNode = aManifestNode->next; // if files don't match, move on to next node.
    }
    aCommitNode = aCommitNode->next; // Go to next commit node
  }
  // Write new manifest
  writeManifestPush(manifestHead, versionDir);
}

void writeManifestPush(FileNode *manifestHead, char *versionDir)
{
  char *manifestPath = getFilePath(versionDir, ".manifest");

  int fd = open(manifestPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if (fd == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }

  char *space = " ";
  char *newLine = "\n";

  int len = 1;
  if (manifestHead->projectVersion != 0)
  {
    len = (int)log10(manifestHead->projectVersion) + 1;
  }
  char projectVersion[len + 1];
  sprintf(projectVersion, "%d", manifestHead->projectVersion);

  write(fd, projectVersion, strlen(projectVersion)); // Write project version to top of manifest
  write(fd, newLine, sizeof(char));                  // write one newline

  FileNode *aNode = manifestHead;
  while (aNode)
  {
    int len = 1;
    if (aNode->fileVersion != 0)
    {
      len = (int)log10(aNode->fileVersion) + 1;
    }
    char fileVersion[len + 1];
    sprintf(fileVersion, "%d", aNode->fileVersion);
    write(fd, fileVersion, strlen(fileVersion));

    write(fd, space, sizeof(char));
    write(fd, aNode->filePathClient, strlen(aNode->filePathClient));
    write(fd, space, sizeof(char));

    write(fd, aNode->hash, strlen(aNode->hash));
    write(fd, newLine, sizeof(char));

    aNode = aNode->next;
  }
  close(fd);
}