#ifndef SEND_TO_SOCKET
#define SEND_TO_SOCKET

#include "fileNode.h"

void sendNumBytes(FileNode *fileNode, void *socketPointer);
void sendFilePath(FileNode *fileNode, void *socketPointer);
void sendFileData(FileNode *fileNode, void *socketPointer);
void sendSourceFiles(FileNode *fileList, void *socketPointer);
void sendSpecialFile(FileNode *fileNode, void *socketPointer);

#endif