#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>

#include "directoryFunctions.h"
#include "fileNode.h"

char *getProjectDir(char *projectName)
{
  char *projectDir = malloc(sizeof(char) * strlen(projectName) + 3); // Malloc space for project directory name
  sprintf(projectDir, "./%s", projectName);                          // Create project directory name
  return projectDir;
}

char *getVersionDir(char *projectDir, char *projectName)
{
  char *versionID = findLatestVersion(projectDir);                                                              // Get latest version number
  char *versionDir = malloc(sizeof(char) * (strlen(projectDir) + strlen(projectName) + strlen(versionID)) + 2); // Malloc space for version subdirectory name
  sprintf(versionDir, "%s/%s%s", projectDir, projectName, versionID);                                           // Create version subdirectory name
  return versionDir;
}

char *getFilePath(char *versionDir, char *fileName)
{
  char *filePath = malloc(sizeof(char) * (strlen(versionDir) + strlen(fileName)) + 2); // Malloc space for file path
  sprintf(filePath, "%s/%s", versionDir, fileName);                                    // Create file path
  return filePath;
}

char *findLatestVersion(char *projectDir)
{
  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(projectDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }
  int hasDirectory = 0;
  char *lastVersionNum = NULL;
  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }
    hasDirectory = 1;                                        // If there's stuff in the directory, set hasDirectory to true
    char *versionNum = strrchr(directoryEntry->d_name, 'v'); // get number from version subdirectory
    // printf("%s\n", versionNum);
    if (!versionNum || versionNum == directoryEntry->d_name) // If subdirecty doesn't have version number for some reason, continue
    {
      continue;
    }
    // printf("%s\n", versionNum);
    if (!lastVersionNum || atoi(versionNum + 1) > atoi(lastVersionNum + 1)) // If lastVersionNum doesn't exist, or if current version is greater than lastVersion
    {
      lastVersionNum = versionNum;
    }
  }

  if (!hasDirectory)
  {
    return "v0";
  }
  closedir(directory);
  return lastVersionNum;
}

int checkDirectory(char *path)
{
  //check if directory exists
  struct stat pathStat;
  stat(path, &pathStat);
  if (S_ISDIR(pathStat.st_mode))
  {
    return 1; // Directory exists
  }
  return 0; // Directory does not exist
}

void setFilePathServer(FileNode *fileList, char *versionDir)
{
  FileNode *aNode = fileList;
  while (aNode) // while node is not null
  {
    char *path = aNode->filePathClient;
    for (int i = 0; i < 2; i++) // Keep splitting on every '/'
    {
      path = strchr(path, '/');
      path++;
    }

    char *fullPath = malloc(strlen(versionDir) + strlen(path) + 2);
    sprintf(fullPath, "%s/%s", versionDir, path);

    aNode->filePathServer = fullPath;
    aNode = aNode->next;
  }
}

int checkVersionDirectory(char *projectDir, char *versionNum)
{
  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(projectDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }
    char *directoryVersion = strrchr(directoryEntry->d_name, 'v'); // get number from version subdirectory
    // printf("%s\n", versionNum);
    if (!directoryVersion || directoryVersion == directoryEntry->d_name) // If subdirecty doesn't have version number for some reason, continue
    {
      continue;
    }
    // printf("%s\n", versionNum);
    if (atoi(directoryVersion + 1) == atoi(versionNum))
    {
      return 1; // matching version exists
    }
  }

  closedir(directory);
  return 0; // no matching version found
}

