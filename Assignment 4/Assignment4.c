#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Token {
  char lex[10];
  char tokStr[10];
  int tokInt;
  char attStr[10];
  int attInt;
};

int main() {
  FILE *file = fopen("ReservedWords.txt", "r");
  if (file == NULL) {
    return 1;
  }

  struct Token tokenArr[20];
  char line[50];
  
  int i = 0;
  while (fgets(line, sizeof(line), file) != NULL) {
    sscanf(line, "%s %s %d %s %d", tokenArr[i].lex, tokenArr[i].tokStr, &(tokenArr[i].tokInt), tokenArr[i].attStr, &(tokenArr[i].attInt));
    i++;
  }
  
  fclose(file);
  return 0;
}
