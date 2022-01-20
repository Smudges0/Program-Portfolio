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

#include "getFromSocket.h"
#include "initGetFromSocket.h"
#include "directoryFunctions.h"

int getBytesToRead(void *socketPointer)
{
  INIT_GET(socketPointer); // variable initialization
  while (1)                // loop
  {
    bytes_read = recv(clientSocket, &byteBuffer, sizeof(byteBuffer), 0); // read one byte at a time
    if (bytes_read < 0)
    {
      perror("File Read Error\n");
      // close(clientSocket);
      // exit(EXIT_FAILURE);
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

    if (byteBuffer == ':') // Check for delimiter
    {
      // Delimiter means we have read the whole number! Now read that many bytes.
      // printf("Found colon! Reading filename...\n");
      tokenBuf[tokenLength++] = '\0';
      bytesToRead = atoi(tokenBuf); // set the number of bytes to read
      free(tokenBuf);
      return bytesToRead;
    }
    else // reading number, didn't hit delimiter
    {
      tokenBuf[tokenLength++] = byteBuffer; // add number to token buffer
    }
  }
}

char *getFileName(int numBytes, void *socketPointer)
{
  INIT_GET(socketPointer); // variable initialization
  bytesToRead = numBytes;  // setting number of bytes to read from socket
  while (1)                // loop
  {
    if (bytesToRead != 0) // if there are bytes left to read
    {
      bytes_read = recv(clientSocket, &byteBuffer, sizeof(byteBuffer), 0); // read one byte
      if (bytes_read < 0)
      {
        perror("File Read Error\n");
        // close(clientSocket);
        // exit(EXIT_FAILURE);
      }
      bytesToRead--; // decrement bytes to read
    }
    else // else if there are no bytes left to read
    {
      tokenBuf[tokenLength] = '\0'; // add null terminator

      char *fileName = malloc(strlen(tokenBuf) + 1);
      strcpy(fileName, tokenBuf);
      free(tokenBuf);
      return fileName; // return fileName
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

    tokenBuf[tokenLength++] = byteBuffer; // add character to buffer
  }
}

void writeFileFromSocket(char *fileName, int numBytes, void *socketPointer)
{
  int sourceFile = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); // open file to write to
  if (sourceFile == -1)
  {
    perror("File write error");
    // free(readBuf);
    // free(tokenBuf);
    // close(fd);
    // close(encodeFile);
    // exit(EXIT_FAILURE);
  }

  char byteBuffer[4096];
  int clientSocket = *(int *)socketPointer;
  int bytes_read;
  int bytesToRead = numBytes;

  while (1) // loop
  {
    // printf("bytesToRead: %i sizeof(byteBuffer): %li\n", bytesToRead, sizeof(byteBuffer));
    if (bytesToRead == 0) // if there are no bytes left to read
    {
      //printf("End of data.\n");
      break;
    }

    if (bytesToRead >= sizeof(byteBuffer)) // if there are more bytes to read than the buffer can hold
    {
      bytes_read = recv(clientSocket, &byteBuffer, sizeof(byteBuffer), 0); // get max number of bytes
      bytesToRead -= bytes_read;                                           // subtract bytes read from bytes left to read
      // printf("Bytes to read larger than buffer size.\n");
    }
    else // if there are less bytes to read than the max buffer size
    {
      bytes_read = recv(clientSocket, &byteBuffer, bytesToRead, 0); // get rest of the bytes
      byteBuffer[bytesToRead] = '\0';                               // add null terminator
      bytesToRead = 0;                                              // set bytes to read to zero
      // printf("Bytes to read less than buffer size.\n");
    }

    if (bytes_read < 0)
    {
      perror("File Read Error\n");
      // close(clientSocket);
      // exit(EXIT_FAILURE);
    }
    // write bytes_read bytes to file
    //printf("%s\n", byteBuffer);
    //printf("%lu\n", strlen(byteBuffer));
    write(sourceFile, byteBuffer, strlen(byteBuffer)); // write buffer to file
  }
  // printf("\n");
  // free(socketPointer);
  close(sourceFile); // close file
}

char *getProjectName(void *socketPointer)
{
  int projectNameBytes = getBytesToRead(socketPointer);             // get project name bytes
  char *projectName = getFileName(projectNameBytes, socketPointer); // get project name
  return projectName;                                               // return project name
}

void getSpecialFile(char *fileName, void *socketPointer)
{
  // special file means .manifest, .commit, etc...
  int specialFileBytes = getBytesToRead(socketPointer);           // get number of bytes to read
  writeFileFromSocket(fileName, specialFileBytes, socketPointer); // receive that many bytes and write them to a file
}

void getSourceFiles(void *socketPointer, char *versionDir)
{
  int clientSocket = *(int *)socketPointer;
  int bytes_read;
  char byteBuffer;

  int sourceFileNameBytes;
  char *sourceFileName;
  int sourceFileDataBytes;

  while (1) // loop
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
      clearRecvBuffer(socketPointer);
      printf("No more files to read.\n");
      break;
    }                                                                 // else...
    sourceFileNameBytes = getBytesToRead(socketPointer);              // get bytes of file name
    sourceFileName = getFileName(sourceFileNameBytes, socketPointer); // get file name
    printf("Source name: %s\n", sourceFileName);
    sourceFileName = getBaseFileName(sourceFileName);
    //char *temp = sourceFileName + 1;
    //char *serverFilePath = malloc(sizeof(char) * (strlen(versionDir) + strlen(temp)) + 1);
    //sprintf(serverFilePath, "%s%s", versionDir, temp);
    char *temp = sourceFileName;
    char *serverFilePath = malloc(sizeof(char) * (strlen(versionDir) + strlen(temp)) + 2);
    sprintf(serverFilePath, "%s/%s", versionDir, temp);

    recursiveMakeDir(serverFilePath);

    sourceFileDataBytes = getBytesToRead(socketPointer);                     // get bytes of file data
    writeFileFromSocket(serverFilePath, sourceFileDataBytes, socketPointer); // receive and write that many bytes to a file

    free(serverFilePath);
  }
}

char *getVersionFromSocket(void *socketPointer)
{
  INIT_GET(socketPointer);

  while (1) // loop
  {
    bytes_read = recv(clientSocket, &byteBuffer, sizeof(byteBuffer), 0); // get max number of bytes

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

    if (byteBuffer == '\0') // if hit End of line
    {
      tokenBuf[tokenLength] = byteBuffer; // add null terminator
      return tokenBuf;
    }

    tokenBuf[tokenLength++] = byteBuffer; // add character to token buffer
  }
}

void clearRecvBuffer(void *socketPointer) // Read extra terminator from client to prevent connection reset by peer error on clientside
{
  int clientSocket = *(int *)socketPointer;

  char byteBuffer[4096];
  int bytes_read;
  while (1)
  {
    bytes_read = recv(clientSocket, byteBuffer, sizeof(byteBuffer), 0);
    if (bytes_read < 0)
    {
      perror("File Read Error\n");
      break;
      // close(clientSocket);
      // exit(EXIT_FAILURE);
    }
    if (bytes_read < sizeof(byteBuffer))
    {
      break;
    }
  }
}