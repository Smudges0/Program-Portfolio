#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "errorCheck.h"

int main(int argc, char *argv[])
{
  int clientSocket; // socket descriptor
  struct sockaddr_in server;

  // clientSocket = socket(AF_INET, SOCK_STREAM, 0); // create socket with IPV4 (AF_INET), TCP (SOCK_STREAM), and protocol (0))

  errorCheck(clientSocket = socket(AF_INET, SOCK_STREAM, 0), "Error: Failed to create socket.\n");

  // Set up address for socket
  server.sin_family = AF_INET;
  server.sin_port = htons(5000);       // htons converts int IP address into correct format
  server.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY connects to local IP, shortcut for 0.0.0.0

  // int connectStatus = connect(clientSocket, (struct sockaddr *)&server, sizeof(server));
  int connectStatus;
  errorCheck(connectStatus = connect(clientSocket, (struct sockaddr *)&server, sizeof(server)), "Error: could not connect to server.\n");

  char *message = malloc(sizeof(char) * 256);

  char *cmd = argv[1];
  if (!strcmp(cmd, "04"))
  {
    message = "0410:myProject2"; //create project;
  }
  else if (!strcmp(cmd, "091"))
  {
    message = "0910:myProject289:1\nA 1 ./subdir1/subdir2/test1.txt 1234\nA 1 ./subdir1/test2.txt 5678\nA 1 ./test3.txt 9012\n";
  }
  else if (!strcmp(cmd, "011"))
  {
    message = "0110:myProject289:1\nA 1 ./subdir1/subdir2/test1.txt 1234\nA 1 ./subdir1/test2.txt 5678\nA 1 ./test3.txt 9012\n27:./subdir1/subdir2/test1.txt9:test1Data19:./subdir1/test2.txt9:test2Data11:./test3.txt9:test3Data"; //push
  }
  else if (!strcmp(cmd, "08"))
  {
    message = "0810:myProject20";
  }
  else if (!strcmp(cmd, "07"))
  {
    message = "0710:myProject2";
  }
  else if (!strcmp(cmd, "092"))
  {
    message = "0910:myProject289:2\nM 2 ./subdir1/subdir2/test1.txt 1234\nM 2 ./subdir1/test2.txt 5678\nM 2 ./test3.txt 9012\n";
  }
  else if (!strcmp(cmd, "012"))
  {
    message = "0110:myProject289:2\nM 2 ./subdir1/subdir2/test1.txt 1234\nM 2 ./subdir1/test2.txt 5678\nM 2 ./test3.txt 9012\n27:./subdir1/subdir2/test1.txt9:test1Data19:./subdir1/test2.txt9:test2Data11:./test3.txt9:test3Data";
  }
  else if (!strcmp(cmd, "05"))
  {
    message = "0510:myProject2";
  }
  else if (!strcmp(cmd, "00"))
  {
    message = "0010:myProject2";
  }
  else if (!strcmp(cmd, "06"))
  {
    message = "0610:myProject2";
  }

  // Send message to server | test multithreading
  // char message[256] = "0110:myProject289:1\nA 1 ./subdir1/subdir2/test1.txt 1234\nA 1 ./subdir1/test2.txt 5678\nA 1 ./test3.txt 9012\n27:./subdir1/subdir2/test1.txt9:test1Data19:./subdir1/test2.txt9:test2Data11:./test3.txt9:test3Data";
  // 008:project1
  // 018:project149:2\nA 1 ./myFile1.txt 5555\nR 1 ./test1.txt 4829482\n13:./myFile1.txt5:Hello
  // 048:project1
  // 058:project2
  // 078:project1
  // 088:project11
  // 098:project1
  //

  // test myProject2
  // char message[256] = "0410:myProject2 //create project";
  // char message[256] = "0910:myProject289:1\nA 1 ./subdir1/subdir2/test1.txt 1234\nA 1 ./subdir1/test2.txt 5678\nA 1 ./test3.txt 9012\n"; //commit a change
  // char message[256] = "0110:myProject289:1\nA 1 ./subdir1/subdir2/test1.txt 1234\nA 1 ./subdir1/test2.txt 5678\nA 1 ./test3.txt 9012\n27:./subdir1/subdir2/test1.txt9:test1Data19:./subdir1/test2.txt9:test2Data11:./test3.txt9:test3Data"; //push
  // char message[256] = "0810:myProject20";
  // char message[256] = "0710:myProject2";
  // char message[256] = "0910:myProject289:2\nM 2 ./subdir1/subdir2/test1.txt 1234\nM 2 ./subdir1/test2.txt 5678\nM 2 ./test3.txt 9012\n";
  // char message[256] = "0110:myProject289:2\nM 2 ./subdir1/subdir2/test1.txt 1234\nM 2 ./subdir1/test2.txt 5678\nM 2 ./test3.txt 9012\n27:./subdir1/subdir2/test1.txt9:test1Data19:./subdir1/test2.txt9:test2Data11:./test3.txt9:test3Data";
  // char message[256] = "0510:myProject2";

  // strcpy(message, argv[1]);
  printf("Sending: %s\n", message);
  send(clientSocket, message, strlen(message) + 1, 0);
  printf("Sent. Waiting for response\n");

  // char message2[256] = "20:commitdata and stuff";
  // send(clientSocket, message2, strlen(message2), 0);
  // Recieve message from server
  int bytes_received = 0;
  char response[4096];
  while (1)
  {
    bytes_received = recv(clientSocket, &response, sizeof(response), 0);
    // if (response[bytes_received] == '\0')
    // {
    //   printf("%s", response);
    //   break;
    // }
    if (bytes_received == 0)
    {
      break;
    }
    // if (bytes_received < sizeof(response))
    // {
    //   printf("Message received shorter than buffer. Ending...\n");
    // }
    if (bytes_received == -1)
    {
      perror("Errno");
    }
    // printf("Bytes received: %d\n", bytes_received);
    response[bytes_received] = '\0';
    printf("%s", response);
  }
  // Close socket
  close(clientSocket);
  return 0;
}