#ifndef FILE_NODE
#define FILE_NODE

typedef struct fileNode
{
  int projectVersion;
  char *filePathServer;
  char *filePathClient;
  int fileVersion;
  char *hash;
  char *op;
  int error;
  struct fileNode *next;
} FileNode;

FileNode *newNode(char *filePath);
void appendNode(FileNode **head, FileNode *aNode);
void freeAllNodes(FileNode *head);
#endif