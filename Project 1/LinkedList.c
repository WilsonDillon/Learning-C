#include "LinkedList.h"

struct Node *createNode(int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, union Attr attr)
{
  struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
  newNode->lineNum = lineNum;
  strncpy(newNode->lex, lex, MAX_LINE_LENGTH - 1);
  strncpy(newNode->tokStr, tokStr, 9);
  newNode->tokInt = tokInt;
  strncpy(newNode->attrStr, attrStr, 9);
  newNode->attr = attr;
  newNode->next = NULL;
  return newNode;
}

void append(struct Node **head, int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, union Attr attr)
{
  struct Node *newNode = createNode(lineNum, lex, tokStr, tokInt, attrStr, attr);
  if (*head == NULL)
  {
    *head = newNode;
    return;
  }
  struct Node *curr = *head;
  while (curr->next != NULL)
  {
    curr = curr->next;
  }
  curr->next = newNode;
}

void insert(struct Node **head, int idx, int lineNum, char *lex, char *tokStr, int tokInt, char *attrStr, union Attr attr)
{
  struct Node *newNode = createNode(lineNum, lex, tokStr, tokInt, attrStr, attr);
  if (idx < 0)
  {
    printf("Invalid Index: %d\n", idx);
    return;
  }
  if (idx == 0)
  {
    newNode->next = *head;
    *head = newNode;
    return;
  }
  struct Node *curr = *head;
  for (int i = 0; i < idx - 1; i++)
  {
    if (curr->next == NULL)
    {
      printf("Invalid Index: %d\n", idx);
      return;
    }
    curr = curr->next;
  }
  newNode->next = curr->next;
  curr->next = newNode;
}

void delNode(struct Node **head, int idx)
{
  if (*head == NULL || idx < 0)
  {
    printf("Invalid Index: %d\n", idx);
    return;
  }
  struct Node *temp = *head;
  if (idx == 0)
  {
    *head = temp->next;
    free(temp);
    return;
  }
  struct Node *prev;
  for (int i = 0; i < idx; i++)
  {
    prev = temp;
    temp = temp->next;
    if (temp == NULL)
    {
      printf("Invalid Index: %d\n", idx);
      return;
    }
  }
  prev->next = temp->next;
  free(temp);
}

void clear(struct Node **head)
{
  struct Node *curr = *head;
  while (curr != NULL)
  {
    struct Node *nextNode = curr->next;

    if (curr->attr.symPtr != NULL)
    {
      free(curr->attr.symPtr);
      curr->attr.symPtr = NULL;
    }

    free(curr);
    curr = nextNode;
  }
  *head = NULL;
}

struct Node *search(struct Node **head, char *lex)
{
  struct Node *curr = *head;
  while (curr != NULL)
  {
    if (strcmp(curr->lex, lex) == 0)
    {
      return curr;
    }
    curr = curr->next;
  }
  // printf("Node not found\n");
  return NULL;
}

int isInList(struct Node **head, char *lex)
{
  struct Node *foundNode = search(head, lex);
  return (foundNode != NULL) ? 0 : 1;
}

void printList(struct Node *head)
{
  struct Node *curr = head;
  printf("Symbol Table (Linked List): \n");
  while (curr != NULL)
  {
    printf("%d %s %s %d %s %p ->\n", curr->lineNum, curr->lex, curr->tokStr, curr->tokInt, curr->attrStr, curr->attr.symPtr);
    curr = curr->next;
  }
  printf("END\n");
}
