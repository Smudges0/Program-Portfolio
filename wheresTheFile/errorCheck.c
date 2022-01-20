#include <stdio.h>
#include <stdlib.h>

#include "errorCheck.h"

int errorCheck(int status, char *message);

int errorCheck(int status, char *message)
{
  if (status == -1)
  {
    perror(message);
    exit(1);
  }
  return status;
}