#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
  int num;
  char data[10];
  struct Node *next;
};

struct Node* create(int num, char *data) {
  struct Node *newNode = malloc(sizeof(struct Node));
  newNode->num = num;
  strcpy(newNode->data, data);
  newNode->next = NULL;
  return newNode;
}

void append(struct Node **head, int num, char *data) {
  struct Node *newNode = create(num, data);
  if (*head == NULL) {
    *head = newNode;
    return;
  }
  struct Node* curr = *head;
  while (curr->next != NULL) {
    curr = curr->next;
  }
  curr->next = newNode;
}

void insert(struct Node **head, int idx, int num, char *data) {
  struct Node *newNode = create(num, data);
  if(idx < 0) {
    printf("Invalid Index (%d): %s\n", idx, "Index must be >= 0");
    return;
  }
  if(idx == 0) {
    newNode->next = *head;
    *head = newNode;
    return;
  }
  struct Node* curr = *head;
  for(int i=0; i < idx-1; i++) {
    if (curr->next == NULL) {
      printf("Invalid Index (%d): %s\n", idx, "Index must be less than the length of the list");
      return;
    }
    curr = curr->next;
  }
  struct Node *nextNode = curr->next;
  curr->next = newNode;
  newNode->next = nextNode;
}

void delete(struct Node **head, int idx) {
  struct Node* curr = *head;
  if(idx < 0) {
    printf("Invalid Index (%d): %s\n", idx, "Index must be >= 0");
    return;
  }
  if(idx == 0) {
    *head = curr->next;
    free(curr);
    return;
  }
  struct Node* prev;
  for(int i=0; i < idx; i++) {
    prev = curr;
    if (curr->next == NULL) {
      printf("Invalid Index (%d): %s\n", idx, "Index must be less than the length of the list");
      return;
    }
    curr = curr->next;
  }
  prev->next = curr->next;
  free(curr);
  return;
}

void clear(struct Node **head) {
  struct Node* curr = *head;
  while(curr != NULL) {
    struct Node* prev = curr;
    curr = curr->next;
    free(prev);
  }
  *head = NULL;
  return;
}

struct Node* search(struct Node **head, int num, char *data) {
  struct Node* curr = *head;
  while(curr != NULL) {
    if(curr->num == num && strcmp(curr->data, data) == 0) {
      return curr;
    }
    curr = curr->next;
  }
  printf("%s", "Node not found");
}

void printList(struct Node* head) {
  struct Node* curr = head;
  while (curr != NULL) {
    printf("%d: %s -> ", curr->num, curr->data);
    curr = curr->next;
  }
  printf("END\n");
}

int main() {
  printf("Linked List: \n");
  struct Node* head = NULL;

  append(&head, 1, "data1");
  append(&head, 2, "data2");
  printList(head);

  insert(&head, 0, 3, "data3");
  insert(&head, 3, 4, "data4");
  insert(&head, 8, 8, "data8");
  printList(head);

  delete(&head, 0);
  delete(&head, 2);
  delete(&head, -1);
  printList(head);

  struct Node* found2 = search(&head, 2, "data2");
  struct Node* found1 = search(&head, 1, "data1");
  printf("%d: %s\n", found2->num, found2->data);
  printf("%d: %s\n", found1->num, found1->data);

  clear(&head);
  printList(head);

  return 0;
}
