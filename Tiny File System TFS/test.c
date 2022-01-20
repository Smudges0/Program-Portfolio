#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *validateKey(char *key);
int encipherText(char *function, char *key);

int main(int argc, char *argv[])
{

  // argument validation
  if (argc != 3)
  {
    printf("Usage: ./subtitution function key\n");
    exit(EXIT_FAILURE);
  }

  // key validation
  char *key = argv[2];
  char *ret_mssg = validateKey(key);
  if (ret_mssg != NULL)
  {
    printf("Error: %s\n", ret_mssg);
    exit(EXIT_FAILURE);
  }

  // key encryption/decryption
  int ret_val = encipherText(argv[1], key);
  if (ret_val == 1)
  {
    printf("Error: Invalid function. Legal functions are encrypt/decrypt.\n");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

char *validateKey(char *key)
{
  char *ret_mssg = NULL;
  int keyLen = strlen(key);

  if (keyLen != 26)
  {
    ret_mssg = "Key must be exactly 26 characters long.";
  }

  for (int i = 0; i < keyLen; i++)
  {
    for (int j = i + 1; j < keyLen; j++)
    {
      if (key[i] == key[j])
      {
        ret_mssg = "Key must not contain dulicate characters.";
      }
    }
  }

  return ret_mssg;
}

int encipherText(char *function, char *key)
{
  // set function to lowercase
  for (int i = 0; function[i]; i++)
  {
    function[i] = tolower(function[i]);
  }

  // encrypt or decrypt validation
  if (strcmp(function, "encrypt") != 0 && strcmp(function, "decrypt") != 0)
  {
    return 1;
  }

  char *old_text = NULL;
  size_t size;

  int read = getline(&old_text, &size, stdin);
  if (read != -1)
  {
    puts(old_text);
  }
  else
  {
    printf("No input.\n");
    return 1;
  }

  char alpha[] = "abcdefghijklmnopqrstuvwxyz";
  char *new_text = malloc(strlen(old_text));

  if (strcmp(function, "encrypt") == 0)
  {
    for (int i = 0; i < strlen(old_text); i++)
    {
      for (int j = 0; j < strlen(key); j++)
      {
        if (old_text[i] == alpha[j])
        {
          old_text[i] == key[j];
        }
      }
    }
  }

  if (strcmp(function, "decrypt") == 0)
  {
    for (int i = 0; i < strlen(old_text); i++)
    {
      for (int j = 0; j < strlen(key); j++)
      {
        if (old_text[i] == key[j])
        {
          old_text[i] == alpha[j];
        }
      }
    }
  }

  printf("%s\n", new_text);
  return 0;
}
