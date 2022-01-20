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
#include <math.h>
// #include <openssl/sha.h>

#include "clientDoUpgrade.h"
#include "errorCheck.h"
#include "fileNode.h"
#include "directoryFunctions.h"
#include "readSpecialFile.h"
#include "getFromSocket.h"
#include "sendToSocket.h"

void doUpgrade(void *socketPointer, char *projectName)
{
  char *projectDir = getProjectDir(projectName);

  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(projectDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

  int foundUpdate = 0;
  char *updatePath = NULL;
  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }

    if (!strcmp(directoryEntry->d_name, ".conflict")) // if found .conflict file, error
    {
      printf("Found .conflict file. Please resolve conflicts, then update.\n");
      return;
    }
    else if (!strcmp(directoryEntry->d_name, ".update")) // If found update file
    {
      foundUpdate = 1;                                                                                                // Flag foundUpdate
      updatePath = malloc((sizeof(char) * strlen(projectDir)) + (sizeof(char) * strlen(directoryEntry->d_name)) + 2); // Create path for update file
      sprintf(updatePath, "%s/%s", projectDir, directoryEntry->d_name);
    }
  }

  if (foundUpdate)
  {
    // Update .manifest
    FileNode *updateNode = newNode(updatePath);    // Create a node for the update file
    FileNode *updateList = readCommit(updateNode); // The update file is formated EXACTLY the same as commit file, so we can use same function to read both

    char *manifestPath = getFilePath(projectDir, ".manifest"); // Get file path for manifest file
    FileNode *manifestNode = newNode(manifestPath);            // Create node for manifest file
    FileNode *manifestList = readManifest(manifestNode);       // Create list of files in .manifest

    // Update manifest with data from update and request files from server
    int clientSocket = *(int *)socketPointer;

    char upgradeBytes[2] = "03";                               // Upgrade command
    send(clientSocket, upgradeBytes, sizeof(upgradeBytes), 0); // Send upgrade command to server
    send(clientSocket, projectName, strlen(projectName), 0);   // Send project name to server
    updateManifestUpgrade(manifestList, updateList, projectDir, socketPointer);

    int bytes_read;
    char statusBytes[3];
    bytes_read = recv(clientSocket, statusBytes, sizeof(statusBytes) - 1, 0);
    statusBytes[2] = '\0';

    if (!strcmp(statusBytes, "00"))
    {
      // Write new manifest
      writeManifestUpgrade(manifestList, projectDir);

      // Receive requested files and write them to project
      getSourceFiles(socketPointer, projectDir); // Receive Modified and Added files from server
    }
    else // Get rest of error message
    {
      char errorBuf[256]; // Buffer to hold error message
      while (1)
      {
        bytes_read = recv(clientSocket, errorBuf, sizeof(errorBuf), 0); // Read from socket
        if (bytes_read < 0)
        {
          perror("Error receiving from server.\n");
          break;
        }

        if (errorBuf[bytes_read - 1] == '\0') // If ended on null terminator, then message is over
        {
          printf("%s\n", errorBuf); // Print out error message
        }
        printf("%s\n", errorBuf); // Print out partial error message
      }
    }
    free(manifestNode);
    free(manifestList);
    free(updateNode);
    free(updateList);
  }
  else // No update file found
  {
    printf("Couldn't find update file. Please update project.\n");
  }

  return;
}

