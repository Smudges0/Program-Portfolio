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
#include <math.h>

#include "doPush.h"
#include "getFromSocket.h"
#include "sendToSocket.h"
#include "readSpecialFile.h"
#include "directoryFunctions.h"
#include "mutexHandler.h"

void doPush(void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;

  char *projectName = getProjectName(socketPointer); // Get project name from client
  char *projectDir = getProjectDir(projectName);     // make project directory name from project name

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

  pthread_mutex_t *pushLock = getLock(projectDir);
  pthread_mutex_lock(pushLock); // Lock directory

  char *pushCommitPath = getFilePath(projectDir, ".pushcommit"); // get filepath for pushcommit
  getSpecialFile(pushCommitPath, socketPointer);                 // get commit file from client and save as .pushcommit

  // diff files
  if (diffFiles(projectDir, pushCommitPath)) // Check for a .commitPending file that is the same as our .pushcommit file
  {
    expirePending(projectDir); // Remove all .commitPending files

    char *oldVersionDir = getVersionDir(projectDir, projectName); // Get filepath of previous version
    char *manifestPath = getFilePath(oldVersionDir, ".manifest"); // get manifest path of previous version

    char *currentVersion = findLatestVersion(projectDir); // Get number of previous version
    int currentVersionInt = atoi(currentVersion + 1);     // Convert to integer
    currentVersionInt += 1;                               // Increment by one
    char newVersion[(int)log10(currentVersionInt) + 2];   // string to hold int
    sprintf(newVersion, "%d", currentVersionInt);         //
    // free(currentVersion); // Cannot free because it was not created with malloc

    char *newVersionDir = malloc(sizeof(char) * (strlen(projectDir) + strlen(projectName) + strlen(newVersion)) + 3); // Create new version directory path
    sprintf(newVersionDir, "%s/%sv%s", projectDir, projectName, newVersion);

    mkdir(newVersionDir, S_IRWXU); // create new version directory

    copyFiles(oldVersionDir, newVersionDir); // copy contents of previous version into new version

    // Read Manifest and commit file
    FileNode *manifestList = NULL;
    FileNode *commitList = NULL;

    FileNode *manifestNode = newNode(getFilePathClient(projectName, ".manifest")); // create manifest node
    manifestNode->filePathServer = manifestPath;                                   // insert filepath of manifest
    FileNode *commitNode = newNode(getFilePathClient(projectName, ".pushcommit")); // create commit node
    commitNode->filePathServer = pushCommitPath;                                   // insert filepath of commit

    manifestList = readManifest(manifestNode); // Create list of files listed in manifest
    commitList = readCommit(commitNode);       // Create list of files listed in commit

    setFilePathServer(manifestList, newVersionDir);
    setFilePathServer(commitList, newVersionDir);

    updateManifest(manifestList, commitList, newVersionDir); // Compare lists and update manifest accordingly
    // This will also delete files as necessary

    getSourceFiles(socketPointer, newVersionDir); // Get modified and new files from client. This will create subdirectories as necessary

    char *newCommitPath = getFilePath(newVersionDir, ".commit"); // new file path and name for .pushcommit
    duplicate(pushCommitPath, newCommitPath);                    // duplicate .pushcommit into .commit

    free(newVersionDir);

    updateHistory(projectDir, pushCommitPath);
    printf("Sending two success bytes...\n");
    char successMessage[3] = "00";
    send(clientSocket, successMessage, sizeof(successMessage), 0);
  }
  else // matching commit file not found
  {
    // send error message
    clearRecvBuffer(socketPointer);
    printf("Matching .commitPending does not exist.\n");
    char failureMessage[29] = "01No matching pending commit";
    send(clientSocket, failureMessage, sizeof(failureMessage), 0);
  }

  unlink(pushCommitPath); // delete pushcommit on success or failure
  pthread_mutex_unlock(pushLock);
}

