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

#include "readSpecialFile.h"
#include "fileNode.h"
#include "initGetFromSocket.h"

int readInt(int fd)
{
  INIT_GET(NULL); // Variable initialization

  while (1) // Loop
  {
    bytes_read = read(fd, (void *)&byteBuffer, sizeof(byteBuffer)); // read one byte at a time
    if (bytes_read < 0)
    {
      perror("File Read Error");
      close(fd);
      exit(EXIT_FAILURE);
    }
    if (bytes_read == 0)
    {
      break;
    }

    if (tokenLength >= tokenBufSize - 1)
    {
      /* time to make it bigger */
      tokenBufSize += 10;
      //printf("-----Growing %d-----", tokenBufSize);
      char *tokenBuf2 = realloc(tokenBuf, tokenBufSize);
      if (tokenBuf2 == NULL)
      {
        printf("out of memory for tokenBuf realloc\n");
        // free(tokenBuf);
        // close(clientSocket);
        // exit(EXIT_FAILURE);
      }
      else
        tokenBuf = tokenBuf2;
    }

    if (isspace(byteBuffer)) // if hit whitespace
    {
      tokenBuf[tokenLength] = '\0'; // add null terminator
      int intRead = atoi(tokenBuf); // convert to int
      free(tokenBuf);               // free
      return intRead;               // return number
    }

    tokenBuf[tokenLength++] = byteBuffer; // add character to token buffer
  }
  return -1; // Failure case
}

char *readString(int fd)
{
  INIT_GET(NULL); // Variable initialization

  while (1) // loop
  {
    bytes_read = read(fd, (void *)&byteBuffer, sizeof(byteBuffer)); // read one byte at a time
    if (bytes_read < 0)
    {
      perror("File Read Error");
      close(fd);
      exit(EXIT_FAILURE);
    }
    if (bytes_read == 0)
    {
      break;
    }

    if (tokenLength >= tokenBufSize - 1)
    {
      /* time to make it bigger */
      tokenBufSize += 10;
      //printf("-----Growing %d-----", tokenBufSize);
      char *tokenBuf2 = realloc(tokenBuf, tokenBufSize);
      if (tokenBuf2 == NULL)
      {
        printf("out of memory for tokenBuf realloc\n");
        // free(tokenBuf);
        // close(clientSocket);
        // exit(EXIT_FAILURE);
      }
      else
        tokenBuf = tokenBuf2;
    }

    if (isspace(byteBuffer)) // if hit whitespace
    {
      tokenBuf[tokenLength] = '\0'; // add null terminator
      return tokenBuf;              // return string
    }

    tokenBuf[tokenLength++] = byteBuffer; // add character to token buffer
  }
  return NULL; // Failure case
}

FileNode *readCommit(FileNode *commitNode)
{
  int fd;
  if (commitNode->filePathServer)
  {
    fd = open(commitNode->filePathServer, O_RDONLY, S_IRWXU); // open file
  }
  else
  {
    fd = open(commitNode->filePathClient, O_RDONLY, S_IRWXU); // open file
  }
  if (fd == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }

  // int reachedEnd = 0;
  FileNode *head = NULL;         // create head pointer
  int projVersion = readInt(fd); // get project version
  if (projVersion == -1)
  {
    printf("Missing project version number.\n");
    char errorMessage[32] = "01Missing version number";
    FileNode *aNode = newNode(NULL);
    aNode->error = 1;
    aNode->filePathClient = errorMessage;
    close(fd);
    return aNode;
  }

  while (1) // loop
  {
    FileNode *aNode = newNode(NULL);
    aNode->projectVersion = projVersion;
    aNode->op = readString(fd);
    aNode->fileVersion = readInt(fd);
    aNode->filePathClient = readString(fd);
    aNode->hash = readString(fd);
    int hash;
    if (aNode->hash)
    {
      hash = atoi(aNode->hash);
    }
    else
    {
      hash = -1;
    }

    if (aNode->op == NULL && aNode->fileVersion == -1 && aNode->filePathClient == NULL && hash == -1)
    {
      break;
    }

    if (aNode->op == NULL || aNode->fileVersion == -1 || aNode->filePathClient == NULL || hash == -1) // If either of these is true, then we hit the end of the file before a node was completed OR file was empty
    {
      printf("Incomplete. Returning...\n");
      FileNode *aNode = newNode("01Incomplete commit");
      aNode->error = 1;
      close(fd);
      return aNode;
    }
    appendNode(&head, aNode); // add node to list
  }

  close(fd);
  return head; // return head of list
}

FileNode *readManifest(FileNode *manifestNode)
{
  // char cwd[_PC_PATH_MAX];
  // getcwd(cwd, sizeof(cwd));
  // printf("Current Directory: %s\n", cwd);

  int fd;
  if (manifestNode->filePathServer)
  {
    fd = open(manifestNode->filePathServer, O_RDONLY, S_IRWXU); // open file
  }
  else
  {
    fd = open(manifestNode->filePathClient, O_RDONLY, S_IRWXU); // open file
  }

  if (fd == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }

  FileNode *head = NULL;         // create head pointer
  int projVersion = readInt(fd); // get project version
  if (projVersion == -1)
  {
    printf("Manifest missing project version number.\n");
    char errorMessage[34] = "01Manifest missing version number";
    FileNode *aNode = newNode(NULL);
    aNode->error = 1;
    aNode->filePathClient = errorMessage;
    close(fd);
    return aNode;
  }

  while (1)
  {
    FileNode *aNode = newNode(NULL);
    aNode->projectVersion = projVersion;
    aNode->fileVersion = readInt(fd);
    aNode->filePathClient = readString(fd);
    aNode->hash = readString(fd);

    int hash;
    if (aNode->hash)
    {
      hash = atoi(aNode->hash);
    }
    else
    {
      hash = -1;
    }

    if (aNode->fileVersion == -1 && aNode->filePathClient == NULL && hash == -1)
    {
      break;
    }

    if (aNode->fileVersion == -1 || aNode->filePathClient == NULL || hash == -1) // If either of these is true, then we hit the end of the file before a node was completed OR file was empty
    {
      printf("Incomplete. Returning...\n");
      FileNode *aNode = newNode("01Incomplete manifest");
      aNode->error = 1;
      close(fd);
      return aNode;
    }
    appendNode(&head, aNode); // add node to list
  }
  close(fd);
  return head; // return head of list
}
