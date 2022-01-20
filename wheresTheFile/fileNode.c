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

#include "fileNode.h"

FileNode *newNode(char *filePath)
{
  FileNode *aNode = (FileNode *)malloc(sizeof(FileNode)); // create new node

  aNode->filePathServer = NULL;
  aNode->filePathClient = NULL;
  aNode->next = NULL;
  aNode->op = NULL;
  aNode->error = 0;
  aNode->hash = NULL;

  if (filePath) // if given a file path
  {
    aNode->filePathClient = malloc(sizeof(char) * strlen(filePath) + 1); // malloc
    strcpy(aNode->filePathClient, filePath);                             // insert file path
  }

  return aNode; // return node
}

void appendNode(FileNode **head, FileNode *appendThis)
{
  FileNode *aNode = *head; // create node pointer
  if (!aNode)              // if list is empty
  {
    aNode = appendThis; // add node to the list
    *head = aNode;      // make new node the head node
    return;
  }
  FileNode *prevNode; // temp pointer
  while (aNode)       // while node is not null
  {
    prevNode = aNode;    // remember previous node
    aNode = aNode->next; // go to next node
  }
  prevNode->next = appendThis; // add node to end of list
}

void freeAllNodes(FileNode *head)
{
  if (head->next) // if there is a next node
  {
    freeAllNodes(head->next); // recursive call on next node
  }

  if (head->filePathClient) // if node has file path
  {
    free(head->filePathClient); // free
  }
  if (head->filePathServer)
  {
    free(head->filePathServer);
  }
  if (head->hash)
  {
    free(head->hash);
  }

  free(head); // free entire node
}