int diffFiles(char *projectDir, char *filePath)
{
  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(projectDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

  int foundMatch = 0;

  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }

    if (!strstr(directoryEntry->d_name, ".commitPending")) // if directory doesn't contain '.commitPending', skip
    {
      continue;
    }

    char *newPathName = malloc((sizeof(char) * strlen(projectDir)) + (sizeof(char) * strlen(directoryEntry->d_name)) + 2); // Create path for entry
    sprintf(newPathName, "%s/%s", projectDir, directoryEntry->d_name);

    if (checkDirectory(newPathName)) // if entry is a directory, skip
    {
      continue;
    }

    if (compareFiles(newPathName, filePath)) // If files are the same
    {
      foundMatch = 1; // update foundmatch
    }
    free(newPathName); // Free newPathName in either case
    if (foundMatch)    // if found a match
    {
      break;
    }
  }
  return foundMatch;
}

int compareFiles(char *pending, char *received)
{
  int pendingFileDesc = open(pending, O_RDONLY, S_IRWXU);
  int receivedFileDesc = open(received, O_RDONLY, S_IRWXU);
  if (pendingFileDesc == -1 || receivedFileDesc == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }

  char readBufferPending[4096];
  char readBufferReceived[4096];
  int bytes_read_pending;
  int bytes_read_received;

  int isSame = 1;

  while (1)
  {
    bytes_read_pending = read(pendingFileDesc, readBufferPending, sizeof(readBufferPending));     // read from pending commit
    bytes_read_received = read(receivedFileDesc, readBufferReceived, sizeof(readBufferReceived)); // read from pushcommit

    if (bytes_read_pending < 0 || bytes_read_received < 0)
    {
      perror("File Read Error");
      close(pendingFileDesc);
      close(receivedFileDesc);
      exit(EXIT_FAILURE);
    }
    else if (bytes_read_pending == 0 || bytes_read_received == 0) // if we didn't read any bytes
    {
      // send(clientSocket, '\0', 1, 0); // send null terminator and break
      break;
    }
    // appending null terminators
    if (bytes_read_pending < sizeof(readBufferPending))
    {
      readBufferPending[bytes_read_pending] = '\0';
    }
    if (bytes_read_received < sizeof(readBufferReceived))
    {
      readBufferReceived[bytes_read_received] = '\0';
    }

    if (strcmp(readBufferPending, readBufferReceived)) // If buffers are different
    {
      isSame = 0; // Files are not the same
      break;
    }
  }
  close(pendingFileDesc);
  close(receivedFileDesc);
  return isSame;
}

void expirePending(char *projectDir)
{
  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(projectDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }
    char *newPathName = malloc((sizeof(char) * strlen(projectDir)) + (sizeof(char) * strlen(directoryEntry->d_name)) + 2); // Create new path name
    sprintf(newPathName, "%s/%s", projectDir, directoryEntry->d_name);

    if (checkDirectory(newPathName)) // if entry is a directory, skip
    {
      free(newPathName);
      continue;
    }

    if (strstr(directoryEntry->d_name, ".commitPending"))
    {
      unlink(newPathName);
      free(newPathName);
    }
  }
}

