#ifndef CLIENT_PUSH
#define CLIENT_PUSH

#include "fileNode.h"

int doPushClient(void *socketPointer, char *projectDir);
void updateManifestPush(FileNode *manifestHead, FileNode *commitHead, char *versionDir);
void writeManifestPush(FileNode *manifestHead, char *versionDir);
#endif