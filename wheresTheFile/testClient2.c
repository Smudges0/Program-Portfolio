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

#include "errorCheck.h"
#include "fileNode.h"
#include "directoryFunctions.h"
#include "readSpecialFile.h"
#include "getFromSocket.h"
#include "sendToSocket.h"
#include "clientDoCommit.h"
#include "clientDoPush.h"
#include "clientDoUpgrade.h"

void doAdd(char *projectname, char *filename);
void doRemove(char *projectname, char *filename);
// char* getHash(char* filename);
void configure(char *ip, char *port);
char *chooseMessage(char *argv1, char *strlen, char *argv2);
void doCreateinC(char *response, char *argv2);
void doDestroyinC(char *response);
void doUpdateinC(int *socketPointer, char *response, char *projectname);
void doUpgrade(void *socketPointer, char *projectName);
char *getCurrentDirName(char *projectname);
FileNode *removeFilename(FileNode *cmani, char *filename);
void doCurrentVersion(int *socketPointer, char *response);
void doCommitClient(void *socketPointer, char *projectName);

int main(int argc, char *argv[])
{
  // Simon
  // if (strcmp(argv[1], "configure") == 0)
  // {
  //   configure(argv[2], argv[3]);
  // }
  // if (strcmp(argv[1], "add") == 0)
  // {
  //   doAdd(argv[2], argv[3]);
  //   exit(1);
  // }
  // if (strcmp(argv[1], "remove") == 0)
  // {
  //   doRemove(argv[2], argv[3]);
  //   exit(1);
  // }

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

  int *socketPointer = malloc(sizeof(int));
  *socketPointer = clientSocket;

  char *message = malloc(sizeof(char) * 256);
  char *cmd = argv[1];
  char *projectName;
  if (!strcmp(cmd, "04"))
  {
    message = "0410:myProject2"; //create project;
  }
  else if (!strcmp(cmd, "commit"))
  {
    // message = "0910:myProject289:1\nA 1 ./subdir1/subdir2/test1.txt 1234\nA 1 ./subdir1/test2.txt 5678\nA 1 ./test3.txt 9012\n";
    projectName = argv[2];
    doCommitClient(socketPointer, projectName);
  }
  else if (!strcmp(cmd, "push"))
  {
    projectName = argv[2];
    doPushClient(socketPointer, projectName);
    // message = "0110:myProject289:1\nA 1 ./subdir1/subdir2/test1.txt 1234\nA 1 ./subdir1/test2.txt 5678\nA 1 ./test3.txt 9012\n27:./subdir1/subdir2/test1.txt9:test1Data19:./subdir1/test2.txt9:test2Data11:./test3.txt9:test3Data"; //push
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

  // Simon

  //char message[256];
  // int len = strlen(argv[2]);
  // char str[2];
  // snprintf(str, 2, "%d", len);
  // //char* message = chooseMessage(argv[1], str, argv[2]);

  // printf("Sending: %s\n", message);
  // if (send(clientSocket, message, sizeof(message), 0) < 0)
  // {
  //   puts("Send failed");
  // }
  // else
  // {
  //   printf("Sent. Waiting for response\n");
  // }
  // strcpy(message, argv[1]);

  // char message2[256] = "20:commitdata and stuff";
  // send(clientSocket, message2, strlen(message2), 0);
  // Recieve message from server
  // int bytes_received = 0;
  // char response[4096];
  // int count = 0;

  // if (strcmp(argv[1], "checkout") == 0)
  // {
  //   while (1)
  //   {
  //     bytes_received = recv(clientSocket, &response, sizeof(response), 0);
  //     if (bytes_received == 0)
  //     {
  //       break;
  //     }
  //     response[bytes_received] = '\0';

  //     char *projectDir = getProjectDir(argv[2]);
  //     char *version = getVersionDir(projectDir, argv[2]);
  //     char dirname[strlen(version) + 1];
  //     strcpy(dirname, version);
  //     strcat(dirname, "c");
  //     //printf("dirname: %s\n", dirname);
  //     int totalbytesofmani;
  //     int bytesoffile;
  //     char *fullfilename;

  //     if (strcmp(response, "00") == 0 && count == 0)
  //     {
  //       //making subdirectory
  //       printf("dirname: %s\n", dirname);
  //       mkdir(dirname, 0700);
  //       printf("it is 00.\n");
  //       count++;
  //     }
  //     else if (count <= 4)
  //     {
  //       //printf("this is count 1.\n");
  //       //printf("%s\n", response);
  //       char newcmani[strlen(dirname) + 10];
  //       strcpy(newcmani, dirname);
  //       strcat(newcmani, "/.manifest");
  //       int fd = open(newcmani, O_RDWR | O_CREAT | O_APPEND);
  //       if (fd == -1)
  //       {
  //         printf("Error Number % d\n", errno);
  //         exit(1);
  //       }

  //       char *token = strtok(response, ":");
  //       while (token != NULL)
  //       {
  //         if (count == 1)
  //         {
  //           sscanf(token, "%d", &totalbytesofmani);
  //           printf("this is the total bytes of manifest file: %d\t.\n", totalbytesofmani);
  //           count++;
  //         }
  //         else if (count == 2)
  //         {
  //           printf("%d\n", count);
  //           printf("the following is manifest file context:\n%s\n", token);
  //           write(fd, token, totalbytesofmani);
  //           close(fd);
  //           sscanf(token + totalbytesofmani, "%d", &bytesoffile);
  //           count++;
  //         }
  //         else if (count == 3)
  //         {
  //           printf("---------------------------this is count 3.\n");
  //           printf("bytesize: %d\n", bytesoffile);
  //           //char fullfilename[bytesoffile];
  //           fullfilename = malloc(bytesoffile);
  //           strcpy(fullfilename, token);
  //           printf("fullfilename: %s\n", fullfilename);
  //           sscanf(token + bytesoffile, "%d", &bytesoffile);
  //           printf("bytesize: %d\n", bytesoffile);
  //           //printf("%s\n", response);
  //           //printf("%s\n", token);
  //           count++;
  //         }
  //         else if (count == 4)
  //         {
  //           printf("===========================this is count 4.\n");
  //           char *currentdir = getCurrentDirName(argv[2]);
  //           printf("current dirname: %s\n", currentdir);
  //           printf("context: %s\n", fullfilename);
  //           char realfilename[strlen(currentdir) + strlen(fullfilename) - 1];
  //           strcpy(realfilename, currentdir);
  //           strcat(realfilename, fullfilename + 1);
  //           printf("real file name: %s\n", realfilename);
  //           // int aFile = open(realfilename, O_RDWR|O_CREAT);
  //           // write(aFile, token, strlen(token));
  //           // close(aFile);
  //           printf("%s\n", token);
  //           count--;
  //         }

  //         token = strtok(NULL, ":");
  //       }
  //     }
  //   } //end while(1)
  // }
  // else
  // {

  //   while (1)
  //   {
  //     bytes_received = recv(clientSocket, &response, sizeof(response), 0);
  //     if (bytes_received == 0)
  //     {
  //       break;
  //     }
  //     int *socketPointer = malloc(sizeof(int));
  //     *socketPointer = clientSocket;
  //     response[bytes_received] = '\0';
  //     printf("%s", response);

  //     if (strcmp(argv[1], "create") == 0)
  //     {
  //       doCreateinC(response, argv[2]);
  //       exit(1);
  //     }
  //     if (strcmp(argv[1], "destroy") == 0)
  //     {
  //       doDestroyinC(response);
  //       exit(1);
  //     }
  //     if (strcmp(argv[1], "update") == 0)
  //     {
  //       doUpdateinC(socketPointer, response, argv[2]);
  //     }

  //     if (strcmp(argv[1], "currentversion") == 0)
  //     {
  //       doCurrentVersion(socketPointer, response);
  //     }
  //   }
  // } // end if checkout else
  // // Close socket
  // close(clientSocket);
  // return 0;
}

char *chooseMessage(char *argv1, char *leninstr, char *argv2)
{
  char *message;
  if (strcmp(argv1, "checkout") == 0)
  {
    int nDigits = floor(log10(abs(strlen(argv2)))) + 1;
    int lengthofprojectname = strlen(argv2);
    char lengthinstr[nDigits];
    int messagelength = 4 + nDigits + strlen(argv2);
    message = malloc(messagelength);
    char firsttwobytes[2] = "00";
    firsttwobytes[2] = '\0';
    sprintf(lengthinstr, "%d", lengthofprojectname);
    strcpy(message, firsttwobytes);
    strcat(message, lengthinstr);
    strcat(message, ":");
    strcat(message, argv2);
  }
  else if (strcmp(argv1, "update") == 0)
  {
    int nDigits = floor(log10(abs(strlen(argv2)))) + 1;
    int lengthofprojectname = strlen(argv2);
    char lengthinstr[nDigits];
    int messagelength = 4 + nDigits + strlen(argv2);
    message = malloc(messagelength);
    char firsttwobytes[2] = "02";
    firsttwobytes[2] = '\0';
    sprintf(lengthinstr, "%d", lengthofprojectname);
    strcpy(message, firsttwobytes);
    strcat(message, lengthinstr);
    strcat(message, ":");
    strcat(message, argv2);
  }
  //      else if (strcmp(argv1,"upgrade")==0){
  //         strcpy(message, "03");
  //         strcat(message, strlen);
  //         strcat(message, ":");
  //         strcat(message, argv2);
  //     }
  //      else if (strcmp(argv1,"commit")==0){
  //         strcpy(message, "09");
  //         strcat(message, strlen);
  //         strcat(message, ":");
  //         strcat(message, argv2);
  //     }
  //     else if (strcmp(argv1,"push")==0){
  //         strcpy(message, "01");
  //         strcat(message, strlen);
  //         strcat(message, ":");
  //         strcat(message, argv2);
  //     }
  else if (strcmp(argv1, "create") == 0)
  {
    int nDigits = floor(log10(abs(strlen(argv2)))) + 1;
    int lengthofprojectname = strlen(argv2);
    char lengthinstr[nDigits];
    int messagelength = 4 + nDigits + strlen(argv2);
    message = malloc(messagelength);
    char firsttwobytes[2] = "04";
    firsttwobytes[2] = '\0';
    sprintf(lengthinstr, "%d", lengthofprojectname);
    strcpy(message, firsttwobytes);
    strcat(message, lengthinstr);
    strcat(message, ":");
    strcat(message, argv2);
  }

  // else if (strcmp(argv1,"destroy")==0){
  //     strcpy(message, "05");
  //     strcat(message, strlen);
  //     strcat(message, ":");
  //     strcat(message, argv2);
  // }
  // // else if (strcmp(argv1,"add")==0){
  // //     add("./makefile");
  // // }
  // else if (strcmp(argv1,"currentversion")==0){
  //     strcpy(message, "06");
  //     strcat(message, strlen);
  //     strcat(message, ":");
  //     strcat(message, argv2);
  // }
  // else if (strcmp(argv1,"history")==0){
  //     strcpy(message, "07");
  //     strcat(message, strlen);
  //     strcat(message, ":");
  //     strcat(message, argv2);
  // }
  // else if (strcmp(argv1,"rollback")==0){
  //     strcpy(message, "08");
  //     strcat(message, strlen);
  //     strcat(message, ":");
  //     strcat(message, argv2);
  // }
  return message;
}

void configure(char *ip, char *port)
{
  int fd;
  fd = open("./Sample files/.configure", O_RDWR | O_TRUNC);
  if (fd == -1)
  {
    printf("Error Number % d\n", errno);
    exit(1);
  }
  write(fd, "# IP:PORT\n", strlen("# IP:PORT\n"));
  int ipandportsize = strlen(ip) + strlen(":") + strlen(port);
  char *ipandport = malloc(ipandportsize);

  strcpy(ipandport, ip);
  strcat(ipandport, ":");
  strcat(ipandport, port);

  if (write(fd, ipandport, ipandportsize) != ipandportsize)
  {
    printf("there might be errors in wrting file");
  }
  else
  {
    printf("configure file successfully updated.");
  }
  close(fd);
}

void doCreateinC(char *response, char *argv2)
{
  if (strlen(response) >= 6)
  {
    printf("%s\n", response);
  }
  else if (strlen(response) < 6)
  {
    printf("no errore.\n");
    char prodirname[strlen(argv2) + 1];
    strcpy(prodirname, argv2);
    strcat(prodirname, "c");
    mkdir(prodirname, 0777);
    opendir(prodirname);
    char *versionDirName = getVersionDir(prodirname, argv2);
    printf("%s\n", versionDirName);
    mkdir(versionDirName, S_IRWXU); // create version subdirectory
    opendir(versionDirName);
    char manifullname[strlen(versionDirName) + 10];
    strcpy(manifullname, versionDirName);
    strcat(manifullname, "/");
    strcat(manifullname, ".manifest");
    int manifest = open(manifullname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    write(manifest, "0\n", 2);
    close(manifest);
  }
}

void doDestroyinC(char *response)
{
  if (strlen(response) > 4)
  {
    printf("%s\n", response);
  }
  else if (strlen(response) <= 4)
  {
    printf("successfully destroyed.\n");
  }
}

char *getCurrentDirName(char *projectname)
{
  char *projectDir = getProjectDir(projectname);
  char *version = getVersionDir(projectDir, projectname);
  char dirname[strlen(version) + 1];
  strcpy(dirname, version);
  strcat(dirname, "c");
  char *result = malloc(strlen(version) + 1);
  strcpy(result, dirname);
  return result;
}

int numDigits(int n)
{
  if (n < 10)
    return 1;
  return 1 + numDigits(n / 10);
}

void doAdd(char *projectname, char *filename)
{
  char *projectDir = getProjectDir(projectname);
  char *version = getVersionDir(projectDir, projectname);
  printf("Latest project version: %s\n", version);

  int pathsize = strlen(version) + strlen(filename);
  char afile[pathsize];
  strcpy(afile, version);
  strcat(afile, "/");
  strcat(afile, filename);
  printf("afile: %s\n", afile);
  //int exist = cfileexists(afile);
  char *manifestPath = getFilePath(version, ".manifest");
  printf("manifestPath: %s\n", manifestPath);
  FileNode *manifestNode = newNode(manifestPath);
  FileNode *head = NULL;
  head = readManifest(manifestNode);
  int digit3 = numDigits(head->projectVersion);
  char proversionstr[digit3];
  sprintf(proversionstr, "%d", head->projectVersion);
  // while (head != NULL){
  //   printf("%d\t%s\n", head->fileVersion, head->fileName);
  //   head = head->next;
  // }
  struct stat st;
  stat(afile, &st);
  if (st.st_mode)
  {
    printf("file exist\n");
  }
  else
  {
    printf("Error: file doesn't exist\n");
    return;
  }
  int fd = open(manifestPath, O_RDWR | O_TRUNC);
  if (fd == -1)
  {
    printf("Error Number % d\n", errno);
    exit(1);
  }
  write(fd, proversionstr, digit3);
  write(fd, "\n", 1);
  //free(manifestPath);
  printf("Before while loop.");
  int checkifdup = 0;
  while (head != NULL)
  {
    //printf("node filepath: %s vs filename : %s\n", head->filePathClient, afile);
    if (strcmp(head->filePathClient, filename) == 0)
    {
      //update the file version
      checkifdup = 1;
      head->fileVersion++;
      int digit = numDigits(head->fileVersion);
      char versionstr[digit];
      sprintf(versionstr, "%d", head->fileVersion);

      //update the file hash
      printf("\n Hash of the file: %s \n\n", afile);

      char buff[st.st_size];
      int i = 0;
      SHA_CTX sha_ctx;
      unsigned char sha_hash[SHA_DIGEST_LENGTH];
      char result[SHA_DIGEST_LENGTH * 2 + 1] = {00};
      SHA1_Init(&sha_ctx);

      int cfd = open(afile, O_RDONLY);
      do
      {
        i = read(cfd, buff, st.st_size);
        SHA1_Update(&sha_ctx, buff, i);
        printf("%d\t", i);
      } while (i > 0);
      close(cfd);
      SHA1_Final(sha_hash, &sha_ctx);

      for (i = 0; i < SHA_DIGEST_LENGTH; ++i)
      {
        sprintf(result + (i * 2), "%02x", sha_hash[i]);
      }
      result[SHA_DIGEST_LENGTH * 2] = '\0';
      write(fd, versionstr, digit);
      write(fd, " ", 1);
      write(fd, head->filePathClient, strlen(head->filePathClient));
      write(fd, " ", 1);
      write(fd, result, 40);
      write(fd, "\n", 1);
    }
    else
    {

      int digit = numDigits(head->fileVersion);
      char versionstr[digit];
      sprintf(versionstr, "%d", head->fileVersion);

      int digit2 = strlen(head->hash);
      char hashstr[digit2];
      sprintf(hashstr, "%s", head->hash);

      write(fd, versionstr, digit);
      write(fd, " ", 1);
      write(fd, head->filePathClient, strlen(head->filePathClient));
      write(fd, " ", 1);
      write(fd, hashstr, digit2);
      write(fd, "\n", 1);
    }
    head = head->next;
  }
  if (checkifdup == 0)
  {
    char buff[st.st_size];
    int i = 0;
    SHA_CTX sha_ctx;
    unsigned char sha_hash[SHA_DIGEST_LENGTH];
    char result[SHA_DIGEST_LENGTH * 2 + 1] = {00};
    SHA1_Init(&sha_ctx);

    int cfd = open(afile, O_RDONLY);
    do
    {
      i = read(cfd, buff, st.st_size);
      SHA1_Update(&sha_ctx, buff, i);
      printf("%d\t", i);
    } while (i > 0);
    close(cfd);
    SHA1_Final(sha_hash, &sha_ctx);

    for (i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
      sprintf(result + (i * 2), "%02x", sha_hash[i]);
    }
    result[SHA_DIGEST_LENGTH * 2] = '\0';

    write(fd, "1 ", 2);
    write(fd, filename, strlen(filename));
    write(fd, " ", 1);
    write(fd, result, 40);
    write(fd, "\n", 1);
  }
  close(fd);
}

void doRemove(char *projectname, char *filename)
{
  char *projectDir = getProjectDir(projectname);
  char *version = getVersionDir(projectDir, projectname);
  printf("Latest project version: %s\n", version);

  char *manifestPath = getFilePath(version, ".manifest");
  FileNode *manifestNode = newNode(manifestPath);
  //free(manifestPath);
  FileNode *head = NULL;
  head = readManifest(manifestNode);
  int digit2 = numDigits(head->projectVersion);
  char proversionstr[digit2];
  sprintf(proversionstr, "%d", head->projectVersion);

  FileNode *ptr = head;
  int checkifdup = 0;
  while (ptr != NULL)
  {
    if (strcmp(ptr->filePathClient, filename) == 0)
    {
      checkifdup = 1;
    }
    ptr = ptr->next;
  }
  if (checkifdup == 0)
  {
    printf("the file doesn't exit in the .manifest.\n");
    return;
  }
  FileNode *removedcmani = removeFilename(head, filename);
  int fd = open(manifestPath, O_RDWR | O_TRUNC);

  write(fd, proversionstr, digit2);
  write(fd, "\n", 1);
  while (removedcmani != NULL)
  {
    //have to write new version of manifest
    int digit = numDigits(removedcmani->fileVersion);
    char versionstr[digit];
    sprintf(versionstr, "%d", removedcmani->fileVersion);

    write(fd, versionstr, digit);
    write(fd, " ", 1);
    write(fd, removedcmani->filePathClient, strlen(removedcmani->filePathClient));
    write(fd, " ", 1);
    write(fd, removedcmani->hash, strlen(removedcmani->hash));
    write(fd, "\n", 1);
    //printf("%d\t%s\t%s\n", removedcmani->fileVersion, removedcmani->fileName, removedcmani->sha_hash);
    removedcmani = removedcmani->next;
  }
  close(fd);
}

FileNode *removeFilename(FileNode *cmani, char *filename)
{
  FileNode *cptr, *followingptr;
  cptr = cmani;
  followingptr = NULL;

  while (cptr != NULL)
  {
    if (strcmp(cptr->filePathClient, filename) == 0)
    {
      if (followingptr == NULL)
      {
        cmani = cptr->next;
      }
      else
      {
        followingptr->next = cptr->next;
      }
    }
    else
    {
    }
    followingptr = cptr;
    cptr = cptr->next;
  }

  return cmani;
}

void doUpdateinC(int *socketPointer, char *response, char *projectname)
{

  getSpecialFile("./manifestfromserver", socketPointer);

  FileNode *manifestfromserverNode = newNode("./manifestfromserver");
  FileNode *headS = NULL;
  headS = readManifest(manifestfromserverNode);
  // while(headS != NULL){
  //   printf("%d\t%s\n",headS->projectVersion, headS->filePathClient);
  //   headS = headS->next;
  // }

  //get the client side manifest node
  char *projectDir = getProjectDir(projectname);
  char *versionDir = getVersionDir(projectDir, projectname);
  //char* manifestfromclientPath = getFilePath(versionDir, ".manifest");
  char *manifestfromclientPath = getFilePath("./", ".manifest"); //test
  FileNode *manifestfromclientNode = newNode(manifestfromclientPath);
  FileNode *headC = NULL;
  headC = readManifest(manifestfromclientNode);
  // while(headC != NULL){
  //   printf("%s\n", headC->filePathClient);
  //   headC = headC->next;
  // }

  //compare two FileNodes
  FileNode *ptrc = headC;
  FileNode *ptrs = headS;
  //FileNode* ptrf = NULL;
  if (ptrc->projectVersion == ptrs->projectVersion)
  {
    printf("'Up To Date'\n");
    //clear update and conflict
    int clearupdate = open("./Sample files/.update", O_TRUNC);
    close(clearupdate);
    int clearconflict = open("./Sample files/.conflict", O_TRUNC);
    close(clearconflict);
    return;
  }
  else
  {
    printf("'Not Up To Date'\n");
    //  while (ptrs != NULL){
    int checkforconflict = 0;
    for (ptrs = headS; ptrs != NULL; ptrs = ptrs->next)
    {
      int checkifdup = 0;
      ptrc = headC;
      while (ptrc != NULL)
      {
        if (strcmp(ptrc->filePathClient, ptrs->filePathClient) == 0)
        {
          checkifdup = 1;
          if (ptrc->fileVersion != ptrs->fileVersion && strcmp(ptrc->hash, ptrs->hash) != 0)
          {
            //=============live hash=========================
            int pathsize = strlen(versionDir) + strlen(ptrc->filePathClient);
            char afile[pathsize - 1];
            strcpy(afile, versionDir);
            strcat(afile, ptrc->filePathClient + 1);
            printf("%s\n", afile);
            char *livehash = getFileHash(afile);
            printf("%s\n", livehash);
            //=============end live hash=========================
            if (strcmp(getFileHash(afile), ptrc->hash) == 0)
            {
              //modify
              int modifyfile = open("./Sample files/.update", O_WRONLY | O_APPEND | O_CREAT);
              write(modifyfile, "M ", 2);
              write(modifyfile, ptrc->filePathClient, strlen(ptrc->filePathClient));
              write(modifyfile, " ", 1);
              write(modifyfile, ptrc->hash, 40);
              write(modifyfile, "\n", 1);
              close(modifyfile);
              printf("'M %s'", ptrc->filePathClient);
            }
            else
            {
              //conflict
              checkforconflict = 1;
              int conflictfile = open("./Sample files/.conflict", O_WRONLY | O_APPEND | O_CREAT);
              write(conflictfile, "C ", 2);
              write(conflictfile, ptrc->filePathClient, strlen(ptrc->filePathClient));
              write(conflictfile, " ", 1);
              write(conflictfile, ptrc->hash, 40);
              write(conflictfile, "\n", 1);
              close(conflictfile);
              printf("'C %s'", ptrc->filePathClient);
            }
          }
        }
        ptrc = ptrc->next;
      } //end while loop

      if (checkifdup == 0)
      {
        //A <filepath> <server's hash> to update
        int appendfile = open("./Sample files/.update", O_WRONLY | O_APPEND | O_CREAT);
        write(appendfile, "A ", 2);
        write(appendfile, ptrs->filePathClient, strlen(ptrs->filePathClient));
        write(appendfile, " ", 1);
        write(appendfile, ptrs->hash, 40);
        write(appendfile, "\n", 1);
        close(appendfile);
        printf("'A %s'", ptrs->filePathClient); //why not printed?
      }
      //  ptrs = ptrs->next;
    } //end for loop
    if (checkforconflict == 1)
    {
      int clear = open("./Sample files/.update", O_TRUNC);
      close(clear);
    }

  } // end else // end first comparison

  // //Delete code
  FileNode *ptrc2 = headC;
  while (ptrc2 != NULL)
  {
    //printf("file name: %s\n", ptrc2->filePathClient);
    //for (ptrc = headC; ptrc != NULL; ptrc = ptrc->next){
    int checkifdupC = 0;
    FileNode *ptrs2 = headS;
    while (ptrs2 != NULL)
    {

      if (strcmp(ptrs2->filePathClient, ptrc2->filePathClient) == 0)
      {
        checkifdupC = 1;
      }
      //  printf("dup #: %d\tfile name: %s\n", checkifdupC, ptrc2->filePathClient);
      ptrs2 = ptrs2->next;
    }

    if (checkifdupC == 0)
    {
      //delete: no dup
      int deletefile = open("./Sample files/.update", O_WRONLY | O_APPEND | O_CREAT);
      write(deletefile, "D ", 2);
      write(deletefile, ptrc2->filePathClient, strlen(ptrc2->filePathClient));
      write(deletefile, " ", 1);
      write(deletefile, ptrc2->hash, 40);
      write(deletefile, "\n", 1);
      close(deletefile);
    }
    ptrc2 = ptrc2->next;
  }
}

void doCurrentVersion(int *socketPointer, char *response)
{
  getSpecialFile("./manifestfromserver", socketPointer);

  FileNode *manifestfromserverNode = newNode("./manifestfromserver");
  FileNode *headS = NULL;
  headS = readManifest(manifestfromserverNode);
  printf("\n");
  while (headS != NULL)
  {
    printf("%d\t%s\n", headS->projectVersion, headS->filePathClient);
    headS = headS->next;
  }
}