void copyFiles(char *sourceDir, char *targetDir)
{
  DIR *directory;                        // create directory
  struct dirent *directoryEntry;         // create directory entry
  if (!(directory = opendir(sourceDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }

    char *sourceFilePath = malloc((sizeof(char) * strlen(sourceDir)) + (sizeof(char) * strlen(directoryEntry->d_name)) + 2); // create name for entry
    sprintf(sourceFilePath, "%s/%s", sourceDir, directoryEntry->d_name);

    char *targetFilePath = malloc(sizeof(char) * (strlen(targetDir) + strlen(directoryEntry->d_name)) + 2); // corresponding name for new version folder
    sprintf(targetFilePath, "%s/%s", targetDir, directoryEntry->d_name);

    if (checkDirectory(sourceFilePath)) // If entry is a directory
    {
      mkdir(targetFilePath, S_IRWXU);            // Create a copy of that directory in the new version folder
      copyFiles(sourceFilePath, targetFilePath); // Recursive call with new source and target
    }
    else
    {
      duplicate(sourceFilePath, targetFilePath); // copy file into new folder
    }
    free(sourceFilePath);
    free(targetFilePath);
  }
}

void duplicate(char *source, char *target)
{
  int sourceFD = open(source, O_RDONLY, S_IRWXU);
  int targetFD = open(target, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if (sourceFD == -1 || targetFD == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }

  char byteBuf[4096];
  int bytes_read;

  while (1)
  {
    bytes_read = read(sourceFD, byteBuf, sizeof(byteBuf)); // read from source file

    if (bytes_read < 0)
    {
      perror("File Read Error");
      close(sourceFD);
      close(targetFD);
      exit(EXIT_FAILURE);
    }
    else if (bytes_read == 0) // if we didn't read any bytes
    {
      break;
    }

    write(targetFD, byteBuf, bytes_read); // write into target file
  }
  close(sourceFD);
  close(targetFD);
}

void updateManifest(FileNode *manifestHead, FileNode *commitHead, char *versionDir)
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
        foundFile = 1;
        if (!strcmp(aCommitNode->op, "M")) // if file is modified
        {
          char *hash = malloc(sizeof(char) * (strlen(aCommitNode->hash)));
          strcpy(hash, aCommitNode->hash);
          aManifestNode->hash = hash;                            // change hash value in manifest
          aManifestNode->fileVersion = aCommitNode->fileVersion; // change file version in manifest
          aManifestNode->projectVersion = newProjectVersion;     // change project version in manifest
          break;                                                 // move on to next commit node
        }
        else // file is deleted
        {
          if (!prevNode) // if node is first in list
          {
            prevNode = aManifestNode;           // store current node, which is the head of the list
            manifestHead = aManifestNode->next; // make head point to the next node
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
          deleteFile(aManifestNode, versionDir);
          break; // move on to next commit node
        }
      }
      prevNode = aManifestNode;
      aManifestNode = aManifestNode->next; // if files don't match, move on to next node.
    }
    if (foundFile)
    {
      aCommitNode = aCommitNode->next;
    }
    else
    {
      prevNode = aCommitNode->next;
      aCommitNode->next = NULL;
      appendNode(&manifestHead, aCommitNode);
      aCommitNode = prevNode;
    }
  }
  // Write new manifest
  writeManifest(manifestHead, versionDir);
}

void deleteFile(FileNode *aNode, char *versionDir)
{
  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(versionDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }

    char *newPathName = malloc((sizeof(char) * strlen(versionDir)) + (sizeof(char) * strlen(directoryEntry->d_name)) + 2); // +2 for '/' and '\0'
    sprintf(newPathName, "%s/%s", versionDir, directoryEntry->d_name);

    if (!strcmp(aNode->filePathServer, newPathName))
    {
      unlink(newPathName);
      free(newPathName);
      break;
    }
    free(newPathName);
  }
}

void writeManifest(FileNode *manifestHead, char *versionDir)
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

void updateHistory(char *projectDir, char *commit)
{
  char *commitPath = commit;
  char *historyPath = getFilePath(projectDir, ".history");

  int commitFD = open(commitPath, O_RDONLY, S_IRWXU);
  int historyFD = open(historyPath, O_WRONLY | O_APPEND, S_IRWXU);
  if (commitFD == -1 || historyFD == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }

  char byteBuf[4096];
  int bytes_read;

  while (1)
  {
    bytes_read = read(commitFD, byteBuf, sizeof(byteBuf)); // read from source file

    if (bytes_read < 0)
    {
      perror("File Read Error");
      close(commitFD);
      close(historyFD);
      exit(EXIT_FAILURE);
    }
    else if (bytes_read == 0) // if we didn't read any bytes
    {
      break;
    }

    write(historyFD, byteBuf, bytes_read); // write into target file
  }
  close(commitFD);
  close(historyFD);
}