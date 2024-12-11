#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 72

union Attr
{
  int attrInt;
  void *symPtr;
};

// Node structure
struct Node
{
  int lineNum;
  char lex[MAX_LINE_LENGTH];
  char tokStr[10];
  int tokInt;
  char attrStr[MAX_LINE_LENGTH];
  union Attr attr;
  struct Node *next;
};

// Function prototypes
struct Node *createNode(int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, union Attr attr);
void append(struct Node **head, int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, union Attr attr);
void insert(struct Node **head, int idx, int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, union Attr attr);
void delNode(struct Node **head, int idx);
void clear(struct Node **head);
struct Node *search(struct Node **head, char *lex);
int isInList(struct Node **head, char *lex);
void printList(struct Node *head);

#endif
