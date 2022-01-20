#ifndef DIRECTORY_FUNCTIONS
#define DIRECTORY_FUNCTIONS

#include "fileNode.h"

char *getProjectDir(char *projectName);
char *getVersionDir(char *projectDir, char *projectName);
char *getFilePath(char *versionDir, char *fileName);
char *findLatestVersion(char *projectDir);
int checkDirectory(char *path);
void setFilePathServer(FileNode *fileList, char *versionDir);
int checkVersionDirectory(char *projectDir, char *versionNum);
void rollback(char *projectDir, char *versionNum);
void deleteDirectory(char *path);
int pendingCommitNum(char *projectDir);
void recursiveMakeDir(char *filePath);
char *appendDir(char *projectDir, char *filePath);
char *getFilePathClient(char *projectName, char *fileName);
char *getBaseFileName(char *filePathClient);
#endif