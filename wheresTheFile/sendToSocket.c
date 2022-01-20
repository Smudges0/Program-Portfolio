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
#include <math.h>

#include "sendToSocket.h"
#include "fileNode.h"

void sendNumBytes(FileNode *fileNode, void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;
  struct stat fileStats; // create stat structure
  if (fileNode->filePathServer)
  {
    stat(fileNode->filePathServer, &fileStats);
  }
  else
  {
    stat(fileNode->filePathClient, &fileStats);
  }
  int bytesToRead = fileStats.st_size; // get file size in bytes
  int len = 1;
  if (bytesToRead != 0)
  {
    len = (int)log10(bytesToRead) + 1;
  }
  char numBytes[len + 1];                            // string to hold int and delimiter
  sprintf(numBytes, "%d:", bytesToRead);             // write number + delimiter into string
  send(clientSocket, numBytes, sizeof(numBytes), 0); // send string on socket
}

void sendFilePath(FileNode *fileNode, void *socketPointer)
{
  int clientSocket = *(int *)socketPointer;
  int bytesInName = strlen(fileNode->filePathClient);
  char numBytes[(int)log10(bytesInName) + 2]; // string to hold int

  sprintf(numBytes, "%d:", bytesInName);             // write length of file name + delimiter to string
  send(clientSocket, numBytes, sizeof(numBytes), 0); // send string on socket

  send(clientSocket, fileNode->filePathClient, strlen(fileNode->filePathClient), 0); // send filename on socket
}

void sendFileData(FileNode *fileNode, void *socketPointer)
{
  int fd;
  if (fileNode->filePathServer)
  {
    fd = open(fileNode->filePathServer, O_RDONLY); // open file from fileNode
  }
  else
  {
    fd = open(fileNode->filePathClient, O_RDONLY); // open file from fileNode
  }

  if (fd == -1)
  {
    perror("File Read Error");
    exit(EXIT_FAILURE);
  }
  char readBuffer[4096];
  int clientSocket = *(int *)socketPointer;
  int bytes_read;

  while (1) // loop
  {
    bytes_read = read(fd, readBuffer, sizeof(readBuffer)); // read readBuffer (4096) bytes

    if (bytes_read < 0)
    {
      perror("File Read Error");
      close(fd);
      exit(EXIT_FAILURE);
    }
    else if (bytes_read == 0) // if we didn't read any bytes
    {
      // send(clientSocket, '\0', 1, 0); // send null terminator and break
      break;
    }

    // if (bytes_read < sizeof(readBuffer)) // if we read less bytes than the size of readBuffer
    // {
    //   readBuffer[bytes_read] = '\0'; // add null terminator to end of readBuffer
    // }
    send(clientSocket, readBuffer, sizeof(char) * bytes_read, 0); // send readBuffer
  }
  close(fd); // close file
}

void sendSpecialFile(FileNode *fileNode, void *socketPointer)
{
  sendNumBytes(fileNode, socketPointer); // send number of bytes in file
  sendFileData(fileNode, socketPointer); // send file data
}

void sendSourceFiles(FileNode *fileList, void *socketPointer)
{
  FileNode *aNode = fileList; // hold list of fileNodes

  while (aNode) // while node is not NULL
  {
    sendFilePath(aNode, socketPointer); // send file name
    sendNumBytes(aNode, socketPointer); // send number of bytes
    sendFileData(aNode, socketPointer); // send file data

    aNode = aNode->next; // move onto next node
  }
}