void updateManifestUpgrade(FileNode *manifestList, FileNode *updateList, char *projectDir, void *socketPointer)
{
  int newProjectVersion = updateList->projectVersion; // Update manifest project version number

  FileNode *aManifestNode = NULL;        // Pointer to a node in manifest list
  FileNode *anUpdateNode = manifestList; // Pointer to a node in update list
  FileNode *prevNode;                    // Pointer to previous node / temp storage
  int foundFile;                         // found file flag
  while (anUpdateNode)                   // Go over each update node
  {
    foundFile = 0;                // Set foundFile to false
    prevNode = NULL;              // Set prevNode to NULL
    aManifestNode = manifestList; // Go to start of manifest node list
    while (aManifestNode)         // go over each manifest node
    {
      if (!strcmp(aManifestNode->filePathClient, anUpdateNode->filePathClient)) // If file is in manifest and commit node
      {
        foundFile = 1;                      // Set foundFile
        if (!strcmp(anUpdateNode->op, "M")) // if file is modified
        {
          char *hash = malloc(sizeof(char) * (strlen(anUpdateNode->hash)));
          strcpy(hash, anUpdateNode->hash);
          aManifestNode->hash = hash;                             // change hash value in manifest
          aManifestNode->fileVersion = anUpdateNode->fileVersion; // change file version in manifest
          aManifestNode->projectVersion = newProjectVersion;      // change project version in manifest

          sendFilePath(aManifestNode, socketPointer); // Request file from server by sending it's file path
          break;                                      // move on to next commit node
        }
        else // file is deleted
        {
          if (!prevNode) // if node is first in list
          {
            prevNode = aManifestNode;           // store current node, which is the head of the list
            manifestList = aManifestNode->next; // make head point to the next node
            // Free prevnode;
            prevNode->next = NULL;  // break link between node and next node
            freeAllNodes(prevNode); // free that node
          }
          else // node is in the middle of the list
          {
            prevNode->next = aManifestNode->next; // connect previous node to the next node
            aManifestNode->next = NULL;           // break link between current node and next node
            free(aManifestNode);                  // free current node
          }

          // Delete file from folder
          unlink(aManifestNode->filePathClient); // Remove file at this file path
          break;                                 // move on to next commit node
        }
      }
      prevNode = aManifestNode;            // store last node
      aManifestNode = aManifestNode->next; // if files don't match, move on to next node.
    }
    if (foundFile) // If we found the file, move on to next node in update file
    {
      anUpdateNode = anUpdateNode->next;
    }
    else // File was added
    {
      sendFilePath(anUpdateNode, socketPointer); // Request file from server by sending it's file path
      prevNode = anUpdateNode->next;             // store next pointer
      anUpdateNode->next = NULL;                 // unlink node from list
      appendNode(&manifestList, anUpdateNode);   // add update node to the manifest list
      anUpdateNode = prevNode;                   // re-link update list
    }
  }
}

void writeManifestUpgrade(FileNode *manifestList, char *projectDir)
{
  char *manifestPath = getFilePath(projectDir, ".manifest");

  int fd = open(manifestPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); // Open file to write
  if (fd == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }

  char *space = " ";
  char *newLine = "\n";

  int len = 1;
  if (manifestList->projectVersion != 0)
  {
    len = (int)log10(manifestList->projectVersion)+1;
  }
  char projectVersion[len + 1];
  sprintf(projectVersion, "%d", manifestList->projectVersion);

  write(fd, projectVersion, strlen(projectVersion)); // Write project version to top of manifest
  write(fd, newLine, sizeof(char));                  // write one newline

  FileNode *aNode = manifestList;
  while (aNode) // Loop through manifest list
  {
    int len = 1;
    if (manifestList->fileVersion != 0)
    {
      len = (int)log10(manifestList->fileVersion)+1;
    }
    char fileVersion[len + 1];
    sprintf(fileVersion, "%d", manifestList->fileVersion);
    write(fd, fileVersion, strlen(fileVersion));

    write(fd, space, sizeof(char));                                  // space
    write(fd, aNode->filePathClient, strlen(aNode->filePathClient)); // Write file path
    write(fd, space, sizeof(char));                                  // space

    write(fd, aNode->hash, strlen(aNode->hash)); // Write hash
    write(fd, newLine, sizeof(char));            // write newline

    aNode = aNode->next; // go to next node
  }
  close(fd); // finish writing manifest
}