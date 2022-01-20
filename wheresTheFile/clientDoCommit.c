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

#include "clientDoCommit.h"
#include "errorCheck.h"
#include "fileNode.h"
#include "directoryFunctions.h"
#include "readSpecialFile.h"
#include "getFromSocket.h"
#include "sendToSocket.h"

void doCommitClient(void *socketPointer, char *projectName)
{
  char *projectDir = getProjectDir(projectName);

  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(projectDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

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
      updatePath = malloc((sizeof(char) * strlen(projectDir)) + (sizeof(char) * strlen(directoryEntry->d_name)) + 2); // Create path for update file
      sprintf(updatePath, "%s/%s", projectDir, directoryEntry->d_name);

      struct stat st;
      stat(updatePath, &st);
      if (st.st_size != 0) // If update has stuff inside
      {
        printf("Non-empty .update found. Please upgrade.\n");
        return;
      }
    }
  }

  int clientSocket = *(int *)socketPointer;
  char commitBytes[2] = "09";                              // Commit command
  send(clientSocket, commitBytes, sizeof(commitBytes), 0); // Send upgrade command to server
  int len = strlen(projectName);
  char nameLength[(int)log10(len) + 2];
  sprintf(nameLength, "%d:", len);
  send(clientSocket, nameLength, strlen(nameLength), 0);
  send(clientSocket, projectName, strlen(projectName), 0); // Send project name to server

  char *clientManifestPath = getFilePath(projectDir, ".manifest");       // get path of client manifest
  char *serverManifestPath = getFilePath(projectDir, ".servermanifest"); // get path of server manifest
  char *commitPath = getFilePath(projectDir, ".commit");
  getSpecialFile(serverManifestPath, socketPointer); // write manifest to .servermanifest

  FileNode *clientManifestNode = newNode(clientManifestPath); // Create node for client manifest
  FileNode *clientList = readManifest(clientManifestNode);    // Get list of files in client manifest
  FileNode *serverManifestNode = newNode(serverManifestPath); // Create node for server manifest
  FileNode *serverList = readManifest(serverManifestNode);    // Get list of files in server manifest

  FileNode *commitNode = newNode(commitPath);
  FileNode *commitList = NULL;

  if ((!serverList && clientList->projectVersion != 0) || (serverList && (serverList->projectVersion != clientList->projectVersion))) // If versions don't match
  {
    printf("Project version mismatch. Update local project.\n");
    return;
  }
  else
  {
    commitList = buildCommit(clientList, serverList); // Get list of changes by comparing client and server manifest
    writeCommitFile(commitList, projectDir);          // Write changes to .commit file locally
    sendSpecialFile(commitNode, socketPointer);       // Send list of changes to server
    send(clientSocket, "\0", sizeof(char), 0);
    unlink(serverManifestPath);
  }
}

FileNode *buildCommit(FileNode *clientList, FileNode *serverList)
{
  FileNode *aClientNode = clientList; // Pointer to a node in clientList
  FileNode *aServerNode = NULL;       // Pointer to a node in serverList
  FileNode *prevNode;
  FileNode *commitList = NULL; // List of nodes to be written to .commit

  int foundFile;
  int newProjectVersion;
  if (clientList)
  {
    newProjectVersion = clientList->projectVersion + 1;
  }
  else
  {
    newProjectVersion = 1;
  }

  while (aClientNode) // Loop through every client node
  {
    aServerNode = serverList; // Set Pointer to head of list
    foundFile = 0;            // Set found file to 0
    prevNode = NULL;          // Set prevnode to null

    char *liveHash = getFileHash(aClientNode->filePathClient);
    while (aServerNode) // Loop through every server node
    {
      if (!strcmp(aClientNode->filePathClient, aServerNode->filePathClient)) // if found matching node
      {
        foundFile = 1;
        if (strcmp(aClientNode->hash, aServerNode->hash) != 0 && aClientNode->fileVersion < aServerNode->fileVersion) // Hashes different & server version is newer
        {
          printf("Files out of date: synch with repository before commiting.\n");
          return NULL;
        }
        else if (!strcmp(aClientNode->hash, aServerNode->hash) && strcmp(liveHash, aClientNode->hash) != 0) // Manifest hashes are the same, live hash is different
        {
          // Add serverNode to commit
          aServerNode->op = malloc(sizeof(char) * 2);      // Malloc space for operator
          sprintf(aServerNode->op, "M");                   // Set Modify operator
          aServerNode->projectVersion = newProjectVersion; // Update project version for node
          aServerNode->hash = liveHash;                    // Set hash to the live hash of the file
          aServerNode->fileVersion += 1;                   // Increment file version
          if (!prevNode)                                   // if node is first in list
          {
            prevNode = aServerNode;            // store current node, which is the head of the list
            serverList = aServerNode->next;    // make head point to the next node
            prevNode->next = NULL;             // break link between node and next node
            appendNode(&commitList, prevNode); // add node to commit list

            break; // Move on to next client node
          }
          else // node is in the middle of the list
          {
            prevNode->next = aServerNode->next;   // connect previous node to the next node
            aServerNode->next = NULL;             // break link between current node and next node
            appendNode(&commitList, aServerNode); // Add node to commit list

            break; // Move on to next client node
          }
        }
      }
      else // not found, move on to next server node
      {
        prevNode = aServerNode;          // save prev node
        aServerNode = aServerNode->next; // go to next node
      }
    }

    if (!foundFile) // Did not find this file in server manifest, add to commit with A command
    {
      // Add clientNode to commit
      aClientNode->op = malloc(sizeof(char) * 2);      // Malloc space for operator
      sprintf(aClientNode->op, "A");                   // Set Modify operator
      aClientNode->projectVersion = newProjectVersion; // Update project version for node
      aClientNode->hash = liveHash;                    // Set hash to the live hash of the file
      aClientNode->fileVersion += 1;                   // Increment file version
      if (!prevNode)                                   // if node is first in list
      {
        prevNode = aClientNode;            // store current node, which is the head of the list
        clientList = aClientNode->next;    // make head point to the next node
        prevNode->next = NULL;             // break link between node and next node
        appendNode(&commitList, prevNode); // add node to commit list

        aClientNode = clientList; // Move on to next node: in this case, new head of client list
      }
      else // node is in the middle of the list
      {
        prevNode->next = aClientNode->next;   // connect previous node to the next node
        aClientNode->next = NULL;             // break link between current node and next node
        appendNode(&commitList, aClientNode); // Add node to commit list

        aClientNode = prevNode->next; // Move on to next node
      }
    }
    else
    {
      aClientNode = aClientNode->next; // Move on to next node
    }
  }
  aServerNode = serverList; // Set back to the head
  if (aServerNode)          // If there are still files left in the server manifest, then add to the commit file with the D command
  {
    while (aServerNode) // go through leftover servernode list
    {
      aServerNode->op = malloc(sizeof(char) * 2);      // Malloc space for operator
      sprintf(aServerNode->op, "D");                   // Set Modify operator
      aServerNode->projectVersion = newProjectVersion; // Update project version for node

      aServerNode = aServerNode->next; // Move on to next node
    }

    aServerNode = serverList;             // Go back to head of list
    appendNode(&commitList, aServerNode); // append entire leftover serverNode list to commit list
  }
  return commitList; // return list of files in commit
}

