#ifndef TOKENARRAY_H
#define TOKENARRAY_H

#define MAX_TOKEN_LENGTH 50 // Define a suitable size

struct Token
{
  int lineNum;
  char lex[MAX_TOKEN_LENGTH];
  char tokName[MAX_TOKEN_LENGTH];
  int tokInt;
  char attrName[MAX_TOKEN_LENGTH];
  int attrInt;
};

// Function declarations
struct Token *create(int lineNum, char *lex, char *tokName, int tokInt, char *attrName, int attrInt);
void append(struct Token **tokArr, int *tokArrSize, int *tokArrCap, int lineNum, char *lex, char *tokName, int tokInt, char *attrName, int attrInt);
void insertN(struct Token ***tokArr, int *tokArrSize, int *tokArrCap, int idx, int lineNum, char *lex, char *tokName, int tokInt, char *attrName, int attrInt);
void deleteN(struct Token **tokArr, int *tokArrSize, int idx);
void clear(struct Token **tokArr, int *tokArrSize);
struct Token *search(struct Token *tokArr, int tokArrSize, int lineNum);
void printList(struct Token *tokArr, int tokArrSize);

#endif // TOKENARRAY_H
