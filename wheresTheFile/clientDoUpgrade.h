#ifndef CLIENT_UPGRADE
#define CLIENT_UPGRADE

#include "fileNode.h"

void doUpgrade(void *socketPointer, char *projectName);
void updateManifestUpgrade(FileNode *manifestList, FileNode *updateList, char *projectDir, void *socketPointer);
void writeManifestUpgrade(FileNode *manifestList, char *projectDir);
#endif