void rollback(char *projectDir, char *versionNum)
{
  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(projectDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }
    char *directoryVersion = strrchr(directoryEntry->d_name, 'v'); // get number from version subdirectory
    // printf("%s\n", versionNum);
    if (!directoryVersion || directoryVersion == directoryEntry->d_name) // If subdirecty doesn't have version number for some reason, continue
    {
      continue;
    }
    // printf("%s\n", versionNum);
    if (atoi(directoryVersion + 1) > atoi(versionNum))
    {
      // delete folder
      char *rollbackToVersion = malloc(sizeof(char) * (strlen(projectDir) + strlen(directoryEntry->d_name) + 2));
      sprintf(rollbackToVersion, "%s/%s", projectDir, directoryEntry->d_name);
      printf("Roll back to version: %s\n", rollbackToVersion);
      deleteDirectory(rollbackToVersion);
      free(rollbackToVersion);
    }
  }
  closedir(directory);
}

void deleteDirectory(char *path)
{
  DIR *directory;                   // create directory
  struct dirent *directoryEntry;    // create directory entry
  if (!(directory = opendir(path))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }

  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }

    char *newPathName = malloc((sizeof(char) * strlen(path)) + (sizeof(char) * strlen(directoryEntry->d_name)) + 2); // +2 for '/' and '\0'
    sprintf(newPathName, "%s/%s", path, directoryEntry->d_name);

    if (checkDirectory(newPathName))
    {
      deleteDirectory(newPathName);
    }
    else
    {
      unlink(newPathName);
    }
    free(newPathName);
  }
  rmdir(path);
}

int pendingCommitNum(char *projectDir)
{
  DIR *directory;                         // create directory
  struct dirent *directoryEntry;          // create directory entry
  if (!(directory = opendir(projectDir))) // open directory
  {
    printf("Failed to open directory.\n");
    exit(EXIT_FAILURE);
  }
  int hasPending = 0;
  int commitNum = 0;

  while ((directoryEntry = readdir(directory)) != NULL) // Loop through all entries in directory
  {
    if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) // Ignore . and .. directories.
    {
      continue;
    }
    char *versionNum = strrchr(directoryEntry->d_name, 'g'); // get number from version subdirectory
    // printf("%s\n", versionNum);
    if (!versionNum || versionNum == directoryEntry->d_name || !strstr(directoryEntry->d_name, ".commitPending")) // If subdirecty doesn't have version number for some reason, continue
    {
      continue;
    }
    // printf("%s\n", versionNum);
    if (commitNum == 0 || atoi(versionNum + 1) >= commitNum) // If lastVersionNum doesn't exist, or if current version is greater than lastVersion
    {
      commitNum = atoi(versionNum + 1) + 1;
      hasPending = 1;
    }
  }

  if (!hasPending)
  {
    return 1;
  }
  closedir(directory);
  return commitNum;
}

void recursiveMakeDir(char *filePath)
{

  char *path = filePath;                                      // Pointer to filepath
  char *temp = malloc(sizeof(char) * (strlen(filePath) + 1)); // Malloc space to hold partial file path

  while ((path = strchr(path, '/')) != NULL) // Keep splitting on every '/'
  {
    memcpy(temp, filePath, path - filePath); // copy filepath up to latest split
    temp[path - filePath] = '\0';            // append null terminator
    path++;                                  // Increment pointer to avoid repeat split

    mkdir(temp, S_IRWXU); // Create directory. If already exists, nothing happens
  }
  free(temp); // Free memory
}

char *appendDir(char *projectDir, char *filePath)
{
  char *tempFilePath = filePath + 1;                                 // remove '.' from path IE: ./file.txt -> /file.txt
  char *dir = malloc(strlen(projectDir) + strlen(tempFilePath) + 1); // Malloc space for new full file path
  sprintf(dir, "%s%s", projectDir, tempFilePath);
  return dir;
}

char *getFilePathClient(char *projectName, char *fileName)
{
  char *filePathClient = malloc(strlen(projectName) + strlen(fileName) + 4);
  sprintf(filePathClient, "./%s/%s", projectName, fileName);
  return filePathClient;
}

char *getBaseFileName(char *filePathClient)
{
  char *path = filePathClient;
  for (int i = 0; i < 2; i++) // Keep splitting on every '/'
  {
    path = strchr(path, '/');
    path++;
  }
  return path;
}