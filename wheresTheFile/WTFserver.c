#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "serverRequestHandler.h"
#include "errorCheck.h"
#include "mutexHandler.h"

MutexNode *mutexHead = NULL;
pthread_mutex_t mutexHandlerLock;
int main(int argc, char *argv[])
{
  if (pthread_mutex_init(&mutexHandlerLock, NULL) != 0)
  {
    printf("Failed to initialize mutexHandlerLock\n");
    exit(EXIT_FAILURE);
  }

  int serverSocket, clientSocket, addrSize;
  struct sockaddr_in serverAddr, clientAddr;

  // serverSocket = socket(AF_INET, SOCK_STREAM, 0); // create socket with IPV4 (AF_INET), TCP (SOCK_STREAM), and protocol (0))
  errorCheck(serverSocket = socket(AF_INET, SOCK_STREAM, 0), "Error: failed to create socket.\n");

  // Set up address for server socket
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(5000);       // htons converts int IP address into correct format
  serverAddr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY connects to local IP, shortcut for 0.0.0.0

  errorCheck(bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)), "Error: failed to bind socket.\n");

  // Number is for backlog of connection requests
  errorCheck(listen(serverSocket, 10), "Error: failed to listen on socket.\n");

  addrSize = sizeof(struct sockaddr_in); // set size of address for clientSocket

  while (1) // Keep Listening for connections forever
  {
    printf("Waiting for connections...\n");

    // clientSocket = accept(serverSocket, NULL, NULL); // Null fields are for structs that contain address and size of address of client
    errorCheck(clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, (socklen_t *)&addrSize), "Error: failed to accept connection.\n");
    printf("Connected!\n");
    pthread_t threadID; // create a thread object

    int *socketPointer = malloc(sizeof(int)); // Malloc space for socket pointer
    *socketPointer = clientSocket;            // Setting socket field in requestArgs

    // pthread_create(&threadID, NULL, getLock, NULL);
    pthread_create(&threadID, NULL, handleRequest, (void *)socketPointer); // Create a new thread, passing it our threadID, NULL for default atrubutes, our function, and it's args
    printf("Outside thread.\n");
  }

  close(serverSocket);

  return 0;
}