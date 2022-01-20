#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "serverRequestHandler.h"
#include "getFromSocket.h"
#include "doCreate.h"
#include "doCheckout.h"
#include "doUpdate.h"
#include "doUpgrade.h"
#include "doRollback.h"
#include "doDestroy.h"
#include "doCommit.h"
#include "doHistory.h"
#include "doPush.h"

void *handleRequest(void *socketPointer)
{
  char firstTwoBytes[3];
  char restOfMessage[256];
  int clientSocket = *(int *)socketPointer;
  int bytes_read = recv(clientSocket, &firstTwoBytes, sizeof(firstTwoBytes) - 1, 0);
  firstTwoBytes[2] = '\0';
  printf("Command: %s\n", firstTwoBytes);
  if (!strcmp(firstTwoBytes, "00"))
  {
    printf("Checkout request.\n");
    // recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    // printf("Data: %s\n", restOfMessage);
    // projectName = getProjectName(socketPointer);
    doCheckout(socketPointer);
  }
  else if (!strcmp(firstTwoBytes, "01"))
  {
    printf("Push request.\n");
    // recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    // printf("Data: %s\n", restOfMessage);
    doPush(socketPointer);
  }
  else if (!strcmp(firstTwoBytes, "02"))
  {
    printf("Update request.\n");
    // recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    // printf("Data: %s\n", restOfMessage);
    doUpdate(socketPointer);
  }
  else if (!strcmp(firstTwoBytes, "03"))
  {
    printf("Upgrade request.\n");
    // recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    // printf("Data: %s\n", restOfMessage);
    doUpgrade(socketPointer);
  }
  else if (!strcmp(firstTwoBytes, "04"))
  {
    printf("Create request.\n");
    // recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    // printf("Data: %s\n", restOfMessage);
    doCreate(socketPointer);
  }
  else if (!strcmp(firstTwoBytes, "05"))
  {
    printf("Destroy request.\n");
    //   recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    //   printf("Data: %s\n", restOfMessage);
    doDestroy(socketPointer);
  }
  else if (!strcmp(firstTwoBytes, "06"))
  {
    printf("Current Version request.\n");
    // recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    // printf("Data: %s\n", restOfMessage);
    // projectName = getProjectName(socketPointer);
    doUpdate(socketPointer); // SAME FUNCTIONALITY: SEND MANIFEST TO CLIENT
  }
  else if (!strcmp(firstTwoBytes, "07"))
  {
    printf("History request.\n");
    // recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    // printf("Data: %s\n", restOfMessage);
    doHistory(socketPointer);
  }
  else if (!strcmp(firstTwoBytes, "08"))
  {
    printf("Rollback request.\n");
    // recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    // printf("Data: %s\n", restOfMessage);
    doRollback(socketPointer);
  }
  else if (!strcmp(firstTwoBytes, "09"))
  {
    printf("Commit request.\n");
    doCommit(socketPointer);
  }
  else
  {
    printf("Error: unknown request.\n");
    recv(clientSocket, &restOfMessage, sizeof(restOfMessage), 0);
    printf("Data: %s\n", restOfMessage);
  }

  // send(clientSocket, firstTwoBytes, sizeof(firstTwoBytes), 0);

  // printf("Test print projectName: %s\n", projectName);
  printf("Closing client connection...\n");
  close(clientSocket);

  return NULL;
}