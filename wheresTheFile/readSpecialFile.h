#ifndef READ_SPECIAL_FILE
#define READ_SPECIAL_FILE

#include "fileNode.h"

int readInt(int fileDescriptor);
char *readString(int fileDescriptor);
FileNode *readCommit(FileNode *commitNode);
FileNode *readManifest(FileNode *manifestNode);
#endif