void writeCommitFile(FileNode *commitList, char *projectDir)
{
  char *commitPath = getFilePath(projectDir, ".commit"); // Get path of commit file

  int fd = open(commitPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); // open and create file
  if (fd == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }

  char *space = " ";
  char *newLine = "\n";

  int len = 1;
  if (commitList->projectVersion != 0)
  {
    len = (int)log10(commitList->projectVersion) + 1;
  }
  char projectVersion[len + 1];
  sprintf(projectVersion, "%d", commitList->projectVersion);

  write(fd, projectVersion, strlen(projectVersion)); // Write project version to top of commit
  write(fd, newLine, sizeof(char));                  // write one newline

  FileNode *aNode = commitList; // start at top of list
  while (aNode)                 // Loop through all nodes
  {
    write(fd, aNode->op, strlen(aNode->op)); // Write operator for that file
    write(fd, space, sizeof(char));

    int len = 1;
    if (aNode->fileVersion != 0)
    {
      len = (int)log10(aNode->fileVersion) + 1;
    }
    char fileVersion[len + 1];
    sprintf(fileVersion, "%d", aNode->fileVersion);
    write(fd, fileVersion, strlen(fileVersion));

    write(fd, space, sizeof(char)); // write file path
    write(fd, aNode->filePathClient, strlen(aNode->filePathClient));
    write(fd, space, sizeof(char));

    write(fd, aNode->hash, strlen(aNode->hash)); // Write hash
    write(fd, newLine, sizeof(char));

    aNode = aNode->next; // Go to next node
  }
  close(fd);
}

char *getFileHash(char *filePath)
{
  SHA_CTX ctx;     // SHA context holds hash while reading
  SHA1_Init(&ctx); // Initialize SHA context

  unsigned char hash[SHA_DIGEST_LENGTH]; // buffer for hash

  int bytes_read;
  char readBuf[4096];                         // buffer to store read data
  int fd = open(filePath, O_RDONLY, S_IRWXU); // Open file for reading

  while (1)
  {
    bytes_read = read(fd, readBuf, sizeof(readBuf)); // Read up to readBuf bytes from the file
    if (bytes_read < 0)
    {
      perror("File Read Error");
      close(fd);
      exit(EXIT_FAILURE);
    }

    if (bytes_read == 0) // If end of file
    {
      SHA1_Final(hash, &ctx); // Write context to hash buffer
      break;
    }

    SHA1_Update(&ctx, readBuf, bytes_read); // Add what we just read to the hash context
  }

  char *hashString = malloc(sizeof(char) * (SHA_DIGEST_LENGTH * 2) + 1); // Malloc space for hash as a printable string
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++)                            // For each element in hash
  {
    sprintf(hashString + (i * 2), "%02x", hash[i]); // print into hashString. This gives printable version of hash.
  }

  hashString[SHA_DIGEST_LENGTH * 2] = '\0'; // Set null terminator

  close(fd);
  return hashString;
}