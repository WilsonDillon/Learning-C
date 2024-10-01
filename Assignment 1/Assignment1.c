#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 100

char input[30];
FILE *oldFile;
FILE *newFile;
char line[MAX_LINE_LENGTH];

int main () {

  printf("Enter the file name: \n");
  scanf("%29s", input);
  printf("FileName: %s\n\n", input);

  oldFile = fopen(input, "r");
  newFile = fopen("newFile.txt", "w");

  if(oldFile == NULL)
    return 1; // can't be opened

  int i = 1;
  while(fgets(line, MAX_LINE_LENGTH, oldFile)){

    if (!strcmp(line, "\n")) {
      continue;
    }

    fprintf(newFile, "%d  %s\n", i, line);
    i++;
  }

  //release memory
  fclose(oldFile); 
  fclose(newFile); 

  return 0;
};
