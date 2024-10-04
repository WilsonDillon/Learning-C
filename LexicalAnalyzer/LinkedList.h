#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 72

// Node structure
struct Node
{
  int lineNum;
  char lex[MAX_LINE_LENGTH];
  char tokStr[10];
  int tokInt;
  char attrStr[10];
  int attrInt;
  struct Node *next;
};

// Function prototypes
struct Node *createNode(int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, int attrInt);
void append(struct Node **head, int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, int attrInt);
void insert(struct Node **head, int idx, int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, int attrInt);
void delNode(struct Node **head, int idx);
void clear(struct Node **head);
struct Node *search(struct Node **head, char *lex);
int isInList(struct Node **head, char *lex);
void printList(struct Node *head);

#endif
