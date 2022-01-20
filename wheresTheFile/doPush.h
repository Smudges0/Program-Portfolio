#ifndef DO_PUSH
#define DO_PUSH

#include "fileNode.h"

void doPush(void *socketPointer);
int diffFiles(char *projectDir, char *filePath);
int compareFiles(char *pending, char *received);
void expirePending(char *projectDir);
void copyFiles(char *sourceDir, char *targetDir);
void duplicate(char *source, char *target);
void updateManifest(FileNode *manifestHead, FileNode *commitHead, char *versionDir);
void deleteFile(FileNode *aNode, char *versionDir);
void writeManifest(FileNode *manifestList, char *versionDir);
void updateHistory(char *projectDir, char *commit);
#endif