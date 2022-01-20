#ifndef CLIENT_COMMIT
#define CLIENT_COMMIT

#include "fileNode.h"

void doCommitClient(void *socketPointer, char *projectName);
FileNode *buildCommit(FileNode *clientList, FileNode *serverList);
void writeCommitFile(FileNode *commitList, char *projectDir);
char *getFileHash(char *filePath);
#endif