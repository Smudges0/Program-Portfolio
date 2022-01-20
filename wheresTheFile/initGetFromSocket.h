#ifndef INIT_GET_FROM_SOCKET
#define INIT_GET_FROM_SOCKET

#define INIT_GET(socketPointer)                                 \
  char byteBuffer;                                              \
  int clientSocket;                                             \
  if (socketPointer)                                            \
  {                                                             \
    clientSocket = *(int *)socketPointer;                       \
  }                                                             \
  int bytes_read;                                               \
  int bytesToRead = -1;                                         \
  int tokenLength = 0;                                          \
  int tokenBufSize = 10;                                        \
  char *tokenBuf = (char *)malloc(sizeof(char) * tokenBufSize); \
  if (tokenBuf == NULL && socketPointer)                        \
  {                                                             \
    printf("out of memory for tokenBuf\n");                     \
    close(clientSocket);                                        \
    exit(EXIT_FAILURE);                                         \
  }

#endif