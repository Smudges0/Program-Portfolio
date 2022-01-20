#ifndef GET_FROM_SOCKET
#define GET_FROM_SOCKET

int getBytesToRead(void *socketPointer);
char *getFileName(int numBytes, void *socketPointer);
void writeFileFromSocket(char *fileName, int bytesToRead, void *socketPointer);
char *getProjectName(void *socketPointer);
void getSpecialFile(char *fileName, void *socketPointer);
void getSourceFiles(void *socketPointer, char *versionDir);
char *getVersionFromSocket(void *socketPointer);
void clearRecvBuffer(void *socketPointer);
#endif